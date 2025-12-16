//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/memory.h>

CR_NAMESPACE_BEGIN

template <typename> class Lambda;
template <typename R, typename ...Args> class Lambda <R (Args...)> final {
private:
   static constexpr uint32_t kSmallBufferSize = sizeof (void *) * 3;

private:
   class LambdaWrapper : public NonCopyable {
   public:
      explicit LambdaWrapper () = default;
      virtual ~LambdaWrapper () = default;

   public:
      virtual R invoke (Args &&... args) const = 0;
      virtual LambdaWrapper *clone (void *obj) const = 0;
      virtual LambdaWrapper *move (void *obj) = 0;
   };

   template <typename T> class LambdaFunctor final : public LambdaWrapper {
   private:
      T callee_;

   public:
      constexpr LambdaFunctor (const LambdaFunctor &rhs) : callee_ { rhs.callee_ } {}
      constexpr LambdaFunctor (LambdaFunctor &&rhs) noexcept : callee_ { cr::move (rhs.callee_) } {}
      constexpr LambdaFunctor (const T &callee) : callee_ { callee } {}
      constexpr LambdaFunctor (T &&callee) : callee_ { cr::move (callee) } {}

      virtual ~LambdaFunctor () = default;

   public:
      virtual R invoke (Args &&... args) const override {
         return callee_ (cr::forward <Args> (args)...);
      }

      virtual LambdaWrapper *clone (void *obj) const override {
         if (!obj) {
            return Memory::getAndConstruct <LambdaFunctor> (*this);
         }
         return Memory::construct (reinterpret_cast <LambdaFunctor *> (obj), *this);
      }

      virtual LambdaWrapper *move (void *obj) override {
         return Memory::construct (reinterpret_cast <LambdaFunctor *> (obj), cr::move (*this));
      }
   };

private:
   LambdaWrapper *lambda_ {};

   union {
      double alignment_;
      uint8_t alias_[kSmallBufferSize];
   } storage_ {};

private:
   constexpr decltype (auto) small () {
      return &storage_;
   }

   constexpr bool isSmall () const {
      return &storage_ == reinterpret_cast <void *> (lambda_);
   }

public:
   constexpr void destroy () noexcept {
      if (!lambda_) {
         return;
      }

      if (isSmall ()) {
         Memory::destruct (lambda_);
      }
      else {
         Memory::release (lambda_);
      }
   }

   constexpr void apply (const Lambda &rhs) noexcept {
      if (!rhs) {
         lambda_ = nullptr;
      }
      else if (rhs.isSmall ()) {
         lambda_ = rhs.lambda_->clone (small ());
      }
      else {
         lambda_ = rhs.lambda_->clone (nullptr);
      }
   }

   constexpr void move (Lambda &&rhs) noexcept {
      if (!rhs) {
         lambda_ = nullptr;
      }
      else if (rhs.isSmall ()) {
         lambda_ = rhs.lambda_->move (small ());
         rhs.destroy ();
         rhs.lambda_ = nullptr;
      }
      else {
         lambda_ = rhs.lambda_;
         rhs.lambda_ = nullptr;
      }
   }

   template <typename U> constexpr void apply (U &&fn) {
      using Type = LambdaFunctor <typename cr::decay <U>::type>;

      if constexpr (sizeof (Type) > sizeof (storage_)) {
         lambda_ = Memory::getAndConstruct <Type> (cr::forward <U> (fn));
      }
      else {
         lambda_ = Memory::construct (reinterpret_cast <Type *> (small ()), cr::forward <U> (fn));
      }
   }

public:
   constexpr Lambda () : lambda_ { nullptr } {}
   constexpr Lambda (nullptr_t) : lambda_ { nullptr } {}

public:
   constexpr Lambda (const Lambda &rhs) {
      apply (rhs);
   }

   constexpr Lambda (Lambda &&rhs) noexcept {
      move (cr::forward <Lambda> (rhs));
   }

   template <typename U> constexpr Lambda (U &&obj) {
      apply (cr::forward <U> (obj));
   }

   ~Lambda () {
      destroy ();
   }

public:
   explicit constexpr operator bool () const {
      return !!lambda_;
   }

   constexpr decltype (auto) operator () (Args... args) const {
      assert (lambda_);
      return lambda_->invoke (cr::forward <Args> (args)...);
   }

public:
   constexpr Lambda &operator = (nullptr_t) {
      destroy ();
      lambda_ = nullptr;
      return *this;
   }

   constexpr Lambda &operator = (const Lambda &rhs) {
      destroy ();
      apply (rhs);
      return *this;
   }

   constexpr Lambda &operator = (Lambda &&rhs) noexcept {
      destroy ();
      move (cr::move (rhs));
      return *this;
   }

   template <typename U> constexpr Lambda &operator = (U &&rhs) {
      destroy ();
      apply (rhs);
      return *this;
   }
};

CR_NAMESPACE_END
