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
	
};

} // namespace inx::numeric

#endif // INXLIB_NUMERIC_FIXED_POINT_HPP
