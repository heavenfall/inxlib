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

#ifndef INXLIB_MEMORY_FACTORY_HPP
#define INXLIB_MEMORY_FACTORY_HPP

#include <inxlib/inx.hpp>
#include <cstddef>

namespace inx::memory {

namespace details {

template <typename Fact>
concept Factory_base = requires (Fact f)
{
	// not movable or copyable
	requires std::default_initializable<Fact>;
	requires !std::movable<Fact>;
	requires !std::copyable<Fact>;

	// required types
	typename Fact::value_type;
	typename Fact::pointer;
	typename Fact::size_type;

	// required functions, can be static or member
	{ f.alignment() } noexcept -> std::same_as<typename Fact::size_type>;
	{ f.element_size() } noexcept -> std::same_as<typename Fact::size_type>;
	{ f.setup() };
};

/**
 * Class is a array factory.  Provides allocation of array's.
 */
template <typename Fact>
concept ArrayFactory_base = details::Factory_base<Fact> && requires (Fact f)
{
	requires requires (typename Fact::pointer ptr, typename Fact::size_type elem) {
		// array allocation
		{ f.allocate(elem) } -> std::same_as<typename Fact::pointer>;
		{ f.deallocate(ptr, elem) };
	};
};

/**
 * Class is a single element factory.  Provides allocation of single elements.
 */
template <typename Fact>
concept SingleFactory_base = details::Factory_base<Fact> && requires (Fact f)
{
	requires requires (typename Fact::pointer ptr) {
		// single allocation
		{ f.create() } -> std::same_as<typename Fact::pointer>;
		{ f.destroy(ptr) };
	};
};

} // namespace details

class void_factory
{
public:
	using value_type = std::byte;
	using pointer = value_type*;
	using size_type = size_t;

	constexpr void_factory() noexcept = default;
	void_factory(const void_factory&) = delete;
	void_factory operator=(const void_factory&) = delete;

	constexpr void setup() noexcept
	{ }

	static consteval size_type alignment() noexcept { return alignof(max_align_t); }
	static consteval size_type element_size() noexcept { return 1; }

	constexpr pointer allocate(size_type)
	{ return {}; }
	constexpr void deallocate(pointer)
	{ }
	constexpr void deallocate(pointer, size_type)
	{ }
};

/**
 * Class is a array factory.  Provides allocation of array's.
 */
template <typename Fact>
concept ArrayFactory = details::ArrayFactory_base<Fact> && !details::SingleFactory_base<Fact>;

/**
 * Class is a single element factory.  Provides allocation of single elements.
 */
template <typename Fact>
concept SingleFactory = details::SingleFactory_base<Fact> && !details::ArrayFactory_base<Fact>;

/**
 * Class is a factory.  Designed to provide flexible memory generation.
 */
template <typename Fact>
concept Factory = ArrayFactory<Fact> || SingleFactory<Fact>;

/**
 * Class is a byte factory.  Provides allocation for byte object.
 */
template <typename Fact>
concept ByteFactory = ArrayFactory<Fact> && requires (Fact f)
{
	requires std::same_as<typename Fact::value_type, std::byte>;
	requires Fact::alignment() == alignof(max_align_t);
	requires Fact::element_size() == 1;
};

template <typename Fact>
concept VoidFactory = std::same_as<Fact, void_factory>;

static_assert(ByteFactory<void_factory> && VoidFactory<void_factory>, "void_factory must be a valid ByteFactory");

/**
 * Class is a factory.  Designed to provide flexible memory generation.
 */
template <typename Fact>
concept ElemFreeFactory = ArrayFactory<Fact> && requires (Fact f, typename Fact::pointer ptr)
{
	{ f.deallocate(ptr) };
};

/**
 * Class has memory reclaim functions.
 */
template <typename Fact>
concept ReclaimFactory = Factory<Fact> && requires (Fact f)
{
	{ f.release() };
	{ f.reclaim() };
};

} // namespace inx::memory

#endif // INXLIB_MEMORY_FACTORY_ARRAY_HPP
