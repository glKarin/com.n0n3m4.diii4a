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

// amp11 - an Audio-MPEG decoder - layer 3 decoder
// huffman tables were taken from mpg123 by Michael Hipp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ampdec.h"

#define _PI 3.14159265358979

#define HUFFMAXLONG 8
//#define HUFFMAXLONG (hdrlsf?6:8)

// some remarks
//
// huffman with blocktypes!=0:
// there are 2 regions for the huffman decoder. in version 1 the limit
// is always 36. in version 2 it seems to be 54 for blocktypes 1 and 3
// and 36 for blocktype 2. why those 54? i cannot understand that.
// the streams i found decode fine with HUFFMAXLONG==8, but not with
// the limit of 36 in all cases (HUFFMAXLONG==version?6:8).
// i suspect this might be a bug in L3ENC by Fraunhofer IIS.
// i have no proof, just a feeling...
//
// mixed blocks:
// in version 1 there are 8 long bands and 9*3 short bands. these values
// seem to be unchanged in version 2 in all the sources i could find.
// this means there are 35 bands in total and that does not match the value
// 33 of sfbtab, which means that there are 2 bands without a scalefactor.
// furthermore there would be bands with double scalefactors, because the
// long bands stop at index 54 and the short bands begin at 36.
// this is nonsense.
// therefore i changed the number of long bands to 6 in version 2, so that
// the limit is 36 for both and there are 35 bands in total. this is the only
// solution with all problems solved.
// i have no streams to test this, but i am very sure about this.
//
// huffman decoder:
// the size of one granule is given in the sideinfo, just why isn't it
// always exact???!!!

//  nbeisert


float ampegdecoder::csatab[8][2];

float ampegdecoder::ggaintab[256];
float ampegdecoder::ktab[3][32][2];
float ampegdecoder::pow2tab[65];
float ampegdecoder::pow43tab[8207];

float ampegdecoder::sec12[3];
float ampegdecoder::sec24wins[6];
float ampegdecoder::cos6[3];
float ampegdecoder::sec72winl[18];
float ampegdecoder::sec36[9];
float ampegdecoder::cos18[9];
float ampegdecoder::winsqs[3];
float ampegdecoder::winsql[12];
float ampegdecoder::winlql[9];
float ampegdecoder::sqrt05;

int ampegdecoder::sfbands[3][3][14] =
{
  {
    {  0, 12, 24, 36, 48, 66, 90,120,156,198,252,318,408,576},
    {  0, 12, 24, 36, 48, 66, 84,114,150,192,240,300,378,576},
    {  0, 12, 24, 36, 48, 66, 90,126,174,234,312,414,540,576}
  },
  {
    {  0, 12, 24, 36, 54, 72, 96,126,168,222,300,396,522,576},
    {  0, 12, 24, 36, 54, 78,108,144,186,240,312,408,540,576},
    {  0, 12, 24, 36, 54, 78,108,144,186,240,312,402,522,576}
  },
  {
    {  0, 12, 24, 36, 54, 78,108,144,186,240,312,402,522,576},
    {  0, 12, 24, 36, 54, 78,108,144,186,240,312,402,522,576},
    {  0, 24, 48, 72,108,156,216,288,372,480,486,492,498,576}
  }
};


int ampegdecoder::sfbandl[3][3][23] =
{
  {
    {  0,  4,  8, 12, 16, 20, 24, 30, 36, 44, 52, 62, 74, 90,110,134,162,196,238,288,342,418,576},
    {  0,  4,  8, 12, 16, 20, 24, 30, 36, 42, 50, 60, 72, 88,106,128,156,190,230,276,330,384,576},
    {  0,  4,  8, 12, 16, 20, 24, 30, 36, 44, 54, 66, 82,102,126,156,194,240,296,364,448,550,576}
  },
  {
    {  0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96,116,140,168,200,238,284,336,396,464,522,576},
    {  0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96,114,136,162,194,232,278,332,394,464,540,576},
    {  0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96,116,140,168,200,238,284,336,396,464,522,576}
  },
  {
    {  0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96,116,140,168,200,238,284,336,396,464,522,576},
    {  0,  6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96,116,140,168,200,238,284,336,396,464,522,576},
    {  0, 12, 24, 36, 48, 60, 72, 88,108,132,160,192,232,280,336,400,476,566,568,570,572,574,576}
  }
};

int ampegdecoder::pretab[22]={0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0};
float ampegdecoder::citab[8]={-0.6,-0.535,-0.33,-0.185,-0.095,-0.041,-0.0142,-0.0037};

int ampegdecoder::slentab[2][16] = {{0,0,0,0,3,1,1,1,2,2,2,3,3,3,4,4},
                                      {0,1,2,3,0,1,2,3,1,2,3,1,2,3,2,3}};

int ampegdecoder::sfbtab[7][3][5]=
{
  {{ 0, 6,11,16,21}, { 0, 9,18,27,36}, { 0, 6,15,24,33}},
  {{ 0, 6,11,18,21}, { 0, 9,18,30,36}, { 0, 6,15,27,33}},
  {{ 0,11,21,21,21}, { 0,18,36,36,36}, { 0,15,33,33,33}},
  {{ 0, 7,14,21,21}, { 0,12,24,36,36}, { 0, 6,21,33,33}},
  {{ 0, 6,12,18,21}, { 0,12,21,30,36}, { 0, 6,18,27,33}},
  {{ 0, 8,16,21,21}, { 0,15,27,36,36}, { 0, 6,24,33,33}},
  {{ 0, 6,11,16,21}, { 0, 9,18,24,36}, { 0, 8,17,23,35}},
};

int ampegdecoder::htab00[] =
{
   0
};

int ampegdecoder::htab01[] =
{
  -5,  -3,  -1,  17,   1,  16,   0
};

int ampegdecoder::htab02[] =
{
 -15, -11,  -9,  -5,  -3,  -1,  34,   2,  18,  -1,  33,  32,  17,  -1,   1,
  16,   0
};

int ampegdecoder::htab03[] =
{
 -13, -11,  -9,  -5,  -3,  -1,  34,   2,  18,  -1,  33,  32,  16,  17,  -1,
   1,   0
};

int ampegdecoder::htab04[] =
{
   0
};

int ampegdecoder::htab05[] =
{
 -29, -25, -23, -15,  -7,  -5,  -3,  -1,  51,  35,  50,  49,  -3,  -1,  19,
   3,  -1,  48,  34,  -3,  -1,  18,  33,  -1,   2,  32,  17,  -1,   1,  16,
   0
};

int ampegdecoder::htab06[] =
{
 -25, -19, -13,  -9,  -5,  -3,  -1,  51,   3,  35,  -1,  50,  48,  -1,  19,
  49,  -3,  -1,  34,   2,  18,  -3,  -1,  33,  32,   1,  -1,  17,  -1,  16,
   0
};

int ampegdecoder::htab07[] =
{
 -69, -65, -57, -39, -29, -17, -11,  -7,  -3,  -1,  85,  69,  -1,  84,  83,
  -1,  53,  68,  -3,  -1,  37,  82,  21,  -5,  -1,  81,  -1,   5,  52,  -1,
  80,  -1,  67,  51,  -5,  -3,  -1,  36,  66,  20,  -1,  65,  64, -11,  -7,
  -3,  -1,   4,  35,  -1,  50,   3,  -1,  19,  49,  -3,  -1,  48,  34,  18,
  -5,  -1,  33,  -1,   2,  32,  17,  -1,   1,  16,   0
};

int ampegdecoder::htab08[] =
{
 -65, -63, -59, -45, -31, -19, -13,  -7,  -5,  -3,  -1,  85,  84,  69,  83,
  -3,  -1,  53,  68,  37,  -3,  -1,  82,   5,  21,  -5,  -1,  81,  -1,  52,
  67,  -3,  -1,  80,  51,  36,  -5,  -3,  -1,  66,  20,  65,  -3,  -1,   4,
  64,  -1,  35,  50,  -9,  -7,  -3,  -1,  19,  49,  -1,   3,  48,  34,  -1,
   2,  32,  -1,  18,  33,  17,  -3,  -1,   1,  16,   0
};

int ampegdecoder::htab09[] =
{
 -63, -53, -41, -29, -19, -11,  -5,  -3,  -1,  85,  69,  53,  -1,  83,  -1,
  84,   5,  -3,  -1,  68,  37,  -1,  82,  21,  -3,  -1,  81,  52,  -1,  67,
  -1,  80,   4,  -7,  -3,  -1,  36,  66,  -1,  51,  64,  -1,  20,  65,  -5,
  -3,  -1,  35,  50,  19,  -1,  49,  -1,   3,  48,  -5,  -3,  -1,  34,   2,
  18,  -1,  33,  32,  -3,  -1,  17,   1,  -1,  16,   0
};

