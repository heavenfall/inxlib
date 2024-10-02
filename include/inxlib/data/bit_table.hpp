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

#ifndef INXLIB_DATA_BIT_TABLE_HPP
#define INXLIB_DATA_BIT_TABLE_HPP

#include <cstring>
#include <inxlib/inx.hpp>
#include <inxlib/util/bits.hpp>
#ifndef NDEBUG
#include <vector>
#endif
#include <memory>
#include <memory_resource>

namespace inx::data {

namespace details {
template <size_t BitCount, std::unsigned_integral PackType>
    requires(!std::same_as<PackType, bool>)
class bit_ops
{
public:
	static_assert(std::is_integral_v<PackType> && std::is_unsigned_v<PackType>,
	              "PackType must be an unsigned integer type");
	static_assert(0 < BitCount && BitCount <= 8,
	              "BitCount must fall within [1,8]");
	static constexpr size_t bit_count = BitCount;
	using pack_type = PackType;

	static constexpr pack_type bit_mask =
	  util::make_mask<pack_type, bit_count>();
	static constexpr size_t bit_adj = std::bit_width(bit_count - 1);
	static constexpr size_t char_adj =
	  std::bit_width(static_cast<uint32_t>(CHAR_BIT - 1));
	static constexpr size_t pack_bits = sizeof(pack_type) * CHAR_BIT;
	static_assert(8 <= pack_bits && pack_bits <= 64,
	              "pack_bits must be between 8 and 64");
	static constexpr size_t pack_bits_size = std::bit_width(pack_bits - 1);
	static constexpr pack_type pack_bits_mask =
	  util::make_mask<pack_type, pack_bits_size>();
	static constexpr size_t pack_size = pack_bits_size - bit_adj;
	static constexpr pack_type pack_mask =
	  util::make_mask<pack_type, pack_size>();
	static constexpr size_t item_count = (1 << pack_size);

	enum class op
	{
		OR,
		AND,
		XOR,
		NAND
	};

	struct index_t
	{
		uint64_t id;
		index_t() noexcept = default;
		constexpr index_t(uint64_t l_id) noexcept
		  : id(l_id)
		{
		}
		constexpr index_t(uint32_t l_word, uint32_t l_bit) noexcept
		  : id((static_cast<uint64_t>(l_word) << pack_bits_size) |
		       static_cast<uint64_t>(l_bit))
		{
			assert((l_bit & pack_bits_mask) == l_bit);
		}

		constexpr uint32_t word() const noexcept
		{
			return static_cast<uint32_t>(id >> pack_bits_size);
		}
		constexpr uint32_t bit() const noexcept
		{
			return static_cast<uint32_t>(id & pack_bits_mask);
		}

		constexpr void adj_col(int64_t i) noexcept
		{
			id =
			  static_cast<uint64_t>(static_cast<int64_t>(id) + (i << bit_adj));
		}
		constexpr void adj_word(int64_t i) noexcept
		{
			id = static_cast<uint64_t>(static_cast<int64_t>(id) +
			                           (i << pack_bits_size));
		}
	};

	struct adj_index : index_t
	{
		int64_t row;
		adj_index() noexcept = default;
		constexpr adj_index(index_t l_idx, int64_t l_row) noexcept
		  : index_t(l_idx)
		  , row(l_row)
		{
		}

		using index_t::word;
		constexpr uint32_t word(int64_t rows) const noexcept
		{
			return static_cast<uint32_t>(
			  static_cast<uint64_t>(static_cast<int64_t>(this->id) +
			                        (rows * row)) >>
			  pack_bits_size);
		}

		constexpr void adj_row(int64_t i) noexcept
		{
			this->id =
			  static_cast<uint64_t>(static_cast<int64_t>(this->id) + (i * row));
		}
		constexpr uint32_t row_word() const noexcept
		{
			return static_cast<uint32_t>(row >> pack_bits_size);
		}
	};

protected:
	template <typename BT2>
	    requires std::derived_from<BT2,
	                               bit_ops<BitCount, typename BT2::pack_type>>
	static void copy(uint32 width,
	                 uint32 height,
	                 const pack_type* bt1,
	                 adj_index id1,
	                 BT2& bt2,
	                 typename BT2::adj_index id2) noexcept
	{
		assert(width > 0 && height > 0);
		using BT2ops = bit_ops<BitCount, typename BT2::pack_type>;
		typename BT2::pack_type* bt2data = bt2.data();
		while (true) {
			auto id1row = id1;
			auto id2row = id2;
			for (uint32 i = width;;) {
				BT2ops::bit_set(bt2data, id2row, bit_get(bt1, id1row));
				if (--i == 0)
					break;
				id1row.adj_col(1);
				id2row.adj_col(1);
			}
			if (--height == 0)
				break;
			id1.adj_row(1);
			id2.adj_row(1);
		}
	}

	static void flip(pack_type* data,
	                 adj_index id,
	                 int32 width,
	                 int32 height) noexcept
	{
		assert(width > 0 && height > 0);
		while (true) {
			auto idrow = id;
			for (uint32 i = width;;) {
				bit_set(data, idrow, ~bit_get(data, idrow));
				if (--i == 0)
					break;
				idrow.adj_col(1);
			}
			if (--height == 0)
				break;
			id.adj_row(1);
		}
	}

