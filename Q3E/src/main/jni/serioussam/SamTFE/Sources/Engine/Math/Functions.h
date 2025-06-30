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

#ifndef SE_INCL_FUNCTIONS_H
#define SE_INCL_FUNCTIONS_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif
 
#include <float.h>

#define _USE_MATH_DEFINES // needed to have definition of M_LOG2E 
#include <math.h>

// asm shortcuts`
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr

/* 
 *  template implementations
 */

template<class Type>
inline Type Abs( const Type x)
{
  return ( x>=Type(0) ? x : -x );
}

template<class Type>
inline Type Max( const Type a, const Type b)
{
  return ( a<b ? b : a );
}

template<class Type>
inline Type Min( const Type a, const Type b)
{
  return ( a>b ? b : a );
}

// linear interpolation
template<class Type>
inline Type Lerp( const Type x0, const Type x1, const FLOAT fRatio)
{
       if( fRatio==0) return x0;
  else if( fRatio==1) return x1;
  else return ((Type) (x0+(x1-x0)*fRatio));
}

template<class Type>
inline Type Sgn( const Type x)
{
  return (x)>Type(0) ? Type(1):( x<0 ? Type(-1):Type(0) );
}

template<class Type>
inline Type SgnNZ( const Type x)
{
  return (x)>=Type(0) ? Type(1):Type(-1);
}

template<class Type>
inline void Swap( Type &a, Type &b)
{
  Type t=a; a=b;  b=t;
} 

template<class Type>
inline Type ClampUp( const Type x, const Type uplimit)
{
  return ( x<=uplimit ? x : uplimit );
}

template<class Type>
inline Type ClampDn( const Type x, const Type dnlimit)
{
  return ( x>=dnlimit ? x : dnlimit );
}

template<class Type>
inline Type Clamp( const Type x, const Type dnlimit, const Type uplimit)
{
  return ( x>=dnlimit ? (x<=uplimit ? x : uplimit): dnlimit );
}

/* 
 *  fast implementations
 */


inline DOUBLE Abs( const DOUBLE f) { return fabs(f); }
inline FLOAT  Abs( const FLOAT f)  { return (FLOAT)fabs(f); }
inline SLONG  Abs( const SLONG sl) { return labs(sl); }


/*
inline FLOAT Min( const FLOAT fA, const FLOAT fB)
{
  FLOAT fRet;
  __asm {
    fld     D [fA]
    fld     D [fB]
    fucomi  st(0),st(1)
    fcmovnb st(0),st(1)
    ffree   st(1)
    fstp    D [fRet]
  }
  return fRet;
}

inline FLOAT Max( const FLOAT fA, const FLOAT fB)
{
  FLOAT fRet;
  __asm {
    fld     D [fA]
    fld     D [fB]
    fucomi  st(0),st(1)
    fcmovb  st(0),st(1)
    ffree   st(1)
    fstp    D [fRet]
  }
  return fRet;
}


inline SLONG Min( const SLONG slA, const SLONG slB)
{
  SLONG slRet;
  __asm {
    mov     eax,D [slA]
    cmp     eax,D [slB]
    cmovg   eax,D [slB]
    mov     D [slRet],eax
  }
  return slRet;
}

inline ULONG Min( const ULONG slA, const ULONG slB)
{
  ULONG ulRet;
  __asm {
    mov     eax,D [slA]
    cmp     eax,D [slB]
    cmova   eax,D [slB]
    mov     D [ulRet],eax
  }
  return ulRet;
}

inline SLONG Max( const SLONG slA, const SLONG slB)
{
  SLONG slRet;
  __asm {
    mov     eax,D [slA]
    cmp     eax,D [slB]
    cmovl   eax,D [slB]
    mov     D [slRet],eax
  }
  return slRet;
}

inline ULONG Max( const ULONG slA, const ULONG slB)
{
  ULONG ulRet;
  __asm {
    mov     eax,D [slA]
    cmp     eax,D [slB]
    cmovb   eax,D [slB]
    mov     D [ulRet],eax
  }
  return ulRet;
}



inline FLOAT ClampUp( const FLOAT f, const FLOAT fuplimit)
{
  FLOAT fRet;
  __asm {
    fld     D [fuplimit]
    fld     D [f]
    fucomi  st(0),st(1)
    fcmovnb st(0),st(1)
    fstp    D [fRet]
    fstp    st(0)
  }
  return fRet;
}

inline FLOAT ClampDn( const FLOAT f, const FLOAT fdnlimit)
{
  FLOAT fRet;
  __asm {
    fld     D [fdnlimit]
    fld     D [f]
    fucomi  st(0),st(1)
    fcmovb  st(0),st(1)
    fstp    D [fRet]
    fstp    st(0)
  }
  return fRet;
}

inline FLOAT Clamp( const FLOAT f, const FLOAT fdnlimit, const FLOAT fuplimit)
{
  FLOAT fRet;
  __asm {
    fld     D [fdnlimit]
    fld     D [fuplimit]
    fld     D [f]
    fucomi  st(0),st(2)
    fcmovb  st(0),st(2)
    fucomi  st(0),st(1)
    fcmovnb st(0),st(1)
    fstp    D [fRet]
    fcompp
  }
  return fRet;
}


inline SLONG ClampDn( const SLONG sl, const SLONG sldnlimit)
{
  SLONG slRet;
  __asm {
    mov     eax,D [sl]
    cmp     eax,D [sldnlimit]
    cmovl   eax,D [sldnlimit]
    mov     D [slRet],eax
  }
  return slRet;
}

inline SLONG ClampUp( const SLONG sl, const SLONG sluplimit)
{
  SLONG slRet;
  __asm {
    mov     eax,D [sl]
    cmp     eax,D [sluplimit]
    cmovg   eax,D [sluplimit]
    mov     D [slRet],eax
  }
  return slRet;
}

inline SLONG Clamp( const SLONG sl, const SLONG sldnlimit, const SLONG sluplimit)
{
  SLONG slRet;
  __asm {
    mov     eax,D [sl]
    cmp     eax,D [sldnlimit]
    cmovl   eax,D [sldnlimit]
    cmp     eax,D [sluplimit]
    cmovg   eax,D [sluplimit]
    mov     D [slRet],eax
  }
  return slRet;
}

*/

