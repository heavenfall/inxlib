/*
MIT License

Copyright (c) 2024 Ryan Hechenberger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <algorithm>
#include <inxflow/util/string.hpp>

namespace inx::flow::util {

std::pair<VarName, size_t>
parse_varname(std::string_view parse, bool whitespace)
{
	using namespace std::string_view_literals;
	// handle default case
	switch (parse.size()) {
	case 0:
		return {};
	case 1:
		if (parse[0] == VarBlockChar)
			return {};
		break;
	default: // either "@@" or "x@" is an invalid parse
		if (parse[1] == VarBlockChar)
			return {};
		break;
	}
	size_t parsed_length;
	std::string_view subparse;
	if (parse[0] == VarBlockChar) {
		assert(parse[1] != VarBlockChar);
		if (auto p = parse.find(VarBlockChar, 1); p != std::string_view::npos) {
			parsed_length = p + 1;
			subparse = parse.substr(1, p - 1);
		} else {
			parsed_length = parse.size();
			subparse = parse.substr(1, p);
		}
	} else {
		if (auto p = parse.find(VarBlockChar); p != std::string_view::npos) {
			return {};
		}
		parsed_length = parse.size();
		subparse = parse;
	}
	assert(!subparse.empty());

	// now subparse must match the whole VarName without VarBlockChar to worry
	// about
	VarName result{};
	result.parsed_string_ = subparse.data();
	uint32 sub_at = 0;
	if (subparse[sub_at] == PrintChar) {
		if (++sub_at >= subparse.size())
			return {};
		result.var_op_ = VarOp::Print;
	} else {
		result.var_op_ = VarOp::Name;
	}
	// group
	if (subparse[sub_at] == GlobalChar) {
		if (++sub_at >= subparse.size())
			return {};
		result.var_class_ = VarClass::Global;
	} else {
		result.var_class_ = VarClass::Local;
	}
	if (auto len = subparse.substr(sub_at).rfind(GroupSepChar); len != std::string_view::npos) {
		// found group, parse
		// len -= sub_at;
		if (len == 0 || len > VarName::MaxGroupLength) {
			// bounds check
			return {};
		}
		if (std::ranges::any_of(subparse.substr(sub_at, len), [](unsigned char c) { return std::isspace(c); })) {
			// no whitespace permitted in group
			return {};
		}
		result.group_start_ = sub_at;
		result.group_len_ = len;
		sub_at += len + 1;
	} else {
		result.group_start_ = result.group_len_ = 0;
	}
	// var
	if (sub_at >= subparse.size())
		return {};
	auto varname = subparse.substr(sub_at);
	if (varname.size() == 0 || varname.size() > VarName::MaxNameLength)
		return {}; // varname size out of bounds
	if (varname.find_first_of(InvalidVarChars) != std::string_view::npos)
		return {}; // invalid character
	if (whitespace && std::ranges::any_of(varname, [](unsigned char c) { return std::isspace(c); }))
		return {}; // whitespace
	result.name_start_ = sub_at;
	result.name_len_ = varname.size();

	return {result, parsed_length};
}

VarName
match_varname(std::string_view parse, bool force_token)
{
	if (parse.empty() || (force_token && parse[0] != '@'))
		return {};
	auto res = parse_varname(parse);
	if (!res.first || res.second != parse.size())
		return {};
	return res.first;
}

} // namespace inx::flow::util
