//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/mathlib.h>
#include <crlib/traits.h>

CR_NAMESPACE_BEGIN

// 3dmath vector
template <typename T> class Vec3D {
public:
#if defined(CR_CXX_MSVC)
#   pragma warning(push)
#   pragma warning(disable: 4201)
#endif
   union {
      struct {
         T x, y, z;
      };
      T data[3] {};
   };
#if defined(CR_CXX_MSVC)
#   pragma warning(pop) 
#endif

public:
   constexpr Vec3D (const T &scaler = 0.0f) : x (scaler), y (scaler), z (scaler) {}

   constexpr Vec3D (const T &x, const T &y, const T &z) : x (x), y (y), z (z) {}

   constexpr Vec3D (T *rhs) : x (rhs[0]), y (rhs[1]), z (rhs[2]) {}

#if defined(CR_HAS_SIMD)
   constexpr Vec3D (const SimdVec3Wrap &rhs) : x (rhs.x), y (rhs.y), z (rhs.z) {}
#endif

   constexpr Vec3D (const Vec3D &) = default;

   constexpr Vec3D (nullptr_t) {
      clear ();
   }

public:
   constexpr operator T * () {
      return data;
   }

   constexpr operator const T * () const {
      return data;
   }

   constexpr decltype (auto) operator + (const Vec3D &rhs) const {
      return Vec3D { x + rhs.x, y + rhs.y, z + rhs.z };
   }

   constexpr decltype (auto) operator - (const Vec3D &rhs) const {
      return Vec3D { x - rhs.x, y - rhs.y, z - rhs.z };
   }

   constexpr decltype (auto) operator - () const {
      return Vec3D { -x, -y, -z };
   }

   friend constexpr decltype (auto) operator * (const T &scale, const Vec3D &rhs) {
      return Vec3D { rhs.x * scale, rhs.y * scale, rhs.z * scale };
   }

   constexpr decltype (auto) operator * (const T &scale) const {
      return Vec3D { scale * x, scale * y, scale * z };
   }

   constexpr decltype (auto) operator / (const T &rhs) const {
      const auto inv = 1 / (rhs + kFloatEqualEpsilon);
      return Vec3D { inv * x, inv * y, inv * z };
   }

   // cross product
   constexpr decltype (auto) operator ^ (const Vec3D &rhs) const {
      return Vec3D { y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x };
   }

   // dot product
   constexpr T operator | (const Vec3D &rhs) const {
      return x * rhs.x + y * rhs.y + z * rhs.z;
   }

   constexpr decltype (auto) operator += (const Vec3D &rhs) {
      x += rhs.x;
      y += rhs.y;
      z += rhs.z;

      return *this;
   }

   constexpr decltype (auto) operator -= (const Vec3D &rhs) {
      x -= rhs.x;
      y -= rhs.y;
      z -= rhs.z;

      return *this;
   }

   constexpr decltype (auto) operator *= (const T &rhs) {
      x *= rhs;
      y *= rhs;
      z *= rhs;

      return *this;
   }

   constexpr decltype (auto) operator /= (const T &rhs) {
      const auto inv = 1 / (rhs + kFloatEqualEpsilon);

      x *= inv;
      y *= inv;
      z *= inv;

      return *this;
   }

   constexpr bool operator == (const Vec3D &rhs) const {
      return cr::fequal (x, rhs.x) && cr::fequal (y, rhs.y) && cr::fequal (z, rhs.z);
   }

   constexpr bool operator != (const Vec3D &rhs) const {
      return !operator == (rhs);
   }

   constexpr void operator = (nullptr_t) {
      clear ();
   }

   constexpr const float &operator [] (const int i) const {
      return data[i];
   }

   constexpr float &operator [] (const int i) {
      return data[i];
   }

   Vec3D &operator = (const Vec3D &) = default;

public:
   T length () const {
#if defined(CR_HAS_SIMD)
      return SimdVec3Wrap { x, y, z }.hypot ();
#else
      return cr::sqrtf (lengthSq ());
#endif
   }

   T lengthSq () const {
      return cr::sqrf (x) + cr::sqrf (y) + cr::sqrf (z);
   }

   T length2d () const {
#if defined(CR_HAS_SIMD)
      return SimdVec3Wrap { x, y }.hypot ();
#else
      return cr::sqrtf (lengthSq2d ());
#endif
   }

   T lengthSq2d () const {
      return cr::sqrf (x) + cr::sqrf (y);
   }

   T distance (const Vec3D &rhs) const {
      return cr::sqrtf ((*this - rhs).lengthSq ());
   }

   T distance2d (const Vec3D &rhs) const {
      return cr::sqrtf ((*this - rhs).lengthSq2d ());
   }

   T distanceSq (const Vec3D &rhs) const {
      return (*this - rhs).lengthSq ();
   }

   T distanceSq2d (const Vec3D &rhs) const {
      return (*this - rhs).lengthSq2d ();
   }

   constexpr decltype (auto) get2d () const {
      return Vec3D { x, y, 0.0f };
   }

   Vec3D normalize () const {
#if defined(CR_HAS_SIMD)
      return SimdVec3Wrap { x, y, z }.normalize ();
#else
      auto len = length ();

      if (cr::fzero (len)) {
         return { 0.0f, 0.0f, kFloatEpsilon };
      }
      len = 1.0f / len;
      return { x * len, y * len, z * len };
#endif
   }

   Vec3D normalize2d () const {
      auto len = length2d ();

      if (cr::fzero (len)) {
         return { 0.0f, kFloatEpsilon, 0.0f };
      }
      len = 1.0f / len;
      return { x * len, y * len, 0.0f };
   }

   Vec3D normalize_apx () const {
      const auto len = cr::rsqrtf (lengthSq () + kFloatEpsilon);
      return { x * len, y * len, z * len };
   }

   Vec3D normalize2d_apx () const {
      const auto len = cr::rsqrtf (lengthSq2d () + kFloatEpsilon);
      return { x * len, y * len, 0.0f };
   }

   T normalizeInPlace () {
      const auto len = length ();

      if (cr::fzero (len)) {
         *this = { 0.0f, 0.0f, kFloatEpsilon };
      }
      else {
         const auto mul = 1.0f / len;
         *this = { x * mul, y * mul, z * mul };
      }
      return len;
   }

   constexpr bool empty () const {
      return cr::fzero (x) && cr::fzero (y) && cr::fzero (z);
   }

   constexpr void clear () {
      x = y = z = 0.0f;
   }

   decltype (auto) clampAngles () {
      x = cr::wrapAngle (x);
      y = cr::wrapAngle (y);
      z = 0.0f;

      return *this;
   }

   // converts a spatial location determined by the vector passed into an absolute X angle (pitch) from the origin of the world.
   T pitch () const {
      if (cr::fzero (z)) {
         return 0.0f;
      }
      return cr::deg2rad (cr::atan2f (z, length2d ()));
   }

   // converts a spatial location determined by the vector passed into an absolute Y angle (yaw) from the origin of the world.
   T yaw () const {
      if (cr::fzero (x) && cr::fzero (y)) {
         return 0.0f;
      }
      return cr::rad2deg (cr::atan2f (y, x));
   }

   // converts a spatial location determined by the vector passed in into constant absolute angles from the origin of the world.
   Vec3D angles () const {
      if (cr::fzero (x) && cr::fzero (y)) {
         return { z > 0.0f ? 90.0f : 270.0f, 0.0, 0.0f };
      }
      const auto len2d = length2d ();

      // else it's another sort of vector compute individually the pitch and yaw corresponding to this vector.
      return { cr::rad2deg (cr::atan2f (z, len2d)), cr::rad2deg (cr::atan2f (y, x)), 0.0f };
   }

   //	builds a 3D referential from a view angle, that is to say, the relative "forward", "right" and "upward" direction 
   // that a player would have if he were facing this view angle. World angles are stored in Vector structs too, the 
   // "x" component corresponding to the X angle (horizontal angle), and the "y" component corresponding to the Y angle 
   // (vertical angle).
   void angleVectors (Vec3D *forward, Vec3D *right, Vec3D *upward) const {
#if defined(CR_HAS_SIMD_SSE)
      SimdVec3Wrap { x, y, z }.angleVectors <Vec3D> (forward, right, upward);
#endif

#if defined(CR_HAS_SIMD_NEON)
      static SimdVec3Wrap s, c;
      SimdVec3Wrap { x, y, z }.angleVectors (s, c);
#elif !defined(CR_HAS_SIMD_SSE)
      static Vec3D s, c, r;
      r = { cr::deg2rad (x), cr::deg2rad (y), cr::deg2rad (z) };

      cr::sincosf (r.x, s.x, c.x);
      cr::sincosf (r.y, s.y, c.y);
      cr::sincosf (r.z, s.z, c.z);
#endif

#if !defined(CR_HAS_SIMD_SSE)
      if (forward) {
         *forward = { c.x * c.y, c.x * s.y, -s.x };
      }

      if (right) {
         *right = { -s.z * s.x * c.y + c.z * s.y, -s.z * s.x * s.y - c.z * c.y, -s.z * c.x };
      }

      if (upward) {
         *upward = { c.z * s.x * c.y + s.z * s.y, c.z * s.x * s.y - s.z * c.y, c.z * c.x };
      }
#endif
   }

   const Vec3D &forward () {
      static Vec3D s_fwd {};
      angleVectors (&s_fwd, nullptr, nullptr);

      return s_fwd;
   }

   const Vec3D &upward () {
      static Vec3D s_up {};
      angleVectors (nullptr, nullptr, &s_up);

      return s_up;
   }

   const Vec3D &right () {
      static Vec3D s_right {};
      angleVectors (nullptr, &s_right, nullptr);

      return s_right;
   }

public:
   static bool bboxIntersects (const Vec3D <float> &min1, const Vec3D <float> &max1, const Vec3D <float> &min2, const Vec3D <float> &max2) {
      return min1.x < max2.x && max1.x > min2.x && min1.y < max2.y && max1.y > min2.y && min1.z < max2.z && max1.z > min2.z;
   }
};

// default is float
using Vector = Vec3D <float>;

CR_NAMESPACE_END
