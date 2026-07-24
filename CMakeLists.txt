cmake_minimum_required(VERSION 3.13)
# CMP0076

project(INXlib
	VERSION 0.5.0
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

include(cmake/headers.cmake)
list(TRANSFORM INXLIB_HEADERS PREPEND "include/" OUTPUT_VARIABLE INXLIB_SOURCES_HEADERS)

target_sources(inxlib_lib INTERFACE
${INXLIB_SOURCES_HEADERS}
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
