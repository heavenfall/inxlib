cmake_minimum_required(VERSION 3.13)

target_sources(inxlib_flow PRIVATE
	framework.cpp
	data/group_template.cpp
	data/serialize.cpp
	util/string.cpp
)
