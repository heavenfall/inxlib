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

#ifndef INXFLOW_DATA_STRING_SERIALIZE_HPP
#define INXFLOW_DATA_STRING_SERIALIZE_HPP

#include "serialize.hpp"
#include <memory_resource>
#include <sstream>

namespace inx::flow::data {

class StringSerialize
{
public:
	StringSerialize() = default;
	StringSerialize(const std::pmr::polymorphic_allocator<char>& alloc)
	  : m_data(alloc)
	{
	}
	StringSerialize(std::string_view str,
	                const std::pmr::polymorphic_allocator<char>& alloc = std::pmr::polymorphic_allocator<char>())
	  : m_data(str, alloc)
	{
	}

	static consteval bool ser_binary() noexcept { return false; }

	void load(std::istream& in)
	{
		std::ostringstream ss;
		ss << in.rdbuf();
		m_data = ss.view();
	}
	void save(std::ostream& out)
	{
		std::istringstream ss(m_data);
		ss >> out.rdbuf();
	}

	StringSerialize& operator=(std::string_view str)
	{
		m_data = str;
		return *this;
	}

	std::pmr::string& str() noexcept { return m_data; }
	const std::pmr::string& str() const noexcept { return m_data; }

	std::string_view view() const noexcept { return m_data; }

protected:
	std::pmr::string m_data;
};

} // namespace inx::flow::data

#endif // INXFLOW_DATA_STRING_SERIALIZE_HPP
