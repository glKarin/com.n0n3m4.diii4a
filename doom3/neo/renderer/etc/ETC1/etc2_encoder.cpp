/*
Copyright (C) 2014 Jean-Philippe ANDRE

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "etc_rg_etc1.h"
#include <cstdint>
#include <limits>
#include <cstring>
#include <vector>

// Enable this flag when working on (quality) optimizations
//#define DEBUG

#ifdef DEBUG
// Weights for the distance (perceptual mode) - sum is ~1024
static const int R_WEIGHT = 299 * 1024 / 1000;
static const int G_WEIGHT = 587 * 1024 / 1000;
static const int B_WEIGHT = 114 * 1024 / 1000;
#endif

static const int kTargetError[3] = {
   5*5*16, // 34 dB
   2*2*16, // 42 dB
   0 // infinite dB
};

// For T and H modes
static const int kDistances[8] = {
   3, 6, 11, 16, 23, 32, 41, 64
};

// For differential mode
static const int kSigned3bit[8] = {
   0, 1, 2, 3, -4, -3, -2, -1
};

// For alpha support
static const int kAlphaModifiers[16][8] = {
   {  -3,  -6,  -9,  -15,  2,  5,  8,  14},
   {  -3,  -7, -10,  -13,  2,  6,  9,  12},
   {  -2,  -5,  -8,  -13,  1,  4,  7,  12},
   {  -2,  -4,  -6,  -13,  1,  3,  5,  12},
   {  -3,  -6,  -8,  -12,  2,  5,  7,  11},
   {  -3,  -7,  -9,  -11,  2,  6,  8,  10},
   {  -4,  -7,  -8,  -11,  3,  6,  7,  10},
   {  -3,  -5,  -8,  -11,  2,  4,  7,  10},
   {  -2,  -6,  -8,  -10,  1,  5,  7,   9},
   {  -2,  -5,  -8,  -10,  1,  4,  7,   9},
   {  -2,  -4,  -8,  -10,  1,  3,  7,   9},
   {  -2,  -5,  -7,  -10,  1,  4,  6,   9},
   {  -3,  -4,  -7,  -10,  2,  3,  6,   9},
   {  -1,  -2,  -3,  -10,  0,  1,  2,   9},
   {  -4,  -6,  -8,   -9,  3,  5,  7,   8},
   {  -3,  -5,  -7,   -9,  2,  4,  6,   8}
};

// Damn OpenGL people, why don't you just pack data as on a CPU???
static const int kBlockWalk[16] = {
   0, 4,  8, 12,
   1, 5,  9, 13,
   2, 6, 10, 14,
   3, 7, 11, 15
};

// Use with static constants so the compiler can optimize everything
#define BITS(byteval, lowbit, highbit) \
   (((byteval) >> (lowbit)) & ((1 << ((highbit) - (lowbit) + 1)) - 1))

#define BIT(byteval, bit) \
   (((byteval) >> (bit)) & 0x1)

#ifndef _MSC_VER
#define LIKELY(x) __builtin_expect((x), 1)
#else
#define LIKELY
#endif

// Real clamp
static inline int
clampi(int a)
{
   if (LIKELY(!(a & ~0xFF)))
	 return a;
   else if (a < 0)
	 return 0;
   else
	 return 0xFF;
}

// Simple abs
static inline int
absi(int a)
{
   if (LIKELY(a >= 0))
	 return a;
   else
	 return (-a);
}

inline uint32_t RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {return ((a << 24) | (b << 16) | (g << 8) | r);}

#define A_VAL(v) ((uint8_t) ((v & 0xFF000000) >> 24))
#define B_VAL(v) ((uint8_t) ((v & 0x00FF0000) >> 16))
#define G_VAL(v) ((uint8_t) ((v & 0x0000FF00) >> 8))
#define R_VAL(v) ((uint8_t) ((v & 0x000000FF)))

#ifndef WORDS_BIGENDIAN
# define RGB_START 0
# define RGB_END   2
#else
# define RGB_START 1
# define RGB_END   3
#endif

#ifndef DBG
# ifdef DEBUG
#  define DBG(fmt, ...) fprintf(stderr, "%s:%d: " fmt "\n", __FUNCTION__, __LINE__, ## __VA_ARGS__)
# else
#  define DBG(...)
# endif
#endif

/** Pack alpha block given a modifier table and a multiplier
 * @returns Squared error
 */
