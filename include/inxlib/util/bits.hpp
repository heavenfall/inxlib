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

#ifndef INXLIB_UTIL_BITS_HPP
#define INXLIB_UTIL_BITS_HPP

#include <bit>
#include <inxlib/inx.hpp>

namespace inx::util {

constexpr size_t byte_size = CHAR_BIT;
constexpr size_t byte_cnt = 3;

///
/// make_mask: bit mask
///   size_t Count: mask bit count
///   size_t Offset: mask offset from lsb
///
template <typename Type>
constexpr Type
make_mask(size_t Count, size_t Offset = 0) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	if (Count == sizeof(Type) * byte_size)
		return static_cast<Type>(~static_cast<std::make_unsigned_t<Type>>(0));
	return static_cast<Type>(
	  ~(~static_cast<std::make_unsigned_t<Type>>(0) << Count) << Offset);
}
template <typename Type, size_t Count, size_t Offset = 0>
constexpr Type
make_mask() noexcept
{
	static_assert(Count + Offset <= sizeof(Type) * byte_size,
	              "Mask exceeds Type bit count");
	return make_mask<Type>(Count, Offset);
}
template <typename Type, size_t Count, size_t Offset = 0>
struct make_mask_c
  : std::integral_constant<Type, make_mask<Type, Count, Offset>()>
{};
template <typename Type, size_t Count, size_t Offset = 0>
inline constexpr Type make_mask_v = make_mask<Type, Count, Offset>();

///
/// make_mask_limit: bit mask but never full
///   size_t Count: mask bit count
///   size_t Offset: mask offset from lsb
///
template <typename Type>
constexpr Type
make_mask_limit(size_t Count, size_t Offset = 0) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	assert(Count < sizeof(Type) * byte_size);
	return static_cast<Type>(
	  ~(~static_cast<std::make_unsigned_t<Type>>(0) << Count) << Offset);
}
template <typename Type, size_t Count, size_t Offset = 0>
constexpr Type
make_mask_limit() noexcept
{
	static_assert(Count + Offset <= sizeof(Type) * byte_size,
	              "Mask exceeds Type bit count");
	return make_mask_limit<Type>(Count, Offset);
}
template <typename Type, size_t Count, size_t Offset = 0>
struct make_mask_limit_c
  : std::integral_constant<Type, make_mask_limit<Type, Count, Offset>()>
{};
template <typename Type, size_t Count, size_t Offset = 0>
inline constexpr Type make_mask_limit_v =
  make_mask_limit<Type, Count, Offset>();

///
/// make_msb_mask: bit mask from msb
///   size_t Count: mask bit count
///   size_t Offset: mask offset from msb
///
template <typename Type>
constexpr Type
make_msb_mask(size_t Count, size_t Offset = 0) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	if (Count == sizeof(Type) * byte_size)
		return static_cast<Type>(~static_cast<std::make_unsigned_t<Type>>(0));
	return static_cast<Type>(
	  ~(~static_cast<std::make_unsigned_t<Type>>(0) >> Count) >> Offset);
}
template <typename Type, size_t Count, size_t Offset = 0>
constexpr Type
make_msb_mask() noexcept
{
	static_assert(Count + Offset <= sizeof(Type) * byte_size,
	              "Mask exceeds Type bit count");
	return make_msb_mask<Type>(Count, Offset);
}
template <typename Type, size_t Count, size_t Offset = 0>
struct make_msb_mask_c
  : std::integral_constant<Type, make_msb_mask<Type, Count, Offset>()>
{};
///
template <typename Type, size_t Count, size_t Offset = 0>
inline constexpr Type make_msb_mask_v = make_msb_mask<Type, Count, Offset>();

