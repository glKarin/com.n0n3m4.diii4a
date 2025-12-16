/* Copyright (C) 2011  Julien Pommier

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  (this is the zlib license)
*/

#pragma once

constexpr auto c_sincof_p0 = -1.9515295891E-4f;
constexpr auto c_sincof_p1 = 8.3321608736E-3f;
constexpr auto c_sincof_p2 = -1.6666654611E-1f;

constexpr auto c_minus_cephes_DP1 = -0.78515625f;
constexpr auto c_minus_cephes_DP2 = -2.4187564849853515625e-4f;
constexpr auto c_minus_cephes_DP3 = -3.77489497744594108e-8f;

constexpr auto c_coscof_p0 = 2.443315711809948E-005f;
constexpr auto c_coscof_p1 = -1.388731625493765E-003f;
constexpr auto c_coscof_p2 = 4.166664568298827E-002f;

constexpr auto c_cephes_FOPI = 1.27323954473516f;

static inline void sincos_ps (float32x4_t x, float32x4_t &ysin, float32x4_t &ycos) {  // any x
   float32x4_t xmm1, xmm2, xmm3, y;

   uint32x4_t emm2;

   uint32x4_t sign_mask_sin, sign_mask_cos;
   sign_mask_sin = vcltq_f32 (x, vdupq_n_f32 (0));
   x = vabsq_f32 (x);

   /* scale by 4/Pi */
   y = vmulq_f32 (x, vdupq_n_f32 (c_cephes_FOPI));

   /* store the integer part of y in mm0 */
   emm2 = vcvtq_u32_f32 (y);
   /* j=(j+1) & (~1) (see the cephes sources) */
   emm2 = vaddq_u32 (emm2, vdupq_n_u32 (1));
   emm2 = vandq_u32 (emm2, vdupq_n_u32 (~1));
   y = vcvtq_f32_u32 (emm2);

   /* get the polynom selection mask
    there is one polynom for 0 <= x <= Pi/4
    and another one for Pi/4<x<=Pi/2

    Both branches will be computed.
 */
   uint32x4_t poly_mask = vtstq_u32 (emm2, vdupq_n_u32 (2));

   /* The magic pass: "Extended precision modular arithmetic"
    x = ((x - y * DP1) - y * DP2) - y * DP3; */
   xmm1 = vmulq_n_f32 (y, c_minus_cephes_DP1);
   xmm2 = vmulq_n_f32 (y, c_minus_cephes_DP2);
   xmm3 = vmulq_n_f32 (y, c_minus_cephes_DP3);
   x = vaddq_f32 (x, xmm1);
   x = vaddq_f32 (x, xmm2);
   x = vaddq_f32 (x, xmm3);

   sign_mask_sin = veorq_u32 (sign_mask_sin, vtstq_u32 (emm2, vdupq_n_u32 (4)));
   sign_mask_cos = vtstq_u32 (vsubq_u32 (emm2, vdupq_n_u32 (2)), vdupq_n_u32 (4));

   /* Evaluate the first polynom  (0 <= x <= Pi/4) in y1,
    and the second polynom      (Pi/4 <= x <= 0) in y2 */
   float32x4_t z = vmulq_f32 (x, x);
   float32x4_t y1, y2;

   y1 = vmulq_n_f32 (z, c_coscof_p0);
   y2 = vmulq_n_f32 (z, c_sincof_p0);
   y1 = vaddq_f32 (y1, vdupq_n_f32 (c_coscof_p1));
   y2 = vaddq_f32 (y2, vdupq_n_f32 (c_sincof_p1));
   y1 = vmulq_f32 (y1, z);
   y2 = vmulq_f32 (y2, z);
   y1 = vaddq_f32 (y1, vdupq_n_f32 (c_coscof_p2));
   y2 = vaddq_f32 (y2, vdupq_n_f32 (c_sincof_p2));
   y1 = vmulq_f32 (y1, z);
   y2 = vmulq_f32 (y2, z);
   y1 = vmulq_f32 (y1, z);
   y2 = vmulq_f32 (y2, x);
   y1 = vsubq_f32 (y1, vmulq_f32 (z, vdupq_n_f32 (0.5f)));
   y2 = vaddq_f32 (y2, x);
   y1 = vaddq_f32 (y1, vdupq_n_f32 (1));

   /* select the correct result from the two polynoms */
   float32x4_t ys = vbslq_f32 (poly_mask, y1, y2);
   float32x4_t yc = vbslq_f32 (poly_mask, y2, y1);

   ysin = vbslq_f32 (sign_mask_sin, vnegq_f32 (ys), ys);
   ycos = vbslq_f32 (sign_mask_cos, yc, vnegq_f32 (yc));
}

static inline float32x4_t div_ps (float32x4_t a, float32x4_t b) {
#if __aarch64__
   return vdivq_f32 (a, b);
#else
   float32x4_t reciprocal = vrecpeq_f32 (b);
   reciprocal = vmulq_f32 (vrecpsq_f32 (b, reciprocal), reciprocal);
   // reciprocal = vmulq_f32(vrecpsq_f32(b, reciprocal), reciprocal);
   return vmulq_f32 (a, reciprocal);
#endif
}

static inline float32x4_t sqrt_ps (float32x4_t a) {
#if __aarch64__
   return vsqrtq_f32 (a);
#else
   float32x4_t _reciprocal = vrsqrteq_f32 (a);
   _reciprocal = vmulq_f32 (vrsqrtsq_f32 (vmulq_f32 (a, _reciprocal), _reciprocal), _reciprocal);
   return vmulq_f32 (a, _reciprocal);
#endif
}
