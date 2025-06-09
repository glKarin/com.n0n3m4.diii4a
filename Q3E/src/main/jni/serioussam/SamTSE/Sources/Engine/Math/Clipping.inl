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

#ifndef SE_INCL_CLIPPING_INL
#define SE_INCL_CLIPPING_INL
#ifdef PRAGMA_ONCE
  #pragma once
#endif

/*
 * Line clipping flags
 */
#define LCF_REMOVED   (0x00L)  // the entire edge is invisible
#define LCF_UNCLIPPED (0x80L)  // this point remains unclipped
#define LCF_NEAR      (0x01L)  // this point is clipped at near plane
#define LCF_FAR       (0x02L)  // this point is clipped at far plane
#define LCF_LEFT      (0x04L)  // this point is clipped at left plane
#define LCF_RIGHT     (0x08L)  // this point is clipped at right plane
#define LCF_TOP       (0x10L)  // this point is clipped at top plane
#define LCF_BOTTOM    (0x20L)  // this point is clipped at bottom plane

#define LCF_EDGEREMOVED (0x0000L)   // used for testing if entire edge is removed
// shifts used for clip flags for start/end vertex
#define LCS_VERTEX0 (0)
#define LCS_VERTEX1 (8)
// masks used for clip flags for start/end vertex
#define LCM_VERTEX0 (0x00FFL)
#define LCM_VERTEX1 (0xFF00L)
// creating line clip flags for start/end vertex
#define LCFVERTEX0(lcf) ((lcf)<<LCS_VERTEX0)
#define LCFVERTEX1(lcf) ((lcf)<<LCS_VERTEX1)

// asm shortcuts
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr

/*
 * Intersecting object (works on edges)
 */
class ENGINE_API CIntersector {
private:
  FLOAT ci_fX0;   // relative coordinates of point
  FLOAT ci_fY0;
  INDEX ci_ct;
public:
  // constructor
  inline CIntersector(FLOAT fX0=0.0f, FLOAT fY0=0.0f)
    : ci_fX0(fX0), ci_fY0(fY0), ci_ct(0) {};

  inline void Clear(void) { ci_ct = 0;}; // Clears intersection count
  inline void AddEdge( FLOAT fedgx1, FLOAT fedgy1, FLOAT fedgx2, FLOAT fedgy2); // Checks for intersection
  inline BOOL IsIntersecting() { return (ci_ct % 2) != 0; };  // Do we have intersection?
};

// inline functions implementation

/////////////////////////////////////////////////////////////////////
//  CIntersector
/////////////////////////////////////////////////////////////////////
/*
 * Checks for intersection of edge with +x axis
 */
ENGINE_API inline void CIntersector::AddEdge( FLOAT fedgx1, FLOAT fedgy1, FLOAT fedgx2, FLOAT fedgy2)
{
  // transform edge relative to the origin
  fedgx1-=ci_fX0; fedgy1-=ci_fY0;
  fedgx2-=ci_fX0; fedgy2-=ci_fY0;

  if( fedgy1 > 0)
  {
    if( fedgy2 > 0) return;

    if( fedgx1 <= 0)
    {
      if( fedgx2 <= 0) return;
    }
    else if( fedgx2 > 0)
    {
      ci_ct ++;
      return;
    }
  }
  else
  {
    if( fedgy2 <= 0) return;

    if( fedgx1 <= 0)
    {
      if( fedgx2 <= 0) return;
    }
    else if( fedgx2 > 0)
    {
      ci_ct ++;
      return;
    }
  }
  // here we calculate x coordinate of edge and +x axis intersection point
  FLOAT a, b;
  a = (fedgy2 - fedgy1)/(fedgx2 - fedgx1);
  b = fedgy1 - a*fedgx1;
  if( -b/a < 0) return;     // no intersection, intersection coordinate x is left from 0
  ci_ct ++;
}

/* rcg10042001 !!! FIXME */
#if (defined __MSVC_INLINE__) && (defined  PLATFORM_32BIT)
  #define ASMOPT 1
#endif

// how much are the clip planes offset inside frustum
#define CLIPPLANE_EPSILON (1E-3f)

/*
 * Clip a line by a single plane -- helper function.
 */
