#ifndef INXLIB_UTIL_VIRTUAL_POINTER_HPP
#define INXLIB_UTIL_VIRTUAL_POINTER_HPP

#include <inxlib/inx.hpp>

namespace inx::util
{

template <typename... Ts>
class virtual_pointer;

namespace details {

template <typename... Ts>
struct virtual_pointer_next;

template <typename T1, typename T2, typename... Ts>
struct virtual_pointer_next<T1, T2, Ts...>
{
	static_assert(std::is_const<T1>::value == std::is_const<T2>::value);
	using type = virtual_pointer<T2, Ts...>;
};

template <typename T1>
struct virtual_pointer_next<T1>
{
	using type = std::conditional_t<std::is_const_v<T1>, virtual_pointer<const void>, virtual_pointer<void>>;
};

template <typename... Ts>
using virtual_pointer_next_t = typename virtual_pointer_next<Ts...>::type;

}

template <typename T, typename... Ts>
class virtual_pointer<T, Ts...> : public details::virtual_pointer_next_t<T, Ts...>
{
protected:
	using super = typename details::virtual_pointer_next<T, Ts...>::type;
	using void_type = typename super::void_type;
	using vfun_type = typename super::vfun_type;
public:
	virtual_pointer(T* ptr) noexcept : super(ptr, &cast_up) { }
	virtual_pointer(const virtual_pointer<T, Ts...>&) noexcept = default;

	T* get() const noexcept { return static_cast<T*>(get(this->m_ptr)); }
\
	T* operator->() const noexcept { return get(); }
	T& operator*() const noexcept { return static_cast<T&>(*get()); }

	template <typename To>
	To* virtual_cast() const noexcept
	{
		if constexpr (!std::is_const<T>::value) {
			using ToC = std::remove_const_t<To>;
			if constexpr (std::is_same<ToC, void>::value) {
				return this->m_ptr;
			} else if constexpr (std::is_same<ToC, virtual_pointer<T, Ts...>>::value) {
				return get();
			} else if constexpr (std::is_base_of<ToC, T>::value) {
				return static_cast<To*>(get());
			} else if constexpr (std::is_base_of<T, ToC>::value) {
				return static_cast<ToC*>(this->m_cast(this->m_ptr, typeid(ToC)));
			} else {
				return nullptr;
			}
		} else {
			if constexpr (!std::is_const<To>::value) {
				static_assert(std::is_const<To>::value, "can't cast away qualifiers");
				return nullptr;
			} else if constexpr (std::is_same<To, const void>::value) {
				return this->m_ptr;
			} else if constexpr (std::is_same<To, virtual_pointer<T, Ts...>>::value) {
				return get();
			} else if constexpr (std::is_base_of<To, T>::value) {
				return static_cast<To*>(get());
			} else if constexpr (std::is_base_of<T, To>::value) {
				return static_cast<To*>(this->m_cast(this->m_ptr, typeid(To)));
			} else {
				return nullptr;
			}
		}
	}
	
protected:
	virtual_pointer(T* l_ptr, vfun_type* l_cast) noexcept : super(l_ptr, l_cast) { }
	static void_type* get(void_type* ptr) noexcept { return static_cast<T*>(super::get(ptr)); }
	static void_type* cast_up(void_type* ptr, const std::type_info& ti [[maybe_unused]]) noexcept
	{
		if (ti == typeid(T)) {
			return get(ptr);
		} else {
			return super::cast_up(ptr, ti);
		}
	}
};

template <>
class virtual_pointer<void>
{
protected:
	using void_type = void;
	using vfun_type = void_type* (void_type*, const std::type_info&) noexcept;
public:
	virtual_pointer() noexcept = delete;
	virtual_pointer(std::nullptr_t) noexcept : m_ptr(nullptr), m_cast(&cast_up) { }
	virtual_pointer(void* ptr) noexcept : m_ptr(ptr), m_cast(&cast_up) { }
	explicit virtual_pointer(const virtual_pointer<void>&) noexcept = default;
	virtual_pointer(virtual_pointer<void>&&) noexcept = default;

	void* get() const noexcept { return m_ptr; }

