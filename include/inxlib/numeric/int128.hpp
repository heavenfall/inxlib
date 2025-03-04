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

#ifndef INXLIB_NUMERIC_INT128_HPP
#define INXLIB_NUMERIC_INT128_HPP

#include <inxlib/inx.hpp>

namespace inx::numeric
{

#ifdef __SIZEOF_INT128__
#define INX_INT128
using int128 = __int128;
// using uint128 = unsigned __int128;
#else
#if 0
struct int128
{
	uint64_t part1;
	int64_t part2;
	constexpr int128& operator=(const int128&) noexcept = default;
	template <std::signed_integral T>
	constexpr int128& operator=(T v) noexcept
	{
		part1 = static_cast<uint64_t>(static_cast<int64_t>(v));
		part2 = v >= 0 ? 0ll : ~0ll;
	}
	template <std::unsigned_integral T>
	constexpr int128& operator=(T v) noexcept
	{
		part1 = static_cast<uint64_t>(v);
		part2 = 0;
	}

	template <std::integral T>
	constexpr operator T() const noexcept
	{
		return static_cast<T>(parts[0]);
	}
};

constexpr int128& operator+=(int128& lhs, int128 rhs) noexcept
{
	int64_t overflow = lhs.parts[0] + rhs.parts[0];
}

int128& operator>>=(size_t v);
#endif
#endif

} // namespace inx::numeric

#endif // INXLIB_NUMERIC_INT128_HPP
