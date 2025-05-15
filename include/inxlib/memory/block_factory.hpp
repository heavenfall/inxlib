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

#ifndef INXLIB_MEMORY_BLOCK_FACTORY_HPP
#define INXLIB_MEMORY_BLOCK_FACTORY_HPP

#include <inxlib/inx.hpp>
#include "area_factory.hpp"

namespace inx::memory {

/**
 * Factory that generates area blocks for area-based factories.
 * Size is static.
 */
template <AreaFactory Upstream, ByteFactory Overflow = void_factory>
class block_factory : private Upstream
{
public:
	using upstream_factory = Upstream;
	using overflow_factory = Overflow;
	using value_type = std::byte;
	using pointer = value_type*;
	using size_type = size_t;
	using area = Upstream::value_type;

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
		return Upstream::setup(std::forward<T>(args)...);
	}

	[[nodiscard]] pointer allocate(size_type elems)
	{
		return allocate(elems, area::align());
	}
	[[nodiscard]] pointer allocate(size_type elems, size_type align)
	{
		if (elems <= Upstream::element_count() * area::size() / 2) [[likely]] {
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

	overflow_factory& overflow() noexcept requires(!VoidFactory<Overflow>) { return m_overflow; }
	const overflow_factory& overflow() const noexcept requires(!VoidFactory<Overflow>) { return m_overflow; }

protected:
	void new_area()
	{
		area* a = Upstream::create();
		a->h[0].u64 = 0;
		a->h[1].p64 = static_cast<void*>(m_currentArea);
		m_currentArea = a;
		m_bumpPtr = static_cast<void*>(&a->data[0]);
		m_bumpSize = Upstream::element_count() * area::size();
	}
	area* new_overflow(size_type elems)
	{
		assert(elems > (Upstream::element_count() >> 1));
		area* a;
		if constexpr (VoidFactory<Overflow>) {
			// go upstream base
			a = reinterpret_cast<area*>( Upstream::upstream().allocate(area::size_n(elems)) );
		} else {
			a = reinterpret_cast<area*>( m_overflow.allocate(area::size_n(elems)) );
		}
		a->h[0].u64 = elems;
		a->h[1].p64 = static_cast<void*>(m_currentArea);
		m_currentArea = a;
		return a;
	}
	void delete_area_overflow(area* a)
	{
		if (size_type s = static_cast<size_type>(a->h[0].u64); s == 0) [[likely]] {
			// area
			Upstream::destroy(a);
		} else {
			s = area::size_n(s);
			if constexpr (VoidFactory<Overflow>) {
				// go upstream base
				Upstream::upstream().deallocate(reinterpret_cast<std::byte*>(a), s);
			} else {
				m_overflow.deallocate(reinterpret_cast<std::byte*>(a), s);
			}
		}
	}
	pointer allocate_bump(size_type elems, size_type align)
	{
		assert(elems <= (Upstream::element_count() >> 1) && std::popcount(align) == 1 && align <= area::align());
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
		assert(elems > (Upstream::element_count() >> 1));
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

#endif // INXLIB_MEMORY_BLOCK_FACTORY_HPP