///
/// make_mask_limit: bit mask but never full
///   size_t Count: mask bit count
///   size_t Offset: mask offset from lsb
template <typename Type>
constexpr Type
make_msb_mask_limit(size_t Count, size_t Offset = 0) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	assert(Count < sizeof(Type) * byte_size);
	return static_cast<Type>(
	  ~(~static_cast<std::make_unsigned_t<Type>>(0) >> Count) >> Offset);
}
template <typename Type, size_t Count, size_t Offset = 0>
constexpr Type
make_msb_mask_limit() noexcept
{
	static_assert(Count + Offset <= sizeof(Type) * byte_size,
	              "Mask exceeds Type bit count");
	return make_msb_mask_limit<Type>(Count, Offset);
}
template <typename Type, size_t Count, size_t Offset = 0>
struct make_msb_mask_limit_c
  : std::integral_constant<Type, make_msb_mask_limit<Type, Count, Offset>()>
{};
template <typename Type, size_t Count, size_t Offset = 0>
inline constexpr Type make_msb_mask_limit_v =
  make_msb_mask_limit<Type, Count, Offset>();

///
/// bit_left_shift: left shift wrapper, equiv Value << Shift
///   Value:
///   Shift:
///
template <typename Type>
constexpr Type
bit_left_shift(Type Value, size_t Shift)
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	assert(Shift < sizeof(Type) * byte_size);
	if constexpr (std::is_unsigned_v<Type>)
		return static_cast<Type>(Value << Shift);
	else
		return static_cast<Type>(static_cast<std::make_unsigned_t<Type>>(Value)
		                         << Shift);
}
template <size_t Shift, typename Type>
constexpr Type
bit_left_shift(Type Value)
{
	static_assert(Shift < sizeof(Type) * byte_size,
	              "Shift exceeds Type bit count");
	if constexpr (Shift == 0)
		return Value;
	else
		return bit_left_shift(Value, Shift);
}
template <size_t Shift, auto Value>
constexpr decltype(Value)
bit_left_shift()
{
	return bit_left_shift<Shift>(Value);
}
template <size_t Shift, auto Value>
inline constexpr decltype(Value) bit_left_shift_v =
  bit_left_shift<Shift, Value>();
template <size_t Shift, auto Value>
struct bit_left_shift_c
  : std::integral_constant<decltype(Value), bit_left_shift_v<Shift, Value>>
{};

///
/// bit_left_shift: right shift wrapper, equiv Value >> Shift
///   Value:
///   Shift:
///
template <typename Type>
constexpr Type
bit_right_shift(Type Value, size_t Shift)
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	assert(Shift < sizeof(Type) * byte_size);
	return static_cast<Type>(Value >> Shift);
}
template <size_t Shift, typename Type>
constexpr Type
bit_right_shift(Type Value)
{
	static_assert(Shift < sizeof(Type) * byte_size,
	              "Shift exceeds Type bit count");
	if constexpr (Shift == 0)
		return Value;
	else
		return bit_right_shift(Value, Shift);
}
template <size_t Shift, auto Value>
constexpr decltype(Value)
bit_right_shift()
{
	return bit_right_shift<Shift>(Value);
}
template <size_t Shift, auto Value>
inline constexpr decltype(Value) bit_right_shift_v =
  bit_right_shift<Shift, Value>();
template <size_t Shift, auto Value>
struct bit_right_shift_c
  : std::integral_constant<decltype(Value), bit_right_shift_v<Shift, Value>>
{};

///
/// bit_left_nshift: right neutral shift, signed values always inserts 0, even
/// for negative numbers
///   Value:
///   Shift:
///
template <typename Type>
constexpr Type
bit_right_nshift(Type Value, size_t Shift)
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	assert(Shift < sizeof(Type) * byte_size);
	if constexpr (std::is_unsigned_v<Type>)
		return static_cast<Type>(Value >> Shift);
	else
		return static_cast<Type>(
		  static_cast<std::make_unsigned_t<Type>>(Value) >> Shift);
}
template <size_t Shift, typename Type>
constexpr Type
bit_right_nshift(Type Value)
{
	static_assert(Shift < sizeof(Type) * byte_size,
	              "Shift exceeds Type bit count");
	if constexpr (Shift == 0)
		return Value;
	else
		return bit_right_nshift(Value, Shift);
}
template <size_t Shift, auto Value>
constexpr decltype(Value)
bit_right_nshift()
{
	return bit_right_nshift<Shift>(Value);
}
template <size_t Shift, auto Value>
inline constexpr decltype(Value) bit_right_nshift_v =
  bit_right_nshift<Shift, Value>();
