cmake_minimum_required(VERSION 3.13)

add_executable(inxflow_test_cmdparse EXCLUDE_FROM_ALL cmdparse.cpp)
target_link_libraries(inxflow_test_cmdparse PRIVATE inxlib::flow Catch2::Catch2WithMain)
