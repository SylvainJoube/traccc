/** TRACCC library, part of the ACTS project (R&D line)
 * 
 * (c) 2021 CERN for the benefit of the ACTS project
 * 
 * Mozilla Public License Version 2.0
 */

/** Inspired by traccc/examples/cpu/seq_example.cpp
 *  I made this file to test whether or not the SparseCCL implemantation
 *  is broken.
 *  As far as I can tell with my naive recursive implementation of CCL,
 *  I do not think SpaseCCL is making any wrong evaluation on the cluster number.
 *  But again, I might be wrong, and I did not spend more than two days on the
 *  matter.
 * 
 *  Please do not hesitate to contact me on GitHub (SylvainJoube)
 *  or by email : sylvain.joube@ijclab.in2p3.fr
 *  or on the traccc mattermost : sylvain.joube
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

#include <iostream>

/** A simple way to run the test :
 *  export TRACCC_TEST_DATA_DIR=<absolute-path-to-traccc>/data/
 * 
 *  The sparse_ccl_debug executable should be in your build/bin/ directory
 *  like traccc/build/bin/sparse_ccl_debug
 *  
 *  param1 <detector_file>  : csv file to use
 *  param2 <cell_directory> : event prefix file name
 *  param3 <events>         : number of events (i.e. separate csv files)
 *  param4 <path_to_python_generated_data> 
 *                          : full path to a compatible CSV file (see data_generation.py)
 *                            you can put an invalid path to disable this check
 *  
 *  Full example : 
 *  ./sparse_ccl_debug tml_detector/trackml-detector.csv tml_pixels 10 <path_to_python_generated_data or invalid value>
 */


// Naive, simple recursive function checking the cluster number found
// by SparseCCL.
// So far, I don't find anything odd, but keep in mind my code might be broken,
// and my input/generated data not testing every confuguration.

namespace traccc::chkccl {


    // =========================================
    // ============ DATA GENERATION ============
    // =========================================


    // A bad nasty horrible hard-coded module dimension
    // (I saw that every module in trackml-detector had those dimensions)
    uint width  = 336;
    uint height = 1280;

    // Grid (pixels) for a given module
    struct mgrid {
        bool **present = nullptr; //[width][height];
        int w = 0;
        int h = 0;
    };

    void free_mgrid(mgrid &coll); // forward

    void init_mgrid(mgrid &coll) {
        if (coll.w != width || coll.h != height || coll.present == nullptr) {
            free_mgrid(coll);
            coll.present = new bool*[width];
            for (int x = 0; x < width; ++x) {
                coll.present[x] = new bool[height];
            }
            coll.w = width;
            coll.h = height;
        }
        
        for (uint x = 0; x < width; ++x) for (uint y = 0; y < height; ++y) {
            coll.present[x][y] = 0;
        }
    }

    void free_mgrid(mgrid &coll) {
        if (coll.w == 0 || coll.h == 0) return;
        if (coll.present != nullptr) {
            for (int x = 0; x < coll.w; ++x) {
                delete coll.present[x];
            }
            delete coll.present;
            coll.present = nullptr;
        }
        coll.w = 0;
        coll.h = 0;
    }

    /// Get a cell_collection from an mgrid
    traccc::cell_collection get_cell_collection(mgrid &coll) {
        traccc::cell_collection c;
        for (uint y = 0; y < height; ++y) for (uint x = 0; x < width; ++x) {
            if (coll.present[x][y]) {
                traccc::cell cell;
                cell.activation = 1;
                cell.channel0 = x;
                cell.channel1 = y;
                cell.time = 0;
                c.items.push_back(cell);
            }
        }
        return c;
    }

    /// Monkey-test : generate random pixels wth a given probability
    traccc::cell_collection generate_full_random(mgrid &coll, uint proba1000000) {
        int pixel_count = 0;
        init_mgrid(coll);
        for (uint x = 0; x < width; ++x) for (uint y = 0; y < height; ++y) {
            uint number = rand() % 1000000;
            if (number < proba1000000) {
                coll.present[x][y] = true;
                ++pixel_count;
            }
        }
        //std::cout << " pixel_count = " << pixel_count << "\n";
        return get_cell_collection(coll);
    }

    /// A more coherent, dense area of pixels
    traccc::cell_collection generate_area(mgrid &coll, uint cluster_proba1000000, uint cell_proba1000, uint cluster_width, uint cluster_height) {
        init_mgrid(coll);
        for (uint x = 0; x < width; ++x) for (uint y = 0; y < height; ++y) {
            uint number = rand() % 1000000;
            if (number < cluster_proba1000000) {
                // make a cluster
                for (uint xoff = 0; xoff < cluster_width;  ++xoff)
                for (uint yoff = 0; yoff < cluster_height; ++yoff) {
                    uint cx = x + xoff;
                    uint cy = y + yoff;
                    if (cx >= width || cy >= height) continue;
                    uint cell_random = rand() % 1000;
                    if (cell_random < cell_proba1000)
                        coll.present[x][y] = true;
                }
            }
        }
        return get_cell_collection(coll);
    }