template <size_t Shift, auto Value>
struct bit_right_nshift_c
  : std::integral_constant<decltype(Value), bit_right_nshift_v<Shift, Value>>
{};

///
/// bit_shift: shifts left or right
///   Value:
///   Shift:
///
template <typename Type>
constexpr Type
bit_shift(Type Value, ssize_t Shift) noexcept
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	assert((Shift < 0 ? -Shift : Shift) < sizeof(Type) * byte_size);
	if (Shift < 0)
		return bit_right_shift(Value, static_cast<size_t>(-Shift));
	else
		return bit_left_shift(Value, static_cast<size_t>(Shift));
}
template <ssize_t Shift, typename Type>
constexpr Type
bit_shift(Type Value) noexcept
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	static_assert((Shift < 0 ? -Shift : Shift) < sizeof(Type) * byte_size,
	              "Shift exceeds Type bit count");
	if constexpr (Shift < 0)
		return bit_right_shift<static_cast<size_t>(-Shift)>(Value);
	else if constexpr (Shift > 0)
		return bit_left_shift<static_cast<size_t>(Shift)>(Value);
	else
		return Value;
}
template <ssize_t Shift, auto Value>
constexpr decltype(Value)
bit_shift() noexcept
{
	return bit_shift<Shift>(Value);
}
template <ssize_t Shift, auto Value>
inline constexpr decltype(Value) bit_shift_v = bit_shift<Shift, Value>();
template <ssize_t Shift, auto Value>
struct bit_shift_c
  : std::integral_constant<decltype(Value), bit_shift_v<Shift, Value>>
{};

///
/// bit_nshift: neutral shifts left or right
///   Value:
///   Shift:
///
template <typename Type>
constexpr Type
bit_nshift(Type Value, ssize_t Shift) noexcept
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	assert((Shift < 0 ? -Shift : Shift) < sizeof(Type) * byte_size);
	if (Shift < 0)
		return bit_right_nshift(Value, static_cast<size_t>(-Shift));
	else
		return bit_left_shift(Value, static_cast<size_t>(Shift));
}
template <ssize_t Shift, typename Type>
constexpr Type
bit_nshift(Type Value) noexcept
{
	static_assert(std::is_integral_v<Type>, "Type must be integral");
	static_assert((Shift < 0 ? -Shift : Shift) < sizeof(Type) * byte_size,
	              "Shift exceeds Type bit count");
	if constexpr (Shift < 0)
		return bit_right_nshift<static_cast<size_t>(-Shift)>(Value);
	else if constexpr (Shift > 0)
		return bit_left_shift<static_cast<size_t>(Shift)>(Value);
	else
		return Value;
}
template <ssize_t Shift, auto Value>
constexpr decltype(Value)
bit_nshift() noexcept
{
	return bit_shift<Shift>(Value);
}
template <ssize_t Shift, auto Value>
inline constexpr decltype(Value) bit_nshift_v = bit_nshift<Shift, Value>();
template <ssize_t Shift, auto Value>
struct bit_nshift_c
  : std::integral_constant<decltype(Value), bit_nshift_v<Shift, Value>>
{};

///
/// bit_shift_set: shift from point to point in a single shift
///   From: bit shift from
///   To: bit shift to
///
template <size_t From, size_t To, typename Type>
constexpr Type
bit_shift_set(Type Value) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	static_assert(From < sizeof(Type) * byte_size,
	              "From exceeds Type bit count");
	static_assert(To < sizeof(Type) * byte_size, "To exceeds Type bit count");
	return bit_shift<static_cast<ssize_t>(To) - static_cast<ssize_t>(From)>(
	  Value);
}
template <size_t From, size_t To, auto Value>
constexpr decltype(Value)
bit_shift_set() noexcept
{
	return bit_shift_set<From, To>(Value);
}
template <size_t From, size_t To, auto Value>
inline constexpr decltype(Value) bit_shift_set_v =
  bit_shift_set<From, To, Value>();