	template <typename BT2>
	    requires std::derived_from<BT2,
	                               bit_ops<BitCount, typename BT2::pack_type>>
	static void region_op(op OP,
	                      uint32 width,
	                      uint32 height,
	                      const pack_type* bt1,
	                      adj_index id1,
	                      BT2& bt2,
	                      typename BT2::adj_index id2) noexcept
	{
		assert(width > 0 && height > 0);
		using BT2ops = bit_ops<BitCount, typename BT2::pack_type>;
		typename BT2::pack_type* bt2data = bt2.data();
		void (*bit_op)(typename BT2ops::pack_type*,
		               typename BT2ops::index_t,
		               typename BT2ops::pack_type);
		switch (OP) {
		case op::AND:
			bit_op = &BT2ops::bit_and;
			break;
		case op::OR:
			bit_op = &BT2ops::bit_or;
			break;
		case op::XOR:
			bit_op = &BT2ops::bit_xor;
			break;
		case op::NAND:
			bit_op = &BT2ops::bit_nand;
			break;
		default:
			assert(false);
			return;
		}
		while (true) {
			auto id1row = id1;
			auto id2row = id2;
			for (uint32 i = width;;) {
				(*bit_op)(bt2data, id2row, bit_get(bt1, id1row));
				if (--i == 0)
					break;
				id1row.adj_col(1);
				id2row.adj_col(1);
			}
			if (--height == 0)
				break;
			id1.adj_row(1);
			id2.adj_row(1);
		}
	}

	static void region_op_fill(op OP,
	                           pack_type value,
	                           uint32 width,
	                           uint32 height,
	                           pack_type* bt1,
	                           adj_index id1) noexcept
	{
		assert(width > 0 && height > 0);
		void (*bit_op)(pack_type*, index_t, pack_type);
		switch (OP) {
		case op::AND:
			bit_op = &bit_and;
			break;
		case op::OR:
			bit_op = &bit_or;
			break;
		case op::XOR:
			bit_op = &bit_xor;
			break;
		case op::NAND:
			bit_op = &bit_nand;
			break;
		default:
			assert(false);
			return;
		}
		while (true) {
			auto id1row = id1;
			for (uint32 i = width;;) {
				(*bit_op)(bt1, id1row, value);
				if (--i == 0)
					break;
				id1row.adj_col(1);
			}
			if (--height == 0)
				break;
			id1.adj_row(1);
		}
	}

	static pack_type bit_get(const pack_type* data, index_t id) noexcept
	{
		return bit_right_shift<pack_type>(data[id.word()], id.bit()) & bit_mask;
	}
	template <size_t I = 0>
	static bool bit_test(const pack_type* data, index_t id) noexcept
	{
		return static_cast<bool>(
		  bit_right_shift<pack_type>(data[id.word()], id.bit() + I) & 1);
	}
	static void bit_set(pack_type* data, index_t id, pack_type value) noexcept
	{
		assert(value <= bit_mask);
		data[id.word()] =
		  (data[id.word()] & ~bit_left_shift<pack_type>(bit_mask, id.bit())) |
		  bit_left_shift<pack_type>(value & bit_mask, id.bit());
	}
	static void bit_clear(pack_type* data, index_t id) noexcept
	{
		data[id.word()] &= ~bit_left_shift<pack_type>(bit_mask, id.bit());
	}
	static void bit_or(pack_type* data, index_t id, pack_type value) noexcept
	{
		assert(value <= bit_mask);
		data[id.word()] |=
		  bit_left_shift<pack_type>(value & bit_mask, id.bit());
	}
	static void bit_and(pack_type* data, index_t id, pack_type value) noexcept
	{
		assert(value <= bit_mask);
		data[id.word()] &=
		  ~bit_left_shift<pack_type>((~value) & bit_mask, id.bit());
	}
	static void bit_xor(pack_type* data, index_t id, pack_type value) noexcept
	{
		assert(value <= bit_mask);
		data[id.word()] ^=
		  bit_left_shift<pack_type>(value & bit_mask, id.bit());
	}
	static void bit_nand(pack_type* data, index_t id, pack_type value) noexcept
	{
		assert(value <= bit_mask);
		data += id.word();
		*data &= ~bit_left_shift<pack_type>((~value) & bit_mask, id.bit());
		*data ^= bit_left_shift<pack_type>(bit_mask, id.bit());
	}
	static void bit_not(pack_type* data, index_t id) noexcept
	{
		data[id.word()] ^= bit_left_shift<pack_type>(bit_mask, id.bit());
	}

	static pack_type word_get(const pack_type* data, index_t id) noexcept
	{
		return data[id.word()];
	}
	static void word_set(pack_type* data, index_t id, pack_type value) noexcept
	{
		data[id.word()] = value;
	}