	operator bool() const noexcept { return m_ptr; }

	template <typename To>
	To* virtual_cast() const noexcept
	{
		if constexpr (!std::is_const<To>::value) {
			if constexpr (std::is_same<To, void>::value) {
				return m_ptr;
			} else {
				return static_cast<To*>(m_cast(m_ptr, typeid(To)));
			}
		} else {
			if constexpr (std::is_same<To, const void>::value) {
				return m_ptr;
			} else {
				return static_cast<To*>(m_cast(m_ptr, typeid(std::remove_const_t<To>)));
			}
		}
	}

protected:
	virtual_pointer(void_type* l_ptr, vfun_type* l_cast) noexcept : m_ptr(l_ptr), m_cast(l_cast) { }
	static void_type* get(void_type* ptr) noexcept { return ptr; }
	static void_type* cast_up(void_type*, const std::type_info& ti [[maybe_unused]]) noexcept
	{
		assert(ti != typeid(void_type));
		return nullptr; // will never be called with type void
	}

	void_type* m_ptr;
	vfun_type* m_cast;
};

template <>
class virtual_pointer<const void>
{
protected:
	using void_type = const void;
	using vfun_type = void_type* (void_type*, const std::type_info&) noexcept;
public:
	virtual_pointer() noexcept = delete;
	virtual_pointer(std::nullptr_t) noexcept : m_ptr(nullptr), m_cast(&cast_up) { }
	virtual_pointer(const void* ptr) noexcept : m_ptr(ptr), m_cast(&cast_up) { }
	explicit virtual_pointer(const virtual_pointer<const void>&) noexcept = default;
	virtual_pointer(virtual_pointer<const void>&&) noexcept = default;

	const void* get() const noexcept { return m_ptr; }

	operator bool() const noexcept { return m_ptr; }

	template <typename To>
	To* virtual_cast() const noexcept
	{
		if constexpr (!std::is_const<To>::value) {
			static_assert(std::is_const<To>::value, "can't cast away qualifiers");
			return nullptr;
		} else if constexpr (std::is_same<To, const void>::value) {
			return m_ptr;
		} else {
			return m_cast(m_ptr, typeid(To));
		}
	}

protected:
	virtual_pointer(void_type* l_ptr, vfun_type* l_cast) noexcept : m_ptr(l_ptr), m_cast(l_cast) { }
	static void_type* get(void_type* ptr) noexcept { return ptr; }
	static void_type* cast_up(void_type*, const std::type_info& ti [[maybe_unused]]) noexcept
	{
		assert(ti != typeid(void_type));
		return nullptr; // will never be called with type void
	}

	void_type* m_ptr;
	vfun_type* m_cast;
};

template <typename... Ts>
using virtual_pointer_const = virtual_pointer<std::add_const_t<Ts>...>;

template <typename P2, typename... VPs>
bool operator==(const virtual_pointer<VPs...>& lhs, const P2* rhs) noexcept
{
	return lhs.get() == rhs;
}
template <typename P2, typename... VPs>
bool operator!=(const virtual_pointer<VPs...>& lhs, const P2* rhs) noexcept
{
	return lhs.get() != rhs;
}
template <typename P2, typename VP1, typename... VPs>
bool operator<(const virtual_pointer<VP1, VPs...>& lhs, const P2* rhs) noexcept
{
	return std::less<std::common_type_t<const VP1*, const P2*>>(lhs.get(), rhs);
}
template <typename P2, typename VP1, typename... VPs>
bool operator>(const virtual_pointer<VP1, VPs...>& lhs, const P2* rhs) noexcept
{
	return std::greater<std::common_type_t<const VP1*, const P2*>>(lhs.get(), rhs);
}
template <typename P2, typename... VPs>
bool operator<=(const virtual_pointer<VPs...>& lhs, const P2* rhs) noexcept
{
	return !(lhs > rhs);
}
template <typename P2, typename... VPs>
bool operator>=(const virtual_pointer<VPs...>& lhs, const P2* rhs) noexcept
{
	return !(lhs < rhs);
}

}

#endif // INXLIB_UTIL_VIRTUAL_POINTER_HPP