static int
_etc2_alpha_block_pack(uint8_t *etc2_alpha,
					   const int base_codeword,
					   const int multiplier,
					   const int modifierIdx,
					   const uint32_t* rgba,
					   const bool write)
{
   const int *alphaModifiers = kAlphaModifiers[modifierIdx];
   uint8_t alphaIndexes[16];
   int errAcc2 = 0;

   // Header
   if (write)
	 {
		etc2_alpha[0] = base_codeword & 0xFF;
		etc2_alpha[1] = ((multiplier << 4) & 0xF0) | (modifierIdx & 0x0F);
	 }

   // Compute alphas now
   for (int i = 0; i < 16; i++)
	 {
		const int realA = A_VAL(rgba[kBlockWalk[i]]);
		int minErr = std::numeric_limits<int>::max(), idx = 0;

		// Brute force -- find modifier index
		for (int k = 0; (k < 8) && minErr; k++)
		  {
			 int tryA = clampi(base_codeword + alphaModifiers[k] * multiplier);
			 int err = absi(realA - tryA);
			 if (err < minErr)
			   {
				  minErr = err;
				  idx = k;
				  if (!minErr) break;
			   }
		  }

		alphaIndexes[i] = idx;

		// Keep some stats
		errAcc2 += minErr * minErr;
	 }

   if (write)
	 for (int k = 0; k < 2; k++)
	   {
		  etc2_alpha[2 + 3 * k]  =  alphaIndexes[0 + 8 * k] << 5;        // A
		  etc2_alpha[2 + 3 * k] |=  alphaIndexes[1 + 8 * k] << 2;        // B
		  etc2_alpha[2 + 3 * k] |= (alphaIndexes[2 + 8 * k] >> 1) & 0x3; // C01
		  etc2_alpha[3 + 3 * k]  = (alphaIndexes[2 + 8 * k] & 0x1) << 7; // C2
		  etc2_alpha[3 + 3 * k] |=  alphaIndexes[3 + 8 * k] << 4;        // D
		  etc2_alpha[3 + 3 * k] |=  alphaIndexes[4 + 8 * k] << 1;        // E
		  etc2_alpha[3 + 3 * k] |= (alphaIndexes[5 + 8 * k] >> 2) & 0x1; // F0
		  etc2_alpha[4 + 3 * k]  = (alphaIndexes[5 + 8 * k] & 0x3) << 6; // F12
		  etc2_alpha[4 + 3 * k] |=  alphaIndexes[6 + 8 * k] << 3;        // G
		  etc2_alpha[4 + 3 * k] |=  alphaIndexes[7 + 8 * k];             // H
	   }

   return errAcc2;
}

static int
_etc2_alpha_encode(uint8_t *etc2_alpha, const uint32_t *rgba,
				   const rg_etc1::etc1_pack_params& params)
{
   int alphas[16], avg = 0, diff = 0, maxDiff = 0, minErr = std::numeric_limits<int>::max();
   int base_codeword;
   int multiplier, bestMult = 0;
   int modifierIdx, bestIdx = 0, bestBase = 128;
   int err, base_range = 40, base_step = 4, max_error = 0;

   // Try to select the best alpha value (avg)
   for (int i = 0; i < 16; i++)
	 {
		alphas[i] = A_VAL(rgba[kBlockWalk[i]]);
		avg += alphas[i];
	 }
   avg /= 16;

   for (int i = 0; i < 16; i++)
	 {
		int thisDiff = absi(alphas[i] - avg);
		if (thisDiff > maxDiff)
		  maxDiff = thisDiff;
		diff += thisDiff;
	 }

   base_codeword = alphas[0];
   if (!diff)
	 {
		// All same alphas
		etc2_alpha[0] = base_codeword;
		memset(etc2_alpha + 1, 0, 7);
		return 0;
	 }

   // Bruteforce -- try all tables and all multipliers, oh my god this will be slow.
   max_error = kTargetError[params.m_quality];
   switch (params.m_quality)
	 {
	  // The follow parameters are completely arbitrary.
	  // Need some real testing.
	  case rg_etc1::cHighQuality: // exhaustive search
		base_range = 255;
		base_step = 1;
		break;
	  case rg_etc1::cMediumQuality: // tweaked for "decent" results
		base_range = 40;
		base_step = 4;
		break;
	  case rg_etc1::cLowQuality: // fast (not even fastest)
		base_range = 8;
		base_step = 4;
		break;
	 }

   // for loop avg, avg-1, avg+1, avg-2, avg+2, ...
   for (int step = 0; step < base_range; step += base_step)
	 for (base_codeword = clampi(avg - step);
		  base_codeword <= clampi(avg + step);)
	   {
		  for (modifierIdx = 0; modifierIdx < 16; modifierIdx++)
			for (multiplier = 0; multiplier < 16; multiplier++)
			  {
				 if ((absi(multiplier * kAlphaModifiers[modifierIdx][3]) + absi(base_codeword - avg)) < maxDiff)
				   continue;

				 err = _etc2_alpha_block_pack(etc2_alpha, base_codeword, multiplier, modifierIdx, rgba, false);
				 if (err < minErr)
				   {
					  minErr = err;
					  bestMult = multiplier;
					  bestIdx = modifierIdx;
					  bestBase = base_codeword;
					  if (err <= max_error)
						goto pack_now;

				   }
			  }
		  if (step <= 0) break;
		  if (base_codeword < 255)
			base_codeword = clampi(base_codeword + 2 * step);
		  else
			break;
	   }

pack_now:
   err = _etc2_alpha_block_pack(etc2_alpha, bestBase, bestMult, bestIdx, rgba, true);
   return err;
}

