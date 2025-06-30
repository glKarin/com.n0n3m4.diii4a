/*
SSE math - Some math functions for packed SSE data

This software is derived from the Cephes Math Library. It is incorporated
herein, and licensed in accordance with the FreeBSD license, by permission
of the author.

Copyright (c) 2010 Sven Heinzel <Sven.Heinzel@gmx.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#ifndef SSE_MATH_PS_H
#define SSE_MATH_PS_H

/** @file
\brief SSE math single precision functions
*/

/** @file
\brief SSE math constants

This file contains some packed SSE math constants and macros to generate your own constants.
*/

/**
\mainpage SSE math Reference Documentation

SSE math contains some inline math functions for packed SSE data.

\section intro_sec Introduction

The goal of this headers is to provide fast math functions (like sin, log, etc.)
for packed SSE data. Speed is favored over high accuracy without too much loss
of precision. All functions perform the described operation on all packed values.

Most of the functions and constants are based on the Cephes Math Library.

\section require_sec Requirements

At least SSE2 support is needed. You also need a compiler that understands SSE2
intrinsics.

\section usage_sec Usage

Simply add the header files to your project and include the ones you need. You
must keep the subfolder ssemath because the headers itself use it as relative
path.

\code
#include "ssemath/ssemath_ps.h"
\endcode

\section license_sec License

This software is derived from the Cephes Math Library. It is incorporated
herein, and licensed in accordance with the FreeBSD license, by permission
of the author.

Copyright © 2010 Sven Heinzel

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- #Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
- #Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#if defined( __GNUC__ ) || defined(__clang__)
#  define SSE_INLINE  static inline __attribute__((always_inline))
#  define SSE_ALIGN __attribute__((aligned(16)))
#elif defined( _MSC_VER ) || defined( __ICL )
#  define SSE_INLINE  static __forceinline
#  define SSE_ALIGN __declspec (align(16))
#else
#  define SSE_INLINE  static inline
#  define SSE_ALIGN
#endif


#define DECLARE_SSEMATH_PS(Name, Val) \
    static const SSE_ALIGN constexpr float SSEMATH_PS_##Name[4] = { static_cast <float> (Val), static_cast <float> (Val), static_cast <float> (Val), static_cast <float> (Val) };

#define DECLARE_SSEMATH_PI32(Name, Val) \
    static const SSE_ALIGN constexpr int32_t SSEMATH_PI32_##Name[4] = { static_cast <int32_t> (Val), static_cast <int32_t> (Val), static_cast <int32_t> (Val), static_cast <int32_t> (Val) };


/** @file

\par Floating point constants.
SSEMATH_PS_E = \f$e\f$\n
SSEMATH_PS_LOG2E = \f$\log_2(e)\f$\n
SSEMATH_PS_LOG10E = \f$\log_{10}(e)\f$\n
SSEMATH_PS_LN2 = \f$\ln(2)\f$\n
SSEMATH_PS_LN10 = \f$\ln(10)\f$\n
SSEMATH_PS_PI = \f$\pi\f$\n
SSEMATH_PS_PI_2 = \f$\frac{\pi}{2}\f$\n
SSEMATH_PS_PI_4 = \f$\frac{\pi}{4}\f$\n
SSEMATH_PS_1_PI = \f$\frac{1}{\pi}\f$\n
SSEMATH_PS_2_PI = \f$\frac{2}{\pi}\f$\n
SSEMATH_PS_4_PI = \f$\frac{4}{\pi}\f$\n
SSEMATH_PS_2_SQRTPI = \f$\frac{2}{\sqrt{\pi}}\f$\n
SSEMATH_PS_SQRT2 = \f$\sqrt{2}\f$\n
SSEMATH_PS_SQRT1_2 = \f$\sqrt{\frac{1}{2}}\f$\n
SSEMATH_PS_CBRT2 = \f$\sqrt[3]{2}\f$\n
SSEMATH_PS_CBRT4 = \f$\sqrt[3]{4}\f$\n
SSEMATH_PS_1_CBRT2 = \f$\frac{1}{\sqrt[3]{2}}\f$\n
SSEMATH_PS_1_CBRT4 = \f$\frac{1}{\sqrt[3]{4}}\f$\n
SSEMATH_PS_EPSILON = \f$\epsilon\f$ for \c float\n

\par Bitmasks for floating point values
SSEMATH_PI32_MIN_NORM - Minimum normal positive value\n
SSEMATH_PI32_MASK_MANT - Mask for mantissa (only keeps exponent)\n
SSEMATH_PI32_INVMASK_MANT - Inverse mask for mantissa (removes exponent)\n
SSEMATH_PI32_MASK_SIGN - Mask for sign bit\n
SSEMATH_PI32_INVMASK_SIGN - Inverse mask for sign bit\n
SSEMATH_PI32_INF - Positive infinity\n
SSEMATH_PI32_MINUS_INF - Negative infinity\n
SSEMATH_PI32_MAX - Positive maximum\n
SSEMATH_PI32_MINUS_MAX - Negative maximum\n

\par Macros for own constants
DECLARE_SSEMATH_PS( Name, Val ) - Creates a constant named SSEMATH_PS_Name with four times the \c float value \p Val.\n
DECLARE_SSEMATH_PI32( Name, Val ) - Creates a constant named SSEMATH_PI32_Name with four times the \c int value \p Val.\n
*/

// Float constants
DECLARE_SSEMATH_PS (E, M_E);
DECLARE_SSEMATH_PS (LOG2E, M_LOG2E);
DECLARE_SSEMATH_PS (LOG10E, M_LOG10E);
DECLARE_SSEMATH_PS (LN2, M_LN2);
DECLARE_SSEMATH_PS (LN10, M_LN10);
DECLARE_SSEMATH_PS (PI, M_PI);
DECLARE_SSEMATH_PS (PI_2, M_PI_2);
DECLARE_SSEMATH_PS (PI_4, M_PI_4);
DECLARE_SSEMATH_PS (1_PI, M_1_PI);
DECLARE_SSEMATH_PS (2_PI, M_2_PI);
DECLARE_SSEMATH_PS (4_PI, 1.27323954473516f);
DECLARE_SSEMATH_PS (2_SQRTPI, M_2_SQRTPI);
DECLARE_SSEMATH_PS (SQRT2, M_SQRT2);
DECLARE_SSEMATH_PS (SQRT1_2, M_SQRT1_2);
DECLARE_SSEMATH_PS (CBRT2, 1.25992104989487316477f);
DECLARE_SSEMATH_PS (CBRT4, 1.58740105196819947475f);
DECLARE_SSEMATH_PS (1_CBRT2, 0.79370052598409973738f);
DECLARE_SSEMATH_PS (1_CBRT4, 0.62996052494743658238f);
DECLARE_SSEMATH_PS (EPSILON, 1.192092896e-7f);

// Integer values to mask out float bits
DECLARE_SSEMATH_PI32 (MIN_NORM, 0x00800000);
DECLARE_SSEMATH_PI32 (MASK_MANT, 0x7f800000);
DECLARE_SSEMATH_PI32 (INVMASK_MANT, ~0x7f800000);
DECLARE_SSEMATH_PI32 (MASK_SIGN, 0x80000000);
DECLARE_SSEMATH_PI32 (INVMASK_SIGN, ~0x80000000);
DECLARE_SSEMATH_PI32 (INF, 0x7f800000);
DECLARE_SSEMATH_PI32 (MINUS_INF, 0xff800000);
DECLARE_SSEMATH_PI32 (MAX, 0x7f7fffff);
DECLARE_SSEMATH_PI32 (MINUS_MAX, 0xff7fffff);
DECLARE_SSEMATH_PI32 (QNAN, 0x7fc00000);

// Float coefficients
DECLARE_SSEMATH_PS (MINUS_DP1, -0.78515625f);
DECLARE_SSEMATH_PS (MINUS_DP2, -2.4187564849853515625e-4f);
DECLARE_SSEMATH_PS (MINUS_DP3, -3.77489497744594108e-8f);

DECLARE_SSEMATH_PS (SIN_P0, -1.9515295891e-4f);
DECLARE_SSEMATH_PS (SIN_P1, 8.3321608736e-3f);
DECLARE_SSEMATH_PS (SIN_P2, -1.6666654611e-1f);
DECLARE_SSEMATH_PS (COS_P0, 2.443315711809948e-5f);
DECLARE_SSEMATH_PS (COS_P1, -1.388731625493765e-3f);
DECLARE_SSEMATH_PS (COS_P2, 4.166664568298827e-2f);

