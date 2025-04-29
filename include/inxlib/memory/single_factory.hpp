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
#include <inxlib/numeric/bits.hpp>
#include <cstdlib>
#include <cstddef>

namespace inx::memory {

template <typename BaseFactory, size_t Elems, size_t Alignment = alignof(max_align_t)>
class single_factory : private BaseFactory
{
public:
	using base_factory = BaseFactory;
	using typename base_factory::value_type;
	using typename base_factory::size_type;
	using typename base_factory::pointer;

	constexpr size_type alignment() noexcept { return BaseFactory::alignment(); }
	consteval size_type element_size() noexcept { return BaseFactory::element_size() * Elems; }

	pointer create(size_type elems)
	{
		return BaseFactory::allocate(element_size());
	}
	void destroy(pointer ptr)
	{
		return BaseFactory::deallocate(ptr, element_size());
	}

	base_factory& base() noexcept { return static_cast<base_factory&>(*this); }
	const base_factory& base() const noexcept { return static_cast<const base_factory&>(*this); }
};

} // namespace inx::memory

#endif // INXLIB_MEMORY_SINGLE_FACTORY_HPP