	template <int32 W, int32 H>
	static pack_type region(const pack_type* data,
	                        uint32_t row_words,
	                        index_t id) noexcept
	{
		static_assert(W > 0 && H > 0, "region must be positive");
		static_assert(static_cast<int64>(W) * static_cast<int64>(H) <=
		                static_cast<int64>(1 << pack_size),
		              "must be packable in a single pack_type");
		constexpr int32 WH = W * H;

#if 0
		if constexpr (bit_count == 1 || bit_count == 2 || bit_count == 4 || bit_count == 8) { // tight packing of bits
			auto id = bit_index(x - X, y - Y);
			uint32_t word = id.word(), bit = id.bit();
			if (bit + (W<<bit_adj) > pack_bits) { // split bits
				uint32 w1count = pack_bits - bit;
				pack_type w2mask = util::make_mask<pack_type>((W << bit_adj) - w1count);
				pack_type ans = bit_right_shift<pack_type>(mCells[word], bit) | bit_left_shift<pack_type>(mCells[word+1] & w2mask, w1count);
				for (uint32 i = W << bit_adj; i < static_cast<uint32>(H * (W<<bit_adj)); i += W << bit_adj) {
					word += mRowWords;
					ans |= bit_left_shift<pack_type>(bit_right_shift<pack_type>(mCells[word], bit) | bit_left_shift<pack_type>(mCells[word+1] & w2mask, w1count), i);
				}

				return ans;
			} else {
				pack_type w1mask = util::make_mask<pack_type>(W << bit_adj);
				pack_type ans = bit_right_shift<pack_type>(mCells[word], bit) & w1mask;
				for (uint32 i = W << bit_adj; i < static_cast<uint32>(H * (W<<bit_adj)); i += W << bit_adj) {
					word += mRowWords;
					ans |= bit_left_shift<pack_type>(bit_right_shift<pack_type>(mCells[word], bit) & w1mask, i);
				}

				return ans;
			}
		} else {
			pack_type ans = 0;
			for (int32 i = 0, j = -Y; j < H-Y; j++)
			for (int32 k = -X; k < W-X; k++, i+=bit_count)
				ans |= bit_get(x+k, y+j) << i;
			return ans;
		}
#else
		if constexpr (W == 1 && H == 1) {
			return bit_get(id);
		} else {
			auto word = id.word();
			auto bit = id.bit();
			if constexpr (W == 1) {
				const auto* cell = data + word;
				pack_type ans =
				  bit_right_shift<pack_type>(*cell, bit) & bit_mask;
				for (size_t i = bit_count;
				     i < bit_count * static_cast<size_t>(H);
				     i += bit_count) {
					cell += row_words;
					ans |=
					  bit_shift_to<i>(*cell, bit) & bit_left_shift<i>(bit_mask);
				}
				return ans;
			} else if constexpr (std::endian::native == std::endian::little &&
			                     static_cast<size_t>(W) * bit_count <=
			                       (sizeof(size_t) - 1) * CHAR_BIT) {
				// readable in a single non-aligned read
				row_words *= sizeof(size_t);
				const auto* cell =
				  std::bit_cast<const std::byte*>(data + word) +
				  (bit >> char_adj);
				bit &= inx::util::make_mask_v<decltype(bit), char_adj>;
				constexpr size_t bit_row = bit_count * static_cast<size_t>(W);
				size_t ans;
				{
					size_t tmp;
					std::memcpy(&tmp, cell, sizeof(size_t));
					ans = (tmp >> bit) & util::make_mask_v<pack_type, bit_row>;
				}
				for (size_t i = bit_row; i < bit_row * static_cast<size_t>(H);
				     i += bit_row) {
					cell += row_words;
					size_t tmp;
					std::memcpy(&tmp, cell, sizeof(size_t));
					ans |= bit_shift_from_to(tmp, bit, i) &
					       util::make_mask<pack_type>(bit_row, i);
				}
				return static_cast<pack_type>(ans);
			} else {
				// TODO: not done
				assert(false);
				// const auto* cell = data() + word;
				// pack_type ans = bit_right_shift<pack_type>(*cell, bit) &
				// bit_mask; for (size_t i = bit_count; i <
				// bit_count*static_cast<size_t>(H); i += bit_count) { 	cell +=
				// row; 	ans |= bit_shift_to<i>(*cell, bit) &
				// bit_left_shift<i>(bit_mask);
				// }
				return {};
			}
		}
#endif
	}
};
} // namespace details

template <size_t BitCount = 1,
          size_t BufferSize = 0,
          std::unsigned_integral PackType = size_t>
class bit_table : public details::bit_ops<BitCount, PackType>
{
public:
	using super = details::bit_ops<BitCount, PackType>;
	using typename super::adj_index;
	using typename super::index_t;
	using typename super::op;
	using typename super::pack_type;
	static constexpr size_t buffer_size = BufferSize;
	using size_type = size_t;

	struct CellsData
	{
		CellsData() noexcept
		  : cells(nullptr)
		{
		}
		pack_type* cells;
		std::unique_ptr<pack_type[]> cells_data;
	};

public:
	bit_table() noexcept
	  : mWidth(0)
	  , mHeight(0)
	  , mRowWords(0)
	{
	}
	bit_table(uint32 width, uint32 height) { setup(width, height); }
	bit_table(bit_table&&) = default;
	bit_table(const bit_table& other)
	{
		mWidth = other.mWidth;
		mHeight = other.mHeight;
		mRowWords = other.mRowWords;
		size_t words = (mHeight + 2 * buffer_size) * mRowWords;
		mCells.cells_data = std::make_unique<pack_type[]>(words);
		mCells.cells = mCells.cells_data.get();
		std::uninitialized_copy_n(other.mCells.cells, words, mCells.cells);
	}
	bit_table& operator=(bit_table&&) = default;
	bit_table& operator=(const bit_table&) = delete;

