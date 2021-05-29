/** TRACCC library, part of the ACTS project (R&D line)
 *
 * (c) 2021 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */


#pragma once

#include "edm/cell.hpp"
//#include <iostream>
#include <fstream>
#include <string.h>
#include <iostream>

#include <filesystem>

#include <vecmem/memory/host_memory_resource.hpp>

/*#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>*/


namespace traccc::binio {
    
    void log(std::string str) {
        std::cout << str << std::endl;
    }

    bool read_cells(std::string fpath, traccc::host_cell_container &hc_container) {
        //traccc::host_cell_container cells_per_event;

        //log("read_cells 0");
        std::ifstream rf(fpath, std::ios::out | std::ios::binary);
        
        if(!rf) {
            return false;
        }

        //log("read_cells 1");

        int module_count;
        rf.read((char *)(&module_count), sizeof(int));

        //log("read_cells module_count " + std::to_string(module_count) + " start...");

        //log("read_cells 2");

        hc_container.headers.reserve(module_count);
        hc_container.items.reserve(module_count);

        for (std::size_t im = 0; im < module_count; ++im) {
            //log("read_cells im " + std::to_string(im) + " start...");
            traccc::cell_module module;
            rf.read((char *)(&module), sizeof(traccc::cell_module));
            hc_container.headers.push_back(module);

            //hc_container.headers[im] = module;
            //rf.read((char *)(&hc_container.headers[im]), sizeof(traccc::cell_module));
            
            //log("read_cells im " + std::to_string(im) + "cell module loaded");
            int cell_count;
            rf.read((char *)(&cell_count), sizeof(int));

            //log("read_cells im " + std::to_string(im) + " cells nb = " + std::to_string(cell_count));

            vecmem::vector<traccc::cell> cells;

            cells.reserve(cell_count);
            //log("read_cells im " + std::to_string(im) + " reserve ok");
            for (std::size_t ic = 0; ic < cell_count; ++ic) {
                traccc::cell cell;
                rf.read((char *)(&cell), sizeof(traccc::cell));
                cells.push_back(cell);
            }
            hc_container.items.push_back(cells);
            //log("read_cells im " + std::to_string(im) + " all cells read.");
        }

        //log("read_cells closing...");
        rf.close();

        //log("read_cells closed !");
        if(!rf.good()) {
            return false;
        }

        return true;
    }


    bool write_cells(const traccc::host_cell_container &cells_per_event, int event_id, std::string fpath) {
        // "/home/sylvain/Desktop/StageM2/traccc/data_bin/event" + std::to_string(event_id) + ".bin"
        std::ofstream wf(fpath, std::ios::out | std::ios::binary);

        int module_count = cells_per_event.headers.size();

        // write module count
        wf.write((char *)(&module_count), sizeof(int));

        for (std::size_t i = 0; i < cells_per_event.items.size(); ++i ) {
	        const traccc::cell_module &module = cells_per_event.headers[i]; // traccc::cell_module
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
            return false;
        }

        return true;
    }

    bool compare_mem(const char *a, const char *b, uint size) {
        //return (std::memcmp(a, b, size) == 0);
        
        for (uint i = 0; i < size; ++i) {
            if ((a[i]) != (b[i]))
                return false;
        }
        return true;
    }

    bool check_results(const traccc::host_cell_container &ca, const traccc::host_cell_container &cb, std::string &message) {
        message = "coucou";

        int sa = ca.items.size();
        int sb = cb.items.size();
        if (sa != sb) {
            message = "a size ("+std::to_string(sa)+") does not equals b size ("+std::to_string(sb)+")";
            return false;
        }

        int sah = ca.headers.size();
        int sbh = cb.headers.size();
        if (sa != sah) {
            message = "bad header item size for a";
            return false;
        }

        if (sb != sbh) {
            message = "bad header item size for b : header_nb(" + std::to_string(sbh) + ")  items_nb(" + std::to_string(sb) + ")";
            return false;
        }

        for (int im = 0; im < sa ; ++im) {

            const traccc::cell_module &cma = ca.headers[im];
            const traccc::cell_module &cmb = cb.headers[im];


            if ( ! compare_mem((char*)&cma, (char*)&cmb, sizeof(traccc::cell_module)) ) {
                message = "modules does not equal at index " + std::to_string(im)
                + "\n cma.event      " + std::to_string(cma.event)
                + "\n cma.module     " + std::to_string(cma.module)
                //+ "\n cma.pixel      " + std::to_string(cma.pixel)
                //+ "\n cma.placement  " + std::to_string(cma.placement)
                + "\n cma.range0     " + std::to_string(cma.range0[0])
                + "\n cma.range1     " + std::to_string(cma.range1[0])
                + "\n"

                + "\n cmb.event      " + std::to_string(cmb.event)
                + "\n cmb.module     " + std::to_string(cmb.module)
                //+ "\n cma.pixel      " + std::to_string(cma.pixel)
                //+ "\n cma.placement  " + std::to_string(cma.placement)
                + "\n cmb.range0     " + std::to_string(cmb.range0[0])
                + "\n cmb.range1     " + std::to_string(cmb.range1[0])
                + "\n"
                + "\n true           " + std::to_string(true)
                + "\n false          " + std::to_string(false)
                + "\n pixel          " + std::to_string(compare_mem((char*)&cma.module, (char*)&cmb.module, sizeof(traccc::geometry_id)))
                + "\n";


                return false;
            }
            
            if (ca.items[im].size() != cb.items[im].size()) {
                message = "cell size does not equal at index " + std::to_string(im);
                return false;
            }

            for (int ic = 0; ic < ca.items[im].size(); ++ic) {
                const traccc::cell &cella = ca.items[im][ic];
                const traccc::cell &cellb = cb.items[im][ic];

                if ( ! compare_mem((char*)&cella, (char*)&cellb, sizeof(traccc::cell)) ) {
                    message = "cell does not equal at module index " + std::to_string(im) + " and cell index " + std::to_string(ic);
                    return false;  
                }
            }
        }

        return true;

    }