inline BOOL ClipLineByNearPlane(FLOAT3D &v0, FLOAT3D &v1, FLOAT fPlaneDistance, 
                            ULONG &ulCode0, ULONG &ulCode1, ULONG ulCodeClip)
{
#if ASMOPT
  static FLOAT f1=1;
  static FLOAT fDistance0, fDistance1;
  __asm {
    mov     esi,D [v0]
    mov     edi,D [v1]

    fld     D [fPlaneDistance]
    fchs

    fld     D [esi+ 8]
    fsubr   st(0),st(1)
    fld     D [edi+ 8]
    fsubp   st(2),st(0)
    fstp    D [fDistance0]
    fst     D [fDistance1]

    fsubr   D [fDistance0]
    fdivr   D [f1]

    cmp     D [fDistance0],0
    jg      firstFront


;firstBack:
    cmp     D [fDistance1],0
    jle     falseRet

;secondFront:
    mov     eax,D [fPlaneDistance]
    mov     ebx,D [ulCode0]
    xor     eax,80000000h
    mov     edx,D [ulCodeClip]
    mov     D [esi+ 8],eax
    mov     D [ebx],edx

    fmul    D [fDistance0]
    ;//st0=fFactor
    fld     D [esi+ 0]
    fsub    D [edi+ 0]
    fld     D [esi+ 4]
    fsub    D [edi+ 4]
    ;//st0=v0(2)-v1(2), st1=v0(1)-v1(1), st2=fFactor
    fxch    st(1)
    fmul    st(0),st(2)
    fxch    st(1)
    fmulp   st(2),st(0)
    ;//st0=(v0(1)-v1(1))*fFactor, st1=(v0(2)-v1(2))*fFactor
    fsubr   D [esi+ 0]
    fxch    st(1)
    fsubr   D [esi+ 4]
    fxch    st(1)
    fstp    D [esi+ 0]
    fst     D [esi+ 4]
    jmp     trueRet

 firstFront:
    cmp     D [fDistance1],0
    jg      trueRet

;secondBack:
    mov     eax,D [fPlaneDistance]
    mov     ebx,D [ulCode1]
    mov     edx,D [ulCodeClip]
    xor     eax,80000000h
    shl     edx,8
    mov     D [edi+ 8],eax
    mov     D [ebx],edx
    
    fmul    D [fDistance1]
    ;//st0=fFactor
    fld     D [esi+ 0]
    fsub    D [edi+ 0]
    fld     D [esi+ 4]
    fsub    D [edi+ 4]
    ;//st0=v0(2)-v1(2), st1=v0(1)-v1(1), st2=fFactor
    fxch    st(1)
    fmul    st(0),st(2)
    fxch    st(1)
    fmulp   st(2),st(0)
    ;//st0=(v0(1)-v1(1))*fFactor, st1=(v0(2)-v1(2))*fFactor
    fsubr   D [edi+ 0]
    fxch    st(1)
    fsubr   D [edi+ 4]
    fxch    st(1)
    fstp    D [edi+ 0]
    fst     D [edi+ 4]
  }
trueRet:
  _asm fstp st(0)
  return TRUE;
falseRet:
  _asm fstp st(0)
  return FALSE;
#else
  // calculate point distances from clip plane
  FLOAT fDistance0 = -fPlaneDistance-v0(3);
  FLOAT fDistance1 = -fPlaneDistance-v1(3);
  if (fDistance0<=0) {
    // if both are back
    if (fDistance1<=0) {
      // no line remains
      return FALSE;
    // if first is back, second front
    } else {
      // clip first
      FLOAT fDivisor = 1.0f/(fDistance0-fDistance1);
      FLOAT fFactor = fDistance0*fDivisor;
      v0(1) = v0(1)-(v0(1)-v1(1))*fFactor;
      v0(2) = v0(2)-(v0(2)-v1(2))*fFactor;
      v0(3) = -fPlaneDistance;
      // mark that first was clipped
      ulCode0 = LCFVERTEX0(ulCodeClip);
      // line remains
      return TRUE;
    }
  } else {
    // if first is front, second back
    if (fDistance1<=0) {
      // clip second
      FLOAT fDivisor = 1.0f/(fDistance0-fDistance1);
      FLOAT fFactor = fDistance1*fDivisor;
      v1(1) = v1(1)-(v0(1)-v1(1))*fFactor;
      v1(2) = v1(2)-(v0(2)-v1(2))*fFactor;
      v1(3) = -fPlaneDistance;
      // mark that second was clipped
      ulCode1 = LCFVERTEX1(ulCodeClip);
      // line remains
      return TRUE;
    // if both are front
    } else {
      // line remains unclipped
      return TRUE;
    }
  }
#endif
}

/*
 * Clip a line by a single plane -- helper function.
 */
