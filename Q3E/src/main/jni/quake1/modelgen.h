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

#ifndef MODELGEN_H
#define MODELGEN_H

#include "qtypes.h"

#define ALIAS_VERSION	6

#define ALIAS_ONSEAM				0x0020

typedef enum aliasframetype_e { ALIAS_SINGLE=0, ALIAS_GROUP } aliasframetype_t;

typedef enum aliasskintype_e { ALIAS_SKIN_SINGLE=0, ALIAS_SKIN_GROUP } aliasskintype_t;

typedef struct mdl_s
{
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
	int			synctype;
	int			flags;
	float		size;
}
mdl_t;

// TODO: could be shorts

typedef struct stvert_s
{
	int		onseam;
	int		s;
	int		t;
}
stvert_t;

typedef struct dtriangle_s
{
	int					facesfront;
	int					vertindex[3];
}
dtriangle_t;

#define DT_FACES_FRONT				0x0010

// This mirrors trivert_t in trilib.h, is present so Quake knows how to
// load this data

typedef struct trivertx_s
{
	unsigned char	v[3];
	unsigned char	lightnormalindex;
}
trivertx_t;

typedef struct daliasframe_s
{
	trivertx_t	bboxmin;	// lightnormal isn't used
	trivertx_t	bboxmax;	// lightnormal isn't used
	char		name[16];	// frame name from grabbing
}
daliasframe_t;

typedef struct daliasgroup_s
{
	int			numframes;
	trivertx_t	bboxmin;	// lightnormal isn't used
	trivertx_t	bboxmax;	// lightnormal isn't used
}
daliasgroup_t;

typedef struct daliasskingroup_s
{
	int			numskins;
}
daliasskingroup_t;

typedef struct daliasinterval_s
{
	float	interval;
}
daliasinterval_t;

typedef struct daliasskininterval_s
{
	float	interval;
}
daliasskininterval_t;

typedef struct daliasframetype_s
{
	aliasframetype_t	type;
}
daliasframetype_t;

typedef struct daliasskintype_s
{
	aliasskintype_t	type;
}
daliasskintype_t;

#endif

