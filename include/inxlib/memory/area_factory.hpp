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
#include <inxlib/numeric/bits.hpp>
#include "factory.hpp"
#include "factory_adaptor.hpp"
#include "object.hpp"
#include <memory>

namespace inx::memory {

struct area_memory_header
{ };

template <size_t Size, size_t Align>
struct alignas(max_align_t) area_memory : area_memory_header
{
	static_assert(Size != 0, "Size must be greater than 0.");
	static_assert(inx::numeric::popcount(Align) == 1 && Align <= alignof(max_align_t), "Align must be a valid alignment.");

	union Val {
		uint64_t u64;
		int64_t i64;
		void* p64;
	};
	std::array<Val, 2> h;
	alignas(max_align_t) std::array<std::byte, Size> data[1];

	consteval static size_t size() noexcept { return Size; }
	consteval static size_t align() noexcept { return Align; }

	constexpr static size_t size_header() noexcept
	{
		return offsetof(area_memory, data);
	}
	constexpr static size_t size_n(size_t elems) noexcept
	{
		return pad_alignment(size_header() + elems * Size, alignof(max_align_t));
	}

	template <std::derived_from<area_memory_header> T>
	constexpr T* cast() noexcept { return static_cast<T*>( static_cast<area_memory_header*>(this) ); }
};
template <typename T>
using area_memory_type = area_memory<sizeof(T), alignof(T)>;
using area_memory_bytes = area_memory<1, alignof(max_align_t)>;

struct area_factory_params
{
	area_factory_params() = default;
	area_factory_params(size_t l_count, bool l_use_size = false) : count(l_count), use_size(l_use_size)
	{ }
	size_t count = 0; ///< amount to set element to, 0 = default
	bool use_size = false; ///< if true: element_size(count), else: element_count(count)
};

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

	static consteval uint32_t traits() noexcept { return FactoryDefault; }

	static consteval size_type alignment() noexcept { return Area::align(); }
	static consteval size_type element_size() noexcept { return Area::size_n(AreaCount); }
	static consteval size_type element_count() noexcept { return AreaCount; }

	template <typename... T>
	constexpr bool setup(T&&... args)
	{
		return Upstream::setup(std::forward<T>(args)...);
	}

	pointer create()
	{
		return reinterpret_cast<pointer>( Upstream::allocate(element_size()) );
	}
	void destroy(pointer ptr)
	{
		Upstream::deallocate(reinterpret_cast<typename Upstream::pointer>(ptr), element_size());
	}

	upstream_factory& upstream() noexcept { return static_cast<upstream_factory&>(*this); }
	const upstream_factory& upstream() const noexcept { return static_cast<const upstream_factory&>(*this); }
};
template <ByteFactory Upstream, typename Area>
class area_factory<Upstream, 0, Area> : private Upstream
{
public:
	using upstream_factory = Upstream;
	using value_type = Area;
	using pointer = value_type*;
	using size_type = size_t;

	static consteval size_type alignment() noexcept { return Area::align(); }
	size_type element_size() const noexcept { return m_elementSize; }
	size_type element_count() noexcept { return m_elementCount; }

	constexpr area_factory() noexcept : area_factory(1024)
	{ }
	constexpr area_factory(size_type elem_size) noexcept
	{
		element_size(elem_size);
	}

	template <typename... T>
	bool setup(area_factory_params params, T&&... args)
	{
		if (!Upstream::setup(std::forward<T>(args)...))
			return false;
		if (params.count != 0) {
			if (params.use_size)
				element_size(params.count);
			else
				element_count(params.count);
		}
		return true;
	}

	[[nodiscard]] pointer create()
	{
		return reinterpret_cast<pointer>( Upstream::allocate(element_size()) );
	}
	void destroy(pointer ptr)
	{
		Upstream::deallocate(reinterpret_cast<typename Upstream::pointer>(ptr), element_size());
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

	upstream_factory& upstream() noexcept { return static_cast<upstream_factory&>(*this); }
	const upstream_factory& upstream() const noexcept { return static_cast<const upstream_factory&>(*this); }

protected:
	uint32_t m_elementCount;
	uint32_t m_elementSize;
};

namespace details {
template <typename T>
struct is_AreaFactor : std::bool_constant<false>
{ };
template <typename Upstream, size_t AreaCount, typename Area>
struct is_AreaFactor<area_factory<Upstream, AreaCount, Area>> : std::bool_constant<true>
{ };
}; // namespace details

template <typename T>
concept AreaFactory = details::is_AreaFactor<T>::value;


/// @brief parameters for block_factory setup, when using 
struct block_factory_params
{
	block_factory_params(size_t l_size, size_t l_align) :
		size(l_size), align(l_align)
	{ }
	size_t size;
	size_t align;
};

namespace details {
template <typename Area>
constexpr bool area_valid_elem_size(size_t size, size_t align) noexcept
{
	if (!(std::popcount(align) == 1 && align <= alignof(max_align_t)))
		return false;
	if (!(size > 0 && size % align == 0))
		return false;
	if (!(size % Area::size() == 0))
		return false;
	return true;
}
template <size_t Size, size_t Align, typename Area>
struct area_size_dynamic
{
	static constexpr bool dynamic = false;
	static_assert(std::popcount(Align) == 1 && Align <= alignof(max_align_t), "Must be a valid alignment");
	static_assert(Size > 0 && Size % Align == 0, "Size must be a multiple of Align");
	static_assert(Size % Area::size() == 0, "Size must be a mulitple of Area::size()");
	consteval size_t size() noexcept { return Size; }
	consteval size_t align() noexcept { return Align; }
	consteval size_t count() noexcept { return Area::; }
};
template <typename Area>
struct area_size_dynamic<0, 0, Area>
{
	static constexpr bool dynamic = true;
	constexpr size_t size() noexcept { return m_size; }
	constexpr size_t align() noexcept { return m_align; }