	void setup(const bit_table& copy_from)
	{
		mWidth = copy_from.mWidth;
		mHeight = copy_from.mHeight;
		mRowWords = copy_from.mRowWords;
		mCells.cells_data = std::make_unique<pack_type[]>(
		  (mHeight + 2 * buffer_size) * mRowWords);
		mCells.cells = mCells.cells_data.get();
		const auto id = bit_adj_index(-static_cast<int32>(buffer_size),
		                              -static_cast<int32>(buffer_size));
		copy_from.super::copy(mWidth + 2 * buffer_size,
		                      mHeight + 2 * buffer_size,
		                      copy_from.data(),
		                      id,
		                      *this,
		                      id);
	}
	void setup(uint32 width, uint32 height)
	{
		assert(width > 0);
		assert(height > 0);
		mWidth = width;
		mHeight = height;
		mRowWords =
		  static_cast<uint32>(-(-static_cast<int32>(static_cast<pack_type>(
		                          width + 2 * buffer_size)) >>
		                        super::pack_size) +
		                      1);
		mCells.cells_data = std::make_unique<pack_type[]>(
		  (mHeight + 2 * buffer_size) * mRowWords);
		mCells.cells = mCells.cells_data.get();
	}
	void setup(uint32 width, uint32 height, pack_type* data)
	{
		assert(width > 0);
		assert(height > 0);
		mWidth = width;
		mHeight = height;
		mRowWords =
		  static_cast<uint32>(-(-static_cast<int32>(static_cast<pack_type>(
		                          width + 2 * buffer_size)) >>
		                        super::pack_size) +
		                      1);
		mCells.cells_data = nullptr;
		mCells.cells = data;
	}
	size_t calc_cells_words() const noexcept
	{
		return (mHeight + 2 * buffer_size) * mRowWords;
	}

	pack_type bit_get(int32 x, int32 y) const noexcept
	{
		return bit_get(bit_index(x, y));
	}
	template <size_t I = 0>
	bool bit_test(int32 x, int32 y) const noexcept
	{
		return bit_test<I>(bit_index(x, y));
	}
	void bit_set(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_set(bit_index(x, y), value);
	}
	void bit_clear(int32 x, int32 y) noexcept
	{
		return bit_clear(bit_index(x, y));
	}
	void bit_and(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_and(bit_index(x, y), value);
	}
	void bit_or(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_or(bit_index(x, y), value);
	}
	void bit_xor(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_xor(bit_index(x, y), value);
	}
	void bit_nand(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_nand(bit_index(x, y), value);
	}

	void set_buffer(pack_type value) noexcept
	{
		if constexpr (BufferSize != 0) {
			for (int32 i = -static_cast<int32>(BufferSize),
			           j = static_cast<int32>(mHeight);
			     i < 0;
			     i++, j++)
				for (int32 x = -static_cast<int32>(BufferSize),
				           xe = x + static_cast<int32>(mWidth) +
				                static_cast<int32>(2 * BufferSize);
				     x < xe;
				     ++x) {
					bit_set(x, i, value);
					bit_set(x, j, value);
				}

			for (int32 i = -static_cast<int32>(BufferSize),
			           j = static_cast<int32>(mWidth);
			     i < 0;
			     i++, j++)
				for (int32 y = 0, ye = static_cast<int32>(mHeight); y < ye;
				     ++y) {
					bit_set(i, y, value);
					bit_set(j, y, value);
				}
		}
	}

	template <int32 W, int32 H>
	pack_type region(index_t id) const noexcept
	{
		return super::template region<W, H>(mCells.cells, mRowWords, id);
	}

	template <int32 X, int32 Y, int32 W, int32 H>
	pack_type region(int32 x, int32 y) const noexcept
	{
		static_assert(X >= 0 && W > 0 && X < W, "x must lie within region");
		static_assert(Y >= 0 && H > 0 && Y < H, "y must lie within region");
		static_assert(static_cast<int64>(W) * static_cast<int64>(H) <=
		                static_cast<int64>(1 << super::pack_size),
		              "must be packable in a single pack_type");
		assert(-static_cast<int32>(buffer_size) <= x - X &&
		       x - X + W <= static_cast<int32>(mWidth + buffer_size));
		assert(-static_cast<int32>(buffer_size) <= y - Y &&
		       y - Y + H <= static_cast<int32>(mHeight + buffer_size));
#ifdef NDEBUG
		return region<W, H>(bit_index(x - X, y - Y));
#else
		auto r = region<W, H>(bit_index(x - X, y - Y));
		auto t = r;
		for (int32 i = -Y; i < H - Y; ++i)
			for (int32 j = -X; j < W - X; ++j) {
				auto q = bit_get(x + j, y + i);
				assert((t & super::bit_mask) == q);
				t >>= super::bit_count;
			}
		return r;
#endif
	}

	uint32 getWidth() const noexcept { return mWidth; }
	uint32 getPadWidth() const noexcept { return mWidth + 2 * buffer_size; }
	uint32 getHeight() const noexcept { return mHeight; }
	uint32 getPadHeight() const noexcept { return mHeight + 2 * buffer_size; }
	uint32 getRowWords() const noexcept { return mRowWords; }
	uint32 getRowBits() const noexcept { return mRowWords * super::item_count; }

	bool empty() const noexcept { return mWidth == 0; }
	void clear()
	{
		mWidth = 0;
		mHeight = 0;
		mRowWords = 0;
		mCells.cells = nullptr;
		mCells.cells_data = nullptr;
	}

