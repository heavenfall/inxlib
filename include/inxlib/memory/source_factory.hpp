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

#ifndef INXLIB_MEMORY_SOURCE_FACTORY_HPP
#define INXLIB_MEMORY_SOURCE_FACTORY_HPP

#include <inxlib/inx.hpp>
#include "factory.hpp"
#include <cstdlib>
#include <cstddef>
#include <memory_resource>

namespace inx::memory {

class malloc_factory
{
public:
	using value_type = std::byte;
	using pointer = value_type*;
	using size_type = size_t;

	static consteval uint32_t traits() noexcept { return FactorySource; }

	constexpr malloc_factory() noexcept = default;
	malloc_factory(const malloc_factory&) = delete;

	constexpr bool setup() noexcept
	{
		return true;
	}

	static consteval size_type alignment() noexcept { return alignof(max_align_t); }
	static consteval size_type element_size() noexcept { return 1; }

	[[nodiscard]] pointer allocate(size_type elems)
	{
		return static_cast<pointer>(std::malloc(elems));
	}
	void deallocate(pointer ptr)
	{
		std::free(ptr);
	}
	void deallocate(pointer ptr, size_type)
	{
		deallocate(ptr);
	}
};

class memory_resource_factory
{
public:
	using value_type = std::byte;
	using pointer = value_type*;
	using size_type = size_t;

	/// @brief memory_resource_factory may not align properly with these underlying traits
	static consteval uint32_t traits() noexcept { return FactorySource; }

	constexpr memory_resource_factory() noexcept = default;
	memory_resource_factory(const memory_resource_factory&) = delete;

	constexpr bool setup(std::pmr::memory_resource* res) noexcept
	{
		if (res == nullptr)
			return false;
		m_memory_resouce = res;
		return true;
	}

	static consteval size_type alignment() noexcept { return alignof(max_align_t); }
	static consteval size_type element_size() noexcept { return 1; }

	[[nodiscard]] pointer allocate(size_type elems)
	{
		return static_cast<pointer>(m_memory_resouce->allocate(elems)); // alignment() should be the same as default
	}
	[[nodiscard]] pointer allocate(size_type elems, size_type align)
	{
		return static_cast<pointer>(m_memory_resouce->allocate(elems, align)); // alignment() should be the same as default
	}
	void deallocate(pointer ptr, size_type elems)
	{
		m_memory_resouce->deallocate(ptr, elems);
	}
	void deallocate(pointer ptr, size_type elems, size_type align)
	{
		m_memory_resouce->deallocate(ptr, elems, align);
	}

	std::pmr::memory_resource* get_memory_resource() const noexcept { return m_memory_resouce; }

private:
	std::pmr::memory_resource* m_memory_resouce = nullptr;
};

static_assert(ArrayFactory<malloc_factory>, "malloc_factory must be a valid Factory");

} // namespace inx::memory

#endif // INXLIB_MEMORY_SOURCE_FACTORY_HPP
