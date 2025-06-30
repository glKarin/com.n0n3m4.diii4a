/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#include "Engine/StdH.h"
#include <Engine/Engine.h>
#include <Engine/Graphics/Gfx_OpenGL.h>

static void FailFunction_t(const char *strName) {
  ThrowF_t(TRANS("Required function %s not found."), strName);
}

void WIN_CheckError(BOOL bRes, const char *strDescription)
{
  if( bRes) return;
  DWORD dwWindowsErrorCode = GetLastError();
  if( dwWindowsErrorCode==ERROR_SUCCESS) return; // ignore stupid 'successful' error 
  WarningMessage("%s: %s", strDescription, GetWindowsError(dwWindowsErrorCode));
}


static void OGL_SetFunctionPointers_t(HINSTANCE hiOGL)
{
  const char *strName;
  // get gl function pointers

  #define DLLFUNCTION(dll, output, name, inputs, params, required) \
    strName = #name;  \
    p##name = (output (__stdcall*) inputs) GetProcAddress( hi##dll, strName); \
    if( required && p##name == NULL) FailFunction_t(strName);
  #include "Engine/Graphics/gl_functions.h"
  #undef DLLFUNCTION
}


// initialize OpenGL driver
BOOL CGfxLibrary::InitDriver_OGL( BOOL b3Dfx/*=FALSE*/)
{
  ASSERT( gl_hiDriver==NONE);
  UINT iOldErrorMode = SetErrorMode( SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
  CTString strDriverFileName = b3Dfx ? "3DFXVGL.DLL" : "OPENGL32.DLL";

  try
  { // if driver doesn't exists on disk
    char strBuffer[_MAX_PATH+1];
    char *strDummy;
    int iRes = SearchPathA( NULL, strDriverFileName, NULL, _MAX_PATH, strBuffer, &strDummy);
    if( iRes==0) ThrowF_t(TRANS("OpenGL driver '%s' not present"), strDriverFileName);

    // load opengl library
    gl_hiDriver = (CDynamicLoader *)::LoadLibraryA( strDriverFileName);

    // if it cannot be loaded (although it is present on disk)
    if( gl_hiDriver==NONE) {
      // if it is 3dfx stand-alone driver
      if( b3Dfx) {
        // do a fatal error and inform user to deinstall it,
        // since this loading attempt probably messed up the entire system
        FatalError(TRANS( "3Dfx OpenGL driver '%s' is installed, but cannot be loaded!\n"
                          "If you previously had a 3Dfx card and it was removed,\n"
                          "please deinstall the driver and restart windows before\n"
                          "continuing.\n"), strDriverFileName);
      } // fail!
      ThrowF_t(TRANS("Cannot load OpenGL driver '%s'"), (const char *) strDriverFileName);
    }
    // prepare functions
    OGL_SetFunctionPointers_t((HINSTANCE)gl_hiDriver);
  }
  catch (const char *strError)
  { // didn't make it :(
    if( gl_hiDriver!=NONE) FreeLibrary((HINSTANCE)gl_hiDriver);
    gl_hiDriver = NONE;
    CPrintF( TRANS("Error starting OpenGL: %s\n"), strError);
    SetErrorMode(iOldErrorMode);
    return FALSE;
  }

  // revert to old error mode
  SetErrorMode(iOldErrorMode);

  // if default driver
  if( !b3Dfx) {
    // use GDI functions
    pwglSwapBuffers       = ::SwapBuffers;
    pwglSetPixelFormat    = ::SetPixelFormat;
    pwglChoosePixelFormat = ::ChoosePixelFormat;
    // NOTE:
    // some ICD implementations are not infact in OPENGL32.DLL, but in some
    // other installed DLL, which is loaded when original OPENGL32.DLL from MS is
    // loaded. For those, we in fact load OPENGL32.DLL from MS, so we must _not_
    // call these functions directly, because they are in MS dll. We must call
    // functions from GDI, which in turn call either OPENGL32.DLL, _or_ the client driver,
    // as appropriate.
  }
  // done
  return TRUE;
} 


void CGfxLibrary::PlatformEndDriver_OGL(void)
{
  // shut the driver down
  if( go_hglRC!=NULL) {
    if( pwglMakeCurrent!=NULL) {
      BOOL bRes = pwglMakeCurrent(NULL, NULL);
      WIN_CHECKERROR( bRes, "MakeCurrent(NULL, NULL)");
    }
    ASSERT( pwglDeleteContext!=NULL);
    BOOL bRes = pwglDeleteContext(go_hglRC);
    WIN_CHECKERROR( bRes, "DeleteContext");
    go_hglRC = NULL;
  }
}

// creates OpenGL drawing context
BOOL CGfxLibrary::CreateContext_OGL(HDC hdc)
{
  if( !SetupPixelFormat_OGL( hdc, TRUE)) return FALSE;
  go_hglRC = pwglCreateContext(hdc);
  if( go_hglRC==NULL) {
    WIN_CHECKERROR(0, "CreateContext");
    return FALSE;
  }
  if( !pwglMakeCurrent(hdc, go_hglRC)) {
    // NOTE: This error is sometimes reported without a reason on 3dfx hardware
    // so we just have to ignore it.
    //WIN_CHECKERROR(0, "MakeCurrent after CreateContext");
    return FALSE;
  }
  return TRUE;
}


#define BACKOFF pwglMakeCurrent( NULL, NULL); \
	              pwglDeleteContext( hglrc); \
	              ReleaseDC( dummyhwnd, hdc); \
	              DestroyWindow( dummyhwnd); \
            	  UnregisterClassA( classname, hInstance);



// helper for choosing t-buffer's pixel format
extern BOOL _TBCapability;
static INDEX ChoosePixelFormatTB( HDC hdc, const PIXELFORMATDESCRIPTOR *ppfd,
                                  PIX pixResWidth, PIX pixResHeight)
{
  _TBCapability = FALSE;
	char *extensions = NULL;
	char *wglextensions = NULL;
	HGLRC hglrc; 
	HWND dummyhwnd;
	WNDCLASSA cls;
  HINSTANCE hInstance = GetModuleHandle(NULL);
	LPCSTR classname = "dummyOGLwin";
	cls.style = CS_OWNDC;
	cls.lpfnWndProc = DefWindowProc;
	cls.cbClsExtra = 0;
	cls.cbWndExtra = 0;
	cls.hInstance = hInstance;
	cls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	cls.hCursor = LoadCursor(NULL, IDC_WAIT);
	cls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	cls.lpszMenuName = NULL;
	cls.lpszClassName = classname;
  // didn't manage to register class?
	if( !RegisterClassA(&cls))	return 0;

	// create window fullscreen 
  //CPrintF( "  Dummy window: %d x %d\n", pixResWidth, pixResHeight);
	dummyhwnd = CreateWindowExA( WS_EX_TOPMOST, classname, "Dummy OGL window",
                              WS_POPUP|WS_VISIBLE, 0, 0, pixResWidth, pixResHeight,
                              NULL, NULL, hInstance, NULL);
  // didn't make it?
  if( dummyhwnd == NULL) {
	  UnregisterClassA( classname, hInstance);
    return 0;
  }
  //CPrintF( "  Dummy passed...\n");
	hdc = GetDC(dummyhwnd);
  // try to choose pixel format
	int iPixelFormat = pwglChoosePixelFormat( hdc, ppfd);
	if( !iPixelFormat) {
    ReleaseDC( dummyhwnd, hdc);
    DestroyWindow(dummyhwnd);
	  UnregisterClassA( classname, hInstance);
	  return 0;
	}
  //CPrintF( "  Choose pixel format passed...\n");
  // try to set pixel format
	if( !pwglSetPixelFormat( hdc, iPixelFormat, ppfd)) {
    ReleaseDC( dummyhwnd, hdc);
    DestroyWindow(dummyhwnd);
	  UnregisterClassA( classname, hInstance);
	  return 0;
	}
  //CPrintF( "  Set pixel format passed...\n");

	// create context using the default accelerated pixelformat that was passed
	hglrc = pwglCreateContext(hdc);
	pwglMakeCurrent( hdc, hglrc);
	// update the value list with information passed from the ppfd.
	aiAttribList[ 9] =  ppfd->cColorBits;
	aiAttribList[11] =  ppfd->cDepthBits;
	aiAttribList[15] = _pGfx->go_ctSampleBuffers;

	// get the extension list.
	extensions = (char*)pglGetString(GL_EXTENSIONS);
	// get the wgl extension list.
	if( strstr((const char*)extensions, "WGL_EXT_extensions_string ") != NULL)
  { // windows extension string supported
    pwglGetExtensionsStringARB = (char* (__stdcall*)(HDC))pwglGetProcAddress( "wglGetExtensionsStringARB");
    if( pwglGetExtensionsStringARB == NULL) {
      BACKOFF
      return 0;
    }
    //CPrintF( "  WGL extension string passed...\n");
		// get WGL extension string
		wglextensions = (char*)pwglGetExtensionsStringARB(hdc);
 	}
  else {
    BACKOFF
    return 0;
	}

 	// check for the pixel format and multisample extension strings
 	if( (strstr((const char*)wglextensions, "WGL_ARB_pixel_format ") != NULL) && 
      (strstr((const char*)extensions,    "GL_3DFX_multisample ")  != NULL)) {
    // 3dfx extensions present
    _TBCapability = TRUE;
    pwglChoosePixelFormatARB      = (BOOL (__stdcall*)(HDC,const int*,const FLOAT*,UINT,int*,UINT*))pwglGetProcAddress( "wglChoosePixelFormatARB");
    pwglGetPixelFormatAttribivARB = (BOOL (__stdcall*)(HDC,int,int,UINT,int*,int*)                 )pwglGetProcAddress( "wglGetPixelFormatAttribivARB");
		pglTBufferMask3DFX = (void (__stdcall*)(GLuint))pwglGetProcAddress("glTBufferMask3DFX");
    if( pwglChoosePixelFormatARB==NULL && pglTBufferMask3DFX==NULL) {
      BACKOFF
      return 0;
		}
    //CPrintF( "  WGL choose pixel format present...\n");
    int iAttribListNum = {WGL_NUMBER_PIXEL_FORMATS_EXT};
  	int iMaxFormats    = 1; // default number to return
		if( pwglGetPixelFormatAttribivARB!=NULL) {
			// get total number of formats supported.
			pwglGetPixelFormatAttribivARB( hdc, NULL, NULL, 1, &iAttribListNum, &iMaxFormats);
      //CPrintF( "Max formats: %d\n", iMaxFormats);
		}
    UINT uiNumFormats;
	  int *piFormats = (int*)AllocMemory( sizeof(UINT) *iMaxFormats);
    // try to get all formats that fit the pixel format criteria
    if( !pwglChoosePixelFormatARB( hdc, piAttribList, NULL, iMaxFormats, piFormats, &uiNumFormats)) {
      FreeMemory(piFormats);
      BACKOFF
      return 0;
    }
    //CPrintF( "  WGL choose pixel format passed...\n");
		// return the first match for now
    iPixelFormat = 0;
    if( uiNumFormats>0) {
      iPixelFormat = piFormats[0];
      //CPrintF( "Num formats: %d\n", uiNumFormats);
      //CPrintF( "First format: %d\n", iPixelFormat);
    }
    FreeMemory(piFormats);
  }
	else
  {	// wglChoosePixelFormatARB extension does not exist :(
		iPixelFormat = 0;
	}
  BACKOFF
  return iPixelFormat;
}

void *CGfxLibrary::OGL_GetProcAddress(const char *procname)
{
    return(pwglGetProcAddress(procname));
}

// prepares pixel format for OpenGL context
BOOL CGfxLibrary::SetupPixelFormat_OGL( HDC hdc, BOOL bReport/*=FALSE*/)
{
  int iPixelFormat = 0;
  const PIX pixResWidth  = gl_dmCurrentDisplayMode.dm_pixSizeI;
  const PIX pixResHeight = gl_dmCurrentDisplayMode.dm_pixSizeJ;
  const DisplayDepth dd  = gl_dmCurrentDisplayMode.dm_ddDepth;

  PIXELFORMATDESCRIPTOR pfd;
  memset( &pfd, 0, sizeof(pfd));
  pfd.nSize      = sizeof(pfd);
  pfd.nVersion   = 1;
  pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;

  // clamp depth/stencil values
  extern INDEX gap_iDepthBits;
  extern INDEX gap_iStencilBits;
       if( gap_iDepthBits <12) gap_iDepthBits   = 0;
  else if( gap_iDepthBits <22) gap_iDepthBits   = 16;
  else if( gap_iDepthBits <28) gap_iDepthBits   = 24;
  else                         gap_iDepthBits   = 32;
       if( gap_iStencilBits<3) gap_iStencilBits = 0;
  else if( gap_iStencilBits<7) gap_iStencilBits = 4;
  else                         gap_iStencilBits = 8;

  // set color/depth buffer values
  pfd.cColorBits   = (dd!=DD_16BIT) ? 32 : 16;
  pfd.cDepthBits   = gap_iDepthBits;
  pfd.cStencilBits = gap_iStencilBits;

  // must be required and works only in full screen via GDI functions
  ogl_iTBufferEffect = Clamp( ogl_iTBufferEffect, (INDEX)0, (INDEX)2);
  if( ogl_iTBufferEffect>0 && pixResWidth>0 && pixResHeight>0)
  { // lets T-buffer ... :)
    //CPrintF( "TBuffer init...\n");
    ogl_iTBufferSamples = (1L) << FastLog2(ogl_iTBufferSamples);
    if( ogl_iTBufferSamples<2) ogl_iTBufferSamples = 4;
    go_ctSampleBuffers = ogl_iTBufferSamples;
    go_iCurrentWriteBuffer = 0;
    iPixelFormat = ChoosePixelFormatTB( hdc, &pfd, pixResWidth, pixResHeight);
	  // need to reset the desktop resolution because CPFTB() resets it
    BOOL bSuccess = CDS_SetMode( pixResWidth, pixResHeight, dd);
    if( !bSuccess) iPixelFormat = 0;
    // check T-buffer support
    if( _TBCapability) pglGetIntegerv( GL_SAMPLES_3DFX, (GLint*)&go_ctSampleBuffers);
    if( !iPixelFormat) { ogl_iTBufferEffect=0; CPrintF( TRANS("TBuffer initialization failed.\n")); }
    else CPrintF( TRANS("TBuffer initialization passed (%d buffers in use).\n"), go_ctSampleBuffers);
  }

  // if T-buffer didn't make it, let's try thru regular path
  if( !iPixelFormat) {
    go_ctSampleBuffers = 0;
    go_iCurrentWriteBuffer = 0;
    iPixelFormat = pwglChoosePixelFormat( hdc, &pfd);
  }

  if( !iPixelFormat) {
    WIN_CHECKERROR( 0, "ChoosePixelFormat");
    return FALSE;
  }
  if( !pwglSetPixelFormat( hdc, iPixelFormat, &pfd)) {
    WIN_CHECKERROR( 0, "SetPixelFormat");
    return FALSE;
  }

  // test acceleration
  memset( &pfd, 0, sizeof(pfd));
  if( !pwglDescribePixelFormat( hdc, iPixelFormat, sizeof(pfd), &pfd)) return FALSE;
  BOOL bGenericFormat      = pfd.dwFlags & PFD_GENERIC_FORMAT;
  BOOL bGenericAccelerated = pfd.dwFlags & PFD_GENERIC_ACCELERATED;
  BOOL bHasAcceleration    = (bGenericFormat &&  bGenericAccelerated) ||  // MCD
                            (!bGenericFormat && !bGenericAccelerated);    // ICD
  if( bHasAcceleration) gl_ulFlags |=  GLF_HASACCELERATION;
  else                  gl_ulFlags &= ~GLF_HASACCELERATION;

  // done if report pixel format info isn't required
  if( !bReport) return TRUE;

  // prepare pixel type description
  CTString strPixelType;
  if( pfd.iPixelType==PFD_TYPE_RGBA) strPixelType = "TYPE_RGBA"; 
  else if( pfd.iPixelType&PFD_TYPE_COLORINDEX) strPixelType = "TYPE_COLORINDEX";
  else strPixelType = "unknown";
  // prepare flags description
  CTString strFlags = "";
  if( pfd.dwFlags&PFD_DRAW_TO_WINDOW)        strFlags += "DRAW_TO_WINDOW "; 
  if( pfd.dwFlags&PFD_DRAW_TO_BITMAP)        strFlags += "DRAW_TO_BITMAP "; 
  if( pfd.dwFlags&PFD_SUPPORT_GDI)           strFlags += "SUPPORT_GDI "; 
  if( pfd.dwFlags&PFD_SUPPORT_OPENGL)        strFlags += "SUPPORT_OPENGL "; 
  if( pfd.dwFlags&PFD_GENERIC_ACCELERATED)   strFlags += "GENERIC_ACCELERATED "; 
  if( pfd.dwFlags&PFD_GENERIC_FORMAT)        strFlags += "GENERIC_FORMAT "; 
  if( pfd.dwFlags&PFD_NEED_PALETTE)          strFlags += "NEED_PALETTE "; 
  if( pfd.dwFlags&PFD_NEED_SYSTEM_PALETTE)   strFlags += "NEED_SYSTEM_PALETTE "; 
  if( pfd.dwFlags&PFD_DOUBLEBUFFER)          strFlags += "DOUBLEBUFFER "; 
  if( pfd.dwFlags&PFD_STEREO)                strFlags += "STEREO "; 
  if( pfd.dwFlags&PFD_SWAP_LAYER_BUFFERS)    strFlags += "SWAP_LAYER_BUFFERS "; 
  if( pfd.dwFlags&PFD_DEPTH_DONTCARE)        strFlags += "DEPTH_DONTCARE "; 
  if( pfd.dwFlags&PFD_DOUBLEBUFFER_DONTCARE) strFlags += "DOUBLEBUFFER_DONTCARE "; 
  if( pfd.dwFlags&PFD_STEREO_DONTCARE)       strFlags += "STEREO_DONTCARE "; 
  if( pfd.dwFlags&PFD_SWAP_COPY)             strFlags += "SWAP_COPY "; 
  if( pfd.dwFlags&PFD_SWAP_EXCHANGE)         strFlags += "SWAP_EXCHANGE "; 
  if( strFlags=="") strFlags = "none";
                              
  // output pixel format description to console (for debugging purposes)
  CPrintF( TRANS("\nPixel Format Description:\n"));
  CPrintF( TRANS("  Number:     %d (%s)\n"), iPixelFormat, strPixelType);
  CPrintF( TRANS("  Flags:      %s\n"), strFlags);
  CPrintF( TRANS("  Color bits: %d (%d:%d:%d:%d)\n"), pfd.cColorBits, 
           pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits, pfd.cAlphaBits);
  CPrintF( TRANS("  Depth bits: %d (%d for stencil)\n"), pfd.cDepthBits, pfd.cStencilBits);
  gl_iCurrentDepth = pfd.cDepthBits; // keep depth bits
  
  // all done
  CPrintF( "\n");
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
    CTempDC tdc(pvp->vp_hWnd);
    if( !CreateContext_OGL(tdc.hdc)) return FALSE;
    gl_pvpActive = pvp; // remember as current viewport (must do that BEFORE InitContext)
    InitContext_OGL();
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
    CTempDC tdc(pvp->vp_hWnd);
    if( !SetupPixelFormat_OGL(tdc.hdc)) return FALSE;
    pvp->vp_ctDisplayChanges = gl_ctDriverChanges;
  }

  if( gl_pvpActive!=NULL) {
    // fail, if only one window is allowed (3dfx driver), already initialized and trying to set non-primary viewport
    const BOOL bOneWindow = (gl_gaAPI[GAT_OGL].ga_adaAdapter[gl_iCurrentAdapter].da_ulFlags & DAF_ONEWINDOW);
    if( bOneWindow && gl_pvpActive->vp_hWnd!=NULL && gl_pvpActive->vp_hWnd!=pvp->vp_hWnd) return FALSE;
    // no need to set context if it is the same window as last time
    if( gl_pvpActive->vp_hWnd==pvp->vp_hWnd) return TRUE;
  }

  // try to set context to this window
  pwglMakeCurrent( NULL, NULL);
  CTempDC tdc(pvp->vp_hWnd);
  // fail, if cannot set context to this window
  if( !pwglMakeCurrent( tdc.hdc, go_hglRC)) return FALSE;

  // remember as current window
  gl_pvpActive = pvp;
  return TRUE;
}

