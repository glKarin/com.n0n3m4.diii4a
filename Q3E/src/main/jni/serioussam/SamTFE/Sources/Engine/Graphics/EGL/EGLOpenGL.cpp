/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#include <Engine/Engine.h>

#define Q3E_PRINTF CPrintF
#define Q3E_ERRORF FatalError
#define Q3E_DEBUGF CPrintF
#define Q3Ebool BOOL
#define Q3E_TRUE TRUE
#define Q3E_FALSE FALSE

#include "q3e/q3e_glimp.inc"

void GLimp_AndroidOpenWindow(volatile ANativeWindow *w)
{
	Q3E_RequireWindow(w);
}

void GLimp_AndroidInit(volatile ANativeWindow *w)
{
	if(Q3E_NoDisplay())
		return;

	if(Q3E_RequireWindow(w))
	{
		Q3E_RestoreGL();
	}
}

void GLimp_AndroidQuit(void)
{
	Q3E_DestroyGL(TRUE);
}

BOOL GLimp_InitGL(void)
{
	Q3E_GL_CONFIG_SET(fullscreen, 1);
	Q3E_GL_CONFIG_SET(swap_interval, 1);
	Q3E_GL_CONFIG_ES_1_1();

	BOOL res = Q3E_InitGL();
	return res;
}

extern void Q3E_GetMouseState(int *, int *);
extern void Q3E_GetWindowSize(void *, int *, int *);
extern BOOL Q3E_GetRelativeMouseMode(void);
void R_DrawCursor(void)
{
	// backup OpenGL state
	GLboolean ve = pglIsEnabled(GL_VERTEX_ARRAY);
	GLboolean te = pglIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean ne = pglIsEnabled(GL_NORMAL_ARRAY);
	GLboolean ce = pglIsEnabled(GL_COLOR_ARRAY);
	if(!ve) pglEnableClientState(GL_VERTEX_ARRAY);
	if(te) pglDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglDisableClientState(GL_NORMAL_ARRAY);
	if(ce) pglDisableClientState(GL_COLOR_ARRAY);
	GLboolean dt = pglIsEnabled(GL_DEPTH_TEST);
	GLboolean bl = pglIsEnabled(GL_BLEND);
	GLboolean tt = pglIsEnabled(GL_TEXTURE_2D);
	if(dt) pglDisable(GL_DEPTH_TEST);
	if(!bl) pglEnable(GL_BLEND);
	if(tt) pglDisable(GL_TEXTURE_2D);
	GLboolean dm;
	pglGetBooleanv(GL_DEPTH_WRITEMASK, &dm);
	if(dm) pglDepthMask(GL_FALSE);
	GLint blsrc, bldst;
	GLint mm;
	pglGetIntegerv(GL_MATRIX_MODE, &mm);
	/*
	GLboolean cm[4];
	pglGetBooleanv(GL_COLOR_WRITEMASK, cm);
	pglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	*/
	pglGetIntegerv(GL_BLEND_SRC, &blsrc);
	pglGetIntegerv(GL_BLEND_DST, &bldst);
	pglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// matrix
	int x, y;
	Q3E_GetMouseState(&x, &y);
	int w, h;
	Q3E_GetWindowSize(nullptr, &w, &h);
	pglMatrixMode(GL_PROJECTION);
	pglPushMatrix();
	pglLoadIdentity();
	pglOrtho(0, w, h, 0, -1, 1);
	pglMatrixMode(GL_MODELVIEW);
	pglPushMatrix();
	pglLoadIdentity();
	pglTranslatef(x, y, 0);
	pglRotatef(150.0f, 0, 0, 1);
	
	// draw
	pglColor4f(1, 1, 1, 0.9f);
	GLfloat r = 50;
	const GLfloat vs[] = {
		0, 0,
		-r * 0.5f, - r * 1.0f,
		0, - r * 0.75f,
		r * 0.5f, - r * 1.0f,
	};
	pglVertexPointer(2, GL_FLOAT, 0, vs);
	pglDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	// reset OpenGL state
	if(!ve) pglDisableClientState(GL_VERTEX_ARRAY);
	if(te) pglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if(ne) pglEnableClientState(GL_NORMAL_ARRAY);
	if(ce) pglEnableClientState(GL_COLOR_ARRAY);
	if(dt) pglEnable(GL_DEPTH_TEST);
	if(!bl) pglDisable(GL_BLEND);
	if(tt) pglEnable(GL_TEXTURE_2D);
	if(dm) pglDepthMask(GL_TRUE);
	pglPopMatrix();
	pglMatrixMode(GL_PROJECTION);
	pglPopMatrix();
	pglMatrixMode((GLenum)mm);
	pglBlendFunc((GLenum)blsrc, (GLenum)bldst);
	//pglColorMask(cm[0], cm[1], cm[2], cm[3]);
}

