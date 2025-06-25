//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/basic.h>

#define _USE_MATH_DEFINES
#include <math.h>

CR_NAMESPACE_BEGIN

constexpr float kFloatOnEpsilon = 0.01f;
constexpr float kFloatEqualEpsilon = 0.001f;
constexpr float kFloatEpsilon = 1.192092896e-07f;
constexpr float kMathPi = 3.141592653583f;
constexpr float kDegreeToRadians = kMathPi / 180.0f;
constexpr float kRadiansToDegree = 180.0f / kMathPi;

CR_NAMESPACE_END

#include <crlib/simd.h>
#include <crlib/traits.h>

CR_NAMESPACE_BEGIN

// abs() overload
template <typename T> constexpr T abs (const T &a) {
   if constexpr (is_same <T, float>::value) {
      return ::fabsf (a);
   }
   else if constexpr (is_same <T, int>::value) {
      return ::abs (a);
   }
}

#if defined(CR_HAS_SIMD_SSE)
template <> CR_FORCE_INLINE float abs (const float &x) {
   return _mm_cvtss_f32 (simd::fabs_ps (_mm_load_ss (&x)));
}

template <> CR_FORCE_INLINE float min (const float &a, const float &b) {
   return _mm_cvtss_f32 (_mm_min_ss (_mm_load_ss (&a), _mm_load_ss (&b)));
}

template <> CR_FORCE_INLINE float max (const float &a, const float &b) {
   return _mm_cvtss_f32 (_mm_max_ss (_mm_load_ss (&a), _mm_load_ss (&b)));
}

template <> CR_FORCE_INLINE float clamp (const float &x, const float &a, const float &b) {
   return _mm_cvtss_f32 (_mm_min_ss (_mm_max_ss (_mm_load_ss (&x), _mm_load_ss (&a)), _mm_load_ss (&b)));
}
#endif

template <typename T> constexpr T sqrf (const T &value) {
   return value * value;
}

CR_FORCE_INLINE float sinf (const float value) {
#if defined(CR_HAS_SIMD_SSE)
   return _mm_cvtss_f32 (simd::sin_ps (_mm_load_ss (&value)));
#else
   return ::sinf (value);
#endif
}

CR_FORCE_INLINE float cosf (const float value) {
#if defined(CR_HAS_SIMD_SSE)
   return _mm_cvtss_f32 (simd::cos_ps (_mm_load_ss (&value)));
#else
   return ::cosf (value);
#endif
}

CR_FORCE_INLINE float atan2f (const float y, const float x) {
#if defined(CR_HAS_SIMD_SSE)
   return _mm_cvtss_f32 (simd::atan2_ps (_mm_load_ss (&y), _mm_load_ss (&x)));
#else
   return ::atan2f (y, x);
#endif
}

CR_FORCE_INLINE float powf (const float x, const float y) {
#if defined(CR_HAS_SIMD_SSE)
   return _mm_cvtss_f32 (simd::pow_ps (_mm_load_ss (&x), _mm_load_ss (&y)));
#else
   return ::powf (x, y);
#endif
}

CR_FORCE_INLINE float sqrtf (const float value) {
#if defined(CR_HAS_SIMD_SSE)
   return _mm_cvtss_f32 (_mm_sqrt_ss (_mm_load_ss (&value)));
#else
   return ::sqrtf (value);
#endif
}

CR_FORCE_INLINE float rsqrtf (const float value) {
#if defined(CR_HAS_SIMD_SSE)
   return _mm_cvtss_f32 (_mm_rsqrt_ss (_mm_load_ss (&value)));
#else
   return 1.0f / ::sqrtf (value);
#endif
}

CR_FORCE_INLINE float tanf (const float value) {
#if defined(CR_HAS_SIMD_SSE)
   return _mm_cvtss_f32 (simd::tan_ps (_mm_load_ss (&value)));
#else
   return ::tanf (value);
#endif
}

CR_FORCE_INLINE float log10 (const float value) {
#if defined(CR_HAS_SIMD_SSE)
   return _mm_cvtss_f32 (simd::log10_ps (_mm_load_ss (&value)));
#else
   return ::log10f (value);
#endif
}

CR_FORCE_INLINE float roundf (const float value) {
#if defined(CR_HAS_SIMD_SSE)
   return _mm_cvtss_f32 (simd::round_ps (_mm_load_ss (&value)));
#else
   return ::roundf (value);
#endif
}

#if defined(CR_HAS_SIMD_SSE)
CR_SIMD_TARGET_TIL ("sse4.1")
#else
CR_FORCE_INLINE
#endif
float ceilf (const float value) {
#if defined(CR_HAS_SIMD_SSE)
   if (cpuflags.sse42) {
      return _mm_cvtss_f32 (_mm_ceil_ps (_mm_load_ss (&value)));
   }
   return _mm_cvtss_f32 (simd::ceil_ps (_mm_load_ss (&value)));
#else
   return ::ceilf (value);
#endif
}

