/*
MIT License

Copyright (c) 2024 Ryan Hechenberger

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

#ifndef INXLIB_DATA_BLOCK_ARRAY_HPP
#define INXLIB_DATA_BLOCK_ARRAY_HPP

#include <bit>
#include <inxlib/inx.hpp>
#include <inxlib/util/iterator.hpp>
#include <memory_resource>
#include <ranges>

namespace inx::data {

namespace details {

template <typename ValueType, size_t BlockPower>
struct BlockArrayIterator
  : inx::util::RandomIteratorWrapper<BlockArrayIterator<ValueType, BlockPower>,
                                     ValueType>
{
	using super = inx::util::RandomIteratorWrapper<
	  BlockArrayIterator<ValueType, BlockPower>,
	  ValueType>;
	ValueType* const* access_;
	BlockArrayIterator() noexcept
	  : access_{}
	{
	}
	BlockArrayIterator(ValueType* const* access) noexcept
	  : access_(access)
	{
	}
	BlockArrayIterator(typename super::difference_type pos,
	                   ValueType* const* access) noexcept
	  : super(pos)
	  , access_(access)
	{
	}

	typename super::value_type& operator[](
	  typename super::difference_type i) const noexcept
	{
		return access_[i >> BlockPower][i & ((1u << BlockPower) - 1)];
	}
};

} // namespace details

// Tollerance is how many levels it will stay within, i.e. 0 will always
// reallocate is needed levels is less, 1 will keep with +-1 before reallocate
template <typename ValueType,
          size_t BlockPower,
          typename Allocator = std::pmr::polymorphic_allocator<ValueType>>
class BlockArray
{
public:
	using self = BlockArray<ValueType, BlockPower>;
	using allocator_type = Allocator;
	using value_type = ValueType;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;

	static consteval size_t block_count() noexcept { return (1 << BlockPower); }
	static consteval size_t block_size() noexcept
	{
		return sizeof(ValueType) * block_count();
	}

private:
	struct Data
	{
		static constexpr bool trivial = std::is_trivial_v<ValueType>;
		static constexpr uint32 min_access_size = 16;
		Allocator allocator;
		ValueType** access;
		ValueType** access_at;
		uint32 access_size;
		uint32 data_size;
		Data() noexcept(noexcept(Allocator()))
		  : access(nullptr)
		  , access_at(nullptr)
		  , access_size(0)
		  , data_size(0)
		{
		}
		Data(const Allocator& alloc) noexcept(noexcept(Allocator(alloc)))
		  : allocator(alloc)
		  , access(nullptr)
		  , access_at(nullptr)
		  , access_size(0)
		  , data_size(0)
		{
		}
		~Data() { reset(); }

		static constexpr std::pair<uint32, uint32> split_index(
		  uint32 size) noexcept
		{
			return {static_cast<uint32>(size >> BlockPower),
			        static_cast<uint32>(size & ((1u << BlockPower) - 1))};
		}

		[[nodiscard]] ValueType* allocate_block()
		{
			return allocator.allocate(block_count());
		}
		void deallocate_block(ValueType* block)
		{
			assert(block != nullptr);
			allocator.deallocate(block, block_count());
		}
		[[nodiscard]] ValueType** allocate_access(uint32 size)
		{
			return allocator.template allocate_object<ValueType*>(size);
		}
		void deallocate_access(ValueType** block, uint32 size)
		{
			assert(block != nullptr);
			allocator.template deallocate_object<ValueType*>(block, size);
		}

		template <typename... Args>
		void construct(ValueType* p, Args&&... args)
		    requires(!trivial)
		{
			allocator.construct(p, std::forward<Args>(args)...);
		}
		void destroy(ValueType* p)
		    requires(!trivial)
		{
			std::allocator_traits<Allocator>::destroy(allocator, p);
		}

		void clear()
		{
			if constexpr (!trivial) {
				if (data_size > 0) {
					auto id = split_index(data_size - 1);
					for (uint32 i = 0; i < id.first; ++i) {
						ValueType* val = access[i];
						for (uint32 j = 0; j < block_count(); ++j) {
							destroy(val + j);
						}
					}
					ValueType* val = access[id.first];
					for (uint32 j = 0; j <= id.second; ++j) {
						destroy(val + j);
					}
				}
			}
			access_at = nullptr;
			data_size = 0;
		}
		void reset()
		{
			clear();
			for (auto it = access, ite = access + access_size; it != ite;
			     ++it) {
				if (*it == nullptr)
					break;
				deallocate_block(*it);
			}
			if (access != nullptr) {
				deallocate_access(access, access_size);
				access = nullptr;
				access_size = 0;
			}
		}

		void update_access(uint32 i)
		{
			if (!access) [[unlikely]] {
				assert(i == 0);
				access_size = min_access_size;
				access = allocate_access(access_size);
				std::uninitialized_fill_n(access, access_size, nullptr);
				access[0] = allocate_block();
				access_at = access;
			} else if (i >= access_size) [[unlikely]] {
				assert(i == access_size);
				access_size <<= 1;
				ValueType** new_access = allocate_access(access_size);
				std::uninitialized_copy_n(access, i, new_access);
				std::uninitialized_fill_n(new_access + i, i, nullptr);
				deallocate_access(access, i);
				access = new_access;
				access[i] = allocate_block();
				access_at = access + i;
			} else {
				access_at = access + i;
				if (*access_at == nullptr)
					*access_at = allocate_block();
			}
		}
		ValueType* push_next_address()
		{
			auto id = split_index(data_size++);
			if (!access_at) [[unlikely]]
				update_access(id.first);
			assert(access_at != nullptr &&
			       data_size <= (access_size << BlockPower));
			ValueType* ret = (*access_at) + id.second;
			if (id.second == block_count() - 1) [[unlikely]]
				access_at = nullptr;
			return ret;
		}

		void reserve(size_t res)
		{
			uint32 blocks = split_index(res).first + 1;
			if (res == 0)
				return;
			// allocate block
			if (!access) [[likely]] {
				access_size = std::max(std::bit_ceil(blocks), min_access_size);
				access = allocate_access(access_size);
				std::uninitialized_fill_n(
				  access + blocks, access_size - blocks, nullptr);
				// access[0] =
				// allocate.allocate_object<ValueType>(block_count()); access_at
				// = access;
			} else {
				if (blocks >= access_size) {
					uint32 new_size = std::bit_ceil(blocks);
					ValueType** new_access = allocate_access(new_size);
					std::uninitialized_copy_n(access, access_size, new_access);
					std::uninitialized_fill_n(
					  access + access_size, new_access - access_size, nullptr);
					deallocate_access(access, access_size);
					access = new_access;
					access_size = new_size;
				}
			}
			for (auto it = access + (blocks - 1);; --it) {
				if (*it == nullptr) {
					*it = allocate_block();
					if (it == access)
						break;
				} else {
					break;
				}
			}
			access_at = access + split_index(data_size).first;
		}
	} m_data;

public:
	// using view_type = const
	// std::ranges::transform_view<std::ranges::iota_view<uint32,uint32>,
	// IteratorMap_<value_type>>; using const_view_type = const
	// std::ranges::transform_view<std::ranges::iota_view<uint32,uint32>,
	// IteratorMap_<const value_type>>;
	using iterator = details::BlockArrayIterator<value_type, BlockPower>;
	using const_iterator =
	  details::BlockArrayIterator<const value_type, BlockPower>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
	BlockArray() noexcept(noexcept(Data())) {}
	BlockArray(const Allocator& alloc) noexcept(noexcept(Data(alloc)))
	  : m_data(alloc)
	{
	}

	void reserve(size_t res) { m_data.reserve(res); }
	size_type capacity() const noexcept
	{
		return static_cast<size_type>(m_data.access_size) << BlockPower;
	}
	void clear() { m_data.clear(); }
	void reset() { m_data.reset(); }

	size_type size() const noexcept { return m_data.data_size; }
	bool empty() const noexcept { return m_data.data_size == 0; }

	void push_back(const value_type& val)
	{
		ValueType* p = m_data.push_next_address();
		if constexpr (Data::trivial) {
			std::construct_at(p, val);
		} else {
			m_data.construct(val);
		}
	}
	void push_back(value_type&& val)
	{
		ValueType* p = m_data.push_next_address();
		if constexpr (Data::trivial) {
			std::construct_at(p, std::move(val));
		} else {
			m_data.construct(p, std::move(val));
		}
	}

	template <typename... Args>
	reference emplace_back(Args&&... args)
	{
		ValueType* p = m_data.push_next_address();
		if constexpr (Data::trivial) {
			return *std::construct_at(p, std::forward<Args>(args)...);
		} else {
			m_data.construct(p, std::forward<Args>(args)...);
			return *std::launder(p);
		}
	}

	// access
	value_type& operator[](size_type pos) noexcept
	{
		auto id = Data::split_index(pos);
		assert(id.first < m_data.access_size &&
		       m_data.access[id.first] != nullptr);
		return m_data.access[id.first][id.second];
	}
	const value_type& operator[](size_type pos) const noexcept
	{
		auto id = Data::split_index(pos);
		assert(id.first < m_data.access_size &&
		       m_data.access[id.first] != nullptr);
		return m_data.access[id.first][id.second];
	}
	value_type& at(size_type pos)
	{
		if (pos >= m_data.data_size)
			throw std::out_of_range("pos");
		return (*this)[pos];
	}
	const value_type& at(size_type pos) const
	{
		if (pos >= m_data.data_size)
			throw std::out_of_range("pos");
		return (*this)[pos];
	}
	value_type& front(size_type pos) noexcept
	{
		assert(m_data.data_size != 0);
		return m_data.access[0][0];
	}
	const value_type& front() const noexcept
	{
		assert(m_data.data_size != 0);
		return m_data.access[0][0];
	}
	value_type& back() noexcept
	{
		assert(m_data.data_size != 0);
		return (*this)[m_data.data_size - 1];
	}
	const value_type& back() const noexcept
	{
		assert(m_data.data_size != 0);
		return (*this)[m_data.data_size - 1];
	}

	// iterators
	// view_type view() noexcept { return
	// view_type(std::ranges::iota_view<uint32,uint32>(static_cast<uint32>(0),
	// m_data.data_size), IteratorMap_<value_type>{
	// const_cast<value_type*const*>(m_data.access) }); } const_view_type view()
	// const noexcept { return
	// const_view_type(std::ranges::iota_view<uint32,uint32>(static_cast<uint32>(0),
	// m_data.data_size), IteratorMap_<const value_type>{ const_cast<const
	// value_type*const*>(m_data.access) }); }
	iterator begin() noexcept
	{
		return iterator(0, const_cast<value_type* const*>(m_data.access));
	}
	const_iterator begin() const noexcept
	{
		return const_iterator(
		  0, const_cast<const value_type* const*>(m_data.access));
	}
	iterator end() noexcept
	{
		return iterator(m_data.data_size,
		                const_cast<value_type* const*>(m_data.access));
	}
	const_iterator end() const noexcept
	{
		return const_iterator(
		  m_data.data_size,
		  const_cast<const value_type* const*>(m_data.access));
	}
	// reverse
	reverse_iterator rbegin() noexcept { return end(); }
	const_reverse_iterator rbegin() const noexcept { return end(); }
	reverse_iterator rend() noexcept { return begin(); }
	const_reverse_iterator rend() const noexcept { return begin(); }
};

} // namespace inx::data

#endif // INXLIB_DATA_BLOCK_ARRAY_HPP
