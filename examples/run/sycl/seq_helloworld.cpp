
#include "sycl/helloworld/helloworld_algorithm.hpp"
#include <iostream>


int seq_run(const std::string& detector_file, const std::string& hits_dir,
            unsigned int events, bool skip_cpu) {

    traccc::sycl::helloworld_algorithm h;
    h.hello_algorithm();

    std::cout << "SYCL seq_run successfully terminated." << std::endl;

    return 0;
}