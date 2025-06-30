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

#ifndef __PTYPES_H
#define __PTYPES_H

#ifdef _MSC_VER
#pragma warning (disable: 4244) // conversion from 'double' to 'float', possible loss of data
#pragma warning (disable: 4305) // truncation from 'const double' to 'float'
#endif

typedef signed char int1;
typedef unsigned char uint1;
typedef signed short int2;
typedef unsigned short uint2;
typedef signed int int4;
typedef unsigned int uint4;
#ifdef __WATCOMC__
#if __WATCOMC__>=1100
#define INT8
typedef signed __int64 int8;
typedef unsigned __int64 uint8;
#endif
#else
#ifdef _MSC_VER
#define INT8
typedef signed __int64 int8;
typedef unsigned __int64 uint8;
#else
#define INT8
typedef signed long long int8;
typedef unsigned long long uint8;
#endif
#endif
typedef signed int intm;    // fast, but at least 4
typedef unsigned int uintm;
#ifdef INT8
typedef int8 intm8;         // fast, but at least 8
typedef uint8 uintm8;
#endif
typedef intm boolm;    // zero:false, nonzero:true
typedef int1 bool1;   // zero:false, nonzero:true
typedef intm errstat; // negative:error, nonnegative:retval, zero:ok, positive:warning
typedef float float4;
typedef double float8;
#ifdef __WATCOMC__
typedef double floatmax;
#else
typedef long double float10;
typedef long double floatmax;
#endif
typedef float floatm;

inline intm normbool(boolm x) { return x?1:0; } // false:0, true:1

inline uint2 swaplb2(uint2 v) { return (v<<8)|(v>>8); }
inline uint4 swaplb4(uint4 v) { return ((v&0xFF)<<24)|((v&0xFF00)<<8)|((v>>8)&0xFF00)|((v>>24)&0xFF); }
#ifdef INT8
inline uint8 swaplb8(uint8 v)
{
  return ((v&0xFF)<<56)
        |((v&0xFF00)<<40)
        |((v&0xFF0000)<<24)
        |((v&0xFF000000)<<8)
        |((v>> 8)&0xFF000000)
        |((v>>24)&0xFF0000)
        |((v>>40)&0xFF00)
        |((v>>56)&0xFF);
}
#endif

#if 0
class intl2
{
private:
  uint1 i[2];
public:
  intl2() {}
  intl2(int2 v) { i[1]=v>>8; i[0]=v; }
  operator int2() { return (i[1]<<8)|i[0]; }
};

class uintl2
{
private:
  uint1 i[2];
public:
  uintl2() {}
  uintl2(uint2 v) { i[1]=v>>8; i[0]=v; }
  operator uint2() { return (i[1]<<8)|i[0]; }
};

class intb2
{
private:
  uint1 i[2];
public:
  intb2() {}
  intb2(int2 v) { i[2]=v>>8; i[3]=v; }
  operator int2() { return (i[2]<<8)|i[3]; }
};

class uintb2
{
private:
  uint1 i[2];
public:
  uintb2() {}
  uintb2(uint2 v) { i[2]=v>>8; i[3]=v; }
  operator uint2() { return (i[2]<<8)|i[3]; }
};

class intl4
{
private:
  uint1 i[4];
public:
  intl4() {}
  intl4(int4 v) { i[3]=v>>24; i[2]=v>>16; i[1]=v>>8; i[0]=v; }
  operator int4() { return (i[3]<<24)|(i[2]<<16)|(i[1]<<8)|i[0]; }
};

class uintl4
{
private:
  uint1 i[4];
public:
  uintl4() {}
  uintl4(uint4 v) { i[3]=v>>24; i[2]=v>>16; i[1]=v>>8; i[0]=v; }
  operator uint4() { return (i[3]<<24)|(i[2]<<16)|(i[1]<<8)|i[0]; }
};

class intb4
{
private:
  uint1 i[4];
public:
  intb4() {}
  intb4(int4 v) { i[0]=v>>24; i[1]=v>>16; i[2]=v>>8; i[3]=v; }
  operator int4() { return (i[0]<<24)|(i[1]<<16)|(i[2]<<8)|i[3]; }
};

class uintb4
{
private:
  uint1 i[4];
public:
  uintb4() {}
  uintb4(uint4 v) { i[0]=v>>24; i[1]=v>>16; i[2]=v>>8; i[3]=v; }
  operator uint4() { return (i[0]<<24)|(i[1]<<16)|(i[2]<<8)|i[3]; }
};

#ifdef INT8
class intl8
{
private:
  uint1 i[8];
public:
  intl8() {}
  intl8(int8 v) { i[7]=v>>56; i[6]=v>>48; i[5]=v>>40; i[4]=v>>32; i[3]=v>>24; i[2]=v>>16; i[1]=v>>8; i[0]=v; }
  operator int8() { return ((uint8)i[7]<<56)|((uint8)i[6]<<48)|((uint8)i[5]<<40)|((uint8)i[4]<<32)|(i[3]<<24)|(i[2]<<16)|(i[1]<<8)|i[0]; }
};