inline BOOL ClipLineByFarPlane(FLOAT3D &v0, FLOAT3D &v1, FLOAT fPlaneDistance, 
                            ULONG &ulCode0, ULONG &ulCode1, ULONG ulCodeClip)
{
#if ASMOPT
  static FLOAT f1=1;
  static FLOAT fDistance0, fDistance1;
  __asm {
    mov     esi,D [v0]
    mov     edi,D [v1]

    fld     D [fPlaneDistance]
    fadd    D [esi+ 8]
    fld     D [fPlaneDistance]
    fadd    D [edi+ 8]
    fxch    st(1)
    fstp    D [fDistance0]
    fst     D [fDistance1]

    fsubr   D [fDistance0]
    fdivr   D [f1]

    cmp     D [fDistance0],0
    jg      firstFront


;firstBack:
    cmp     D [fDistance1],0
    jle     falseRet

;secondFront:
    mov     eax,D [fPlaneDistance]
    mov     ebx,D [ulCode0]
    xor     eax,80000000h
    mov     edx,D [ulCodeClip]
    mov     D [esi+ 8],eax
    mov     D [ebx],edx

    fmul    D [fDistance0]
    ;//st0=fFactor
    fld     D [esi+ 0]
    fsub    D [edi+ 0]
    fld     D [esi+ 4]
    fsub    D [edi+ 4]
    ;//st0=v0(2)-v1(2), st1=v0(1)-v1(1), st2=fFactor
    fxch    st(1)
    fmul    st(0),st(2)
    fxch    st(1)
    fmulp   st(2),st(0)
    ;//st0=(v0(1)-v1(1))*fFactor, st1=(v0(2)-v1(2))*fFactor
    fsubr   D [esi+ 0]
    fxch    st(1)
    fsubr   D [esi+ 4]
    fxch    st(1)
    fstp    D [esi+ 0]
    fst     D [esi+ 4]
    jmp     trueRet

 firstFront:
    cmp     D [fDistance1],0
    jg      trueRet

;secondBack:
    mov     eax,D [fPlaneDistance]
    mov     ebx,D [ulCode1]
    mov     edx,D [ulCodeClip]
    xor     eax,80000000h
    shl     edx,8
    mov     D [edi+8],eax
    mov     D [ebx],edx
    
    fmul    D [fDistance1]
    ;//st0=fFactor
    fld     D [esi+ 0]
    fsub    D [edi+ 0]
    fld     D [esi+ 4]
    fsub    D [edi+ 4]
    ;//st0=v0(2)-v1(2), st1=v0(1)-v1(1), st2=fFactor
    fxch    st(1)
    fmul    st(0),st(2)
    fxch    st(1)
    fmulp   st(2),st(0)
    ;//st0=(v0(1)-v1(1))*fFactor, st1=(v0(2)-v1(2))*fFactor
    fsubr   D [edi+ 0]
    fxch    st(1)
    fsubr   D [edi+ 4]
    fxch    st(1)
    fstp    D [edi+ 0]
    fst     D [edi+ 4]
  }
trueRet:
  _asm fstp st(0)
  return TRUE;
falseRet:
  _asm fstp st(0)
  return FALSE;
#else
  // calculate point distances from clip plane
  FLOAT fDistance0 = fPlaneDistance+v0(3);
  FLOAT fDistance1 = fPlaneDistance+v1(3);
  if (fDistance0<=0) {
    // if both are back
    if (fDistance1<=0) {
      // no line remains
      return FALSE;
    // if first is back, second front
    } else {
      // clip first
      FLOAT fDivisor = 1.0f/(fDistance0-fDistance1);
      FLOAT fFactor = fDistance0*fDivisor;
      v0(1) = v0(1)-(v0(1)-v1(1))*fFactor;
      v0(2) = v0(2)-(v0(2)-v1(2))*fFactor;
      v0(3) = -fPlaneDistance;
      // mark that first was clipped
      ulCode0 = LCFVERTEX0(ulCodeClip);
      // line remains
      return TRUE;
    }
  } else {
    // if first is front, second back
    if (fDistance1<=0) {
      // clip second
      FLOAT fDivisor = 1.0f/(fDistance0-fDistance1);
      FLOAT fFactor = fDistance1*fDivisor;
      v1(1) = v1(1)-(v0(1)-v1(1))*fFactor;
      v1(2) = v1(2)-(v0(2)-v1(2))*fFactor;
      v1(3) = -fPlaneDistance;
      // mark that second was clipped
      ulCode1 = LCFVERTEX1(ulCodeClip);
      // line remains
      return TRUE;
    // if both are front
    } else {
      // line remains unclipped
      return TRUE;
    }
  }
#endif
}

static inline void MakeClipPlane(const FLOAT3D &vN, FLOAT fD, FLOATplane3D &pl)
{
  FLOAT fOoL = 1.0f/vN.Length();
  pl = FLOATplane3D(vN*fOoL, fD*fOoL);
}

#undef ASMOPT

#undef O
#undef Q
#undef D
#undef W
#undef B

#endif /* include-once blocker. */

