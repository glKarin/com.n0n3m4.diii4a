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
#if !defined(STUDIO_H)
#define STUDIO_H

/*
==============================================================================

STUDIO MODELS

Studio models are position independent, so the cache manager can move them.
==============================================================================
*/

// header
#define STUDIO_VERSION	10
#define IDSTUDIOHEADER	(('T'<<24)+('S'<<16)+('D'<<8)+'I') // little-endian "IDST"
#define IDSEQGRPHEADER	(('Q'<<24)+('S'<<16)+('D'<<8)+'I') // little-endian "IDSQ"

// studio limits
#define MAXSTUDIOTRIANGLES		32768	// max triangles per model
#define MAXSTUDIOVERTS		4096	// max vertices per submodel
#define MAXSTUDIOSEQUENCES		2048	// total animation sequences
#define MAXSTUDIOSKINS		256	// total textures
#define MAXSTUDIOSRCBONES		512	// bones allowed at source movement
#define MAXSTUDIOBONES		128	// total bones actually used
#define MAXSTUDIOMODELS		32	// sub-models per model
#define MAXSTUDIOBODYPARTS		32	// body parts per submodel
#define MAXSTUDIOGROUPS		16	// sequence groups (e.g. barney01.mdl, barney02.mdl, e.t.c)
#define MAXSTUDIOANIMATIONS		2048	// max frames per sequence
#define MAXSTUDIOMESHES		256	// max textures per model
#define MAXSTUDIOEVENTS		1024	// events per model
#define MAXSTUDIOPIVOTS		256	// pivot points
#define MAXSTUDIOBLENDS		16	// max anim blends
#define MAXSTUDIOCONTROLLERS		8	// max controllers per model
#define MAXSTUDIOATTACHMENTS		4	// max attachments per model

// client-side model flags
#define STUDIO_ROCKET		0x0001	// leave a trail
#define STUDIO_GRENADE		0x0002	// leave a trail
#define STUDIO_GIB			0x0004	// leave a trail
#define STUDIO_ROTATE		0x0008	// rotate (bonus items)
#define STUDIO_TRACER		0x0010	// green split trail
#define STUDIO_ZOMGIB		0x0020	// small blood trail
#define STUDIO_TRACER2		0x0040	// orange split trail + rotate
#define STUDIO_TRACER3		0x0080	// purple trail
#define STUDIO_DYNAMIC_LIGHT		0x0100	// dynamically get lighting from floor or ceil (flying monsters)
#define STUDIO_TRACE_HITBOX		0x0200	// always use hitbox trace instead of bbox

// lighting & rendermode options
#define STUDIO_NF_FLATSHADE		0x0001
#define STUDIO_NF_CHROME		0x0002
#define STUDIO_NF_FULLBRIGHT		0x0004
#define STUDIO_NF_COLORMAP		0x0008	// can changed by colormap command
#define STUDIO_NF_ALPHA		0x0010	// rendering as transparent texture
#define STUDIO_NF_ADDITIVE		0x0020	// rendering with additive mode
#define STUDIO_NF_TRANSPARENT		0x0040	// use texture with alpha channel
#define STUDIO_NF_NORMALMAP		0x0080	// indexed normalmap
#define STUDIO_NF_GLOSSMAP		0x0100	// heightmap that can be used for parallax or normalmap
#define STUDIO_NF_GLOSSPOWER		0x0200	// glossmap
#define STUDIO_NF_UV_COORDS		(1<<31)	// using fixed coords instead of ST

// motion flags
#define STUDIO_X			0x0001
#define STUDIO_Y			0x0002	
#define STUDIO_Z			0x0004
#define STUDIO_XR			0x0008
#define STUDIO_YR			0x0010
#define STUDIO_ZR			0x0020
#define STUDIO_LX			0x0040
#define STUDIO_LY			0x0080
#define STUDIO_LZ			0x0100
#define STUDIO_AX			0x0200
#define STUDIO_AY			0x0400
#define STUDIO_AZ			0x0800
#define STUDIO_AXR			0x1000
#define STUDIO_AYR			0x2000
#define STUDIO_AZR			0x4000
#define STUDIO_TYPES		0x7FFF
#define STUDIO_RLOOP		0x8000	// controller that wraps shortest distance

// bonecontroller types
#define STUDIO_MOUTH		4	// hardcoded

// sequence flags
#define STUDIO_LOOPING		0x0001
#define STUDIO_STATIC		0x8000	// studiomodel is static

// bone flags
#define STUDIO_HAS_NORMALS		0x0001
#define STUDIO_HAS_VERTICES		0x0002
#define STUDIO_HAS_BBOX		0x0004
#define STUDIO_HAS_CHROME		0x0008	// if any of the textures have chrome on them

