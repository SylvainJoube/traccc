enable_language(SYCL)

set( TRACCC_SYCL_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
set( TRACCC_HELLOWORLD_SOURCE_DIR ${TRACCC_SYCL_DIR}/src/helloworld )

add_subdirectory(include/sycl/helloworld)

add_library(traccc_sycl INTERFACE)
target_link_libraries(traccc_sycl INTERFACE
  traccc::sycl::helloworld)
add_library(traccc::sycl ALIAS traccc_sycl)
