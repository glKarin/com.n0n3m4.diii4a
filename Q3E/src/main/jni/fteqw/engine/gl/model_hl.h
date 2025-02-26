/*
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Half-Life Model Renderer (Experimental) Copyright (C) 2001 James 'Ender' Brown [ender@quakesrc.org] This program is
    free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details. You should have received a copy of the GNU General Public License along with this program; if not, write
    to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. fromquake.h -
    
	model_hl.h - halflife model structure
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
#define HLPOLYHEADER	(('T' << 24) + ('S' << 16) + ('D' << 8) + 'I')	/* little-endian "IDST" */
#define HLMDLHEADER		"IDST"

/* flags - eukara */
#define HLMDLFL_FLAT		0x0001
#define HLMDLFL_CHROME		0x0002
#define HLMDLFL_FULLBRIGHT	0x0004
#define HLMDLFL_MASKED		0x0040
#define HLMDLFL_ALPHASOLID	0x0800

#define HLSHADER_FULLBRIGHT \
		"{\n" \
			"program defaultskin\n" \
			"{\n" \
				"map $diffuse\n" \
			"}\n" \
		"}\n"

#define HLSHADER_CHROME \
		"{\n" \
			"program defaultskin#CHROME\n" \
			"{\n" \
				"map $diffuse\n" \
				"tcgen environment\n" \
				"rgbgen lightingdiffuse\n" \
			"}\n" \
		"}\n"

#define HLSHADER_MASKED \
		"{\n" \
			"program defaultskin#MASK=0.5\n" \
			"{\n" \
				"map $diffuse\n" \
				"rgbgen lightingdiffuse\n" \
				"alphaFunc GE128\n" \
			"}\n" \
		"}\n"

#define HLSHADER_FULLBRIGHTCHROME \
		"{\n" \
			"program defaultskin#CHROME\n" \
			"{\n" \
				"map $diffuse\n" \
				"tcgen environment\n" \
			"}\n" \
		"}\n"

/*
 -----------------------------------------------------------------------------------------------------------------------
    main model header
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	int		filetypeid;	//IDSP
	int		version;	//10
	char	name[64];
	int		filesize;
	vec3_t	unknown3[5];
	int		unknown4;	//flags
	int		numbones;
	int		boneindex;
	int		numcontrollers;
	int		controllerindex;
	int		num_hitboxes;
	int		ofs_hitboxes;
	int		numseq;
	int		seqindex;
	int		unknown6;		//external sequences
	int		seqgroups;
	int		numtextures;
	int		textures;
	int		unknown7;	//something to do with external textures
	int		skinrefs;
	int		skingroups;
	int		skins;
	int		numbodyparts;
	int		bodypartindex;
	int		num_attachments;
	int		ofs_attachments;
	int		unknown9[6];	//sounds, transitions
} hlmdl_header_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    skin info
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	char	name[64];
	int		flags; /*flat, chrome, fullbright*/
	int		w;	/* width */
	int		h;	/* height */
	int		offset;	/* index */
} hlmdl_tex_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    body part index
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	char	name[64];
	int		nummodels;
	int		base;
	int		modelindex;
} hlmdl_bodypart_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    meshes
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	int numtris;
	int index;
	int skinindex;
	int unknown2;
	int unknown3;
} hlmdl_mesh_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    bones
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	char	name[32];
	int		parent;
	int		unknown1;
	int		bonecontroller[6];
	float	value[6];
	float	scale[6];
} hlmdl_bone_t;

typedef struct
{
	char name[32];	//I assume
	int unk;
	int bone;
	vec3_t org;
	vec3_t unk2[3];
} hlmdl_attachment_t;

typedef struct
{
	int bone;
	int body;	//value reported to gamecode on impact
	vec3_t mins;
	vec3_t maxs;
} hlmdl_hitbox_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    bone controllers
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	int		name;
	int		type;
	float	start;
	float	end;
	int		unknown1;
	int		index;
} hlmdl_bonecontroller_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    halflife submodel descriptor
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	char	name[64];
	int		unknown1;
	float	unknown2;
	int		nummesh;
	int		meshindex;
	int		numverts;
	int		vertinfoindex;
	int		vertindex;
	int		unknown3[2];
	int		normindex;
	int		unknown4[2];
} hlmdl_submodel_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    animation
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	unsigned short	offset[6];
} hlmdl_anim_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    animation frames
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef union
{
	struct {
		qbyte	valid;
		qbyte	total;
	} num;
	short	value;
} hlmdl_animvalue_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    sequence descriptions
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	char	name[32];
	float	timing;
	int		loop;
	int		action;
	int		actionweight;
	int		num_events;
	int		ofs_events;
	int		numframes;
	int		unknown2[2];
	int		motiontype;
	int		motionbone;
	vec3_t	unknown3;
	int		unknown4[2];
	vec3_t	bbox[2];
	int		hasblendseq;
	int		index;
	int		unknown7[2];
	float	unknown[4];
	int		unknown8;
	unsigned int		seqindex;
	int		unknown9[4];
} hlmdl_sequencelist_t;

