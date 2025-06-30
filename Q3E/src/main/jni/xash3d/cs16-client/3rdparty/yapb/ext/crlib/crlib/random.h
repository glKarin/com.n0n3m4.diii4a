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
   private:
      uint32_t state_ {};

   public:
      explicit SplitMix32 (const uint32_t state) noexcept : state_ (state) {}

   public:
      [[nodiscard]] decltype (auto) next () noexcept {
         auto z = (state_ += 0x9e3779b9);
         z = (z ^ (z >> 16)) * 0x85ebca6b;
         z = (z ^ (z >> 13)) * 0xc2b2ae35;
         return z ^ (z >> 16);
      }
   };
}

// random number generator (xoshiro based)
class RWrand final : public Singleton <RWrand> {
private:
   static constexpr uint64_t kRandMax { static_cast <uint64_t> (1) << 32ull };

private:
   uint32_t states_[3] {};

public:
   explicit RWrand () noexcept  {
      const auto seed = static_cast <uint32_t> (time (nullptr));

      detail::SplitMix32 smix (seed);

      for (auto &state : states_) {
         state = smix.next ();
      }
   }
   ~RWrand () = default;


private:
   [[nodiscard]] uint32_t rotl (const uint32_t x, const int32_t k) noexcept  {
      return (x << k) | (x >> (32 - k));
   }

   [[nodiscard]] uint32_t next () noexcept {
      states_[0] = rotl (states_[0] + states_[1] + states_[2], 16);
      states_[1] = states_[0];
      states_[2] = states_[1];

      return states_[0];
   }

public:
   template <typename U> [[nodiscard]] decltype (auto) operator () (const U low, const U high) noexcept {
      if constexpr (cr::is_same <U, int32_t>::value) {
         return static_cast <int32_t> (next () * (static_cast <double> (high) - static_cast <double> (low) + 1.0) / kRandMax + static_cast <double> (low));
      }
      else if constexpr (cr::is_same <U, float>::value) {
         return static_cast <float> (next () * (static_cast <double> (high) - static_cast <double> (low)) / (kRandMax - 1) + static_cast <double> (low));
      }
   }

   template <int32_t Low = 0, int32_t High = 100> [[nodiscard]] decltype (auto) chance (const int32_t limit) noexcept {
      return operator () <int32_t> (Low, High) <= limit;
   }
};

// expose global random generator
CR_EXPOSE_GLOBAL_SINGLETON (RWrand, rg);

CR_NAMESPACE_END
