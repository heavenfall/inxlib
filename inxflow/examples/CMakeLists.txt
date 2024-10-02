cmake_minimum_required(VERSION 3.13)

add_executable(var_store var_store/var_store.cpp)
add_executable(inxlib::examples::var_store ALIAS var_store)
target_link_libraries(var_store PUBLIC inxlib::flow)
