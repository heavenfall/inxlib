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

#ifndef INXLIB_MEMORY_INDEXED_BLOCK_FACTORY_HPP
#define INXLIB_MEMORY_INDEXED_BLOCK_FACTORY_HPP

#include "_factory.hpp"
#include "factory_adaptor.hpp"

namespace inx::memory {

/// @brief parameters for indexed_block_factory setup, when using
struct indexed_block_factory_params
{
	constexpr indexed_block_factory_params(size_t l_elements, size_t l_size, size_t l_align)
	  : elements(l_elements)
	  , size(l_size)
	  , align(l_align)
	{
	}
	size_t elements; ///< number of elements in each area, area must support elements*size within align
	size_t size; ///< size of each single item
	size_t align; ///< alignment of each single item
};

/**
 * SingleFactory that sections off area blocks into set sized allocations.
 * Similar to block_factory, except this one is indexable by pointer to area buffers.
 * Use resize() to change number of elements, or use create() to append one additional element.
 * Use item(i) to select the ith element, only assert checks for range thus undefined if out-of-range.
 * 
 * If Size == 0: size is dynamically determined through setup.
 * Otherwise: set size and align at compile time.
 * Elements == 0 makes elements defined, setup() returns false if elements do not fit on area.
 * 
 * @tparam Overflow the memory source of pointer to areas once greater than SlabCount
 * @tparam SlabCount the number of areas to store in-class, takes 8*SlabCount of class size, supports max
 *         of SlabCount * Elements of elements before invoking overflow allocations.
 */
template <SlabFactory Upstream, ByteFactory Overflow = void_factory, size_t SlabCount = 0, size_t Elements = 0, size_t Size = 0, size_t Align = 0>
class indexed_block_factory : private slab_link_pattern<overflow_pattern<Upstream, Overflow>>
{
	using pattern = slab_link_pattern<overflow_pattern<Upstream, Overflow>>;
	using size_set = slab_reshape<Upstream, Size, Align>;

public:
	using upstream_factory = Upstream;
	using value_type = std::byte;
	using pointer = value_type*;
	using size_type = size_t;
	using area = upstream_factory::value_type;

public:
	static consteval uint32_t traits() noexcept { return FactoryOwn | FactoryNoFree; }

	static consteval size_type alignment() noexcept { return Align; }
	static consteval size_type element_size() noexcept { return Size; }

	/// @brief setup(std::tuple<OverflowParams>, ...)
	template <typename... T>
	constexpr bool setup(T&&... args)
	    requires(!size_set::dynamic)
	{
		return pattern::setup(std::forward<T>(args)...);
	}
	/// @brief setup(block_factory_params, std::tuple<OverflowParams>, ...)
	template <typename... T>
	constexpr bool setup(block_factory_params param, T&&... args)
	    requires(size_set::dynamic)
	{
		if (!pattern::setup(std::forward<T>(args)...))
			return false;
		if (!m_size.set(param.size, param.align))
			return false;
		return true;
	}

	[[nodiscard]] pointer create()
	{
		if (m_currentLeft == 0) [[unlikely]] {
			new_root();
		}
		assert(m_currentLeft > 0);
		pointer ptr = reinterpret_cast<pointer>(pattern::root()) + m_currentPos;
		m_currentPos += m_size.size();
		m_currentLeft -= 1;
		return ptr;
	}
	void destroy(pointer ptr) {}

	/// @brief only releases memory calimed for reuse
	/// @param free_upstream destorys memory upstream
	void release(bool free_upstream = true)
	{
		pattern::release(free_upstream);
		m_currentPos = 0;
		m_currentLeft = 0;
	}
	void reclaim()
	{
		pattern::reclaim();
		m_currentPos = 0;
		m_currentLeft = 0;
	}

	upstream_factory& upstream() noexcept { return static_cast<upstream_factory&>(*this); }
	const upstream_factory& upstream() const noexcept { return static_cast<const upstream_factory&>(*this); }

protected:
	void new_root() noexcept
	{
		pattern::create();
		m_currentPos = m_size.header();
		m_currentLeft = m_size.count(upstream());
	}

protected:
	uint32_t m_currentPos = 0;
	uint32_t m_currentLeft = 0;
	[[no_unique_address]] size_set m_size;
	std::array<pointer, 1+SlabCount> m_areas = {};
};

template <typename Upstream, typename T>
using block_factory_type = block_factory<Upstream, sizeof(T), alignof(T)>;

} // namespace inx::memory

#endif // INXLIB_MEMORY_INDEXED_BLOCK_FACTORY_HPP
