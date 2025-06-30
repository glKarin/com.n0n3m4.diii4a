/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#pragma once
#if !defined(WADFILE_H)
#define WADFILE_H

/*
========================================================================
.WAD archive format	(WhereAllData - WAD)

List of compressed files, that can be identify only by TYPE_*

<format>
header:	dwadinfo_t[dwadinfo_t]
file_1:	byte[dwadinfo_t[num]->disksize]
file_2:	byte[dwadinfo_t[num]->disksize]
file_3:	byte[dwadinfo_t[num]->disksize]
...
file_n:	byte[dwadinfo_t[num]->disksize]
infotable	dlumpinfo_t[dwadinfo_t->numlumps]
========================================================================
*/

#define IDWAD3HEADER	(('3'<<24)+('D'<<16)+('A'<<8)+'W')

// dlumpinfo_t->compression
#define CMP_NONE		0	// compression none
#define CMP_LZSS		1	// LZSS compression

// dlumpinfo_t->type
#define TYP_QPAL		64	// quake palette
#define TYP_QTEX		65	// probably was never used
#define TYP_QPIC		66	// quake1 and hl pic (lmp_t)
#define TYP_MIPTEX		67	// half-life (mip_t) previous was TYP_SOUND but never used in quake1
#define TYP_QMIP		68	// quake1 (mip_t) (replaced with TYPE_MIPTEX while loading)
#define TYP_RAW		69	// raw data
#define TYP_QFONT		70	// half-life font (qfont_t)

/*
========================================================================

.LMP image format	(Half-Life gfx.wad lumps)

========================================================================
*/
typedef struct lmp_s
{
	unsigned int	width;
	unsigned int	height;
} lmp_t;

/*
========================================================================

.MIP image format	(half-Life textures)

========================================================================
*/
typedef struct mip_s
{
	char		name[16];
	unsigned int	width;
	unsigned int	height;
	unsigned int	offsets[4];	// four mip maps stored
} mip_t;

#endif//WADFILE_H