    // =========================================
    // ============ CCL COMPUTATION ============
    // =========================================


    std::ofstream myfile; // a nasty global variable
    uint file_is_open = 0;
    const bool draw_array = true;
    const bool use_log_file = false;

    void open_log_file() {
        if (not use_log_file) return;
        if (file_is_open == 0) {
            file_is_open = 1;
            myfile.open("/home/sylvain/Desktop/traccc-local/example.txt");
            myfile << "Writing this to a file.\n";
        }
    }

    void write_log_file(const char *str) {
        if (not use_log_file) return;
        myfile << str;
    }

    void close_log_file() {
        if (not use_log_file) return;
        if (file_is_open == 1) {
            file_is_open = 0;
            myfile.close();
        }
    }

    using present_t = uint8_t;
    using label_t = uint32_t;

    struct cmodule {
        present_t **present; // pixel presence
        label_t   **label;   // recursively computed label
        int w = 0;
        int h = 0;
    };

    void free_cmodule(cmodule &module); // forward declaration

    void init_cmodule(cmodule &module) {
        
        if (module.w != width || module.h != height || module.present == nullptr || module.label == nullptr) {
            free_cmodule(module);
            module.present = new present_t*[width];
            for (int x = 0; x < width; ++x) {
                module.present[x] = new present_t[height];
            }
            module.label = new label_t*[width];
            for (int x = 0; x < width; ++x) {
                module.label[x] = new label_t[height];
            }
            module.w = width;
            module.h = height;
        }

        for (uint x = 0; x < width; ++x) {
            for (uint y = 0; y < height; ++y) {
                module.label[x][y] = 0;   // no label
                module.present[x][y] = 0; // not present (1 if there is a cell there)
            }
        }
    }

    void free_cmodule(cmodule &module) {
        if (module.w == 0 || module.h == 0) return;
        if (module.present != nullptr) {
            for (int x = 0; x < module.w; ++x) {
                delete module.present[x];
            }
            delete module.present;
            module.present = nullptr;
        }
        if (module.label != nullptr) {
            for (int x = 0; x < module.w; ++x) {
                delete module.label[x];
            }
            delete module.label;
            module.label = nullptr;
        }
        module.w = 0;
        module.h = 0;
    }

    void check_ccl_validity_recur(int x, int y, label_t current_label, cmodule &module) {
        if (x < 0 || x >= width) return;
        if (y < 0 || y >= height) return;
        if ( (module.label[x][y] != 0) || (not module.present[x][y]) ) return;

        // mark the current cell
        module.label[x][y] = current_label;

        // recursively mark the neighbors (8-connectivity)
        for (int xoff = -1; xoff <= 1; ++xoff) {
            for (int yoff = -1; yoff <= 1; ++yoff) {
                if ( (xoff == 0) && (yoff == 0) ) continue; // current cell
                check_ccl_validity_recur(x + xoff, y + yoff, current_label, module);
            }
        }
    }

    cmodule module;

    /// Simple CCL implementation to check the cluster number
    /// return 0 if okay, another number = err code
    /// Dirty, extremely slow, but theoretically functional
    int check_ccl_validity(const cell_collection& cells, unsigned int expected_label_nb) {
        
        if (cells.items.size() == 0) {
            if (expected_label_nb != 0) return 1;
            return 0;
        }

        
        init_cmodule(module);

        // Add the pixels on the module captor
        for (const traccc::cell &c : cells.items) {
            module.present[c.channel0][c.channel1] = 1;
        }

        // For every pixel, run the recursive algorithm.
        // Each component is recursively computed.
        // (not meant to be fast of beautiful, just to check for errors
        // in SparseCCL found cluster number)
        int component_count = 0;
        for (int x = 0; x < width; ++x) for (int y = 0; y < height; ++y) {
            if ( module.present[x][y] && (module.label[x][y] == 0) ) 
                check_ccl_validity_recur(x, y, ++component_count, module);
        }

        if (component_count == expected_label_nb) {
            //myfile << "all good.\n";
            free_cmodule(module);
            return 0;
        } else {

            std::cout << "ERROR sparseCCL found " << expected_label_nb << " cluster.s  and recursively found "
                        << component_count << " cluster.s. Cell number = " << cells.items.size() << "\n";

            if (use_log_file && file_is_open) {
                myfile << "BEGIN";
                for (int x = 0; x < width - 5; ++x)  {
                    myfile << "v";
                }
                myfile << std::endl;

                myfile << "ERROR sparseCCL found " << expected_label_nb << " cluster.s  and recursively found "
                        << component_count << " cluster.s. Cell number = " << cells.items.size();

                if (draw_array) {
                    // Prints the array
                    uint chk_cell_count = 0;
                    for (int y = 0; y < height; ++y) {
                        for (int x = 0; x < width; ++x) {
                            if ( module.present[x][y] ) {
                                myfile << module.label[x][y]; // yes, if label is > 9, it will be ugly.
                                ++chk_cell_count;
                            } else
                                myfile << " ";
                        }
                        myfile << std::endl;
                    }
                }

                // some code useful for further verification
                /*uint chk_cell_count = 0;
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        if ( module.present[x][y] ) ++chk_cell_count;
                    }
                }
                myfile << std::endl << "chk_cell_count " << chk_cell_count << std::endl;
                for (const traccc::cell &c : cells.items) {
                    myfile << "(" << c.channel0 << ", " << c.channel1 << ") ";
                }*/

                myfile << std::endl << "END";
                for (int x = 0; x < width - 3; ++x)  {
                    myfile << "^";
                }
                myfile << std::endl << std::endl;
            }

            free_cmodule(module);
            return 2;
        }
    }
}