template <size_t From, size_t To, auto Value>
struct bit_shift_set_c
  : std::integral_constant<decltype(Value), bit_shift_set_v<From, To, Value>>
{};

///
/// bit_nshift_set: neutral shift from point to point in a single shift
///   From: bit shift from
///   To: bit shift to
///
template <size_t From, size_t To, typename Type>
constexpr Type
bit_nshift_set(Type Value) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	static_assert(From < sizeof(Type) * byte_size,
	              "From exceeds Type bit count");
	static_assert(To < sizeof(Type) * byte_size, "To exceeds Type bit count");
	return bit_nshift<static_cast<ssize_t>(To) - static_cast<ssize_t>(From)>(
	  Value);
}
template <size_t From, size_t To, auto Value>
constexpr decltype(Value)
bit_nshift_set() noexcept
{
	return bit_nshift_set<From, To>(Value);
}
template <size_t From, size_t To, auto Value>
inline constexpr decltype(Value) bit_nshift_set_v =
  bit_nshift_set<From, To, Value>();
template <size_t From, size_t To, auto Value>
struct bit_nshift_set_c
  : std::integral_constant<decltype(Value), bit_nshift_set_v<From, To, Value>>
{};

///
/// bit_shift_to: shift bit to from variable from, all bits before from are
/// cleared
///   From: bit shift from
///   To: bit shift to
///
template <size_t To, typename Type>
constexpr Type
bit_shift_to(Type Value, size_t From) noexcept
{
	assert(From < sizeof(Type) * byte_size);
	static_assert(To < sizeof(Type) * byte_size, "To exceeds Type bit count");
	return bit_left_shift<To>(bit_right_shift(Value, From));
}

///
/// bit_shift_to: neutral shift bit to from variable from, all bits before from
/// are cleared
///   From: bit shift from
///   To: bit shift to
///
template <size_t To, typename Type>
constexpr Type
bit_nshift_to(Type Value, size_t From) noexcept
{
	assert(From < sizeof(Type) * byte_size);
	static_assert(To < sizeof(Type) * byte_size, "To exceeds Type bit count");
	return bit_left_shift<To>(bit_right_nshift(Value, From));
}

///
/// bit_shift_from: shift bit to from variable from, all bits before from are
/// cleared
///   From: bit shift from
///   To: bit shift to
///
template <size_t From, typename Type>
constexpr Type
bit_shift_from(Type Value, size_t To) noexcept
{
	static_assert(From < sizeof(Type) * byte_size,
	              "From exceeds Type bit count");
	assert(To < sizeof(Type) * byte_size);
	return bit_left_shift(bit_right_shift<From>(Value), To);
}

///
/// bit_shift_from: shift bit to from variable from, all bits before from are
/// cleared
///   From: bit shift from
///   To: bit shift to
///
template <size_t From, typename Type>
constexpr Type
bit_nshift_from(Type Value, size_t To) noexcept
{
	static_assert(From < sizeof(Type) * byte_size,
	              "From exceeds Type bit count");
	assert(To < sizeof(Type) * byte_size);
	return bit_left_shift(bit_right_nshift<From>(Value), To);
}

///
/// bit_shift_from_to: shift bit variable to from variable from, all bits before
/// from are cleared
///   From: bit shift from
///   To: bit shift to
///
template <typename Type>
constexpr Type
bit_shift_from_to(Type Value, size_t From, size_t To) noexcept
{
	assert(From < sizeof(Type) * byte_size);
	assert(To < sizeof(Type) * byte_size);
	return bit_left_shift(bit_right_shift(Value, From), To);
}
template <typename Type>
constexpr Type
bit_nshift_from_to(Type Value, size_t From, size_t To) noexcept
{
	assert(From < sizeof(Type) * byte_size);
	assert(To < sizeof(Type) * byte_size);
	return bit_left_shift(bit_right_nshift(Value, From), To);
}