typedef struct
{
	int pose;
	int code;
	int unknown1;
	char data[64];
} hlmdl_event_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    sequence groups
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	char			name[96];	/* should be split label[32] and name[64] */
	unsigned int	cache;
	int				data;
} hlmdl_sequencedata_t;

typedef struct
{
	int magic;		//IDSQ
	int version;	//10
	char name[64];
	int unk1;
} hlmdl_sequencefile_t;
/*
 -----------------------------------------------------------------------------------------------------------------------
    halflife model internal structure
 -----------------------------------------------------------------------------------------------------------------------
 */

#define MAX_ANIM_GROUPS	16	//submodel files containing anim data.
typedef struct	//this is stored as the cache. an hlmodel_t is generated when drawing
{
	//updated while rendering...
	float	controller[5];				/* Position of bone controllers */
	float	adjust[5];

	hlmdl_header_t			*header;
	hlmdl_bone_t			*bones;
	struct galiasbone_s		*compatbones;
	hlmdl_bonecontroller_t	*bonectls;
	hlmdl_sequencefile_t	*animcache[MAX_ANIM_GROUPS];
	zonegroup_t				*memgroup;
	struct hlmodelshaders_s
	{
		char name[MAX_QPATH];
		char *defaultshadertext;
		texnums_t defaulttex;
		shader_t *shader;
		int w, h;
		int atlasid;
		unsigned short x,y;
	} *shaders;
	short *skinref;
	int numskinrefs;
	int numskingroups;

	int numgeomsets;
	struct
	{
		int numalternatives;
		struct hlalternative_s
		{
			int numsubmeshes;
			mesh_t *submesh;
		} *alternatives;
	} *geomset;
	mesh_t mesh;
	vbo_t vbo;
	qboolean vbobuilt;
} hlmodel_t;

/* HL mathlib prototypes: */
void	QuaternionGLAngle(const vec3_t angles, vec4_t quaternion);
void	QuaternionGLMatrix(float x, float y, float z, float w, vec4_t *GLM);
//void	UploadTexture(hlmdl_tex_t *ptexture, qbyte *data, qbyte *pal);

qboolean QDECL Mod_LoadHLModel (model_t *mod, void *buffer, size_t fsize);

/* physics stuff */
void *Mod_GetHalfLifeModelData(model_t *mod);

//reflectioney things, including bone data
int HLMDL_BoneForName(model_t *mod, const char *name);
int HLMDL_FrameForName(model_t *mod, const char *name);
int HLMDL_FrameForAction(model_t *mod, int actionid);
const char *HLMDL_FrameNameForNum(model_t *model, int surfaceidx, int num);
qboolean HLMDL_FrameInfoForNum(model_t *model, int surfaceidx, int num, char **name, int *numframes, float *duration, qboolean *loop, int *act);
qboolean HLMDL_GetModelEvent(model_t *model, int animation, int eventidx, float *timestamp, int *eventcode, char **eventdata);
int HLMDL_GetNumBones(model_t *mod, qboolean tagstoo);
int HLMDL_GetBoneParent(model_t *mod, int bonenum);
const char *HLMDL_GetBoneName(model_t *mod, int bonenum);
int HLMDL_GetBoneData(model_t *model, int firstbone, int lastbone, const framestate_t *fstate, float *result);
int HLMDL_GetAttachment(model_t *model, int tagnum, float *resultmatrix);

#ifdef HAVE_CLIENT
//stuff only useful for clients that need to draw stuff
void	R_DrawHLModel(entity_t	*curent);
void HLMDL_DrawHitBoxes(entity_t *ent);
void R_HalfLife_GenerateBatches(entity_t *rent, batch_t **batches);
void R_HalfLife_TouchTextures(model_t *mod);
#endif
