/** TRACCC library, part of the ACTS project (R&D line)
 *
 * (c) 2021 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#include "edm/cell.hpp"
#include "edm/cluster.hpp"
#include "edm/measurement.hpp"
#include "edm/spacepoint.hpp"
#include "geometry/pixel_segmentation.hpp"
#include "algorithms/component_connection.hpp"
#include "algorithms/measurement_creation.hpp"
#include "algorithms/spacepoint_formation.hpp"
#include "csv/csv_io.hpp"
#include "binary/binary_io.hpp"

#include <vecmem/memory/host_memory_resource.hpp>

#include <iostream>

int seq_run(const std::string& detector_file, const std::string& cells_dir, unsigned int events)
{
    auto env_d_d = std::getenv("TRACCC_TEST_DATA_DIR");
    if (env_d_d == nullptr)
    {
        throw std::ios_base::failure("Test data directory not found. Please set TRACCC_TEST_DATA_DIR.");
    }
    auto data_directory = std::string(env_d_d) + std::string("/");

    // Read the surface transforms
    std::string io_detector_file = data_directory + detector_file;
    traccc::surface_reader sreader(io_detector_file, {"geometry_id", "cx", "cy", "cz", "rot_xu", "rot_xv", "rot_xw", "rot_zu", "rot_zv", "rot_zw"});
    auto surface_transforms = traccc::read_surfaces(sreader);

    // Algorithms
    traccc::component_connection cc;
    traccc::measurement_creation mt;
    traccc::spacepoint_formation sp;

    // Output stats
    uint64_t n_cells = 0;
    uint64_t m_modules = 0;
    uint64_t n_clusters = 0;
    uint64_t n_measurements = 0;
    uint64_t n_space_points = 0;

    // Memory resource used by the EDM.
    vecmem::host_memory_resource resource;

    // Loop over events
    for (unsigned int event = 0; event < events; ++event){

        // Read the cells from the relevant event file
        std::string event_string = "000000000";
        std::string event_number = std::to_string(event);
        event_string.replace(event_string.size()-event_number.size(), event_number.size(), event_number);

        std::string io_cells_file = data_directory+cells_dir+std::string("/event")+event_string+std::string("-cells.csv");
        traccc::cell_reader creader(io_cells_file, {"geometry_id", "hit_id", "cannel0", "channel1", "activation", "time"});
        traccc::host_cell_container cells_per_event = traccc::read_cells(creader, resource, surface_transforms);
        m_modules += cells_per_event.headers.size();
        
        std::string event_fpath = "/home/sylvain/Desktop/StageM2/traccc/data_bin/event"+std::to_string(event)+".bin";

        bool success;
        
        success = traccc::binio::write_cells(cells_per_event, event, event_fpath);

        if (! success) {
            std::cout << "ERROR WRITE CELLS for event " + std::to_string(event) + "\n";
        }
        std::cout << "Event " + std::to_string(event) + " written..." << std::endl;

        traccc::host_cell_container cells_per_event_verif;

        std::cout << "Event " + std::to_string(event) + " reading cells..." << std::endl;

        success = traccc::binio::read_cells(event_fpath, cells_per_event_verif);

        if (! success) {
            std::cout << "ERROR READ CELLS for event " + std::to_string(event) + "\n";
        }
        std::cout << "Event " + std::to_string(event) + " read..." << std::endl;

        std::string msg;

        success = traccc::binio::check_results(cells_per_event, cells_per_event_verif, msg);


        if (! success) {
            std::cout << "ERROR CHECK RESULT for event " + std::to_string(event) + "\n";
            std::cout << "   message =  " + msg + "\n";

        }

        std::cout << "Event " + std::to_string(event) + " all checks passed !" << std::endl;



        /*int module_count = cells_per_event.headers.size();

        std::ofstream wf("/home/sylvain/Desktop/StageM2/traccc/data_bin/event"+std::to_string(event)+".bin", std::ios::out | std::ios::binary);

        // write module count
        wf.write((char *)(&module_count), sizeof(int));

        for (std::size_t i = 0; i < cells_per_event.items.size(); ++i ) {
	        traccc::cell_module &module = cells_per_event.headers[i]; // traccc::cell_module
            auto& cells  = cells_per_event.items[i]; // vecmem::vector<traccc::cell>

            // write module struct
            wf.write((char *)(&module), sizeof(module));

            // write number of cells
            int cell_count = cells.size();
            wf.write((char *)(&cell_count), sizeof(int));

            // write cells
            for (std::size_t ic = 0; ic < cell_count; ++ic) {
                traccc::cell cell = cells[ic];
                wf.write((char *)(&cell), sizeof(traccc::cell));
            }
        }

        wf.close();
        if(!wf.good()) {
            std::cout << "Error occurred at writing time!" << std::endl;
            return 1;
        }*/
    }



    std::cout << "==> Wote everything ! " << std::endl;

    return 0;
}

// The main routine
//
int main(int argc, char *argv[])
{
    if (argc < 4){
        std::cout << "Not enough arguments, minimum requirement: " << std::endl;
        std::cout << "./seq_example <detector_file> <cell_directory> <events>" << std::endl;
        return -1;
    }

    auto detector_file = std::string(argv[1]);
    auto cell_directory = std::string(argv[2]);
    auto events = std::atoi(argv[3]);

    std::cout << "Running ./seq_exammple " << detector_file << " " << cell_directory << " " << events << std::endl;
    return seq_run(detector_file, cell_directory, events);
}
