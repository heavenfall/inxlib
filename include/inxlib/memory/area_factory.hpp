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

#ifndef INXLIB_MEMORY_BUMP_FACTORY_HPP
#define INXLIB_MEMORY_BUMP_FACTORY_HPP

#include <inxlib/inx.hpp>
#include "factory.hpp"
#include "object.hpp"

namespace inx::memory {

template <size_t Size, size_t Align>
struct area_memory alingas(max_align_t)
{
	union {
		uint64_t u64[2];
		int64_t i64[2];
		void* p64[2];
	};
	object_bytes_size<Size, Align> data[1];

	constexpr static size() noexcept { return Size; }
	constexpr static align() noexcept { return Align; }

	constexpr static size_header() noexcept
	{
		return offsetoff(area_memory, data);
	}
	constexpr static size_n(size_t elems) noexcept
	{
		return pad_alignment(offsetoff(area_memory, data[elems]), alingof(max_align_t));
	}
};
template <typename T>
using area_memory_type = area_memory<sizeof(T), alignof(T)>;
using area_memory_bytes = area_memory_type<std::byte>;

/**
 * Factory that generates area blocks for area-based factories.
 * If AreaCount == 0, area_factory holds a dynamic size.
 */
template <ByteFactory Upstream, size_t AreaCount, typename Area = area_memory_bytes>
class area_factory : private Upstream
{
public:
	using upstream_factory = Upstream;
	using value_type = Area;
	using pointer = value_type*;
	using size_type = size_t;

	static consteval size_type alignment() noexcept { return alignof(max_align_t); }
	static consteval size_type element_size() noexcept { return area_memory::size_n(AreaCount); }
	static consteval size_type element_count() noexcept { return AreaCount; }

	pointer create()
	{
		return Upstream::allocate(element_size());
	}
	void destroy(pointer ptr)
	{
		Upstream::deallocate(ptr, element_size());
	}

	upstream_factory& upstream() noexcept { return static_cast<upstream_factory&>(*this); }
	const upstream_factory& upstream() const noexcept { return static_cast<const upstream_factory&>(*this); }
};
template <ByteFactory Upstream, typename Area>
class area_factory<Upstream, 0, Area> : private Upstream
{
public:
	using value_type = Area;
	using pointer = value_type*;
	using size_type = size_t;

	static consteval size_type alignment() noexcept { return alignof(max_align_t); }
	size_type element_size() const noexcept { return m_elementSize; }
	size_type element_count() noexcept { return m_elementCount; }

	constexpr area_factory() noexcept : area_factory(1024)
	{ }
	constexpr area_factory(size_type elem_size) noexcept
	{
		element_size(count);
	}

	pointer create()
	{
		return Upstream::allocate(element_size());
	}
	void destroy(pointer ptr)
	{
		Upstream::deallocate(ptr, element_size());
	}

	void element_size(size_type size)
	{
		size = std::max(size, static_cast<size_type>(Area::size_n(2)));
		m_elementCount = (size - Area::size_header()) / Area::size();
		m_elementSize = Area::size_n(m_elementCount);
	}
	void element_count(size_type count)
	{
		count = std::max(count, static_cast<size_type>(0));
		m_elementCount = count;
		m_elementSize = Area::size_n(count);
		if (m_elementSize < 64) {
			// set to 64
			element_size(64);
		}
	}

protected:
	uint32_t m_elementCount;
	uint32_t m_elementSize;
};

template <template <typename,size_t,typename> class Fact, typename Upstream, size_t AreaCount, typename Area>
concept AreaFactory = std::same_as<Fact<Upstream, AreaCount, Area>, area_factory<Upstream, AreaCount, Area>>;

/**
 * Factory that generates area blocks for area-based factories.
 * If AreaCount == 0, area_factory holds a dynamic size.
 */
template <AreaFactory Upstream, ByteFactory Overflow = void_factory, typename Area = area_memory_bytes>
class bump_factory : private Upstream
{
public:
	using upstream_factory = Upstream;
	using overflow_factory = Overflow;
	using value_type = Area;
	using pointer = value_type*;
	using size_type = size_t;

	static consteval size_type alignment() noexcept { return alignof(max_align_t); }
	static consteval size_type element_size() noexcept { return area_memory::size_n(AreaCount); }
	static consteval size_type element_count() noexcept { return AreaCount; }

	pointer create()
	{
		return Upstream::allocate(element_size());
	}
	void destroy(pointer ptr)
	{
		Upstream::deallocate(ptr, element_size());
	}

	upstream_factory& upstream() noexcept { return static_cast<upstream_factory&>(*this); }
	const upstream_factory& upstream() const noexcept { return static_cast<const upstream_factory&>(*this); }

	overflow_factory& overflow() noexcept requires(!VoidFactory<Overflow>) { return m_overflow; }
	const overflow_factory& overflow() const noexcept requires(!VoidFactory<Overflow>) { return m_overflow; }

protected:
	[[no_unique_address]] Overflow m_overflow;
};

} // namespace inx::memory

#endif // INXLIB_MEMORY_BUMP_FACTORY_HPP
