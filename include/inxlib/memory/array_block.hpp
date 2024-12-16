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

#ifndef INXLIB_MEMORY_ARRAY_BLOCK_HPP
#define INXLIB_MEMORY_ARRAY_BLOCK_HPP

#include <forward_list>
#include <inxlib/inx.hpp>
#include <inxlib/memory/object.hpp>
#include <memory_resource>

namespace inx::memory {

namespace details {
template <size_t Size, size_t Align>
struct ArrayFactoryBlock
{
	static_assert(Align < alignof(std::max_align_t) && util::is_log2(Align), "Align must be to power of 2 and less/equal to max_align_t.");
	static_assert(Size != 0 && Size % Align == 0, "Size must be greater than 0 and divisible by Align.");
	ArrayFactoryBlock* next_block;
	int64_t reuse; /// <0 = no reuse, otherwise reuse starting at element[reuse]
	object_bytes_size<Size, Align> element[1];
};
template <size_t Align>
struct ArrayFactoryBlock<0, Align>
{
	static_assert(Align < alignof(std::max_align_t) && util::is_log2(Align), "Align must be to power of 2 and less/equal to max_align_t.");
	ArrayFactoryBlock* next_block;
	int64_t reuse; /// <0 = no reuse, otherwise reuse starting at element[reuse]
	alignas(Align) object_bytes<std::byte> element[1];
};
} // namespace details

struct factory_dynamic { };

/**
 * Block Factory providing for Type. Type=factory_dynamic for runtime types.
 * BlockPower is the amount of elements of Type to store in each block to the power of 2.
 * BlockPower of 0 can be specified at runtime.
 */
template <typename Type = factory_dynamic, size_t ElementPower = 0>
	requires (std::is_trivially_destructible_v<Type>)
class ArrayBlockFactory
{
	static_assert(BlockPower <= 30, "array block must not exceed 2^30.");
	static_assert(std::is_same_v<Type, factory_dynamic> || sizeof(Type) < std::numeric_limits<uint16>::max(), "Type size must fit in uint16_t.")
	struct BlockParam {
		uint32 block_bytes;
		uint16 type_size;
		uint8 type_align;
		uint8 element_power;
	};
	template <typename = void>
	struct DataType
	{
		using value_type = Type;
		static constexpr size_t Size = sizeof(Type);
		static constexpr size_t Align = alignof(Type);
		using block_type = details::ArrayFactoryBlock<Size, Align>;
		std::pmr::memory_resource* res;
		
		constexpr BlockParam params() const noexcept
		{
			return BlockParam{INXLIB_VAR_STRUCT_SIZE(block_type,element,1<<Align)}
		}
	};
public:
	using value_type = Type;

private:
};

} // namespace inx::data

#endif // INXLIB_MEMORY_ARRAY_BLOCK_HPP
