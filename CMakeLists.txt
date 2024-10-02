cmake_minimum_required(VERSION 3.13)
# CMP0076

project(INXlib
	VERSION 0.1.0
	LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(InxIsTopProject OFF)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.21)
	if(${PROJECT_IS_TOP_LEVEL})
		set(InxIsTopProject ON)
	endif()
else()
	if(${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		set(InxIsTopProject ON)
	endif()
endif()

add_library(inxlib_lib INTERFACE)
add_library(inxlib::lib ALIAS inxlib_lib)
target_include_directories(inxlib_lib INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
target_sources(inxlib_lib PUBLIC include/inxlib/inx.hpp
# find include/inxlib/{data,io,util} -type f | sort
include/inxlib/data/binary_tree.hpp
include/inxlib/data/bit_table.hpp
include/inxlib/data/block_array.hpp
include/inxlib/data/factory.hpp
include/inxlib/data/mary_tree.hpp
include/inxlib/data/redblack_tree.hpp
include/inxlib/data/slice_array.hpp
include/inxlib/data/slice_factory.hpp
include/inxlib/io/transformers.hpp
include/inxlib/util/bits.hpp
include/inxlib/util/functions.hpp
include/inxlib/util/iterator.hpp
include/inxlib/util/math.hpp
include/inxlib/util/numeric_types.hpp
include/inxlib/util/virtual_pointer.hpp
include/inxlib/util/xoshiro256.hpp
)

if(InxIsTopProject)
	option(BUILD_TESTING "Building testing suite" OFF)
	include(CTest)
	include(FetchContent)
	FetchContent_Declare(
		Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG        v3.7.1
		EXCLUDE_FROM_ALL
	)

	if(BUILD_TESTING)
		FetchContent_MakeAvailable(Catch2)
		include(Catch)
		add_subdirectory(tests)
	endif()
endif()

add_subdirectory(inxflow)