int ampegdecoder::htab10[] =
{
-125,-121,-111, -83, -55, -35, -21, -13,  -7,  -3,  -1, 119, 103,  -1, 118,
  87,  -3,  -1, 117, 102,  71,  -3,  -1, 116,  86,  -1, 101,  55,  -9,  -3,
  -1, 115,  70,  -3,  -1,  85,  84,  99,  -1,  39, 114, -11,  -5,  -3,  -1,
 100,   7, 112,  -1,  98,  -1,  69,  53,  -5,  -1,   6,  -1,  83,  68,  23,
 -17,  -5,  -1, 113,  -1,  54,  38,  -5,  -3,  -1,  37,  82,  21,  -1,  81,
  -1,  52,  67,  -3,  -1,  22,  97,  -1,  96,  -1,   5,  80, -19, -11,  -7,
  -3,  -1,  36,  66,  -1,  51,   4,  -1,  20,  65,  -3,  -1,  64,  35,  -1,
  50,   3,  -3,  -1,  19,  49,  -1,  48,  34,  -7,  -3,  -1,  18,  33,  -1,
   2,  32,  17,  -1,   1,  16,   0
};

int ampegdecoder::htab11[] =
{
-121,-113, -89, -59, -43, -27, -17,  -7,  -3,  -1, 119, 103,  -1, 118, 117,
  -3,  -1, 102,  71,  -1, 116,  -1,  87,  85,  -5,  -3,  -1,  86, 101,  55,
  -1, 115,  70,  -9,  -7,  -3,  -1,  69,  84,  -1,  53,  83,  39,  -1, 114,
  -1, 100,   7,  -5,  -1, 113,  -1,  23, 112,  -3,  -1,  54,  99,  -1,  96,
  -1,  68,  37, -13,  -7,  -5,  -3,  -1,  82,   5,  21,  98,  -3,  -1,  38,
   6,  22,  -5,  -1,  97,  -1,  81,  52,  -5,  -1,  80,  -1,  67,  51,  -1,
  36,  66, -15, -11,  -7,  -3,  -1,  20,  65,  -1,   4,  64,  -1,  35,  50,
  -1,  19,  49,  -5,  -3,  -1,   3,  48,  34,  33,  -5,  -1,  18,  -1,   2,
  32,  17,  -3,  -1,   1,  16,   0
};

int ampegdecoder::htab12[] =
{
-115, -99, -73, -45, -27, -17,  -9,  -5,  -3,  -1, 119, 103, 118,  -1,  87,
 117,  -3,  -1, 102,  71,  -1, 116, 101,  -3,  -1,  86,  55,  -3,  -1, 115,
  85,  39,  -7,  -3,  -1, 114,  70,  -1, 100,  23,  -5,  -1, 113,  -1,   7,
 112,  -1,  54,  99, -13,  -9,  -3,  -1,  69,  84,  -1,  68,  -1,   6,   5,
  -1,  38,  98,  -5,  -1,  97,  -1,  22,  96,  -3,  -1,  53,  83,  -1,  37,
  82, -17,  -7,  -3,  -1,  21,  81,  -1,  52,  67,  -5,  -3,  -1,  80,   4,
  36,  -1,  66,  20,  -3,  -1,  51,  65,  -1,  35,  50, -11,  -7,  -5,  -3,
  -1,  64,   3,  48,  19,  -1,  49,  34,  -1,  18,  33,  -7,  -5,  -3,  -1,
   2,  32,   0,  17,  -1,   1,  16
};

int ampegdecoder::htab13[] =
{
-509,-503,-475,-405,-333,-265,-205,-153,-115, -83, -53, -35, -21, -13,  -9,
  -7,  -5,  -3,  -1, 254, 252, 253, 237, 255,  -1, 239, 223,  -3,  -1, 238,
 207,  -1, 222, 191,  -9,  -3,  -1, 251, 206,  -1, 220,  -1, 175, 233,  -1,
 236, 221,  -9,  -5,  -3,  -1, 250, 205, 190,  -1, 235, 159,  -3,  -1, 249,
 234,  -1, 189, 219, -17,  -9,  -3,  -1, 143, 248,  -1, 204,  -1, 174, 158,
  -5,  -1, 142,  -1, 127, 126, 247,  -5,  -1, 218,  -1, 173, 188,  -3,  -1,
 203, 246, 111, -15,  -7,  -3,  -1, 232,  95,  -1, 157, 217,  -3,  -1, 245,
 231,  -1, 172, 187,  -9,  -3,  -1,  79, 244,  -3,  -1, 202, 230, 243,  -1,
  63,  -1, 141, 216, -21,  -9,  -3,  -1,  47, 242,  -3,  -1, 110, 156,  15,
  -5,  -3,  -1, 201,  94, 171,  -3,  -1, 125, 215,  78, -11,  -5,  -3,  -1,
 200, 214,  62,  -1, 185,  -1, 155, 170,  -1,  31, 241, -23, -13,  -5,  -1,
 240,  -1, 186, 229,  -3,  -1, 228, 140,  -1, 109, 227,  -5,  -1, 226,  -1,
  46,  14,  -1,  30, 225, -15,  -7,  -3,  -1, 224,  93,  -1, 213, 124,  -3,
  -1, 199,  77,  -1, 139, 184,  -7,  -3,  -1, 212, 154,  -1, 169, 108,  -1,
 198,  61, -37, -21,  -9,  -5,  -3,  -1, 211, 123,  45,  -1, 210,  29,  -5,
  -1, 183,  -1,  92, 197,  -3,  -1, 153, 122, 195,  -7,  -5,  -3,  -1, 167,
 151,  75, 209,  -3,  -1,  13, 208,  -1, 138, 168, -11,  -7,  -3,  -1,  76,
 196,  -1, 107, 182,  -1,  60,  44,  -3,  -1, 194,  91,  -3,  -1, 181, 137,
  28, -43, -23, -11,  -5,  -1, 193,  -1, 152,  12,  -1, 192,  -1, 180, 106,
  -5,  -3,  -1, 166, 121,  59,  -1, 179,  -1, 136,  90, -11,  -5,  -1,  43,
  -1, 165, 105,  -1, 164,  -1, 120, 135,  -5,  -1, 148,  -1, 119, 118, 178,
 -11,  -3,  -1,  27, 177,  -3,  -1,  11, 176,  -1, 150,  74,  -7,  -3,  -1,
  58, 163,  -1,  89, 149,  -1,  42, 162, -47, -23,  -9,  -3,  -1,  26, 161,
  -3,  -1,  10, 104, 160,  -5,  -3,  -1, 134,  73, 147,  -3,  -1,  57,  88,
  -1, 133, 103,  -9,  -3,  -1,  41, 146,  -3,  -1,  87, 117,  56,  -5,  -1,
 131,  -1, 102,  71,  -3,  -1, 116,  86,  -1, 101, 115, -11,  -3,  -1,  25,
 145,  -3,  -1,   9, 144,  -1,  72, 132,  -7,  -5,  -1, 114,  -1,  70, 100,
  40,  -1, 130,  24, -41, -27, -11,  -5,  -3,  -1,  55,  39,  23,  -1, 113,
  -1,  85,   7,  -7,  -3,  -1, 112,  54,  -1,  99,  69,  -3,  -1,  84,  38,
  -1,  98,  53,  -5,  -1, 129,  -1,   8, 128,  -3,  -1,  22,  97,  -1,   6,
  96, -13,  -9,  -5,  -3,  -1,  83,  68,  37,  -1,  82,   5,  -1,  21,  81,
  -7,  -3,  -1,  52,  67,  -1,  80,  36,  -3,  -1,  66,  51,  20, -19, -11,
  -5,  -1,  65,  -1,   4,  64,  -3,  -1,  35,  50,  19,  -3,  -1,  49,   3,
  -1,  48,  34,  -3,  -1,  18,  33,  -1,   2,  32,  -3,  -1,  17,   1,  16,
   0
};

int ampegdecoder::htab14[] =
{
   0
};