static bool
_etc2_t_mode_header_pack(uint8_t *etc2, uint32_t color1, uint32_t color2, int distance)
{
   // 4 bit colors
   int r1_4 = R_VAL(color1) >> 4;
   int g1_4 = G_VAL(color1) >> 4;
   int b1_4 = B_VAL(color1) >> 4;
   int r2_4 = R_VAL(color2) >> 4;
   int g2_4 = G_VAL(color2) >> 4;
   int b2_4 = B_VAL(color2) >> 4;
   int distanceIdx, R, dR;

   for (distanceIdx = 0; distanceIdx < 8; distanceIdx++)
	 if (kDistances[distanceIdx] == distance) break;

   if (distanceIdx >= 8)
	 return false;

   // R1. R + [dR] must be outside [0..31]. Scanning all values. Not smart.
   R  = r1_4 >> 2;
   dR = r1_4 & 0x3;
   for (int Rx = 0; Rx < 8; Rx++)
	 for (int dRx = 0; dRx < 2; dRx++)
	   {
		  int Rtry = R | (Rx << 2);
		  int dRtry = dR | (dRx << 2);
		  if ((Rtry + kSigned3bit[dRtry]) < 0 || (Rtry + kSigned3bit[dRtry] > 31))
			{
			   R = Rtry;
			   dR = dRtry;
			   break;
			}
	   }
   if ((R + kSigned3bit[dR]) >= 0 && (R + kSigned3bit[dR] <= 31))
	 // this can't happen, should be an assert
	 return false;

   etc2[0] = ((R & 0x1F) << 3) | (dR & 0x7);

   // G1, B1
   etc2[1] = (g1_4 << 4) | b1_4;

   // R2, G2
   etc2[2] = (r2_4 << 4) | g2_4;

   // B2, distance
   etc2[3] = (b2_4 << 4) | ((distanceIdx >> 1) << 2) | (1 << 1) | (distanceIdx & 0x1);

   return true;
}

