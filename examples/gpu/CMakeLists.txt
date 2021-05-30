add_executable (sycl_ccl_usm_explicit_global sycl_ccl_usm_explicit_global.cpp)
target_link_libraries (sycl_ccl_usm_explicit_global LINK_PUBLIC traccc::core traccc::io vecmem::core)

add_executable (sycl_multi_event_explicit sycl_multi_event_explicit.cpp)
target_link_libraries (sycl_multi_event_explicit LINK_PUBLIC traccc::core traccc::io vecmem::core)

add_executable (sycl_multi_event_implicit sycl_multi_event_implicit.cpp)
target_link_libraries (sycl_multi_event_implicit LINK_PUBLIC traccc::core traccc::io vecmem::core)

add_executable (sycl_multi_event_access sycl_multi_event_access.cpp)
target_link_libraries (sycl_multi_event_access LINK_PUBLIC traccc::core traccc::io vecmem::core)