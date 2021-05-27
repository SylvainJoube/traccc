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

        log("read_cells module_count " + std::to_string(module_count) + " start...");

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

    



}