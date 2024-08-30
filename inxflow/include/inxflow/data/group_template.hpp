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

#ifndef INXFLOW_DATA_GROUP_TEMPLATE_HPP
#define INXFLOW_DATA_GROUP_TEMPLATE_HPP

#include "serialize.hpp"
#include <inxlib/inx.hpp>
#include <inxlib/util/functions.hpp>

namespace inx::flow::data {

/// @brief Contains the information required to generate a Serialize type from
/// group string
class GroupSignature
{
public:
	std::string_view name() const noexcept { return m_name; }
	const Serialize& base() const noexcept { return *m_base; }

	template <concepts::serializable T>
	std::unique_ptr

	  private : GroupSignature();
	GroupSignature(GroupSignature&&) = delete;

	std::string_view m_name;
	std::unique_ptr<Serialize, inx::util::functor<[](Serialize&) noexcept {}>>
	  m_base;
};

class GroupTemplate
{};

} // namespace inx::flow::data

#endif // INXFLOW_DATA_GROUP_TEMPLATE_HPP
