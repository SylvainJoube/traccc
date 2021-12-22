add_library(traccc_algorithms_sycl INTERFACE)

target_include_directories(traccc_algorithms_sycl
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(traccc_algorithms_sycl
  INTERFACE traccc::io traccc::core traccc::sycl)

add_library(traccc::algorithms_sycl ALIAS traccc_algorithms_sycl)
