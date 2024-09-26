#include <inxflow/util/string.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace std::string_view_literals;

namespace inx::flow {

TEST_CASE( "String variable parsing", "[var]") {
	
	SECTION( "VarName as name" ) {
		auto ps = "@name1@"sv;
		auto p = util::parse_varname(ps);
		CHECK(p.first.cls() == util::VarClass::Global);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == ""sv);
		CHECK(p.first.name() == "name1"sv);
		CHECK(p.second == ps.size());
		
		ps = "@group!name2@"sv;
		p = util::parse_varname(ps);
		CHECK(p.first.cls() == util::VarClass::Global);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == "group"sv);
		CHECK(p.first.name() == "name2"sv);
		CHECK(p.second == ps.size());
		
		ps = "@$name3@"sv;
		p = util::parse_varname(ps);
		CHECK(p.first.cls() == util::VarClass::Local);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == ""sv);
		CHECK(p.first.name() == "name3"sv);
		CHECK(p.second == ps.size());
		
		ps = "@$gsd!name4@"sv;
		p = util::parse_varname(ps);
		CHECK(p.first.cls() == util::VarClass::Local);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == "gsd"sv);
		CHECK(p.first.name() == "name4"sv);
		CHECK(p.second == ps.size());
	}

	SECTION( "VarName grouping test" ) {
		auto ps = "name1"sv;
		auto p = util::parse_varname(ps);
		CHECK(p.first.cls() == util::VarClass::Global);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == ""sv);
		CHECK(p.first.name() == "name1"sv);
		CHECK(p.second == ps.size());

		ps = "@name2"sv;
		p = util::parse_varname(ps);
		CHECK(p.first.cls() == util::VarClass::Global);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == ""sv);
		CHECK(p.first.name() == "name2"sv);
		CHECK(p.second == ps.size());

		ps = "%$qx!name3"sv;
		p = util::parse_varname(ps);
		CHECK(p.first.cls() == util::VarClass::Local);
		CHECK(p.first.op() == util::VarOp::Print);
		CHECK(p.first.group() == "qx"sv);
		CHECK(p.first.name() == "name3"sv);
		CHECK(p.second == ps.size());

		ps = "%$qx!name3@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == true);
	}
}

}
