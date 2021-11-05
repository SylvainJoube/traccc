/** TRACCC library, part of the ACTS project (R&D line)
 *
 * (c) 2021 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

#include <iostream>

namespace traccc::sycl {

class hello_world {
    public:
    void say_hello() const;
    void test_sycl_vector_sum() const;
};

}  // namespace traccc::sycl
