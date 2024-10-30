cmake_minimum_required(VERSION 3.13)

add_executable(example_var_store var_store/var_store.cpp)
add_executable(inxlib::examples::var_store ALIAS example_var_store)
target_link_libraries(example_var_store PUBLIC inxlib::flow)

add_executable(example_custom_command custom_command/custom_command.cpp)
add_executable(inxlib::examples::custom_command ALIAS example_custom_command)
target_link_libraries(example_custom_command PUBLIC inxlib::flow)