DECLARE_SSEMATH_PS (LOG_P0, 7.0376836292e-2f);
DECLARE_SSEMATH_PS (LOG_P1, -1.1514610310e-1f);
DECLARE_SSEMATH_PS (LOG_P2, 1.1676998740e-1f);
DECLARE_SSEMATH_PS (LOG_P3, -1.2420140846e-1f);
DECLARE_SSEMATH_PS (LOG_P4, 1.4249322787e-1f);
DECLARE_SSEMATH_PS (LOG_P5, -1.6668057665e-1f);
DECLARE_SSEMATH_PS (LOG_P6, 2.0000714765e-1f);
DECLARE_SSEMATH_PS (LOG_P7, -2.4999993993e-1f);
DECLARE_SSEMATH_PS (LOG_P8, 3.3333331174e-1f);
DECLARE_SSEMATH_PS (LOG_Q1, -2.12194440e-4f);
DECLARE_SSEMATH_PS (LOG_Q2, 0.693359375f);
DECLARE_SSEMATH_PS (LOG2EA, 0.44269504088896340735992f);
DECLARE_SSEMATH_PS (LOG210, 3.32192809488736234787f);
DECLARE_SSEMATH_PS (LOG102A, 3.0078125e-1f);
DECLARE_SSEMATH_PS (LOG102B, 2.48745663981195213739e-4f);
DECLARE_SSEMATH_PS (LOG10EA, 4.3359375e-1f);
DECLARE_SSEMATH_PS (LOG10EB, 7.00731903251827651129e-4f);

DECLARE_SSEMATH_PS (EXP_HI, 88.3762626647949f);
DECLARE_SSEMATH_PS (EXP_LO, -103.278929903431851103f);
DECLARE_SSEMATH_PS (EXP_C1, 0.693359375f);
DECLARE_SSEMATH_PS (EXP_C2, -2.12194440e-4f);
DECLARE_SSEMATH_PS (EXP_P0, 1.9875691500e-4f);
DECLARE_SSEMATH_PS (EXP_P1, 1.3981999507e-3f);
DECLARE_SSEMATH_PS (EXP_P2, 8.3334519073e-3f);
DECLARE_SSEMATH_PS (EXP_P3, 4.1665795894e-2f);
DECLARE_SSEMATH_PS (EXP_P4, 1.6666665459e-1f);
DECLARE_SSEMATH_PS (EXP_P5, 5.0000001201e-1f);
DECLARE_SSEMATH_PS (EXP2_P0, 1.535336188319500e-4f);
DECLARE_SSEMATH_PS (EXP2_P1, 1.339887440266574e-3f);
DECLARE_SSEMATH_PS (EXP2_P2, 9.618437357674640e-3f);
DECLARE_SSEMATH_PS (EXP2_P3, 5.550332471162809e-2f);
DECLARE_SSEMATH_PS (EXP2_P4, 2.402264791363012e-1f);
DECLARE_SSEMATH_PS (EXP2_P5, 6.931472028550421e-1f);
DECLARE_SSEMATH_PS (EXP10_P0, 2.063216740311022e-1f);
DECLARE_SSEMATH_PS (EXP10_P1, 5.420251702225484e-1f);
DECLARE_SSEMATH_PS (EXP10_P2, 1.171292686296281f);
DECLARE_SSEMATH_PS (EXP10_P3, 2.034649854009453f);
DECLARE_SSEMATH_PS (EXP10_P4, 2.650948748208892f);
DECLARE_SSEMATH_PS (EXP10_P5, 2.302585167056758f);

DECLARE_SSEMATH_PS (TAN_B0, 1.0e-4f);
DECLARE_SSEMATH_PS (TAN_B1, 1.0e-35f);
DECLARE_SSEMATH_PS (TAN_P0, 9.38540185543e-3f);
DECLARE_SSEMATH_PS (TAN_P1, 3.11992232697e-3f);
DECLARE_SSEMATH_PS (TAN_P2, 2.44301354525e-2f);
DECLARE_SSEMATH_PS (TAN_P3, 5.34112807005e-2f);
DECLARE_SSEMATH_PS (TAN_P4, 1.33387994085e-1f);
DECLARE_SSEMATH_PS (TAN_P5, 3.33331568548e-1f);

DECLARE_SSEMATH_PS (CBRT_P0, -0.13466110473359520655053f);
DECLARE_SSEMATH_PS (CBRT_P1, 0.54664601366395524503440f);
DECLARE_SSEMATH_PS (CBRT_P2, -0.95438224771509446525043f);
DECLARE_SSEMATH_PS (CBRT_P3, 1.1399983354717293273738f);
DECLARE_SSEMATH_PS (CBRT_P4, 0.40238979564544752126924f);

DECLARE_SSEMATH_PS (ASIN_P0, 4.2163199048e-2f);
DECLARE_SSEMATH_PS (ASIN_P1, 2.4181311049e-2f);
DECLARE_SSEMATH_PS (ASIN_P2, 4.5470025998e-2f);
DECLARE_SSEMATH_PS (ASIN_P3, 7.4953002686e-2f);
DECLARE_SSEMATH_PS (ASIN_P4, 1.6666752422e-1f);

DECLARE_SSEMATH_PS (ATAN_Q0, 2.414213562373095f);
DECLARE_SSEMATH_PS (ATAN_Q1, 0.414213562373095f);
DECLARE_SSEMATH_PS (ATAN_P0, 8.05374449538e-2f);
DECLARE_SSEMATH_PS (ATAN_P1, -1.38776856032e-1f);
DECLARE_SSEMATH_PS (ATAN_P2, 1.99777106478e-1f);
DECLARE_SSEMATH_PS (ATAN_P3, -3.33329491539e-1f);

/**
split mantissa and exponent

\return Returns the mantissa (and stores the exponent in \p rExp)
*/
SSE_INLINE __m128 frexp_ps (
   __m128 val,  //!< Value
   __m128i &rExp //!< Returns the exponent
) {
   // get exponent
   __m128 z = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);
   __m128i tmpi = _mm_srli_epi32 (_mm_castps_si128 (z), 23);
   rExp = _mm_sub_epi32 (tmpi, _mm_set1_epi32 (0x7e));

   // only keep mantissa
   __m128 x = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_MANT);
   return _mm_or_ps (x, _mm_set1_ps (0.5f));
}

/**
join mantissa and exponent

\return Returns \f${base}\,2^{exp}\f$
*/
SSE_INLINE __m128 ldexp_ps (
   __m128 base,    //!< Base
   __m128i exp      //!< Exponent
) {
   // y = y * 2 ^ exp
  __m128i x = _mm_add_epi32 (exp, _mm_set1_epi32 (0x7f));
   x = _mm_slli_epi32 (x, 23);
   return _mm_mul_ps (base, _mm_castsi128_ps (x));
}

/**
ceiling function

\return Returns \f$\lceil{val}\rceil\f$
*/
SSE_INLINE __m128 ceil_ps (
   __m128 val   //!< Value
) {
   // truncate value and add offset depending on sign
  __m128i trunc = _mm_cvttps_epi32 (val);
   __m128 mask = _mm_castsi128_ps (_mm_cmpeq_epi32 (trunc, _mm_set1_epi32 (0x80000000u)));
   mask = _mm_or_ps (mask, _mm_cmpeq_ps (_mm_cvtepi32_ps (trunc), val));
   __m128i sign_off = _mm_xor_si128 (_mm_set1_epi32 (1), _mm_srli_epi32 (_mm_castps_si128 (val), 31));
   trunc = _mm_add_epi32 (trunc, sign_off);

   // convert back to float and mask out invalid values
   __m128 x = _mm_cvtepi32_ps (trunc);
   x = _mm_andnot_ps (mask, x);
   return _mm_add_ps (x, _mm_and_ps (mask, val));
}

/**
floor function

\return Returns \f$\lfloor{val}\rfloor\f$
*/
SSE_INLINE __m128 floor_ps (
   __m128 val   //!< Value
) {
   // truncate value and add offset depending on sign
  __m128i trunc = _mm_cvttps_epi32 (val);
   __m128 mask = _mm_castsi128_ps (_mm_cmpeq_epi32 (trunc, _mm_set1_epi32 (0x80000000u)));
   mask = _mm_or_ps (mask, _mm_cmpeq_ps (_mm_cvtepi32_ps (trunc), val));
   __m128i sign_off = _mm_srli_epi32 (_mm_castps_si128 (val), 31);
   trunc = _mm_sub_epi32 (trunc, sign_off);

   // convert back to float and mask out invalid values
   __m128 x = _mm_cvtepi32_ps (trunc);
   x = _mm_andnot_ps (mask, x);
   return _mm_add_ps (x, _mm_and_ps (mask, val));
}

