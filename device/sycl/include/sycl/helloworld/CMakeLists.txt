add_library(traccc_sycl_helloworld SHARED
  ${TRACCC_HELLOWORLD_SOURCE_DIR}/helloworld.sycl)
# ${TRACCC_CUDA_SEEDING_SOURCE_DIR}/doublet_finding.cu

target_include_directories(traccc_sycl_helloworld
  PUBLIC
  $<BUILD_INTERFACE:${TRACCC_SYCL_DIR}/include>
  $<INSTALL_INTERFACE:include>
  #${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
  )

# target_compile_options(traccc_sycl_helloworld
#   PUBLIC $<$<COMPILE_LANGUAGE:SYCL>:
#   #-arch=sm_52 
#   #-gencode=arch=compute_75,code=sm_75 
#   --expt-relaxed-constexpr>)
#   #--use_fast_math>)
#   #-fmad=false>)

target_link_libraries(traccc_sycl_helloworld PUBLIC traccc::core)
#vecmem::cuda)

add_library(traccc::sycl::helloworld ALIAS traccc_sycl_helloworld)
