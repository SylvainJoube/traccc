/** TRACCC library, part of the ACTS project (R&D line)
 * 
 * (c) 2021 CERN for the benefit of the ACTS project
 * 
 * Mozilla Public License Version 2.0
 *
 * WIP by Sylvain Joube - joube@ijclab.in2p3.fr - github.com/SylvainJoube 
 */


#pragma once

//#include "definitions/algebra.hpp"
//#include "definitions/primitives.hpp"
//#include <vector>
//#include <limits>
#include "edm/cell.hpp"

// TODO : have the namespace sycl::ccl in its own file
// Sparse CCL for SyCL, accessors and buffers
// modified code to execute on GPU
// (the entire code needed rethinking)
// This is a simple v1.
namespace traccc::sycl::ccl {
    // Input module controller (one per module)
    struct input_module_ctrl {
        uint cell_start_index;
        uint cell_count;
    };

    /*struct cpu_sort_input_module { not used
        uint cell_count;
        traccc::cell_collection local_cpu_cells;
    };

    bool compare_input_module_struct(cpu_sort_input_module s1, cpu_sort_input_module s2) {
        return (s1.cell_count < s2.cell_count);
    }*/

    // Output module controller (one per module)
    struct output_module_ctrl {
        uint cluster_count = 0;
    };
    // Output cell
    using label_type = uint;
    struct output_cell {
        label_type label = 0;
    };
    // Input cell
    struct input_cell {
        traccc::cell cell;
    };

    // For local USM
    // contains input and output data
    struct local_module {
        input_cell *in_cells = nullptr; // input cells
        output_cell *out_labels = nullptr; // output labels
        uint cell_count = 0; // number of cells
        uint cluster_count = 0;
        traccc::sycl::ccl::input_cell *device_input_cells = nullptr; // should be freed at the end of computation
        traccc::sycl::ccl::output_cell *device_output_cells = nullptr;
        uint *device_culster_count = nullptr;
    };

    struct device_module {
        traccc::sycl::ccl::input_cell *device_input_cells = nullptr; // should be freed at the end of computation
        traccc::sycl::ccl::output_cell *device_output_cells = nullptr;
        uint *device_culster_count = nullptr;
        uint cell_count = 0; // number of cells
    };

}