	std::pair<uint32_t, uint32_t> bit_pair_index(int32 x, int32 y)
	  const noexcept /// returns pair[word,bit]
	{
		assert(-static_cast<int32>(buffer_size) <= x &&
		       x < static_cast<int32>(mWidth + buffer_size));
		assert(-static_cast<int32>(buffer_size) <= y &&
		       y < static_cast<int32>(mHeight + buffer_size));
		x += buffer_size;
		return {static_cast<uint32>((y + buffer_size) * mRowWords +
		                            (x >> super::pack_size)),
		        static_cast<uint32>((x & super::pack_mask) << super::bit_adj)};
	}
	index_t bit_index(int32 x, int32 y) const noexcept
	{
		assert(-static_cast<int32>(buffer_size) <= x &&
		       x < static_cast<int32>(mWidth + buffer_size));
		assert(-static_cast<int32>(buffer_size) <= y &&
		       y < static_cast<int32>(mHeight + buffer_size));
		x += buffer_size;
		return index_t(
		  static_cast<uint32>((y + buffer_size) * mRowWords +
		                      (x >> super::pack_size)),
		  static_cast<uint32>((x & super::pack_mask) << super::bit_adj));
	}
	adj_index bit_adj_index(index_t id) const noexcept
	{
		return adj_index(id, mRowWords << super::pack_bits_size);
	}
	adj_index bit_adj_index(int32 x, int32 y) const noexcept
	{
		return bit_adj_index(bit_index(x, y));
	}
	pack_type bit_get(index_t id) const noexcept
	{
		return super::bit_get(mCells.cells, id);
	}
	template <size_t I = 0>
	bool bit_test(index_t id) const noexcept
	{
		return super::template bit_test<I>(mCells.cells, id);
	}
	void bit_set(index_t id, pack_type value) noexcept
	{
		super::bit_set(mCells.cells, id, value);
	}
	void bit_clear(index_t id) noexcept { super::bit_clear(mCells.cells, id); }
	void bit_or(index_t id, pack_type value) noexcept
	{
		super::bit_or(mCells.cells, id, value);
	}
	void bit_and(index_t id, pack_type value) noexcept
	{
		super::bit_and(mCells.cells, id, value);
	}
	void bit_xor(index_t id, pack_type value) noexcept
	{
		super::bit_xor(mCells.cells, id, value);
	}
	void bit_nand(index_t id, pack_type value) noexcept
	{
		super::bit_nand(mCells.cells, id, value);
	}
	void bit_not(index_t id) noexcept { super::bit_not(mCells.cells, id); }

	pack_type word_get(index_t id) const noexcept
	{
		return super::word_get(mCells.cells, id);
	}
	void word_set(index_t id, pack_type value) noexcept
	{
		super::word_set(mCells.cells, id, value);
	}

	const pack_type* data() const noexcept { return mCells.cells; }
	pack_type* data() noexcept { return mCells.cells; }

	/**
	 * @brief Copy part to some region
	 */
	template <typename T>
	void copy(int32 o_x,
	          int32 o_y,
	          int32 width,
	          int32 height,
	          T& dest,
	          int32 x,
	          int32 y) const
	{
		assert(static_cast<uint32>(o_x) < mWidth && width > 0 &&
		       static_cast<uint32>(o_x + width) <= mWidth);
		assert(static_cast<uint32>(o_y) < mHeight && height > 0 &&
		       static_cast<uint32>(o_y + height) <= mHeight);
		super::copy(width,
		            height,
		            data(),
		            bit_adj_index(o_x, o_y),
		            dest,
		            dest.bit_adj_index(x, y));
	}
	template <typename T>
	void copy(index_t id, int32 width, int32 height, T& dest, int32 x, int32 y)
	  const
	{
		assert(static_cast<uint32>(width - 1) < mWidth);
		assert(static_cast<uint32>(height - 1) < mHeight);
		super::copy(width,
		            height,
		            data(),
		            bit_adj_index(id),
		            dest,
		            dest.bit_adj_index(x, y));
	}

	void flip() { super::flip(data(), bit_adj_index(0, 0), mWidth, mHeight); }
	void flip(int32 x, int32 y, int32 width, int32 height)
	{
		super::flip(data(), bit_adj_index(x, y), width, height);
	}
	void flip(index_t id, int32 width, int32 height)
	{
		super::flip(data(), bit_adj_index(id), width, height);
	}

	template <typename T>
	void region_op(op OP, T& dest, int32 x, int32 y) const
	{
		super::region_op(OP,
		                 mWidth,
		                 mHeight,
		                 data(),
		                 bit_adj_index(0, 0),
		                 dest,
		                 dest.bit_adj_index(x, y));
	}
	template <typename T>
	void region_op(op OP,
	               int32 o_x,
	               int32 o_y,
	               int32 width,
	               int32 height,
	               T& dest,
	               int32 x,
	               int32 y) const
	{
		assert(static_cast<uint32>(o_x) < mWidth && width > 0 &&
		       static_cast<uint32>(o_x + width) <= mWidth);
		assert(static_cast<uint32>(o_y) < mHeight && height > 0 &&
		       static_cast<uint32>(o_y + height) <= mHeight);
		super::region_op(width,
		                 height,
		                 data(),
		                 bit_adj_index(o_x, o_y),
		                 dest,
		                 dest.bit_adj_index(x, y));
	}
	template <typename T>
	void region_op(op OP,
	               index_t id,
	               int32 width,
	               int32 height,
	               T& dest,
	               int32 x,
	               int32 y) const
	{
		assert(static_cast<uint32>(width - 1) < mWidth);
		assert(static_cast<uint32>(height - 1) < mHeight);
		super::region_op(width,
		                 height,
		                 data(),
		                 bit_adj_index(id),
		                 dest,
		                 dest.bit_adj_index(x, y));
	}

