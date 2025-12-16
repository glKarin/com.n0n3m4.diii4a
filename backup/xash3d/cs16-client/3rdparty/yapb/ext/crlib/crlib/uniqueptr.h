//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/basic.h>
#include <crlib/memory.h>
#include <crlib/movable.h>
#include <crlib/traits.h>

CR_NAMESPACE_BEGIN

// simple unique ptr
template <typename T> class UniquePtr final : public NonCopyable {
private:
   T *ptr_ {};

public:
   constexpr UniquePtr () = default;

   constexpr explicit UniquePtr (T *ptr) : ptr_ (ptr)
   { }

   constexpr UniquePtr (UniquePtr &&rhs) noexcept : ptr_ (rhs.release ())
   { }

   template <typename U> constexpr UniquePtr (UniquePtr <U> &&rhs) noexcept : ptr_ (rhs.release ())
   { }

   ~UniquePtr () {
      destroy ();
   }

public:
   constexpr T *get () const {
      return ptr_;
   }

   constexpr T *release () {
      auto ret = ptr_;
      ptr_ = nullptr;

      return ret;
   }

   constexpr void reset (T *ptr = nullptr) {
      destroy ();
      ptr_ = ptr;
   }

private:
   constexpr void destroy () {
      delete ptr_;
      ptr_ = nullptr;
   }

public:
   constexpr UniquePtr &operator = (UniquePtr &&rhs) noexcept {
      if (this != &rhs) {
         reset (rhs.release ());
      }
      return *this;
   }

   template <typename U> constexpr UniquePtr &operator = (UniquePtr <U> &&rhs) noexcept {
      if (this != &rhs) {
         reset (rhs.release ());
      }
      return *this;
   }

   constexpr UniquePtr &operator = (nullptr_t) {
      destroy ();
      return *this;
   }

   constexpr T &operator * () const {
      return *ptr_;
   }

   constexpr T *operator -> () const {
      return ptr_;
   }

   explicit constexpr operator bool () const {
      return ptr_ != nullptr;
   }
};

template <typename T> class UniquePtr <T[]> final : public NonCopyable {
private:
   T *ptr_ {};

public:
   constexpr UniquePtr () = default;

   explicit constexpr UniquePtr (T *ptr) : ptr_ (ptr)
   { }

   constexpr UniquePtr (UniquePtr &&rhs) noexcept : ptr_ (rhs.release ())
   { }

   template <typename U> constexpr UniquePtr (UniquePtr <U> &&rhs) noexcept : ptr_ (rhs.release ())
   { }

   ~UniquePtr () {
      destroy ();
   }

public:
   constexpr T *get () const {
      return ptr_;
   }

   constexpr T *release () {
      auto ret = ptr_;
      ptr_ = nullptr;

      return ret;
   }

   constexpr void reset (T *ptr = nullptr) {
      destroy ();
      ptr_ = ptr;
   }

private:
   constexpr void destroy () {
      delete[] ptr_;
      ptr_ = nullptr;
   }

public:
   constexpr UniquePtr &operator = (UniquePtr &&rhs) noexcept {
      if (this != &rhs) {
         reset (rhs.release ());
      }
      return *this;
   }

   template <typename U> constexpr UniquePtr &operator = (UniquePtr <U> &&rhs) noexcept {
      if (this != &rhs) {
         reset (rhs.release ());
      }
      return *this;
   }

   constexpr UniquePtr &operator = (nullptr_t) {
      destroy ();
      return *this;
   }

   constexpr T &operator [] (size_t index) {
      return ptr_[index];
   }

   constexpr const T &operator [] (size_t index) const {
      return ptr_[index];
   }

   explicit constexpr operator bool () const {
      return ptr_ != nullptr;
   }
};

namespace detail {
   template <typename T> struct unique_if {
      using single = UniquePtr <T>;
   };

   template <typename T> struct unique_if <T[]> {
      using unknown = UniquePtr <T[]>;
   };

   template <typename T, size_t N> struct unique_if <T[N]> {
      using known = void;
   };
}

template <typename T, typename... Args> constexpr typename detail::unique_if <T>::single makeUnique (Args &&... args) {
   return UniquePtr <T> { new T (cr::forward <Args> (args)...) };
}

template <typename T> constexpr typename detail::unique_if <T>::unknown makeUnique (const size_t size) {
   return UniquePtr <T> { new typename cr::clear_extent <T>::type[size] {}};
}

template <typename T, typename... Args> typename detail::unique_if <T>::known makeUnique (Args &&...) = delete;

CR_NAMESPACE_END