int ampegdecoder::htab15[] =
{
-495,-445,-355,-263,-183,-115, -77, -43, -27, -13,  -7,  -3,  -1, 255, 239,
  -1, 254, 223,  -1, 238,  -1, 253, 207,  -7,  -3,  -1, 252, 222,  -1, 237,
 191,  -1, 251,  -1, 206, 236,  -7,  -3,  -1, 221, 175,  -1, 250, 190,  -3,
  -1, 235, 205,  -1, 220, 159, -15,  -7,  -3,  -1, 249, 234,  -1, 189, 219,
  -3,  -1, 143, 248,  -1, 204, 158,  -7,  -3,  -1, 233, 127,  -1, 247, 173,
  -3,  -1, 218, 188,  -1, 111,  -1, 174,  15, -19, -11,  -3,  -1, 203, 246,
  -3,  -1, 142, 232,  -1,  95, 157,  -3,  -1, 245, 126,  -1, 231, 172,  -9,
  -3,  -1, 202, 187,  -3,  -1, 217, 141,  79,  -3,  -1, 244,  63,  -1, 243,
 216, -33, -17,  -9,  -3,  -1, 230,  47,  -1, 242,  -1, 110, 240,  -3,  -1,
  31, 241,  -1, 156, 201,  -7,  -3,  -1,  94, 171,  -1, 186, 229,  -3,  -1,
 125, 215,  -1,  78, 228, -15,  -7,  -3,  -1, 140, 200,  -1,  62, 109,  -3,
  -1, 214, 227,  -1, 155, 185,  -7,  -3,  -1,  46, 170,  -1, 226,  30,  -5,
  -1, 225,  -1,  14, 224,  -1,  93, 213, -45, -25, -13,  -7,  -3,  -1, 124,
 199,  -1,  77, 139,  -1, 212,  -1, 184, 154,  -7,  -3,  -1, 169, 108,  -1,
 198,  61,  -1, 211, 210,  -9,  -5,  -3,  -1,  45,  13,  29,  -1, 123, 183,
  -5,  -1, 209,  -1,  92, 208,  -1, 197, 138, -17,  -7,  -3,  -1, 168,  76,
  -1, 196, 107,  -5,  -1, 182,  -1, 153,  12,  -1,  60, 195,  -9,  -3,  -1,
 122, 167,  -1, 166,  -1, 192,  11,  -1, 194,  -1,  44,  91, -55, -29, -15,
  -7,  -3,  -1, 181,  28,  -1, 137, 152,  -3,  -1, 193,  75,  -1, 180, 106,
  -5,  -3,  -1,  59, 121, 179,  -3,  -1, 151, 136,  -1,  43,  90, -11,  -5,
  -1, 178,  -1, 165,  27,  -1, 177,  -1, 176, 105,  -7,  -3,  -1, 150,  74,
  -1, 164, 120,  -3,  -1, 135,  58, 163, -17,  -7,  -3,  -1,  89, 149,  -1,
  42, 162,  -3,  -1,  26, 161,  -3,  -1,  10, 160, 104,  -7,  -3,  -1, 134,
  73,  -1, 148,  57,  -5,  -1, 147,  -1, 119,   9,  -1,  88, 133, -53, -29,
 -13,  -7,  -3,  -1,  41, 103,  -1, 118, 146,  -1, 145,  -1,  25, 144,  -7,
  -3,  -1,  72, 132,  -1,  87, 117,  -3,  -1,  56, 131,  -1, 102,  71,  -7,
  -3,  -1,  40, 130,  -1,  24, 129,  -7,  -3,  -1, 116,   8,  -1, 128,  86,
  -3,  -1, 101,  55,  -1, 115,  70, -17,  -7,  -3,  -1,  39, 114,  -1, 100,
  23,  -3,  -1,  85, 113,  -3,  -1,   7, 112,  54,  -7,  -3,  -1,  99,  69,
  -1,  84,  38,  -3,  -1,  98,  22,  -3,  -1,   6,  96,  53, -33, -19,  -9,
  -5,  -1,  97,  -1,  83,  68,  -1,  37,  82,  -3,  -1,  21,  81,  -3,  -1,
   5,  80,  52,  -7,  -3,  -1,  67,  36,  -1,  66,  51,  -1,  65,  -1,  20,
   4,  -9,  -3,  -1,  35,  50,  -3,  -1,  64,   3,  19,  -3,  -1,  49,  48,
  34,  -9,  -7,  -3,  -1,  18,  33,  -1,   2,  32,  17,  -3,  -1,   1,  16,
   0
};

int ampegdecoder::htab16[] =
{
-509,-503,-461,-323,-103, -37, -27, -15,  -7,  -3,  -1, 239, 254,  -1, 223,
 253,  -3,  -1, 207, 252,  -1, 191, 251,  -5,  -1, 175,  -1, 250, 159,  -3,
  -1, 249, 248, 143,  -7,  -3,  -1, 127, 247,  -1, 111, 246, 255,  -9,  -5,
  -3,  -1,  95, 245,  79,  -1, 244, 243, -53,  -1, 240,  -1,  63, -29, -19,
 -13,  -7,  -5,  -1, 206,  -1, 236, 221, 222,  -1, 233,  -1, 234, 217,  -1,
 238,  -1, 237, 235,  -3,  -1, 190, 205,  -3,  -1, 220, 219, 174, -11,  -5,
  -1, 204,  -1, 173, 218,  -3,  -1, 126, 172, 202,  -5,  -3,  -1, 201, 125,
  94, 189, 242, -93,  -5,  -3,  -1,  47,  15,  31,  -1, 241, -49, -25, -13,
  -5,  -1, 158,  -1, 188, 203,  -3,  -1, 142, 232,  -1, 157, 231,  -7,  -3,
  -1, 187, 141,  -1, 216, 110,  -1, 230, 156, -13,  -7,  -3,  -1, 171, 186,
  -1, 229, 215,  -1,  78,  -1, 228, 140,  -3,  -1, 200,  62,  -1, 109,  -1,
 214, 155, -19, -11,  -5,  -3,  -1, 185, 170, 225,  -1, 212,  -1, 184, 169,
  -5,  -1, 123,  -1, 183, 208, 227,  -7,  -3,  -1,  14, 224,  -1,  93, 213,
  -3,  -1, 124, 199,  -1,  77, 139, -75, -45, -27, -13,  -7,  -3,  -1, 154,
 108,  -1, 198,  61,  -3,  -1,  92, 197,  13,  -7,  -3,  -1, 138, 168,  -1,
 153,  76,  -3,  -1, 182, 122,  60, -11,  -5,  -3,  -1,  91, 137,  28,  -1,
 192,  -1, 152, 121,  -1, 226,  -1,  46,  30, -15,  -7,  -3,  -1, 211,  45,
  -1, 210, 209,  -5,  -1,  59,  -1, 151, 136,  29,  -7,  -3,  -1, 196, 107,
  -1, 195, 167,  -1,  44,  -1, 194, 181, -23, -13,  -7,  -3,  -1, 193,  12,
  -1,  75, 180,  -3,  -1, 106, 166, 179,  -5,  -3,  -1,  90, 165,  43,  -1,
 178,  27, -13,  -5,  -1, 177,  -1,  11, 176,  -3,  -1, 105, 150,  -1,  74,
 164,  -5,  -3,  -1, 120, 135, 163,  -3,  -1,  58,  89,  42, -97, -57, -33,
 -19, -11,  -5,  -3,  -1, 149, 104, 161,  -3,  -1, 134, 119, 148,  -5,  -3,
  -1,  73,  87, 103, 162,  -5,  -1,  26,  -1,  10, 160,  -3,  -1,  57, 147,
  -1,  88, 133,  -9,  -3,  -1,  41, 146,  -3,  -1, 118,   9,  25,  -5,  -1,
 145,  -1, 144,  72,  -3,  -1, 132, 117,  -1,  56, 131, -21, -11,  -5,  -3,
  -1, 102,  40, 130,  -3,  -1,  71, 116,  24,  -3,  -1, 129, 128,  -3,  -1,
   8,  86,  55,  -9,  -5,  -1, 115,  -1, 101,  70,  -1,  39, 114,  -5,  -3,
  -1, 100,  85,   7,  23, -23, -13,  -5,  -1, 113,  -1, 112,  54,  -3,  -1,
  99,  69,  -1,  84,  38,  -3,  -1,  98,  22,  -1,  97,  -1,   6,  96,  -9,
  -5,  -1,  83,  -1,  53,  68,  -1,  37,  82,  -1,  81,  -1,  21,   5, -33,
 -23, -13,  -7,  -3,  -1,  52,  67,  -1,  80,  36,  -3,  -1,  66,  51,  20,
  -5,  -1,  65,  -1,   4,  64,  -1,  35,  50,  -3,  -1,  19,  49,  -3,  -1,
   3,  48,  34,  -3,  -1,  18,  33,  -1,   2,  32,  -3,  -1,  17,   1,  16,
   0
};

