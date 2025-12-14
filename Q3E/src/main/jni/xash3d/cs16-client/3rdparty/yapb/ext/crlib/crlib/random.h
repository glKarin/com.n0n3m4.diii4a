//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/basic.h>
#include <crlib/traits.h>

CR_NAMESPACE_BEGIN

namespace detail {
   class SplitMix32 final {
      uint32_t state_ = 0;
   public:
      explicit SplitMix32 (uint32_t seed) noexcept : state_ (seed) {}
      uint32_t next () noexcept {
         uint32_t z = (state_ += 0x9e3779b9u);
         z = (z ^ (z >> 16)) * 0x85ebca6bu;
         z = (z ^ (z >> 13)) * 0xc2b2ae35u;
         return z ^ (z >> 16);
      }
   };
}

class RWrand final : public Singleton <RWrand> {
   uint32_t s0_ = 0, s1_ = 0, s2_ = 0;

    [[nodiscard]] static uint32_t rotl (uint32_t x, int k) noexcept {
        return (x << (k & 31)) | (x >> ((32 - k) & 31));
    }

   [[nodiscard]] uint32_t next32 () noexcept {
      const auto s0 = s0_, s1 = s1_, s2 = s2_;

      s0_ = rotl (s0 + s1 + s2, 16);
      s1_ = s0;
      s2_ = s1;

      return s0_;
   }

public:
   RWrand () noexcept {
      const uint32_t seed = static_cast <uint32_t> (time (nullptr));
      detail::SplitMix32 smix (seed);

      s0_ = smix.next ();
      s1_ = smix.next ();
      s2_ = smix.next ();
   }

   [[nodiscard]] int32_t get (int32_t low, int32_t high) noexcept {
      if (low == high) {
         return low;
      }
      const auto range = static_cast <uint32_t> (high - low) + 1u;
      return low + static_cast <int32_t> (next32 () % range);
   }

   [[nodiscard]] float get (float low, float high) noexcept {
      constexpr float scale = 1.0f / 4294967296.0f;

      return low + (high - low) * (next32 () * scale);
   }

   [[nodiscard]] int32_t operator () (int32_t low, int32_t high) noexcept {
      return get (low, high);
   }

   [[nodiscard]] float operator () (float low, float high) noexcept {
      return get (low, high);
   }

   [[nodiscard]] bool chance (int32_t percent) noexcept {
      return get (0, 99) < percent;
   }
};

// expose global random generator
CR_EXPOSE_GLOBAL_SINGLETON (RWrand, rg);

CR_NAMESPACE_END