void EGL_SwapBuffers(void) {
	if(!Q3E_GetRelativeMouseMode())
		R_DrawCursor();
    Q3E_SwapBuffers();
}

int EGL_GetSwapInterval(void) {
	return Q3E_GL_CONFIG(swap_interval);
}



static void FailFunction_t(const char *strName) {
  ThrowF_t(TRANS("Required function %s not found."), strName);
}

static void OGL_SetFunctionPointers_t(HINSTANCE hiOGL)
{
  const char *strName;
  // get gl function pointers

  #define DLLFUNCTION(dll, output, name, inputs, params, required) \
    strName = #name;  \
    p##name = (output (__stdcall*) inputs) (void*)eglGetProcAddress(strName); \
    if( required && p##name == NULL) FailFunction_t(strName);
  #include "Engine/Graphics/gl_functions.h"
  #undef DLLFUNCTION
}

BOOL CGfxLibrary::InitDriver_OGL(BOOL init3dfx)
{
  return TRUE;
}


void CGfxLibrary::PlatformEndDriver_OGL(void)
{
  // shut the driver down
	Q3E_DestroyGL(FALSE);
  go_hglRC = NULL;
}

// creates OpenGL drawing context
BOOL CGfxLibrary::CreateContext_OGL(HDC hdc)
{
  if( !SetupPixelFormat_OGL( hdc, TRUE)) return FALSE;
  GLimp_InitGL();
  gl_iCurrentDepth = Q3E_GL_CONFIG(depth);  // oh well.

  // prepare functions
  OGL_SetFunctionPointers_t(gl_hiDriver);

  return TRUE;
}

void *CGfxLibrary::OGL_GetProcAddress(const char *procname)
{
    return((void *)eglGetProcAddress(procname));
}

// prepares pixel format for OpenGL context
BOOL CGfxLibrary::SetupPixelFormat_OGL( HDC hdc, BOOL bReport/*=FALSE*/)
{
  const DisplayDepth dd  = gl_dmCurrentDisplayMode.dm_ddDepth;

  // clamp depth/stencil values
  extern INDEX gap_iDepthBits;
  extern INDEX gap_iStencilBits;
       if( gap_iDepthBits <22) gap_iDepthBits   = 16; // this includes 0; 16 is a safe default
  else if( gap_iDepthBits <28) gap_iDepthBits   = 24;
  else                         gap_iDepthBits   = 32;
       if( gap_iStencilBits<3) gap_iStencilBits = 0;
  else if( gap_iStencilBits<7) gap_iStencilBits = 4;
  else                         gap_iStencilBits = 8;

  STUBBED("co-opt the existing T-buffer support for multisampling?");

  return TRUE;
}

// prepare current viewport for rendering thru OpenGL
BOOL CGfxLibrary::SetCurrentViewport_OGL(CViewPort *pvp)
{
  // if must init entire opengl
  if( gl_ulFlags & GLF_INITONNEXTWINDOW)
  {
    gl_ulFlags &= ~GLF_INITONNEXTWINDOW;
    // reopen window
    pvp->CloseCanvas();
    pvp->OpenCanvas();
    // init now
    if( !CreateContext_OGL((HDC) pvp->vp_hWnd)) return FALSE;
    gl_pvpActive = pvp; // remember as current viewport (must do that BEFORE InitContext)
    InitContext_OGL();
    gl_ulFlags |=  GLF_HASACCELERATION;  // !!! FIXME: might be a lie, though...
    pvp->vp_ctDisplayChanges = gl_ctDriverChanges;
    return TRUE;
  }

  // if window was not set for this driver
  if( pvp->vp_ctDisplayChanges<gl_ctDriverChanges)
  {
    // reopen window
    pvp->CloseCanvas();
    pvp->OpenCanvas();

    // set it
    pvp->vp_ctDisplayChanges = gl_ctDriverChanges;
  }

  // remember as current window
  gl_pvpActive = pvp;
  return TRUE;
}

// end of SDLOpenGL.cpp ...


