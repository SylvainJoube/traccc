add_executable (seq_helloworld seq_helloworld.cpp)
target_link_libraries (seq_helloworld LINK_PUBLIC traccc::io traccc::algorithms)

