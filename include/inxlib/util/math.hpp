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

#ifndef INXLIB_UTIL_MATH_HPP
#define INXLIB_UTIL_MATH_HPP

#include <inxlib/inx.hpp>
#include "bits.hpp"
#include "numeric_types.hpp"

namespace inx::util
{

template <typename T1, typename T2, typename = std::enable_if_t<std::is_integral_v<T1> && std::is_integral_v<T2>>>
auto div(T1 a, T2 b) noexcept
{
	assert(b != 0);
	if constexpr (std::is_unsigned_v<T1> != std::is_unsigned_v<T2>) {
		using ir = std::common_type_t<std::make_signed_t<T1>, std::make_signed_t<T2>>;
		using rr = raise_integral_level_t<ir>;
		if constexpr (std::is_unsigned_v<T1>) { assert(static_cast<size_t>(a) <= static_cast<size_t>(std::numeric_limits<ssize_t>::max())); }
		else { assert(static_cast<size_t>(b) <= static_cast<size_t>(std::numeric_limits<ssize_t>::max())); }
		return static_cast<rr>(a) / static_cast<rr>(b);
	} else {
		using ir = std::common_type_t<T1, T2>;
		return static_cast<ir>(a) / static_cast<ir>(b);
	}
}

#if 0
template <typename T1, typename T2, typename = std::enable_if_t<std::is_integral_v<T1> && std::is_integral_v<T2>>>
auto div_ceil(T1 a, T2 b) noexcept
{
	if constexpr (std::is_unsigned_v<T2>) {
		if constexpr (std::is_unsigned_v<T1>) {
			using ir = std::common_type_t<T1, T2>;
			using rr = std::raise_integral_level_t<ir>;
			return div(static_cast<rr>(a)+static_cast<rr>(b)-1, static_cast<rr>(b));
		} else {
			assert(b != 0);
			using ir = std::common_type_t<T1, T2>;
			using rr2 = std::std::raise_integral_level_t<T2>;
			if (a >= 0) return 
		}
	}
}
#endif

template <typename T1, typename T2, typename = std::enable_if_t<std::is_integral_v<T1> && std::is_integral_v<T2> && std::is_signed_v<T1>>>
T2 div_modulo(T1& a, T2 b) noexcept
{
	assert(b > 0);
	using t2u = std::make_unsigned_t<T2>;

	t2u m = a % b; a /= b;
	if (m < 0) { --a; m += b; }
	return m;
}

template <typename F>
constexpr F ipow_(F val, uint32 p)
{
	static_assert(std::is_floating_point_v<F>, "F must be a floating point number");
	F res = 1.0;
	for (uint32 i = 1u << clz_index(p); i != 0; i >>= 1) {
		res *= res;
		if ( (p & i) != 0 )
			res *= val;
	}
	return res;
}
template <typename F>
constexpr F ipow(F val, int32 p)
{
	if constexpr (std::is_floating_point_v<F>) {
		// raise F to power of p
		if (p == 0)
			return 1;
		if (p > 0)
			return ipow_(val, static_cast<uint32>(p));
		else
			return static_cast<F>(1.0) / ipow_(val, static_cast<uint32>(-p));
	} else {
		if (p <= 0)
			return 1;
		F ans = val;
		while (--p > 0) {
			ans *= val;
		}
		return ans;
	}
}

} // namespace inx

#endif // INXLIB_UTIL_MATH_HPP