	void region_op_fill(op OP, pack_type value)
	{
		super::region_op_fill(
		  OP, value, mWidth, mHeight, data(), bit_adj_index(0, 0));
	}
	void region_op_fill(op OP,
	                    pack_type value,
	                    int32 o_x,
	                    int32 o_y,
	                    int32 width,
	                    int32 height)
	{
		assert(static_cast<uint32>(o_x) < mWidth && width > 0 &&
		       static_cast<uint32>(o_x + width) <= mWidth);
		assert(static_cast<uint32>(o_y) < mHeight && height > 0 &&
		       static_cast<uint32>(o_y + height) <= mHeight);
		super::region_op_fill(
		  OP, value, width, height, data(), bit_adj_index(o_x, o_y));
	}
	void region_op_fill(op OP,
	                    pack_type value,
	                    index_t id,
	                    int32 width,
	                    int32 height)
	{
		assert(static_cast<uint32>(width - 1) < mWidth);
		assert(static_cast<uint32>(height - 1) < mHeight);
		super::region_op_fill(
		  OP, value, width, height, data(), bit_adj_index(id));
	}

private:
	uint32 mWidth, mHeight, mRowWords;
	CellsData mCells;
};

template <size_t BitCount = 1, std::unsigned_integral PackType = size_t>
class bit_cell : public details::bit_ops<BitCount, PackType>
{
public:
	using super = details::bit_ops<BitCount, PackType>;
	using typename super::adj_index;
	using typename super::index_t;
	using typename super::op;
	using typename super::pack_type;
	using length_type = std::conditional_t<
	  sizeof(PackType) == 8,
	  uint32_t,
	  std::conditional_t<sizeof(PackType) == 4, uint16_t, uint8_t>>;
	using size_type =
	  std::conditional_t<sizeof(PackType) <= 2, uint32_t, size_t>;

protected:
	bit_cell() = default;
	bit_cell(const bit_cell&) = delete;
	bit_cell(bit_cell&&) = delete;
	bit_cell& operator=(const bit_cell&) = delete;
	bit_cell& operator=(bit_cell&&) = delete;

	struct Header
	{
		union
		{
			struct
			{
				length_type width, height;
			} d;
			std::conditional_t<
			  sizeof(length_type) == 1,
			  uint16_t,
			  std::conditional_t<sizeof(length_type) == 2, uint32_t, uint64_t>>
			  word;
		};
	};

public:
	static constexpr size_type bit_count(length_type width,
	                                     length_type height) noexcept
	{
		return (static_cast<size_type>(width) * static_cast<size_type>(height))
		       << super::bit_adj;
	}
	static constexpr size_type word_count(size_type bits) noexcept
	{
		return (bits + (super::pack_bits - 1)) >> super::pack_bits_size;
	}
	static constexpr size_type word_count(length_type width,
	                                      length_type height) noexcept
	{
		return word_count(bit_count(width, height));
	}
	static constexpr size_type total_bytes(length_type width,
	                                       length_type height) noexcept
	{
		return sizeof(bit_cell) + word_count(width, height) * sizeof(pack_type);
	}

	size_type size() const noexcept
	{
		return bit_count(m_header.d.width, m_header.d.height);
	}
	size_type size_word() const noexcept
	{
		return word_count(m_header.d.width, m_header.d.height);
	}
	size_type size_byte() const noexcept
	{
		return size_word() * sizeof(pack_type);
	}

	static bit_cell* construct(std::pmr::memory_resource& res,
	                           length_type width,
	                           length_type height,
	                           bool value = false)
	{
		size_type bits = bit_count(width, height);
		size_type data_words = word_count(bits);
		bit_cell* s = static_cast<bit_cell*>(
		  res.allocate(sizeof(bit_cell) + data_words * sizeof(pack_type),
		               alignof(bit_cell)));
		s->m_header.d.width = width;
		s->m_header.d.height = height;
		std::memset(
		  s->data(), !value ? 0 : 0xff, data_words * sizeof(pack_type));
		// ensure non-used bits at end are set to zero
		int offset =
		  static_cast<int64>(data_words << super::pack_bits_size) - bits;
		assert(offset >= 0 && offset < super::pack_bits);
		s->data()[data_words - 1] &= static_cast<pack_type>(~0ull) << offset;
		return s;
	}
	static bit_cell* construct(void*& res,
	                           size_t& res_size,
	                           length_type width,
	                           length_type height,
	                           bool value = false)
	{
		size_type bits = bit_count(width, height);
		size_type data_words = word_count(bits);
		bit_cell* s = static_cast<bit_cell*>(
		  std::align(alignof(bit_cell),
		             sizeof(bit_cell) + data_words * sizeof(pack_type),
		             res,
		             res_size));
		if (s == nullptr)
			throw std::runtime_error("invalid memory amount");
		s->m_header.d.width = width;
		s->m_header.d.height = height;
		std::memset(
		  s->data(), !value ? 0 : 0xff, data_words * sizeof(pack_type));
		// ensure non-used bits at end are set to zero
		int offset =
		  static_cast<int64>(data_words << super::pack_bits_size) - bits;
		assert(offset >= 0 && offset < super::pack_bits);
		s->data()[data_words - 1] &= static_cast<pack_type>(~0ull) >> offset;
		return s;
	}
	static bit_cell* construct(std::pmr::memory_resource& res,
	                           const bit_cell& copy_from)
	{
		auto total_size =
		  total_bytes(copy_from.m_header.d.width, copy_from.m_header.d.height);
		void* s = res.allocate(total_size, alignof(bit_cell));
		std::memcpy(s, static_cast<const void*>(&copy_from), total_size);
		return static_cast<bit_cell*>(s);
	}
	static bit_cell* construct(void*& res,
	                           size_t& res_size,
	                           const bit_cell& copy_from)
	{
		auto total_size =
		  total_bytes(copy_from.m_header.d.width, copy_from.m_header.d.height);
		bit_cell* s = static_cast<bit_cell*>(
		  std::align(alignof(bit_cell), total_size, res, res_size));
		if (s == nullptr)
			throw std::runtime_error("invalid memory amount");
		std::memcpy(s, static_cast<const void*>(&copy_from), total_size);
		return s;
	}
	static void destruct(std::pmr::memory_resource& res, bit_cell& cell)
	{
		res.deallocate(
		  &cell,
		  total_bytes(cell.m_header.d.width, cell.m_header.d.height),
		  alignof(bit_cell));
	}