template <typename Type, size_t Segment, typename... Args>
constexpr std::enable_if_t<
  std::conjunction_v<std::bool_constant<std::is_convertible_v<Args, Type>>...>,
  Type> // Type
bit_pack_lsb(Args... args) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	static_assert(Segment <= sizeof(Type) * byte_size,
	              "Segment exceeds number of available bits of Type");
	static_assert(Segment * sizeof...(Args) <= sizeof(Type) * byte_size,
	              "Number of slotted segments exceeds availble bit count");
	struct helper
	{
		Type out;
		constexpr helper(Type a)
		  : out(a & make_mask<Type, Segment>())
		{
		}
		constexpr helper(Type a, Type b)
		  : out(a | (b << Segment))
		{
		}
		constexpr helper operator<<(helper x) { return helper(out, x.out); }
	};
	// return (helper(args) << ... << helper(0)).out;
	return (helper(args) << ...).out;
}
template <typename Type, size_t Segment, Type... Args>
constexpr std::enable_if_t<(sizeof...(Args) > 0), Type> // Type
bit_pack_lsb() noexcept
{
	return bit_pack_lsb<Type, Segment>(Args...);
}
template <typename Type, size_t Segment, Type... Args>
inline constexpr Type bit_pack_lsb_v = bit_pack_lsb<Type, Segment, Args...>();
template <typename Type, size_t Segment, Type... Args>
struct bit_pack_lsb_c
  : std::integral_constant<Type, bit_pack_lsb_v<Type, Segment, Args...>>
{};

template <size_t Segment, typename Type>
constexpr Type
bit_unpack_lsb(size_t i, Type pack) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	static_assert(Segment <= sizeof(Type) * byte_size,
	              "Segment exceeds number of available bits of Type");
	assert(i <= sizeof(Type) * byte_size &&
	       i * Segment <= sizeof(Type) * byte_size);
	if constexpr (std::is_signed_v<Type>)
		return bit_shift_set<sizeof(Type) * byte_size - Segment, 0>(
		  bit_left_shift(pack, sizeof(Type) * byte_size - (i + 1) * Segment));
	else
		return bit_right_shift(pack, i * Segment) & make_mask_v<Type, Segment>;
}
template <size_t Segment, size_t I, typename Type>
constexpr Type
bit_unpack_lsb(Type pack) noexcept
{
	static_assert(I <= sizeof(Type) * byte_size &&
	                I * Segment <= sizeof(Type) * byte_size,
	              "Number of slotted segments exceeds availble bit count");
	return bit_unpack_lsb<Segment>(I, pack);
}
template <size_t Segment, size_t I, auto Pack>
constexpr decltype(Pack)
bit_unpack_lsb() noexcept
{
	return bit_unpack_lsb<Segment, I>(Pack);
}
template <size_t Segment, size_t I, auto Pack>
inline constexpr decltype(Pack) bit_unpack_lsb_v =
  bit_unpack_lsb<Segment, I, Pack>();
template <size_t Segment, size_t I, auto Pack>
struct bit_unpack_lsb_c
  : std::integral_constant<decltype(Pack), bit_unpack_lsb_v<Segment, I, Pack>>
{};

template <typename Type, size_t Segment, typename... Args>
constexpr std::enable_if_t<
  std::conjunction_v<std::bool_constant<std::is_convertible_v<Args, Type>>...>,
  Type> // Type
bit_pack_msb(Args... args) noexcept
{
	static_assert(Segment <= sizeof(Type) * byte_size,
	              "Segment exceeds number of available bits of Type");
	static_assert(Segment * sizeof...(Args) <= sizeof(Type) * byte_size,
	              "Number of slotted segments exceeds availble bit count");
	struct helper
	{
		Type out;
		constexpr helper(Type a)
		  : out(a & make_mask<Type, Segment>())
		{
		}
		constexpr helper(Type a, Type b)
		  : out((a << Segment) | b)
		{
		}
		constexpr helper operator<<(helper x) { return helper(out, x.out); }
	};
	return (helper(0) << ... << helper(args)).out;
}
template <typename Type, size_t Segment, Type... Args>
constexpr std::enable_if_t<(sizeof...(Args) > 0), Type> // Type
bit_pack_msb() noexcept
{
	return bit_pack_msb<Type, Segment>(Args...);
}
template <typename Type, size_t Segment, Type... Args>
struct bit_pack_msb_c
  : std::integral_constant<Type, bit_pack_msb<Type, Segment, Args...>()>
{};
template <typename Type, size_t Segment, Type... Args>
inline constexpr Type bit_pack_msb_v = bit_pack_msb<Type, Segment, Args...>();

