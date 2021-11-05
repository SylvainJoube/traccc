

#include "sycl/helloworld/helloworld.hpp"

namespace traccc::sycl {

class helloworld_algorithm {
    public:

    // Say hello from the SYCL library
    // and test a simple vector sum on a sycl kernel
    void hello_algorithm() const {
        hello_world h;
        h.say_hello();
        h.test_sycl_vector_sum();
    }
};

}