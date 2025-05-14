/*
MIT License

Copyright (c) 2025 Ryan Hechenberger

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

#ifndef INXLIB_MEMORY_FACTORY_CHAIN_HPP
#define INXLIB_MEMORY_FACTORY_CHAIN_HPP

#include <inxlib/inx.hpp>
#include "factory.hpp"
#include <memory_resource>

namespace inx::memory {

template <typename Fact>
struct factory_pointer;

template <Factory Fact>
struct factory_pointer<Fact>
{
	using factory_type = Fact;
	factory_type* m_factoryBase = nullptr;
	using value_type = typename factory_type::value_type;
	using pointer = typename factory_type::pointer;
	using size_type = typename factory_type::size_type;

	factory_pointer() = default;
	factory_pointer(const factory_pointer&) = delete;

	static constexpr size_type alignment() noexcept requires requires { Fact::alignment(); } { return Fact::alignment(); }
	constexpr size_type alignment() noexcept requires (!requires { Fact::alignment(); }) { return m_factoryBase->alignment(); }
	static constexpr size_type element_size() noexcept requires requires { Fact::element_size(); } { return Fact::element_size(); }
	constexpr size_type element_size() noexcept requires (!requires { Fact::element_size(); }) { return m_factoryBase->element_size(); }

	constexpr void setup(factory_type& upstream)
	{
		m_factoryBase = &upstream;
	}

	[[nodiscard]] pointer allocate(size_type elems) requires ArrayFactory<Fact>
	{
		return m_factoryBase->allocate(elems);
	}
	void deallocate(pointer ptr, size_type elems) requires ArrayFactory<Fact>
	{
		return m_factoryBase->deallocate(ptr, elems);
	}
	void deallocate(pointer ptr) requires ArrayFactory<Fact> && requires { { m_factoryBase->deallocate(ptr) }; }
	{
		return m_factoryBase->deallocate(ptr);
	}

	[[nodiscard]] pointer create() requires SingleFactory<Fact>
	{
		return m_factoryBase->create();
	}
	[[nodiscard]] pointer destroy(pointer ptr) requires SingleFactory<Fact>
	{
		return m_factoryBase->destroy(ptr);
	}

	void release(bool free_upstream = true) requires requires { m_factoryBase->release(free_upstream); }
	{
		m_factoryBase->release(free_upstream);
	}
	void reclaim() requires requires { m_factoryBase->reclaim(); }
	{
		m_factoryBase->reclaim();
	}
};

} // namespace inx::memory

#endif // INXLIB_MEMORY_FACTORY_CHAIN_HPP
