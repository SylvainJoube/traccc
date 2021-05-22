/** TRACCC library, part of the ACTS project (R&D line)
 * 
 * (c) 2021 CERN for the benefit of the ACTS project
 * 
 * Mozilla Public License Version 2.0
 * 
 * WIP by Sylvain Joube - joube@ijclab.in2p3.fr - github.com/SylvainJoube
 * 
 * Tested with dpcpp ver. 2021.1.1 on an Intel UHD Graphics 620.
 * 
 * That still is very WIP... And I will have to update from the last traccc repo.
 */

#include "edm/cell.hpp"
#include "edm/sycl_gpu_module.hpp"
#include "edm/cluster.hpp"
#include "edm/measurement.hpp"
#include "edm/spacepoint.hpp"
#include "geometry/pixel_segmentation.hpp"
#include "algorithms/component_connection.hpp"
#include "algorithms/measurement_creation.hpp"
#include "algorithms/spacepoint_formation.hpp"
#include "csv/csv_io.hpp"
#include <iomanip>
#include <stdlib.h>
#include <sstream>

#include <vecmem/memory/host_memory_resource.hpp>

#include <iostream>

// SyCL specific includes
#include <CL/sycl.hpp>
#include <array>
#include <sys/time.h>
#include "algorithms/detail/sparse_ccl_sycl.hpp"

#define DO_BENCHMARK true
#define _DEBUG true

// SyCL asynchronous exception handler
// Create an exception handler for asynchronous SYCL exceptions
static auto exception_handler = [](cl::sycl::exception_list e_list) {
  for (std::exception_ptr const &e : e_list) {
    try {
      std::rethrow_exception(e);
    }
    catch (std::exception const &e) {
#if _DEBUG
      std::cout << "Failure" << std::endl;
#endif
      std::terminate();
    }
  }
};


uint64_t get_ms() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    uint64_t ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    return ms;
}

void log(std::string str) {
    std::cout << str << std::endl;
}

