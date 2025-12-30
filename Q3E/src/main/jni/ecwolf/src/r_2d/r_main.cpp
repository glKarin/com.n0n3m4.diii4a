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
// $Log:$
//
// DESCRIPTION:
//		Rendering main loop and setup functions,
//		 utility functions (BSP, geometry, trigonometry).
//		See tables.c, too.
//
//-----------------------------------------------------------------------------

// HEADER FILES ------------------------------------------------------------

#include <stdlib.h>
#include <math.h>

#include "templates.h"
#include "wl_def.h"
#include "r_data/colormaps.h"
#include "r_data/r_translate.h"
#include "r_2d/r_draw.h"

// PRIVATE DATA DECLARATIONS -----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

FDynamicColormap*basecolormap;		// [RH] colormap currently drawing with
int				fixedlightlev;
lighttable_t	*fixedcolormap;
fixed_t 		centeryfrac;

void (*colfunc) (void);
void (*basecolfunc) (void);
void (*fuzzcolfunc) (void);
void (*transcolfunc) (void);
void (*spanfunc) (void);

void (*hcolfunc_pre) (void);
void (*hcolfunc_post1) (int hx, int sx, int yl, int yh);
void (*hcolfunc_post2) (int hx, int sx, int yl, int yh);
void (STACK_ARGS *hcolfunc_post4) (int sx, int yl, int yh);

//==========================================================================
//
// R_SetupBuffer
//
// Precalculate all row offsets and fuzz table.
//
//==========================================================================

#define RenderTarget screen
#define viewwindowx 0
#define viewwindowy 0
void R_SetupBuffer ()
{
	//static BYTE *lastbuff = NULL;

	int pitch = RenderTarget->GetPitch();
	BYTE *lineptr = RenderTarget->GetBuffer() + viewwindowy*pitch + viewwindowx;

	//if (dc_pitch != pitch || lineptr != lastbuff)
	{
		if (dc_pitch != pitch)
		{
			dc_pitch = pitch;
			R_InitFuzzTable (pitch);
#if defined(X86_ASM) || defined(X64_ASM)
			ASM_PatchPitch ();
#endif
		}
		dc_destorg = lineptr;
		for (int i = 0; i < RenderTarget->GetHeight(); i++)
		{
			ylookup[i] = i * pitch;
		}
	}
}

//==========================================================================
//
// R_Init
//
//==========================================================================

void R_ShutdownRenderer()
{
	R_DeinitTranslationTables ();
}

void R_InitRenderer()
{
	atterm(R_ShutdownRenderer);
	// viewwidth / viewheight are set by the defaults

	//R_InitPlanes ();
	//R_InitShadeMaps();
	R_InitTranslationTables ();
	R_InitColumnDrawers ();

	colfunc = basecolfunc = R_DrawColumn;
	fuzzcolfunc = R_DrawFuzzColumn;
	transcolfunc = R_DrawTranslatedColumn;
	spanfunc = R_DrawSpan;

	// [RH] Horizontal column drawers
	hcolfunc_pre = R_DrawColumnHoriz;
	hcolfunc_post1 = rt_map1col;
	hcolfunc_post4 = rt_map4cols;
}