/**
truncation

\return Returns \f$int({val})\f$
*/
SSE_INLINE __m128 trunc_ps (
   __m128 val   //!< Value
) {
   // truncate value
  __m128i trunc = _mm_cvttps_epi32 (val);
   __m128 mask = _mm_castsi128_ps (_mm_cmpeq_epi32 (trunc, _mm_set1_epi32 (0x80000000u)));

   // convert back to float and mask out invalid values
   __m128 x = _mm_cvtepi32_ps (trunc);
   x = _mm_andnot_ps (mask, x);
   return _mm_add_ps (x, _mm_and_ps (mask, val));
}

/**
absolute value

\return Returns \f$\left|{val}\right|\f$
*/
SSE_INLINE __m128 fabs_ps (
   __m128 val   //!< Value
) {
   return _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);
}

/**
copy sign

\return Returns \p x with the sign of \p y
*/
SSE_INLINE __m128 copysign_ps (
   __m128 x,    //!< Value
   __m128 y     //!< Sign source
) {
   __m128 x_abs = _mm_and_ps (x, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);
   __m128 y_sign = _mm_and_ps (y, *(__m128 *)SSEMATH_PI32_MASK_SIGN);
   return _mm_or_ps (x_abs, y_sign);
}

/**
floating-point remainder

\return Returns \f${num}-{den}\,int(\frac{num}{den})\f$. If \f${den}\,=\,0\f$ NaN is returned.
*/
SSE_INLINE __m128 fmod_ps (
   __m128 num,  //!< Numerator
   __m128 den   //!< Denominator
) {
   // save invalid mask
   __m128 invalid_mask = _mm_cmpeq_ps (den, _mm_setzero_ps ());

   // abs( x ) and abs( y )
   __m128 x = _mm_and_ps (num, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);
   __m128 y = _mm_and_ps (den, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);

   // mask for x >= y (ignore y == 0)
   __m128 mask_xgey = _mm_andnot_ps (invalid_mask, _mm_cmple_ps (y, x));

   // x mod y == ( x - n * y ) mod y
  __m128i x_exp;
   __m128 x_base = frexp_ps (x, x_exp);
  __m128i y_exp;
   __m128 y_base = frexp_ps (y, y_exp);

   // build y * 2 ^ n
   __m128 offs_mask = _mm_cmplt_ps (x_base, y_base);
   __m128i offs_exp = _mm_and_si128 (_mm_set1_epi32 (1), _mm_castps_si128 (offs_mask));
   x_exp = _mm_sub_epi32 (x_exp, offs_exp);

   // only change x where x >= y
   __m128 n = _mm_and_ps (mask_xgey, ldexp_ps (y_base, x_exp));
   x = _mm_sub_ps (x, n);

   // restore sign
   __m128 sign_bit = _mm_and_ps (num, *(__m128 *)SSEMATH_PI32_MASK_SIGN);
   x = _mm_xor_ps (x, sign_bit);

   // fmod: z = x - den * int( x / den )
   __m128 z = trunc_ps (_mm_div_ps (x, den));
   z = _mm_mul_ps (z, den);
   z = _mm_sub_ps (x, z);

   // set invalid values to Nan
   return _mm_or_ps (invalid_mask, z);
}

/**
integral and fractional part

\return Returns \f$\{{val}\}\f$.
*/
SSE_INLINE __m128 modf_ps (
   __m128 val,      //!< Value
   __m128 &rInt     //!< Returns \f$int({val})\f$
) {
   rInt = trunc_ps (val);
   return _mm_sub_ps (val, rInt);
}

/**
rounding

\return Returns \p val rounded to nearest integral value
*/
SSE_INLINE __m128 round_ps (
   __m128 val   //!< Value
) {
   __m128 i = _mm_setzero_ps ();
   __m128 frac = modf_ps (val, i);

   // add rounding to fractional part to avoid precision loss
   frac = _mm_add_ps (frac, _mm_set1_ps (0.5f));

   // floor( frac )
  __m128i trunc = _mm_cvttps_epi32 (frac);
   __m128i sign_off = _mm_srli_epi32 (_mm_castps_si128 (frac), 31);
   trunc = _mm_sub_epi32 (trunc, sign_off);
   frac = _mm_cvtepi32_ps (trunc);
   return _mm_add_ps (frac, i);
}

/**
positive difference

\return Returns the positive difference between \p x and \p y
*/
SSE_INLINE __m128 fdim_ps (
   __m128 x,    //!< X value
   __m128 y     //!< Y value
) {
   return _mm_max_ps (_mm_sub_ps (x, y), _mm_setzero_ps ());
}

/**
euclidean distance function

\return Returns \f$\sqrt{x^2+y^2}\f$
*/
SSE_INLINE __m128 hypot_ps (
   __m128 x,    //!< X value
   __m128 y     //!< Y value
) {
   __m128 xx = _mm_mul_ps (x, x);
   __m128 yy = _mm_mul_ps (y, y);
   return _mm_sqrt_ps (_mm_add_ps (xx, yy));
}

/**
sine

The angle \p ang should be in the range from -8192.0 to 8192.0 for maximum
precision.
\return Returns \f$\sin({ang})\f$
*/
SSE_INLINE __m128 sin_ps (
   __m128 ang   //!< Angle in radian
) {
   // abs( x )
   __m128 x = _mm_and_ps (ang, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);

   // tmp = x / ( PI / 4 )
   __m128 tmp = _mm_mul_ps (x, *(__m128 *)SSEMATH_PS_4_PI);

   // from cephes: j = ( j + 1 ) & ( ~1 )
  __m128i tmpi = _mm_cvttps_epi32 (tmp);
   tmpi = _mm_add_epi32 (tmpi, _mm_set1_epi32 (1));
   tmpi = _mm_and_si128 (tmpi, _mm_set1_epi32 (~1));
   tmp = _mm_cvtepi32_ps (tmpi);

   // x = ( ( x - y * DP1 ) - y * DP2 ) - y * DP3;
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP1));
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP2));
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP3));

   // set sign bit
   __m128 sign_bit = _mm_and_ps (ang, *(__m128 *)SSEMATH_PI32_MASK_SIGN);
   __m128 swap_sign_bit = _mm_castsi128_ps (_mm_slli_epi32 (_mm_and_si128 (tmpi, _mm_set1_epi32 (4)), 29));
   sign_bit = _mm_xor_ps (sign_bit, swap_sign_bit);

   // xx = x ^ 2
   __m128 xx = _mm_mul_ps (x, x);

   // cosine polynom
   __m128 y = *(__m128 *)SSEMATH_PS_COS_P0;
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_COS_P1);
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_COS_P2);
   y = _mm_mul_ps (y, xx);
   y = _mm_mul_ps (y, xx);
   y = _mm_sub_ps (y, _mm_mul_ps (xx, _mm_set1_ps (0.5f)));
   y = _mm_add_ps (y, _mm_set1_ps (1.0f));

   // sine polynom
   __m128 y2 = *(__m128 *)SSEMATH_PS_SIN_P0;
   y2 = _mm_mul_ps (y2, xx);
   y2 = _mm_add_ps (y2, *(__m128 *)SSEMATH_PS_SIN_P1);
   y2 = _mm_mul_ps (y2, xx);
   y2 = _mm_add_ps (y2, *(__m128 *)SSEMATH_PS_SIN_P2);
   y2 = _mm_mul_ps (y2, xx);
   y2 = _mm_mul_ps (y2, x);
   y2 = _mm_add_ps (y2, x);

   // use mask to select polynom
   tmpi = _mm_and_si128 (tmpi, _mm_set1_epi32 (2));
   tmpi = _mm_cmpeq_epi32 (tmpi, _mm_setzero_si128 ());
   __m128 poly_mask = _mm_castsi128_ps (tmpi);
   y2 = _mm_and_ps (poly_mask, y2);
   y = _mm_andnot_ps (poly_mask, y);
   y = _mm_add_ps (y, y2);

   // toggle sign
   return _mm_xor_ps (y, sign_bit);
}