/* 
 *  fast functions
 */

#define FP_ONE_BITS  0x3F800000

// fast reciprocal value
inline FLOAT FastRcp( const FLOAT f)
{
  INDEX i = 2*FP_ONE_BITS - *(INDEX*)&(f); 
  FLOAT r = *(FLOAT*)&i;
  return( r * (2.0f - f*r));
}

// convert float from 0.0f to 1.0f -> ulong form 0 to 255
inline ULONG NormFloatToByte( const FLOAT f)
{
    /* rcg10042001 !!! FIXME: Move this elsewhere. */
#ifdef __MSVC_INLINE__
  const FLOAT f255 = 255.0f;
  ULONG ulRet;
  __asm {
    fld   D [f]
    fmul  D [f255]
    fistp D [ulRet]
  }
  return ulRet;
#else
  ASSERT((f >= 0.0f) && (f <= 1.0f));
  return( (ULONG) (f * 255.0f) );
#endif
}

// convert ulong from 0 to 255 -> float form 0.0f to 255.0f
inline FLOAT NormByteToFloat( const ULONG ul)
{
  return (FLOAT)ul * (1.0f/255.0f);
}

// fast float to int conversio _MSC_VER <= 1700
#if defined(_WIN64) && (_MSC_VER <= 1700)
inline SLONG FloatToInt( double d )
{
   union Cast
   {
      double d;
      long l;
    };
   volatile Cast c;
   c.d = d + 6755399441055744.0;
   return c.l;
}
#else
// fast float to int conversion
inline SLONG FloatToInt( FLOAT f)
{
#if (defined __MSVC_INLINE__)
  SLONG slRet;
  __asm {
    fld    D [f]
    fistp  D [slRet]
  }
  return slRet;

#elif (defined __GNU_INLINE_X86_32__)
  SLONG slRet;
  __asm__ __volatile__ (
    "flds     (%%eax)   \n\t"
    "fistpl   (%%esi)   \n\t"
        :
        : "a" (&f), "S" (&slRet)
        : "memory"
  );
  return(slRet);

#else
  // round to nearest by adding/subtracting 0.5 (depending on f pos/neg) before converting to SLONG
  float addToRound = copysignf(0.5f, f); // copy f's signbit to 0.5 => if f<0 then addToRound = -0.5, else 0.5
  return((SLONG) (f + addToRound));

#endif
}
#endif // _MSC_VER <= 1700


// log base 2 of any float numero
inline FLOAT Log2( FLOAT f) {
#if (defined __MSVC_INLINE__)
  FLOAT fRet;
  _asm {
    fld1
    fld     D [f]
    fyl2x
    fstp    D [fRet]
  }
  return fRet;

#elif (defined __GNU_INLINE_X86_32__)
  FLOAT fRet;
  __asm__ __volatile__ (
    "fld1               \n\t"
    "flds     (%%eax)   \n\t"
    "fyl2x              \n\t"
    "fstps    (%%esi)   \n\t"
        :
        : "a" (&f), "S" (&fRet)
        : "memory"
  );
  return(fRet);
#else

#if ((defined(_WIN64) && (_MSC_VER >= 1800)) || defined(PLATFORM_UNIX))
//#pragma message(">> log2f(f); <<")
  return log2f(f);
#else
  /*
  int * const    exp_ptr = reinterpret_cast <int *> (&f);
  int            x = *exp_ptr;
  const int      log_2 = ((x >> 23) & 255) - 128;
  x &= ~(255 << 23);
  x += 127 << 23;
  *exp_ptr = x;
  f = ((-1.0f/3) * f + 2) * f - 2.0f/3;   // (1)
  return (f + log_2);
  */
  return log(f) * M_LOG2E;
#endif  // _MSC_VER >= 1800
   
#endif
}


