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

#ifndef INXLIB_UTIL_FUNCTIONS_HPP
#define INXLIB_UTIL_FUNCTIONS_HPP

#include <inxlib/inx.hpp>
#include <memory>

namespace inx::util {

template <typename Ftype>
struct functor_ptr
{
	using type = Ftype;

	functor_ptr() noexcept
	  : m_ptr(nullptr)
	{
	}
	functor_ptr(type* ptr)
	  : m_ptr(ptr)
	{
	}

	functor_ptr<Ftype>& operator=(type* ptr) noexcept { m_ptr = ptr; }

	template <typename... Args>
	std::enable_if_t<std::is_invocable_v<type, Args&&...>,
	                 std::invoke_result_t<type, Args&&...>>
	operator()(Args&&... args) const
	  noexcept(std::is_nothrow_invocable_v<type, Args&&...>)
	{
		return std::invoke(*m_ptr, std::forward<Args>(args)...);
	}

	type* m_ptr;
};

class any_ptr : public std::unique_ptr<void, functor_ptr<void(void*)>>
{
public:
	using std::unique_ptr<void, functor_ptr<void(void*)>>::unique_ptr;
	template <typename T>
	any_ptr(T* ptr) noexcept
	  : unique_ptr(static_cast<void*>(ptr),
	               functor_ptr<void(void*)>([](void* p) noexcept {
		               std::destroy_at<T>(static_cast<T*>(p));
	               }))
	{
	}
	using unique_ptr<void, functor_ptr<void(void*)>>::operator=;
	any_ptr& operator=(any_ptr&&) = default;
	template <typename T>
	any_ptr& operator=(std::unique_ptr<T>&& ptr)
	{
		reset(ptr.release());
		return *this;
	}
	void reset(std::nullptr_t = nullptr) noexcept { unique_ptr::reset(); }
	template <typename T>
	void reset(T* ptr) noexcept
	{
		if (ptr == nullptr) {
			unique_ptr::reset();
		} else {
			(*this) = unique_ptr(static_cast<void*>(ptr),
			                     functor_ptr<void(void*)>([](void* p) noexcept {
				                     std::destroy_at<T>(static_cast<T*>(p));
			                     }));
		}
	}
};

template <typename T, typename Deleter = std::default_delete<T>>
class unique_clear_ptr : public std::unique_ptr<T, Deleter>
{
public:
	using std::unique_ptr<T, Deleter>::unique_ptr;
	unique_clear_ptr(const unique_clear_ptr&) noexcept
	  : std::unique_ptr<T, Deleter>()
	{
	}
	using std::unique_ptr<T, Deleter>::operator=;
};

template <auto F>
struct functor
{
	using type = decltype(F);

	template <typename... Args>
	std::enable_if_t<std::is_invocable_v<type, Args&&...>,
	                 std::invoke_result_t<type, Args&&...>>
	operator()(Args&&... args) const
	  noexcept(std::is_nothrow_invocable_v<type, Args&&...>)
	{
		return std::invoke(F, std::forward<Args>(args)...);
	}
};

template <typename D>
struct owned_functor : D
{
	bool owned;

	owned_functor() noexcept(std::is_nothrow_default_constructible_v<D>)
	  : owned(false)
	{
	}
	owned_functor(bool own) noexcept(std::is_nothrow_default_constructible_v<D>)
	  : owned(own)
	{
	}
	template <typename DT,
	          typename = std::enable_if_t<std::is_constructible_v<D, DT&&>>>
	owned_functor(DT&& d, bool own = false) noexcept(
	  std::is_nothrow_constructible_v<D, DT&&>)
	  : D(std::forward<DT>(d))
	  , owned(own)
	{
	}
	owned_functor(const owned_functor<D>&) = default;
	owned_functor(owned_functor<D>&&) = default;

	owned_functor<D>& operator=(const owned_functor<D>&) = default;
	owned_functor<D>& operator=(owned_functor<D>&&) = default;

	template <typename... Args>
	void operator()(Args&&... args) noexcept(
	  noexcept(std::declval<D>()(std::forward<Args>(args)...)))
	{
		if (owned)
			D::operator()(std::forward<Args>(args)...);
	}
};

template <typename T>
using owned_delete = owned_functor<std::default_delete<T>>;

template <typename T, typename Fn>
struct assignment_adaptor
{
	T* obj;
	std::remove_reference_t<Fn> func;

