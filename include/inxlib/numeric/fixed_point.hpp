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

#ifndef INXLIB_NUMERIC_FIXED_POINT_HPP
#define INXLIB_NUMERIC_FIXED_POINT_HPP

#include <inxlib/inx.hpp>
#include <inxlib/util/bits.hpp>

namespace inx::numeric
{

template <size_t Digits>
struct binary_fixed_width_deduce
{
	using type = std::int64_t;
	constexpr size_t digits = 64;
	constexpr bool overflow = true;
};
// fit into i64
template <size_t Digits>
requires (32 > Digits && Digits <= 64)
struct binary_fixed_width_deduce<Digits>
{
	using type = std::int64_t;
	constexpr size_t digits = Digits;
	constexpr bool overflow = false;
};
// fit into i32
template <size_t Digits>
requires (16 > Digits && Digits <= 32)
struct binary_fixed_width_deduce<Digits>
{
	using type = std::int32_t;
	constexpr size_t digits = Digits;
	constexpr bool overflow = false;
};
// fit into i16
template <size_t Digits>
requires (8 > Digits && Digits <= 16)
struct binary_fixed_width_deduce<Digits>
{
	using type = std::int16_t;
	constexpr size_t digits = Digits;
	constexpr bool overflow = false;
};
// fit into i8
template <size_t Digits>
requires (Digits <= 8)
struct binary_fixed_width_deduce<Digits>
{
	using type = std::int8_t;
	constexpr size_t digits = Digits;
	constexpr bool overflow = false;
};

template <size_t Digits, size_t Fraction>
struct binary_fixed_point
{
	using deduce = binary_fixed_width_deduce<Digits>;
	using value_type = typename deduce::type;
	static_assert(Digits <= deduce::digits, "Digits must not exceed max integer size.");

	consteval static size_t digits() noexcept { return deduce::digits; }
	consteval static size_t logical_digits() noexcept { return Digits; }
	consteval static size_t frac() noexcept { return Fraction; }
	consteval static bool overflow() noexcept { return deduce::overflow; }

	value_type value;
	
	binary_fixed_point() noexcept = default;
	explicit binary_fixed_point(value_type v) noexcept : value{v}
	{ }
	binary_fixed_point(binary_fixed_point v) noexcept = default;
	binary_fixed_point(binary_fixed_point&& v) noexcept = default;
	template <std::floating_point FP>
	explicit binary_fixed_point(FP val) noexcept :
		value(static_cast<FP>(val) * (1 << Fraction))
	{ }
	~binary_fixed_point() = default;

	template <std::floating_point FP>
	explicit operator FP() const noexcept
	{
		return static_cast<FP>(value) / (1 << Fraction);
	}
	template <std::integral Int>
	explicit operator Int() const noexcept
	{
		return static_cast<int>(value >> Fraction);
	}
	template <size_t OD, size_t OF>
	explicit(OD < Digits) operator binary_fixed_point<OD, OF>() const noexcept
	{
		using to_type = binary_fixed_point<OD, OF>;
		using bit_type = std::conditional_t<(Digits >= OD), value_type, typename to_type::value_type>;
		return to_type(static_cast<typename to_type::value_type>(
			inx::util::bit_shift(static_cast<bit_type>(value), OF - Fraction)));
	}
};

template <typename T>
concept BinaryFixedPoint = std::same_as<T, binary_fixed_point<T::digits(), T::frac()>>;

template <BinaryFixedPoint A, BinaryFixedPoint B>
using common_binary_fixed_point = binary_fixed_point<
	std::max(A::logical_digits(), B::logical_digits()) + ( std::max(A::frac(), B::frac()) - std::min(A::frac(), B::frac()) ),
	std::max(A::frac(), B::frac()) >;

/**
 * @return plus operator of type common_binary_fixed_point<A,B>
 */
template <BinaryFixedPoint A, BinaryFixedPoint B>
auto operator+(A a, B b) noexcept
{
	if (std::same_as<A, B>) {
		return A(a.value + b.value);
	} else {
		using type = common_binary_fixed_point<A,B>;
		using vt = typename type::value_type;
		return type(static_cast<vt>( static_cast<type>(a).value + static_cast<type>(b).value ));
	}
}
template <BinaryFixedPoint A, BinaryFixedPoint B>
A& operator+=(A& a, B b) noexcept
{
	a.value += static_cast<A>(b).value;
	return a;
}

/**
 * @return plus operator of type common_binary_fixed_point<A,B>
 */
template <BinaryFixedPoint A, BinaryFixedPoint B>
auto operator-(A a, B b) noexcept
{
	if (std::same_as<A, B>) {
		return A(a.value - b.value);
	} else {
		using type = common_binary_fixed_point<A,B>;
		using vt = typename type::value_type;
		return type(static_cast<vt>( static_cast<type>(a).value - static_cast<type>(b).value ));
	}
}
template <BinaryFixedPoint A, BinaryFixedPoint B>
A& operator-=(A& a, B b) noexcept
{
	a.value -= static_cast<A>(b).value;
	return a;
}


} // namespace inx::numeric

#endif // INXLIB_NUMERIC_FIXED_POINT_HPP