/**
cosine

The angle \p ang should be in the range from -8192.0 to 8192.0 for maximum
precision.
\return Returns \f$\cos({ang})\f$
*/
SSE_INLINE __m128 cos_ps (
   __m128 ang   //!< Angle in radian
) {
   // abs( x )
   __m128 x = _mm_and_ps (ang, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);

   // tmp = x / ( PI / 4 )
   __m128 tmp = _mm_mul_ps (x, *(__m128 *)SSEMATH_PS_4_PI);

   // from cephes: j = ( j + 1 ) & ( ~1 )
  __m128i tmpi = _mm_cvttps_epi32 (tmp);
   tmpi = _mm_add_epi32 (tmpi, _mm_set1_epi32 (1));
   tmpi = _mm_and_si128 (tmpi, _mm_set1_epi32 (~1));
   tmp = _mm_cvtepi32_ps (tmpi);

   // x = ( ( x - y * DP1 ) - y * DP2 ) - y * DP3;
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP1));
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP2));
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP3));

   // set sign bit
   tmpi = _mm_sub_epi32 (tmpi, _mm_set1_epi32 (2));
   __m128 sign_bit = _mm_castsi128_ps (_mm_slli_epi32 (_mm_andnot_si128 (tmpi, _mm_set1_epi32 (4)), 29));

   // xx = x ^ 2
   __m128 xx = _mm_mul_ps (x, x);

   // cosine polynom
   __m128 y = *(__m128 *)SSEMATH_PS_COS_P0;
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_COS_P1);
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_COS_P2);
   y = _mm_mul_ps (y, xx);
   y = _mm_mul_ps (y, xx);
   y = _mm_sub_ps (y, _mm_mul_ps (xx, _mm_set1_ps (0.5f)));
   y = _mm_add_ps (y, _mm_set1_ps (1.0f));

   // sine polynom
   __m128 y2 = *(__m128 *)SSEMATH_PS_SIN_P0;
   y2 = _mm_mul_ps (y2, xx);
   y2 = _mm_add_ps (y2, *(__m128 *)SSEMATH_PS_SIN_P1);
   y2 = _mm_mul_ps (y2, xx);
   y2 = _mm_add_ps (y2, *(__m128 *)SSEMATH_PS_SIN_P2);
   y2 = _mm_mul_ps (y2, xx);
   y2 = _mm_mul_ps (y2, x);
   y2 = _mm_add_ps (y2, x);

   // use mask to select polynom
   tmpi = _mm_and_si128 (tmpi, _mm_set1_epi32 (2));
   tmpi = _mm_cmpeq_epi32 (tmpi, _mm_setzero_si128 ());
   __m128 poly_mask = _mm_castsi128_ps (tmpi);
   y2 = _mm_and_ps (poly_mask, y2);
   y = _mm_andnot_ps (poly_mask, y);
   y = _mm_add_ps (y, y2);

   // toggle sign
   return _mm_xor_ps (y, sign_bit);
}

/**
sine and cosine

The angle \p ang should be in the range from -8192.0 to 8192.0 for maximum
precision.
\return Returns \f$\sin({ang})\f$ and \f$\cos({ang})\f$
*/
SSE_INLINE void sincos_ps (
   __m128 ang,      //!< Angle in radian
   __m128 &rSin,    //!< Returns \f$\sin({ang})\f$
   __m128 &rCos     //!< Returns \f$\cos({ang})\f$
) {
   // abs( x )
   __m128 x = _mm_and_ps (ang, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);

   // tmp = x / ( PI / 4 )
   __m128 tmp = _mm_mul_ps (x, *(__m128 *)SSEMATH_PS_4_PI);

   // from cephes: j = ( j + 1 ) & ( ~1 )
  __m128i tmpi1 = _mm_cvttps_epi32 (tmp);
   tmpi1 = _mm_add_epi32 (tmpi1, _mm_set1_epi32 (1));
   tmpi1 = _mm_and_si128 (tmpi1, _mm_set1_epi32 (~1));
   tmp = _mm_cvtepi32_ps (tmpi1);

   // x = ( ( x - y * DP1 ) - y * DP2 ) - y * DP3;
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP1));
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP2));
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP3));

   // set cosine sign bit
   __m128i tmpi2 = _mm_sub_epi32 (tmpi1, _mm_set1_epi32 (2));
   __m128 sign_bit_cos = _mm_castsi128_ps (_mm_slli_epi32 (_mm_andnot_si128 (tmpi2, _mm_set1_epi32 (4)), 29));

   // set sine sign bit
   __m128 sign_bit_sin = _mm_and_ps (ang, *(__m128 *)SSEMATH_PI32_MASK_SIGN);
   __m128 swap_sign_bit_sin = _mm_castsi128_ps (_mm_slli_epi32 (_mm_and_si128 (tmpi1, _mm_set1_epi32 (4)), 29));
   sign_bit_sin = _mm_xor_ps (sign_bit_sin, swap_sign_bit_sin);

   // xx = x ^ 2
   __m128 xx = _mm_mul_ps (x, x);

   // cosine polynom
   __m128 y = *(__m128 *)SSEMATH_PS_COS_P0;
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_COS_P1);
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_COS_P2);
   y = _mm_mul_ps (y, xx);
   y = _mm_mul_ps (y, xx);
   y = _mm_sub_ps (y, _mm_mul_ps (xx, _mm_set1_ps (0.5f)));
   y = _mm_add_ps (y, _mm_set1_ps (1.0f));

   // sine polynom
   __m128 y2 = *(__m128 *)SSEMATH_PS_SIN_P0;
   y2 = _mm_mul_ps (y2, xx);
   y2 = _mm_add_ps (y2, *(__m128 *)SSEMATH_PS_SIN_P1);
   y2 = _mm_mul_ps (y2, xx);
   y2 = _mm_add_ps (y2, *(__m128 *)SSEMATH_PS_SIN_P2);
   y2 = _mm_mul_ps (y2, xx);
   y2 = _mm_mul_ps (y2, x);
   y2 = _mm_add_ps (y2, x);

   // use masks to select polynom
   tmpi1 = _mm_and_si128 (tmpi1, _mm_set1_epi32 (2));
   tmpi1 = _mm_cmpeq_epi32 (tmpi1, _mm_setzero_si128 ());
   __m128 poly_mask = _mm_castsi128_ps (tmpi1);
   __m128 ysin2 = _mm_and_ps (poly_mask, y2);
   __m128 ysin1 = _mm_andnot_ps (poly_mask, y);
   y2 = _mm_sub_ps (y2, ysin2);
   y = _mm_sub_ps (y, ysin1);

   // toggle sign
   rSin = _mm_xor_ps (_mm_add_ps (ysin1, ysin2), sign_bit_sin);
   rCos = _mm_xor_ps (_mm_add_ps (y, y2), sign_bit_cos);
}

/**
tangens

The angle \p ang should be in the range from -8192.0 to 8192.0 for maximum
precision.
\return Returns \f$\tan({ang})\f$
*/
SSE_INLINE __m128 tan_ps (
   __m128 ang   //!< Angle in radian
) {
   // abs( x )
   __m128 x = _mm_and_ps (ang, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);

   // tmp = x / ( PI / 4 )
   __m128 tmp = _mm_mul_ps (x, *(__m128 *)SSEMATH_PS_4_PI);

   // from cephes: j = ( j + 1 ) & ( ~1 )
  __m128i tmpi = _mm_cvttps_epi32 (tmp);
   tmpi = _mm_add_epi32 (tmpi, _mm_set1_epi32 (1));
   tmpi = _mm_and_si128 (tmpi, _mm_set1_epi32 (~1));
   tmp = _mm_cvtepi32_ps (tmpi);

   // mask for selecting correct value
   __m128 mask = _mm_cmple_ps (*(__m128 *)SSEMATH_PS_TAN_B0, x);

   // x = ( ( x - y * DP1 ) - y * DP2 ) - y * DP3;
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP1));
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP2));
   x = _mm_add_ps (x, _mm_mul_ps (tmp, *(__m128 *)SSEMATH_PS_MINUS_DP3));

   // xx = x ^ 2
   __m128 xx = _mm_mul_ps (x, x);

   // tangens polynom
   __m128 y = *(__m128 *)SSEMATH_PS_TAN_P0;
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_TAN_P1);
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_TAN_P2);
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_TAN_P3);
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_TAN_P4);
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_TAN_P5);
   y = _mm_mul_ps (y, xx);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, x);

   // select values
   y = _mm_and_ps (y, mask);
   mask = _mm_andnot_ps (mask, x);
   y = _mm_add_ps (y, mask);

   // if ( tmpi & 2 ) y = -1.0 / y;
   tmpi = _mm_and_si128 (tmpi, _mm_set1_epi32 (2));
   tmpi = _mm_cmpeq_epi32 (tmpi, _mm_setzero_si128 ());
   __m128 mask2 = _mm_castsi128_ps (tmpi);

   // z = -1 / y
   __m128 z = _mm_div_ps (_mm_set1_ps (-1.0f), y);
   y = _mm_and_ps (mask2, y);
   z = _mm_andnot_ps (mask2, z);
   y = _mm_add_ps (y, z);

   // toggle sign
   __m128 sign_bit = _mm_and_ps (ang, *(__m128 *)SSEMATH_PI32_MASK_SIGN);
   return _mm_xor_ps (y, sign_bit);
}

