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
// modelgen.h: header file for model generation program
//

// *********************************************************
// * This file must be identical in the modelgen directory *
// * and in the Quake directory, because it's used to      *
// * pass data from one to the other via model files.      *
// *********************************************************

#ifdef INCLUDELIBS

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "cmdlib.h"
#include "scriplib.h"
#include "trilib.h"
#include "lbmlib.h"
#include "mathlib.h"

#endif

#define ALIAS_VERSION	6
#define QTESTALIAS_VERSION 3

#define ALIAS_ONSEAM				0x0020

// must match definition in spritegn.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
typedef enum {ST_SYNC=0, ST_RAND } synctype_t;
#endif

typedef enum { ALIAS_SINGLE=0, ALIAS_GROUP, ALIAS_GROUP_SWAPPED=0x01000000 } aliasframetype_t;

typedef enum { ALIAS_SKIN_SINGLE=0, ALIAS_SKIN_GROUP } aliasskintype_t;

typedef struct {
	int			ident;
	int			version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;
	int			skinwidth;
	int			skinheight;
	int			numverts;
	int			numtris;
	int			numframes;
	synctype_t	synctype;
//qtest stops here
	int			flags;		//offset 0x4c
	float		size;
//quake stops here
	int			num_st;
//rapo stops here
} dmdl_t;

// TODO: could be shorts

typedef struct {
	int		onseam;
	int		s;
	int		t;
} dstvert_t;

typedef struct {
	short		s;
	short		t;
} dmd2stvert_t;

typedef struct dtriangle_s {
	int					facesfront;
	int					vertindex[3];
} dtriangle_t;

typedef struct dh2triangle_s {
	int					facesfront;
	unsigned short		vertindex[3];
	unsigned short      stindex[3];
} dh2triangle_t;

typedef struct dmd2triangle_s {
	short					xyz_index[3];
	short					st_index[3];
} dmd2triangle_t;

#define DT_FACES_FRONT				0x0010

// This mirrors trivert_t in trilib.h, is present so Quake knows how to
// load this data

typedef struct {
	qbyte	v[3];
	qbyte	lightnormalindex;
} dtrivertx_t;

typedef struct {
	dtrivertx_t	bboxmin;	// lightnormal isn't used
	dtrivertx_t	bboxmax;	// lightnormal isn't used
// qtest stops here
	char		name[16];	// frame name from grabbing
} daliasframe_t;

typedef struct {
	int			numframes;
	dtrivertx_t	bboxmin;	// lightnormal isn't used
	dtrivertx_t	bboxmax;	// lightnormal isn't used
} daliasgroup_t;

typedef struct {
	int			numskins;
} daliasskingroup_t;

typedef struct {
	float	interval;
} daliasinterval_t;

typedef struct {
	float	interval;
} daliasskininterval_t;

typedef struct {
	aliasframetype_t	type;
} daliasframetype_t;

typedef struct {
	aliasskintype_t	type;
} daliasskintype_t;

#define IDPOLYHEADER	"IDPO",4 /*little-endian "IDPO"*/
#define RAPOLYHEADER	"RAPO",4 /*used by hexen2 mp*/
#define MD3_IDENT		"IDP3",4 /*quake3, duh*/