	/**
	 * @brief Copy to a bit_cell of same dimension
	 * @param dest
	 */
	void copy(bit_cell& dest) const
	{
		if (m_header.word() != dest.m_header.word())
			throw std::runtime_error(
			  "invalid dest dimensions, must match for copy");
		std::memcpy(dest.data(), data(), size_byte());
	}
	/**
	 * @brief Copy to some bit table at (x,y)
	 */
	template <typename T>
	void copy(T& dest, int32 x, int32 y) const
	{
		super::copy(m_header.d.width,
		            m_header.d.height,
		            data(),
		            bit_adj_index(0, 0),
		            dest,
		            dest.bit_adj_index(x, y));
	}
	/**
	 * @brief Copy part to some region
	 */
	template <typename T>
	void copy(int32 o_x,
	          int32 o_y,
	          int32 width,
	          int32 height,
	          T& dest,
	          int32 x,
	          int32 y) const
	{
		assert(static_cast<uint32>(o_x) < m_header.d.width && width > 0 &&
		       static_cast<uint32>(o_x + width) <= m_header.d.width);
		assert(static_cast<uint32>(o_y) < m_header.d.height && height > 0 &&
		       static_cast<uint32>(o_y + height) <= m_header.d.height);
		super::copy(width,
		            height,
		            data(),
		            bit_adj_index(o_x, o_y),
		            dest,
		            dest.bit_adj_index(x, y));
	}
	template <typename T>
	void copy(index_t id, int32 width, int32 height, T& dest, int32 x, int32 y)
	  const
	{
		assert(static_cast<uint32>(width - 1) < m_header.d.width);
		assert(static_cast<uint32>(height - 1) < m_header.d.height);
		super::copy(width,
		            height,
		            data(),
		            bit_adj_index(id),
		            dest,
		            dest.bit_adj_index(x, y));
	}

	void flip()
	{
		super::flip(
		  data(), bit_adj_index(0, 0), m_header.d.width, m_header.d.height);
	}
	void flip(int32 x, int32 y, int32 width, int32 height)
	{
		super::flip(data(), bit_adj_index(x, y), width, height);
	}
	void flip(index_t id, int32 width, int32 height)
	{
		super::flip(data(), bit_adj_index(id), width, height);
	}

	template <typename T>
	void region_op(op OP, T& dest, int32 x, int32 y)
	{
		super::region_op(OP,
		                 m_header.d.width,
		                 m_header.d.height,
		                 data(),
		                 bit_adj_index(0, 0),
		                 dest,
		                 dest.bit_adj_index(x, y));
	}
	template <typename T>
	void region_op(op OP,
	               int32 o_x,
	               int32 o_y,
	               int32 width,
	               int32 height,
	               T& dest,
	               int32 x,
	               int32 y)
	{
		assert(static_cast<uint32>(o_x) < m_header.d.width && width > 0 &&
		       static_cast<uint32>(o_x + width) <= m_header.d.width);
		assert(static_cast<uint32>(o_y) < m_header.d.height && height > 0 &&
		       static_cast<uint32>(o_y + height) <= m_header.d.height);
		super::region_op(width,
		                 height,
		                 data(),
		                 bit_adj_index(o_x, o_y),
		                 dest,
		                 dest.bit_adj_index(x, y));
	}
	template <typename T>
	void region_op(op OP,
	               index_t id,
	               int32 width,
	               int32 height,
	               T& dest,
	               int32 x,
	               int32 y)
	{
		assert(static_cast<uint32>(width - 1) < m_header.d.width);
		assert(static_cast<uint32>(height - 1) < m_header.d.height);
		super::region_op(width,
		                 height,
		                 data(),
		                 bit_adj_index(id),
		                 dest,
		                 dest.bit_adj_index(x, y));
	}

	void region_op_fill(op OP, pack_type value)
	{
		super::region_op_fill(OP,
		                      value,
		                      m_header.d.width,
		                      m_header.d.height,
		                      data(),
		                      bit_adj_index(0, 0));
	}
	void region_op_fill(op OP,
	                    pack_type value,
	                    int32 o_x,
	                    int32 o_y,
	                    int32 width,
	                    int32 height)
	{
		assert(static_cast<uint32>(o_x) < m_header.d.width && width > 0 &&
		       static_cast<uint32>(o_x + width) <= m_header.d.width);
		assert(static_cast<uint32>(o_y) < m_header.d.height && height > 0 &&
		       static_cast<uint32>(o_y + height) <= m_header.d.height);
		super::region_op_fill(
		  OP, value, width, height, data(), bit_adj_index(o_x, o_y));
	}
	void region_op_fill(op OP,
	                    pack_type value,
	                    index_t id,
	                    int32 width,
	                    int32 height)
	{
		assert(static_cast<uint32>(width - 1) < m_header.d.width);
		assert(static_cast<uint32>(height - 1) < m_header.d.height);
		super::region_op_fill(
		  OP, value, width, height, data(), bit_adj_index(id));
	}

