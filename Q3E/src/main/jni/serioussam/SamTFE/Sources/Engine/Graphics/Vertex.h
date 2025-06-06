/* Copyright (c) 2002-2012 Croteam Ltd. 
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

#ifndef SE_INCL_VERTEX_H
#define SE_INCL_VERTEX_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif


#include "Color.h"

// !!! FIXME: rcg11162001 I have the structures packed to assure positioning.
// !!! FIXME: rcg11162001 This should be fixed on win32, and then this
// !!! FIXME: rcg11162001 ifndef should be removed.
#ifndef PLATFORM_WIN32
#pragma pack(1)
#endif

struct GFXVertex3
{
  FLOAT x,y,z;
};


struct GFXNormal3
{
  FLOAT nx,ny,nz;
};


struct GFXTexCoord
{
	union {
		struct { FLOAT u, v; } uv;
		struct { FLOAT s, t; } st;
	};
};


struct GFXTexCoord4
{
  FLOAT s,t,r,q;
};


struct GFXColor
{
	union {
		struct { UBYTE r, g, b, a; } ub;
		struct { ULONG abgr; } ul;  // reverse order - use ByteSwap()!
	};

  GFXColor() {};

/*
 * rcg10052001 This is a REALLY bad idea;
 *  never rely on the memory layout of even a
 *  simple class. It works for MSVC, though,
 *  so we'll keep it.
 */
#if (defined __MSVC_INLINE__)
  GFXColor( COLOR col) {
    _asm mov   ecx,dword ptr [this]
    _asm mov   eax,dword ptr [col]
    _asm bswap eax
    _asm mov   dword ptr [ecx],eax
  }

  __forceinline void Set( COLOR col) {
    _asm mov   ecx,dword ptr [this]
    _asm mov   eax,dword ptr [col]
    _asm bswap eax
    _asm mov   dword ptr [ecx],eax
  }
#else
  GFXColor( COLOR col) { ul.abgr = ByteSwap(col); }
  __forceinline void Set( COLOR col) { ul.abgr = ByteSwap(col); }
#endif

  void MultiplyRGBA( const GFXColor &col1, const GFXColor &col2) {
    ub.r = (ULONG(col1.ub.r)*col2.ub.r)>>8;
    ub.g = (ULONG(col1.ub.g)*col2.ub.g)>>8;
    ub.b = (ULONG(col1.ub.b)*col2.ub.b)>>8;
    ub.a = (ULONG(col1.ub.a)*col2.ub.a)>>8;
  }

  void MultiplyRGB( const GFXColor &col1, const GFXColor &col2) {
    ub.r = (ULONG(col1.ub.r)*col2.ub.r)>>8;
    ub.g = (ULONG(col1.ub.g)*col2.ub.g)>>8;
    ub.b = (ULONG(col1.ub.b)*col2.ub.b)>>8;
  }

  void MultiplyRGBCopyA1( const GFXColor &col1, const GFXColor &col2) {
    ub.r = (ULONG(col1.ub.r)*col2.ub.r)>>8;
    ub.g = (ULONG(col1.ub.g)*col2.ub.g)>>8;
    ub.b = (ULONG(col1.ub.b)*col2.ub.b)>>8;
    ub.a = col1.ub.a;
  }

  void AttenuateRGB( ULONG ulA) {
    ub.r = (ULONG(ub.r)*ulA)>>8;
    ub.g = (ULONG(ub.g)*ulA)>>8;
    ub.b = (ULONG(ub.b)*ulA)>>8;
  }

  void AttenuateA( ULONG ulA) {
    ub.a = (ULONG(ub.a)*ulA)>>8;
  }
};


#define GFXVertex GFXVertex4

#if (defined _MSC_VER)
struct GFXVertex4
{
  GFXVertex4()
  {
  }
  FLOAT x,y,z;
  union {
    struct { struct GFXColor col; };
    struct { SLONG shade; };
  };
};
#else
/*
 * rcg10042001 Removed the union; objects with constructors can't be
 *  safely unioned, and there's not a whole lot of memory lost here anyhow.
 */

// IF YOU CHANGE THIS STRUCT, YOU WILL BREAK THE INLINE ASSEMBLY
//  ON GNU PLATFORMS! THIS INCLUDES CHANGING THE STRUCTURE'S PACKING.
//  You have been warned.
struct GFXVertex4 {
  FLOAT x,y,z;
  struct GFXColor col;
  SLONG shade;
  void Clear(void) {};
};
#endif


#define GFXNormal GFXNormal4
struct GFXNormal4
{
  FLOAT nx,ny,nz;
  ULONG ul;
};


// !!! FIXME: rcg11162001 I have the structures packed to assure positioning.
// !!! FIXME: rcg11162001 This should be fixed on win32, and then this
// !!! FIXME: rcg11162001 ifndef should be removed.
#ifndef PLATFORM_WIN32
#pragma pack()
#endif

#endif  /* include-once check. */

