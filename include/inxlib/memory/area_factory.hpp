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

#ifndef INXLIB_MEMORY_AREA_FACTORY_HPP
#define INXLIB_MEMORY_AREA_FACTORY_HPP

#include "factory.hpp"
#include "factory_adaptor.hpp"
#include "object.hpp"

#include <inxlib/numeric/bits.hpp>

#include <memory>

namespace inx::memory {

struct area_memory_header
{};

template <size_t Size, size_t Align>
struct alignas(max_align_t) area_memory : area_memory_header
{
	static_assert(Size != 0, "Size must be greater than 0.");
	static_assert(inx::numeric::popcount(Align) == 1 && Align <= alignof(max_align_t),
	              "Align must be a valid alignment.");

	union Val
	{
		uint64_t u64;
		int64_t i64;
		void* p64;
	};
	std::array<Val, 2> h;
	alignas(max_align_t) std::array<std::byte, Size> data[1];

	consteval static size_t size() noexcept { return Size; }
	consteval static size_t align() noexcept { return Align; }

	consteval static size_t size_header() noexcept { return offsetof(area_memory, data); }
	constexpr static size_t size_n(size_t elems) noexcept
	{
		return pad_alignment(size_header() + elems * Size, alignof(max_align_t));
	}

	template <std::derived_from<area_memory_header> T>
	constexpr T* cast() noexcept
	{
		return static_cast<T*>(static_cast<area_memory_header*>(this));
	}
};

template <typename T>
using area_memory_type = area_memory<sizeof(T), alignof(T)>;
using area_memory_bytes = area_memory<1, alignof(max_align_t)>;

/// @brief Determine the size (in bytes) each area slab requires for number of Elements of Size in Area
/// (area_memory_type,area_memory_bytes)
/// @tparam Area
template <size_t Size, typename Area = area_memory_bytes>
constexpr size_t
calc_area_size(size_t Elements) noexcept
{
	return Area::size_n(Elements);
}

/// @brief Determine the size (in bytes) of area_memory_type<ElementType> with number of Elements
template <typename ElementType>
constexpr size_t
calc_area_size_type(size_t Elements) noexcept
{
	return area_memory_type<ElementType>::size_n(Elements);
}

struct area_factory_params
{
	area_factory_params() = default;
	area_factory_params(size_t l_count, bool l_use_size = false)
	  : count(l_count)
	  , use_size(l_use_size)
	{
	}
	size_t count = 0;      ///< amount to set element to, 0 = default
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
	static consteval size_type item_size() noexcept { return Area::size(); }
	static consteval size_type item_count() noexcept { return AreaCount; }

	using Upstream::setup;
	// template <typename... T>
	// constexpr bool setup(T&&... args)
	// {
	// 	return Upstream::setup(std::forward<T>(args)...);
	// }

	pointer create() { return reinterpret_cast<pointer>(Upstream::allocate(element_size())); }
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

	static consteval uint32_t traits() noexcept { return FactoryDefault; }

	static consteval size_type alignment() noexcept { return Area::align(); }
	size_type element_size() const noexcept { return m_elementSize; }
	static consteval size_type item_size() noexcept { return Area::size(); }
	size_type item_count() noexcept { return m_elementCount; }

	constexpr area_factory() noexcept
	  : area_factory(1024)
	{
	}
	constexpr area_factory(size_type elem_size) noexcept { element_size(elem_size); }

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

	[[nodiscard]] pointer create() { return reinterpret_cast<pointer>(Upstream::allocate(element_size())); }
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
{};
template <typename Upstream, size_t AreaCount, typename Area>
struct is_AreaFactor<area_factory<Upstream, AreaCount, Area>> : std::bool_constant<true>
{};
template <typename Upstream>
    requires FactoryTraitAll<Upstream, FactoryPointer> && FactoryTraitNone<Upstream, FactorySource>
struct is_AreaFactor<Upstream> : is_AreaFactor<typename Upstream::upstream_factory>
{};
}; // namespace details

template <typename T>
concept AreaFactory = details::is_AreaFactor<T>::value;

/// @brief Add support to area_factory to manage a forward list of areas.
///        O(1) reclaim operations.
///        area.h[0] and area.h[1] are managed by this adaptor.
/// @tparam Upstream
template <AreaFactory Upstream>
    requires FactoryTraitNone<Upstream, FactoryOwn>
class area_link_pattern : public Upstream
{
public:
	using typename Upstream::pointer;
	using typename Upstream::size_type;
	using typename Upstream::value_type;

	static consteval uint32_t traits() noexcept { return FactoryOwn | FactoryReuse; }

	using Upstream::alignment;
	using Upstream::element_size;

	~area_link_pattern() { release(factory_chain_free_release<Upstream>); }

	pointer create()
	{
		pointer res;
		if (m_reuse) {
			res = pop_reuse();
		} else {
			// allocate new
			res = Upstream::create();
		}
		push_front(res);
		return res;
	}
	void destroy(pointer ptr)
	{
		// reuse for later
		remove_from_list(ptr);
		push_reuse(ptr);
	}

