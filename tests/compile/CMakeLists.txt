cmake_minimum_required(VERSION 3.13)

list(TRANSFORM INXLIB_HEADERS
	APPEND .cpp
	OUTPUT_VARIABLE COMPILE_SOURCES)
list(TRANSFORM COMPILE_SOURCES
	PREPEND "${CMAKE_CURRENT_BINARY_DIR}/"
	OUTPUT_VARIABLE COMPILE_SOURCES)

list(JOIN INXLIB_HEADERS "\\;" COMPILE_HEADERS_STR)


add_custom_command(OUTPUT ${COMPILE_SOURCES}
	COMMAND ${CMAKE_COMMAND} -DGEN_TEMPLATE=${CMAKE_CURRENT_SOURCE_DIR}/compile_file.cpp.in -DGEN_COMPILE="${COMPILE_HEADERS_STR}" -P ${CMAKE_CURRENT_SOURCE_DIR}/generate_src.cmake
)

add_executable(inxlib_test_compile compile.cpp)
add_executable(inxlib::test::compile ALIAS inxlib_test_compile)
target_link_libraries(inxlib_test_compile PUBLIC inxlib::lib)
configure_file(compile_file.cpp.in inxlib/util/virtual_pointer.hpp)
target_sources(inxlib_test_compile PRIVATE ${COMPILE_SOURCES})
