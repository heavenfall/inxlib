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

#include <charconv>
#include <inxflow/exceptions.hpp>
#include <inxflow/util/params.hpp>

namespace inx::flow::util {

auto
param_values::from_str(std::string_view v) -> value_store
{
	auto vend = v.data() + v.size();
	value_store value;
	if (auto r = std::from_chars(v.data(), vend, value.v.i);
	    r.ec == std::errc{} && r.ptr == vend) {
		value.length = -1;
	} else if (r = std::from_chars(v.data(), vend, value.v.f);
	           r.ec == std::errc{} && r.ptr == vend) {
		value.length = -2;
	} else {
		value.length = v.size();
		value.v.c = v.data();
	}
	return value;
}

std::string
param_values::to_str(value_type v)
{
	struct Visitor
	{
		std::string operator()(int64 x) const { return std::to_string(x); }
		std::string operator()(double x) const { return std::to_string(x); }
		std::string operator()(std::string_view x) const
		{
			return std::string(x);
		}
	};
	return std::visit<std::string>(Visitor(), v);
}

params::params() {}
// ParameterDict::~ParameterDict()
// {
// 	reclaim();
// }

void
params::assign(std::string_view params)
{
	setup(params);
}
void
params::clear()
{
	reclaim();
}

void
params::setup(std::string_view param)
{
	using sv = std::string_view;
	if (!m_parsed_param.empty() || !m_dict.empty()) {
		reclaim(); // reset memory layout
	}
	m_parsed_param.reserve(param.size());
	sv at = param;
	if (at.empty())
		return;
	std::vector<param_values::value_store> values;
	std::string param_row;
	std::string param_val;
	std::string param_sub;
	auto delimit_string =
	  [](std::string& push, std::string_view at, char c, bool keep) -> size_t {
		size_t pos = 0;
		do {
			if (at[pos] == c)
				return pos;
			if (at[pos] == '\\') {
				if (++pos >= at.size())
					throw parse_error(
					  "params \'\\\' not followed by any character");
				switch (char x = at[pos]; x) {
				case '\\':
				case ':':
				case ',':
					if (keep)
						push.push_back('\\');
					push.push_back(x);
					break;
				default:
					throw parse_error(
					  "params \'\\\' followed by invalid delimit");
				}
			} else {
				push.push_back(at[pos]);
			}
		} while (++pos < at.size());
		return std::string_view::npos;
	};
	while (!at.empty()) {
		param_row.clear();
		auto pos = delimit_string(param_row, at, ',', true);
		sv param = param_row;
		at.remove_prefix(pos == sv::npos ? at.size() : pos + 1);
		if (param.empty())
			continue;
		pos = param.find('=');
		sv value;
		if (pos != sv::npos)
			value = param.substr(0, pos++);
		else
			pos = 0;
		if (m_dict.find(value) != m_dict.end())
			throw parse_error(std::string("duplicate key detected: ") +
			                  std::string(value));
		auto&& [V, ins] = m_dict.try_emplace(copy_str_(value));
		assert(ins);
		if (!ins)
			continue;
		values.clear();
		param.remove_prefix(pos);
		// repeat, seperating each : into element in list
		param_val.clear();
		while (!param.empty()) {
			value_type v;
			// store string length into i, later will convert to string
			v.v.i = param_val.size();
			pos = delimit_string(param_val, param, ':', false);
			v.length = param_val.size() - v.v.i;
			values.push_back(v);
			if (pos != sv::npos) {
				param.remove_prefix(pos + 1);
				param_val.push_back(':');
			} else {
				param = sv();
			}
		}
		// store values into result
		sv s = (V->second.full_str = copy_str_(param_val));
		if (!values.empty()) {
			assert(!s.empty());
			// setup values to real value
			auto size = values.size();
			for (auto& v : values) {
				// convert value_type to expected owned value type
				sv value_str(s.data() + v.v.i, v.length);
				v = param_values::from_str(value_str);
			}
			value_type* ptr = static_cast<value_type*>(m_buffer.allocate(
			  size * sizeof(value_type), alignof(value_type)));
			std::uninitialized_copy_n(values.data(), size, ptr);
			V->second.data = param_values::list_type(const_cast<const value_type*>(ptr), size);
		}
	}
}
void
params::reclaim()
{
	// for (auto& [name, value] : m_dict) {
	// 	if (auto* ptr = value.data.data(); ptr != nullptr) {
	// 		m_buffer->deallocate(const_cast<value_type*>(ptr), value.data.size()
	// * sizeof(value_type), alignof(value_type)); 		value.data = {};
	// 	}
	// }
	m_dict.clear();
	m_parsed_param = std::pmr::string();
	m_buffer.release();
	m_parsed_param = std::pmr::string(&m_buffer);
}

std::string_view
params::copy_str_(std::string_view s)
{
	auto ssize = s.size();
	if (ssize == 0)
		return std::string_view();
	auto psize = m_parsed_param.size();
	if (psize + ssize > m_parsed_param.capacity())
		throw parse_error("params::copy_str_ over capacity");
	m_parsed_param.insert(m_parsed_param.end(), s.begin(), s.end());
	return std::string_view(m_parsed_param.data() + psize, ssize);
}

auto
params::value_str_(std::string_view v) -> value_type
{
	value_type value = param_values::from_str(v);
	if (value.length >= 0 && value.v.c != nullptr) {
		value.v.c = copy_str_(v).data();
	}
	return value;
}

} // namespace inx::flow::util
