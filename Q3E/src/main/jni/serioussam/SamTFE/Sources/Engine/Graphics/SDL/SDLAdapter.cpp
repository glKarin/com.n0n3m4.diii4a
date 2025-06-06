/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */
#ifdef _DIII4A
#include "../EGL/EGLAdapter.cpp"
#else
#define __STDC_LIMIT_MACROS 1

#include <stdio.h>

#include "SDL.h"
#include <Engine/Base/Types.h>
#include <Engine/Base/Assert.h>

// !!! FIXME: can we move this one function somewhere else?

ULONG DetermineDesktopWidth(void)
{
    const int dpy = 0;   // !!! FIXME: add a cvar for this?
    SDL_DisplayMode mode;
    const int rc = SDL_GetDesktopDisplayMode(dpy, &mode);
    ASSERT(rc == 0);
    return(mode.w);
} // DetermineDesktopWidth

// end of SDLAdapter.cpp ...

#endif
