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

#ifndef INXLIB_UTIL_ITERATOR_HPP
#define INXLIB_UTIL_ITERATOR_HPP

#include <concepts>
#include <inxlib/inx.hpp>

namespace inx::util {

template <typename Iter, typename ValueType>
struct RandomIteratorWrapper;

// namespace details {

// } // namespace details

// template <typename T>
// concept random_iterator_wrapper = requires {

// };

template <typename Iter, typename ValueType>
struct RandomIteratorWrapper
{
	// using self = RandomIteratorWrapper<Iter, ValueType>;
	using value_type = ValueType;
	using difference_type = ptrdiff_t;
	using reference = ValueType&;
	using pointer = ValueType*;

	difference_type pos_;
	auto operator<=>(const RandomIteratorWrapper&) const noexcept = default;
	template <typename Iter2, typename ValueType2>
	    requires std::same_as<std::remove_cv_t<ValueType>,
	                          std::remove_cv_t<ValueType2>>
	auto operator<=>(
	  const RandomIteratorWrapper<Iter2, ValueType2>& o) const noexcept
	{
		return pos_ <=> o.pos_;
	}

	RandomIteratorWrapper() noexcept
	  : pos_{}
	{
	}
	explicit RandomIteratorWrapper(difference_type pos) noexcept
	  : pos_(pos)
	{
	}
	RandomIteratorWrapper(RandomIteratorWrapper&&) noexcept = default;
	RandomIteratorWrapper(const RandomIteratorWrapper&) noexcept = default;
	~RandomIteratorWrapper() noexcept = default;

	RandomIteratorWrapper& operator=(RandomIteratorWrapper&&) noexcept =
	  default;
	RandomIteratorWrapper& operator=(const RandomIteratorWrapper&) noexcept =
	  default;

	// adjust index
	Iter& operator++() noexcept
	{
		pos_ += 1;
		return static_cast<Iter&>(*this);
	}
	Iter operator++(int) noexcept
	{
		Iter res = static_cast<const Iter&>(std::as_const(*this));
		pos_ += 1;
		return res;
	}
	Iter& operator--() noexcept
	{
		pos_ -= 1;
		return static_cast<Iter&>(*this);
	}
	Iter operator--(int) noexcept
	{
		Iter res = static_cast<const Iter&>(std::as_const(*this));
		pos_ -= 1;
		return res;
	}

	// dereference
	reference operator*() const noexcept
	{
		return static_cast<const Iter&>(*this)[pos_];
	}
	pointer operator->() const noexcept
	{
		return &static_cast<const Iter&>(this)[pos_];
	}
};

// +
template <typename Iter, typename ValueType>
Iter&
operator+=(RandomIteratorWrapper<Iter, ValueType>& it, ptrdiff_t n) noexcept
{
	it.pos_ += n;
	return static_cast<Iter>(it);
}
template <typename Iter, typename ValueType>
Iter
operator+(const RandomIteratorWrapper<Iter, ValueType>& it,
          ptrdiff_t n) noexcept
{
	Iter res = static_cast<const Iter&>(it);
	res.pos_ += n;
	return res;
}
template <typename Iter, typename ValueType>
Iter
operator+(ptrdiff_t n,
          const RandomIteratorWrapper<Iter, ValueType>& it) noexcept
{
	return it + n;
}
// -
template <typename Iter, typename ValueType>
Iter&
operator-=(RandomIteratorWrapper<Iter, ValueType>& it, ptrdiff_t n) noexcept
{
	it.pos_ -= n;
	return static_cast<Iter>(it);
}
template <typename Iter, typename ValueType>
Iter
operator-(const RandomIteratorWrapper<Iter, ValueType>& it,
          ptrdiff_t n) noexcept
{
	Iter res = static_cast<const Iter&>(it);
	res.pos_ -= n;
	return res;
}
template <typename Iter, typename ValueType>
ptrdiff_t
operator-(const RandomIteratorWrapper<Iter, ValueType>& it,
          const RandomIteratorWrapper<Iter, ValueType>& it2) noexcept
{
	return it2.pos_ - it.pos_;
}

} // namespace inx::util

#endif // INXLIB_UTIL_ITERATOR_HPP