// returns accurate values only for integers that are power of 2
inline SLONG FastLog2( SLONG x)
{
#if (defined __MSVC_INLINE__)
  SLONG slRet;
  __asm {
    bsr   eax,D [x]
    mov   D [slRet],eax
  }
  return slRet;

#elif (defined __GNU_INLINE_X86_32__)
  SLONG slRet;
  __asm__ __volatile__ (
    "bsrl   %%ecx, %%eax      \n\t"
        : "=a" (slRet)
        : "c" (x)
        : "memory"
  );
  return(slRet);
#elif (defined __GNUC__)
  if(x == 0) return 0; // __builtin_clz() is undefined for 0
  int numLeadingZeros  = __builtin_clz(x);
  return 31 - numLeadingZeros;
#else
  register SLONG val = x;
  register SLONG retval = 31;
  while (retval > 0)
  {
    if (val & (1 << retval))
        return retval;
    retval--;
  }

  return 0;
#endif
}

/* DG: function is unused => doesn't matter that portable implementation is not optimal :)
// returns log2 of first larger value that is a power of 2
inline SLONG FastMaxLog2( SLONG x)
{ 
#if (defined __MSVC_INLINE__)
  SLONG slRet;
  __asm {
    bsr   eax,D [x]
    bsf   edx,D [x]
    cmp   edx,eax
    adc   eax,0
    mov   D [slRet],eax
  }
  return slRet;

#elif (defined __GNU_INLINE_X86_32__)
  SLONG slRet;
  __asm__ __volatile__ (
    "bsrl  %%ecx, %%eax     \n\t"
    "bsfl  %%ecx, %%edx     \n\t"
    "cmpl  %%eax, %%edx       \n\t"
    "adcl  $0, %%eax          \n\t"
        : "=a" (slRet)
        : "c" (x)
        : "memory"
  );
  return(slRet);
#else
printf("CHECK THIS: %s:%d\n", __FILE__, __LINE__);
  return((SLONG) log2((double) x));

#endif
}
*/



// square root (works with negative numbers)
#ifdef __arm__
inline FLOAT Sqrt( FLOAT x) { return sqrtf( ClampDn( x, 0.0f)); }
#else
inline FLOAT Sqrt( FLOAT x) { return (FLOAT)sqrt( ClampDn( x, 0.0f)); }
#endif



/*
 * Trigonometrical functions
 */

//#define ANGLE_MASK 0x3fff
#define ANGLE_SNAP (0.25f)   //0x0010
// Wrap angle to be between 0 and 360 degrees
inline ANGLE WrapAngle(ANGLE a) {
  return (ANGLE) fmod( fmod(a,360.0f) + 360.0f, 360.0f);  // 0..360
}

// Normalize angle to be between -180 and +180 degrees
inline ANGLE NormalizeAngle(ANGLE a) {
  return WrapAngle(a+ANGLE_180)-ANGLE_180;
}

// math constants
static const FLOAT PI = FLOAT(3.14159265359);

// convert degrees into angle
inline ANGLE AngleDeg(FLOAT fDegrees) {
  //return ANGLE (fDegrees*ANGLE_180/FLOAT(180.0));
  return fDegrees;
}
// convert radians into angle
inline ANGLE AngleRad(FLOAT fRadians) {
  return ANGLE (fRadians*ANGLE_180/PI);
}
// convert radians into angle
inline ANGLE AngleRad(DOUBLE dRadians) {
  return ANGLE (dRadians*ANGLE_180/PI);
}
// convert angle into degrees
inline FLOAT DegAngle(ANGLE aAngle) {
  //return FLOAT (WrapAngle(aAngle)*FLOAT(180.0)/ANGLE_180);
  return WrapAngle(aAngle);
}
// convert angle into radians
inline FLOAT RadAngle(ANGLE aAngle) {
  return FLOAT (WrapAngle(aAngle)*PI/ANGLE_180);
}

#ifdef __arm__
inline ENGINE_API FLOAT Sin(ANGLE a) { return sinf(a*(PI/ANGLE_180)); };
inline ENGINE_API FLOAT Cos(ANGLE a) { return cosf(a*(PI/ANGLE_180)); };
inline ENGINE_API FLOAT Tan(ANGLE a) { return tanf(a*(PI/ANGLE_180)); };
#else
inline ENGINE_API FLOAT Sin(ANGLE a) { return (FLOAT)sin(a*(PI/ANGLE_180)); };
inline ENGINE_API FLOAT Cos(ANGLE a) { return (FLOAT)cos(a*(PI/ANGLE_180)); };
inline ENGINE_API FLOAT Tan(ANGLE a) { return (FLOAT)tan(a*(PI/ANGLE_180)); };
#endif