class uintl8
{
private:
  uint1 i[8];
public:
  uintl8() {}
  uintl8(uint8 v) { i[7]=v>>56; i[6]=v>>48; i[5]=v>>40; i[4]=v>>32; i[3]=v>>24; i[2]=v>>16; i[1]=v>>8; i[0]=v; }
  operator uint8() { return ((uint8)i[7]<<56)|((uint8)i[6]<<48)|((uint8)i[5]<<40)|((uint8)i[4]<<32)|(i[3]<<24)|(i[2]<<16)|(i[1]<<8)|i[0]; }
};

class intb8
{
private:
  uint1 i[8];
public:
  intb8() {}
  intb8(int8 v) { i[0]=v>>56; i[1]=v>>48; i[2]=v>>40; i[3]=v>>32; i[4]=v>>24; i[5]=v>>16; i[6]=v>>8; i[7]=v; }
  operator int8() { return ((uint8)i[0]<<56)|((uint8)i[1]<<48)|((uint8)i[2]<<40)|((uint8)i[3]<<32)|(i[4]<<24)|(i[5]<<16)|(i[6]<<8)|i[7]; }
};

class uintb8
{
private:
  uint1 i[8];
public:
  uintb8() {}
  uintb8(uint8 v) { i[0]=v>>56; i[1]=v>>48; i[2]=v>>40; i[3]=v>>32; i[4]=v>>24; i[5]=v>>16; i[6]=v>>8; i[7]=v; }
  operator uint8() { return ((uint8)i[0]<<56)|((uint8)i[1]<<48)|((uint8)i[2]<<40)|((uint8)i[3]<<32)|(i[4]<<24)|(i[5]<<16)|(i[6]<<8)|i[7]; }
};
#endif

#else

#ifndef BIGENDIAN

#ifdef __WATCOMC__
#pragma aux swaplb2 parm [ax] value [ax] = "xchg al,ah"
#pragma aux swaplb4 parm [eax] value [eax] = "bswap eax"
#ifdef INT8
#pragma aux swaplb8 parm [eax edx] value [eax edx] = "bswap eax" "bswap edx" "xchg eax,edx"
#endif
#endif

typedef int2 intl2;
typedef uint2 uintl2;
typedef int4 intl4;
typedef uint4 uintl4;
#ifdef INT8
typedef int8 intl8;
typedef uint8 uintl8;
#endif

class intb2
{
private:
  int2 i;
public:
  intb2() {}
  intb2(int2 v) { i=swaplb2(v); }
  operator int2() { return swaplb2(i); }
};

class uintb2
{
private:
  uint2 i;
public:
  uintb2() {}
  uintb2(uint2 v) { i=swaplb2(v); }
  operator uint2() { return swaplb2(i); }
};

class intb4
{
private:
  int4 i;
public:
  intb4() {}
  intb4(int4 v) { i=swaplb4(v); }
  operator int4() { return swaplb4(i); }
};

class uintb4
{
private:
  uint4 i;
public:
  uintb4() {}
  uintb4(uint4 v) { i=swaplb4(v); }
  operator uint4() { return swaplb4(i); }
};

#ifdef INT8
class intb8
{
private:
  int8 i;
public:
  intb8() {}
  intb8(int8 v) { i=swaplb8(v); }
  operator int8() { return swaplb8(i); }
};

class uintb8
{
private:
  uint8 i;
public:
  uintb8() {}
  uintb8(uint8 v) { i=swaplb8(v); }
  operator uint8() { return swaplb8(i); }
};
#endif

#else

typedef int2 intb2;
typedef uint2 uintb2;
typedef int4 intb4;
typedef uint4 uintb4;
typedef int8 intb8;
typedef uint8 uintb8;

class intl2
{
private:
  int2 i;
public:
  intl2() {}
  intl2(int2 v) { i=swaplb2(v); }
  operator int2() { return swaplb2(i); }
};

class uintl2
{
private:
  uint2 i;
public:
  uintl2() {}
  uintl2(uint2 v) { i=swaplb2(v); }
  operator uint2() { return swaplb2(i); }
};

class intl4
{
private:
  int4 i;
public:
  intl4() {}
  intl4(int4 v) { i=swaplb4(v); }
  operator int4() { return swaplb4(i); }
};

class uintl4
{
private:
  uint4 i;
public:
  uintl4() {}
  uintl4(uint4 v) { i=swaplb4(v); }
  operator uint4() { return swaplb4(i); }
};

#ifdef INT8
class intl8
{
private:
  int8 i;
public:
  intl8() {}
  intl8(int8 v) { i=swaplb8(v); }
  operator int8() { return swaplb8(i); }
};

class uintl8
{
private:
  uint8 i;
public:
  uintl8() {}
  uintl8(uint8 v) { i=swaplb8(v); }
  operator uint8() { return swaplb8(i); }
};
#endif

#endif
#endif

#ifdef __WATCOMC__
void store8710(void *, floatm);
#pragma aux store8710 parm [eax] [8087] = "fstp tbyte ptr [eax]"
floatm load8710(void *);
#pragma aux load8710 parm [eax] value [8087] = "fld tbyte ptr [eax]"

class float10
{
private:
  char buf[10];
public:
  float10() {}
  float10(floatm v) { store8710(buf,v); }
  operator floatm() { return load8710(buf); }
};
#endif

typedef float4 float874;
typedef float8 float878;
typedef float10 float8710;

#endif