static bool
_etc2_h_mode_header_pack(uint8_t *etc2, bool *swap_colors,
						 uint32_t color1, uint32_t color2, int distance)
{
	int distanceIdx;
	for (distanceIdx = 0; distanceIdx < 8; distanceIdx++)
	{
		if (kDistances[distanceIdx] == distance)
			break;
	}
	if (distanceIdx >= 8)
		return false;

	// The distance is coded in 3 bits. But in H mode, one bit is not coded
	// in the header, as we use the comparison result between the two colors
	// to select it.
	int distanceSpecialBit = BIT(distanceIdx, 0);
	int db = BIT(distanceIdx, 1);
	int da = BIT(distanceIdx, 2);

	// Note: if c1 == c2, no big deal because H is not the best choice of mode
	uint32_t c1, c2;
	if (distanceSpecialBit)
	{
		c1 = (color1 > color2) ? color1 : color2;
		c2 = (color1 <= color2) ? color1 : color2;
	}
	else
	{
		c1 = (color1 <= color2) ? color1 : color2;
		c2 = (color1 > color2) ? color1 : color2;
	}

	// Return flag so we use the proper colors when packing the block
	*swap_colors = (c1 != color1);

	// 4 bit colors
	int r1_4 = R_VAL(c1) >> 4;
	int g1_4 = G_VAL(c1) >> 4;
	int b1_4 = B_VAL(c1) >> 4;
	int r2_4 = R_VAL(c2) >> 4;
	int g2_4 = G_VAL(c2) >> 4;
	int b2_4 = B_VAL(c2) >> 4;

	// R1 + G1a. R + [dR] must be inside [0..31]. Scanning all values. Not smart.
	int R  = r1_4;
	int dR = g1_4 >> 1;
	if ((R + kSigned3bit[dR]) < 0 || (R + kSigned3bit[dR] > 31))
		R |= (1 << 4);

	if ((R + kSigned3bit[dR]) < 0 || (R + kSigned3bit[dR] > 31))
		return false; // wtf?

	etc2[0] = ((R & 0x1F) << 3) | (dR & 0x7);

	// G1b + B1a + B1b[2 msb]. G + dG must be outside the range.
	int G  = (g1_4 & 0x1) << 1;
	G |= BIT(b1_4, 3);
	int dG = BITS(b1_4, 1, 2);
	for (int Gx = 0; Gx < 8; Gx++)
		for (int dGx = 0; dGx < 2; dGx++)
		{
			int Gtry = G | (Gx << 2);
			int dGtry = dG | (dGx << 2);
			if ((Gtry + kSigned3bit[dGtry]) < 0 || (Gtry + kSigned3bit[dGtry] > 31))
			{
				G = Gtry;
				dG = dGtry;
				break;
			}
		}

	if ((G + kSigned3bit[dG]) >= 0 && (G + kSigned3bit[dG] <= 31))
		return false; // wtf?

	etc2[1] = ((G & 0x1F) << 3) | (dG & 0x7);

	// B1[lsb] + R2 + G2 [3 msb]
	etc2[2] = ((b1_4 & 0x1) << 7) | (r2_4 << 3) | (g2_4 >> 1);

	// G2[lsb] + B2 + distance
	etc2[3] = ((g2_4 & 0x1) << 7) | (b2_4 << 3) | (da << 2) | 0x2 | db;

	return true;
}

#ifdef DEBUG
static inline int
_rgb_distance_percept(uint32_t color1, uint32_t color2)
{
   int R = R_VAL(color1) - R_VAL(color2);
   int G = G_VAL(color1) - G_VAL(color2);
   int B = B_VAL(color1) - B_VAL(color2);
   return (R * R * R_WEIGHT) + (G * G * G_WEIGHT) + (B * B * B_WEIGHT);
}
#endif

static inline int
_rgb_distance_euclid(uint32_t color1, uint32_t color2)
{
   int R = R_VAL(color1) - R_VAL(color2);
   int G = G_VAL(color1) - G_VAL(color2);
   int B = B_VAL(color1) - B_VAL(color2);
   return (R * R) + (G * G) + (B * B);
}

static unsigned int
_etc2_th_mode_block_pack(uint8_t *etc2, bool h_mode,
						 uint32_t c1, uint32_t c2, int distance,
						 const uint32_t *rgba, bool write,
						 bool *swap_colors)
{
   union {
	  uint8_t c[4];
	  uint32_t v;
   } paint_colors[4], color1, color2;
   int errAcc = 0;

   paint_colors[0].v = 0;
   paint_colors[1].v = 0;
   paint_colors[2].v = 0;
   paint_colors[3].v = 0;
   color1.v = c1;
   color2.v = c2;

   if (write)
	 {
		memset(etc2 + 4, 0, 4);
		if (!h_mode)
		  {
			 if (!_etc2_t_mode_header_pack(etc2, color1.v, color2.v, distance))
			   return std::numeric_limits<int>::max(); // assert
		  }
		else
		  {
			 if (!_etc2_h_mode_header_pack(etc2, swap_colors,
										   color1.v, color2.v, distance))
			   return std::numeric_limits<int>::max(); // assert
			 if (*swap_colors)
			   {
				  uint32_t tmp = color1.v;
				  color1.v = color2.v;
				  color2.v = tmp;
			   }
		  }
	 }

   // Set paint_colors using R,G,B values (byte order is preserved)
   for (int k = RGB_START; k <= RGB_END; k++)
	 {
		if (!h_mode)
		  {
			 paint_colors[0].c[k] = color1.c[k];
			 paint_colors[1].c[k] = clampi(color2.c[k] + distance);
			 paint_colors[2].c[k] = color2.c[k];
			 paint_colors[3].c[k] = clampi(color2.c[k] - distance);
		  }
		else
		  {
			 paint_colors[0].c[k] = clampi(color1.c[k] + distance);
			 paint_colors[1].c[k] = clampi(color1.c[k] - distance);
			 paint_colors[2].c[k] = clampi(color2.c[k] + distance);
			 paint_colors[3].c[k] = clampi(color2.c[k] - distance);
		  }
	 }

   for (int k = 0; k < 16; k++)
	 {
		uint32_t pixel = rgba[kBlockWalk[k]];
		int bestDist = std::numeric_limits<int>::max();
		int bestIdx = 0;

		for (int idx = 0; idx < 4; idx++)
		  {
			 int dist = _rgb_distance_euclid(pixel, paint_colors[idx].v);
			 if (dist < bestDist)
			   {
				  bestDist = dist;
				  bestIdx = idx;
				  if (!dist) break;
			   }
		  }

		errAcc += bestDist;

		if (write)
		  {
			 etc2[5 - (k >> 3)] |= ((bestIdx & 0x2) ? 1 : 0) << (k & 0x7);
			 etc2[7 - (k >> 3)] |= (bestIdx & 0x1) << (k & 0x7);
		  }
	 }

   return errAcc;
}