	assignment_adaptor(T& o, Fn&& fn) noexcept(
	  std::is_nothrow_constructible_v<decltype(func),
	                                  decltype(std::forward<Fn>(fn))>)
	  : obj(&o)
	  , func(std::forward<Fn>(fn))
	{
	}

	template <typename V>
	assignment_adaptor<T, Fn>& operator=(const V&& value) noexcept(
	  std::is_nothrow_invocable_v<decltype(func),
	                              decltype(*obj),
	                              decltype(std::forward<const V>(value))>)
	{
		std::invoke(func, *obj, std::forward<const V>(value));
		return *this;
	}
};
template <typename Fn>
struct assignment_adaptor<void, Fn>
{
	std::remove_reference_t<Fn> func;

	assignment_adaptor(Fn&& fn) noexcept(
	  std::is_nothrow_constructible_v<decltype(func),
	                                  decltype(std::forward<Fn>(fn))>)
	  : func(std::forward<Fn>(fn))
	{
	}

	template <typename V>
	assignment_adaptor<void, Fn>& operator=(const V&& value) noexcept(
	  std::is_nothrow_invocable_v<decltype(func),
	                              decltype(std::forward<const V>(value))>)
	{
		std::invoke(func, std::forward<const V>(value));
		return *this;
	}
};

template <typename T, typename Fn>
assignment_adaptor(T& o, Fn&& fn) -> assignment_adaptor<T, Fn>;
template <typename Fn>
assignment_adaptor(Fn&& fn) -> assignment_adaptor<void, Fn>;

template <typename Fn>
struct destruct_adaptor
{
	std::remove_reference_t<Fn> func;

	destruct_adaptor(Fn&& fn) noexcept(
	  std::is_nothrow_constructible_v<decltype(func),
	                                  decltype(std::forward<Fn>(fn))>)
	  : func(std::forward<Fn>(fn))
	{
	}

	~destruct_adaptor() { func(); }
};

template <typename Fn>
destruct_adaptor(Fn&& fn) -> destruct_adaptor<Fn>;

template <typename T, typename Fn = void (*)(T* obj) noexcept>
struct destruct_object_adaptor
{
protected:
	T* obj;
	std::remove_reference_t<Fn> func;

public:
	destruct_object_adaptor() noexcept(
	  noexcept(std::is_nothrow_constructible_v<decltype(func)>))
	  : obj(nullptr)
	  , func{}
	{
	}
	template <typename FnImp>
	destruct_object_adaptor(T* o, FnImp&& fn) noexcept(noexcept(
	  std::is_nothrow_constructible_v<decltype(func),
	                                  decltype(std::forward<FnImp>(fn))>))
	  : obj(o)
	  , func(std::forward<FnImp>(fn))
	{
	}
	destruct_object_adaptor(const destruct_object_adaptor<T, Fn>&) = delete;
	destruct_object_adaptor(destruct_object_adaptor<T, Fn>&& other) noexcept(
	  noexcept(std::is_nothrow_move_constructible_v<decltype(func)>))
	  : obj(other.obj)
	  , func(std::move(other.func))
	{
		other.obj = nullptr;
	}

	~destruct_object_adaptor()
	{
		if (obj)
			func(obj);
	}

	destruct_object_adaptor<T, Fn>& operator=(
	  const destruct_object_adaptor<T, Fn>&) = delete;
	destruct_object_adaptor<T, Fn>&
	operator=(destruct_object_adaptor<T, Fn>&& other) noexcept(
	  noexcept(std::is_nothrow_move_assignable_v<decltype(func)>))
	{
		if (obj)
			func(obj);
		obj = other.obj;
		other.obj = nullptr;
		func = std::move(other.func);
	}

	void assign(T* o) noexcept
	{
		if (obj)
			func(obj);
		obj = o;
	}
	template <typename FnImp>
	void assign(T* o, FnImp&& fn) noexcept(
	  noexcept(std::is_nothrow_assignable_v<decltype(func), FnImp&&>))
	{
		if (obj)
			func(obj);
		obj = o;
		func = std::forward<FnImp>(fn);
	}
	template <typename ObjType, typename... Args>
	void emplace(Args&&... args)
	{
		if (obj)
			func(obj);
		obj = new ObjType(std::forward<Args>(args)...);
		func = [](T* o) noexcept { delete static_cast<ObjType*>(o); };
	}

	T* get() noexcept { return obj; }
	const T* get() const noexcept { return obj; }
};

} // namespace inx::util

#endif // INXLIB_UTIL_FUNCTIONS_HPP
