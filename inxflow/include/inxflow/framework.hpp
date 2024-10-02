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
#include "util/string.hpp"
#include <inxlib/inx.hpp>
#include <memory_resource>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
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
		return m_mutRes;
	}

	template <data::concepts::serializable T, typename... Args>
	std::pair<const signature*, bool> emplace_signature(std::string_view name,
	                                                    Args&&... args)
	{
		if (auto it = m_signatures.find(name); it != m_signatures.end())
			return {&it->second, false};
		auto base_obj = std::allocate_shared<T>(
		  std::pmr::polymorphic_allocator<>(&get_mutable_resource()),
		  std::forward<Args>(args)...);
		signature sig = std::allocate_shared<signature::element_type>(
		  std::pmr::polymorphic_allocator<>(&get_immutable_resource()),
		  std::string(name),
		  std::move(base_obj),
		  &get_mutable_resource());
		return push_signature(std::move(sig));
	}
	std::pair<const signature*, bool> push_signature(signature&& sig);
	const signature* get_signature(std::string_view name)
	{
		if (auto it = m_signatures.find(name); it != m_signatures.end())
			return &it->second;
		return nullptr;
	}

	void emplace_scope(std::string_view name, signature&& sig);
	void emplace_scope(std::string_view name, std::string_view sig_name);

	/**
	 * Returns varable var.
	 * Requires an explicit group that exists.
	 * Will create variable name if not exists.
	 * Calls var(l_var)
	 */
	data::Serialize& operator[](util::VarName l_var);
	/**
	 * Calls var(l_var)
	 */
	data::Serialize& operator[](std::string_view l_var);

	/**
	 * Returns varable l_var.
	 * If no group exists and default_group is specified, use that group,
	 * requires group to exists. Will create variable name if not exists.
	 */
	data::Serialize& var(util::VarName l_var,
	                     std::string_view default_group = std::string_view());
	data::Serialize& var(std::string_view l_var,
	                     std::string_view default_group = std::string_view());

	/**
	 * Returns varable var.
	 * If no group exists and default_group is specified, use that group,
	 * requires group to exists. Will not create variable name.
	 */
	data::Serialize& at(util::VarName l_var,
	                    std::string_view default_group = std::string_view());
	data::Serialize& at(std::string_view l_var,
	                    std::string_view default_group = std::string_view());

protected:
	VarScope& get_group(std::string_view l_var,
	                    std::string_view default_group = std::string_view());

protected:
	std::pmr::monotonic_buffer_resource m_immRes;
	std::pmr::synchronized_pool_resource m_mutRes;
	std::vector<std::string> m_arguments;
	std::pmr::unordered_set<std::string> m_strings;
	std::pmr::unordered_map<std::string_view, signature> m_signatures;
	std::pmr::unordered_map<std::string_view, VarScope> m_variables;
};

/**
 * Setup the default layout of framework.
 */
void
framework_default(Framework& fw);

} // namespace inx::flow

#endif // INXFLOW_FRAMEWORK_HPP