	/// @brief only releases memory calimed for reuse
	/// @param free_upstream destorys memory upstream
	void release(bool free_upstream = true)
	{
		if (free_upstream) {
			release_list(m_root);
			pointer at = m_reuse;
			while (at != nullptr) {
				release_list(reinterpret_cast<pointer>(at->h[0].p64));
				pointer next = reinterpret_cast<pointer>(at->h[1].p64);
				Upstream::destroy(at);
				at = next;
			}
		}
		m_root = nullptr;
		m_reuse = nullptr;
	}
	void reclaim()
	{
		if (m_root != nullptr) {
			push_reuse_list(m_root);
			m_root = nullptr;
		}
	}

protected:
	pointer root() noexcept { return m_root; }
	void push_front(pointer at) noexcept
	{
		if (m_root) [[likely]] {
			at->h[0].p64 = m_root;
			m_root->h[1].p64 = at;
		} else {
			at->h[0].p64 = nullptr;
		}
		at->h[1].p64 = nullptr;
		m_root = at;
	}
	void remove_from_list(pointer at) noexcept
	{
		if (at == m_root) [[unlikely]] {
			pointer anext = reinterpret_cast<pointer>(at->h[0].p64);
			m_root = anext;
			if (anext)
				anext->h[1].p64 = nullptr;
		} else {
			// double link
			pointer anext = reinterpret_cast<pointer>(at->h[0].p64);
			pointer aprev = reinterpret_cast<pointer>(at->h[1].p64);
			assert(aprev != nullptr); // this is not root
			aprev->h[0].p64 = anext;
			if (anext)
				anext->h[1].p64 = aprev;
		}
	}
	void push_reuse(pointer at) noexcept
	{
		at->h[0].p64 = nullptr;
		at->h[1].p64 = m_reuse;
		m_reuse = at;
	}
	void push_reuse_list(pointer front) noexcept
	{
		front->h[1].p64 = m_reuse;
		m_reuse = front;
	}
	[[nodiscard]] pointer pop_reuse() noexcept
	{
		assert(m_reuse != nullptr);
		// reuse
		pointer res = m_reuse;
		if (pointer rnext = reinterpret_cast<pointer>(res->h[0].p64); rnext != nullptr) {
			// move next to reuse
			rnext->h[1].p64 = res->h[1].p64;
			m_reuse = rnext;
		} else {
			m_reuse = reinterpret_cast<pointer>(res->h[1].p64);
		}
		return res;
	}
	void release_list(pointer at)
	{
		while (at != nullptr) {
			pointer next = reinterpret_cast<pointer>(at->h[0].p64);
			Upstream::destroy(at);
			at = next;
		}
	}

protected:
	pointer m_root = nullptr;
	pointer m_reuse = nullptr;
};

namespace details {
template <typename Area>
constexpr bool
area_valid_elem_size(size_t size, size_t align) noexcept
{
	if (!(std::popcount(align) == 1 && align <= alignof(max_align_t)))
		return false;
	if (!(size > 0 && size % align == 0))
		return false;
	if (!(size % Area::size() == 0))
		return false;
	return true;
}
} // namespace details

/// @brief params for reshaping AreaFactory
/// @tparam Fact
/// @tparam Size
/// @tparam Align
template <AreaFactory Fact, size_t Size, size_t Align, size_t MinSize = 1>
struct area_reshape
{
	static constexpr bool dynamic = false;
	static_assert(std::popcount(Align) == 1 && Align <= alignof(max_align_t), "Must be a valid alignment");
	static_assert(Size > 0 && Size % Align == 0, "Size must be a multiple of Align");
	static_assert(Size % Fact::value_type::size() == 0, "Size must be a mulitple of Area::size()");
	constexpr size_t header() noexcept { return Fact::value_type::size_header(); }
	constexpr size_t size() noexcept { return std::max(Size, MinSize); }
	constexpr size_t align() noexcept { return Align; }
	constexpr size_t count(const Fact& F) noexcept { return Fact::value_type::size() * F.item_count() / size(); }
};
template <AreaFactory Fact, size_t MinSize>
struct area_reshape<Fact, 0, 0, MinSize>
{
	static constexpr bool dynamic = true;
	constexpr size_t header() noexcept { return Fact::value_type::size_header(); }
	constexpr size_t size() noexcept { return m_size; }
	constexpr size_t align() noexcept { return m_align; }
	constexpr size_t count(const Fact& F) noexcept { return Fact::value_type::size() * F.item_count() / m_size; }

	bool set(uint32_t l_size, uint32_t l_align) noexcept
	{
		l_size = std::max(l_size, static_cast<uint32_t>(MinSize));
		if (!area_valid_elem_size<typename Fact::value_type>(l_size, l_align))
			return false;
		m_size = l_size;
		m_align = l_align;
		return true;
	}

protected:
	uint32_t m_size = 0;
	uint32_t m_align = 0;
};

} // namespace inx::memory

#endif // INXLIB_MEMORY_AREA_FACTORY_HPP