template <size_t From, size_t To, size_t Count, typename Type>
constexpr Type
bit_shift_mask(Type Value) noexcept
{
	static_assert(std::is_integral<Type>(), "Type must be integral");
	static_assert(From < sizeof(Type) * byte_size,
	              "From exceeds Type bit count");
	static_assert(To < sizeof(Type) * byte_size, "To exceeds Type bit count");
	static_assert(From + Count <= sizeof(Type) * byte_size &&
	                To + Count <= sizeof(Type) * byte_size,
	              "Count must make a valid mask");
	return bit_shift_set<From, To>(Value) & make_mask<Type, Count, To>();
}
template <size_t From, size_t To, size_t Count, auto Value>
constexpr decltype(Value)
bit_shift_mask() noexcept
{
	return bit_shift_mask(Value);
}
template <size_t From, size_t To, size_t Count, auto Value>
struct bit_shift_mask_c
  : std::integral_constant<decltype(Value),
                           bit_shift_mask<From, To, Count, Value>()>
{};
template <size_t From, size_t To, size_t Count, auto Value>
inline constexpr decltype(Value) bit_shift_mask_v =
  bit_shift_mask_c<From, To, Count, Value>::value;

template <size_t From, size_t To, size_t Count, typename Type>
constexpr Type
bit_nshift_mask(Type Value) noexcept
{
	return static_cast<Type>(bit_shift_mask<From, To, Count>(
	  static_cast<std::make_unsigned_t<Type>>(Value)));
}
template <size_t From, size_t To, size_t Count, auto Value>
constexpr decltype(Value)
bit_nshift_mask() noexcept
{
	return bit_nshift_mask(Value);
}
template <size_t From, size_t To, size_t Count, auto Value>
struct bit_nshift_mask_c
  : std::integral_constant<decltype(Value),
                           bit_nshift_mask<From, To, Count, Value>()>
{};
template <size_t From, size_t To, size_t Count, auto Value>
inline constexpr decltype(Value) bit_nshift_mask_v =
  bit_nshift_mask_c<From, To, Count, Value>::value;

#if defined(__GNUC__) || defined(__clang__)

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr int
clz(T val) noexcept
{
	assert(val > 0);
	if constexpr (sizeof(T) <= 4) {
		return __builtin_clz(static_cast<unsigned int>(val));
	} else {
		return __builtin_clzll(static_cast<unsigned long long>(val));
	}
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr int
ctz(T val) noexcept
{
	assert(val > 0);
	if constexpr (sizeof(T) <= 4) {
		return __builtin_ctz(static_cast<unsigned int>(val));
	} else {
		return __builtin_ctzll(static_cast<unsigned long long>(val));
	}
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr int
popcount(T val) noexcept
{
	if constexpr (sizeof(T) <= 4) {
		return __builtin_popcount(static_cast<unsigned int>(val));
	} else {
		return __builtin_popcountll(static_cast<unsigned long long>(val));
	}
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr int
clz_index(T val) noexcept
{
	assert(val > 0);
	if constexpr (sizeof(T) <= 4) {
		return (sizeof(uint32) * byte_size - 1) -
		       __builtin_clz(static_cast<unsigned int>(val));
	} else {
		return (sizeof(uint64) * byte_size - 1) -
		       __builtin_clzll(static_cast<unsigned long long>(val));
	}
}

#endif

} // namespace inx::util

#endif // INXLIB_UTIL_BITS_HPP
