cmake_minimum_required(VERSION 3.13)
# CMP0076

project(INXlib
	VERSION 0.1.0
	LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(inxlib_core INTERFACE)
add_library(inxlib::inxlib ALIAS inxlib_core)
target_include_directories(inxlib_core INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
target_sources(inxlib_core PUBLIC include/inxlib/inx.hpp
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

add_subdirectory(tests)
