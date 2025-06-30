/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */
#define __STDC_LIMIT_MACROS 1

#include <stdio.h>

#include <Engine/Base/Types.h>
#include <Engine/Base/Assert.h>

// !!! FIXME: can we move this one function somewhere else?

extern int screen_width;
ULONG DetermineDesktopWidth(void)
{
	return screen_width;
} // DetermineDesktopWidth

// end of SDLAdapter.cpp ...


