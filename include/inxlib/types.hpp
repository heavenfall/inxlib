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

#ifndef INXLIB_TYPES_HPP_INCLUDED
#define INXLIB_TYPES_HPP_INCLUDED

#include "inx.hpp"

namespace inx {

namespace details {
template <typename T>
struct TypeTemplate : std::false_type
{};
template <template <typename...> typename T, typename... Ts>
struct TypeTemplate<T<Ts...>> : std::true_type
{};
template <typename T>
struct TypeTemplateSize : std::integral_constant<size_t, 0>
{};
template <template <typename...> typename T, typename... Ts>
struct TypeTemplateSize<T<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)>
{};
template <typename T>
struct TupleType : std::false_type
{};
template <typename... Ts>
struct TupleType<std::tuple<Ts...>> : std::true_type
{};
} // namespace details

template <typename T>
concept Plain = !std::is_void_v<T> && !std::is_reference_v<T> && !std::is_volatile_v<T>;
template <typename T>
concept ConstPlain = Plain<T> && std::is_const_v<T>;
template <typename T>
concept NonConstPlain = Plain<T> && !std::is_const_v<T>;

template <typename T>
concept TypeTemplate = details::TypeTemplate<T>::value;
template <typename T>
constexpr size_t TypeTemplateSize = details::TypeTemplateSize<T>::value;
template <typename T>
concept Tuple = details::TupleType<T>::value;

template <typename T, typename T2>
concept same_as_cvref = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<T2>>;

namespace details {

template <typename From, typename To>
struct copy_ref_
{
	using type = To;
};
template <typename From, typename To>
    requires(std::is_lvalue_reference_v<From>)
struct copy_ref_<From, To>
{
	using type = std::add_lvalue_reference_t<To>;
};
template <typename From, typename To>
    requires(std::is_rvalue_reference_v<From>)
struct copy_ref_<From, To>
{
	using type = std::add_rvalue_reference_t<To>;
};

template <typename From, typename To>
struct copy_const_
{
	using type = To;
};
template <typename From, typename To>
    requires(std::is_const_v<From>)
struct copy_const_<From, To>
{
	using type = std::add_const_t<To>;
};

template <typename From, typename To>
struct copy_volatile_
{
	using type = To;
};
template <typename From, typename To>
    requires(std::is_volatile_v<From>)
struct copy_volatile_<From, To>
{
	using type = std::add_volatile_t<To>;
};

} // namespace details

/// Copy cvref (if any) of type From to type To, overriding unto To.
/// e.g. From=const int&, To=volatile double, Result=const double&
template <typename From, typename To>
using copy_cvref = typename details::copy_ref_<
  From,
  typename details::copy_volatile_<
    std::remove_reference_t<From>,
    typename details::copy_const_<std::remove_reference_t<From>, std::remove_cvref_t<To>>::type>::type>::type;

/// Copy ref (if any) of type From to type To, overriding unto To.
/// e.g. From=const int&, To=volatile double, Result=double&
template <typename From, typename To>
using copy_ref = typename details::copy_ref_<From, std::remove_cvref_t<To>>::type;

/// Copy ref (if any) of type From to type To, overriding unto To.
/// e.g. From=const int&, To=volatile double&, Result=const double
template <typename From, typename To>
using copy_const = typename details::copy_const_<std::remove_reference_t<From>, std::remove_cvref_t<To>>::type;

/// Copy ref (if any) of type From to type To, overriding unto To.
/// e.g. From=const int&, To=volatile double&, Result=double
template <typename From, typename To>
using copy_volatile = typename details::copy_volatile_<std::remove_reference_t<From>, std::remove_cvref_t<To>>::type;

/// Copy cvref (if any) of type From to type To, overriding unto To.
/// e.g. From=const int&, To=volatile double, Result=const double&
template <typename From, typename To>
using copy_cv = typename details::copy_volatile_<
  std::remove_reference_t<From>,
  typename details::copy_const_<std::remove_reference_t<From>, std::remove_cvref_t<To>>::type>::type;

} // namespace inx

#endif // INXLIB_TYPES_HPP_INCLUDED
