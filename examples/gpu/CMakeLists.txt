add_executable (sycl_ccl_usm_explicit_global sycl_ccl_usm_explicit_global.cpp)
target_link_libraries (sycl_ccl_usm_explicit_global LINK_PUBLIC traccc::core traccc::io vecmem::core)