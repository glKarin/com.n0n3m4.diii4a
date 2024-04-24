/*
Copyright (C) 2021 David Knapp (Cloudwalk)

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

#ifndef MODEL_VBSP_H
#define MODEL_VBSP_H

#include "qtypes.h"
#include "qdefs.h"

/*
 * The Valve BSP format, used by Source engine, has some interesting
 * advantages, but makes it quite messy.
 *
 * Notably, it supports version numbers for its lumps. Some games may introduce
 * new features to a lump and increment its version number while leaving the
 * VBSP version number the same.
 */

#define VBSPVERSION ('V' + 'B' * 256 + 'S' * 65536 + 'P' * 16777216)

// Content flags
#define HL2CONTENTS_NONE 0
#define HL2CONTENTS_SOLID 0x1
#define HL2CONTENTS_WINDOW 0x2
#define HL2CONTENTS_AUX 0x4
#define HL2CONTENTS_GRATE 0x8
#define HL2CONTENTS_SLIME 0x10
#define HL2CONTENTS_WATER 0x20
#define HL2CONTENTS_MIST 0x40
#define HL2CONTENTS_OPAQUE 0x80
#define HL2CONTENTS_TESTFOGVOLUME 0x100
#define HL2CONTENTS_TEAM1 0x800
#define HL2CONTENTS_TEAM2 0x1000
#define HL2CONTENTS_IGNORE_NODRAW_OPAQUE 0x2000
#define HL2CONTENTS_MOVEABLE 0x4000
#define HL2CONTENTS_AREAPORTAL 0x8000
#define HL2CONTENTS_PLAYERCLIP 0x10000
#define HL2CONTENTS_MONSTERCLIP 0x20000
#define HL2CONTENTS_CURRENT_0 0x40000
#define HL2CONTENTS_CURRENT_90 0x80000
#define HL2CONTENTS_CURRENT_180 0x100000
#define HL2CONTENTS_CURRENT_270 0x200000
#define HL2CONTENTS_CURRENT_UP 0x400000
#define HL2CONTENTS_CURRENT_DOWN 0x800000
#define HL2CONTENTS_ORIGIN 0x1000000
#define HL2CONTENTS_MONSTER 0x2000000
#define HL2CONTENTS_DEBRIS 0x4000000
#define HL2CONTENTS_DETAIL 0x8000000
#define HL2CONTENTS_TRANSLUCENT 0x10000000
#define HL2CONTENTS_LADDER 0x20000000
#define HL2CONTENTS_HITBOX 0x40000000

// Lumps

#define HL2HEADER_LUMPS 64

#define HL2LUMP_ENTITIES 0
#define HL2LUMP_PLANES 1
#define HL2LUMP_TEXDATA 2
#define HL2LUMP_VERTEXES 3
#define HL2LUMP_VISIBILITY 4
#define HL2LUMP_NODES 5
#define HL2LUMP_TEXINFO 6
#define HL2LUMP_FACES 7
#define HL2LUMP_LIGHTING 8
#define HL2LUMP_OCCLUSION 9
#define HL2LUMP_LEAFS 10
#define HL2LUMP_FACEIDS 11
#define HL2LUMP_EDGES 12
#define HL2LUMP_SURFEDGES 13
#define HL2LUMP_MODELS 14
#define HL2LUMP_WORLDLIGHTS 15
#define HL2LUMP_LEAFFACES 16
#define HL2LUMP_LEAFBRUSHES 17
#define HL2LUMP_BRUSHES 18
#define HL2LUMP_BRUSHSIDES 19
#define HL2LUMP_AREAS 20
#define HL2LUMP_AREAPORTALS 21
// These are unused in VBSP 20
#define HL2LUMP_PORTALS 22 // VBSP 19
#define HL2LUMP_PROPCOLLISION 22 // VBSP 21+
#define HL2LUMP_CLUSTERS 23 // VBSP 19
#define HL2LUMP_PROPHULLS 23 // VBSP 21+
#define HL2LUMP_PORTALVERTS 24 // VBSP 19
#define HL2LUMP_PROPHULLVERTS 24 // VBSP 21+
#define HL2LUMP_CLUSTERPORTALS 25 // VBSP 19
#define HL2LUMP_PROPTRIS 25 // VBSP 21+
// End unused
#define HL2LUMP_DISPINFO 26
#define HL2LUMP_ORIGINALFACES 27
#define HL2LUMP_PHYSDISP 28 // VBSP 20+ only
#define HL2LUMP_PHYSCOLLIDE 29
#define HL2LUMP_VERTNORMALS 30
#define HL2LUMP_VERTNORMALINDICES 31
#define HL2LUMP_DISP_LIGHTMAP_ALPHAS 32
#define HL2LUMP_DISP_VERTS 33
#define HL2LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS 34
#define HL2LUMP_GAME_LUMP 35
#define HL2LUMP_LEAFWATERDATA 36
#define HL2LUMP_PRIMITIVES 37
#define HL2LUMP_PRIMVERTS 38
#define HL2LUMP_PRIMINDICES 39
#define HL2LUMP_PAKFILE 40
#define HL2LUMP_CLIPPORTALVERTS 41
#define HL2LUMP_CUBEMAPS 42
#define HL2LUMP_TEXDATA_STRING_DATA 43
#define HL2LUMP_TEXDATA_STRING_TABLE 44
#define HL2LUMP_OVERLAYS 45
#define HL2LUMP_LEAFMINDISTTOWATER 46
#define HL2LUMP_FACE_MACRO_TEXTURE_INFO 47
#define HL2LUMP_DISP_TRIS 48
#define HL2LUMP_PHYSCOLLIDESURFACE 49 // VBSP 19 and 20
#define HL2LUMP_PROP_BLOB 49 // VBSP 21+
// Everything from here is VBSP 20+ only
#define HL2LUMP_WATEROVERLAYS 50
#define HL2LUMP_LEAF_AMBIENT_INDEX_HDR 51
#define HL2LUMP_LEAF_AMBIENT_INDEX 52
#define HL2LUMP_LIGHTING_HDR 53
#define HL2LUMP_WORLDLIGHTS_HDR 54
#define HL2LUMP_LEAF_AMBIENT_LIGHTING_HDR 55
#define HL2LUMP_LEAF_AMBIENT_LIGHTING 56
#define HL2LUMP_XZIPPAKFILE 57
#define HL2LUMP_FACES_HDR 58
#define HL2LUMP_MAP_FLAGS 59
#define HL2LUMP_OVERLAY_FADES 60
#define HL2LUMP_OVERLAY_SYSTEM_LEVELS 61
// VBSP 21+
#define HL2LUMP_PHYSLEVEL 62
#define HL2LUMP_DISP_MULTIBLEND 63

typedef struct hl2lump_s
{
	int32_t fileofs, filelen;
	int32_t version;
	int32_t id;
} hl2lump_t;

typedef struct hl2dheader_s
{
	int32_t ident;
	int32_t version;
	hl2lump_t lumps[HL2HEADER_LUMPS];
	int32_t revision;
} hl2dheader_t;

#endif