    std::string get_binary_directory(const std::string& cells_dir, const std::string& data_directory) {
        std::string cells_base_directory = data_directory + cells_dir;
        std::string bin_directory = cells_base_directory + std::string("/binary");
        return bin_directory;
    }

    // Create binary files from csv, when does not exist
    bool create_binary_from_csv_verbose(const std::string& detector_file, const std::string& cells_dir,
                                        const std::string& data_directory, int event_count) {

        /*log("create_binary_from_csv_verbose");
        log("detector_file = " + detector_file);
        log("cells_dir = " + cells_dir);
        log("data_directory = " + data_directory);*/

        std::string cells_base_directory = data_directory + cells_dir;
        std::string bin_directory = get_binary_directory(cells_dir, data_directory); //cells_base_directory + std::string("/binary");
        std::string csv_directory = cells_base_directory;

        // First, checks if the needed binary files exists
        // assumes that if a file exists, it's correct.
        // To fix a broken file, just delete it in your filesystem.

        std::vector<unsigned int> missing_events;

        for (unsigned int event = 0; event < event_count; ++event) {
            std::string event_fpath = bin_directory + "/event"+std::to_string(event)+".bin";
            if ( ! std::filesystem::exists(event_fpath) ) {
                missing_events.push_back(event);
            }
        }

        if (missing_events.empty()) {
            log("All binary files already present !");
            log(std::string("If a file is broken (making bad results), please delete it in your filesystem, ")
              + std::string("as it will be re-created automatically on next run."));
            return true;
        }


        // Read the surface transforms
        std::string io_detector_file = data_directory + detector_file;
        traccc::surface_reader sreader(io_detector_file, {"geometry_id", "cx", "cy", "cz", "rot_xu", "rot_xv", "rot_xw", "rot_zu", "rot_zv", "rot_zw"});
        auto surface_transforms = traccc::read_surfaces(sreader);

        // Memory resource used by the EDM.
        vecmem::host_memory_resource resource;

        if ( ! std::filesystem::is_directory(bin_directory) ) {
            log("Creating binary directory... " + bin_directory);
            std::filesystem::create_directory(bin_directory);
        }

        if ( ! std::filesystem::is_directory(bin_directory) ) {
            log("ERROR creating csv to binary directory, unable to create :\n" + bin_directory);
            return false;
        }

        // Loop over events
        // Only converts missing binary files
        //for (unsigned int event = 0; event < event_count; ++event){
        for (unsigned int event : missing_events) {

            std::cout << "== loading and converting event " + std::to_string(event) + "... ===" << std::endl;

            // Read the cells from the relevant event file
            std::string event_string = "000000000";
            std::string event_number = std::to_string(event);
            event_string.replace(event_string.size()-event_number.size(), event_number.size(), event_number);

            std::string io_cells_file = csv_directory + std::string("/event")+event_string+std::string("-cells.csv");
            traccc::cell_reader creader(io_cells_file, {"geometry_id", "hit_id", "cannel0", "channel1", "activation", "time"});
            traccc::host_cell_container cells_per_event = traccc::read_cells(creader, resource, surface_transforms);
            
            // /home/sylvain/Desktop/StageM2/traccc/data_bin
            std::string event_fpath = bin_directory + "/event"+std::to_string(event)+".bin";

            bool success;
            
            success = traccc::binio::write_cells(cells_per_event, event, event_fpath);

            if (! success) {
                std::cout << "ERROR WRITE CELLS for event " + std::to_string(event) + "\n";
                return false;
            }
            std::cout << "Event " + std::to_string(event) + " written..." << std::endl;

            traccc::host_cell_container cells_per_event_verif;

            std::cout << "Event " + std::to_string(event) + " reading cells..." << std::endl;

            success = traccc::binio::read_cells(event_fpath, cells_per_event_verif);

            if (! success) {
                std::cout << "ERROR READ CELLS for event " + std::to_string(event) + "\n";
                return false;
            }
            std::cout << "Event " + std::to_string(event) + " read..." << std::endl;

            std::string msg;

            success = traccc::binio::check_results(cells_per_event, cells_per_event_verif, msg);

            if (! success) {
                std::cout << "ERROR CHECK RESULT for event " + std::to_string(event) + "\n";
                std::cout << "   message =  " + msg + "\n";
                return false;
            }

            std::cout << "Event " + std::to_string(event) + " all checks passed !" << std::endl;
        }
        std::cout << "==> Wrote everything ! " << std::endl;
        return true;
    }

    



}