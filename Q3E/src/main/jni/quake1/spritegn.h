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
//
// spritegn.h: header file for sprite generation program
//

// **********************************************************
// * This file must be identical in the spritegen directory *
// * and in the Quake directory, because it's used to       *
// * pass data from one to the other via .spr files.        *
// **********************************************************

#ifndef SPRITEGEN_H
#define SPRITEGEN_H

//-------------------------------------------------------
// This program generates .spr sprite package files.
// The format of the files is as follows:
//
// dsprite_t file header structure
// <repeat dsprite_t.numframes times>
//   <if spritegroup, repeat dspritegroup_t.numframes times>
//     dspriteframe_t frame header structure
//     sprite bitmap
//   <else (single sprite frame)>
//     dspriteframe_t frame header structure
//     sprite bitmap
// <endrepeat>
//-------------------------------------------------------

#define SPRITE_VERSION		1
#define SPRITEHL_VERSION	2
#define SPRITE32_VERSION	32

#define SPRITE2_VERSION		2

typedef struct dsprite_s
{
	int			ident;
	int			version;
	int			type;
	float		boundingradius;
	int			width;
	int			height;
	int			numframes;
	float		beamlength;
	int			synctype;
} dsprite_t;

typedef struct dspritehl_s
{
	int			ident;
	int			version;
	int			type;
	int			rendermode;
	float		boundingradius;
	int			width;
	int			height;
	int			numframes;
	float		beamlength;
	int			synctype;
} dspritehl_t;

typedef struct dsprite2frame_s
{
	int		width, height;
	int		origin_x, origin_y;		// raster coordinates inside pic
	char	name[64];				// name of pcx file
} dsprite2frame_t;

typedef struct dsprite2_s
{
	int				ident;
	int				version;
	int				numframes;
	dsprite2frame_t	frames[1];		// variable sized
} dsprite2_t;

#define SPR_VP_PARALLEL_UPRIGHT		0
#define SPR_FACING_UPRIGHT			1
#define SPR_VP_PARALLEL				2
#define SPR_ORIENTED				3
#define SPR_VP_PARALLEL_ORIENTED	4
#define SPR_LABEL               	5
#define SPR_LABEL_SCALE         	6
#define SPR_OVERHEAD				7

#define SPRHL_OPAQUE	0
#define SPRHL_ADDITIVE	1
#define SPRHL_INDEXALPHA	2
#define SPRHL_ALPHATEST	3

typedef struct dspriteframe_s {
	int			origin[2];
	int			width;
	int			height;
} dspriteframe_t;

typedef struct dspritegroup_s {
	int			numframes;
} dspritegroup_t;

typedef struct dspriteinterval_s {
	float	interval;
} dspriteinterval_t;

typedef enum spriteframetype_e { SPR_SINGLE=0, SPR_GROUP } spriteframetype_t;

typedef struct dspriteframetype_s {
	spriteframetype_t	type;
} dspriteframetype_t;

#endif
