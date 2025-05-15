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
#include <cstring>
#include <memory>

namespace inx::memory {

template <ArrayFactory Upstream, size_t Elems, size_t Alignment = alignof(max_align_t)>
class single_factory : private Upstream
{
public:
	using upstream_factory = Upstream;
	using typename Upstream::value_type;
	using typename Upstream::size_type;
	using typename Upstream::pointer;

	static consteval uint32_t traits() noexcept { return FactoryDefault; }

	constexpr size_type alignment() noexcept { return Alignment; }
	constexpr size_type element_size() noexcept { return Upstream::element_size() * Elems; }
	
	template <typename... T>
	constexpr bool setup(T&&... args)
	{
		return Upstream::setup(std::forward<T>(args)...);
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

template <SingleFactory Upstream>
class single_reuse_adaptor : private Upstream
{
public:
	using upstream_factory = Upstream;
	using typename Upstream::value_type;
	using typename Upstream::size_type;
	using typename Upstream::pointer;

	static consteval uint32_t traits() noexcept { return FactoryReuse; }

	using Upstream::alignment;
	using Upstream::element_size;

	~single_reuse_adaptor()
	{
		release(true);
	}

	template <typename... T>
	constexpr bool setup(T&&... args)
	{
		if (!Upstream::setup(std::forward<T>(args)...))
			return false;
		if (Upstream::element_size() < sizeof(pointer))
			return false;
		return true;
	}

	[[nodiscard]] pointer create()
	{
		pointer res;
		if (m_reuse) {
			// reuse
			res = m_reuse;
			m_reuse = reuse_get(res);
		} else {
			// allocate new
			res = Upstream::create();
		}
		return res;
	}
	void destroy(pointer ptr)
	{
		// keep for reuse
		reuse_set(ptr, m_reuse);
		m_reuse = ptr;
	}

	/// @brief only releases memory calimed for reuse
	/// @param free_upstream destorys memory upstream
	void release(bool free_upstream = true)
	{
		if (free_upstream) {
			pointer p = m_reuse;
			while (p) {
				pointer pnext = reuse_get(p);
				Upstream::destroy(p);
				p = pnext;
			}
		}
		m_reuse = nullptr;
	}

	upstream_factory& upstream() noexcept { return static_cast<upstream_factory&>(*this); }
	const upstream_factory& upstream() const noexcept { return static_cast<const upstream_factory&>(*this); }

protected:
	static pointer reuse_get(pointer p) noexcept
	{
		assert(element_size() >= sizeof(pointer));
		// handle unaligned access
		pointer value;
		std::memcpy(&value, p, sizeof(pointer));
		return value;
	}
	static void reuse_set(pointer p, pointer value) noexcept
	{
		assert(element_size() >= sizeof(pointer));
		// handle unaligned access
		std::memcpy(p, &value, sizeof(pointer));
	}

protected:
	pointer* m_reuse = nullptr;
};
template <typename Upstream, typename T>
using single_factory_type = single_factory<Upstream, sizeof(T), alignof(T)>;

} // namespace inx::memory

#endif // INXLIB_MEMORY_SINGLE_FACTORY_HPP
