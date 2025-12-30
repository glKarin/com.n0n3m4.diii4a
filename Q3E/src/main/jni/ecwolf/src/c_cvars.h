/*
** c_cvars.h
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

// There's no console yet, but I'm putting user configurable stuff in here in
// case I get around to it in the future.

#ifndef __C_CVARS__
#define __C_CVARS__

#include "wl_def.h"

void FinalReadConfig();
void ReadConfig();
void WriteConfig();

extern enum Aspect
{
	ASPECT_NONE,
	ASPECT_16_9,
	ASPECT_16_10,
	ASPECT_17_10,
	ASPECT_4_3,
	ASPECT_5_4,
	ASPECT_64_27 // marketed as 21:9
} r_ratio;

extern bool		forcegrabmouse;
extern bool		r_depthfog;
extern bool		vid_fullscreen;
extern Aspect	vid_aspect;
extern bool		vid_vsync;
extern bool		quitonescape;
extern fixed	movebob;

extern float	localDesiredFOV;
//
// control info
//
extern  bool		alwaysrun;
extern  bool		mouseenabled, mouseyaxisdisabled, joystickenabled;

#endif /* __C_CVARS__ */