// ===================================
// ============ CCL TESTS ============
// ===================================

/// Extensive testing of the CCL algorithm
int generate_data_and_test_ccl() {
    //std::cout << __func__ << "\n";

    // Generates a grid of pixels (random and with clusters)
    // creates a cell_collection ordered in column major
    // runs SparceCCL on CPU (the test is located in SparceCCL)

    traccc::component_connection cc; // CCL

    std::cout << "vvvvvv BEGIN TESTS ON GENERATED DATA vvvvvv" << std::endl << std::endl;

    const int trial_nb = 100;

    srand(42);
    
    traccc::chkccl::mgrid coll;

    int error_count = 0;

    // fully random, at first
    for (uint p = 1; p < 1000000; p = p * 10) {
        std::cout << "checking sparsity " << double(p) * 100 / 1000000 << "% ...\n";
        for (uint i = 0; i < trial_nb; ++i) {
            traccc::cell_collection cells_per_module = generate_full_random(coll, p);
            traccc::cluster_collection clusters_per_module =  cc(cells_per_module);
            uint sparse_ccl_size = clusters_per_module.items.size();
            int errnumber = traccc::chkccl::check_ccl_validity(cells_per_module, sparse_ccl_size);
            if (errnumber != 0) {
                ++error_count;
            }
        }
    }

    std::cout << "\nTerminated with " << error_count << " errors.\n" << std::endl;

    std::cout << "^^^^^^ END TESTS ON GENERATED DATA ^^^^^^" << std::endl << std::endl;

    return error_count;
}

std::string get_data_directory() {
    auto env_d_d = std::getenv("TRACCC_TEST_DATA_DIR");
    if (env_d_d == nullptr) {
        throw std::ios_base::failure("Test data directory not found. Please set TRACCC_TEST_DATA_DIR.");
    }
    return std::string(env_d_d) + std::string("/");
}



void process_event_file(std::string io_cells_file, const int eventTotalNb, const int eventCount, uint &m_modules, uint &n_errors,
                        uint &n_cells, uint &n_clusters, traccc::component_connection cc) {

    //io_cells_file = "/home/sylvain/Desktop/traccc-local/traccc/data//ccl_test_gen_data.csv";
    traccc::cell_reader creader(io_cells_file, {"geometry_id", "hit_id", "cannel0", "channel1", "activation", "time"});
    traccc::cell_container cells_per_event = traccc::read_cells(creader/*, surface_transforms*/);
    
    int module_event_count = cells_per_event.size();
    int local_module_count = 0;
    m_modules += module_event_count;
    int n_module_this_event = 0;
    int n_error_this_event = 0;

    std::cout << "Processing event file " << eventCount + 1 << " / " << eventTotalNb << "..." << std::endl;

    int perten = -1; // pencent /100; perten /10

    for (auto &cells_per_module : cells_per_event)
    {
        // The algorithmic code part: start
        // CCL algorithm
        traccc::cluster_collection clusters_per_module = cc(cells_per_module);

        int sparse_ccl_cluster_count = clusters_per_module.items.size();

        // Simple, naive recursive check
        int res = traccc::chkccl::check_ccl_validity(cells_per_module, sparse_ccl_cluster_count);
        
        if (res != 0) {
            ++n_errors;
            ++n_error_this_event;
        }
        
        ++n_module_this_event;

        int new_perten = 10 * n_module_this_event / module_event_count;
        if (perten != new_perten) {
            perten = new_perten;
            std::cout << perten << "0% " << std::flush;// << std::endl; // "[event " << event << "] " << 
            
        }
        
        n_cells += cells_per_module.items.size();
        n_clusters += sparse_ccl_cluster_count;
    }
    std::cout << "\n- errors : " << n_error_this_event << "\n\n";
}



