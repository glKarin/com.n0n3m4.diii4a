//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/basic.h>
#include <crlib/movable.h>

CR_NAMESPACE_BEGIN

// simple pair (twin)
template <typename A, typename B> class Twin final {
public:
   A first;
   B second;

public:
   template <typename T, typename U> constexpr Twin (T &&a, U &&b) : first (cr::forward <T> (a)), second (cr::forward <U> (b)) {}
   template <typename T, typename U> constexpr Twin (const Twin <T, U> &rhs) : first (rhs.first), second (rhs.second) {}
   template <typename T, typename U> constexpr Twin (Twin <T, U> &&rhs) noexcept : first (cr::move (rhs.first)), second (cr::move (rhs.second)) {}

public:
   explicit constexpr Twin () = default;
   ~Twin () = default;

public:
   template <typename T, typename U> constexpr Twin &operator = (const Twin <T, U> &rhs) {
      first = rhs.first;
      second = rhs.second;

      return *this;
   }

   template <typename T, typename U> constexpr Twin &operator = (Twin <T, U> &&rhs) {
      first = cr::move (rhs.first);
      second = cr::move (rhs.second);

      return *this;
   }

public:
   constexpr bool operator < (const Twin <A, B> &rhs) const {
      return first < rhs.first || (first == rhs.first && second < rhs.second);
   }

   constexpr bool operator > (const Twin <A, B> &rhs) const {
      return first > rhs.first || (first == rhs.first && second > rhs.second);
   }

   constexpr bool operator <= (const Twin<A, B> &rhs) const {
      return first < rhs.first || (first == rhs.first && second <= rhs.second);
   }

   constexpr bool operator >= (const Twin <A, B> &rhs) const {
      return first > rhs.first || (first == rhs.first && second >= rhs.second);
   }

   constexpr bool operator == (const Twin <A, B> &rhs) const {
      return first == rhs.first && second == rhs.second;
   }

   constexpr bool operator != (const Twin <A, B> &rhs) const {
      return first != rhs.first || second != rhs.second;
   }
};

CR_NAMESPACE_END
