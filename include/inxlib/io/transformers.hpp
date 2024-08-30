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

#ifndef INXLIB_IO_TRANSFORMERS_HPP
#define INXLIB_IO_TRANSFORMERS_HPP

#include <cmath>
#include <inxlib/inx.hpp>
#include <inxlib/util/math.hpp>
#include <iomanip>
#include <limits>
#include <ostream>

namespace inx::io {

template <typename T>
std::enable_if_t<std::is_floating_point_v<std::decay_t<T>>, bool>
float_is_integer(T x)
{
	T ign;
	return std::modf(x, &ign) == 0;
}

template <typename Int, typename T>
std::enable_if_t<std::is_integral_v<std::decay_t<Int>> &&
                   std::is_floating_point_v<std::decay_t<T>>,
                 bool>
float_fits_integer(T x)
{
	if constexpr (std::numeric_limits<Int>::digits10 >
	              std::numeric_limits<T>::digits10) {
		return float_is_integer(x);
	} else {
		return float_is_integer(x) &&
		       std::abs(x) <
		         std::pow(static_cast<T>(10),
		                  static_cast<int>(std::numeric_limits<Int>::digits10));
	}
}

template <typename T>
struct accurate_number
{
	static_assert(std::is_same_v<std::decay_t<T>, T> &&
	                (std::is_integral_v<T> || std::is_floating_point_v<T>),
	              "T must be fully decayed and a number");
	constexpr accurate_number(T val) noexcept
	  : m_val(val)
	{
	}

	T m_val;
};

template <typename T>
accurate_number(T x) -> accurate_number<std::decay_t<T>>;

template <typename T>
std::ostream&
operator<<(std::ostream& out, const accurate_number<T>& num)
{
	if constexpr (std::is_integral_v<T>) {
		out << num.m_val;
	} else {
		double nabs = std::abs(static_cast<double>(num.m_val));
		constexpr size_t doublerep =
		  std::numeric_limits<double>::max_digits10 - 3;
		if (float_fits_integer<ssize_t>(nabs)) {
			out << std::fixed << static_cast<ssize_t>(num.m_val);
		} else if (nabs >= 10e10 || nabs <= 10e-6) {
			out << std::scientific << std::setprecision(doublerep) << num.m_val;
		} else {
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(2 * doublerep) << nabs;
			std::string toStr = oss.str();
			std::size_t deciS = toStr.find('.');
			if (deciS == std::string::npos) {
				out << toStr; // should never occur, but just in case print
				              // integer
				return out;
			}
			int deci = static_cast<int>(deciS);
			int frac = static_cast<int>(doublerep) - deci;
			if (frac <= 0) { // no space left for decimal, should never occur
				             // but just in case
				toStr.erase(toStr.begin() + deci, toStr.end());
				out << toStr;
				return out;
			}
			deci += 1;
			bool rndUp = toStr[deci + frac] >= '5'; // round up
			toStr.erase(toStr.begin() + (deci + frac), toStr.end());
			if (rndUp) {
				int i;
				for (i = deci + frac - 1; i >= deci; i--) {
					if (toStr[i] == '9') {
						toStr[i] = '0';
					} else {
						toStr[i]++;
						break;
					}
				}
				if (i == deci - 1) { // round up integer
					oss.str(std::string());
					oss << std::setprecision(0) << std::round(num.m_val);
					out << oss.str();
					return out;
				}
			}
			while (toStr.back() == '0') {
				toStr.pop_back();
			}
			if (toStr.back() == '.') {
				toStr.pop_back();
			}
			out << (num.m_val < 0 ? "-" : "") << toStr;
		}
	}
	return out;
}

} // namespace inx::io

#endif // INXLIB_IO_TRANSFORMERS_HPP
