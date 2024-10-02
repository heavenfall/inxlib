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

#include <inxflow/data/string_serialize.hpp>
#include <inxflow/framework.hpp>

namespace inx::flow {

namespace {
using namespace std::string_view_literals;
}

VarScope::VarScope(signature&& l_sig,
                   const std::pmr::polymorphic_allocator<>& alloc)
  : sig(std::move(l_sig))
  , global(alloc, signature(sig))
  , local(global)
{
}

std::pair<const signature*, bool>
Framework::push_signature(signature&& sig)
{
	auto it = m_signatures.try_emplace(*m_strings.insert(sig->m_name).first,
	                                   std::move(sig));
	return {&it.first->second, it.second};
}

void
Framework::emplace_scope(std::string_view name, signature&& sig)
{
	m_variables.try_emplace(*m_strings.insert(sig->m_name).first,
	                        std::move(sig),
	                        &get_mutable_resource());
}
void
Framework::emplace_scope(std::string_view name, std::string_view sig_name)
{
	emplace_scope(name, signature(m_signatures.at(sig_name)));
}

VarScope&
Framework::get_group(std::string_view var, std::string_view default_group)
{
	if (var.empty()) {
		var = default_group;
	}
	return m_variables.at(var);
}

data::Serialize&
Framework::operator[](util::VarName l_var)
{
	return var(l_var);
}
data::Serialize&
Framework::operator[](std::string_view l_var)
{
	return var(l_var);
}

data::Serialize&
Framework::var(util::VarName l_var, std::string_view default_group)
{
	auto& group = get_group(l_var.group(), default_group);
	switch (l_var.cls()) {
	case util::VarClass::Global:
		return group.global.at_make(l_var.name());
	case util::VarClass::Local:
		return group.local.at_make(l_var.name());
	default:
		throw std::logic_error("l_var.cls()");
	}
}
data::Serialize&
Framework::var(std::string_view l_var, std::string_view default_group)
{
	return var(util::parse_varname(l_var).first, default_group);
}

data::Serialize&
Framework::at(util::VarName l_var, std::string_view default_group)
{
	auto& group = get_group(l_var.group(), default_group);
	switch (l_var.cls()) {
	case util::VarClass::Global:
		return group.global.at_chain(l_var.name());
	case util::VarClass::Local:
		return group.local.at_chain(l_var.name());
	default:
		throw std::logic_error("l_var.cls()");
	}
}
data::Serialize&
Framework::at(std::string_view l_var, std::string_view default_group)
{
	return at(util::parse_varname(l_var).first, default_group);
}

void
framework_default(Framework& fw)
{
	auto* var_sig =
	  fw.emplace_signature<data::SerializeWrap<data::StringSerialize>>("var"sv)
	    .first;
	assert(var_sig != nullptr);
	fw.emplace_scope("var"sv, signature(*var_sig));
}

} // namespace inx::flow
