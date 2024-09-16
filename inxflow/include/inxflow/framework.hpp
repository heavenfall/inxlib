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

#ifndef INXFLOW_FRAMEWORK_HPP
#define INXFLOW_FRAMEWORK_HPP

#include "data/group_template.hpp"
#include "data/string_serialize.hpp"
#include <inxlib/inx.hpp>
#include <memory_resource>
#include <ranges>
#include <vector>

namespace inx::flow {

struct VarScope
{
	VarScope(signature&& l_sig, const std::pmr::polymorphic_allocator<>& alloc);
	signature sig;
	data::GroupTemplate global;
	data::GroupTemplate local;
};

class Framework
{
public:
	void set_args_main(int argc, char* argv[])
	{
		if (argc < 1)
			throw std::out_of_range("argc");
		m_arguments.assign(argv + 1, argv + argc);
	}
	template <std::ranges::input_range G>
	void set_args_range(G&& args)
	{
		m_arguments.assign(args.begin(), args.end());
	}

	/// @brief A memory_resource that is never freed, not thread safe
	/// @return monotonic_buffer_resource
	std::pmr::memory_resource& get_immutable_resource() noexcept
	{
		return m_immRes;
	}
	/// @brief A memory_resource that is never freed, thread safe
	/// @return synchronized_pool_resource
	std::pmr::memory_resource& get_mutable_resource() noexcept
	{
		return m_immRes;
	}

	template <data::concepts::serializable T, typename... Args>
	const signature& emplace_signature(std::string_view name, Args&&... args)
	{
		signature sig = std::allocate_shared<T>(std::forward<Args>(args)...);
		return push_signature(name, std::move(sig));
	}
	const signature& push_signature(std::string_view name, signature&& sig);

protected:
	std::pmr::monotonic_buffer_resource m_immRes;
	std::pmr::synchronized_pool_resource m_mutRes;
	std::vector<std::string> m_arguments;
	std::pmr::unordered_map<std::string, signature> m_signatures;
	std::pmr::unordered_map<std::string, VarScope> m_variables;
};

void
framework_data_default(Framework& fw);

} // namespace inx::flow

#endif // INXFLOW_FRAMEWORK_HPP
