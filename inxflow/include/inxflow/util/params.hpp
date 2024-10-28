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

#ifndef INXFLOW_UTIL_PARAMS_HPP
#define INXFLOW_UTIL_PARAMS_HPP

#include <inxlib/inx.hpp>
#include <memory_resource>
#include <span>
#include <unordered_map>
#include <variant>

namespace inx::flow::util {

struct param_values
{
	using value_type = std::variant<int64, double, std::string_view>;
	struct value_store
	{
		int64 length; /// >=0: string_view, -1: int, -2: double
		union
		{
			const char* c;
			int64 i;
			double f;
		} v;

		value_type value() const noexcept
		{
			assert(length >= -2);
			if (length >= 0) {
				return std::string_view(v.c, length);
			} else if (length == -1) {
				return v.i;
			} else {
				return v.f;
			}
		}
	};
	using list_type = std::span<const value_store>;
	list_type data;
	value_type operator[](int i) const noexcept { return data[i].value(); }
	value_type at(int i = 0) const
	{
		if (static_cast<size_t>(i) >= data.size())
			throw std::out_of_range("i");
		return data[i].value();
	}
	value_store at_store(int i = 0) const
	{
		if (static_cast<size_t>(i) >= data.size())
			throw std::out_of_range("i");
		return data[i];
	}
	int64 as_int(int i = 0) const { return std::get<int64>(at(i)); }
	double as_double(int i = 0) const
	{
		switch (value_store c = at_store(i); c.length) {
		case -1:
			return static_cast<double>(c.v.i);
		case -2:
			return c.v.f;
		default:
			throw std::bad_variant_access();
		}
	}
	std::string_view as_string(int i = 0) const
	{
		return std::get<std::string_view>(at(i));
	}
	std::string to_string(int i = 0) const { return to_str(at(i)); }

	bool is_int(int i = 0) const
	{
		return std::holds_alternative<int64>(at(i));
	}
	bool is_float(int i = 0) const
	{
		return std::holds_alternative<double>(at(i));
	}
	bool is_string(int i = 0) const
	{
		return std::holds_alternative<std::string_view>(at(i));
	}

	bool empty() const noexcept { return data.empty(); }
	size_t size() const noexcept { return data.size(); }
	const param_values& single() const
	{
		if (data.size() != 1)
			throw std::out_of_range("size()");
		return *this;
	}

	auto begin() const noexcept { return data.begin(); }
	auto end() const noexcept { return data.begin(); }
	auto rbegin() const noexcept { return data.rbegin(); }
	auto rend() const noexcept { return data.rbegin(); }

	static value_store from_str(std::string_view v);
	static std::string to_str(value_type v);
};

class params
{
public:
	params();
	// ~params();

	using value_type = param_values::value_store;

	void assign(std::string_view params, bool parse_list = true);
	void clear();

	const param_values& operator[](std::string_view d) const
	{
		return m_dict.at(d);
	}
	const param_values* try_get(std::string_view d) const
	{
		if (auto it = m_dict.find(d); it != m_dict.end()) {
			return &it->second;
		} else {
			return nullptr;
		}
	}
	const auto& dict() const noexcept { return m_dict; }

protected:
	void setup(std::string_view param, bool parse_list = true);
	void reclaim();

	std::string_view copy_str_(std::string_view s);
	value_type value_str_(std::string_view v);

private:
	std::pmr::monotonic_buffer_resource m_buffer;
	std::pmr::string m_parsed_param;
	std::unordered_map<std::string_view, param_values> m_dict;
};

} // namespace inx::flow::util

#endif // INXFLOW_UTIL_PARAM_HPP
