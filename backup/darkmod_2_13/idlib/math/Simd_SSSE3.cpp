/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include "Simd_SSSE3.h"

idSIMD_SSSE3::idSIMD_SSSE3() {
	name = "SSSE3";
}

#ifdef ENABLE_SSE_PROCESSORS

#include "tmmintrin.h"

//in "Debug with Inlines" config, optimize all the remaining functions of this file
DEBUG_OPTIMIZE_ON

#define SHUF(i0, i1, i2, i3) _MM_SHUFFLE(i3, i2, i1, i0)

/*
============
idSIMD_SSSE3::ConvertRowToRGBA8
============
*/
bool idSIMD_SSSE3::ConvertRowToRGBA8( const byte *srcPtr, int width, int bitsPerPixel, bool bgr, byte *dstPtr ) {
	if (bitsPerPixel == 8) {
		int i;
		for (i = 0; i + 16 <= width; i += 16) {
			__m128i data = _mm_loadu_si128((__m128i*)(srcPtr + i));
			__m128i left = _mm_unpacklo_epi8(data, data);
			__m128i rigt = _mm_unpackhi_epi8(data, data);
			__m128i q0 = _mm_or_si128(_mm_unpacklo_epi8(left, left), _mm_set1_epi32(0xFF000000));
			__m128i q1 = _mm_or_si128(_mm_unpackhi_epi8(left, left), _mm_set1_epi32(0xFF000000));
			__m128i q2 = _mm_or_si128(_mm_unpacklo_epi8(rigt, rigt), _mm_set1_epi32(0xFF000000));
			__m128i q3 = _mm_or_si128(_mm_unpackhi_epi8(rigt, rigt), _mm_set1_epi32(0xFF000000));
			_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 0), q0);
			_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 16), q1);
			_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 32), q2);
			_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 48), q3);
		}
		for (; i < width; i++) {
			byte val = srcPtr[i];
			dstPtr[4*i+0] = val;
			dstPtr[4*i+1] = val;
			dstPtr[4*i+2] = val;
			dstPtr[4*i+3] = 255;
		}
		return true;
	}

	if (bitsPerPixel == 32) {
		if (bgr) {
			int i;
			for (i = 0; i + 16 <= width; i += 16) {
				__m128i data0 = _mm_loadu_si128((__m128i*)(srcPtr + 4*i + 0));
				__m128i data1 = _mm_loadu_si128((__m128i*)(srcPtr + 4*i + 16));
				__m128i data2 = _mm_loadu_si128((__m128i*)(srcPtr + 4*i + 32));
				__m128i data3 = _mm_loadu_si128((__m128i*)(srcPtr + 4*i + 48));
				__m128i shuf = _mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15);
				data0 = _mm_shuffle_epi8(data0, shuf);
				data1 = _mm_shuffle_epi8(data1, shuf);
				data2 = _mm_shuffle_epi8(data2, shuf);
				data3 = _mm_shuffle_epi8(data3, shuf);
				_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 0), data0);
				_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 16), data1);
				_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 32), data2);
				_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 48), data3);
			}
			for (; i < width; i++) {
				dstPtr[4*i+0] = srcPtr[4*i+2];
				dstPtr[4*i+1] = srcPtr[4*i+1];
				dstPtr[4*i+2] = srcPtr[4*i+0];
				dstPtr[4*i+3] = srcPtr[4*i+3];
			}
		}
		else {
			Memcpy(dstPtr, srcPtr, width * 4);
		}
		return true;
	}

	if (bitsPerPixel == 24) {
		__m128i shuf;
		if (bgr)
			shuf = _mm_setr_epi8(2, 1, 0, -1, 5, 4, 3, -1, 8, 7, 6, -1, 11, 10, 9, -1);
		else
			shuf = _mm_setr_epi8(0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1);
		int i;
		for (i = 0; i + 16 <= width; i += 16) {
			__m128i data0 = _mm_loadu_si128((__m128i*)(srcPtr + 3*i + 0));
			__m128i data1 = _mm_loadu_si128((__m128i*)(srcPtr + 3*i + 16));
			__m128i data2 = _mm_loadu_si128((__m128i*)(srcPtr + 3*i + 32));
			__m128i out0 = _mm_shuffle_epi8(data0, shuf);
			__m128i out1 = _mm_shuffle_epi8(_mm_alignr_epi8(data1, data0, 12), shuf);
			__m128i out2 = _mm_shuffle_epi8(_mm_alignr_epi8(data2, data1, 8), shuf);
			__m128i out3 = _mm_shuffle_epi8(_mm_bsrli_si128(data2, 4), shuf);
			out0 = _mm_xor_si128(out0, _mm_set1_epi32(0xFF000000));
			out1 = _mm_xor_si128(out1, _mm_set1_epi32(0xFF000000));
			out2 = _mm_xor_si128(out2, _mm_set1_epi32(0xFF000000));
			out3 = _mm_xor_si128(out3, _mm_set1_epi32(0xFF000000));
			_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 0), out0);
			_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 16), out1);
			_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 32), out2);
			_mm_storeu_si128((__m128i*)(dstPtr + 4*i + 48), out3);
		}
		for (; i < width; i++) {
			if (bgr) {
				dstPtr[4*i+0] = srcPtr[3*i+2];
				dstPtr[4*i+1] = srcPtr[3*i+1];
				dstPtr[4*i+2] = srcPtr[3*i+0];
			}
			else {
				dstPtr[4*i+0] = srcPtr[3*i+0];
				dstPtr[4*i+1] = srcPtr[3*i+1];
				dstPtr[4*i+2] = srcPtr[3*i+2];
			}
			dstPtr[4*i+3] = 255;
		}
		return true;
	}

	return false;
}

#endif