int ampegdecoder::htab24[] =
{
-451,-117, -43, -25, -15,  -7,  -3,  -1, 239, 254,  -1, 223, 253,  -3,  -1,
 207, 252,  -1, 191, 251,  -5,  -1, 250,  -1, 175, 159,  -1, 249, 248,  -9,
  -5,  -3,  -1, 143, 127, 247,  -1, 111, 246,  -3,  -1,  95, 245,  -1,  79,
 244, -71,  -7,  -3,  -1,  63, 243,  -1,  47, 242,  -5,  -1, 241,  -1,  31,
 240, -25,  -9,  -1,  15,  -3,  -1, 238, 222,  -1, 237, 206,  -7,  -3,  -1,
 236, 221,  -1, 190, 235,  -3,  -1, 205, 220,  -1, 174, 234, -15,  -7,  -3,
  -1, 189, 219,  -1, 204, 158,  -3,  -1, 233, 173,  -1, 218, 188,  -7,  -3,
  -1, 203, 142,  -1, 232, 157,  -3,  -1, 217, 126,  -1, 231, 172, 255,-235,
-143, -77, -45, -25, -15,  -7,  -3,  -1, 202, 187,  -1, 141, 216,  -5,  -3,
  -1,  14, 224,  13, 230,  -5,  -3,  -1, 110, 156, 201,  -1,  94, 186,  -9,
  -5,  -1, 229,  -1, 171, 125,  -1, 215, 228,  -3,  -1, 140, 200,  -3,  -1,
  78,  46,  62, -15,  -7,  -3,  -1, 109, 214,  -1, 227, 155,  -3,  -1, 185,
 170,  -1, 226,  30,  -7,  -3,  -1, 225,  93,  -1, 213, 124,  -3,  -1, 199,
  77,  -1, 139, 184, -31, -15,  -7,  -3,  -1, 212, 154,  -1, 169, 108,  -3,
  -1, 198,  61,  -1, 211,  45,  -7,  -3,  -1, 210,  29,  -1, 123, 183,  -3,
  -1, 209,  92,  -1, 197, 138, -17,  -7,  -3,  -1, 168, 153,  -1,  76, 196,
  -3,  -1, 107, 182,  -3,  -1, 208,  12,  60,  -7,  -3,  -1, 195, 122,  -1,
 167,  44,  -3,  -1, 194,  91,  -1, 181,  28, -57, -35, -19,  -7,  -3,  -1,
 137, 152,  -1, 193,  75,  -5,  -3,  -1, 192,  11,  59,  -3,  -1, 176,  10,
  26,  -5,  -1, 180,  -1, 106, 166,  -3,  -1, 121, 151,  -3,  -1, 160,   9,
 144,  -9,  -3,  -1, 179, 136,  -3,  -1,  43,  90, 178,  -7,  -3,  -1, 165,
  27,  -1, 177, 105,  -1, 150, 164, -17,  -9,  -5,  -3,  -1,  74, 120, 135,
  -1,  58, 163,  -3,  -1,  89, 149,  -1,  42, 162,  -7,  -3,  -1, 161, 104,
  -1, 134, 119,  -3,  -1,  73, 148,  -1,  57, 147, -63, -31, -15,  -7,  -3,
  -1,  88, 133,  -1,  41, 103,  -3,  -1, 118, 146,  -1,  25, 145,  -7,  -3,
  -1,  72, 132,  -1,  87, 117,  -3,  -1,  56, 131,  -1, 102,  40, -17,  -7,
  -3,  -1, 130,  24,  -1,  71, 116,  -5,  -1, 129,  -1,   8, 128,  -1,  86,
 101,  -7,  -5,  -1,  23,  -1,   7, 112, 115,  -3,  -1,  55,  39, 114, -15,
  -7,  -3,  -1,  70, 100,  -1,  85, 113,  -3,  -1,  54,  99,  -1,  69,  84,
  -7,  -3,  -1,  38,  98,  -1,  22,  97,  -5,  -3,  -1,   6,  96,  53,  -1,
  83,  68, -51, -37, -23, -15,  -9,  -3,  -1,  37,  82,  -1,  21,  -1,   5,
  80,  -1,  81,  -1,  52,  67,  -3,  -1,  36,  66,  -1,  51,  20,  -9,  -5,
  -1,  65,  -1,   4,  64,  -1,  35,  50,  -1,  19,  49,  -7,  -5,  -3,  -1,
   3,  48,  34,  18,  -1,  33,  -1,   2,  32,  -3,  -1,  17,   1,  -1,  16,
   0
};

int ampegdecoder::htaba[] =
{
 -29, -21, -13,  -7,  -3,  -1,  11,  15,  -1,  13,  14,  -3,  -1,   7,   5,
   9,  -3,  -1,   6,   3,  -1,  10,  12,  -3,  -1,   2,   1,  -1,   4,   8,
   0
};

int ampegdecoder::htabb[] = // get 4 bits
{
 -15,  -7,  -3,  -1,  15,  14,  -1,  13,  12,  -3,  -1,  11,  10,  -1,   9,
   8,  -7,  -3,  -1,   7,   6,  -1,   5,   4,  -3,  -1,   3,   2,  -1,   1,
   0
};

int ampegdecoder::htablinbits[34]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,6,8,10,13,4,5,6,7,8,9,11,13,0,0};
int *ampegdecoder::htabs[34]={htab00,htab01,htab02,htab03,htab04,htab05,htab06,htab07,htab08,htab09,htab10,htab11,htab12,htab13,htab14,htab15,htab16,htab16,htab16,htab16,htab16,htab16,htab16,htab16,htab24,htab24,htab24,htab24,htab24,htab24,htab24,htab24,htaba,htabb};




// This uses Byeong Gi Lee's Fast Cosine Transform algorithm to
// decompose the 36 point and 12 point IDCT's into 9 point and 3
// point IDCT's, respectively. -- Jeff Tsay.

// 9 Point Inverse Discrete Cosine Transform
//
// This piece of code is Copyright 1997 Mikko Tommila and is freely usable
// by anybody. The algorithm itself is of course in the public domain.
//
// Again derived heuristically from the 9-point WFTA.
//
// The algorithm is optimized (?) for speed, not for small rounding errors or
// good readability.
//
// 36 additions, 11 multiplications
//
// Again this is very likely sub-optimal.
//
// The code is optimized to use a minimum number of temporary variables,
// so it should compile quite well even on 8-register Intel x86 processors.
// This makes the code quite obfuscated and very difficult to understand.
//
// References:
// [1] S. Winograd: "On Computing the Discrete Fourier Transform",
//     Mathematics of Computation, Volume 32, Number 141, January 1978,
//     Pages 175-199

#ifndef FDCTDEXT
void ampegdecoder::fdctd6(float *out, float *in)
{
  float t0,t1;
  out[1] = in[0*3] - (in[4*3]+in[3*3]);
  t0 = (in[1*3]+in[2*3]) * cos6[1];
  t1 = in[0*3] + (in[4*3]+in[3*3]) * cos6[2];
  out[0] = t1 + t0;
  out[2] = t1 - t0;

  in[1*3] += in[0*3];
  in[3*3] += in[2*3];
  in[5*3] += in[3*3]+in[4*3];
  out[4] = in[1*3] - in[5*3];
  t0 = (in[3*3]+in[1*3]) * cos6[1];
  t1 = in[1*3] + in[5*3] * cos6[2];
  out[5] = t1 + t0;
  out[3] = t1 - t0;

  float tmp;
  tmp = out[5] * sec12[0];
  out[5] = (tmp - out[0])*sec24wins[5];
  out[0] = (tmp + out[0])*sec24wins[0];
  tmp = out[4] * sec12[1];
  out[4] = (tmp - out[1])*sec24wins[4];
  out[1] = (tmp + out[1])*sec24wins[1];
  tmp = out[3] * sec12[2];
  out[3] = (tmp - out[2])*sec24wins[3];
  out[2] = (tmp + out[2])*sec24wins[2];
}