/**
natural logarithm

\return Returns \f$\ln({val})\f$
*/
SSE_INLINE __m128 log_ps (
   __m128 val   //!< Value
) {
   // get exponent
  __m128i tmpi = _mm_srli_epi32 (_mm_castps_si128 (val), 23);
   tmpi = _mm_sub_epi32 (tmpi, _mm_set1_epi32 (0x7f));
   __m128 exp = _mm_cvtepi32_ps (tmpi);
   exp = _mm_add_ps (exp, _mm_set1_ps (1.0f));

   // only keep mantissa
   __m128 x = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_MANT);
   x = _mm_or_ps (x, _mm_set1_ps (0.5f));

   /*
   if ( x < SQRT1_2 )
   {
       exp -= 1;
       x = x + x - 1.0;
   }
   else
   {
       x = x - 1.0;
   }
   */
   __m128 mask = _mm_cmplt_ps (x, *(__m128 *)SSEMATH_PS_SQRT1_2);
   __m128 tmp = _mm_and_ps (x, mask);
   x = _mm_sub_ps (x, _mm_set1_ps (1.0f));
   exp = _mm_sub_ps (exp, _mm_and_ps (_mm_set1_ps (1.0f), mask));
   x = _mm_add_ps (x, tmp);
   __m128 invalid_mask = _mm_cmple_ps (val, _mm_setzero_ps ());

   // xx = x ^ 2
   __m128 xx = _mm_mul_ps (x, x);

   // logarithm polynom
   __m128 y = *(__m128 *)SSEMATH_PS_LOG_P0;
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P1);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P2);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P3);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P4);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P5);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P6);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P7);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P8);
   y = _mm_mul_ps (y, x);
   y = _mm_mul_ps (y, xx);

   // put everything together
   y = _mm_add_ps (y, _mm_mul_ps (exp, *(__m128 *)SSEMATH_PS_LOG_Q1));
   y = _mm_sub_ps (y, _mm_mul_ps (xx, _mm_set1_ps (0.5f)));
   x = _mm_add_ps (x, _mm_mul_ps (exp, *(__m128 *)SSEMATH_PS_LOG_Q2));
   x = _mm_add_ps (x, y);

   // negative values become NaN
   return _mm_or_ps (x, invalid_mask);
}

/**
base-2 logarithm

\return Returns \f$\log_2({val})\f$
*/
SSE_INLINE __m128 log2_ps (
   __m128 val   //!< Value
) {
   // get exponent
  __m128i tmpi = _mm_srli_epi32 (_mm_castps_si128 (val), 23);
   tmpi = _mm_sub_epi32 (tmpi, _mm_set1_epi32 (0x7f));
   __m128 exp = _mm_cvtepi32_ps (tmpi);
   exp = _mm_add_ps (exp, _mm_set1_ps (1.0f));

   // only keep mantissa
   __m128 x = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_MANT);
   x = _mm_or_ps (x, _mm_set1_ps (0.5f));

   /*
   if ( x < SQRT1_2 )
   {
       exp -= 1;
       x = x + x - 1.0;
   }
   else
   {
       x = x - 1.0;
   }
   */
   __m128 mask = _mm_cmplt_ps (x, *(__m128 *)SSEMATH_PS_SQRT1_2);
   __m128 tmp = _mm_and_ps (x, mask);
   x = _mm_sub_ps (x, _mm_set1_ps (1.0f));
   exp = _mm_sub_ps (exp, _mm_and_ps (_mm_set1_ps (1.0f), mask));
   x = _mm_add_ps (x, tmp);
   __m128 invalid_mask = _mm_cmple_ps (val, _mm_setzero_ps ());

   // xx = x ^ 2
   __m128 xx = _mm_mul_ps (x, x);

   // logarithm polynom
   __m128 y = *(__m128 *)SSEMATH_PS_LOG_P0;
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P1);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P2);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P3);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P4);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P5);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P6);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P7);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P8);
   y = _mm_mul_ps (y, x);
   y = _mm_mul_ps (y, xx);
   y = _mm_sub_ps (y, _mm_mul_ps (xx, _mm_set1_ps (0.5f)));

   // Multiply log of fraction by log2(e) and base 2 exponent by 1
   __m128 z = _mm_mul_ps (x, *(__m128 *)SSEMATH_PS_LOG2EA);
   z = _mm_add_ps (z, y);
   z = _mm_add_ps (z, _mm_mul_ps (y, *(__m128 *)SSEMATH_PS_LOG2EA));
   z = _mm_add_ps (z, x);
   z = _mm_add_ps (z, exp);

   // negative values become NaN
   return _mm_or_ps (z, invalid_mask);
}

/**
base-10 logarithm

\return Returns \f$\log_{10}({val})\f$
*/
SSE_INLINE __m128 log10_ps (
   __m128 val   //!< Value
) {
   // get exponent
  __m128i tmpi = _mm_srli_epi32 (_mm_castps_si128 (val), 23);
   tmpi = _mm_sub_epi32 (tmpi, _mm_set1_epi32 (0x7f));
   __m128 exp = _mm_cvtepi32_ps (tmpi);
   exp = _mm_add_ps (exp, _mm_set1_ps (1.0f));

   // only keep mantissa
   __m128 x = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_MANT);
   x = _mm_or_ps (x, _mm_set1_ps (0.5f));

   /*
   if ( x < SQRT1_2 )
   {
       exp -= 1;
       x = x + x - 1.0;
   }
   else
   {
       x = x - 1.0;
   }
   */
   __m128 mask = _mm_cmplt_ps (x, *(__m128 *)SSEMATH_PS_SQRT1_2);
   __m128 tmp = _mm_and_ps (x, mask);
   x = _mm_sub_ps (x, _mm_set1_ps (1.0f));
   exp = _mm_sub_ps (exp, _mm_and_ps (_mm_set1_ps (1.0f), mask));
   x = _mm_add_ps (x, tmp);
   __m128 invalid_mask = _mm_cmple_ps (val, _mm_setzero_ps ());

   // xx = x ^ 2
   __m128 xx = _mm_mul_ps (x, x);

   // logarithm polynom
   __m128 y = *(__m128 *)SSEMATH_PS_LOG_P0;
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P1);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P2);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P3);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P4);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P5);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P6);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P7);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P8);
   y = _mm_mul_ps (y, x);
   y = _mm_mul_ps (y, xx);
   y = _mm_sub_ps (y, _mm_mul_ps (xx, _mm_set1_ps (0.5f)));

   // multiply log of fraction by log10(e) and base 2 exponent by log10(2)
   __m128 eb = _mm_mul_ps (exp, *(__m128 *)SSEMATH_PS_LOG102B);
   __m128 ea = _mm_mul_ps (exp, *(__m128 *)SSEMATH_PS_LOG102A);
   __m128 z = _mm_mul_ps (_mm_add_ps (x, y), *(__m128 *)SSEMATH_PS_LOG10EB);
   z = _mm_add_ps (z, _mm_mul_ps (y, *(__m128 *)SSEMATH_PS_LOG10EA));
   z = _mm_add_ps (z, eb);
   z = _mm_add_ps (z, _mm_mul_ps (x, *(__m128 *)SSEMATH_PS_LOG10EA));
   z = _mm_add_ps (z, ea);

   // negative values become NaN
   return _mm_or_ps (z, invalid_mask);
}