static uint32_t
_color_reduce_444(uint32_t color)
{
   int R = R_VAL(color);
   int G = G_VAL(color);
   int B = B_VAL(color);

	int R1 = (R & 0xF0) | (R >> 4);
	int R2 = ((R & 0xF0) + 0x10) | ((R >> 4) + 1);
	int G1 = (G & 0xF0) | (G >> 4);
	int G2 = ((G & 0xF0) + 0x10) | ((G >> 4) + 1);
	int B1 = (B & 0xF0) | (B >> 4);
	int B2 = ((B & 0xF0) + 0x10) | ((B >> 4) + 1);

   R = (absi(R - R1) <= absi(R - R2)) ? R1 : R2;
   G = (absi(G - G1) <= absi(G - G2)) ? G1 : G2;
   B = (absi(B - B1) <= absi(B - B2)) ? B1 : B2;

   return RGBA(R, G, B, 255);
}

static uint32_t
_color_reduce_676(uint32_t color)
{
   int R = R_VAL(color);
   int G = G_VAL(color);
   int B = B_VAL(color);
   int R1, G1, B1;

   // FIXME: Do we have better candidates to try?
   R1 = (R & 0xFC) | (R >> 6);
   G1 = (G & 0xFE) | (G >> 7);
   B1 = (B & 0xFC) | (B >> 6);

   return RGBA(R1, G1, B1, 255);
}

static int
_block_main_colors_find(uint32_t *color1_out, uint32_t *color2_out,
						uint32_t color1, uint32_t color2, const uint32_t *rgba,
						const rg_etc1::etc1_pack_params& params)
{
   static const int kMaxIterations = 20;

   int errAcc;

   /* k-means complexity is O(n^(d.k+1) log n)
	* In this case, n = 16, k = 2, d = 3 so 20 loops
	*/

   if (color1 == color2)
	 {
		// We should select another mode (planar) to encode flat colors
		// We could also dither with two approximated colors
		*color1_out = *color2_out = color1;
		goto found;
	 }

   if (color1 == color2)
	 {
		// We should dither...
		*color1_out = *color2_out = color1;
		goto found;
	 }

	for (int iter = 0; iter < kMaxIterations; iter++)
	{
		int r1 = 0, r2 = 0, g1 = 0, g2 = 0, b1 = 0, b2 = 0;
		int cluster1_cnt = 0, cluster2_cnt = 0;
		int cluster1[16], cluster2[16];
		int maxDist1 = 0, maxDist2 = 0;

		memset(cluster1, 0, sizeof(cluster1));
		memset(cluster2, 0, sizeof(cluster2));

		// k-means assignment step
		for (int k = 0; k < 16; k++)
		{
			int dist1 = _rgb_distance_euclid(color1, rgba[k]);
			int dist2 = _rgb_distance_euclid(color2, rgba[k]);
			if (dist1 <= dist2)
			{
				cluster1[cluster1_cnt++] = k;
				if (dist1 > maxDist1)
					maxDist1 = dist1;
			}
			else
			{
				cluster2[cluster2_cnt++] = k;
				if (dist2 > maxDist2)
					maxDist2 = dist2;
			}
		}

		// k-means failed
		if (!cluster1_cnt || !cluster2_cnt)
			return -1;

		// k-means update step
		for (int k = 0; k < cluster1_cnt; k++)
		  {
			 r1 += R_VAL(rgba[cluster1[k]]);
			 g1 += G_VAL(rgba[cluster1[k]]);
			 b1 += B_VAL(rgba[cluster1[k]]);
		  }

		for (int k = 0; k < cluster2_cnt; k++)
		  {
			 r2 += R_VAL(rgba[cluster2[k]]);
			 g2 += G_VAL(rgba[cluster2[k]]);
			 b2 += B_VAL(rgba[cluster2[k]]);
		  }

		r1 /= cluster1_cnt;
		g1 /= cluster1_cnt;
		b1 /= cluster1_cnt;
		r2 /= cluster2_cnt;
		g2 /= cluster2_cnt;
		b2 /= cluster2_cnt;

		uint32_t c1 = _color_reduce_444(RGBA(r1, g1, b1, 255));
		uint32_t c2 = _color_reduce_444(RGBA(r2, g2, b2, 255));
		if (c1 == color1 && c2 == color2)
			break;

		if (c1 != c2)
		  {
			 color1 = c1;
			 color2 = c2;
		  }
		else if (_rgb_distance_euclid(c1, color1) > _rgb_distance_euclid(c2, color2))
		  {
			 color1 = c1;
		  }
		else
		  {
			 color2 = c2;
		  }
	 }

   *color1_out = color1;
   *color2_out = color2;

found:
   errAcc = 0;
   for (int k = 0; k < 16; k++)
	 errAcc += _rgb_distance_euclid(rgba[k], color2);
   return errAcc;
}

