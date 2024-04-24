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

#ifndef SBAR_H
#define SBAR_H

#define	SBAR_HEIGHT		24

extern	int			sb_lines;			///< scan lines to draw
extern	struct cvar_s		sbar_alpha_bg;
extern	struct cvar_s		sbar_alpha_fg;

void Sbar_Init (void);

/// called every frame by screen
void Sbar_Draw (void);

void Sbar_ShowFPS(void);
void Sbar_ShowFPS_Update(void);

int Sbar_GetSortedPlayerIndex (int index);
void Sbar_SortFrags (void);

extern cvar_t cl_deathscoreboard;
void Sbar_DeathmatchOverlay (void);

#endif