void ampegdecoder::fdctd18(float *out, float *in)
{
  in[17]+=in[16]; in[16]+=in[15]; in[15]+=in[14]; in[14]+=in[13];
  in[13]+=in[12]; in[12]+=in[11]; in[11]+=in[10]; in[10]+=in[9];
  in[9] +=in[8];  in[8] +=in[7];  in[7] +=in[6];  in[6] +=in[5];
  in[5] +=in[4];  in[4] +=in[3];  in[3] +=in[2];  in[2] +=in[1];
  in[1] +=in[0];
  in[17]+=in[15]; in[15]+=in[13]; in[13]+=in[11]; in[11]+=in[9];
  in[9] +=in[7];  in[7] +=in[5];  in[5] +=in[3];  in[3] +=in[1];

  float t0,t1,t2,t3;

  t0 = cos18[2] * (in[ 4] + in[ 8]);
  t1 = cos18[4] * (in[ 4] + in[16]);
  t2 = cos18[6] *  in[12] + in[ 0];
  t3 = cos18[8] * (in[16] - in[ 8]);
  out[0] = + t2 + t0 + t3;
  out[2] = + t2 + t1 - t0;
  out[3] = + t2 - t3 - t1;

  t0 = in[ 8] + in[16] - in[ 4];
  t1 = in[ 0] - in[12];
  out[1] = + t1 - t0 * cos18[6];
  out[4] = + t1 + t0;

  t0 = cos18[1] * (in[ 2] + in[10]);
  t1 = cos18[3] *  in[ 6];
  t2 = cos18[5] * (in[14] + in[ 2]);
  t3 = cos18[7] * (in[14] - in[10]);
  out[5] = - t1 - t2 + t0;
  out[6] = - t1 + t3 + t2;
  out[8] = + t1 + t0 + t3;

  t0 = in[ 2] - in[10] - in[14];
  out[7] = + t0 * cos18[3];

  float tmp;
  tmp=out[0];
  out[0] = tmp + out[8];
  out[8] = tmp - out[8];
  tmp=out[1];
  out[1] = tmp + out[7];
  out[7] = tmp - out[7];
  tmp=out[2];
  out[2] = tmp + out[6];
  out[6] = tmp - out[6];
  tmp=out[3];
  out[3] = tmp + out[5];
  out[5] = tmp - out[5];

  t0 = cos18[2] * (in[ 5] + in[ 9]);
  t1 = cos18[4] * (in[ 5] + in[17]);
  t2 = cos18[6] *  in[13] + in[ 1];
  t3 = cos18[8] * (in[17] - in[ 9]);
  out[15] = + t2 - t0 + t1;
  out[17] = + t2 + t3 + t0;
  out[14] = + t2 - t1 - t3;

  t0 = in[9] + in[17] - in[5];
  t1 = in[1] - in[13];
  out[16] = + t1 - t0 * cos18[6];
  out[13] = + t1 + t0;

  t0 = cos18[1] * (in[ 3] + in[11]);
  t1 = cos18[3] *  in[ 7];
  t2 = cos18[5] * (in[ 3] + in[15]);
  t3 = cos18[7] * (in[15] - in[11]);
  out[12] = - t1 - t2 + t0;
  out[11] = - t1 + t3 + t2;
  out[ 9] = + t1 + t0 + t3;

  t0 = in[ 3] - in[11] - in[15];
  out[10] = + t0 * cos18[3];

  tmp = out[17];
  out[17] = tmp + out[ 9];
  out[ 9] = tmp - out[ 9];
  tmp = out[16];
  out[16] = tmp + out[10];
  out[10] = tmp - out[10];
  tmp = out[15];
  out[15] = tmp + out[11];
  out[11] = tmp - out[11];
  tmp = out[14];
  out[14] = tmp + out[12];
  out[12] = tmp - out[12];

  tmp = out[17] * sec36[0];
  out[17] = (tmp-out[ 0])*sec72winl[17];
  out[ 0] = (tmp+out[ 0])*sec72winl[ 0];
  tmp = out[16] * sec36[1];
  out[16] = (tmp-out[ 1])*sec72winl[16];
  out[ 1] = (tmp+out[ 1])*sec72winl[ 1];
  tmp = out[15] * sec36[2];
  out[15] = (tmp-out[ 2])*sec72winl[15];
  out[ 2] = (tmp+out[ 2])*sec72winl[ 2];
  tmp = out[14] * sec36[3];
  out[14] = (tmp-out[ 3])*sec72winl[14];
  out[ 3] = (tmp+out[ 3])*sec72winl[ 3];
  tmp = out[13] * sec36[4];
  out[13] = (tmp-out[ 4])*sec72winl[13];
  out[ 4] = (tmp+out[ 4])*sec72winl[ 4];
  tmp = out[12] * sec36[5];
  out[12] = (tmp-out[ 5])*sec72winl[12];
  out[ 5] = (tmp+out[ 5])*sec72winl[ 5];
  tmp = out[11] * sec36[6];
  out[11] = (tmp-out[ 6])*sec72winl[11];
  out[ 6] = (tmp+out[ 6])*sec72winl[ 6];
  tmp = out[10] * sec36[7];
  out[10] = (tmp-out[ 7])*sec72winl[10];
  out[ 7] = (tmp+out[ 7])*sec72winl[ 7];
  tmp = out[ 9] * sec36[8];
  out[ 9] = (tmp-out[ 8])*sec72winl[ 9];
  out[ 8] = (tmp+out[ 8])*sec72winl[ 8];
}
#endif

void ampegdecoder::imdct(float *out, float *in, float *prv, int type)
{
  if (type!=2)
  {
    float tmp[18];

    fdctd18(tmp, in);

    if (type!=3)
    {
      out[ 0] = prv[ 0] - tmp[ 9];
      out[ 1] = prv[ 1] - tmp[10];
      out[ 2] = prv[ 2] - tmp[11];
      out[ 3] = prv[ 3] - tmp[12];
      out[ 4] = prv[ 4] - tmp[13];
      out[ 5] = prv[ 5] - tmp[14];
      out[ 6] = prv[ 6] - tmp[15];
      out[ 7] = prv[ 7] - tmp[16];
      out[ 8] = prv[ 8] - tmp[17];
      out[ 9] = prv[ 9] + tmp[17] * winlql[ 8];
      out[10] = prv[10] + tmp[16] * winlql[ 7];
      out[11] = prv[11] + tmp[15] * winlql[ 6];
      out[12] = prv[12] + tmp[14] * winlql[ 5];
      out[13] = prv[13] + tmp[13] * winlql[ 4];
      out[14] = prv[14] + tmp[12] * winlql[ 3];
      out[15] = prv[15] + tmp[11] * winlql[ 2];
      out[16] = prv[16] + tmp[10] * winlql[ 1];
      out[17] = prv[17] + tmp[ 9] * winlql[ 0];
    }
    else
    {
      out[ 0] = prv[ 0];
      out[ 1] = prv[ 1];
      out[ 2] = prv[ 2];
      out[ 3] = prv[ 3];
      out[ 4] = prv[ 4];
      out[ 5] = prv[ 5];
      out[ 6] = prv[ 6] - tmp[15] * winsql[11];
      out[ 7] = prv[ 7] - tmp[16] * winsql[10];
      out[ 8] = prv[ 8] - tmp[17] * winsql[ 9];
      out[ 9] = prv[ 9] + tmp[17] * winsql[ 8];
      out[10] = prv[10] + tmp[16] * winsql[ 7];
      out[11] = prv[11] + tmp[15] * winsql[ 6];
      out[12] = prv[12] + tmp[14] * winsql[ 5];
      out[13] = prv[13] + tmp[13] * winsql[ 4];
      out[14] = prv[14] + tmp[12] * winsql[ 3];
      out[15] = prv[15] + tmp[11] * winsql[ 2];
      out[16] = prv[16] + tmp[10] * winsql[ 1];
      out[17] = prv[17] + tmp[ 9] * winsql[ 0];
    }
    if (type!=1)
    {
      prv[ 0] =           tmp[ 8] * winlql[ 0];
      prv[ 1] =           tmp[ 7] * winlql[ 1];
      prv[ 2] =           tmp[ 6] * winlql[ 2];
      prv[ 3] =           tmp[ 5] * winlql[ 3];
      prv[ 4] =           tmp[ 4] * winlql[ 4];
      prv[ 5] =           tmp[ 3] * winlql[ 5];
      prv[ 6] =           tmp[ 2] * winlql[ 6];
      prv[ 7] =           tmp[ 1] * winlql[ 7];
      prv[ 8] =           tmp[ 0] * winlql[ 8];
      prv[ 9] =           tmp[ 0];
      prv[10] =           tmp[ 1];
      prv[11] =           tmp[ 2];
      prv[12] =           tmp[ 3];
      prv[13] =           tmp[ 4];
      prv[14] =           tmp[ 5];
      prv[15] =           tmp[ 6];
      prv[16] =           tmp[ 7];
      prv[17] =           tmp[ 8];
    }
    else
    {
      prv[ 0] =           tmp[ 8] * winsql[ 0];
      prv[ 1] =           tmp[ 7] * winsql[ 1];
      prv[ 2] =           tmp[ 6] * winsql[ 2];
      prv[ 3] =           tmp[ 5] * winsql[ 3];
      prv[ 4] =           tmp[ 4] * winsql[ 4];
      prv[ 5] =           tmp[ 3] * winsql[ 5];
      prv[ 6] =           tmp[ 2] * winsql[ 6];
      prv[ 7] =           tmp[ 1] * winsql[ 7];
      prv[ 8] =           tmp[ 0] * winsql[ 8];
      prv[ 9] =           tmp[ 0] * winsql[ 9];
      prv[10] =           tmp[ 1] * winsql[10];
      prv[11] =           tmp[ 2] * winsql[11];
      prv[12] = 0;
      prv[13] = 0;
      prv[14] = 0;
      prv[15] = 0;
      prv[16] = 0;
      prv[17] = 0;
    }
  }
  else
  {
    float tmp[3][6];

    fdctd6(tmp[0], in+0);
    fdctd6(tmp[1], in+1);
    fdctd6(tmp[2], in+2);

    out[ 0] = prv[ 0];
    out[ 1] = prv[ 1];
    out[ 2] = prv[ 2];
    out[ 3] = prv[ 3];
    out[ 4] = prv[ 4];
    out[ 5] = prv[ 5];
    out[ 6] = prv[ 6] - tmp[0][3];
    out[ 7] = prv[ 7] - tmp[0][4];
    out[ 8] = prv[ 8] - tmp[0][5];
    out[ 9] = prv[ 9] + tmp[0][5] *  winsqs[2];
    out[10] = prv[10] + tmp[0][4] *  winsqs[1];
    out[11] = prv[11] + tmp[0][3] *  winsqs[0];
    out[12] = prv[12] + tmp[0][2] *  winsqs[0] - tmp[1][3];
    out[13] = prv[13] + tmp[0][1] *  winsqs[1] - tmp[1][4];
    out[14] = prv[14] + tmp[0][0] *  winsqs[2] - tmp[1][5];
    out[15] = prv[15] + tmp[0][0]              + tmp[1][5] *  winsqs[2];
    out[16] = prv[16] + tmp[0][1]              + tmp[1][4] *  winsqs[1];
    out[17] = prv[17] + tmp[0][2]              + tmp[1][3] *  winsqs[0];

    prv[ 0] =         - tmp[2][3]              + tmp[1][2] *  winsqs[0];
    prv[ 1] =         - tmp[2][4]              + tmp[1][1] *  winsqs[1];
    prv[ 2] =         - tmp[2][5]              + tmp[1][0] *  winsqs[2];
    prv[ 3] =           tmp[2][5] *  winsqs[2] + tmp[1][0];
    prv[ 4] =           tmp[2][4] *  winsqs[1] + tmp[1][1];
    prv[ 5] =           tmp[2][3] *  winsqs[0] + tmp[1][2];
    prv[ 6] =           tmp[2][2] *  winsqs[0];
    prv[ 7] =           tmp[2][1] *  winsqs[1];
    prv[ 8] =           tmp[2][0] *  winsqs[2];
    prv[ 9] =           tmp[2][0];
    prv[10] =           tmp[2][1];
    prv[11] =           tmp[2][2];
    prv[12] = 0;
    prv[13] = 0;
    prv[14] = 0;
    prv[15] = 0;
    prv[16] = 0;
    prv[17] = 0;
  }
}