/**
exponential function

\return Returns \f$e^{val}\f$
*/
SSE_INLINE __m128 exp_ps (
   __m128 val   //!< Value
) {
   // express exp( x ) = exp( y + x * log2( e ) )
   __m128 fx = _mm_mul_ps (val, *(__m128 *)SSEMATH_PS_LOG2E);
   fx = _mm_add_ps (fx, _mm_set1_ps (0.5f));

   // tmp = floor( fx )
  __m128i tmpi = _mm_cvttps_epi32 (fx);
   __m128 tmp = _mm_cvtepi32_ps (tmpi);

   // if tmp is greater subtract 1
   __m128 mask = _mm_cmple_ps (fx, tmp);
   mask = _mm_and_ps (mask, _mm_set1_ps (1.0f));
   fx = _mm_sub_ps (tmp, mask);

   __m128 x = _mm_sub_ps (val, _mm_mul_ps (fx, *(__m128 *)SSEMATH_PS_EXP_C1));
   x = _mm_sub_ps (x, _mm_mul_ps (fx, *(__m128 *)SSEMATH_PS_EXP_C2));

   // xx = x ^ 2
   __m128 xx = _mm_mul_ps (x, x);

   // exponential polynom
   __m128 y = *(__m128 *)SSEMATH_PS_EXP_P0;
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP_P1);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP_P2);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP_P3);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP_P4);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP_P5);
   y = _mm_mul_ps (y, xx);
   y = _mm_add_ps (y, x);
   y = _mm_add_ps (y, _mm_set1_ps (1.0f));

   // y = y * 2 ^ fx
   tmpi = _mm_cvttps_epi32 (fx);
   tmpi = _mm_add_epi32 (tmpi, _mm_set1_epi32 (0x7f));
   tmpi = _mm_slli_epi32 (tmpi, 23);
   y = _mm_mul_ps (y, _mm_castsi128_ps (tmpi));
   return _mm_min_ps (y, *(__m128 *)SSEMATH_PI32_MAX);
}

/**
base-2 exponential function

\return Returns \f$2^{val}\f$
*/
SSE_INLINE __m128 exp2_ps (
   __m128 val   //!< Value
) {
   // separate into integer and fractional parts
  __m128i trunc = _mm_cvttps_epi32 (val);
   __m128i sign_off = _mm_srli_epi32 (_mm_castps_si128 (val), 31);
   trunc = _mm_sub_epi32 (trunc, sign_off);
   __m128 px = _mm_cvtepi32_ps (trunc);
   __m128 x = _mm_sub_ps (val, px);

   __m128 mask = _mm_cmplt_ps (_mm_set1_ps (0.5f), x);
   __m128 tmp = _mm_and_ps (mask, _mm_set1_ps (1.0f));
   x = _mm_sub_ps (x, tmp);
   __m128 mask_0 = _mm_cmpeq_ps (val, _mm_setzero_ps ());

   // exponential polynom
   __m128 y = *(__m128 *)SSEMATH_PS_EXP2_P0;
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP2_P1);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP2_P2);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP2_P3);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP2_P4);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP2_P5);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, _mm_set1_ps (1.0f));

   // y = y * 2 ^ fx
   __m128 one = _mm_and_ps (mask_0, _mm_set1_ps (1.0f));
   px = _mm_add_ps (px, tmp);
  __m128i tmpi = _mm_cvttps_epi32 (px);
   tmpi = _mm_add_epi32 (tmpi, _mm_set1_epi32 (0x7f));
   tmpi = _mm_slli_epi32 (tmpi, 23);
   y = _mm_mul_ps (y, _mm_castsi128_ps (tmpi));
   y = _mm_min_ps (y, *(__m128 *)SSEMATH_PI32_MAX);

   // if ( val == 0 ) y = 1; This is necessary because range reduction blows up
   y = _mm_andnot_ps (mask_0, y);
   return _mm_add_ps (y, one);
}

/**
base-10 exponential function

\return Returns \f$10^{val}\f$
*/
SSE_INLINE __m128 exp10_ps (
   __m128 val   //!< Value
) {
   // express 10 ^ x = 10 ^ ( g + n log10( 2 ) )
   __m128 px = _mm_mul_ps (val, *(__m128 *)SSEMATH_PS_LOG210);
   __m128 qx = _mm_add_ps (px, _mm_set1_ps (0.5f));
  __m128i trunc = _mm_cvttps_epi32 (qx);
   __m128i sign_off = _mm_srli_epi32 (_mm_castps_si128 (qx), 31);
   trunc = _mm_sub_epi32 (trunc, sign_off);
   qx = _mm_cvtepi32_ps (trunc);
   __m128 x = _mm_sub_ps (val, _mm_mul_ps (qx, *(__m128 *)SSEMATH_PS_LOG102A));
   x = _mm_sub_ps (x, _mm_mul_ps (qx, *(__m128 *)SSEMATH_PS_LOG102B));
   __m128 mask_0 = _mm_cmpeq_ps (val, _mm_setzero_ps ());

   // exponential polynom
   __m128 y = *(__m128 *)SSEMATH_PS_EXP10_P0;
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP10_P1);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP10_P2);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP10_P3);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP10_P4);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP10_P5);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, _mm_set1_ps (1.0f));

   // y = y * 2 ^ fx
   __m128 one = _mm_and_ps (mask_0, _mm_set1_ps (1.0f));
  __m128i tmpi = _mm_cvttps_epi32 (qx);
   tmpi = _mm_add_epi32 (tmpi, _mm_set1_epi32 (0x7f));
   tmpi = _mm_slli_epi32 (tmpi, 23);
   y = _mm_mul_ps (y, _mm_castsi128_ps (tmpi));
   y = _mm_min_ps (y, *(__m128 *)SSEMATH_PI32_MAX);

   // if ( val == 0 ) y = 1; This is necessary because range reduction blows up
   y = _mm_andnot_ps (mask_0, y);
   return _mm_add_ps (y, one);
}

/**
exponentiation

\note This implementation only works for \f${base}\,>=\,0\f$. It is also not very accurate, but quite fast.
\return Returns \f${base}^{exp}\f$. Special cases:\n
  \f${base}^0\,=\,1\f$\n
  \f$0^0\,=\,1\f$\n
  \f$0^{exp}\,=\,\inf\,({exp}\,<\,0)\f$
*/
SSE_INLINE __m128 pow_ps (
   __m128 base, //!< Base
   __m128 exp   //!< Exponent
) {
#if 0
   __m128 k = _mm_mul_ps (exp, _mm_set1_ps (1.4426950408889634f));
   __m128 y = exp2_ps (_mm_mul_ps (k, log_ps (base)));
#else
   /*
   y = exp2( exp * k * log( base ) )

   log( base ):
   */

   // get exponent
  __m128i tmpi = _mm_srli_epi32 (_mm_castps_si128 (base), 23);
   tmpi = _mm_sub_epi32 (tmpi, _mm_set1_epi32 (0x7f));
   __m128 xp = _mm_cvtepi32_ps (tmpi);
   xp = _mm_add_ps (xp, _mm_set1_ps (1.0f));

   // only keep mantissa
   __m128 x = _mm_and_ps (base, *(__m128 *)SSEMATH_PI32_INVMASK_MANT);
   x = _mm_or_ps (x, _mm_set1_ps (0.5f));

   /*
   if ( x < SQRT1_2 )
   {
       xp -= 1;
       x = x + x - 1.0;
   }
   else
   {
       x = x - 1.0;
   }
   */
   __m128 mask = _mm_cmplt_ps (x, *(__m128 *)SSEMATH_PS_SQRT1_2);
   __m128 tmp = _mm_and_ps (x, mask);
   x = _mm_sub_ps (x, _mm_set1_ps (1.0f));
   xp = _mm_sub_ps (xp, _mm_and_ps (_mm_set1_ps (1.0f), mask));
   x = _mm_add_ps (x, tmp);

   // xx = x ^ 2
   __m128 xx = _mm_mul_ps (x, x);

   // logarithm polynom
   __m128 y = *(__m128 *)SSEMATH_PS_LOG_P0;
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P1);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P2);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P3);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P4);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P5);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P6);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P7);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_LOG_P8);
   y = _mm_mul_ps (y, x);
   y = _mm_mul_ps (y, xx);

   // put everything together
   y = _mm_add_ps (y, _mm_mul_ps (xp, *(__m128 *)SSEMATH_PS_LOG_Q1));
   y = _mm_sub_ps (y, _mm_mul_ps (xx, _mm_set1_ps (0.5f)));
   x = _mm_add_ps (x, _mm_mul_ps (xp, *(__m128 *)SSEMATH_PS_LOG_Q2));
   x = _mm_add_ps (x, y);

   __m128 k = _mm_mul_ps (exp, _mm_set1_ps (1.4426950408889634f));
   x = _mm_mul_ps (x, k);

   /*
   y = exp2( exp * k * log( base ) )

   exp2( x ):
   */

   // separate into integer and fractional parts
  __m128i trunc = _mm_cvttps_epi32 (x);
   __m128i sign_off = _mm_srli_epi32 (_mm_castps_si128 (x), 31);
   trunc = _mm_sub_epi32 (trunc, sign_off);
   __m128 px = _mm_cvtepi32_ps (trunc);
   x = _mm_sub_ps (x, px);

   __m128 mask2 = _mm_cmplt_ps (_mm_set1_ps (0.5f), x);
   tmp = _mm_and_ps (mask2, _mm_set1_ps (1.0f));
   x = _mm_sub_ps (x, tmp);

   // exponential polynom
   y = *(__m128 *)SSEMATH_PS_EXP2_P0;
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP2_P1);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP2_P2);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP2_P3);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP2_P4);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_EXP2_P5);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, _mm_set1_ps (1.0f));

   // y = y * 2 ^ fx
   px = _mm_add_ps (px, tmp);
   tmpi = _mm_cvttps_epi32 (px);
   tmpi = _mm_add_epi32 (tmpi, _mm_set1_epi32 (0x7f));
   tmpi = _mm_slli_epi32 (tmpi, 23);
   y = _mm_mul_ps (y, _mm_castsi128_ps (tmpi));
   y = _mm_min_ps (y, *(__m128 *)SSEMATH_PI32_MAX);
