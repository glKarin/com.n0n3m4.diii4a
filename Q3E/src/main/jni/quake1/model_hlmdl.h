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

#ifndef MODEL_HLMDL_H
#define MODEL_HLMDL_H

#include "qtypes.h"
#include "qdefs.h"

/*
 * The Half-Life MDL format is Valve's format for models in GoldSrc engine.
 * 
 * These headers were added based on information found at
 * https://github.com/malortie/assimp/wiki/MDL:-Half-Life-1-file-format
 */

#define HLPOLYHEADER (('T' << 24) + ('S' << 16) + ('D' << 8) + 'I')
#define HLMDLHEADER "IDST"
#define HLSEQHEADER "IDSQ"

// Flags
#define HLMDLFLAG_FLAT 0x1
#define HLMDLFLAG_CHROME 0x2
#define HLMDLFLAG_FULLBRIGHT 0x4

// Header
typedef struct dhlmdl_header_s
{
	int32_t id; // Should be IDST
	int32_t version; // Should be 10
	int8_t name[64];
	int32_t filesize;
	vec3_t eyeposition;
	vec3_t min, max;
	vec3_t bbmin, bbmax;
	int32_t flags;

	int32_t num_bones;
	int32_t ofs_bones;

	int32_t num_bonecontrollers;
	int32_t ofs_bonecontrollers;

	int32_t num_hitboxes;
	int32_t ofs_hitboxes;

	int32_t num_seq;
	int32_t ofs_seq;

	int32_t num_seqgroups;
	int32_t ofs_seqgroups;

	int32_t num_textures;
	int32_t ofs_textures;
	int32_t ofs_texturedata;

	int32_t num_skins;
	int32_t num_skingroups;
	int32_t ofs_skins;

	int32_t num_bodyparts;
	int32_t ofs_bodyparts;

	int32_t num_attachments;
	int32_t ofs_attachments;

	int32_t soundtable;
	int32_t soundindex;

	int32_t num_soundgroups;
	int32_t ofs_soundgroups;

	int32_t num_transitions;
	int32_t ofs_transitions;
} dhlmdl_header_t;

typedef struct dhlmdl_sequence_header_s
{
	int32_t id; // Should be IDSQ
	int32_t version; // Should be 10
	int8_t name[64];
	int32_t size;
} dhlmdl_sequence_header_t;

typedef struct dhlmdl_texture_s
{
	int8_t name[64];
	int32_t flags;
	int32_t w, h;
	int32_t ofs;
} dhlmdl_texture_t;

typedef struct dhlmdl_bone_s
{
	int8_t name[32];
	int32_t parent;
	int32_t flags;
	int32_t bonecontroller[6];
	float value[6];
	float scale[6];
} dhlmdl_bone_t;

typedef struct dhlmdl_bone_controller_s
{
	int32_t bone;
	int32_t type;
	float start;
	float end;
	int32_t rest;
	int32_t index;
} dhlmdl_bone_controller_t;

typedef struct dhlmdl_hitbox_s
{
	int32_t bone;
	int32_t group;
	vec3_t bbmin, bbmax;
} dhlmdl_hitbox_t;

typedef struct dhlmdl_sequence_group_s
{
	int8_t label[32];
	int8_t name[64];
	int64_t unused;
} dhlmdl_sequence_group_t;

typedef struct dhlmdl_sequence_description_s
{
	int8_t label[32];
	float fps;
	int32_t flags;
	int32_t activity;
	int32_t actweight;

	int32_t num_events;
	int32_t ofs_events;

	int32_t num_frames;
	
	int64_t unused0;
	
	int32_t motiontype;
	int32_t motionbone;
	vec3_t linearmovement;
	int64_t unused1;
	vec3_t bbmin, bbmax;

	int32_t num_blends;

	int32_t ofs_anim;

	int32_t blendtype[2];
	float blendstart[2], blendend[2];
	int32_t unused2; // blendparent
	int32_t seqgroup;
	int32_t entrynode;
	int32_t exitnode;
	int32_t nodeflags;
	int32_t unused3; // nextseq
} dhlmdl_sequence_description_t;

typedef uint16_t dhlmdl_animoffset_t[6];

typedef union dhlmdl_animvalue_s
{
	struct {
		uint8_t valid;
		uint8_t total;
	} num;
	int16_t value;
} dhlmdl_animvalue_t;

typedef struct dhlmdl_animevent_s
{
	int32_t frame;
	int32_t event;
	int32_t unused;
	int8_t options[64];
} dhlmdl_animevent_t;

typedef struct dhlmdl_attachment_s
{
	int8_t unused0[36];
	int32_t type;
	int32_t bone;
	vec3_t org;
	vec3_t unused1[3];
} dhlmdl_attachment_t;

typedef struct dhlmdl_bodypart_s
{
	int8_t name[64];
	int32_t num_models;
	int32_t base;
	int32_t ofs_models;
} dhlmdl_bodypart_t;

typedef struct dhlmdl_s
{
	int8_t name[64];
	int64_t unused0;

	int32_t num_mesh;
	int32_t ofs_mesh;

	int32_t num_verts;
	int32_t ofs_vertinfo;
	int32_t ofs_verts;
	
	int32_t num_norms;
	int32_t ofs_norminfo;
	int32_t ofs_norms;
	
	int64_t unused1;
} dhlmdl_t;

typedef struct dhlmdl_mesh_s
{
	int32_t num_tris;
	int32_t ofs_tris;
	int32_t ofs_skins;
	int64_t unused;
} dhlmdl_mesh_t;

typedef struct dhlmdl_trivert_s
{
	int16_t vertindex;
	int16_t normindex;
	int16_t s, t;
} dhlmdl_trivert_t;

#endif
