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

#include <inxflow/data/serialize.hpp>

namespace inx::flow::data {

Serialize::Serialize(std::type_index type, wrapper_fn* fn, serialize_construct* dup)
  : m_type(type)
  , m_operators(fn)
  , m_duplicate(dup)
{
	assert(fn != nullptr && dup != nullptr);
}

Serialize::Serialize(const Serialize& other)
  : Serialize(other.m_type, other.m_operators, other.m_duplicate)
{
	if (other.m_data && supported(wrapper_op::Copy)) {
		wrapper_input send{wrapper_op::Copy, {}, {}, &m_data, {.v = other.m_data.get()}, {}};
		(*m_operators)(send);
	}
}
Serialize::Serialize(Serialize&& other)
  : Serialize(other.m_type, other.m_operators, other.m_duplicate)
{
	if (other.m_data && supported(wrapper_op::Move)) {
		wrapper_input send{wrapper_op::Copy, {}, {}, &m_data, {.v = other.m_data.get()}, {}};
		(*m_operators)(send);
	}
}
Serialize::Serialize(const Serialize& other, bool copy)
  : Serialize(other.m_type, other.m_operators, other.m_duplicate)
{
	if (copy) {
		if (!supported(wrapper_op::Copy))
			throw std::logic_error("unsupported");
		if (other.m_data) {
			wrapper_input send{wrapper_op::Copy, {}, {}, &m_data, {.v = other.m_data.get()}, {}};
			(*m_operators)(send);
		}
	}
}

void
Serialize::copy_(const void* other)
{
	if (other == nullptr || !supported(wrapper_op::Copy))
		throw std::logic_error("unsupported");
	wrapper_input send{wrapper_op::Copy, {}, {}, &m_data, {.v = const_cast<void*>(other)}, {}};
	(*m_operators)(send);
}

void
Serialize::move_(void* other)
{
	bool do_move = supported(wrapper_op::Move);
	bool do_copy = !do_move && supported(wrapper_op::Copy);
	if (other == nullptr || !do_move || !do_copy)
		throw std::logic_error("unsupported");
	if (do_copy) [[unlikely]] {
		wrapper_input send{wrapper_op::Copy, {}, {}, &m_data, {.v = const_cast<void*>(other)}, {}};
		(*m_operators)(send);
	} else {
		wrapper_input send{wrapper_op::Move, {}, {}, &m_data, {.v = const_cast<void*>(other)}, {}};
		(*m_operators)(send);
	}
}

bool
Serialize::supported(wrapper_op op) const
{
	wrapper_input send{wrapper_op::Support, op, {}, {}, {}, {}};
	return (*m_operators)(send);
}

serialize
Serialize::construct_new(const std::pmr::polymorphic_allocator<>& alloc) const
{
	auto obj = m_duplicate(&alloc);
	wrapper_input send{wrapper_op::Construct, {}, {}, &obj->m_data, {}, {}};
	bool success [[maybe_unused]] = (*obj->m_operators)(send);
	assert(success);
	return obj;
}

Serialize&
Serialize::operator=(const Serialize& other)
{
	if (m_type != other.m_type)
		throw std::logic_error("type mismatch");
	if (auto* g = other.m_data.get(); g == nullptr) {
		clear();
	} else {
		copy_(g);
	}
	return *this;
}

Serialize&
Serialize::operator=(Serialize&& other)
{
	if (m_type != other.m_type)
		throw std::logic_error("type mismatch");
	if (auto* g = other.m_data.get(); g == nullptr) {
		clear();
	} else {
		move_(g);
	}
	return *this;
}

void
Serialize::clear()
{
	m_data = nullptr;
}

void
Serialize::load(std::istream* in, const std::filesystem::path& fname, StreamType type)
{
	wrapper_input send{wrapper_op::Load, wrapper_op{}, type, &m_data, {.i = in}, &fname};
	(*m_operators)(send);
}
void
Serialize::load(std::istream& in)
{
	load(&in, {}, StreamType::Stream);
}
void
Serialize::load(const std::filesystem::path& fname)
{
	load(nullptr, fname, StreamType::File);
}
void
Serialize::load(std::istream& in, const std::filesystem::path& fname)
{
	load(&in, fname, StreamType::Stream);
}
void
Serialize::load_stdin()
{
	load(nullptr, {}, StreamType::StdIn);
}
void
Serialize::load_null()
{
	load(nullptr, {}, StreamType::DevNull);
}

void
Serialize::save(std::ostream* out, const std::filesystem::path& fname, StreamType type)
{
	wrapper_input send{wrapper_op::Save, wrapper_op{}, type, &m_data, {.o = out}, &fname};
	(*m_operators)(send);
}
void
Serialize::save(std::ostream& in)
{
	save(&in, {}, StreamType::Stream);
}
void
Serialize::save(const std::filesystem::path& fname)
{
	save(nullptr, fname, StreamType::File);
}
void
Serialize::save(std::ostream& in, const std::filesystem::path& fname)
{
	save(&in, fname, StreamType::Stream);
}
void
Serialize::save_stdout()
{
	save(nullptr, {}, StreamType::StdOut);
}
void
Serialize::save_null()
{
	save(nullptr, {}, StreamType::DevNull);
}

} // namespace inx::flow::data
