/** TRACCC library, part of the ACTS project (R&D line)
 * 
 * (c) 2021 CERN for the benefit of the ACTS project
 * 
 * Mozilla Public License Version 2.0
 *
 * WIP by Sylvain Joube - joube@ijclab.in2p3.fr - github.com/SylvainJoube 
 */

#include "algorithms/detail/sparse_ccl.hpp" // only used for is_adjacent and is_far_enough
#include "edm/sycl_gpu_module.hpp"

namespace traccc::detail {

    /// Implemementation of SparseCCL, following [DOI: 10.1109/DASIP48288.2019.9049184]
    ///
    /// Requires cells to be sorted in column major.

    /// Find root of the tree for entry @param e
    ///
    /// @param L an equivalance table
    ///
    /// @return the root of @param e 
    unsigned int find_root(const unsigned int* L, unsigned int e) {
        unsigned int r = e;
        while (L[r] != r) {
            r = L[r];
        }
        return r;
    } 

    unsigned int find_root(const traccc::sycl::ccl::output_cell * L, unsigned int e) {
        unsigned int r = e;
        while (L[r].label != r) {
            r = L[r].label;
        }
        return r;
    }

    /// Create a union of two entries @param e1 and @param e2
    ///
    /// @param L an equivalance table
    ///
    /// @return the rleast common ancestor of the entries 
    unsigned int make_union(unsigned int* L, unsigned int e1, unsigned int e2){
        int e;
        if (e1 < e2){
            e = e1;
            L[e2] = e;
        } else {
            e = e2;
            L[e1] = e;
        }
        return e;
    }

    unsigned int make_union(traccc::sycl::ccl::output_cell * L, unsigned int e1, unsigned int e2){
        int e;
        if (e1 < e2){
            e = e1;
            L[e2].label = e;
        } else {
            e = e2;
            L[e1].label = e;
        }
        return e;
    }

    /// SyCL version, made to be gpu compliant
    void sparse_ccl_sycl(const cell* cells, int size, unsigned int* L, uint *new_size) {

        // cells must be of length "size"
        // new_size is the final number of distinct labels
        // L is the association table between each cell index and label

        // Internal list linking

        // first scan: pixel association
        unsigned int start_j = 0;
        for (unsigned int i=0; i < size; ++i){
            L[i] = i;
            int ai = i;
            if (i > 0){
                for (unsigned int j = start_j; j < i; ++j){
                    if (is_adjacent(cells[i], cells[j])){
                        ai = make_union(L, ai, find_root(L, j));
                    } else if (is_far_enough(cells[i], cells[j])){
                        ++start_j;
                    }
                }
            }
        }

        // second scan: transitive closure
        *new_size = 0;
        //unsigned int label_count = 0;
        for (unsigned int i = 0; i < size; ++i){
            unsigned int l = 0;
            if (L[i] == i){
                ++*new_size;
                l = *new_size; 
            } else {
                l = L[L[i]];
            }
            L[i] = l;
        }

        return;
    }
}