#if defined(CR_HAS_SIMD_SSE)
CR_SIMD_TARGET_TIL ("sse4.1")
#else
CR_FORCE_INLINE
#endif
float floorf (const float value) {
#if defined(CR_HAS_SIMD_SSE)
   if (cpuflags.sse42) {
      return _mm_cvtss_f32 (_mm_floor_ps (_mm_load_ss (&value)));
   }
   return _mm_cvtss_f32 (simd::floor_ps (_mm_load_ss (&value)));
#else
   return ::floorf (value);
#endif
}

CR_FORCE_INLINE void sincosf (const float &x, float &s, float &c) {
#if defined(CR_HAS_SIMD_SSE)
   __m128 _s, _c;
   simd::sincos_ps (_mm_load_ss (&x), _s, _c);

   s = _mm_cvtss_f32 (_s);
   c = _mm_cvtss_f32 (_c);
#elif defined(CR_HAS_SIMD_NEON)
   float32x4_t _s, _c;
   simd::sincos_ps (vsetq_lane_f32 (x, vdupq_n_f32 (0), 0), _s, _c);

   s = vgetq_lane_f32 (_s, 0);
   c = vgetq_lane_f32 (_c, 0);
#else
   s = cr::sinf (x);
   c = cr::cosf (x);
#endif
}

CR_FORCE_INLINE bool fzero (const float e) {
   return cr::abs (e) < kFloatOnEpsilon;
}

CR_FORCE_INLINE bool fequal (const float a, const float b) {
   return cr::abs (a - b) < kFloatEqualEpsilon;
}

constexpr float rad2deg (const float r) {
   return r * kRadiansToDegree;
}

constexpr float deg2rad (const float d) {
   return d * kDegreeToRadians;
}

namespace detail {
#if defined(CR_HAS_SIMD_SSE)
   template <int D> CR_SIMD_TARGET ("sse4.1") float sse4_wrapAngleFn (float x) {
      __m128 v0 = _mm_load_ss (&x);
      __m128 v1 = _mm_set_ss (static_cast <float> (D));

      __m128 mul0 = _mm_mul_ss (_mm_set_ss (2.0f), v1);
      __m128 div0 = _mm_div_ss (v0, mul0);
      __m128 add0 = _mm_add_ss (div0, _mm_set_ss (0.5f));
      __m128 flr0 = _mm_floor_ss (_mm_setzero_ps (), add0);

      __m128 mul1 = _mm_mul_ss (mul0, flr0);
      __m128 sub0 = _mm_sub_ss (v0, mul1);

      return _mm_cvtss_f32 (sub0);
   }

   template <int D> CR_FORCE_INLINE float sse2_wrapAngleFn (float x) {
      __m128 v0 = _mm_load_ss (&x);
      __m128 v1 = _mm_set_ss (static_cast <float> (D));

      __m128 mul0 = _mm_mul_ss (_mm_set_ss (2.0f), v1);
      __m128 div0 = _mm_div_ss (v0, mul0);
      __m128 add0 = _mm_add_ss (div0, _mm_set_ss (0.5f));
      __m128 flr0 = simd::floor_ps (add0);

      __m128 mul1 = _mm_mul_ss (mul0, flr0);
      __m128 sub0 = _mm_sub_ss (v0, mul1);

      return _mm_cvtss_f32 (sub0);
   }
#endif

   template <int D> CR_FORCE_INLINE float scalar_wrapAngleFn (float x) {
      return x - 2.0f * static_cast <float> (D) * ::floorf (x / (2.0f * static_cast <float> (D)) + 0.5f);
   }

   template <int D> CR_FORCE_INLINE float _wrapAngleFn (float x) {
   #if defined(CR_HAS_SIMD_SSE)
      if (cpuflags.sse42) {
         return sse4_wrapAngleFn <D> (x);
      }
      return sse2_wrapAngleFn <D> (x);
   #else
      return scalar_wrapAngleFn <D> (x);
   #endif
   }
}

// adds or subtracts 360.0f enough times need to given angle in order to set it into the range[0.0, 360.0f).
CR_FORCE_INLINE float wrapAngle360 (float a) {
   return detail::_wrapAngleFn <360> (a);
}

// adds or subtracts 360.0f enough times need to given angle in order to set it into the range [-180.0, 180.0f).
CR_FORCE_INLINE float wrapAngle (const float a) {
   return detail::_wrapAngleFn <180> (a);
}

CR_FORCE_INLINE float anglesDifference (const float a, const float b) {
   return wrapAngle (a - b);
}

CR_NAMESPACE_END