void ampegdecoder::readsfsi(grsistruct &si)
{
  int i;
  for (i=0; i<4; i++)
    si.sfsi[i]=si.gr?mpgetbit():0;
}

void ampegdecoder::readgrsi(grsistruct &si, int &bitpos)
{
  int i;
  si.grstart=bitpos;
  bitpos+=mpgetbits(12);
  si.grend=bitpos;
  si.regionend[2]=mpgetbits(9)*2;
  si.globalgain=mpgetbits(8);
  if (hdrlsf&&(hdrmode==1)&&(hdrmodeext&1)&&si.ch)
  {
    si.sfcompress=mpgetbits(8);
    si.ktabsel=mpgetbit();
  }
  else
  {
    si.sfcompress=mpgetbits(hdrlsf?9:4);
    si.ktabsel=2;
  }
  if (mpgetbit())
  {
    si.blocktype = mpgetbits(2);
    si.mixedblock = mpgetbit();
    for (i=0; i<2; i++)
      si.tabsel[i]=mpgetbits(5);
    si.tabsel[2]=0;
    for (i=0; i<3; i++)
      si.subblockgain[i]=4*mpgetbits(3);

    if (si.blocktype==2)
      si.regionend[0]=sfbands[hdrlsf][hdrfreq][3];
    else
      si.regionend[0]=sfbandl[hdrlsf][hdrfreq][HUFFMAXLONG];
    si.regionend[1]=576;
  }
  else
  {
    for (i=0; i<3; i++)
      si.tabsel[i]=mpgetbits(5);
    int region0count = mpgetbits(4)+1;
    int region1count = mpgetbits(3)+1+region0count;
    si.regionend[0]=sfbandl[hdrlsf][hdrfreq][region0count];
    si.regionend[1]=sfbandl[hdrlsf][hdrfreq][region1count];
    si.blocktype = 0;
    si.mixedblock = 0;
  }
  if (si.regionend[0]>si.regionend[2])
    si.regionend[0]=si.regionend[2];
  if (si.regionend[1]>si.regionend[2])
    si.regionend[1]=si.regionend[2];
  si.regionend[3]=576;
  si.preflag=hdrlsf?(si.sfcompress>=500)?1:0:mpgetbit();
  si.sfshift=mpgetbit();
  si.tabsel[3]=mpgetbit()?33:32;

  if (si.blocktype)
    for (i=0; i<4; i++)
      si.sfsi[i]=0;
}

int ampegdecoder::huffmandecoder(int *tab)
{
  while (1)
  {
    int v=*tab++;
    if (v>=0)
      return v;
    if (mpgetbit())
      tab-=v;
  }
}

void ampegdecoder::readscalefac(grsistruct &si, int *scalefacl)
{
  *bitbufpos=si.grstart;
  int newslen[4];
  int blocknumber;
  if (!hdrlsf)
  {
    newslen[0]=slentab[0][si.sfcompress];
    newslen[1]=slentab[0][si.sfcompress];
    newslen[2]=slentab[1][si.sfcompress];
    newslen[3]=slentab[1][si.sfcompress];
    blocknumber=6;
  }
  else
    if ((hdrmode!=1)||!(hdrmodeext&1)||!si.ch)
    {
      if (si.sfcompress>=500)
      {
        newslen[0] = ((si.sfcompress-500)/ 3)%4;
        newslen[1] = ((si.sfcompress-500)/ 1)%3;
        newslen[2] = ((si.sfcompress-500)/ 1)%1;
        newslen[3] = ((si.sfcompress-500)/ 1)%1;
        blocknumber = 2;
      }
      else
      if (si.sfcompress>=400)
      {
        newslen[0] = ((si.sfcompress-400)/20)%5;
        newslen[1] = ((si.sfcompress-400)/ 4)%5;
        newslen[2] = ((si.sfcompress-400)/ 1)%4;
        newslen[3] = ((si.sfcompress-400)/ 1)%1;
        blocknumber = 1;
      }
      else
      {
        newslen[0] = ((si.sfcompress-  0)/80)%5;
        newslen[1] = ((si.sfcompress-  0)/16)%5;
        newslen[2] = ((si.sfcompress-  0)/ 4)%4;
        newslen[3] = ((si.sfcompress-  0)/ 1)%4;
        blocknumber = 0;
      }
    }
    else
    {
      if (si.sfcompress>=244)
      {
        newslen[0] = ((si.sfcompress-244)/ 3)%4;
        newslen[1] = ((si.sfcompress-244)/ 1)%3;
        newslen[2] = ((si.sfcompress-244)/ 1)%1;
        newslen[3] = ((si.sfcompress-244)/ 1)%1;
        blocknumber = 5;
      }
      else
      if (si.sfcompress>=180)
      {
        newslen[0] = ((si.sfcompress-180)/16)%4;
        newslen[1] = ((si.sfcompress-180)/ 4)%4;
        newslen[2] = ((si.sfcompress-180)/ 1)%4;
        newslen[3] = ((si.sfcompress-180)/ 1)%1;
        blocknumber = 4;
      }
      else
      {
        newslen[0] = ((si.sfcompress-  0)/36)%5;
        newslen[1] = ((si.sfcompress-  0)/ 6)%6;
        newslen[2] = ((si.sfcompress-  0)/ 1)%6;
        newslen[3] = ((si.sfcompress-  0)/ 1)%1;
        blocknumber = 3;
      }
    }

  int i,k;

  int *sfb=sfbtab[blocknumber][(si.blocktype!=2)?0:si.mixedblock?2:1];
  int *sfp=scalefacl;
  for (i=0;i<4;i++)
    if (!si.sfsi[i])
      for (k=sfb[i]; k<sfb[i+1]; k++)
        *sfp++=mpgetbits(newslen[i]);
    else
      sfp+=sfb[i+1]-sfb[i];
  *sfp++=0;
  *sfp++=0;
  *sfp++=0;
}




