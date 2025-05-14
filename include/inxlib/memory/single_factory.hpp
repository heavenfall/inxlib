/*
MIT License

Copyright (c) 2025 Ryan Hechenberger

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

#ifndef INXLIB_MEMORY_SINGLE_FACTORY_HPP
#define INXLIB_MEMORY_SINGLE_FACTORY_HPP

#include <inxlib/inx.hpp>
#include "factory.hpp"

namespace inx::memory {

template <ArrayFactory Upstream, size_t Elems, size_t Alignment = alignof(max_align_t)>
class single_factory : private Upstream
{
public:
	using upstream_factory = Upstream;
	using typename Upstream::value_type;
	using typename Upstream::size_type;
	using typename Upstream::pointer;

	constexpr size_type alignment() noexcept { return Alignment; }
	constexpr size_type element_size() noexcept { return Upstream::element_size() * Elems; }
	
	template <typename... T>
	constexpr void setup(T&&... args)
	{
		Upstream::setup(std::forward<T>(args)...);
	}

	[[nodiscard]] pointer create()
	{
		if constexpr (AlignByteFactory<Upstream>) {
			return Upstream::allocate(element_size(), alignment());
		} else {
			return Upstream::allocate(element_size());
		}
	}
	void destroy(pointer ptr)
	{
		if constexpr (AlignByteFactory<Upstream>) {
			return Upstream::deallocate(ptr, element_size(), alignment());
		} else {
			return Upstream::deallocate(ptr, element_size());
		}
	}

	upstream_factory& upstream() noexcept { return static_cast<upstream_factory&>(*this); }
	const upstream_factory& upstream() const noexcept { return static_cast<const upstream_factory&>(*this); }
};
template <typename Upstream, typename T>
using single_factory_type = single_factory<Upstream, sizeof(T), alignof(T)>;

} // namespace inx::memory

#endif // INXLIB_MEMORY_SINGLE_FACTORY_HPP