	bool set(uint32_t l_size, uint32_t l_align) noexcept
	{
		if (!area_valid_elem_size<Area>(l_size, l_align))
			return false;
		m_size = l_size;
		m_align = l_align;
		return true;
	}

protected:
	uint32_t m_size = 0;
	uint32_t m_align = 0;
};
} // namespace details

/**
 * Factory that generates area blocks for area-based factories.
 * If AreaCount == 0, area_factory holds a dynamic size.
 */
template <AreaFactory Upstream, ByteFactory Overflow = void_factory>
class bump_factory : private overflow_adaptor<Upstream, Overflow>
{
public:
	using upstream_factory = overflow_adaptor<Upstream, Overflow>;
	using overflow_factory = Overflow;
	using value_type = std::byte;
	using pointer = value_type*;
	using size_type = size_t;
	using area = upstream_factory::value_type;
	using typename upstream_factory::overflow_type;

	~bump_factory()
	{
		release(true);
	}

	static consteval uint32_t traits() noexcept { return FactoryOwn; }

	static consteval size_type alignment() noexcept { return area::align(); }
	static consteval size_type element_size() noexcept { return area::size(); }
	
	template <typename... T>
	constexpr bool setup(T&&... args)
	{
		return upstream_factory::setup(std::forward<T>(args)...);
	}

	[[nodiscard]] pointer allocate(size_type elems)
	{
		return allocate(elems, area::align());
	}
	[[nodiscard]] pointer allocate(size_type elems, size_type align)
	{
		if (elems <= upstream_factory::element_count() * area::size() / 2) [[likely]] {
			return allocate_bump(elems, align);
		} else {
			return allocate_overflow(elems);
		}
	}
	void deallocate(pointer ptr, size_type elems)
	{ }

	void release(bool free_upstream = true)
	{
		if (free_upstream) {
			area* a = m_currentArea;
			while (a != nullptr) {
				area* anext = static_cast<area*>( a->h[1].p64 );
				delete_area_overflow(a);
				a = anext;
			}
		}
		m_bumpPtr = nullptr;
		m_bumpSize = 0;
		m_currentArea = nullptr;
	}
	void reclaim()
	{
		area* a = m_currentArea;
		area* keep = nullptr;
		while (a != nullptr) {
			area* anext = a->h[1].p64;
			if (!keep && a->h[0].u64 == 0) {
				keep = a;
			} else {
				delete_area_overflow(a);
			}
			a = anext;
		}
		m_bumpPtr = nullptr;
		m_bumpSize = 0;
		m_currentArea = keep;
	}

	upstream_factory& upstream() noexcept { return static_cast<upstream_factory&>(*this); }
	const upstream_factory& upstream() const noexcept { return static_cast<const upstream_factory&>(*this); }

	using upstream_factory::overflow;

protected:
	void new_area()
	{
		area* a = upstream_factory::create();
		a->h[0].u64 = 0;
		a->h[1].p64 = static_cast<void*>(m_currentArea);
		m_currentArea = a;
		m_bumpPtr = static_cast<void*>(&a->data[0]);
		m_bumpSize = upstream_factory::element_count() * area::size();
	}
	area* new_overflow(size_type elems)
	{
		assert(elems > (upstream_factory::element_count() >> 1));
		area* a = reinterpret_cast<area*>( upstream_factory::overflow_allocate(area::size_n(elems)) );
		a->h[0].u64 = elems;
		a->h[1].p64 = static_cast<void*>(m_currentArea);
		m_currentArea = a;
		return a;
	}
	void delete_area_overflow(area* a)
	{
		if (size_type s = static_cast<size_type>(a->h[0].u64); s == 0) [[likely]] {
			// area
			upstream_factory::destroy(a);
		} else {
			upstream_factory::overflow_deallocate(reinterpret_cast<std::byte*>(a), area::size_n(s));
		}
	}
	pointer allocate_bump(size_type elems, size_type align)
	{
		assert(elems <= (upstream_factory::element_count() >> 1) && std::popcount(align) == 1 && align <= area::align());
		void* p = align_adjust(align, element_size() * elems, m_bumpPtr, m_bumpSize);
		if (p != nullptr) [[likely]]
			return static_cast<pointer>(p);
		new_area();
		p = align_adjust(align, element_size() * elems, m_bumpPtr, m_bumpSize);
		assert(p != nullptr);
		return static_cast<pointer>(p);
	}
	pointer allocate_overflow(size_type elems)
	{
		// assume expected alignment is less or equal to area::align
		assert(elems > (upstream_factory::element_count() >> 1));
		area* a = new_overflow(elems);
		return reinterpret_cast<pointer>( &a->data[0] );
	}

protected:
	void* m_bumpPtr = nullptr;
	size_t m_bumpSize = 0;
	area* m_currentArea = nullptr;
	[[no_unique_address]] Overflow m_overflow;
};

} // namespace inx::memory

#endif // INXLIB_MEMORY_BUMP_FACTORY_HPP
