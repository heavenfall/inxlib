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

#ifndef INXLIB_UTIL_NUMERIC_TYPES_HPP
#define INXLIB_UTIL_NUMERIC_TYPES_HPP

#include <inxlib/inx.hpp>

#include "bits.hpp"

namespace inx::util {

template <typename T>
struct raise_integral_level;
template <>
struct raise_integral_level<int8> {
	using type = int16;
};
template <>
struct raise_integral_level<int16> {
	using type = int32;
};
template <>
struct raise_integral_level<int32> {
	using type = int64;
};
template <>
struct raise_integral_level<int64> {
	using type = int64;
};
template <>
struct raise_integral_level<uint8> {
	using type = uint16;
};
template <>
struct raise_integral_level<uint16> {
	using type = uint32;
};
template <>
struct raise_integral_level<uint32> {
	using type = uint64;
};
template <>
struct raise_integral_level<uint64> {
	using type = uint64;
};
template <typename T>
using raise_integral_level_t = typename raise_integral_level<T>::type;

template <typename T>
struct raise_numeric_level : raise_integral_level<T> {};
template <>
struct raise_numeric_level<float> {
	using type = float;
};
template <>
struct raise_numeric_level<double> {
	using type = double;
};
template <>
struct raise_numeric_level<long double> {
	using type = long double;
};
template <typename T>
using raise_numeric_level_t = typename raise_numeric_level<T>::type;

template <typename T>
std::enable_if_t<std::is_integral_v<T>, bool> is_zero(T x) noexcept {
	return x == 0;
}
template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, bool> is_zero(T x) noexcept {
	return std::abs(x) < epsilon<T>;
}
template <typename T, typename... Ts>
std::enable_if_t<std::conjunction_v<std::is_same<T, Ts>...>, bool> is_all_zero(
    T x, Ts... xs) noexcept {
	if constexpr (std::is_integral_v<T>) {
		return ((x | ... | xs) == 0);
	} else {
		return (is_zero(x) && ... && is_zero(xs));
	}
}
template <typename T, typename... Ts>
std::enable_if_t<std::conjunction_v<std::is_same<T, Ts>...>, bool> is_any_zero(
    T x, Ts... xs) noexcept {
	if constexpr (std::is_integral_v<T>) {
		return ((x & ... & xs) == 0);
	} else {
		return (is_zero(x) || ... || is_zero(xs));
	}
}

}  // namespace inx::util

#endif  // INXLIB_UTIL_NUMERIC_TYPES_HPP