typedef struct
{
	int		ident;
	int		version;

	char		name[64];
	int		length;

	vec3_t		eyeposition;	// ideal eye position
	vec3_t		min;		// ideal movement hull size
	vec3_t		max;			

	vec3_t		bbmin;		// clipping bounding box
	vec3_t		bbmax;		

	int		flags;

	int		numbones;		// bones
	int		boneindex;

	int		numbonecontrollers;	// bone controllers
	int		bonecontrollerindex;

	int		numhitboxes;	// complex bounding boxes
	int		hitboxindex;			
	
	int		numseq;		// animation sequences
	int		seqindex;

	int		numseqgroups;	// demand loaded sequences
	int		seqgroupindex;

	int		numtextures;	// raw textures
	int		textureindex;
	int		texturedataindex;

	int		numskinref;	// replaceable textures
	int		numskinfamilies;
	int		skinindex;

	int		numbodyparts;		
	int		bodypartindex;

	int		numattachments;	// queryable attachable points
	int		attachmentindex;

	int		soundtable;
	int		soundindex;
	int		soundgroups;
	int		soundgroupindex;

	int		numtransitions;	// animation node to animation node transition graph
	int		transitionindex;
} studiohdr_t;

// header for demand loaded sequence group data
typedef struct 
{
	int		id;
	int		version;

	char		name[64];
	int		length;
} studioseqhdr_t;

// bones
typedef struct 
{
	char		name[32];		// bone name for symbolic links
	int		parent;		// parent bone
	int		flags;		// ??
	int		bonecontroller[6];	// bone controller index, -1 == none
	float		value[6];		// default DoF values
	float		scale[6];		// scale for delta DoF values
} mstudiobone_t;

// bone controllers
typedef struct 
{
	int		bone;		// -1 == 0
	int		type;		// X, Y, Z, XR, YR, ZR, M
	float		start;
	float		end;
	int		rest;		// byte index value at rest
	int		index;		// 0-3 user set controller, 4 mouth
} mstudiobonecontroller_t;

// intersection boxes
typedef struct
{
	int		bone;
	int		group;		// intersection group
	vec3_t		bbmin;		// bounding box
	vec3_t		bbmax;		
} mstudiobbox_t;

#if !defined(CACHE_USER)
#define CACHE_USER
typedef struct cache_user_s
{
	void		*data;		// extradata
} cache_user_t;
#endif

// demand loaded sequence groups
typedef struct
{
	char		label[32];	// textual name
	char		name[64];	// file name
	int		unused1;	// was "cache"  - index pointer
	int		unused2;	// was "data" -  hack for group 0
} mstudioseqgroup_t;

// sequence descriptions
typedef struct
{
	char		label[32];	// sequence label

	float		fps;		// frames per second	
	int		flags;		// looping/non-looping flags

	int		activity;
	int		actweight;

	int		numevents;
	int		eventindex;

	int		numframes;	// number of frames per sequence

	int		numpivots;	// number of foot pivots
	int		pivotindex;

	int		motiontype;	
	int		motionbone;
	vec3_t		linearmovement;
	int		automoveposindex;
	int		automoveangleindex;

	vec3_t		bbmin;		// per sequence bounding box
	vec3_t		bbmax;		

	int		numblends;
	int		animindex;	// mstudioanim_t pointer relative to start of sequence group data
					// [blend][bone][X, Y, Z, XR, YR, ZR]

	int		blendtype[2];	// X, Y, Z, XR, YR, ZR
	float		blendstart[2];	// starting value
	float		blendend[2];	// ending value
	int		blendparent;

	int		seqgroup;		// sequence group for demand loading

	int		entrynode;	// transition node at entry
	int		exitnode;		// transition node at exit
	int		nodeflags;	// transition rules
	
	int		nextseq;		// auto advancing sequences
} mstudioseqdesc_t;

// events
#include "studio_event.h"

// pivots
typedef struct 
{
	vec3_t		org;		// pivot point
	int		start;
	int		end;
} mstudiopivot_t;

// attachment
typedef struct 
{
	char		name[32];
	int		type;
	int		bone;
	vec3_t		org;		// attachment point
	vec3_t		vectors[3];
} mstudioattachment_t;

typedef struct
{
	unsigned short	offset[6];
} mstudioanim_t;

// animation frames
typedef union 
{
	struct
	{
		byte	valid;
		byte	total;
	} num;
	short		value;
} mstudioanimvalue_t;

// body part index
typedef struct
{
	char		name[64];
	int		nummodels;
	int		base;
	int		modelindex;	// index into models array
} mstudiobodyparts_t;

// skin info
typedef struct mstudiotex_s
{
	char		name[64];
	unsigned short	flags;
	unsigned short	unused;		// lower 16 bit is a user area
	int		width;
	int		height;
	int		index;
} mstudiotexture_t;

// skin families
// short	index[skinfamilies][skinref]

// studio models
typedef struct
{
	char		name[64];

	int		type;
	float		boundingradius;

	int		nummesh;
	int		meshindex;

	int		numverts;		// number of unique vertices
	int		vertinfoindex;	// vertex bone info
	int		vertindex;	// vertex vec3_t
	int		numnorms;		// number of unique surface normals
	int		norminfoindex;	// normal bone info
	int		normindex;	// normal vec3_t

	int		numgroups;	// deformation groups
	int		groupindex;
} mstudiomodel_t;

// vec3_t	boundingbox[model][bone][2];		// complex intersection info

// meshes
typedef struct 
{
	int		numtris;
	int		triindex;
	int		skinref;
	int		numnorms;		// per mesh normals
	int		normindex;	// normal vec3_t
} mstudiomesh_t;

// triangles
typedef struct 
{
	short		vertindex;	// index into vertex array
	short		normindex;	// index into normal array
	short		s,t;		// s,t position on skin
} mstudiotrivert_t;

#endif//STUDIO_H