static unsigned int
_etc2_th_mode_block_encode(uint8_t *etc2, const uint32_t *rgba,
						   const rg_etc1::etc1_pack_params& params)
{
   int err, bestDist = kDistances[0];
	int minErr = std::numeric_limits<int>::max(), bestMode = 0;
	uint32_t bestC1 = rgba[0], bestC2 = rgba[1];
   bool swap_colors = false;

	struct ColorPair
	{
		uint32_t low;
		uint32_t high;
	};
	std::vector<ColorPair> tried_pairs;
	ColorPair pair;

   /* Bruteforce algo:
	* Bootstrap k-means clustering with all possible pairs of colors
	* from the 4x4 block.
	* TODO: Don't retry the same rgb444 pairs again
	*/

	for (int pix1 = 0; pix1 < 15; pix1++)
		for (int pix2 = pix1 + 1; pix2 < 16; pix2++)
		{
			bool already_tried = false;

			// Bootstrap k-means. Find new pair of colors.
			uint32_t c1 = _color_reduce_444(rgba[pix1]);
			uint32_t c2 = _color_reduce_444(rgba[pix2]);

			if (c2 > c1)
			{
				uint32_t tmp = c2;
				c2 = c1;
				c1 = tmp;
			}

			for(ColorPair p: tried_pairs)
			{
				if (c1 == p.high && c2 == p.low)
				{
					already_tried = true;
					pair = p;
					break;
				}
			}

		  if (already_tried)
			continue;

			pair.high = c1;
			pair.low = c2;
			tried_pairs.push_back(pair);


		  // Run k-means
		  err = _block_main_colors_find(&c1, &c2, c1, c2, rgba, params);
		  if (err < 0)
			continue;

		  for (int distIdx = 0; distIdx < 8; distIdx++)
			{
			   for (int mode = 0; mode < 2; mode++)
				 {

					for (int swap = 0; swap < 2; swap++)
					  {
						 if (mode == 0 && swap)
						   {
							  uint32_t tmp = c2;
							  c2 = c1;
							  c1 = tmp;
						   }
						 err = _etc2_th_mode_block_pack(etc2, mode, c1, c2,
														kDistances[distIdx],
														rgba, false,
														&swap_colors);
						 if (err < minErr)
						   {
							  bestDist = kDistances[distIdx];
							  minErr = err;
							  bestC1 = (!swap_colors) ? c1 : c2;
							  bestC2 = (!swap_colors) ? c2 : c1;
							  bestMode = mode;
							  if (err <= kTargetError[params.m_quality])
								goto found;
						   }
					  }
				 }
			}
	   }

found:
   err = _etc2_th_mode_block_pack(etc2, bestMode, bestC1, bestC2, bestDist,
								  rgba, true, &swap_colors);

   return err;
}