#endif

   /*
   if ( y == 0 ) z = 1;
   else if ( x == 0 && y < 0 ) z = inf;
   else if ( x == 0 ) z = 0;
   */
   __m128 mask_x0 = _mm_cmpeq_ps (base, _mm_setzero_ps ());
   __m128 mask_y0 = _mm_cmpeq_ps (exp, _mm_setzero_ps ());
   __m128 mask_inf = _mm_and_ps (mask_x0, _mm_cmplt_ps (exp, _mm_setzero_ps ()));
   __m128 mask_0 = _mm_or_ps (mask_x0, mask_y0);
   y = _mm_andnot_ps (mask_0, y);
   y = _mm_add_ps (y, _mm_and_ps (mask_y0, _mm_set1_ps (1.0f)));
   return _mm_add_ps (y, _mm_and_ps (mask_inf, *(__m128 *)SSEMATH_PI32_INF));
}

/**
cube root

\return Returns \f$\sqrt[3]{val}\f$
*/
SSE_INLINE __m128 cbrt_ps (
   __m128 val   //!< Value
) {
   // abs( x )
   __m128 x = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);
   __m128 z = x;

   // get exponent
  __m128i tmpi = _mm_srli_epi32 (_mm_castps_si128 (x), 23);
   tmpi = _mm_sub_epi32 (tmpi, _mm_set1_epi32 (0x7f));
   tmpi = _mm_add_epi32 (tmpi, _mm_set1_epi32 (1));
   __m128 exp = _mm_cvtepi32_ps (tmpi);

   // only keep mantissa
   x = _mm_and_ps (x, *(__m128 *)SSEMATH_PI32_INVMASK_MANT);
   x = _mm_or_ps (x, _mm_set1_ps (0.5f));
   __m128 mask_explt0 = _mm_cmplt_ps (exp, _mm_setzero_ps ());

   // cbrt polynom
   __m128 y = *(__m128 *)SSEMATH_PS_CBRT_P0;
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_CBRT_P1);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_CBRT_P2);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_CBRT_P3);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_CBRT_P4);

   // save sign bit of exponent
   __m128 sign_exp = _mm_and_ps (exp, *(__m128 *)SSEMATH_PI32_MASK_SIGN);

   // integer abs( exp )
   exp = _mm_and_ps (exp, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);
   tmpi = _mm_cvtps_epi32 (exp);

   // exponent handling
  __m128i rem = tmpi;
   __m128 tmp = _mm_mul_ps (exp, _mm_set1_ps (0.33333333333f));
   tmpi = _mm_cvttps_epi32 (tmp);
   exp = _mm_cvtepi32_ps (tmpi);
   tmp = _mm_mul_ps (exp, _mm_set1_ps (3.0f));
   tmpi = _mm_cvtps_epi32 (tmp);
   rem = _mm_sub_epi32 (rem, tmpi);
   __m128 mask_rem1 = _mm_castsi128_ps (_mm_cmpeq_epi32 (rem, _mm_set1_epi32 (1)));
   __m128 mask_rem2 = _mm_castsi128_ps (_mm_cmpeq_epi32 (rem, _mm_set1_epi32 (2)));

   // select factor for y
   __m128 mask = _mm_andnot_ps (mask_explt0, mask_rem1);
   __m128 fac = _mm_and_ps (mask, *(__m128 *)SSEMATH_PS_CBRT2);
   mask = _mm_andnot_ps (mask_explt0, mask_rem2);
   mask = _mm_and_ps (mask, *(__m128 *)SSEMATH_PS_CBRT4);
   fac = _mm_or_ps (fac, mask);
   mask = _mm_and_ps (mask_explt0, mask_rem1);
   mask = _mm_and_ps (mask, *(__m128 *)SSEMATH_PS_1_CBRT2);
   fac = _mm_or_ps (fac, mask);
   mask = _mm_and_ps (mask_explt0, mask_rem2);
   mask = _mm_and_ps (mask, *(__m128 *)SSEMATH_PS_1_CBRT4);
   fac = _mm_or_ps (fac, mask);
   mask = _mm_cmpeq_ps (fac, _mm_setzero_ps ());
   mask = _mm_and_ps (mask, _mm_set1_ps (1.0f));
   fac = _mm_or_ps (fac, mask);
   y = _mm_mul_ps (y, fac);

   // Restore sign bit of exponent
   exp = _mm_xor_ps (exp, sign_exp);

   // y = y * 2 ^ exp
   tmpi = _mm_cvttps_epi32 (exp);
   tmpi = _mm_add_epi32 (tmpi, _mm_set1_epi32 (0x7f));
   tmpi = _mm_slli_epi32 (tmpi, 23);
   y = _mm_mul_ps (y, _mm_castsi128_ps (tmpi));

   // Newton iteration
   z = _mm_div_ps (z, _mm_mul_ps (y, y));
   z = _mm_sub_ps (y, z);
   z = _mm_mul_ps (z, _mm_set1_ps (0.33333333333f));
   y = _mm_sub_ps (y, z);

   // toggle sign
   __m128 sign_bit = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_MASK_SIGN);
   return _mm_xor_ps (y, sign_bit);
}

/**
inverse sine

The value \p val must be in the range from -1.0 to 1.0 inclusive.
\return Returns \f$\arcsin({val})\f$ in radians. If \f$\left|{val}\right|\,>\,1\f$ NaN is returned.
*/
SSE_INLINE __m128 asin_ps (
   __m128 val   //!< Value
) {
   // abs( x )
   __m128 x = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);

   // mask for polynom selection
   __m128 poly_mask = _mm_cmplt_ps (_mm_set1_ps (0.5f), x);

   // calculate both cases
   __m128 z1 = _mm_mul_ps (_mm_set1_ps (0.5f), _mm_sub_ps (_mm_set1_ps (1.0f), x));
   __m128 x1 = _mm_sqrt_ps (z1);
   __m128 z2 = _mm_mul_ps (x, x);

   // use mask to select case
   z1 = _mm_and_ps (poly_mask, z1);
   z2 = _mm_andnot_ps (poly_mask, z2);
   __m128 z = _mm_add_ps (z1, z2);
   x1 = _mm_and_ps (poly_mask, x1);
   x = _mm_andnot_ps (poly_mask, x);
   x = _mm_add_ps (x, x1);

   // inverse sine polynom
   __m128 y = *(__m128 *)SSEMATH_PS_ASIN_P0;
   y = _mm_mul_ps (y, z);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_ASIN_P1);
   y = _mm_mul_ps (y, z);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_ASIN_P2);
   y = _mm_mul_ps (y, z);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_ASIN_P3);
   y = _mm_mul_ps (y, z);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_ASIN_P4);
   y = _mm_mul_ps (y, z);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, x);

   // adjustment
   __m128 y2 = _mm_sub_ps (*(__m128 *)SSEMATH_PS_PI_2, _mm_add_ps (y, y));
   y2 = _mm_and_ps (poly_mask, y2);
   y = _mm_andnot_ps (poly_mask, y);
   y = _mm_add_ps (y, y2);

   // toggle sign
   __m128 sign_bit = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_MASK_SIGN);
   y = _mm_xor_ps (y, sign_bit);

   // invalid values become NaN
   __m128 abs_val = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);
   __m128 invalid_mask = _mm_cmplt_ps (_mm_set1_ps (1.0f), abs_val);
   return _mm_or_ps (y, invalid_mask);
}