#ifdef __arm__
inline ENGINE_API FLOAT SinFast(ANGLE a) { return sinf(a*(PI/ANGLE_180)); };
inline ENGINE_API FLOAT CosFast(ANGLE a) { return cosf(a*(PI/ANGLE_180)); };
inline ENGINE_API FLOAT TanFast(ANGLE a) { return tanf(a*(PI/ANGLE_180)); };
inline ANGLE ASin(FLOAT y) {
  return AngleRad (asinf(Clamp(y, -1.0f, 1.0f)));
}
inline ANGLE ACos(FLOAT x) {
  return AngleRad (acosf(Clamp(x, -1.0f, 1.0f)));

}
inline ANGLE ATan(FLOAT z) {
  return AngleRad (atanf(z));
}
inline ANGLE ATan2(FLOAT y, FLOAT x) {
  return AngleRad (atan2f(y, x));
}
#else
inline ENGINE_API FLOAT SinFast(ANGLE a) { return (FLOAT)sin(a*(PI/ANGLE_180)); };
inline ENGINE_API FLOAT CosFast(ANGLE a) { return (FLOAT)cos(a*(PI/ANGLE_180)); };
inline ENGINE_API FLOAT TanFast(ANGLE a) { return (FLOAT)tan(a*(PI/ANGLE_180)); };
inline ANGLE ASin(FLOAT y) {
  return AngleRad (asin(Clamp(y, -1.0f, 1.0f)));
}
inline ANGLE ACos(FLOAT x) {
  return AngleRad (acos(Clamp(x, -1.0f, 1.0f)));
}
inline ANGLE ATan(FLOAT z) {
  return AngleRad (atan(z));
}
inline ANGLE ATan2(FLOAT y, FLOAT x) {
  return AngleRad (atan2(y, x));
}
#endif

inline ANGLE ASin(DOUBLE y) {
  return AngleRad (asin(Clamp(y, -1.0, 1.0)));
}
inline ANGLE ACos(DOUBLE x) {
  return AngleRad (acos(Clamp(x, -1.0, 1.0)));
}
inline ANGLE ATan(DOUBLE z) {
  return AngleRad (atan(z));
}
inline ANGLE ATan2(DOUBLE y, DOUBLE x) {
  return AngleRad (atan2(y, x));
}

// does "snap to grid" for given coordinate
ENGINE_API void Snap( FLOAT &fDest, FLOAT fStep);
ENGINE_API void Snap( DOUBLE &fDest, DOUBLE fStep);
// does "snap to grid" for given angle
//ENGINE_API void Snap( ANGLE &angDest, ANGLE angStep);


/* 
 *  linear interpolation, special functions for floats and angles
 */

inline FLOAT LerpFLOAT(FLOAT f0, FLOAT f1, FLOAT fFactor)
{
  return f0+(f1-f0)*fFactor;
}

inline ANGLE LerpANGLE(ANGLE a0, ANGLE a1, FLOAT fFactor)
{
  // calculate delta
  ANGLE aDelta = WrapAngle(a1)-WrapAngle(a0);
  // adjust delta not to wrap around 360
  if (aDelta>ANGLE_180) {
    aDelta-=ANGLE(ANGLE_360);
  } else if (aDelta<-ANGLE_180) {
    aDelta+=ANGLE(ANGLE_360);
  }
  // interpolate the delta
  return a0+ANGLE(fFactor*aDelta);
}

// Calculates ratio function /~~\ where 0<x<1, taking in consideration fade in and fade out percentages
// (ie. 0.2f means 20% fade in, 0.1f stands for 10% fade out)
inline FLOAT CalculateRatio(FLOAT fCurr, FLOAT fMin, FLOAT fMax, FLOAT fFadeInRatio, FLOAT fFadeOutRatio)
{
  if(fCurr<=fMin || fCurr>=fMax)
  {
    return 0.0f;
  }
  FLOAT fDelta = fMax-fMin;
  FLOAT fRatio=(fCurr-fMin)/fDelta;
  if(fRatio<fFadeInRatio) {
    fRatio = Clamp( fRatio/fFadeInRatio, 0.0f, 1.0f);
  } else if(fRatio>(1-fFadeOutRatio)) {
    fRatio = Clamp( (1.0f-fRatio)/fFadeOutRatio, 0.0f, 1.0f);
  } else {
    fRatio = 1.0f;
  }
  return fRatio;
}


#undef O
#undef Q
#undef D
#undef W
#undef B

#endif  /* include-once check. */


