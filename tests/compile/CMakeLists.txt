cmake_minimum_required(VERSION 3.13)

# find include/inxlib/*/ -type f | sed 's|include/||' | sort
set(COMPILE_HEADERS
inxlib/data/binary_tree.hpp
inxlib/data/bit_table.hpp
inxlib/data/mary_tree.hpp
inxlib/data/redblack_tree.hpp
inxlib/io/null.hpp
inxlib/io/transformers.hpp
inxlib/memory/area_factory.hpp
inxlib/memory/block_array.hpp
inxlib/memory/block_factory.hpp
inxlib/memory/factory.hpp
inxlib/memory/factory_adaptor.hpp
inxlib/memory/factory_chain.hpp
inxlib/memory/object.hpp
inxlib/memory/single_factory.hpp
inxlib/memory/slice_array.hpp
inxlib/memory/slice_factory.hpp
inxlib/memory/source_factory.hpp
inxlib/numeric/bits.hpp
inxlib/numeric/fixed_point.hpp
inxlib/numeric/int128.hpp
inxlib/numeric/math.hpp
inxlib/numeric/numeric_types.hpp
inxlib/util/bits.hpp
inxlib/util/functions.hpp
inxlib/util/iterator.hpp
inxlib/util/math.hpp
inxlib/util/numeric_types.hpp
inxlib/util/virtual_pointer.hpp
inxlib/util/xoshiro256.hpp
)

list(TRANSFORM COMPILE_HEADERS
	APPEND .cpp
	OUTPUT_VARIABLE COMPILE_SOURCES)
list(TRANSFORM COMPILE_SOURCES
	PREPEND "${CMAKE_CURRENT_BINARY_DIR}/"
	OUTPUT_VARIABLE COMPILE_SOURCES)

list(JOIN COMPILE_HEADERS "\\;" COMPILE_HEADERS_STR)

add_custom_command(OUTPUT ${COMPILE_SOURCES}
	COMMAND ${CMAKE_COMMAND} -DGEN_TEMPLATE=${CMAKE_CURRENT_SOURCE_DIR}/compile_file.cpp.in -DGEN_COMPILE="${COMPILE_HEADERS_STR}" -P ${CMAKE_CURRENT_SOURCE_DIR}/generate_src.cmake
)

add_executable(inxlib_test_compile compile.cpp)
add_executable(inxlib::test::compile ALIAS inxlib_test_compile)
target_link_libraries(inxlib_test_compile PUBLIC inxlib::lib)
configure_file(compile_file.cpp.in inxlib/util/virtual_pointer.hpp)
target_sources(inxlib_test_compile PRIVATE ${COMPILE_SOURCES})