static inline bool
_etc2_planar_mode_header_pack(uint8_t *etc2,
							  uint32_t RO, uint32_t RH, uint32_t RV,
							  uint32_t GO, uint32_t GH, uint32_t GV,
							  uint32_t BO, uint32_t BH, uint32_t BV)
{
   int R, dR;
   int G, dG;
   int B, dB;

   // RO_6 [2..5]
   R = BITS(RO >> 2, 2, 5);
   // RO_6 [0..1] + GO_7[6]
   dR = (BITS(RO >> 2, 0, 1) << 1) | BIT(GO >> 1, 6);

   if (!((R + kSigned3bit[dR] >= 0) && (R + kSigned3bit[dR] <= 31)))
	 R |= 1 << 4;

   // GO_7[2..5]
   G = BITS(GO >> 1, 2, 5);
   // GO_7[0..1] + BO_6[5]
   dG = (BITS(GO >> 1, 0, 1) << 1) | BIT(BO >> 2, 5);

   if (!((G + kSigned3bit[dG] >= 0) && (G + kSigned3bit[dG] <= 31)))
	 G |= 1 << 4;

   // BO_6[3..4]
   B = BITS(BO >> 2, 3, 4);
   // BO_6[1..2]
   dB = BITS(BO >> 2, 1, 2);

   // B + dB must be outside the range.
   for (int Bx = 0; Bx < 8; Bx++)
	 for (int dBx = 0; dBx < 2; dBx++)
	   {
		  int Btry = B | (Bx << 2);
		  int dBtry = dB | (dBx << 2);
		  if ((Btry + kSigned3bit[dBtry]) < 0 || (Btry + kSigned3bit[dBtry] > 31))
			{
			   B = Btry;
			   dB = dBtry;
			   break;
			}
	   }

   if (!((R + kSigned3bit[dR] >= 0) && (R + kSigned3bit[dR] <= 31)))
	 return false;

   if (!((G + kSigned3bit[dG] >= 0) && (G + kSigned3bit[dG] <= 31)))
	 return false;

   if ((B + kSigned3bit[dB] >= 0) && (B + kSigned3bit[dB] <= 31))
	 return false;

   // Write everything
   etc2[0] = (R << 3) | dR;
   etc2[1] = (G << 3) | dG;
   etc2[2] = (B << 3) | dB;
   etc2[3] = (BIT(BO >> 2, 0) << 7) | (BITS(RH >> 2, 1, 5) << 2) | 0x2 | BIT(RH >> 2, 0);
   etc2[4] = ((GH >> 1) << 1) | BIT(BH >> 2, 5);
   etc2[5] = (BITS(BH >> 2, 0, 4) << 3) | BITS(RV >> 2, 3, 5);
   etc2[6] = (BITS(RV >> 2, 0, 2) << 5) | BITS(GV >> 1, 2, 6);
   etc2[7] = (BITS(GV >> 1, 0, 1) << 6) | (BV >> 2);

   return true;
}

static unsigned int
_etc2_planar_mode_block_pack(uint8_t *etc2,
							 uint32_t Ocol, uint32_t Hcol, uint32_t Vcol,
							 const uint32_t *rgba, bool write)
{	
	uint32_t RO = R_VAL(Ocol);
	uint32_t RH = R_VAL(Hcol);
	uint32_t RV = R_VAL(Vcol);
	uint32_t GO = G_VAL(Ocol);
	uint32_t GH = G_VAL(Hcol);
	uint32_t GV = G_VAL(Vcol);
	uint32_t BO = B_VAL(Ocol);
	uint32_t BH = B_VAL(Hcol);
	uint32_t BV = B_VAL(Vcol);

   if (write)
	 {
		if (!_etc2_planar_mode_header_pack(etc2, RO, RH, RV, GO, GH, GV, BO, BH, BV))
		  return std::numeric_limits<int>::max();
	 }

   // Compute MSE, that's all we need to do

	unsigned int err = 0;
	for (int y = 0; y < 4; y++)
		for (int x = 0; x < 4; x++)
		{
			const int R = clampi(((x * (RH - RO)) + y * (RV - RO) + 4 * RO + 2) >> 2);
			const int G = clampi(((x * (GH - GO)) + y * (GV - GO) + 4 * GO + 2) >> 2);
			const int B = clampi(((x * (BH - BO)) + y * (BV - BO) + 4 * BO + 2) >> 2);
			uint32_t color = RGBA(R, G, B, 255);

			err += _rgb_distance_euclid(color, rgba[x + y * 4]);
		}

	return err;
}

