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

#ifndef INXLIB_MEMORY_MALLOC_FACTORY_HPP
#define INXLIB_MEMORY_MALLOC_FACTORY_HPP

#include <inxlib/inx.hpp>
#include "factory.hpp"
#include <cstdlib>
#include <cstddef>

namespace inx::memory {

class malloc_factory
{
public:
	using value_type = std::byte;
	using pointer = value_type*;
	using size_type = size_t;

	static consteval size_type alignment() noexcept { return alignof(max_align_t); }
	static consteval size_type element_size() noexcept { return 1; }

	pointer allocate(size_type elems)
	{
		return static_cast<value_type*>(std::malloc(elems));
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

static_assert(ArrayFactory<malloc_factory>, "malloc_factory must be a valid Factory");

template <typename BaseFactory>
	requires ByteFactory<BaseFactory>
class reclaim_factory : private BaseFactory
{
public:
	using base_factory = BaseFactory;
	using typename base_factory::value_type;
	using typename base_factory::size_type;
	using typename base_factory::pointer;
	using base_factory::alignment;
	using base_factory::element_size;

private:
	struct alignas(max_align_t) pointer_meta_size
	{
		pointer prev;
		pointer next;
		uintptr_t size;
	};
	struct alignas(max_align_t) pointer_meta
	{
		pointer prev;
		pointer next;
		// pointer_meta& operator=(const pointer_meta_size& m)
		// {
		// 	prev = m.prev;
		// 	next = m.next;
		// }
	};
	static constexpr bool store_size = !ElemFreeFactory<base_factory>;
	using store_meta = std::conditional_t<store_size, pointer_meta_size, pointer_meta>;

public:
	~reclaim_factory()
	{
		release();
	}
	pointer allocate(size_type elems)
	{
		auto* ptr = reinterpret_cast<pointer>( reinterpret_cast<pointer*>(base_factory::allocate(elems + 2*sizeof(pointer*))) + 2 );
		pointer next = std::exchange(m_linkStart, ptr);
		link_next(ptr) = next;
		if (next != nullptr) [[likely]] {
			link_prev(next) = ptr;
		}
	}
	void deallocate(pointer ptr)
	{
		auto meta = link_meta(ptr);
		if (meta.prev != nullptr) [[likely]] {
			link_next(meta.prev) = meta.next;
		} else {
			m_linkStart = meta.next;
		}
		if (meta.next != nullptr) [[likely]] {
			link_prev(meta.next) = meta.prev;
		}
		if constexpr (store_size) {
			free_(ptr, meta.size);
		} else {
			free_(ptr, 0);
		}
	}
	void deallocate(pointer ptr, size_type elems)
	{
		deallocate(ptr);
	}
	void release()
	{
		for (pointer p = m_linkStart; p != nullptr; ) {
			auto meta = link_meta(p);
			auto next_p = meta.next;
			if constexpr (store_size) {
				free_(p, meta.size);
			} else {
				free_(p, 0);
			}
			p = next_p;
		}
		m_linkStart = nullptr;
	}
	void reclaim()
	{
		release();
	}

	base_factory& base() noexcept { return static_cast<base_factory&>(*this); }
	const base_factory& base() const noexcept { return static_cast<const base_factory&>(*this); }

protected:
	static store_meta& link_meta(pointer mem) noexcept
	{
		return *reinterpret_cast<store_meta*>(mem - sizeof(store_meta));
	}
	pointer allocate_(size_type elems)
	{
		elems += sizeof(store_meta);
		pointer ptr = base_factory::allocate(elems);
		reinterpret_cast<store_meta*>(ptr).size = elems;
		return ptr + sizeof(store_meta);
	}
	void free_(pointer ptr, size_type elems[[maybe_unused]])
	{
		if constexpr (store_size) {
			base_factory::deallocate(ptr - sizeof(store_meta), elems);
		} else {
			base_factory::deallocate(ptr - sizeof(store_meta));
		}
	}

protected:
	pointer m_linkStart = nullptr;
};

using reclaim_malloc_factory = reclaim_factory<malloc_factory>;
static_assert(ReclaimFactory<reclaim_malloc_factory>, "reclaim_factory<malloc_factory> must provide ReclaimFactory concept.");

} // namespace inx::memory

#endif // INXLIB_MEMORY_MALLOC_FACTORY_HPP