void show_statistics(const uint m_modules, const uint n_errors, const uint n_clusters, const uint n_cells) {
    std::cout << "==> Statistics ... " << std::endl;
    std::cout << "- " << m_modules << " modules tested" << std::endl;
    std::cout << "- " << n_errors  << " error on cluster count" << std::endl;
    std::cout << "- " << n_clusters   << " clusters found" << std::endl;
    std::cout << "- " << n_cells   << " total pixels read" << std::endl;
    std::cout << std::endl;
}

int seq_run_debug(const std::string& detector_file, const std::string& cells_dir, unsigned int events)
{

    std::cout << "vvvvvv BEGIN TESTS ON CSV DATA vvvvvv" << std::endl << std::endl;

    std::string data_directory = get_data_directory();

    // Algorithms
    traccc::component_connection cc;

    // Output stats
    uint n_cells = 0;
    uint m_modules = 0;
    uint n_clusters = 0;
    uint n_errors = 0;

    // Loop over events
    for (unsigned int event = 0; event < events; ++event){

        // Read the cells from the relevant event file
        std::string event_string = "000000000";
        std::string event_number = std::to_string(event);
        event_string.replace(event_string.size()-event_number.size(), event_number.size(), event_number);

        std::string io_cells_file = data_directory+cells_dir+std::string("/event")+event_string+std::string("-cells.csv");
        
        process_event_file(io_cells_file, events, event, m_modules, n_errors, n_cells, n_clusters, cc);
        
    }

    show_statistics(m_modules, n_errors, n_clusters, n_cells);

    std::cout << "^^^^^^ END TESTS ON CSV DATA ^^^^^^" << std::endl << std::endl;

    return 0;
}

int test_stephen_data(std::string io_cells_file) {
    // csv data as "x, y, value, time"
    int old_width = traccc::chkccl::width;
    int old_height = traccc::chkccl::height;

    std::cout << "vvvvvv BEGIN TESTS STEPHEN DATA vvvvvv" << std::endl;

    std::cout << "WARNING : THIS MAY TAKE A LOT OF RAM. LIKE 20Go.\n\n";

    traccc::chkccl::width  = 65535;
    traccc::chkccl::height = 65535;

    //std::string fullpath = get_data_directory() + io_cells_file;
    std::cout << "Generated data path = --" << io_cells_file << "--\n\n";

    // Parse CSV data
    traccc::cell_reader creader(io_cells_file, {"geometry_id", "hit_id", "channel0", "channel1", "activation", "time"});
    traccc::cell_container cells_per_event = traccc::read_cells(creader);

    std::cout << "Processing Stephen file " << 1 << " / " << 1 << "..." << std::endl;

    uint events = 1;
    uint event = 0;

    // Algorithms
    traccc::component_connection cc;

    // Output stats
    uint n_cells = 0;
    uint m_modules = 0;
    uint n_clusters = 0;
    uint n_errors = 0;

    process_event_file(io_cells_file, events, event, m_modules, n_errors, n_cells, n_clusters, cc);

    show_statistics(m_modules, n_errors, n_clusters, n_cells);

    std::cout << "^^^^^^ END TESTS STEPHEN CSV DATA ^^^^^^" << std::endl << std::endl;

    // To not break things
    traccc::chkccl::width  = old_width;
    traccc::chkccl::height = old_height;
    return 0;
}


// taken from 
inline bool file_exists(const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}

// The main routine
//
int main(int argc, char *argv[])
{
    if (argc < 5){
        std::cout << "Not enough arguments, minimum requirement: " << std::endl;
        std::cout << "./sparse_ccl_debug <detector_file> <cell_directory> <events> <path_to_python_generated_data>" << std::endl;
        return -1;
    }

    auto detector_file = std::string(argv[1]);
    auto cell_directory = std::string(argv[2]);
    auto events = std::atoi(argv[3]);
    auto path_to_python_generated_data = std::string(argv[4]);

    std::cout << "\n";

    if (file_exists(path_to_python_generated_data))
        test_stephen_data(path_to_python_generated_data);
    else {
        std::cout << "WARNING : No valid <path_to_python_generated_data> provided, ignoring this check...\n\n";
    }
    

    std::cout << "Running ./sparse_ccl_debug " << detector_file << " " << cell_directory << " " << events << "\n\n";
    int res = seq_run_debug(detector_file, cell_directory, events);
    
    res = res || generate_data_and_test_ccl();

    return res;
}
