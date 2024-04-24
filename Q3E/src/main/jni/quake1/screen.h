/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// screen.h

#ifndef SCREEN_H
#define SCREEN_H

#include <stddef.h>
#include "qtypes.h"
struct portable_samplepair_s;

void CL_Screen_Init (void);
void CL_UpdateScreen (void);
void SCR_CenterPrint(const char *str);

void SCR_BeginLoadingPlaque (qbool startup);
void SCR_DeferLoadingPlaque (qbool startup);
void SCR_EndLoadingPlaque (void);

// pushes an item on the loading screen
void SCR_PushLoadingScreen (const char *msg, float len_in_parent);
void SCR_PopLoadingScreen (qbool redraw);
void SCR_ClearLoadingScreen (qbool redraw);

void SCR_CaptureVideo_SoundFrame(const struct portable_samplepair_s *paintbuffer, size_t length);

extern unsigned int scr_con_current; // current height of displayed console

extern int sb_lines;

extern struct cvar_s scr_viewsize;
extern struct cvar_s scr_fov;
extern struct cvar_s cl_showfps;
extern struct cvar_s cl_showtime;
extern struct cvar_s cl_showdate;

extern struct cvar_s crosshair;
extern struct cvar_s crosshair_size;

extern struct cvar_s scr_conalpha;
extern struct cvar_s scr_conalphafactor;
extern struct cvar_s scr_conalpha2factor;
extern struct cvar_s scr_conalpha3factor;
extern struct cvar_s scr_conscroll_x;
extern struct cvar_s scr_conscroll_y;
extern struct cvar_s scr_conscroll2_x;
extern struct cvar_s scr_conscroll2_y;
extern struct cvar_s scr_conscroll3_x;
extern struct cvar_s scr_conscroll3_y;
extern struct cvar_s scr_conbrightness;
extern struct cvar_s r_letterbox;

extern struct cvar_s scr_refresh;
extern struct cvar_s scr_stipple;

extern struct cvar_s r_stereo_separation;
extern struct cvar_s r_stereo_angle;
qbool R_Stereo_Active(void);
extern int r_stereo_side;

typedef struct scr_touchscreenarea_s
{
	const char *pic;
	const char *text;
	float rect[4];
	float textheight;
	float active;
	float activealpha;
	float inactivealpha;
}
scr_touchscreenarea_t;

// FIXME: should resize dynamically?
extern int scr_numtouchscreenareas;
extern scr_touchscreenarea_t scr_touchscreenareas[128];

#endif

