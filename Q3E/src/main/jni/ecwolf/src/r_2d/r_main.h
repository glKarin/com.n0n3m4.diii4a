// Emacs style mode select	 -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//		System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __R_MAIN_H__
#define __R_MAIN_H__

#include "v_palette.h"
#include "v_video.h"
#include "r_data/colormaps.h"


typedef BYTE lighttable_t;	// This could be wider for >8 bit display.

//
// POV related.
//

extern fixed_t			centeryfrac;

extern FDynamicColormap*basecolormap;	// [RH] Colormap for sector currently being drawn

extern int				fixedlightlev;
extern lighttable_t*	fixedcolormap;


//
// Function pointers to switch refresh/drawing functions.
// Used to select shadow mode etc.
//
extern void 			(*colfunc) (void);
extern void 			(*basecolfunc) (void);
extern void 			(*fuzzcolfunc) (void);
extern void				(*transcolfunc) (void);
// No shadow effects on floors.
extern void 			(*spanfunc) (void);

// [RH] Function pointers for the horizontal column drawers.
extern void (*hcolfunc_pre) (void);
extern void (*hcolfunc_post1) (int hx, int sx, int yl, int yh);
extern void (*hcolfunc_post2) (int hx, int sx, int yl, int yh);
extern void (STACK_ARGS *hcolfunc_post4) (int sx, int yl, int yh);


void R_SetupBuffer ();
void R_InitRenderer();


#endif // __R_MAIN_H__
