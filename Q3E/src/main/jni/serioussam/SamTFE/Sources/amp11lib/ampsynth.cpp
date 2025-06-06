/* Copyright (c) 1997-2001 Niklas Beisert, Alen Ladavac. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

// amp11 - an Audio-MPEG decoder - synthesizer
// some ideas regarding optimization of the subband synthesis
// were taken from mpg123 by Michael Hipp

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ampdec.h"

#define _PI 3.14159265358979323846
#define BUFFEROFFSET (16*17)

float ampegdecoder::dwin[1024];
float ampegdecoder::dwin2[512];
float ampegdecoder::dwin4[256];
float ampegdecoder::sectab[32];

#ifdef __WATCOMC__
#define MULADDINLINE
float __muladd16a(float *a, float *b);
#pragma aux __muladd16a parm [ebx] [ecx] value [8087] modify exact [8087] = \
  "fld  dword ptr [ebx+ 0]" \
  "fmul dword ptr [ecx+ 0]" \
  "fld  dword ptr [ebx+ 4]" \
  "fmul dword ptr [ecx+ 4]" \
  "fxch st(1)" \
  "fld  dword ptr [ebx+ 8]" \
  "fmul dword ptr [ecx+ 8]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+12]" \
  "fmul dword ptr [ecx+12]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+16]" \
  "fmul dword ptr [ecx+16]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+20]" \
  "fmul dword ptr [ecx+20]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+24]" \
  "fmul dword ptr [ecx+24]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+28]" \
  "fmul dword ptr [ecx+28]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+32]" \
  "fmul dword ptr [ecx+32]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+36]" \
  "fmul dword ptr [ecx+36]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+40]" \
  "fmul dword ptr [ecx+40]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+44]" \
  "fmul dword ptr [ecx+44]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+48]" \
  "fmul dword ptr [ecx+48]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+52]" \
  "fmul dword ptr [ecx+52]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+56]" \
  "fmul dword ptr [ecx+56]" \
  "fxch st(2)" \
  "fsubp st(1),st" \
  "fld  dword ptr [ebx+60]" \
  "fmul dword ptr [ecx+60]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fsubrp st(1),st"

float __muladd16b(float *a, float *b);
#pragma aux __muladd16b parm [ebx] [ecx] value [8087] modify exact [8087] = \
  "fld  dword ptr [ebx+60]" \
  "fmul dword ptr [ecx+ 0]" \
  "fld  dword ptr [ebx+56]" \
  "fmul dword ptr [ecx+ 4]" \
  "fxch st(1)" \
  "fld  dword ptr [ebx+52]" \
  "fmul dword ptr [ecx+ 8]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+48]" \
  "fmul dword ptr [ecx+12]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+44]" \
  "fmul dword ptr [ecx+16]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+40]" \
  "fmul dword ptr [ecx+20]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+36]" \
  "fmul dword ptr [ecx+24]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+32]" \
  "fmul dword ptr [ecx+28]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+28]" \
  "fmul dword ptr [ecx+32]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+24]" \
  "fmul dword ptr [ecx+36]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+20]" \
  "fmul dword ptr [ecx+40]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+16]" \
  "fmul dword ptr [ecx+44]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+12]" \
  "fmul dword ptr [ecx+48]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+ 8]" \
  "fmul dword ptr [ecx+52]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+ 4]" \
  "fmul dword ptr [ecx+56]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "fld  dword ptr [ebx+ 0]" \
  "fmul dword ptr [ecx+60]" \
  "fxch st(2)" \
  "faddp st(1),st" \
  "faddp st(1),st" \
  "fchs"
#endif

#ifdef GNUCI486
#define MULADDINLINE
static inline float __muladd16a(float *a, float *b)
{
  float res;
  asm
  (
    "flds   0(%1)\n"
    "fmuls  0(%2)\n"
    "flds   4(%1)\n"
    "fmuls  4(%2)\n"
    "fxch %%st(1)\n"
    "flds   8(%1)\n"
    "fmuls  8(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  12(%1)\n"
    "fmuls 12(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  16(%1)\n"
    "fmuls 16(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  20(%1)\n"
    "fmuls 20(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  24(%1)\n"
    "fmuls 24(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  28(%1)\n"
    "fmuls 28(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  32(%1)\n"
    "fmuls 32(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  36(%1)\n"
    "fmuls 36(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  40(%1)\n"
    "fmuls 40(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  44(%1)\n"
    "fmuls 44(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  48(%1)\n"
    "fmuls 48(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  52(%1)\n"
    "fmuls 52(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  56(%1)\n"
    "fmuls 56(%2)\n"
    "fxch %%st(2)\n"
    "fsubrp %%st,%%st(1)\n"
    "flds  60(%1)\n"
    "fmuls 60(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "fsubp %%st,%%st(1)"
    : "=st"(res) : "r"(a),"r"(b) : "st(7)","st(6)","st(5)"
  );
  return res;
}

static inline float __muladd16b(float *a, float *b)
{
  float res;
  asm
  (
    "flds  60(%1)\n"
    "fmuls  0(%2)\n"
    "flds  56(%1)\n"
    "fmuls  4(%2)\n"
    "fxch %%st(1)\n"
    "flds  52(%1)\n"
    "fmuls  8(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  48(%1)\n"
    "fmuls 12(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  44(%1)\n"
    "fmuls 16(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  40(%1)\n"
    "fmuls 20(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  36(%1)\n"
    "fmuls 24(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  32(%1)\n"
    "fmuls 28(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  28(%1)\n"
    "fmuls 32(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  24(%1)\n"
    "fmuls 36(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  20(%1)\n"
    "fmuls 40(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  16(%1)\n"
    "fmuls 44(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds  12(%1)\n"
    "fmuls 48(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds   8(%1)\n"
    "fmuls 52(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds   4(%1)\n"
    "fmuls 56(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "flds   0(%1)\n"
    "fmuls 60(%2)\n"
    "fxch %%st(2)\n"
    "faddp %%st,%%st(1)\n"
    "faddp %%st,%%st(1)\n"
    "fchs"
    : "=st"(res) : "r"(a),"r"(b) : "st(7)","st(6)","st(5)"
  );
  return res;
}
#endif

float ampegdecoder::muladd16a(float *a, float *b)
{
#ifdef MULADDINLINE
  return __muladd16a(a,b);
#else
  return +a[ 0]*b[ 0]-a[ 1]*b[ 1]+a[ 2]*b[ 2]-a[ 3]*b[ 3]
         +a[ 4]*b[ 4]-a[ 5]*b[ 5]+a[ 6]*b[ 6]-a[ 7]*b[ 7]
         +a[ 8]*b[ 8]-a[ 9]*b[ 9]+a[10]*b[10]-a[11]*b[11]
         +a[12]*b[12]-a[13]*b[13]+a[14]*b[14]-a[15]*b[15];
#endif
}

float ampegdecoder::muladd16b(float *a, float *b)
{
#ifdef MULADDINLINE
  return __muladd16b(a,b);
#else
  return -a[15]*b[ 0]-a[14]*b[ 1]-a[13]*b[ 2]-a[12]*b[ 3]
         -a[11]*b[ 4]-a[10]*b[ 5]-a[ 9]*b[ 6]-a[ 8]*b[ 7]
         -a[ 7]*b[ 8]-a[ 6]*b[ 9]-a[ 5]*b[10]-a[ 4]*b[11]
         -a[ 3]*b[12]-a[ 2]*b[13]-a[ 1]*b[14]-a[ 0]*b[15];
#endif
}

#ifndef FDCTBEXT
void ampegdecoder::fdctb8(float *out1, float *out2, float *in)
{
  float p1[8];
  float p2[8];

  p1[ 0] =               in[ 0] + in[ 7];
  p1[ 7] = sectab[ 4] * (in[ 0] - in[ 7]);
  p1[ 1] =               in[ 1] + in[ 6];
  p1[ 6] = sectab[ 5] * (in[ 1] - in[ 6]);
  p1[ 2] =               in[ 2] + in[ 5];
  p1[ 5] = sectab[ 6] * (in[ 2] - in[ 5]);
  p1[ 3] =               in[ 3] + in[ 4];
  p1[ 4] = sectab[ 7] * (in[ 3] - in[ 4]);

  p2[ 0] =               p1[ 0] + p1[ 3];
  p2[ 3] = sectab[ 2] * (p1[ 0] - p1[ 3]);
  p2[ 1] =               p1[ 1] + p1[ 2];
  p2[ 2] = sectab[ 3] * (p1[ 1] - p1[ 2]);
  p2[ 4] =               p1[ 4] + p1[ 7];
  p2[ 7] =-sectab[ 2] * (p1[ 4] - p1[ 7]);
  p2[ 5] =               p1[ 5] + p1[ 6];
  p2[ 6] =-sectab[ 3] * (p1[ 5] - p1[ 6]);

  p1[ 0] =               p2[ 0] + p2[ 1];
  p1[ 1] = sectab[ 1] * (p2[ 0] - p2[ 1]);
  p1[ 2] =               p2[ 2] + p2[ 3];
  p1[ 3] =-sectab[ 1] * (p2[ 2] - p2[ 3]);
  p1[ 2] += p1[ 3];
  p1[ 4] =               p2[ 4] + p2[ 5];
  p1[ 5] = sectab[ 1] * (p2[ 4] - p2[ 5]);
  p1[ 6] =               p2[ 6] + p2[ 7];
  p1[ 7] =-sectab[ 1] * (p2[ 6] - p2[ 7]);
  p1[ 6] += p1[ 7];
  p1[ 4] += p1[ 6];
  p1[ 6] += p1[ 5];
  p1[ 5] += p1[ 7];

  out1[16* 0]=p1[ 1];
  out1[16* 1]=p1[ 6];
  out1[16* 2]=p1[ 2];
  out1[16* 3]=p1[ 4];
  out1[16* 4]=p1[ 0];
  out2[16* 0]=p1[ 1];
  out2[16* 1]=p1[ 5];
  out2[16* 2]=p1[ 3];
  out2[16* 3]=p1[ 7];
}

void ampegdecoder::fdctb16(float *out1, float *out2, float *in)
{
  float p1[16];
  float p2[16];

  p2[ 0] =               in[ 0] + in[15];
  p2[15] = sectab[ 8] * (in[ 0] - in[15]);
  p2[ 1] =               in[ 1] + in[14];
  p2[14] = sectab[ 9] * (in[ 1] - in[14]);
  p2[ 2] =               in[ 2] + in[13];
  p2[13] = sectab[10] * (in[ 2] - in[13]);
  p2[ 3] =               in[ 3] + in[12];
  p2[12] = sectab[11] * (in[ 3] - in[12]);
  p2[ 4] =               in[ 4] + in[11];
  p2[11] = sectab[12] * (in[ 4] - in[11]);
  p2[ 5] =               in[ 5] + in[10];
  p2[10] = sectab[13] * (in[ 5] - in[10]);
  p2[ 6] =               in[ 6] + in[ 9];
  p2[ 9] = sectab[14] * (in[ 6] - in[ 9]);
  p2[ 7] =               in[ 7] + in[ 8];
  p2[ 8] = sectab[15] * (in[ 7] - in[ 8]);

  p1[ 0] =               p2[ 0] + p2[ 7];
  p1[ 7] = sectab[ 4] * (p2[ 0] - p2[ 7]);
  p1[ 1] =               p2[ 1] + p2[ 6];
  p1[ 6] = sectab[ 5] * (p2[ 1] - p2[ 6]);
  p1[ 2] =               p2[ 2] + p2[ 5];
  p1[ 5] = sectab[ 6] * (p2[ 2] - p2[ 5]);
  p1[ 3] =               p2[ 3] + p2[ 4];
  p1[ 4] = sectab[ 7] * (p2[ 3] - p2[ 4]);
  p1[ 8] =               p2[ 8] + p2[15];
  p1[15] =-sectab[ 4] * (p2[ 8] - p2[15]);
  p1[ 9] =               p2[ 9] + p2[14];
  p1[14] =-sectab[ 5] * (p2[ 9] - p2[14]);
  p1[10] =               p2[10] + p2[13];
  p1[13] =-sectab[ 6] * (p2[10] - p2[13]);
  p1[11] =               p2[11] + p2[12];
  p1[12] =-sectab[ 7] * (p2[11] - p2[12]);

  p2[ 0] =               p1[ 0] + p1[ 3];
  p2[ 3] = sectab[ 2] * (p1[ 0] - p1[ 3]);
  p2[ 1] =               p1[ 1] + p1[ 2];
  p2[ 2] = sectab[ 3] * (p1[ 1] - p1[ 2]);
  p2[ 4] =               p1[ 4] + p1[ 7];
  p2[ 7] =-sectab[ 2] * (p1[ 4] - p1[ 7]);
  p2[ 5] =               p1[ 5] + p1[ 6];
  p2[ 6] =-sectab[ 3] * (p1[ 5] - p1[ 6]);
  p2[ 8] =               p1[ 8] + p1[11];
  p2[11] = sectab[ 2] * (p1[ 8] - p1[11]);
  p2[ 9] =               p1[ 9] + p1[10];
  p2[10] = sectab[ 3] * (p1[ 9] - p1[10]);
  p2[12] =               p1[12] + p1[15];
  p2[15] =-sectab[ 2] * (p1[12] - p1[15]);
  p2[13] =               p1[13] + p1[14];
  p2[14] =-sectab[ 3] * (p1[13] - p1[14]);

  p1[ 0] =               p2[ 0] + p2[ 1];
  p1[ 1] = sectab[ 1] * (p2[ 0] - p2[ 1]);
  p1[ 2] =               p2[ 2] + p2[ 3];
  p1[ 3] =-sectab[ 1] * (p2[ 2] - p2[ 3]);
  p1[ 2] += p1[ 3];
  p1[ 4] =               p2[ 4] + p2[ 5];
  p1[ 5] = sectab[ 1] * (p2[ 4] - p2[ 5]);
  p1[ 6] =               p2[ 6] + p2[ 7];
  p1[ 7] =-sectab[ 1] * (p2[ 6] - p2[ 7]);
  p1[ 6] += p1[ 7];
  p1[ 4] += p1[ 6];
  p1[ 6] += p1[ 5];
  p1[ 5] += p1[ 7];
  p1[ 8] =               p2[ 8] + p2[ 9];
  p1[ 9] = sectab[ 1] * (p2[ 8] - p2[ 9]);
  p1[10] =               p2[10] + p2[11];
  p1[11] =-sectab[ 1] * (p2[10] - p2[11]);
  p1[10] += p1[11];
  p1[12] =               p2[12] + p2[13];
  p1[13] = sectab[ 1] * (p2[12] - p2[13]);
  p1[14] =               p2[14] + p2[15];
  p1[15] =-sectab[ 1] * (p2[14] - p2[15]);
  p1[14] += p1[15];
  p1[12] += p1[14];
  p1[14] += p1[13];
  p1[13] += p1[15];

  out1[16* 0]=p1[ 1];
  out1[16* 1]=p1[14]+p1[ 9];
  out1[16* 2]=p1[ 6];
  out1[16* 3]=p1[10]+p1[14];
  out1[16* 4]=p1[ 2];
  out1[16* 5]=p1[12]+p1[10];
  out1[16* 6]=p1[ 4];
  out1[16* 7]=p1[ 8]+p1[12];
  out1[16* 8]=p1[ 0];
  out2[16* 0]=p1[ 1];
  out2[16* 1]=p1[ 9]+p1[13];
  out2[16* 2]=p1[ 5];
  out2[16* 3]=p1[13]+p1[11];
  out2[16* 4]=p1[ 3];
  out2[16* 5]=p1[11]+p1[15];
  out2[16* 6]=p1[ 7];
  out2[16* 7]=p1[15];
}

void ampegdecoder::fdctb32(float *out1, float *out2, float *in)
{
  float p1[32];
  float p2[32];
  p1[ 0] =               in[0]  + in[31];
  p1[31] = sectab[16] * (in[0]  - in[31]);
  p1[ 1] =               in[1]  + in[30];
  p1[30] = sectab[17] * (in[1]  - in[30]);
  p1[ 2] =               in[2]  + in[29];
  p1[29] = sectab[18] * (in[2]  - in[29]);
  p1[ 3] =               in[3]  + in[28];
  p1[28] = sectab[19] * (in[3]  - in[28]);
  p1[ 4] =               in[4]  + in[27];
  p1[27] = sectab[20] * (in[4]  - in[27]);
  p1[ 5] =               in[5]  + in[26];
  p1[26] = sectab[21] * (in[5]  - in[26]);
  p1[ 6] =               in[6]  + in[25];
  p1[25] = sectab[22] * (in[6]  - in[25]);
  p1[ 7] =               in[7]  + in[24];
  p1[24] = sectab[23] * (in[7]  - in[24]);
  p1[ 8] =               in[8]  + in[23];
  p1[23] = sectab[24] * (in[8]  - in[23]);
  p1[ 9] =               in[9]  + in[22];
  p1[22] = sectab[25] * (in[9]  - in[22]);
  p1[10] =               in[10] + in[21];
  p1[21] = sectab[26] * (in[10] - in[21]);
  p1[11] =               in[11] + in[20];
  p1[20] = sectab[27] * (in[11] - in[20]);
  p1[12] =               in[12] + in[19];
  p1[19] = sectab[28] * (in[12] - in[19]);
  p1[13] =               in[13] + in[18];
  p1[18] = sectab[29] * (in[13] - in[18]);
  p1[14] =               in[14] + in[17];
  p1[17] = sectab[30] * (in[14] - in[17]);
  p1[15] =               in[15] + in[16];
  p1[16] = sectab[31] * (in[15] - in[16]);

  p2[ 0] =               p1[ 0] + p1[15];
  p2[15] = sectab[ 8] * (p1[ 0] - p1[15]);
  p2[ 1] =               p1[ 1] + p1[14];
  p2[14] = sectab[ 9] * (p1[ 1] - p1[14]);
  p2[ 2] =               p1[ 2] + p1[13];
  p2[13] = sectab[10] * (p1[ 2] - p1[13]);
  p2[ 3] =               p1[ 3] + p1[12];
  p2[12] = sectab[11] * (p1[ 3] - p1[12]);
  p2[ 4] =               p1[ 4] + p1[11];
  p2[11] = sectab[12] * (p1[ 4] - p1[11]);
  p2[ 5] =               p1[ 5] + p1[10];
  p2[10] = sectab[13] * (p1[ 5] - p1[10]);
  p2[ 6] =               p1[ 6] + p1[ 9];
  p2[ 9] = sectab[14] * (p1[ 6] - p1[ 9]);
  p2[ 7] =               p1[ 7] + p1[ 8];
  p2[ 8] = sectab[15] * (p1[ 7] - p1[ 8]);
  p2[16] =               p1[16] + p1[31];
  p2[31] =-sectab[ 8] * (p1[16] - p1[31]);
  p2[17] =               p1[17] + p1[30];
  p2[30] =-sectab[ 9] * (p1[17] - p1[30]);
  p2[18] =               p1[18] + p1[29];
  p2[29] =-sectab[10] * (p1[18] - p1[29]);
  p2[19] =               p1[19] + p1[28];
  p2[28] =-sectab[11] * (p1[19] - p1[28]);
  p2[20] =               p1[20] + p1[27];
  p2[27] =-sectab[12] * (p1[20] - p1[27]);
  p2[21] =               p1[21] + p1[26];
  p2[26] =-sectab[13] * (p1[21] - p1[26]);
  p2[22] =               p1[22] + p1[25];
  p2[25] =-sectab[14] * (p1[22] - p1[25]);
  p2[23] =               p1[23] + p1[24];
  p2[24] =-sectab[15] * (p1[23] - p1[24]);

  p1[ 0] =               p2[ 0] + p2[ 7];
  p1[ 7] = sectab[ 4] * (p2[ 0] - p2[ 7]);
  p1[ 1] =               p2[ 1] + p2[ 6];
  p1[ 6] = sectab[ 5] * (p2[ 1] - p2[ 6]);
  p1[ 2] =               p2[ 2] + p2[ 5];
  p1[ 5] = sectab[ 6] * (p2[ 2] - p2[ 5]);
  p1[ 3] =               p2[ 3] + p2[ 4];
  p1[ 4] = sectab[ 7] * (p2[ 3] - p2[ 4]);
  p1[ 8] =               p2[ 8] + p2[15];
  p1[15] =-sectab[ 4] * (p2[ 8] - p2[15]);
  p1[ 9] =               p2[ 9] + p2[14];
  p1[14] =-sectab[ 5] * (p2[ 9] - p2[14]);
  p1[10] =               p2[10] + p2[13];
  p1[13] =-sectab[ 6] * (p2[10] - p2[13]);
  p1[11] =               p2[11] + p2[12];
  p1[12] =-sectab[ 7] * (p2[11] - p2[12]);
  p1[16] =               p2[16] + p2[23];
  p1[23] = sectab[ 4] * (p2[16] - p2[23]);
  p1[17] =               p2[17] + p2[22];
  p1[22] = sectab[ 5] * (p2[17] - p2[22]);
  p1[18] =               p2[18] + p2[21];
  p1[21] = sectab[ 6] * (p2[18] - p2[21]);
  p1[19] =               p2[19] + p2[20];
  p1[20] = sectab[ 7] * (p2[19] - p2[20]);
  p1[24] =               p2[24] + p2[31];
  p1[31] =-sectab[ 4] * (p2[24] - p2[31]);
  p1[25] =               p2[25] + p2[30];
  p1[30] =-sectab[ 5] * (p2[25] - p2[30]);
  p1[26] =               p2[26] + p2[29];
  p1[29] =-sectab[ 6] * (p2[26] - p2[29]);
  p1[27] =               p2[27] + p2[28];
  p1[28] =-sectab[ 7] * (p2[27] - p2[28]);

  p2[ 0] =               p1[ 0] + p1[ 3];
  p2[ 3] = sectab[ 2] * (p1[ 0] - p1[ 3]);
  p2[ 1] =               p1[ 1] + p1[ 2];
  p2[ 2] = sectab[ 3] * (p1[ 1] - p1[ 2]);
  p2[ 4] =               p1[ 4] + p1[ 7];
  p2[ 7] =-sectab[ 2] * (p1[ 4] - p1[ 7]);
  p2[ 5] =               p1[ 5] + p1[ 6];
  p2[ 6] =-sectab[ 3] * (p1[ 5] - p1[ 6]);
  p2[ 8] =               p1[ 8] + p1[11];
  p2[11] = sectab[ 2] * (p1[ 8] - p1[11]);
  p2[ 9] =               p1[ 9] + p1[10];
  p2[10] = sectab[ 3] * (p1[ 9] - p1[10]);
  p2[12] =               p1[12] + p1[15];
  p2[15] =-sectab[ 2] * (p1[12] - p1[15]);
  p2[13] =               p1[13] + p1[14];
  p2[14] =-sectab[ 3] * (p1[13] - p1[14]);
  p2[16] =               p1[16] + p1[19];
  p2[19] = sectab[ 2] * (p1[16] - p1[19]);
  p2[17] =               p1[17] + p1[18];
  p2[18] = sectab[ 3] * (p1[17] - p1[18]);
  p2[20] =               p1[20] + p1[23];
  p2[23] =-sectab[ 2] * (p1[20] - p1[23]);
  p2[21] =               p1[21] + p1[22];
  p2[22] =-sectab[ 3] * (p1[21] - p1[22]);
  p2[24] =               p1[24] + p1[27];
  p2[27] = sectab[ 2] * (p1[24] - p1[27]);
  p2[25] =               p1[25] + p1[26];
  p2[26] = sectab[ 3] * (p1[25] - p1[26]);
  p2[28] =               p1[28] + p1[31];
  p2[31] =-sectab[ 2] * (p1[28] - p1[31]);
  p2[29] =               p1[29] + p1[30];
  p2[30] =-sectab[ 3] * (p1[29] - p1[30]);

  p1[ 0] =               p2[ 0] + p2[ 1];
  p1[ 1] = sectab[ 1] * (p2[ 0] - p2[ 1]);
  p1[ 2] =               p2[ 2] + p2[ 3];
  p1[ 3] =-sectab[ 1] * (p2[ 2] - p2[ 3]);
  p1[ 2] += p1[ 3];
  p1[ 4] =               p2[ 4] + p2[ 5];
  p1[ 5] = sectab[ 1] * (p2[ 4] - p2[ 5]);
  p1[ 6] =               p2[ 6] + p2[ 7];
  p1[ 7] =-sectab[ 1] * (p2[ 6] - p2[ 7]);
  p1[ 6] += p1[ 7];
  p1[ 4] += p1[ 6];
  p1[ 6] += p1[ 5];
  p1[ 5] += p1[ 7];
  p1[ 8] =               p2[ 8] + p2[ 9];
  p1[ 9] = sectab[ 1] * (p2[ 8] - p2[ 9]);
  p1[10] =               p2[10] + p2[11];
  p1[11] =-sectab[ 1] * (p2[10] - p2[11]);
  p1[10] += p1[11];
  p1[12] =               p2[12] + p2[13];
  p1[13] = sectab[ 1] * (p2[12] - p2[13]);
  p1[14] =               p2[14] + p2[15];
  p1[15] =-sectab[ 1] * (p2[14] - p2[15]);
  p1[14] += p1[15];
  p1[12] += p1[14];
  p1[14] += p1[13];
  p1[13] += p1[15];
  p1[16] =               p2[16] + p2[17];
  p1[17] = sectab[ 1] * (p2[16] - p2[17]);
  p1[18] =               p2[18] + p2[19];
  p1[19] =-sectab[ 1] * (p2[18] - p2[19]);
  p1[18] += p1[19];
  p1[20] =               p2[20] + p2[21];
  p1[21] = sectab[ 1] * (p2[20] - p2[21]);
  p1[22] =               p2[22] + p2[23];
  p1[23] =-sectab[ 1] * (p2[22] - p2[23]);
  p1[22] += p1[23];
  p1[20] += p1[22];
  p1[22] += p1[21];
  p1[21] += p1[23];
  p1[24] =               p2[24] + p2[25];
  p1[25] = sectab[ 1] * (p2[24] - p2[25]);
  p1[26] =               p2[26] + p2[27];
  p1[27] =-sectab[ 1] * (p2[26] - p2[27]);
  p1[26] += p1[27];
  p1[28] =               p2[28] + p2[29];
  p1[29] = sectab[ 1] * (p2[28] - p2[29]);
  p1[30] =               p2[30] + p2[31];
  p1[31] =-sectab[ 1] * (p2[30] - p2[31]);
  p1[30] += p1[31];
  p1[28] += p1[30];
  p1[30] += p1[29];
  p1[29] += p1[31];

  out1[16* 0] = p1[ 1];
  out1[16* 1] = p1[17] + p1[30] + p1[25];
  out1[16* 2] = p1[14] + p1[ 9];
  out1[16* 3] = p1[22] + p1[30] + p1[25];
  out1[16* 4] = p1[ 6];
  out1[16* 5] = p1[22] + p1[26] + p1[30];
  out1[16* 6] = p1[10] + p1[14];
  out1[16* 7] = p1[18] + p1[26] + p1[30];
  out1[16* 8] = p1[ 2];
  out1[16* 9] = p1[18] + p1[28] + p1[26];
  out1[16*10] = p1[12] + p1[10];
  out1[16*11] = p1[20] + p1[28] + p1[26];
  out1[16*12] = p1[ 4];
  out1[16*13] = p1[20] + p1[24] + p1[28];
  out1[16*14] = p1[ 8] + p1[12];
  out1[16*15] = p1[16] + p1[24] + p1[28];
  out1[16*16] = p1[ 0];
  out2[16* 0] = p1[ 1];
  out2[16* 1] = p1[17] + p1[25] + p1[29];
  out2[16* 2] = p1[ 9] + p1[13];
  out2[16* 3] = p1[21] + p1[25] + p1[29];
  out2[16* 4] = p1[ 5];
  out2[16* 5] = p1[21] + p1[29] + p1[27];
  out2[16* 6] = p1[13] + p1[11];
  out2[16* 7] = p1[19] + p1[29] + p1[27];
  out2[16* 8] = p1[ 3];
  out2[16* 9] = p1[19] + p1[27] + p1[31];
  out2[16*10] = p1[11] + p1[15];
  out2[16*11] = p1[23] + p1[27] + p1[31];
  out2[16*12] = p1[ 7];
  out2[16*13] = p1[23] + p1[31];
  out2[16*14] = p1[15];
  out2[16*15] = p1[31];
}
#endif

void ampegdecoder::synth(void *dest, float (*bandsl)[32], float (*bandsr)[32])
{
  int blk;
  for (blk=0; blk<36; blk++)
  {
    int i,j,k;

    int nsmp=32>>ratereduce;
    int nsmp2=16>>ratereduce;

    if (usevoltab)
    {
      if (tomono)
        for (i=0; i<nsmp; i++)
          bandsl[0][i]=bandsl[0][i]*stereotab[2][0]+bandsr[0][i]*stereotab[2][1];
      else
      if (srcchan==2)
        for (i=0; i<nsmp; i++)
        {
          float t=bandsl[0][i];
          bandsl[0][i]=bandsl[0][i]*stereotab[0][0]+bandsr[0][i]*stereotab[0][1];
          bandsr[0][i]=t*stereotab[1][0]+bandsr[0][i]*stereotab[1][1];
        }
      else
        for (i=0; i<nsmp; i++)
          bandsl[0][i]*=stereotab[2][2];
    }
    else
      if (tomono)
        for (i=0; i<nsmp; i++)
          bandsl[0][i]=0.5*(bandsl[0][i]+bandsr[0][i]);
    if (volume!=1)
    {
      for (i=0; i<nsmp; i++)
        bandsl[0][i]*=volume;
      if (!tomono&&(srcchan!=1))
        for (i=0; i<nsmp; i++)
          bandsr[0][i]*=volume;
    }
    if (equalon)
    {
      for (i=0; i<nsmp; i++)
        bandsl[0][i]*=equal[i];
      if (!tomono&&(srcchan!=1))
        for (i=0; i<nsmp; i++)
          bandsr[0][i]*=equal[i];
    }

    for (k=0; k<dctstereo; k++)
    {
      float *out1=synbuf+k*2*BUFFEROFFSET+(synbufoffset&1)*BUFFEROFFSET+((synbufoffset+1)&14);
      float *out2=synbuf+k*2*BUFFEROFFSET+((synbufoffset+1)&1)*BUFFEROFFSET+(synbufoffset|1);

      if (ratereduce==0)
        fdctb32(out1, out2, k?bandsr[0]:bandsl[0]);
      else
      if (ratereduce==1)
        fdctb16(out1, out2, k?bandsr[0]:bandsl[0]);
      else
        fdctb8(out1, out2, k?bandsr[0]:bandsl[0]);
    }

    bandsl++;
    bandsr++;

    float *in=synbuf+((synbufoffset+1)&1)*BUFFEROFFSET;
    float *dw1=((ratereduce==0)?dwin:(ratereduce==1)?dwin2:dwin4)+16-(synbufoffset|1);
    float *dw2=dw1-16+2*(synbufoffset|1);
    synbufoffset = (synbufoffset-1)&15;
    if (!dest)
      continue;

    if (samplesize==2)
    {
      short *samples=(short*)dest;
      if (!tostereo)
        if (dstchan==2)
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[2*j+0]=cliptoshort(muladd16a(dw1,in));
            samples[2*j+1]=cliptoshort(muladd16a(dw1,in+BUFFEROFFSET*2));
            if (!j||(j==nsmp2))
              continue;
            samples[2*2*nsmp2-2*j+0]=cliptoshort(muladd16b(dw2,in));
            samples[2*2*nsmp2-2*j+1]=cliptoshort(muladd16b(dw2,in+BUFFEROFFSET*2));
          }
        else
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[j]=cliptoshort(muladd16a(dw1,in));
            if (!j||(j==nsmp2))
              continue;
            samples[2*nsmp2-j]=cliptoshort(muladd16b(dw2,in));
          }
      else
        if (!usevoltab)
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[2*j+0]=samples[2*j+1]=cliptoshort(muladd16a(dw1,in));
            if (!j||(j==nsmp2))
              continue;
            samples[2*2*nsmp2-2*j+0]=samples[2*2*nsmp2-2*j+1]=cliptoshort(muladd16b(dw2,in));
          }
        else
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            double sum=muladd16a(dw1,in);
            samples[2*j+0]=cliptoshort(sum*stereotab[0][2]);
            samples[2*j+1]=cliptoshort(sum*stereotab[1][2]);
            if (!j||(j==nsmp2))
              continue;
            sum=muladd16b(dw2,in);
            samples[2*2*nsmp2-2*j+0]=cliptoshort(sum*stereotab[0][2]);
            samples[2*2*nsmp2-2*j+1]=cliptoshort(sum*stereotab[1][2]);
          }
    }
    else
    {
      float *samples=(float*)dest;
      if (!tostereo)
        if (dstchan==2)
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[2*j+0]=muladd16a(dw1,in);
            samples[2*j+1]=muladd16a(dw1,in+BUFFEROFFSET*2);
            if (!j||(j==nsmp2))
              continue;
            samples[2*2*nsmp2-2*j+0]=muladd16b(dw2,in);
            samples[2*2*nsmp2-2*j+1]=muladd16b(dw2,in+BUFFEROFFSET*2);
          }
        else
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[j]=muladd16a(dw1,in);
            if (!j||(j==nsmp2))
              continue;
            samples[2*nsmp2-j]=muladd16b(dw2,in);
          }
      else
        if (!usevoltab)
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            samples[2*j+0]=samples[2*j+1]=muladd16a(dw1,in);
            if (!j||(j==nsmp2))
              continue;
            samples[2*2*nsmp2-2*j+0]=samples[2*2*nsmp2-2*j+1]=muladd16b(dw2,in);
          }
        else
          for (j=0; j<=nsmp2; j++, dw1+=32, dw2+=32, in+=16)
          {
            double sum=muladd16a(dw1,in);
            samples[2*j+0]=sum*stereotab[0][2];
            samples[2*j+1]=sum*stereotab[1][2];
            if (!j||(j==nsmp2))
              continue;
            sum=muladd16b(dw2,in);
            samples[2*2*nsmp2-2*j+0]=sum*stereotab[0][2];
            samples[2*2*nsmp2-2*j+1]=sum*stereotab[1][2];
          }
    }
    dest=((char*)dest)+samplesize*dstchan*(32>>ratereduce);
  }
}

void ampegdecoder::resetsynth()
{
  int i;
  synbufoffset=0;
  for (i=0; i<(BUFFEROFFSET*4); i++)
    synbuf[i]=0;
}

int ampegdecoder::opensynth()
{
  int i,j;
  resetsynth();
  float dwincc[8];
//  dwincc[5]=-0.342943448;   //   2.739098988
//  dwincc[6]=0.086573376;    //  11.50752718
//  dwincc[7]=-0.00773018993; // 129.3590624
  dwincc[5]=-0.341712191984;
  dwincc[6]=0.0866307578916;
  dwincc[7]=-0.00849728985506;
  dwincc[0]=0.5;
  dwincc[1]=-sqrt(1-dwincc[7]*dwincc[7]);
  dwincc[2]=sqrt(1-dwincc[6]*dwincc[6]);
  dwincc[3]=-sqrt(1-dwincc[5]*dwincc[5]);
  dwincc[4]=sqrt(0.5);
  for (i=0; i<1024; i++)
  {
    double v=0;
    for (j=0; j<8; j++)
      v+=cos(2*_PI/512*((i<<5)+(i>>5))*j)*dwincc[j]*8192;
    dwin[i]=(i&2)?-v:v;
  }
  for (i=0; i<512; i++)
    dwin2[i]=dwin[(i&31)+((i&~31)<<1)];
  for (i=0; i<256; i++)
    dwin4[i]=dwin[(i&31)+((i&~31)<<2)];

  for (j=0; j<5; j++)
    for (i=0; i<(1<<j); i++)
      sectab[i+(1<<j)]=0.5/cos(_PI*(2*i+1)/(4<<j));

  if (!dstchan)
    dstchan=srcchan;
  tomono=0;
  tostereo=0;
  if (dstchan==-2)
  {
    dstchan=2;
    tostereo=1;
    if (srcchan==2)
      tomono=1;
  }
  if ((srcchan==2)&&(dstchan==1))
    tomono=1;
  if ((srcchan==1)&&(dstchan==2))
    tostereo=1;
  dctstereo=(tomono||tostereo||(srcchan==1))?1:2;
  usevoltab=0;
  volume=1;
  equalon=0;

  return 1;
}

void ampegdecoder::setvol(float v)
{
  volume=v;
}

void ampegdecoder::setstereo(const float *v)
{
  if (!v)
  {
    usevoltab=0;
    return;
  }
  if ((v[0]==1)&&(v[1]==0)&&(v[2]==1)&&(v[3]==0)&&(v[4]==1)&&(v[5]==1)&&(v[6]==0.5)&&(v[7]==0.5)&&(v[8]==1))
  {
    usevoltab=0;
    return;
  }
  stereotab[0][0]=v[0];
  stereotab[0][1]=v[1];
  stereotab[0][2]=v[2];
  stereotab[1][0]=v[3];
  stereotab[1][1]=v[4];
  stereotab[1][2]=v[5];
  stereotab[2][0]=v[6];
  stereotab[2][1]=v[7];
  stereotab[2][2]=v[8];
  usevoltab=1;
}

void ampegdecoder::setequal(const float *buf)
{
  if (!buf)
  {
    equalon=0;
    return;
  }
  int i;
  for (i=0; i<32; i++)
    if (buf[i]!=1)
      break;
  if (i==32)
  {
    equalon=0;
    return;
  }
  if (ratereduce==0)
    for (i=0; i<32; i++)
      equal[i]=buf[i];
  else
  if (ratereduce==1)
    for (i=0; i<16; i++)
      equal[i]=(buf[2*i+0]+buf[2*i+1])/2;
  else
    for (i=0; i<8; i++)
      equal[i]=(buf[4*i+0]+buf[4*i+1]+buf[4*i+2]+buf[4*i+3])/4;
  equalon=1;
}
