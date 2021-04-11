add_executable (sparse_ccl_debug sparse_ccl_debug.cpp)
target_link_libraries (sparse_ccl_debug LINK_PUBLIC traccc::core traccc::io)