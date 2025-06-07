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

#ifndef SE_INCL_OPENGL_H
#define SE_INCL_OPENGL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include "gl_types.h"

/* rcg10042001 wraped for platform. */
#if (defined _MSC_VER)
#define DLLFUNCTION(dll, output, name, inputs, params, required) \
  extern output (__stdcall *p##name) inputs
#elif (defined PLATFORM_UNIX)
#define DLLFUNCTION(dll, output, name, inputs, params, required) \
  extern output (*p##name) inputs
  #define __stdcall
#else
  #error please define your platform here.
#endif

#include "gl_functions.h"
#ifdef _GLES //karin: GLES function alias
#define pglClearDepth pglClearDepthf
#define pglDepthRange pglDepthRangef
#define pglClipPlane pglClipPlanef                            #define pglGetClipPlane pglGetClipPlanef
#define pglPolygonMode(x, y)
#define pglDrawBuffer(x)
#define pglReadBuffer(x)
#define pglGetTexLevelParameteriv(x, y, z, w)
#define pglGetTexLevelParameterfv(x, y, z, w)
#define pglOrtho pglOrthof
#define pglFrustum pglFrustumf
#define pglColor4ubv(v) pglColor4ub((v)[0], (v)[1], (v)[2], (v)[3])
#endif

#undef DLLFUNCTION

// extensions
extern void (__stdcall *pglLockArraysEXT)(GLint first, GLsizei count);
extern void (__stdcall *pglUnlockArraysEXT)(void);

#ifdef PLATFORM_WIN32  // SDL handles this elsewhere.
extern GLboolean (__stdcall *pwglSwapIntervalEXT)(GLint interval);
extern GLint     (__stdcall *pwglGetSwapIntervalEXT)(void);
#endif

extern void (__stdcall *pglActiveTextureARB)(GLenum texunit);
extern void (__stdcall *pglClientActiveTextureARB)(GLenum texunit);

#ifdef PLATFORM_WIN32 /* !!! FIXME: Move to abstraction layer. --rcg. */
// t-buffer support
extern char *(__stdcall *pwglGetExtensionsStringARB)(HDC hdc);
extern BOOL  (__stdcall *pwglChoosePixelFormatARB)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
extern BOOL  (__stdcall *pwglGetPixelFormatAttribivARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues);
#endif
extern void  (__stdcall *pglTBufferMask3DFX)(GLuint mask);

// GL_NV_vertex_array_range & GL_NV_fence
#ifdef PLATFORM_WIN32 /* !!! FIXME: Move to abstraction layer. --rcg. */
extern void *(__stdcall *pwglAllocateMemoryNV)(GLint size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
extern void  (__stdcall *pwglFreeMemoryNV)(void *pointer);
#endif
extern void  (__stdcall *pglVertexArrayRangeNV)(GLsizei length, void *pointer);
extern void  (__stdcall *pglFlushVertexArrayRangeNV)(void);

extern GLboolean (__stdcall *pglTestFenceNV)(GLuint fence);
extern GLboolean (__stdcall *pglIsFenceNV)(GLuint fence);
extern void  (__stdcall *pglGenFencesNV)(GLsizei n, GLuint *fences);
extern void  (__stdcall *pglDeleteFencesNV)(GLsizei n, const GLuint *fences);
extern void  (__stdcall *pglSetFenceNV)(GLuint fence, GLenum condition);
extern void  (__stdcall *pglFinishFenceNV)(GLuint fence);
extern void  (__stdcall *pglGetFenceivNV)(GLuint fence, GLenum pname, GLint *params);

// ATI GL_ATI[X]_pn_triangles
extern void  (__stdcall *pglPNTrianglesiATI)( GLenum pname, GLint param);
extern void  (__stdcall *pglPNTrianglesfATI)( GLenum pname, GLfloat param);


// additional tools -----------------------------------------------------

#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/Vertex.h>
#include <Engine/Templates/StaticStackArray.cpp>


// set color from croteam format
inline void glCOLOR( COLOR col)
{
/* rcg10052001 Platform-wrappers. */
#if (defined __MSVC_INLINE__)
  __asm {
    mov     eax,dword ptr [col]
    bswap   eax
    mov     dword ptr [col],eax
  }

#elif (defined __GNU_INLINE_X86_32__)
  __asm__ __volatile__ (
    "bswapl   %%eax    \n\t"
        : "=a" (col)
        : "a" (col)
  );

#else
  col = ( ((col << 24)            ) |
          ((col << 8) & 0x00FF0000) |
          ((col >> 8) & 0x0000FF00) |
          ((col >> 24)            ) );

#endif

  pglColor4ubv((GLubyte*)&col);
}

// check windows errors always
extern void WIN_CheckError(BOOL bRes, const char *strDescription);
#define WIN_CHECKERROR(result, string)   WIN_CheckError(result, string);

extern BOOL glbUsingVARs;   // vertex_array_range

// common textures
extern GLuint _uiFillTextureNo;    // binding for flat fill emulator texture
extern GLuint _uiFogTextureNo;     // binding for fog texture
extern GLuint _uiHazeTextureNo;    // binding for haze texture
extern GLuint _uiPatternTextureNo; // binding for pattern texture


// internal!
inline void pglActiveTexture(INDEX texunit)
{
  ASSERT( texunit>=0 && texunit<4);
  ASSERT( pglActiveTextureARB!=NULL);
  ASSERT( pglClientActiveTextureARB!=NULL);
  pglActiveTextureARB(      GLenum(GL_TEXTURE0_ARB+texunit));
  pglClientActiveTextureARB(GLenum(GL_TEXTURE0_ARB+texunit));
}

#endif  /* include-once check. */

