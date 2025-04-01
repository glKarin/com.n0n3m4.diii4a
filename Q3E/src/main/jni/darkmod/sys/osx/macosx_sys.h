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
#import "../posix/posix_public.h"

#import <Foundation/NSGeometry.h>
@class NSEvent, NSOpenGLContext, NSWindow;

#import <ApplicationServices/ApplicationServices.h>
#import <OpenGL/CGLTypes.h>

// sys

const char *macosx_scanForLibraryDirectory(void);

// In macosx_input.m
void Sys_InitInput(void);
void Sys_ShutdownInput(void);
CGDirectDisplayID Sys_DisplayToUse(void);
//extern void osxQuit();
void SetProgramPath(char *path);
void Sys_SetMouseInputRect(CGRect newRect);

void Sys_AnnoyingBanner();

// In macosx_glimp.m
bool Sys_Hide();
bool Sys_Unhide();

typedef struct {
    CGDirectDisplayID     display;
    CGTableCount          tableSize;
    CGGammaValue	 *red;
    CGGammaValue	 *blue;
    CGGammaValue	 *green;
} glwgamma_t;

typedef struct
{
    CGDirectDisplayID	display;
    NSDictionary		*desktopMode;
    NSDictionary		*gameMode;

    CGDisplayCount		displayCount;
    glwgamma_t			*originalDisplayGammaTables;
    glwgamma_t			inGameTable;
    glwgamma_t			tempTable;
    
    NSOpenGLContext		*_ctx;
    CGLContextObj		_cgl_ctx;
    bool				_ctx_is_current;
    NSWindow			*window;
    
    FILE				*log_fp;
    
    unsigned int		bufferSwapCount;
    unsigned int		glPauseCount;
} glwstate_t;

extern glwstate_t glw_state;

#define OSX_SetGLContext(context) \
do { \
    NSOpenGLContext *_context = (context); \
    glw_state._ctx = _context; \
    glw_state._cgl_ctx = [_context cglContext]; \
} while (0)

#define OSX_GetNSGLContext() glw_state._ctx
#define OSX_GetCGLContext() glw_state._cgl_ctx

#define OSX_GLContextIsCurrent() glw_state._ctx_is_current
#define OSX_GLContextSetCurrent() \
do { \
  [glw_state._ctx makeCurrentContext]; \
  glw_state._ctx_is_current = (glw_state._ctx != nil); \
} while (0)

#define OSX_GLContextClearCurrent() \
do { \
  [NSOpenGLContext clearCurrentContext]; \
  glw_state._ctx_is_current = NO; \
} while (0)


void Sys_PauseGL();
void Sys_ResumeGL();
