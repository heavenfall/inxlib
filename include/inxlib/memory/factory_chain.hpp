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

template <ArrayFactory Fact>
struct factory_pointer<Fact>
{
	using factory_type = Fact;
	factory_type* m_factoryBase = nullptr;
	using typename factory_type::value_type;
	using typename factory_type::pointer;
	using typename factory_type::size_type;

	constexpr size_type alignment() noexcept { return m_factoryBase->alignment(); }
	constexpr size_type element_size() noexcept { return m_factoryBase->element_size(); }

	pointer allocate(size_type elems)
	{
		return m_factoryBase->allocate(elems);
	}
	pointer deallocate(pointer ptr, size_type elems)
	{
		return m_factoryBase->deallocate(ptr, elems);
	}
	pointer deallocate(pointer ptr) requires requires { { m_factoryBase->deallocate(ptr) }; }
	{
		return m_factoryBase->deallocate(ptr);
	}
};

template <SingleFactory Fact>
struct factory_pointer<Fact>
{
	using factory_type = Fact;
	factory_type* m_factoryBase = nullptr;
	using typename factory_type::value_type;
	using typename factory_type::pointer;
	using typename factory_type::size_type;

	constexpr size_type alignment() noexcept { return m_factoryBase->alignment(); }
	constexpr size_type element_size() noexcept { return m_factoryBase->element_size(); }

	pointer create()
	{
		return m_factoryBase->create();
	}
	pointer destroy(pointer ptr)
	{
		return m_factoryBase->destroy(ptr);
	}
};

} // namespace inx::memory

#endif // INXLIB_MEMORY_FACTORY_CHAIN_HPP