	pack_type bit_get(int32 x, int32 y) const noexcept
	{
		return bit_get(bit_index(x, y));
	}
	template <size_t I = 0>
	bool bit_test(int32 x, int32 y) const noexcept
	{
		return bit_test<I>(bit_index(x, y));
	}
	void bit_set(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_set(bit_index(x, y), value);
	}
	void bit_clear(int32 x, int32 y) noexcept
	{
		return bit_clear(bit_index(x, y));
	}
	void bit_and(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_and(bit_index(x, y), value);
	}
	void bit_or(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_or(bit_index(x, y), value);
	}
	void bit_xor(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_xor(bit_index(x, y), value);
	}
	void bit_nand(int32 x, int32 y, pack_type value) noexcept
	{
		return bit_nand(bit_index(x, y), value);
	}

	// region does not work in non-row aligned words

	length_type getWidth() const noexcept { return m_header.d.width; }
	length_type getHeight() const noexcept { return m_header.d.height; }
	const auto getHeaderWord() const noexcept { return m_header.word; }

	std::pair<uint32_t, uint32_t> bit_pair_index(int32 x, int32 y)
	  const noexcept /// returns pair[word,bit]
	{
		index_t id = bit_index(x, y);
		return {id.word(), id.bit()};
	}
	index_t bit_index(int32 x, int32 y) const noexcept
	{
		assert(static_cast<uint32>(x) < static_cast<uint32>(m_header.d.width));
		assert(static_cast<uint32>(y) < static_cast<uint32>(m_header.d.height));
		return index_t((y * m_header.d.width + x) << super::bit_adj);
	}
	adj_index bit_adj_index(index_t id) const noexcept
	{
		return adj_index(id, m_header.d.width << super::bit_adj);
	}
	adj_index bit_adj_index(int32 x, int32 y) const noexcept
	{
		return bit_adj_index(bit_index(x, y));
	}
	pack_type bit_get(index_t id) const noexcept
	{
		return super::bit_get(data(), id);
	}
	template <size_t I = 0>
	bool bit_test(index_t id) const noexcept
	{
		return super::template bit_test<I>(data(), id);
	}
	void bit_set(index_t id, pack_type value) noexcept
	{
		super::bit_set(data(), id, value);
	}
	void bit_clear(index_t id) noexcept { super::bit_clear(data(), id); }
	void bit_or(index_t id, pack_type value) noexcept
	{
		super::bit_or(data(), id, value);
	}
	void bit_and(index_t id, pack_type value) noexcept
	{
		super::bit_and(data(), id, value);
	}
	void bit_xor(index_t id, pack_type value) noexcept
	{
		super::bit_xor(data(), id, value);
	}
	void bit_nand(index_t id, pack_type value) noexcept
	{
		super::bit_nand(data(), id, value);
	}
	void bit_not(index_t id) noexcept { super::bit_not(data(), id); }

	pack_type word_get(index_t id) const noexcept
	{
		return super::word_get(data(), id);
	}
	void word_set(index_t id, pack_type value) noexcept
	{
		super::word_set(data(), id, value);
	}

	const pack_type* data() const noexcept
	{
		static_assert(sizeof(pack_type) == 2 * sizeof(length_type) ||
		                sizeof(pack_type) == sizeof(length_type),
		              "length_type must either be double pack_type or equal to "
		              "pack_type");
		if constexpr (sizeof(pack_type) == 2 * sizeof(length_type)) {
			return reinterpret_cast<const pack_type*>(this) + 1;
		} else {
			return reinterpret_cast<const pack_type*>(this) + 2;
		}
	}
	pack_type* data() noexcept
	{
		return const_cast<pack_type*>(std::as_const(*this).data());
	}

	bool operator==(const bit_cell& o) const noexcept
	{
		return m_header.word == o.m_header.word &&
		       std::memcmp(data(), o.data(), size_byte()) == 0;
	}
	bool operator!=(const bit_cell& o) const noexcept { return !(*this == o); }

private:
	Header m_header;
};

} // namespace inx::data

namespace std {

template <size_t BitCount, typename PackType>
struct hash<inx::data::bit_cell<BitCount, PackType>>
{
	size_t operator()(
	  const inx::data::bit_cell<BitCount, PackType>& val) const noexcept
	{
		std::hash<size_t> hasher;
		size_t seed = hasher(val.getHeaderWord());
		int words = val.size_word();
		auto ptr = val.data();
		while (words > 0) {
			size_t next_hash;
			if constexpr (sizeof(PackType) == sizeof(size_t)) {
				next_hash = *ptr++;
				words--;
			} else {
				next_hash = *ptr++;
				for (int i = 1;
				     --words > 0 && i < sizeof(size_t) / sizeof(PackType);
				     ++i)
					next_hash |= *ptr++ << (i * (sizeof(PackType) * CHAR_BIT));
			}
			seed ^= hasher(next_hash) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}
};

} // namespace std

#endif // INXLIB_DATA_BIT_TABLE_HPP
