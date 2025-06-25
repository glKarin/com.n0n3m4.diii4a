//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

CR_NAMESPACE_BEGIN

template <typename T> struct clear_extent {
   using type = T;
};

template <typename T> struct clear_extent <T[]> {
   using type = T;
};

template <typename T, size_t N> struct clear_extent <T[N]> {
   using type = T;
};

template <typename T, typename U> struct is_same {
   static constexpr bool value = false;
};

template <typename T> struct is_same <T, T> {
   static constexpr bool value = true;
};

namespace detail {
template <typename T> struct type_identity { using type = T; };
template <typename T> auto try_add_lvalue_reference (int) -> type_identity <T &>;
template <typename T> auto try_add_lvalue_reference (...) -> type_identity <T>;

template <typename T> auto try_add_rvalue_reference (int) -> type_identity <T &&>;
template <typename T> auto try_add_rvalue_reference (...) -> type_identity <T>;
}

template <typename T> struct add_lvalue_reference : decltype (detail::try_add_lvalue_reference <T> (0)) {};
template <typename T> struct add_rvalue_reference : decltype (detail::try_add_rvalue_reference <T> (0)) {};

template <typename T> constexpr bool always_false = false;

template<typename T> typename cr::add_rvalue_reference <T>::type declval () noexcept {
   static_assert (always_false <T>, "declval not allowed in an evaluated context");
}

template <typename T> struct decay {
   template <typename U> static U _t (U);
   using type = decltype (_t (cr::declval <T> ()));
};


template <bool B, class T = void> struct enable_if {};
template <typename T > struct enable_if <true, T> { typedef T type; };

template <bool b, class T = void> using enable_if_t = typename enable_if <b, T>::type;

// defined nullptr type
using nullptr_t = decltype (nullptr);

template <typename T, T v> struct integral_constant {
   static constexpr T value = v;

   using value_type = T;
   using type = integral_constant;

   constexpr operator value_type () const noexcept { return value; }
   constexpr value_type operator () () const noexcept { return value; }
};

template <bool B> using bool_constant = integral_constant <bool, B>;

using true_type = bool_constant <true>;
using false_type = bool_constant <false>;

CR_NAMESPACE_END
