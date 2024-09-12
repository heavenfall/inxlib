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
#include <mutex>

namespace inx::flow::data {

/// @brief Contains the information required to generate a Serialize type from
/// group string
class GroupSignature
{
public:
	std::string_view name() const noexcept { return m_name; }
	const Serialize& base() const noexcept { return *m_base; }

	[[nodiscard]]
	serialize construct()
	{
		std::lock_guard lock(m_mutex);
		return m_base->construct_new(m_alloc);
	}
	[[nodiscard]]
	serialize construct_nolock()
	{
		return m_base->construct_new(m_alloc);
	}

	GroupSignature(std::string&& name,
	               serialize&& base,
	               const std::pmr::polymorphic_allocator<>* alloc = nullptr);

	std::string m_name;
	serialize m_base;
	const std::pmr::polymorphic_allocator<>* m_alloc;
	std::mutex m_mutex;
};

/// @brief Variable scope of GroupSignature
class GroupTemplate
{
public:
	GroupTemplate(const std::pmr::polymorphic_allocator<>& var_alloc,
	              std::string_view name,
	              serialize&& base,
	              const std::pmr::polymorphic_allocator<>* alloc = nullptr);
	GroupTemplate(const std::pmr::polymorphic_allocator<>& var_alloc,
	              GroupTemplate& higher);
	GroupTemplate(GroupTemplate& higher);

	Serialize& at(std::string_view id)
	  const; /// finds id, raises std::out_of_range if key does not exist
	serialize get(std::string_view id)
	  const; /// finds id, returns null shared_ptr if key does not exist

	Serialize& at_chain(std::string_view id)
	  const; /// finds id, chains to higher scope if does not exists
	serialize get_chain(std::string_view id)
	  const; /// finds id, chains to higher scope if does not exists

	Serialize& at_make(
	  std::string_view id); /// finds id, constructs if does not exists on scope
	serialize get_make(
	  std::string_view id); /// finds id, constructs if does not exists on scope

	const auto& data() const noexcept { return m_vars; }

private:
	serialize get_chain_nolock(std::string_view id)
	  const; /// finds id, chains to higher scope if does not exists

protected:
	std::shared_ptr<GroupSignature> m_signature; /// Object signature
	GroupTemplate* m_higherScope; /// higher variable scope (global), if present
	std::pmr::unordered_map<std::pmr::string, serialize> m_vars;
	std::pmr::string mutable m_varTemp;
};

} // namespace inx::flow::data

#endif // INXFLOW_DATA_GROUP_TEMPLATE_HPP