static unsigned int
_etc2_planar_mode_block_encode(uint8_t *etc2, const uint32_t* rgba,
							   const rg_etc1::etc1_pack_params& params)
{
   unsigned int err;
   unsigned int Ocol, Hcol, Vcol, RO, GO, BO, Rx, Gx, Bx;

   // TODO: Scan a broader range to avoid artifacts when the
   // points O, H or V are exceptions

   /* O is at (0,0)
	* H is at (4,0)
	* V is at (0,4)
	* So, H and V are outside the block.
	* We extrapolate the values from (0,3) and (3,0).
	*/

	RO = R_VAL((rgba[0]));
	GO = G_VAL((rgba[0]));
	BO = B_VAL((rgba[0]));
	Ocol = _color_reduce_676(rgba[0]);

	Rx = clampi(RO + (4 * (R_VAL((rgba[3])) - RO)) / 3);
	Gx = clampi(GO + (4 * (G_VAL((rgba[3])) - GO)) / 3);
	Bx = clampi(BO + (4 * (B_VAL((rgba[3])) - BO)) / 3);
	Hcol = _color_reduce_676(RGBA(Rx, Gx, Bx, 0xFF));

	Rx = clampi(RO + (4 * (R_VAL((rgba[12])) - RO)) / 3);
	Gx = clampi(GO + (4 * (G_VAL((rgba[12])) - GO)) / 3);
	Bx = clampi(BO + (4 * (B_VAL((rgba[12])) - BO)) / 3);
	Vcol = _color_reduce_676(RGBA(Rx, Gx, Bx, 0xFF));

	err = _etc2_planar_mode_block_pack(etc2, Ocol, Hcol, Vcol, rgba, true);

	return err;
}

#ifdef DEBUG
static unsigned int
_block_error_calc(const uint32_t *enc, const uint32_t *orig, bool perceptual)
{
   unsigned int errAcc = 0;

   for (int k = 0; k < 16; k++)
	 {
		if (perceptual)
		  errAcc += _rgb_distance_percept(enc[k], orig[k]);
		else
		  errAcc += _rgb_distance_euclid(enc[k], orig[k]);
	 }

   return errAcc;
}
#endif

unsigned int
etc2_rgba8_block_pack(unsigned char *etc2, const unsigned int* rgba, rg_etc1::etc1_pack_params params)
{
   unsigned int errors[3] = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
   unsigned int minErr = std::numeric_limits<int>::max();
   uint8_t etc2_try[3][8];
   int bestSolution = 0;

#ifdef DEBUG
   static int cnt [3] = {0};
#endif

   errors[0] = rg_etc1::pack_etc1_block(etc2_try[0], rgba, params);
   errors[1] = _etc2_th_mode_block_encode(etc2_try[1], rgba, params);
   errors[2] = _etc2_planar_mode_block_encode(etc2_try[2], rgba, params);

#ifdef DEBUG
   for (int i = 1; i < 3; i++)
	 {
		const char *mode = (i == 1) ? "T or H" : "Planar";
		if (errors[i] < std::numeric_limits<int>::max())
		  for (unsigned k = 0; k < sizeof(errors) / sizeof(*errors); k++)
			{
			   uint32_t decoded[16];
			   unsigned int real_errors[2];
			   rg_etc2_rgb8_decode_block(etc2_try[i], decoded);
			   real_errors[0] = _block_error_calc(decoded, rgba, false);
			   real_errors[1] = _block_error_calc(decoded, rgba, true);

			   if (real_errors[0] != errors[i])
				 DBG("Invalid error calc in %s mode", mode);
			}
	 }
#endif

   for (unsigned k = 0; k < sizeof(errors) / sizeof(*errors); k++)
	 if (errors[k] < minErr)
	   {
		  minErr = errors[k];
		  bestSolution = k;
	   }

   memcpy(etc2 + 8, etc2_try[bestSolution], 8);

   minErr += _etc2_alpha_encode(etc2, rgba, params);

#ifdef DEBUG
   cnt[bestSolution]++;
   DBG("Block count by mode: ETC1: %d T/H: %d Planar: %d. Err %d",
	   cnt[0], cnt[1], cnt[2], minErr);
#endif

   return minErr;
}

unsigned int
etc2_rgb8_block_pack(unsigned char *etc2, const unsigned int *rgba,
					 rg_etc1::etc1_pack_params params)
{
  unsigned int errors[3] = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
  unsigned int minErr = std::numeric_limits<int>::max();
  uint8_t etc2_try[3][8];
  int bestSolution = 0;

  errors[0] = rg_etc1::pack_etc1_block(etc2_try[0], rgba, params);
  errors[1] = _etc2_th_mode_block_encode(etc2_try[1], rgba, params);
  errors[2] = _etc2_planar_mode_block_encode(etc2_try[2], rgba, params);

  for (unsigned k = 0; k < sizeof(errors) / sizeof(*errors); k++)
	if (errors[k] < minErr)
	  {
		 minErr = errors[k];
		 bestSolution = k;
	  }

  memcpy(etc2, etc2_try[bestSolution], 8);

  return minErr;
}
