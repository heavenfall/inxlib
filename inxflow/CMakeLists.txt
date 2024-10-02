cmake_minimum_required(VERSION 3.13)

project(INXflow
	VERSION 0.1.0
	LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(Threads REQUIRED)

add_library(inxlib_flow)
add_library(inxlib::flow ALIAS inxlib_flow)
target_link_libraries(inxlib_flow PUBLIC inxlib::lib Threads::Threads)
target_include_directories(inxlib_lib INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

add_subdirectory(src)
# find include/inxflow -type f | sort
target_sources(inxlib_flow PUBLIC
include/inxflow/cmd/types.hpp
include/inxflow/data/group_template.hpp
include/inxflow/data/serialize.hpp
include/inxflow/data/string_serialize.hpp
include/inxflow/data/types.hpp
include/inxflow/framework.hpp
include/inxflow/util/string.hpp
)

if(InxIsTopProject AND BUILD_TESTING)
	add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()