void ampegdecoder::readhuffman(grsistruct &si, float *xr)
{
  int *ro=rotab[(si.blocktype!=2)?0:si.mixedblock?2:1];
  int i;
  for (i=0; i<si.regionend[2]; i+=2)
  {
    int t=(i<si.regionend[0])?0:(i<si.regionend[1])?1:2;
    int linbits=htablinbits[si.tabsel[t]];
    int val=huffmandecoder(htabs[si.tabsel[t]]);
    int x;
    double v;
    x=val>>4;
    if (x==15)
      x=15+mpgetbits(linbits);
    v=pow43tab[x];
    if (x)
      if (mpgetbit())
        v=-v;
    xr[ro[i+0]]=v;

    x=val&15;
    if (x==15)
      x=15+mpgetbits(linbits);
    v=pow43tab[x];
    if (x)
      if (mpgetbit())
        v=-v;
    xr[ro[i+1]]=v;
  }

  while ((*bitbufpos<si.grend)&&(i<576))
  {
    int val=huffmandecoder(htabs[si.tabsel[3]]);
    int x;
    x=(val>>3)&1;
    if (x)
      if (mpgetbit())
        x=-x;
    xr[ro[i+0]]=x;
    x=(val>>2)&1;
    if (x)
      if (mpgetbit())
        x=-x;
    xr[ro[i+1]]=x;
    x=(val>>1)&1;
    if (x)
      if (mpgetbit())
        x=-x;
    xr[ro[i+2]]=x;
    x=val&1;
    if (x)
      if (mpgetbit())
        x=-x;
    xr[ro[i+3]]=x;

    i+=4;
  }

  if (*bitbufpos>si.grend)
    i-=4;

  while (i<576)
    xr[ro[i++]]=0;
}


void ampegdecoder::doscale(grsistruct &si, float *xr, int *scalefacl)
{
  int *bil=sfbandl[hdrlsf][hdrfreq];
  int *bis=sfbands[hdrlsf][hdrfreq];
  int largemax=(si.blocktype==2)?si.mixedblock?(hdrlsf?6:8):0:22;
  int smallmin=(si.blocktype==2)?si.mixedblock?3:0:13;

  int j,k,i;
  int *sfp=scalefacl;
  float gain=ggaintab[si.globalgain];
  for (j=0; j<largemax; j++)
  {
    double f=gain*pow2tab[(*sfp++ +(si.preflag?pretab[j]:0))<<si.sfshift];
    for (i=bil[j]; i<bil[j+1]; i++)
      xr[i]*=f;
  }
  float sgain[3];
  if (smallmin!=13)
  {
    for (k=0; k<3; k++)
      sgain[k]=gain*pow2tab[si.subblockgain[k]];
  }
  for (j=smallmin; j<13; j++)
    for (k=0; k<3; k++)
    {
      double f=sgain[k]*pow2tab[*sfp++<<si.sfshift];
      for (i=bis[j]+k; i<bis[j+1]; i+=3)
        xr[i]*=f;
    }
}

void ampegdecoder::jointstereo(grsistruct &si, float (*xr)[576], int *scalefacl)
{
  int i,j;

  if (!hdrmodeext)
    return;
  int max=576>>ratereduce;
  if (hdrmodeext==2)
  {
    for (i=0; i<max; i++)
    {
      double a=xr[0][i];
      xr[0][i] = (a+xr[1][i])*sqrt05;
      xr[1][i] = (a-xr[1][i])*sqrt05;
    }
    return;
  }

  int *bil=sfbandl[hdrlsf][hdrfreq];
  int *bis=sfbands[hdrlsf][hdrfreq];

  for (i=0; i<max; i++)
    ispos[i]=7;

  float (*kt)[2]=ktab[si.ktabsel];

  int sfb;
  int sfbhigh=22;
  int _tmpsfb=0;
  if (si.blocktype==2)
  {
    int sfblow=si.mixedblock?3:0;
    int *scalefacs=scalefacl+(si.mixedblock?hdrlsf?(6-3*3):(8-3*3):0);
    sfbhigh=si.mixedblock?(hdrlsf?6:8):0;

    for (j=0; j<3; j++)
    {
      for (i=bis[13]+j-3; i>=bis[sfblow]; i-=3)
        if (xr[1][i])
          break;
      for (sfb=sfblow; sfb<13; sfb++)
        if (i<bis[sfb])
          break;
      if (sfb>3)
        sfbhigh=0;
      int v=7;
      for (_tmpsfb=sfb; _tmpsfb<13; _tmpsfb++)
      {
        v=(_tmpsfb==12)?v:scalefacs[_tmpsfb*3+j];
        for (i=bis[_tmpsfb]+j; i<bis[_tmpsfb+1]; i+=3)
          ispos[i]=v;
      }
    }
  }
  for (i=bil[sfbhigh]-1; i>=0; i--)
    if (xr[1][i])
      break;
  for (sfb=0; sfb<sfbhigh; sfb++)
    if (i<bil[sfb])
      break;
  int v=7;
  for (_tmpsfb=sfb; _tmpsfb<sfbhigh; _tmpsfb++)
  {
    v=(_tmpsfb==21)?v:scalefacl[_tmpsfb];
    for (i=bil[_tmpsfb]; i<bil[_tmpsfb+1]; i++)
      ispos[i]=v;
  }

  int msstereo=hdrmodeext&2;
  for (i=0; i<max; i++)
  {
    if (ispos[i]==7)
    {
      if (msstereo)
      {
        double a=xr[0][i];
        xr[0][i] = (a+xr[1][i])*sqrt05;
        xr[1][i] = (a-xr[1][i])*sqrt05;
      }
    }
    else
    {
      xr[1][i] = xr[0][i] * kt[ispos[i]][1];
      xr[0][i] = xr[0][i] * kt[ispos[i]][0];
    }
  }
}

void ampegdecoder::hybrid(grsistruct &si, float (*hout)[32], float (*prev)[18], float *xr)
{
  int nbands=32>>ratereduce;
  int lim=(si.blocktype!=2)?(32>>ratereduce):si.mixedblock?(sfbands[hdrlsf][hdrfreq][3]/18):0;

  int sb,ss;
  for (sb=1;sb<lim;sb++)
    for (ss=0;ss<8;ss++)
    {
      float bu = xr[(sb-1)*18+17-ss];
      float bd = xr[sb*18+ss];
      xr[(sb-1)*18+17-ss] = (bu * csatab[ss][0]) - (bd * csatab[ss][1]);
      xr[sb*18+ss] = (bd * csatab[ss][0]) + (bu * csatab[ss][1]);
    }

  if (l3equalon)
  {
    int i;
    int shlim=lim*6;
    for (i=0; i<shlim; i++)
    {
      xr[3*i+0]*=l3equals[i];
      xr[3*i+1]*=l3equals[i];
      xr[3*i+2]*=l3equals[i];
    }
    int nbnd=nbands*18;
    for (i=shlim*3; i<nbnd; i++)
      xr[i]*=l3equall[i];
  }

  int mixlim=si.mixedblock?(sfbands[hdrlsf][hdrfreq][3]/18):0;
  for (sb=0; sb<nbands; sb++)
  {
    float rawout[18];
    imdct(rawout, xr+sb*18, prev[sb], (sb<mixlim)?0:si.blocktype);
    if (sb&1)
      for (ss=0; ss<18; ss+=2)
      {
        hout[ss][sb]=rawout[ss];
        hout[ss+1][sb]=-rawout[ss+1];
      }
    else
      for (ss=0; ss<18; ss++)
        hout[ss][sb]=rawout[ss];
  }
}

