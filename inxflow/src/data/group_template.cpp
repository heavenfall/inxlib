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

#include <inxflow/data/group_template.hpp>

namespace inx::flow::data {

GroupSignature::GroupSignature(std::string&& name,
                               serialize&& base,
                               const std::pmr::polymorphic_allocator<>* alloc)
  : m_name(std::move(name))
  , m_base(std::move(base))
  , m_alloc(alloc)
{
}

GroupTemplate::GroupTemplate(const std::pmr::polymorphic_allocator<>& var_alloc,
                             std::string_view name,
                             serialize&& base,
                             const std::pmr::polymorphic_allocator<>* alloc)
  : m_signature(std::make_shared<GroupSignature>(std::string(name),
                                                 std::move(base),
                                                 alloc))
  , m_higherScope(nullptr)
  , m_vars(var_alloc)
{
}
GroupTemplate::GroupTemplate(const std::pmr::polymorphic_allocator<>& var_alloc,
                             GroupTemplate& higher)
  : m_signature(higher.m_signature)
  , m_higherScope(&higher)
  , m_vars(var_alloc)
{
}
GroupTemplate::GroupTemplate(GroupTemplate& higher)
  : GroupTemplate(higher.m_vars.get_allocator(), higher)
{
}

Serialize&
GroupTemplate::at(std::string_view id) const
{
	std::lock_guard lock(m_signature->m_mutex);
	m_varTemp = id;
	return *m_vars.at(m_varTemp);
}
serialize
GroupTemplate::get(std::string_view id) const
{
	std::lock_guard lock(m_signature->m_mutex);
	m_varTemp = id;
	auto it = m_vars.find(m_varTemp);
	return it != m_vars.end() ? it->second : serialize();
}

Serialize&
GroupTemplate::at_chain(std::string_view id) const
{
	std::lock_guard lock(m_signature->m_mutex);
	auto obj = get_chain_nolock(id);
	if (obj == nullptr)
		throw std::out_of_range("id");
	return *obj;
}
serialize
GroupTemplate::get_chain(std::string_view id) const
{
	std::lock_guard lock(m_signature->m_mutex);
	return get_chain_nolock(id);
}
serialize
GroupTemplate::get_chain_nolock(std::string_view id) const
{
	m_varTemp = id;
	auto it = m_vars.find(m_varTemp);
	if (it != m_vars.end())
		return it->second;
	if (m_higherScope != nullptr)
		return m_higherScope->get_chain_nolock(id);
	// item does not exists
	return serialize();
}

Serialize&
GroupTemplate::at_make(std::string_view id)
{
	auto ser = get_make(id);
	if (ser == nullptr)
		throw std::out_of_range("id");
	return *ser;
}
serialize
GroupTemplate::get_make(std::string_view id)
{
	std::lock_guard lock(m_signature->m_mutex);
	m_varTemp = id;
	serialize& ser = m_vars[m_varTemp];
	if (ser == nullptr) {
		ser = m_signature->construct_nolock();
	}
	return ser;
}

} // namespace inx::flow::data
