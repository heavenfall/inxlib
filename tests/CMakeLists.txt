cmake_minimum_required(VERSION 3.13)

add_subdirectory(compile)

add_executable(inx_test_memory factory.cpp)
target_link_libraries(inx_test_memory PRIVATE inxlib::lib Catch2::Catch2WithMain)
catch_discover_tests(inx_test_memory)