void ampegdecoder::readmain(grsistruct (*si)[2])
{
  int stereo=(hdrmode==3)?1:2;

  int maindatabegin = mpgetbits(hdrlsf?8:9);

  mpgetbits(hdrlsf?(stereo==1)?1:2:(stereo==1)?5:3);

  if (!si)
    mpgetbits(hdrlsf?(stereo==1)?64:128:(stereo==1)?127:247);
  else
  {
    int ngr=hdrlsf?1:2;
    int gr,ch;
    for (gr=0; gr<ngr; gr++)
      for (ch=0; ch<stereo; ch++)
      {
        si[ch][gr].ch=ch;
        si[ch][gr].gr=gr;
      }

    for (gr=0; gr<ngr; gr++)
      for (ch=0; ch<stereo; ch++)
        readsfsi(si[ch][gr]);

    int bitpos=0;
      for (gr=0; gr<ngr; gr++)
        for (ch=0; ch<stereo; ch++)
          readgrsi(si[ch][gr], bitpos);
  }

  int mainslots=(hdrlsf?72:144)*1000*ratetab[hdrlsf?1:0][2][hdrbitrate]/(freqtab[hdrfreq]>>hdrlsf)+(hdrpadding?1:0)-4-(hdrcrc?2:0)-(hdrlsf?(stereo==1)?9:17:(stereo==1)?17:32);

  if (huffoffset<maindatabegin)
    huffoffset=maindatabegin;
  memmove(huffbuf, huffbuf+huffoffset-maindatabegin, maindatabegin);
  getbytes(huffbuf+maindatabegin, mainslots);
  huffoffset=maindatabegin+mainslots;
  bitbuf=huffbuf;
  bitbufpos=&huffbit;
}


void ampegdecoder::decode3()
{
  int fr,gr,ch,sb,ss;
  for (fr=0; fr<(hdrlsf?2:1); fr++)
  {
    grsistruct si0[2][2];

    if (fr)
      decodehdr(0);

    if (!hdrbitrate)
    {
      for (gr=fr; gr<2; gr++)
        for (ch=0; ch<2; ch++)
          for (sb=0; sb<32; sb++)
            for (ss=0; ss<18; ss++)
            {
              fraction[ch][gr*18+ss][sb]=((sb&ss&1)?-1:1)*prevblck[ch][sb][ss];
              prevblck[ch][sb][ss]=0;
            }
      return;
    }

    readmain(si0);
    int stereo=(hdrmode==3)?1:2;
    int ngr=hdrlsf?1:2;
    for (gr=0;gr<ngr;gr++)
    {
      for (ch=0; ch<stereo; ch++)
      {
        readscalefac(si0[ch][gr], scalefac0[ch]);
        readhuffman(si0[ch][gr], xr0[ch]);
        doscale(si0[ch][gr], xr0[ch], scalefac0[ch]);
      }

      if (hdrmode==1)
        jointstereo(si0[1][gr], xr0, scalefac0[1]);

      for (ch=0; ch<stereo; ch++)
        hybrid(si0[ch][gr], fraction[ch]+(fr+gr)*18, prevblck[ch], xr0[ch]);
    }
  }
}

void ampegdecoder::seekinit3(int discard)
{
  int i,j,k;
  int extra=(seekmode==seekmodeexact)?1:0;
  if ((discard>=seekinitframes)&&extra)
    for (i=0; i<2; i++)
      for (j=0; j<32; j++)
        for (k=0; k<18; k++)
          prevblck[i][j][k]=0;
  huffoffset=0;
  for (i=discard; i<seekinitframes; i++)
    if (i<(seekinitframes-extra))
      for (j=0; j<(hdrlsf?2:1); j++)
      {
        if (!decodehdr(0))
          return;
        readmain(0);
      }
    else
    {
      if (!decodehdr(0))
        return;
      decode3();
    }
}

void ampegdecoder::setl3equal(const float *buf)
{
  if (!buf)
  {
    l3equalon=0;
    return;
  }
  int i;
  for (i=0; i<576; i++)
    if (buf[i]!=1)
      break;
  if (i==576)
  {
    l3equalon=0;
    return;
  }
  if (ratereduce==0)
    for (i=0; i<576; i++)
      l3equall[i]=buf[i];
  else
  if (ratereduce==1)
    for (i=0; i<288; i++)
      l3equall[i]=(buf[2*i+0]+buf[2*i+1])/2;
  else
    for (i=0; i<144; i++)
      l3equall[i]=(buf[4*i+0]+buf[4*i+1]+buf[4*i+2]+buf[4*i+3])/4;
  for (i=0; i<(192>>ratereduce); i++)
    l3equals[i]=(l3equall[3*i+0]+l3equall[3*i+1]+l3equall[3*i+2])/3;
  l3equalon=1;
}

void ampegdecoder::init3()
{
  int i;
  for (i=0; i<32; i++)
  {
    ktab[0][i][0]=(i&1)?exp(-log(2.0f)*0.5*((i+1)>>1)):1;
    ktab[0][i][1]=(i&1)?1:exp(-log(2.0f)*0.5*(i>>1));
	ktab[1][i][0] = (i & 1) ? exp(-log(2.0f)*0.25*((i + 1) >> 1)) : 1;
	ktab[1][i][1] = (i & 1) ? 1 : exp(-log(2.0f)*0.25*(i >> 1));
    ktab[2][i][0]=sin(_PI/12*i)/(sin(_PI/12*i)+cos(_PI/12*i));
    ktab[2][i][1]=cos(_PI/12*i)/(sin(_PI/12*i)+cos(_PI/12*i));
  }

  for (i=0; i<65; i++)
    pow2tab[i]=exp(log(0.5)*0.5*i);

  for (i=0;i<8;i++)
  {
    csatab[i][0] = 1.0/sqrt(1.0+citab[i]*citab[i]);
    csatab[i][1] = citab[i]*csatab[i][0];
  }

  for (i=0; i<3; i++)
    winsqs[i]=cos(_PI/24*(2*i+1))/sin(_PI/24*(2*i+1));
  for (i=0; i<9; i++)
    winlql[i]=cos(_PI/72*(2*i+1))/sin(_PI/72*(2*i+1));
  for (i=0; i<6; i++)
    winsql[i]=1/sin(_PI/72*(2*i+1));
  for (i=6; i<9; i++)
    winsql[i]=sin(_PI/24*(2*i+1))/sin(_PI/72*(2*i+1));
  for (i=9; i<12; i++)
    winsql[i]=sin(_PI/24*(2*i+1))/cos(_PI/72*(2*i+1));
  for (i=0; i<6; i++)
    sec24wins[i]=0.5/cos(_PI*(2*i+1)/24.0)*sin(_PI/24*(2*i+1)-_PI/4);
  for (i=0; i<18; i++)
    sec72winl[i]=0.5/cos(_PI*(2*i+1)/72.0)*sin(_PI/72*(2*i+1)-_PI/4);
  for (i=0; i<3; i++)
    sec12[i]=0.5/cos(_PI*(2*i+1)/12.0);
  for (i=0; i<9; i++)
    sec36[i]=0.5/cos(_PI*(2*i+1)/36.0);
  for (i=0;i<3;i++)
    cos6[i]=cos(_PI/6*i);
  for (i=0;i<9;i++)
    cos18[i]=cos(_PI/18*i);

  for (i=0;i<256;i++)
    ggaintab[i]=exp(log(0.5)*0.25*(210-i));

  pow43tab[0]=0;
  for (i=1; i<8207; i++)
	  pow43tab[i] = exp(log((float) i) * 4 / 3);

  sqrt05=sqrt(0.5);
}

void ampegdecoder::openlayer3(int rate)
{
  if (rate)
  {
    slotsize=1;
    slotdiv=freqtab[orgfreq]>>orglsf;
    nslots=(144*rate)/(freqtab[orgfreq]>>orglsf);
    fslots=(144*rate)%slotdiv;
    seekinitframes=3+(orglsf?254:510)/(nslots-38);
  }

  int i,j,k;

  for (i=0; i<13; i++)
  {
    int sfbstart=sfbands[orglsf][orgfreq][i];
    int sfblines=(sfbands[orglsf][orgfreq][i+1]-sfbands[orglsf][orgfreq][i])/3;
    for (k=0; k<3; k++)
      for (j=0;j<sfblines;j++)
      {
        rotab[0][sfbstart+k*sfblines+j]=sfbstart+k*sfblines+j;
        rotab[1][sfbstart+k*sfblines+j]=sfbstart+k+j*3;
        rotab[2][sfbstart+k*sfblines+j]=(i<3)?(sfbstart+k*sfblines+j):(sfbstart+k+j*3);
      }
  }

  huffoffset=0;

  for (i=0; i<2; i++)
    for (j=0; j<32; j++)
      for (k=0; k<18; k++)
        prevblck[i][j][k]=0;

  l3equalon=0;
}
