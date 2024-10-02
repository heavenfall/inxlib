#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <inxflow/util/string.hpp>

using namespace std::string_view_literals;

namespace inx::flow {

TEST_CASE( "String variable parsing", "[var]") {
	
	SECTION( "VarName as name" ) {
		auto ps = "@name1@"sv;
		auto p = util::parse_varname(ps);
		CHECK(bool{p.first} == true);
		CHECK(p.first.cls() == util::VarClass::Global);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == ""sv);
		CHECK(p.first.name() == "name1"sv);
		CHECK(p.second == ps.size());

		ps = "@group.name2@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == true);
		CHECK(p.first.cls() == util::VarClass::Global);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == "group"sv);
		CHECK(p.first.name() == "name2"sv);
		CHECK(p.second == ps.size());

		ps = "@group.1.name2@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == true);
		CHECK(p.first.cls() == util::VarClass::Global);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == "group.1"sv);
		CHECK(p.first.name() == "name2"sv);
		CHECK(p.second == ps.size());

		ps = "@$name3@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == true);
		CHECK(p.first.cls() == util::VarClass::Local);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == ""sv);
		CHECK(p.first.name() == "name3"sv);
		CHECK(p.second == ps.size());

		ps = "@$gsd.name4@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == true);
		CHECK(p.first.cls() == util::VarClass::Local);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == "gsd"sv);
		CHECK(p.first.name() == "name4"sv);
		CHECK(p.second == ps.size());

		ps = "@$gsd.md3.name5@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == true);
		CHECK(p.first.cls() == util::VarClass::Local);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == "gsd.md3"sv);
		CHECK(p.first.name() == "name5"sv);
		CHECK(p.second == ps.size());
	}

	SECTION( "VarName grouping test" ) {
		auto ps = "name1"sv;
		auto p = util::parse_varname(ps);
		CHECK(bool{p.first} == true);
		CHECK(p.first.cls() == util::VarClass::Global);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == ""sv);
		CHECK(p.first.name() == "name1"sv);
		CHECK(p.second == ps.size());

		ps = "@name2"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == true);
		CHECK(p.first.cls() == util::VarClass::Global);
		CHECK(p.first.op() == util::VarOp::Name);
		CHECK(p.first.group() == ""sv);
		CHECK(p.first.name() == "name2"sv);
		CHECK(p.second == ps.size());

		ps = "%$qx.name3"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == true);
		CHECK(p.first.cls() == util::VarClass::Local);
		CHECK(p.first.op() == util::VarOp::Print);
		CHECK(p.first.group() == "qx"sv);
		CHECK(p.first.name() == "name3"sv);
		CHECK(p.second == ps.size());

		ps = "%$qx.name3@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == false);
	}

	SECTION("VarName invalid")
	{
		auto ps = ""sv;
		auto p = util::parse_varname(ps);
		CHECK(bool{p.first} == false);

		ps = "@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == false);

		ps = "@@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == false);

		ps = "@a.b.@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == false);

		ps = "@.xyz@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == false);

		ps = "@.@"sv;
		p = util::parse_varname(ps);
		CHECK(bool{p.first} == false);
	}

	SECTION("VarName limits")
	{
		auto i = GENERATE(0, 1, 2);
		std::string group = std::string(util::VarName::MaxGroupLength + i, 'a');
		std::string name = std::string(util::VarName::MaxNameLength + i, 'b');
		std::string ps = std::format("@{0}.{1}@", group, "xyz");
		auto p = util::parse_varname(ps);
		if (i != 0) {
			CHECK(bool{p.first} == false);
		} else {
			CHECK(bool{p.first} == true);
			CHECK(p.first.cls() == util::VarClass::Global);
			CHECK(p.first.op() == util::VarOp::Name);
			CHECK(p.first.group() == group);
			CHECK(p.first.name() == "xyz");
			CHECK(p.second == ps.size());
		}

		ps = std::format("@{0}.{1}@", "abc", name);
		p = util::parse_varname(ps);
		if (i != 0) {
			CHECK(bool{p.first} == false);
		} else {
			CHECK(bool{p.first} == true);
			CHECK(p.first.cls() == util::VarClass::Global);
			CHECK(p.first.op() == util::VarOp::Name);
			CHECK(p.first.group() == "abc");
			CHECK(p.first.name() == name);
			CHECK(p.second == ps.size());
		}

		ps = std::format("@{0}.{1}@", group, name);
		p = util::parse_varname(ps);
		if (i != 0) {
			CHECK(bool{p.first} == false);
		} else {
			CHECK(bool{p.first} == true);
			CHECK(p.first.cls() == util::VarClass::Global);
			CHECK(p.first.op() == util::VarOp::Name);
			CHECK(p.first.group() == group);
			CHECK(p.first.name() == name);
			CHECK(p.second == ps.size());
		}
	}
}

}