/**
inverse cosine

The value \p val must be in the range from -1.0 to 1.0 inclusive.
\return Returns \f$\arccos({val})\f$ in radians. If \f$\left|{val}\right|\,>\,1\f$ NaN is returned.
*/
SSE_INLINE __m128 acos_ps (
   __m128 val   //!< Value
) {
   // mask for value selection
   __m128 poly_mask1 = _mm_cmplt_ps (val, _mm_set1_ps (-0.5f));
   __m128 poly_mask2 = _mm_cmplt_ps (_mm_set1_ps (0.5f), val);

   // calculate values
   __m128 x1 = _mm_add_ps (_mm_set1_ps (1.0f), val);
   __m128 x2 = _mm_sub_ps (_mm_set1_ps (1.0f), val);

   // use mask to select case
   x1 = _mm_and_ps (poly_mask1, x1);
   x2 = _mm_and_ps (poly_mask2, x2);
   __m128 x = _mm_add_ps (x1, x2);
   x = _mm_sqrt_ps (_mm_mul_ps (_mm_set1_ps (0.5f), x));

   // include third case (this mask is inverted)
   __m128 poly_mask3 = _mm_or_ps (poly_mask1, poly_mask2);
   __m128 x3 = _mm_andnot_ps (poly_mask3, val);
   x = _mm_add_ps (x, x3);

   // save sign bit
   __m128 sign_bit = _mm_and_ps (x, *(__m128 *)SSEMATH_PI32_MASK_SIGN);

   // abs( x )
   x = _mm_and_ps (x, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);

   // inverse sine polynom
   __m128 z = _mm_mul_ps (x, x);
   __m128 y = *(__m128 *)SSEMATH_PS_ASIN_P0;
   y = _mm_mul_ps (y, z);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_ASIN_P1);
   y = _mm_mul_ps (y, z);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_ASIN_P2);
   y = _mm_mul_ps (y, z);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_ASIN_P3);
   y = _mm_mul_ps (y, z);
   y = _mm_add_ps (y, *(__m128 *)SSEMATH_PS_ASIN_P4);
   y = _mm_mul_ps (y, z);
   y = _mm_mul_ps (y, x);
   y = _mm_add_ps (y, x);

   // toggle sign
   x = _mm_xor_ps (y, sign_bit);

   // get multiplicators and offsets for different cases
   __m128 fact1 = _mm_and_ps (poly_mask1, _mm_set1_ps (-2.0f));
   __m128 fact2 = _mm_and_ps (poly_mask2, _mm_set1_ps (2.0f));
   __m128 fact = _mm_add_ps (fact1, fact2);
   __m128 fact3 = _mm_andnot_ps (poly_mask3, _mm_set1_ps (-1.0f));
   fact = _mm_add_ps (fact, fact3);
   x = _mm_mul_ps (x, fact);
   __m128 offs1 = _mm_and_ps (poly_mask1, *(__m128 *)SSEMATH_PS_PI);
   __m128 offs2 = _mm_and_ps (poly_mask2, _mm_setzero_ps ());
   __m128 offs = _mm_add_ps (offs1, offs2);
   __m128 offs3 = _mm_andnot_ps (poly_mask3, *(__m128 *)SSEMATH_PS_PI_2);
   offs = _mm_add_ps (offs, offs3);
   x = _mm_add_ps (x, offs);

   // invalid values become NaN
   __m128 abs_val = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);
   __m128 invalid_mask = _mm_cmplt_ps (_mm_set1_ps (1.0f), abs_val);
   return _mm_or_ps (x, invalid_mask);
}

/**
inverse tangens

\return Returns \f$\arctan({val})\f$ in the range \f$\left[-\frac{\pi}{2}, \frac{\pi}{2}\right]\f$ radians.
*/
SSE_INLINE __m128 atan_ps (
   __m128 val   //!< Value
) {
   // abs( x )
   __m128 x = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_INVMASK_SIGN);

   // mask for case selection
   __m128 mask1 = _mm_cmplt_ps (*(__m128 *)SSEMATH_PS_ATAN_Q0, x);
   __m128 mask2 = _mm_andnot_ps (mask1, _mm_cmplt_ps (*(__m128 *)SSEMATH_PS_ATAN_Q1, x));

   // this mask is inverted
   __m128 mask3 = _mm_or_ps (mask1, mask2);

   // range reduction
   __m128 y = _mm_and_ps (mask1, *(__m128 *)SSEMATH_PS_PI_2);
   y = _mm_add_ps (y, _mm_and_ps (mask2, *(__m128 *)SSEMATH_PS_PI_4));
   __m128 x1 = _mm_div_ps (_mm_set1_ps (-1.0f), x);
   __m128 x2 = _mm_div_ps (_mm_sub_ps (x, _mm_set1_ps (1.0f)), _mm_add_ps (x, _mm_set1_ps (1.0f)));
   x = _mm_andnot_ps (mask3, x);
   x = _mm_add_ps (x, _mm_and_ps (mask1, x1));
   x = _mm_add_ps (x, _mm_and_ps (mask2, x2));

   // inverse tangent polynom
   __m128 z = _mm_mul_ps (x, x);
   __m128 tmp = *(__m128 *)SSEMATH_PS_ATAN_P0;
   tmp = _mm_mul_ps (tmp, z);
   tmp = _mm_add_ps (tmp, *(__m128 *)SSEMATH_PS_ATAN_P1);
   tmp = _mm_mul_ps (tmp, z);
   tmp = _mm_add_ps (tmp, *(__m128 *)SSEMATH_PS_ATAN_P2);
   tmp = _mm_mul_ps (tmp, z);
   tmp = _mm_add_ps (tmp, *(__m128 *)SSEMATH_PS_ATAN_P3);
   tmp = _mm_mul_ps (tmp, z);
   tmp = _mm_mul_ps (tmp, x);
   tmp = _mm_add_ps (tmp, x);
   tmp = _mm_add_ps (tmp, y);

   // toggle sign
   __m128 sign_bit = _mm_and_ps (val, *(__m128 *)SSEMATH_PI32_MASK_SIGN);
   return _mm_xor_ps (tmp, sign_bit);
}

/**
inverse tangens

\return Returns \f$\arctan(\frac{y}{x})\f$ in the range \f$\left[-\pi, \pi\right]\f$ radians.
*/
SSE_INLINE __m128 atan2_ps (
   __m128 y,    //!< Y value
   __m128 x     //!< X value
) {
   // build special constant values
   __m128 mask_ygt0 = _mm_cmplt_ps (_mm_setzero_ps (), y);
   __m128 mask_ylt0 = _mm_cmplt_ps (y, _mm_setzero_ps ());
   __m128 tmp1 = _mm_and_ps (mask_ygt0, *(__m128 *)SSEMATH_PS_PI_2);
   __m128 tmp2 = _mm_and_ps (mask_ylt0, *(__m128 *)SSEMATH_PS_PI_2);
   __m128 val = _mm_sub_ps (tmp1, tmp2);

   // offset for atan value
   __m128 mask_xlt0 = _mm_cmplt_ps (x, _mm_setzero_ps ());
   mask_ygt0 = _mm_andnot_ps (mask_ylt0, mask_xlt0);
   mask_ylt0 = _mm_and_ps (mask_ylt0, mask_xlt0);
   tmp1 = _mm_and_ps (mask_ygt0, *(__m128 *)SSEMATH_PS_PI);
   tmp2 = _mm_and_ps (mask_ylt0, *(__m128 *)SSEMATH_PS_PI);
   __m128 offs = _mm_sub_ps (tmp1, tmp2);

   // calculate atan( y / x )
   __m128 mask_xeq0 = _mm_cmpeq_ps (x, _mm_setzero_ps ());
   __m128 atan = atan_ps (_mm_div_ps (y, x));

   // add offset and select result or special value
   atan = _mm_add_ps (atan, offs);
   atan = _mm_andnot_ps (mask_xeq0, atan);
   val = _mm_and_ps (mask_xeq0, val);
   return _mm_add_ps (atan, val);
}

#endif
