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
#include "factory_adaptor.hpp"

namespace inx::memory {

/**
 * Factory that generates area blocks for area-based factories.
 * Size is static.
 */
template <AreaFactory Upstream, size_t Size = 0, size_t Align = 0>
class block_factory : private Upstream
{
public:
	using upstream_factory = Upstream;
	using value_type = std::byte;
	using pointer = value_type*;
	using size_type = size_t;
	using area = upstream_type::value_type;
protected:
	using size_set = details::area_size_dynamic<Size, Align, area>;

public:
	~block_factory()
	{
		release(true);
	}

	static consteval uint32_t traits() noexcept { return FactoryOwn; }

	static consteval size_type alignment() noexcept { return Align; }
	static consteval size_type element_size() noexcept { return Size; }
	
	template <typename... T>
	constexpr bool setup(T&&... args) requires (!size_set::dynamic)
	{
		return upstream_type::setup(std::forward<T>(args)...);
	}
	template <typename... T>
	constexpr bool setup(block_factory_params param, T&&... args) requires (size_set::dynamic)
	{
		if (!upstream_type::setup(std::forward<T>(args)...))
			return false;
		if (!m_size.set(param.size, param.align))
			return false;
		return true;
	}

	[[nodiscard]] pointer create()
	{
		if (m_currentLeft == 0) [[unlikely]] {
			aquire_area();
		}
		assert(m_currentLeft > 0);
		pointer ptr = reinterpret_cast<pointer>( m_currentArea ) + m_currentPos;
		m_currentPos += m_size.size();
		m_currentLeft -= 1;
		return ptr;
	}
	void destroy(pointer ptr)
	{ }

	void release(bool free_upstream = true)
	{
		if (free_upstream && m_currentArea != nullptr) {
			std::array<area*, 2> relptr{m_currentArea, m_currentArea->h[1].p64};
			for (area* a : relptr) {
				while (a != nullptr) {
					area* anext = static_cast<area*>( a->h[0].p64 );
					delete_area(a);
					a = anext;
				}
			}
		}
		m_currentArea = nullptr;
		m_currentPos = 0;
		m_currentLeft = 0;
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
	area* new_area()
	{
		area* a = upstream_type::create();
		a->h[0].p64 = nullptr; // next-link
		a->h[1].p64 = nullptr; // reclaim-link
	}
	void delete_area(area* a)
	{
		upstream_type::destroy(a);
	}
	void push_area(area* a)
	{
		a->h[0].p64 = std::exchange(m_currentArea, a);
		m_currentPos = area::size_header();
		m_currentLeft = m_size.count();
	}
	/// @brief aquire a new area, checking for reclaimed first, then new_area
	area* aquire_area()
	{
		area* a;
		if (m_currentArea == nullptr || m_currentArea->h[1].p64 == nullptr) {
			a = new_area();
		} else {
			// get reclaimed area
			a = static_cast<area*>( std::exchange(m_currentArea->p[1].p64, nullptr) );
			// move next-link to reclaim-link
			assert(a->h[1].p64 == nullptr);
			a->h[1].p64 = a->h[0].p64;
		}
		// with new area, assign
		push_area(a);
		return a;
	}

protected:
	area* m_currentArea = nullptr;
	uint32_t m_currentPos = 0;
	uint32_t m_currentLeft = 0;
	[[no_unique_address]] size_set m_size;
};

} // namespace inx::memory

#endif // INXLIB_MEMORY_BLOCK_FACTORY_HPP
