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

#ifndef INXFLOW_UTIL_STRING_HPP
#define INXFLOW_UTIL_STRING_HPP

#include <inxlib/inx.hpp>

namespace inx::flow::util {

enum class VarClass : uint8
{
	Local,
	Global,
};
enum class VarOp : uint8
{
	Name,
	Print,
};

constexpr char VarBlockChar = '@';
constexpr char GroupSepChar = '.';
constexpr char GlobalChar = '+';
constexpr char PrintChar = '%';
constexpr std::string_view InvalidVarChars("+%");

/**
 * Stored result of parsed variable. Does not own the underlying string.
 */
struct VarName
{
	static constexpr size_t MaxGroupLength = 63;
	static constexpr size_t MaxNameLength = (1 << 14) - 1;
	const char* parsed_string_;
	VarOp var_op_;
	VarClass var_class_;
	uint8 group_start_;
	uint8 group_len_;
	uint16 name_start_;
	uint16 name_len_;
	static_assert(MaxGroupLength <= std::numeric_limits<decltype(group_start_)>::max() - 8,
	              "MaxGroupLength must fit group_start_.");
	static_assert(MaxNameLength <= std::numeric_limits<decltype(name_len_)>::max() - MaxGroupLength - 8,
	              "MaxNameLength must fit name_len_.");

	std::string_view group() const noexcept
	{
		return group_len_ != 0 ? std::string_view(parsed_string_ + group_start_, group_len_) : std::string_view();
	}
	std::string_view name() const noexcept { return std::string_view(parsed_string_ + name_start_, name_len_); }
	VarOp op() const noexcept { return var_op_; }
	VarClass cls() const noexcept { return var_class_; }
	bool global() const noexcept { return var_class_ == VarClass::Global; }
	bool local() const noexcept { return var_class_ == VarClass::Local; }
	bool null() const noexcept { return parsed_string_ == nullptr; }

	operator bool() const noexcept { return parsed_string_ != nullptr; }
};

/**
 * Parse string and produce the VarName as formatted by
 * `@[op]?[group:]?[varname]@`. `@` is optional. On success returns a filled
 * VarName and the position parsed to. Expects to match either the whole string
 * or if the string starts with `@`, match only until the next `@` or the whole
 * string if missing second `@`. On invalid string, returns a null VarName and
 * 0.
 * @param whitespace If true, allows for whitespace in variable name. Group
 * never permits whitespace.
 */
std::pair<VarName, size_t>
parse_varname(std::string_view parse, bool whitespace = false);

VarName
match_varname(std::string_view parse, bool force_token);

} // namespace inx::flow::util

#endif // INXFLOW_UTIL_STRING_HPP
