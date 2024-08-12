cmake_minimum_required(VERSION 3.13)

project(INXflow
	VERSION 0.1.0
	LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(inxlib_flow)
add_library(inxlib::flow ALIAS inxlib_flow)
target_link_libraries(inxlib_flow PUBLIC inxlib::lib)
target_include_directories(inxlib_lib INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

add_subdirectory(src)
target_sources(inxlib_flow PUBLIC include/inxflow/util/string.hpp)
