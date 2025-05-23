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

#ifndef INXLIB_MEMORY_FACTORY_ADAPTOR_HPP
#define INXLIB_MEMORY_FACTORY_ADAPTOR_HPP

#include <inxlib/inx.hpp>
#include "factory.hpp"
#include <inxlib/types.hpp>
#include <cstring>
#include <memory>
#include <tuple>

namespace inx::memory {

constexpr size_t reuse_size(size_t size) noexcept
{
	return std::max(size, sizeof(void*));
}
template <typename T>
constexpr size_t reuse_size() noexcept
{
	return reuse_size(sizeof(T));
}

template <SingleFactory Upstream>
	requires (!FactoryTraitAny<Upstream, FactoryOwn | FactoryPointer>)
class reuse_adaptor : public Upstream
{
public:
	using typename Upstream::value_type;
	using typename Upstream::size_type;
	using typename Upstream::pointer;

	static consteval uint32_t traits() noexcept { return (Upstream::traits() & ~FactoryNoFree) | FactoryReuse; }

	using Upstream::alignment;
	using Upstream::element_size;

	~reuse_adaptor()
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
		if constexpr (requires { { Upstream::release(free_upstream) }; }) {
			// Upstream has release, just call it
			Upstream::release(free_upstream);
		} else if constexpr (!FactoryTraitAll<Upstream, FactoryNoFree>) {
			// only call Upstream::destory if upsteam uses one has one 
			if (free_upstream) {
				pointer p = m_reuse;
				while (p) {
					pointer pnext = reuse_get(p);
					Upstream::destroy(p);
					p = pnext;
				}
			}
		}
		m_reuse = nullptr;
	}
	void reclaim() requires ReclaimFactory<Upstream>
	{
		Upstream::reclaim();
		m_reuse = nullptr;
	}

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


template <Factory Upstream, ByteFactory Overflow = void_factory>
class overflow_pattern : public Upstream
{
public:
	using typename Upstream::pointer;
	using typename Upstream::size_type;
	using overflow_type = Overflow;

	template <Tuple OverflowTuple, typename... T>
	constexpr bool setup(OverflowTuple&& setup_overflow, T&&... args)
	{
		if (!Upstream::setup(std::forward<T>(args)...))
			return false;
		if (std::apply([this](auto&&... ts) {
				this->setup(std::forward<decltype(ts)>(ts)...);
			}, setup_overflow)) {
			return false;
		}
		return true;
	}

	overflow_type& overflow() noexcept { return m_overflow; }
	const overflow_type& overflow() const noexcept { return m_overflow; }

protected:
	constexpr pointer overflow_allocate(size_type elems)
	{
		return m_overflow.allocate(elems);
	}
	constexpr void overflow_deallocate(pointer ptr, size_type elems)
	{
		return m_overflow.deallocate(ptr, elems);
	}

protected:
	[[no_unique_address]] Overflow m_overflow;
};

template <Factory Upstream, ByteFactory Overflow>
	requires VoidFactory<Overflow>
class overflow_pattern<Upstream, Overflow> : public Upstream
{
public:
	using typename Upstream::pointer;
	using typename Upstream::size_type;

	using Upstream::Upstream;

protected:
	template <Factory Fact>
	static ByteFactory auto& overflow_upstream(Fact& upsteam_fact) noexcept
	{
		if constexpr (ByteFactory<Fact>) {
			return upsteam_fact;
		} else {
			return overflow_upstream(upsteam_fact.upstream());
		}
	}

public:
	using overflow_type = std::remove_cvref_t<decltype(overflow_upstream(std::declval<Upstream>().upstream()))>;
	overflow_type& overflow() noexcept { return overflow_upstream(Upstream::upstream()); }
	const overflow_type& overflow() const noexcept { return overflow_upstream(Upstream::upstream()); }

protected:
	constexpr std::byte* overflow_allocate(size_type elems)
	{
		return overflow().allocate(elems);
	}
	constexpr void overflow_deallocate(std::byte* ptr, size_type elems)
	{
		return overflow().deallocate(ptr, elems);
	}

protected:
	[[no_unique_address]] Overflow m_overflow;
};

} // namespace inx::memory

#endif // INXLIB_MEMORY_SINGLE_FACTORY_HPP