int seq_run(const std::string& detector_file, const std::string& cells_dir, unsigned int events)
{
    std::cout << "Version SYCL buffers / accesseurs\n";
    auto env_d_d = std::getenv("TRACCC_TEST_DATA_DIR");
    if (env_d_d == nullptr)
    {
        throw std::ios_base::failure("Test data directory not found. Please set TRACCC_TEST_DATA_DIR.");
    }
    auto data_directory = std::string(env_d_d);

    // Read the surface transforms
    std::string io_detector_file = data_directory + detector_file;
    traccc::surface_reader sreader(io_detector_file, {"geometry_id", "cx", "cy", "cz", "rot_xu", "rot_xv", "rot_xw", "rot_zu", "rot_zv", "rot_zw"});
    auto surface_transforms = traccc::read_surfaces(sreader);

    // Algorithms
    traccc::component_connection cc; // CCL
    traccc::measurement_creation mt; // CCA
    traccc::spacepoint_formation sp; // 3D

    // Output stats
    uint64_t n_cells = 0;
    uint64_t m_modules = 0;
    uint64_t n_clusters = 0;
    uint64_t n_measurements = 0;
    uint64_t n_space_points = 0;

    // Memory resource used by the EDM.
    vecmem::host_memory_resource resource;

    // Used for verification
    uint sycl_cluster_verification_count = 0;
    uint sycl_cluster_error_count = 0;

    // Used for duration info
    uint64_t t_start, t_start2;
    uint64_t t_cpu = 0; // cpu-land
    uint64_t t_gpu = 0; // sycl-land
    uint64_t t_linearization = 0; // transformation into 1D array
    uint64_t t_allocation = 0;
    uint64_t t_copy_to_device = 0;
    uint64_t t_parallel_for = 0;
    uint64_t t_read_from_device = 0;
    uint64_t t_sum_clusters_from_device = 0;
    uint64_t t_free_all = 0;
    uint64_t t_free_gpu = 0;
    uint64_t t_free_cpu = 0;

    uint max_cell_count_per_module_global = 0;


    // SyCL test code
    // The default device selector will select the most performant device.
    cl::sycl::default_selector d_selector;

    try {
        cl::sycl::queue sycl_q(d_selector, exception_handler);

        // Print out the device information used for the kernel code.
        std::cout << "Running on device: "
                << sycl_q.get_device().get_info<cl::sycl::info::device::name>() << "\n";
        
        log("Processing events...");

        // Loop over events
        for (unsigned int event = 0; event < events; ++event){
            log("\n===== Processing event " + std::to_string(event) + " =====");

            // Read the cells from the relevant event file
            std::string event_string = "000000000";
            std::string event_number = std::to_string(event);
            event_string.replace(event_string.size()-event_number.size(), event_number.size(), event_number);


            std::string io_cells_file = data_directory+cells_dir+std::string("/event")+event_string+std::string("-cells.csv");
            traccc::cell_reader creader(io_cells_file, {"geometry_id", "hit_id", "cannel0", "channel1", "activation", "time"});
            traccc::host_cell_container cells_per_event = traccc::read_cells(creader, resource, surface_transforms);
            
            // Number of modules from this event
            uint module_count = cells_per_event.headers.size();

            //log("S - I think header size(" + std::to_string(module_count) + ") should be equal to "
            //+ "items size(" + std::to_string(cells_per_event.items.size()) + ")");
            // a vector of modules
            // and a jagged vector of cells
            
            m_modules += module_count;

            // Output containers
            traccc::host_measurement_container measurements_per_event;
            traccc::host_spacepoint_container spacepoints_per_event;

            measurements_per_event.headers.reserve(module_count);
            measurements_per_event.items.reserve(module_count);
            spacepoints_per_event.headers.reserve(module_count);
            spacepoints_per_event.items.reserve(module_count);

            std::cout << "[event " << event << "] module_count = " << module_count << std::endl;
            
            t_start = get_ms();

            // ================== Explicit USM, with a single large buffer ==================

            // ==== 1D array creation from modules and cells (RAM) ====

            // Module controllers allocation (input / output)
            traccc::sycl::ccl::input_module_ctrl *ctrl_input_array = new traccc::sycl::ccl::input_module_ctrl[module_count];
            traccc::sycl::ccl::output_module_ctrl *ctrl_output_array = new traccc::sycl::ccl::output_module_ctrl[module_count];
            uint current_cell_global_index = 0;
            uint current_module_index = 0;

            uint total_cell_count = 0;

            for (std::size_t i = 0; i < cells_per_event.items.size(); ++i ) {
                //traccc::cell_module& module = cells_per_event.headers[i];
                total_cell_count += cells_per_event.items[i].size();
            }

            // Allocation of the main arrays of cells
            traccc::sycl::ccl::input_cell *input_cell_array = new traccc::sycl::ccl::input_cell[total_cell_count];
            traccc::sycl::ccl::output_cell *output_cell_array = new traccc::sycl::ccl::output_cell[total_cell_count];

            uint max_cell_count_per_module_t = 0;
            // Filling the modules and cells
            // For each module...
            vecmem::jagged_vector<traccc::cell> &cells_per_module_jagged = cells_per_event.items;

            for (std::size_t i = 0; i < cells_per_module_jagged.size(); ++i) {
                vecmem::vector<traccc::cell> &cells_per_module = cells_per_module_jagged[i];
                
                // initialize the module
                ctrl_input_array[current_module_index].cell_count = cells_per_module.size();
                ctrl_input_array[current_module_index].cell_start_index = current_cell_global_index;
                ctrl_output_array[current_module_index].cluster_count = 0; // <- will be removed
                // save the cells to the 1D array
                for (auto &cell : cells_per_module) {
                    input_cell_array[current_cell_global_index].cell = cell;
                    ++current_cell_global_index;
                }
                if (max_cell_count_per_module_t < cells_per_module.size()) {
                    max_cell_count_per_module_t = cells_per_module.size();
                }
                ++current_module_index;
            }

            // Ok, that's actually VERY VERY DIRTY, I'm sorry fot that...
            const uint max_cell_count_per_module = 1000; //max_cell_count_per_module_t;

            if (max_cell_count_per_module_t > max_cell_count_per_module_global)
                max_cell_count_per_module_global = max_cell_count_per_module_t;

            // For event 1 to 10, that was about 991 cells.
            //log("max_cell_count_per_module = " + std::to_string(max_cell_count_per_module));

            sycl_q.wait_and_throw();
            t_linearization = get_ms() - t_start;

            // ==== malloc_device part ====

            t_start = get_ms();

            // ctrl structs
            traccc::sycl::ccl::input_module_ctrl *device_ctrl_input_array =
                static_cast<traccc::sycl::ccl::input_module_ctrl *>
                (cl::sycl::malloc_device(module_count * sizeof(traccc::sycl::ccl::input_module_ctrl), sycl_q));

            traccc::sycl::ccl::output_module_ctrl *device_ctrl_output_array =
                static_cast<traccc::sycl::ccl::output_module_ctrl *>
                (cl::sycl::malloc_device(module_count * sizeof(traccc::sycl::ccl::output_module_ctrl), sycl_q));

            // cells structs
            traccc::sycl::ccl::input_cell *device_input_cell_array =
                static_cast<traccc::sycl::ccl::input_cell *>
                (cl::sycl::malloc_device(total_cell_count * sizeof(traccc::sycl::ccl::input_cell), sycl_q));

            traccc::sycl::ccl::output_cell *device_output_cell_array =
                static_cast<traccc::sycl::ccl::output_cell *>
                (cl::sycl::malloc_device(total_cell_count * sizeof(traccc::sycl::ccl::output_cell), sycl_q));

            sycl_q.wait_and_throw();
            t_allocation = get_ms() - t_start;
            t_start = get_ms();

            // ==== data copy to device ====

            sycl_q.memcpy(device_ctrl_input_array, ctrl_input_array, module_count * sizeof(traccc::sycl::ccl::input_module_ctrl));
            sycl_q.memcpy(device_input_cell_array, input_cell_array, total_cell_count * sizeof(traccc::sycl::ccl::input_cell));
            // no copy for write only data

            sycl_q.wait_and_throw();
            t_copy_to_device = get_ms() - t_start;

            // Module data should be around 30 872 bytes for event 1
            // Cells data should be around 3 192 752 bytes for event 1
            log("  ==> Data sent to device, in bytes :");
            log("      Modules   = "
                + std::to_string(module_count * sizeof(traccc::sycl::ccl::input_module_ctrl)));
            log("      Cells     = "
                + std::to_string(total_cell_count * sizeof(traccc::sycl::ccl::input_cell)));


            log("  <== Data retrieved from device, in bytes :");
            log("      Cluster_count   = "
                + std::to_string(module_count * sizeof(traccc::sycl::ccl::output_module_ctrl)));
            log("      Labels          = "
                + std::to_string(total_cell_count * sizeof(traccc::sycl::ccl::output_cell)));

            t_start = get_ms();

            // ==== parallel for ====

            uint rep = module_count;
            sycl_q.parallel_for(cl::sycl::range<1>(rep), [=](cl::sycl::id<1> module_indexx) {

                uint module_index = module_indexx[0] % module_count;
                // ---- SparseCCL part ----
                // Internal list linking
                uint first_cindex = device_ctrl_input_array[module_index].cell_start_index;
                uint cell_count = device_ctrl_input_array[module_index].cell_count;
                uint cell_index = first_cindex;
                uint stop_cindex = first_cindex + cell_count;
                
                // The very dirty part : statically allocate a buffer of the maximum pixel density per module...
                uint L[max_cell_count_per_module];

                for (uint i = first_cindex; i < stop_cindex; ++i) {
                    device_output_cell_array[i].label = 0;
                }

                unsigned int start_j = 0;
                for (unsigned int i=0; i < cell_count; ++i){
                    L[i] = i;
                    int ai = i;
                    if (i > 0){
                        const traccc::sycl::ccl::input_cell &ci = device_input_cell_array[i + first_cindex];
                        for (unsigned int j = start_j; j < i; ++j){
                            const traccc::sycl::ccl::input_cell &cj = device_input_cell_array[j + first_cindex];
                            if (traccc::detail::is_adjacent(ci.cell, cj.cell)){
                                ai = traccc::detail::make_union(L, ai, traccc::detail::find_root(L, j));
                            } else if (traccc::detail::is_far_enough(ci.cell, cj.cell)){
                                ++start_j;
                            }
                        }
                    }
                }

                // second scan: transitive closure
                uint labels = 0;
                for (unsigned int i = 0; i < cell_count; ++i){
                    unsigned int l = 0;
                    if (L[i] == i){
                        ++labels;
                        l = labels; 
                    } else {
                        l = L[L[i]];
                    }
                    L[i] = l;
                }

                // Update the output values
                for (unsigned int i = 0; i < cell_count; ++i){
                    device_output_cell_array[i + first_cindex].label = L[i];
                }
                device_ctrl_output_array[module_index].cluster_count = labels;
            });

            sycl_q.wait_and_throw();

            t_parallel_for = get_ms() - t_start;
            t_start = get_ms();

            // Reading data from device
            sycl_q.memcpy(ctrl_output_array, device_ctrl_output_array, module_count * sizeof(traccc::sycl::ccl::output_module_ctrl));
            sycl_q.memcpy(output_cell_array, device_output_cell_array, total_cell_count * sizeof(traccc::sycl::ccl::output_cell));

            sycl_q.wait_and_throw();

            t_read_from_device = get_ms() - t_start;
            t_start = get_ms();


            uint total_cluster_count = 0;
            for (int i = 0; i < module_count; ++i) {
                total_cluster_count += ctrl_output_array[i].cluster_count;
            }
            t_sum_clusters_from_device = get_ms() - t_start;
            t_start = get_ms();

            uint total_cluster_count_chk = 0;
            for (std::size_t i = 0; i < cells_per_module_jagged.size(); ++i) {
                // cells_per_module_jagged[i] is equal to cells_per_event.items[i]
                vecmem::vector<traccc::cell> &cells_per_module = cells_per_module_jagged[i];
                traccc::cell_module &module_info = cells_per_event.headers[i];
                traccc::cluster_collection clusters_per_module_cpu_verif = cc(cells_per_module, module_info);
                total_cluster_count_chk += clusters_per_module_cpu_verif.items.size();
            }
            t_cpu = get_ms() - t_start;

            sycl_cluster_verification_count += total_cluster_count_chk;

            // Only checks the culster count, not the labels (I will do that in the future)

            if (total_cluster_count_chk == total_cluster_count) {
                log("OK Seems to have worked, same number of clusters on GPU and CPU ! ("
                + std::to_string(total_cluster_count) + ")");
            } else {
                log("!!!!!-ERROR-!!!!! : cluster number does not match between CPU and GPU.");
                log("Cluster count at event " + std::to_string(event) + " = " + std::to_string(total_cluster_count)
                + " should be " + std::to_string(total_cluster_count_chk)
                + "  total_cell_count = " + std::to_string(total_cell_count));
                sycl_cluster_error_count += std::abs(static_cast<int>(total_cluster_count_chk) - static_cast<int>(total_cluster_count));
            }

            t_start = get_ms();

            cl::sycl::free(device_ctrl_input_array, sycl_q);
            cl::sycl::free(device_ctrl_output_array, sycl_q);
            cl::sycl::free(device_input_cell_array, sycl_q);
            cl::sycl::free(device_output_cell_array, sycl_q);

            t_free_gpu = get_ms() - t_start;
            t_start = get_ms();

            free(ctrl_input_array);
            free(ctrl_output_array);
            free(input_cell_array);
            free(output_cell_array);

            t_free_cpu = get_ms() - t_start;
            t_start = get_ms();




        t_gpu = t_allocation + t_copy_to_device + t_parallel_for + t_read_from_device;
        std::cout << "t_cpu              = " << t_cpu << std::endl
                << "t_gpu              = " << t_gpu << std::endl
                << "t_linearization    = " << t_linearization << std::endl
                << "t_allocation       = " << t_allocation << std::endl
                << "t_copy_to_device   = " << t_copy_to_device << std::endl
                << "t_parallel_for     = " << t_parallel_for << std::endl
                << "t_read_from_device = " << t_read_from_device << std::endl
                << "t_sum_clusters_from_device = " << t_sum_clusters_from_device << std::endl
                << "t_free_gpu         = " << t_free_gpu << std::endl
                << "t_free_cpu         = " << t_free_cpu << std::endl
                << std::endl;
        }


    } catch (cl::sycl::exception const &e) {
        std::cout << "An exception is caught while processing SyCL code.\n";
        std::terminate();
    }

    std::cout << "SYCL sycl_cluster_verification_count(" << sycl_cluster_verification_count << ")  "
              << "sycl_cluster_error_count(" << sycl_cluster_error_count << ")" << std::endl;


    log("-- max_cell_count_per_module_global = " + std::to_string(max_cell_count_per_module_global) + " --");

    /*if (DO_BENCHMARK) {
        
        t_gpu = t_allocation + t_copy_to_device + t_parallel_for + t_read_from_device;

        std::cout << "t_cpu              = " << t_cpu << std::endl
                  << "t_gpu              = " << t_gpu << std::endl
                  << "t_linearization    = " << t_linearization << std::endl
                  << "t_allocation       = " << t_allocation << std::endl
                  << "t_copy_to_device   = " << t_copy_to_device << std::endl
                  << "t_parallel_for     = " << t_parallel_for << std::endl
                  << "t_read_from_device = " << t_read_from_device << std::endl
                  << "t_sum_clusters_from_device = " << t_sum_clusters_from_device << std::endl
                  << "t_free_gpu         = " << t_free_gpu << std::endl
                  << "t_free_cpu         = " << t_free_cpu << std::endl
                  << std::endl;

    }*/
    return 0;
}

// The main routine
//
int main(int argc, char *argv[])
{
    if (argc < 4){
        std::cout << "Not enough arguments, minimum requirement: " << std::endl;
        std::cout << "./sycl_ccl_usm_explicit_global <detector_file> <cell_directory> <event_count>" << std::endl;
        return -1;
    }

    std::cout << "Modified version  - SYCL explicit global USM - v1" << std::endl;
    // Usage example : 
    // export TRACCC_TEST_DATA_DIR=<absolute_path_to_traccc>/data/
    // ./sycl_ccl_usm_explicit_global tml_detector/trackml-detector.csv tml_pixels 1
    // located in the build/bin directory.

    auto detector_file = std::string(argv[1]);
    auto cell_directory = std::string(argv[2]);
    auto events = std::atoi(argv[3]);

    std::cout << "Running ./sycl_ccl_usm_explicit_global " << detector_file << " " << cell_directory << " " << events << std::endl;
    
    return seq_run(detector_file, cell_directory, events);
}
