#include "../plugin.h"
#include "../engine/common/com_mesh.h"


/*
   Half-Life 2 / Source models store much of their data in other files.
   I'm only going to try loading simple static models, so we don't need much data from the .mdl itself.

   FIXME: multiple meshes are still buggy.
   FIXME: materials are not loaded properly.
   FIXME: no lod stuff.
*/

static plugfsfuncs_t *filefuncs;
static plugmodfuncs_t *modfuncs;
static plugfsfuncs_t *fsfuncs;

//Utility functions. silly plugins.
float Length(const vec3_t v) {return sqrt(DotProduct(v,v));}
float RadiusFromBounds (const vec3_t mins, const vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length (corner);
}

//blargh
typedef struct
{
	unsigned int magic;
	unsigned int version;
	unsigned int revisionid;
	char name[64];
	unsigned int filesize;

	vec3_t _80;
	vec3_t _92;
	vec3_t mins;
	vec3_t maxs;
	vec3_t _128;
	vec3_t _140;
	unsigned int _152;
	unsigned int num_bones;
	unsigned int ofs_bones;		//hl2mdlbone_t
	unsigned int _164;
	unsigned int _168;
	unsigned int _172;
	unsigned int _176;
	unsigned int num_anims;
	unsigned int ofs_anims;		//hl2mdlanim_t
	unsigned int _188;
	unsigned int _192;
	unsigned int _196;
	unsigned int _200;
	unsigned int tex_count;
	unsigned int tex_ofs;		//hl2mdltexture_t
	unsigned int texpath_count;
	unsigned int texpath_ofs;	//hl2mdltexturepath_t
	unsigned int texbind_count;	//N slots per skin
	unsigned int skin_count;
	unsigned int texbind_offset;//provides skin|slot->texture mappings
	unsigned int body_count;
	unsigned int body_ofs;		//hl2mdlbody_t
	unsigned int _240;
	unsigned int _244;
	unsigned int _248;
	unsigned int _252;
	unsigned int _256;
	unsigned int _260;
	unsigned int _264;
	unsigned int _268;
	unsigned int _272;
	unsigned int _276;
	unsigned int _280;
	unsigned int _284;
	unsigned int _288;
	unsigned int _292;
	unsigned int _296;
	unsigned int _300;
	unsigned int _304;
	unsigned int _308;
	unsigned int _312;
	unsigned int _316;
	unsigned int _320;
	unsigned int _324;
	unsigned int _328;
	unsigned int _332;
	unsigned int _336;
	unsigned int _340;
	unsigned int _344;
	unsigned int ofs_aniname;	//348
	unsigned int num_aniblocks;
	unsigned int ofs_aniblocks;
	//other stuff?
} hl2mdlheader_t;
typedef struct
{
	unsigned int name_ofs;
	int parent;
	int junk[6];
	vec3_t pos;
	float quat[4];
	vec3_t rot;
	vec3_t posscale;
	vec3_t rotscale;
	float inverse[12];
	int junk3[18];
} hl2mdlbone_t;
typedef struct
{
	qbyte bone;		//skip ones in their base pose.
	qbyte flags;	//says what the data is
	short next;		//to walk the list when its variable sized...
	unsigned short data[1];
} hl2mdlpose_t;
typedef struct
{
	unsigned int _0;
	unsigned int name_ofs;
	float fps;
	unsigned int loop;
	unsigned int numframes;
	unsigned int _20;
	unsigned int _24;
	unsigned int _28;
	unsigned int _32;
	unsigned int _36;
	unsigned int _40;
	unsigned int _44;
	unsigned int _48;
	struct poseofs_s{
		unsigned int externalblock;
		unsigned int ofs_pose;	//hl2mdlpose_t
	} defpose;
	unsigned int _60;
	unsigned int _64;
	unsigned int _68;
	unsigned int _72;
	unsigned int _76;
	unsigned int ofs_posesections;	//{block, ofs}
	unsigned int posespersection;	//0 if its small enough to avoid the extra indirection.
	unsigned int _88;
	unsigned int _92;
	unsigned int _96;
} hl2mdlanim_t;
typedef struct
{
	unsigned int name_ofs;
	unsigned int surf_count;
	unsigned int base;
	unsigned int surf_ofs;
} hl2mdlbody_t;
typedef struct
{
	char name[64];
	unsigned int type;
	unsigned int radius;
	unsigned int mesh_count;
	unsigned int mesh_ofs;
	unsigned int vertex_count;
	unsigned int _84;
	unsigned int _88;
	unsigned int _92;
	unsigned int _96;
	unsigned int _100;
	unsigned int _104;
	unsigned int _108;
	unsigned int _112;
	unsigned int _116;
	unsigned int _120;
	unsigned int _124;
	unsigned int _128;
	unsigned int _132;
	unsigned int _136;
	unsigned int _140;
	unsigned int _144;
} hl2mdlsurf_t;
typedef struct
{
	unsigned int mat_idx;
	unsigned int model_ofs;
	unsigned int vert_count;
	unsigned int vert_first;
	unsigned int _16;
	unsigned int _20;
	unsigned int _24;
	unsigned int _28;
	unsigned int _32;
	unsigned int _36;
	unsigned int _40;
	unsigned int _44;
	unsigned int _48;
	unsigned int _52;
	unsigned int _56;
	unsigned int _60;
	unsigned int _64;
	unsigned int _68;
	unsigned int _72;
	unsigned int _76;
	unsigned int _80;
	unsigned int _84;
	unsigned int _88;
	unsigned int _92;
	unsigned int _96;
	unsigned int _100;
	unsigned int _104;
	unsigned int _108;
	unsigned int _112;
} hl2mdlmesh_t;
typedef struct
{
	unsigned int nameofs;
	unsigned int _4;
	unsigned int _8;
	unsigned int _12;
	unsigned int _16;
	unsigned int _20;
	unsigned int _24;
	unsigned int _28;
	unsigned int _32;
	unsigned int _36;
	unsigned int _40;
	unsigned int _44;
	unsigned int _48;
	unsigned int _52;
	unsigned int _56;
	unsigned int _60;
} hl2mdltexture_t;
typedef struct
{
	unsigned int nameofs;
} hl2mdltexturepath_t;
#pragma pack(push,1)	//urgh wtf is this bullshit
typedef struct
{
	unsigned int numskins;
	unsigned int offsetskin;
} hl2vtxskins_t;
typedef struct
{
	unsigned short foo;
	//no padding
	unsigned int offsetskinname;
} hl2vtxskin_t;
typedef struct
{
	unsigned int version;
	unsigned int vertcachesize;
	unsigned short bonesperstrip;
	unsigned short bonespertri;
	unsigned int bonespervert;
	unsigned int revisionid;
	unsigned int lod_count;
	unsigned int texreplacements_offset;
	unsigned int body_count;
	unsigned int body_ofs;
} hl2vtxheader_t;
typedef struct
{
	unsigned int surf_count;
	unsigned int surf_ofs;
} hl2vtxbody_t;
typedef struct
{
	unsigned int lod_count;
	unsigned int lod_ofs;
} hl2vtxsurf_t;
typedef struct
{
	unsigned int mesh_count;
	unsigned int mesh_ofs;
	float dist;
} hl2vtxlod_t;
typedef struct
{
	unsigned int stripg_count;
	unsigned int stripg_ofs;
	unsigned char flags;
	//no padding (3 bytes)
} hl2vtxmesh_t;
typedef struct
{
	unsigned int vert_count;
	unsigned int vert_ofs;
	unsigned int idx_count;
	unsigned int idx_ofs;
	unsigned int strip_count;
	unsigned int strip_ofs;
	unsigned char flags;
	//no padding (3 bytes)
} hl2vtxstripg_t;
typedef struct
{
	unsigned int idx_num;
	unsigned int idx_ofs;
	unsigned int vert_count;
	unsigned int vert_ofs;
	unsigned short bone_count;
	unsigned char flags;
	//no padding (1 byte)
	unsigned int bonestate_count;
	unsigned int bonestate_ofs;
} hl2vtxstrip_t;
typedef struct
{
	qbyte bone[3];
	qbyte bone_count;
	unsigned short vert;
	qbyte boneID[3];
	//no padding
} hl2vtxvert_t;
typedef struct
{
	unsigned int magic;
	unsigned int version;
	unsigned int revisionid;
	unsigned int lod_count;
	unsigned int lodverts_count[8];
	unsigned int fixups_count;
	unsigned int fixups_offset;
	unsigned int verts_offset;
	unsigned int tangents_offset;
} hl2vvdheader_t;
typedef struct
{
	unsigned int lod;
	unsigned int sourcevert;
	unsigned int numverts;
} hl2vvdfixup_t;
typedef struct
{
	float weight[3];
	qbyte bone[3];
	qbyte numbones;
	vec3_t xyz;
	vec3_t norm;
	vec2_t st;
} hl2vvdvert_t;
#pragma pack(pop)
/*seriously, how many structs do you need?*/
typedef struct
{
	model_t *mod;
	hl2mdlheader_t *header;
	void *ani;

	float *basepose;
	float *baserelpose;
	galiasbone_t *bones;

	unsigned int num_animations;
	galiasanimation_t *ofs_animations;

	unsigned numverts;
	vec2_t *ofs_st_array;
	vecV_t *ofs_skel_xyz;
	vec3_t *ofs_skel_norm;
	vec3_t *ofs_skel_svect;
	vec3_t *ofs_skel_tvect;
	byte_vec4_t *ofs_skel_idx;
	vec4_t *ofs_skel_weight;
	struct
	{
		unsigned int numfixups;
		index_t *fixup;
	} lod[1];	//must remain at 1 (instead of 8) until fixups are handled.
} hl2parsecontext_t;

static index_t *Mod_HL2_LoadIndexes(hl2parsecontext_t *ctx, unsigned int *idxcount, const hl2vtxmesh_t *vmesh, unsigned int lod, index_t firstindex, index_t bias)
{
	size_t numidx = 0, g;
	const hl2vtxstripg_t *vg;
	index_t *idx, *ret = NULL;

	vg = (const void*)((const qbyte*)vmesh+vmesh->stripg_ofs);
	for (g = 0; g < vmesh->stripg_count; g++, vg++)
	{
		if (vg->idx_count%3)
		{
			*idxcount = 0;
			return NULL;
		}
		numidx += vg->idx_count;
	}

	ret = idx = plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(*idx)*numidx);

	vg = (const void*)((const qbyte*)vmesh+vmesh->stripg_ofs);
	for (g = 0; g < vmesh->stripg_count; g++, vg++)
	{
		const unsigned short *in = (const void*)((const qbyte*)vg+vg->idx_ofs);
		const unsigned short *e = in+vg->idx_count;
		const hl2vtxvert_t *v = (const void*)((const qbyte*)vg+vg->vert_ofs);
		if (ctx->lod[lod].numfixups)
		{
			index_t *fixup = ctx->lod[lod].fixup;
			for(;;)
			{
				if (in == e)
					break;
				*idx++ = fixup[v[*in++].vert+firstindex] - bias;
			}
		}
		else
		{
			for(;;)
			{
				if (in == e)
					break;
				*idx++ = v[*in++].vert+firstindex - bias;
			}
		}
	}
	*idxcount = idx-ret;
	return ret;
}
static qboolean Mod_HL2_LoadVTX(hl2parsecontext_t *ctx, const void *buffer, size_t fsize)
{	//horribly overcomplicated way to express this stuff.
	const hl2mdlheader_t *mdl = ctx->header;
	size_t totalsurfs = 0, b, s, l, m, t, z;
	const hl2vtxheader_t *header = buffer;
	const hl2vtxbody_t *vbody;
	const hl2vtxsurf_t *vsurf;
	const hl2vtxlod_t *vlod;
	const hl2vtxmesh_t *vmesh;
//	const hl2vtxskins_t *vskins;
//	const hl2vtxskin_t *vskin;

	const hl2mdlbody_t *mbody = (const hl2mdlbody_t*)((const qbyte*)mdl + mdl->body_ofs);
	const hl2mdltexture_t *mtex = (const hl2mdltexture_t*)((const qbyte*)mdl + mdl->tex_ofs);
	const unsigned short *skinbind;

	galiasinfo_t *surf=NULL;
	galiasskin_t *skin;
	skinframe_t *skinframe;
	size_t firstvert = 0;

	if (fsize < sizeof(*header) || header->version != 7 || header->revisionid != ctx->header->revisionid || header->body_count == 0)
		return false;

	vbody = (const void*)((const qbyte*)header + header->body_ofs);
	for (b = 0; b < header->body_count; b++, vbody++)
	{
		vsurf = (const void*)((const qbyte*)vbody + vbody->surf_ofs);
		for (s = 0; s < vbody->surf_count; s++, vsurf++)
		{
			vlod = (const void*)((const qbyte*)vsurf + vsurf->lod_ofs);
			for (l = 0; l < min(vsurf->lod_count, countof(ctx->lod)); l++, vlod++)
				totalsurfs += vlod->mesh_count;
		}
	}

	if (!totalsurfs)
		return false;

	ctx->mod->meshinfo = surf = plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(*surf)*totalsurfs);

	t = mdl->skin_count*mdl->texbind_count;
	skinbind = (const unsigned short*)((const qbyte*)mdl+mdl->texbind_offset);
	skin = plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(*skin)*t + sizeof(*skinframe)*t);
	skinframe = (skinframe_t*)(skin+t);
	for (s = 0; s < mdl->skin_count; s++)
	for (t = 0; t < mdl->texbind_count; t++)
	{
		galiasskin_t *ns = &skin[s + t*mdl->skin_count];
		Q_snprintfz(ns->name, sizeof(ns->name), "skin%u %u", (unsigned)s, (unsigned)t);

		m = *skinbind++;
		if (mdl->texpath_count)
		{
			for (z = 0; z < mdl->texpath_count; z++) {
				char fsTest[MAX_QPATH];
				const hl2mdltexturepath_t *mpath = (const hl2mdltexturepath_t*)((const qbyte*)mdl + mdl->texpath_ofs + sizeof(hl2mdltexturepath_t) * z);
				Q_strlcpy(skinframe->shadername, (const char*)mdl+mpath->nameofs, sizeof(skinframe->shadername));
				Q_strlcat(skinframe->shadername, (const char*)&mtex[m]+mtex[m].nameofs, sizeof(skinframe->shadername));
				Q_strlcat(skinframe->shadername, ".vmt", sizeof(skinframe->shadername));
				Q_snprintfz(fsTest, sizeof(fsTest), "materials\\%s", skinframe->shadername);

				if (fsfuncs->LocateFile(fsTest, FSLF_IFFOUND, NULL)) {
					break;
				}
			}
		}
		else
		{
			modfuncs->StripExtension((const char*)ctx->mod->name, skinframe->shadername, sizeof(skinframe->shadername));
			Q_strlcat(skinframe->shadername, "/", sizeof(skinframe->shadername));
			Q_strlcat(skinframe->shadername, (const char*)&mtex[m]+mtex[m].nameofs, sizeof(skinframe->shadername));
			Q_strlcat(skinframe->shadername, ".vmt", sizeof(skinframe->shadername));
		}

		ns->numframes = 1;	//no skingroups... not that kind anyway.
		ns->skinspeed = 10;
		ns->frame = skinframe;
		skinframe++;
	}

	vbody = (const void*)((const qbyte*)header + header->body_ofs);
	for (b = 0; b < header->body_count; b++, vbody++, mbody++)
	{
		const hl2mdlsurf_t *msurf = (const hl2mdlsurf_t*)((const qbyte*)mbody + mbody->surf_ofs);
		vsurf = (const void*)((const qbyte*)vbody + vbody->surf_ofs);
		for (s = 0; s < vbody->surf_count; s++, vsurf++, msurf++)
		{
			vlod = (const void*)((const qbyte*)vsurf + vsurf->lod_ofs);
//			vskins = (const hl2vtxskins_t*)((const qbyte*)header + header->texreplacements_offset);
			for (l = 0, t = 0; l < min(vsurf->lod_count, countof(ctx->lod)); l++, vlod++/*, vskins++*/)
			{
				const hl2mdlmesh_t *mmesh = (const hl2mdlmesh_t*)((const qbyte*)msurf + msurf->mesh_ofs);
				vmesh = (const void*)((const qbyte*)vlod + vlod->mesh_ofs);
				for (m = 0; m < vlod->mesh_count; m++, vmesh++, mmesh++)
				{
					Q_snprintfz(surf->surfacename, sizeof(surf->surfacename), "%s:%s:l%u:m%u", (const char*)mbody+mbody->name_ofs, msurf->name, (unsigned)l, (unsigned)m);

					/*animation info*/
					surf->numanimations = ctx->num_animations;
					surf->ofsanimations = ctx->ofs_animations;	//we have no animation data

					surf->baseframeofs = ctx->basepose;
					surf->ofsbones = ctx->bones;
					surf->numbones = mdl->num_bones;
					surf->shares_bones = 0;	//all the same id

					#ifndef SERVERONLY
					/*skin data*/
					surf->numskins = mdl->skin_count;
					surf->ofsskins = skin+mmesh->mat_idx*mdl->skin_count;

					/*vertdata*/
					surf->ofs_rgbaf = NULL;
					surf->ofs_rgbaub = NULL;
					surf->ofs_st_array = ctx->ofs_st_array;
					#endif
					surf->shares_verts = 0;
					surf->numverts = ctx->numverts;
					surf->ofs_skel_xyz = ctx->ofs_skel_xyz;
					surf->ofs_skel_norm = ctx->ofs_skel_norm;
					surf->ofs_skel_svect = ctx->ofs_skel_svect;
					surf->ofs_skel_tvect = ctx->ofs_skel_tvect;
					surf->ofs_skel_idx = ctx->ofs_skel_idx;
					surf->ofs_skel_weight = ctx->ofs_skel_weight;

					/*index data*/
					if (mmesh->vert_first+mmesh->vert_count > surf->numverts)
						surf->ofs_indexes = NULL, surf->numindexes = 0;	//erk?
					else if (!ctx->lod[l].numfixups /*&& mdl->num_bones > sh_config.max_gpu_bones*/)
					{	//FIXME: fixups make this too screwy, so we can't use this path when they're in use, which means we will probably exceed out gpu bones limit more than we otherwise would (just a perf issue)
						unsigned int bias = firstvert+mmesh->vert_first;
						surf->numverts = mmesh->vert_count;
						surf->ofs_st_array += bias;
						surf->ofs_skel_xyz += bias;
						surf->ofs_skel_norm += bias;
						surf->ofs_skel_svect += bias;
						surf->ofs_skel_tvect += bias;
						surf->ofs_skel_idx += bias;
						surf->ofs_skel_weight += bias;
						surf->shares_verts = surf-(galiasinfo_t*)ctx->mod->meshinfo;

						surf->ofs_indexes = Mod_HL2_LoadIndexes(ctx, &surf->numindexes, vmesh, l, bias, bias);
					}
					else
						surf->ofs_indexes = Mod_HL2_LoadIndexes(ctx, &surf->numindexes, vmesh, l, firstvert+mmesh->vert_first, 0);

					/*misc data*/
					surf->geomset = 0;
					surf->geomid = 0;
					surf->contents = FTECONTENTS_BODY;
					surf->csurface.flags = 0;
					surf->surfaceid = b;
					surf->mindist = 0;	/*fixme: lods*/
					surf->maxdist = 0;

					if (surf != ctx->mod->meshinfo)
						surf[-1].nextsurf = surf;
					surf->nextsurf = NULL;
					surf++;
				}
			}
			firstvert += msurf->vertex_count;
		}
	}

	if (surf == ctx->mod->meshinfo)
		return false;

	return true;
}
void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}
static qboolean Mod_HL2_LoadVVD(hl2parsecontext_t *ctx, const void *buffer, size_t fsize)
{
	const hl2vvdheader_t *header = buffer;
	size_t lod;
	const hl2vvdvert_t *in;
	const vec4_t *it;
	if (fsize < sizeof(*header) || header->magic != (('I'<<0)|('D'<<8)|('S'<<16)|('V'<<24)) || header->version != 4 || header->revisionid != ctx->header->revisionid || header->lodverts_count[0] == 0)
		return false;

	{
		size_t v, numverts = ctx->numverts = header->lodverts_count[0];
		vec2_t *st = ctx->ofs_st_array		= plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(*st)*numverts);
		vecV_t *xyz = ctx->ofs_skel_xyz		= plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(*xyz)*numverts);
		vec3_t *norm = ctx->ofs_skel_norm	= plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(*norm)*numverts);
		vec3_t *sdir = ctx->ofs_skel_svect	= plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(*sdir)*numverts);
		vec3_t *tdir = ctx->ofs_skel_tvect	= plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(*tdir)*numverts);
		byte_vec4_t *bone = ctx->ofs_skel_idx = plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(*bone)*numverts);
		vec4_t *weight = ctx->ofs_skel_weight = plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(*weight)*numverts);

		in = (const void*)((const char*)buffer+header->verts_offset);
		it = (const void*)((const char*)buffer+header->tangents_offset);
		for(v = 0; v < numverts; v++, in++, it++)
		{
			Vector2Copy(in->st, st[v]);
			VectorCopy(in->xyz, xyz[v]);
			VectorCopy(in->norm, norm[v]);

			VectorCopy(in->bone, bone[v]);
			bone[v][3] = bone[v][2];	//make sure its valid, and in cache.
			VectorCopy(in->weight, weight[v]);
			weight[v][3] = 0;			//missing influences cannot influence.

			//tangents are compacted, and for some reason in a different part of the file.
			VectorCopy((*it), sdir[v]);
			CrossProduct(in->norm, (*it), tdir[v]);
			VectorScale(tdir[v], (*it)[3], tdir[v]);
		}
	}

	if (header->fixups_count)
	{
		size_t fixups = header->fixups_count, f, v;
		const hl2vvdfixup_t *fixup = (const hl2vvdfixup_t*)((const qbyte*)header+header->fixups_offset);
		for (lod = 0; lod < countof(ctx->lod) && lod < header->lod_count; lod++)
		{
			size_t numverts;
			for (numverts=0, f = 0; f < fixups; f++)
			{
				if (fixup[f].lod >= lod)
					numverts += fixup[f].numverts;
			}
			if (numverts != header->lodverts_count[lod])
				continue;
			ctx->lod[lod].numfixups = numverts;
			ctx->lod[lod].fixup = plugfuncs->GMalloc(&ctx->mod->memgroup, sizeof(index_t)*numverts);
			for (numverts=0, f = 0; f < fixups; f++)
			{
				if (fixup[f].lod >= lod)
					for (v = 0; v < fixup[f].numverts; v++)
						ctx->lod[lod].fixup[numverts++] = fixup[f].sourcevert+v;
			}
		}
	}
	return true;
}

static void Angle2Quaternion(const vec3_t angles, vec4_t quaternion)
{
	float	yaw = angles[2] * 0.5;
	float	pitch = angles[1] * 0.5;
	float	roll = angles[0] * 0.5;
	float	siny = sin(yaw);
	float	cosy = cos(yaw);
	float	sinp = sin(pitch);
	float	cosp = cos(pitch);
	float	sinr = sin(roll);
	float	cosr = cos(roll);

	quaternion[0] = sinr * cosp * cosy - cosr * sinp * siny;
	quaternion[1] = cosr * sinp * cosy + sinr * cosp * siny;
	quaternion[2] = cosr * cosp * siny - sinr * sinp * cosy;
	quaternion[3] = cosr * cosp * cosy + sinr * sinp * siny;
}
static signed short Mod_HL2_ReadAnimValue(const void *baseptr, signed short offset, int posenum)
{
	const hlmdl_animvalue_t	*animvalue = (const hlmdl_animvalue_t *) ((const qbyte *) baseptr + offset);
	if (!offset)
		return 0;	//nope, axis not present.
	/* find values including the required frame */
	while(animvalue->num.total <= posenum)
	{
		posenum -= animvalue->num.total;
		animvalue += animvalue->num.valid + 1;
	}
	if (posenum >= animvalue->num.valid)
		posenum = animvalue->num.valid;
	else
		posenum += 1;
	return animvalue[posenum].value;
}
static const void *Mod_HL2_GetExternalBlock(hl2parsecontext_t *ctx, const void *base, int blockidx, int offset)
{
	if (!blockidx)
	{	//data is relative to the parent structure...
		return (const qbyte*)base + offset;
	}
	else
	{	//data is elsewhere...
		const struct {
			int start;
			int end;
		} *block = (const void*)((const qbyte*)ctx->header + ctx->header->ofs_aniblocks);
		block += blockidx;

		if (!ctx->ani)
		{
			vfsfile_t *f = filefuncs->OpenVFS((const char *)ctx->header + ctx->header->ofs_aniname, "rb", FS_GAME);
			if (f)
			{
				size_t sz = f->GetLen(f);
				ctx->ani = plugfuncs->GMalloc(&ctx->mod->memgroup, sz);
				f->ReadBytes(f, ctx->ani, sz);
				f->Close(f);
			}
		}
		if (ctx->ani)
			return (const qbyte*)ctx->ani + block->start + offset;
	}
	return NULL;
}

static float HalfToFloat(unsigned short val)
{	//hl2 supported shitty low-ram consoles. yay for data compression?
	union
	{
		float f;
		unsigned int u;
	} u;
	if (val&0x7c00)
		u.u = (((val&0x7c00)>>10)-15+127)<<23;	//read exponent, rebias it, and reshift.
	else
		u.u = 0;	//denormal (or 0).
	u.u |= ((val & 0x3ff)<<13);//shift up the mantissa, but don't fold
	u.u |= (val&0x8000)<<16;	//retain the sign bit.
	return u.f;
}
static void Mod_HL2_ReadPose(hl2parsecontext_t *ctx, const hl2mdlanim_t *anim, int posenum, matrix3x4 *out, unsigned int numbones, const hl2mdlbone_t *boneinfo)
{	//hl2 models seem to have both animations and sequences
	vec3_t org;
	vec4_t quat;
	static vec3_t scale={1,1,1};
	const unsigned short *data;
	const hl2mdlpose_t *pose;
	struct poseofs_s poseofs = anim->defpose;
	int sect = 0;
	if (anim->posespersection)
	{	//can be stored in groups, so we don't have to walk as many rle entries.
		sect = posenum/anim->posespersection;
		poseofs = ((const struct poseofs_s*)((const qbyte*)anim + anim->ofs_posesections))[sect];
		posenum -= sect * anim->posespersection;
	}

	memcpy(out, ctx->baserelpose, sizeof(*out)*numbones);
	pose = Mod_HL2_GetExternalBlock(ctx, anim, poseofs.externalblock, poseofs.ofs_pose);
	if (!pose || pose->bone==255)
		return;
	for (;;)
	{
		data = pose->data;

		if (pose->flags & 0x2)
		{	//static data shared between all frames in this cluster
			quat[0] = ((int)(data[0]       )-0x8000) / (float)0x8000;
			quat[1] = ((int)(data[1]       )-0x8000) / (float)0x8000;
			quat[2] = ((int)(data[2]&0x7fff)-0x4000) / (float)0x4000;
			quat[3] = 1 - DotProduct(quat, quat);
			quat[3] = sqrt(quat[3]);
			if (data[2]&0x8000)
				quat[3] *= -1;
			data+=3;
		}
		else if (pose->flags & 0x20)
		{	//apparently these are 21+21+21+1 bits each
			quint64_t q64 = *(const quint64_t*)data;
			quat[0] = (((int)(q64>> 0)&((1<<21)-1))-(1<<20)) / (float)(1<<20);
			quat[1] = (((int)(q64>>21)&((1<<21)-1))-(1<<20)) / (float)(1<<20);
			quat[2] = (((int)(q64>>42)&((1<<21)-1))-(1<<20)) / (float)(1<<20);
			quat[3] = 1 - DotProduct(quat, quat);
			quat[3] = sqrt(quat[3]);
			if (q64&(((quint64_t)1)<<63))
				quat[3] *= -1;
			data+=sizeof(q64)/sizeof(*data);
		}
		else if (pose->flags & 0x8)
		{	//animated version (using some sort of RLE)
			vec3_t ang;
			if (pose->flags & 0x10)
				VectorClear(ang);
			else
				VectorCopy(boneinfo[pose->bone].rot, ang);
			ang[0] += Mod_HL2_ReadAnimValue(data, data[0], posenum) * boneinfo[pose->bone].rotscale[0];
			ang[1] += Mod_HL2_ReadAnimValue(data, data[1], posenum) * boneinfo[pose->bone].rotscale[1];
			ang[2] += Mod_HL2_ReadAnimValue(data, data[2], posenum) * boneinfo[pose->bone].rotscale[2];
			data+=3;
			Angle2Quaternion(ang, quat);
		}
		else if (pose->flags & 0x10)
			Vector4Set(quat, 0, 0, 0, 1);
		else
			VectorCopy(boneinfo[pose->bone].quat, quat);

		if (pose->flags & 1)
		{	//static data
			org[0] = HalfToFloat(data[0]);
			org[1] = HalfToFloat(data[1]);
			org[2] = HalfToFloat(data[2]);
			data+=3;
		}
		else
		{
			if (pose->flags & 0x10)
				VectorClear(org);
			else
				VectorCopy(boneinfo[pose->bone].pos, org);
			if (pose->flags & 0x4)
			{	//animated version (using some sort of RLE)
				org[0] += Mod_HL2_ReadAnimValue(data, data[0], posenum) * boneinfo[pose->bone].posscale[0];
				org[1] += Mod_HL2_ReadAnimValue(data, data[1], posenum) * boneinfo[pose->bone].posscale[1];
				org[2] += Mod_HL2_ReadAnimValue(data, data[2], posenum) * boneinfo[pose->bone].posscale[2];
				data+=3;
			}
		}

		//FIXME: bone controllers

		modfuncs->GenMatrixPosQuat4Scale(org, quat, scale, (float*)(out + pose->bone));
		if (!pose->next)
			break;
		pose = (const hl2mdlpose_t*)((const qbyte*)pose + pose->next);
	}
}


// matches engine Mod_InsertEvent from parses a foo.mdl.events file and inserts the events into the relevant animations
static void Mod_InsertEvent(zonegroup_t *mem, galiasanimation_t *anims, unsigned int numanimations, unsigned int eventanimation, float eventpose, int eventcode, const char *eventdata)
{
	galiasevent_t *ev, **link;
	if (eventanimation >= numanimations)
	{
		Con_Printf("Mod_InsertEvent: invalid frame index\n");
		return;
	}
	ev = plugfuncs->GMalloc(mem, sizeof(*ev) + strlen(eventdata)+1);
	ev->data = (char*)(ev+1);

	ev->timestamp = eventpose;
	ev->timestamp /= anims[eventanimation].rate;
	ev->code = eventcode;
	strcpy(ev->data, eventdata);
	link = &anims[eventanimation].events;
	while (*link && (*link)->timestamp <= ev->timestamp)
		link = &(*link)->next;
	ev->next = *link;
	*link = ev;
}

// matches engine Mod_ParseModelEvents
static qboolean Mod_ParseModelEvents(model_t *mod, galiasanimation_t *anims, unsigned int numanimations)
{
	unsigned int anim;
	float pose;
	int eventcode;

	const char *modelname = mod->name;
	zonegroup_t *mem = &mod->memgroup;
	char fname[MAX_QPATH], tok[2048];
	size_t fsize;
	char *line, *file, *eol;
	Q_snprintfz(fname, sizeof(fname), "%s.events", modelname);
	line = file = filefuncs->LoadFile(fname, &fsize);
	if (!file)
		return false;
	while(line && *line)
	{
		eol = strchr(line, '\n');
		if (eol)
			*eol = 0;

		line = cmdfuncs->ParseToken(line, tok, sizeof(tok), 0);
		anim = strtoul(tok, NULL, 0);
		line = cmdfuncs->ParseToken(line, tok, sizeof(tok), 0);
		pose = strtod(tok, NULL);
		line = cmdfuncs->ParseToken(line, tok, sizeof(tok), 0);
		eventcode = (long)strtol(tok, NULL, 0);
		line = cmdfuncs->ParseToken(line, tok, sizeof(tok), 0);
		Mod_InsertEvent(mem, anims, numanimations, anim, pose, eventcode, tok);

		if (eol)
			line = eol+1;
		else
			break;
	}
	plugfuncs->Free(file);
	return true;
}


qboolean QDECL Mod_LoadHL2Model (model_t *mod, void *buffer, size_t fsize)
{
	hl2parsecontext_t ctx = {mod, buffer};
	void *vtx = NULL, *vvd = NULL;
	size_t vtxsize = 0, vvdsize = 0;
	char base[MAX_QPATH];
	qboolean result;
	size_t i, p;
	const char *vtxpostfixes[] = {
		".dx90.vtx",
		".dx80.vtx",
		".vtx",
		".sw.vtx"
	};
	const char *vvdpostfixes[] = {
		".vvd"
	};
	galiasinfo_t *galias;

	for (i = 0; !vtx && i < countof(vtxpostfixes); i++)
	{
		modfuncs->StripExtension(mod->name, base, sizeof(base));
		Q_strncatz(base, vtxpostfixes[i], sizeof(base));
		vtx = filefuncs->LoadFile(base, &vtxsize);
	}
	for (i = 0; !vvd && i < countof(vvdpostfixes); i++)
	{
		modfuncs->StripExtension(mod->name, base, sizeof(base));
		Q_strncatz(base, vvdpostfixes[i], sizeof(base));
		vvd = filefuncs->LoadFile(base, &vvdsize);
	}

	if (ctx.header->num_bones)
	{
		const hl2mdlbone_t *in = (const hl2mdlbone_t*)((const qbyte*)ctx.header + ctx.header->ofs_bones);
		vec3_t scale = {1,1,1};
		ctx.basepose = plugfuncs->GMalloc(&mod->memgroup, sizeof(float)*12*ctx.header->num_bones);
		ctx.baserelpose = alloca(sizeof(float)*12*ctx.header->num_bones);
		ctx.bones = plugfuncs->GMalloc(&mod->memgroup, sizeof(*ctx.bones)*ctx.header->num_bones);

		for (i = 0; i < ctx.header->num_bones; i++)
		{
			float *pose = ctx.baserelpose + 12*i;
			Q_strlcpy(ctx.bones[i].name, (const char*)&in[i]+in[i].name_ofs, sizeof(ctx.bones[i].name));
			ctx.bones[i].parent = in[i].parent;
			modfuncs->GenMatrixPosQuat4Scale(in[i].pos, in[i].quat, scale, pose);
			if (in[i].parent>=0)
				modfuncs->ConcatTransforms((const void*)(ctx.basepose+in[i].parent*12), (const void*)pose, (void*)(ctx.basepose+i*12));
			else
				memcpy(ctx.basepose+12*i, pose, sizeof(float)*12);
			memcpy(ctx.bones[i].inverse, in[i].inverse, sizeof(float)*12);	//use the provided value, but should match modfuncs->M3x4_Invert(ctx.basepose+12*i, ctx.bones[i].inverse);
		}

		if (ctx.header->num_anims)
		{
			galiasanimation_t *a;
			const hl2mdlanim_t *in = (const hl2mdlanim_t*)((const qbyte*)ctx.header + ctx.header->ofs_anims);
			a = ctx.ofs_animations = plugfuncs->GMalloc(&mod->memgroup, sizeof(*ctx.ofs_animations)*ctx.header->num_anims);
			//note that hl2 models have anims and sequences... I'm guessing I ought to be exposing sequences instead of anims.
			for (i = 0; i < ctx.header->num_anims; i++, in++)
			{
				a->skeltype = SKEL_RELATIVE;
				a->numposes = in->numframes;

				a->GetRawBones = NULL;	//FIXME: for delay loading the proper way...
										//FIXME: replace Alias_FindRawSkelData instead, to handle bone controllers etc.
				a->boneofs = plugfuncs->GMalloc(&mod->memgroup, sizeof(float)*a->numposes*12*ctx.header->num_bones);
				a->loop = in->loop;
				a->rate = in->fps;
				a->action = -1;
				a->actionweight = 0;
				a->events = NULL;
				Q_strlcpy(a->name, (const char*)in + in->name_ofs, sizeof(a->name));

				for (p = 0; p < a->numposes; p++)
					Mod_HL2_ReadPose(&ctx, in, p, (matrix3x4*)a->boneofs+p*ctx.header->num_bones, ctx.header->num_bones, (const hl2mdlbone_t*)((const qbyte*)ctx.header + ctx.header->ofs_bones));
				a++;
			}
			ctx.num_animations = a-ctx.ofs_animations;
		}
	}


	result  = Mod_HL2_LoadVVD(&ctx, vvd, vvdsize);
	result &= Mod_HL2_LoadVTX(&ctx, vtx, vtxsize);
	plugfuncs->Free(vvd);
	plugfuncs->Free(vtx);

	if (ctx.header->num_bones && !mod->meshinfo)
	{	//it has a rig... make sure we spit out a surface so we can read the bones.
		galiasinfo_t *surf = mod->meshinfo = plugfuncs->GMalloc(&mod->memgroup, sizeof(galiasinfo_t));
		surf->numbones = ctx.header->num_bones;
		surf->baseframeofs = ctx.basepose;
		surf->ofsbones = ctx.bones;

		surf->numanimations = ctx.num_animations;
		surf->ofsanimations = ctx.ofs_animations;
		result = true;
	}

	VectorCopy(ctx.header->mins, mod->mins);
	VectorCopy(ctx.header->maxs, mod->maxs);
	galias = (galiasinfo_t*)mod->meshinfo;
	Mod_ParseModelEvents(mod, galias->ofsanimations, galias->numanimations);

	mod->type = mod_alias;
	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
	modfuncs->BIH_BuildAlias(mod, mod->meshinfo);
	return result;
}

qboolean MDL_Init(void)
{
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	cmdfuncs = plugfuncs->GetEngineInterface(plugcmdfuncs_name, sizeof(*cmdfuncs));
	fsfuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*fsfuncs));

	if (modfuncs && modfuncs->version != MODPLUGFUNCS_VERSION)
		modfuncs = NULL;

	if (modfuncs && filefuncs)
	{
		modfuncs->RegisterModelFormatMagic("Source model (v44)", "IDST\x2c\0\0\0",8, Mod_LoadHL2Model);
		modfuncs->RegisterModelFormatMagic("Source model (v45)", "IDST\x2d\0\0\0",8, Mod_LoadHL2Model);
		modfuncs->RegisterModelFormatMagic("Source model (v46)", "IDST\x2e\0\0\0",8, Mod_LoadHL2Model);
		modfuncs->RegisterModelFormatMagic("Source model (v47)", "IDST\x2f\0\0\0",8, Mod_LoadHL2Model);
		modfuncs->RegisterModelFormatMagic("Source model (v48)", "IDST\x30\0\0\0",8, Mod_LoadHL2Model);
		modfuncs->RegisterModelFormatMagic("Source model (v49)", "IDST\x31\0\0\0",8, Mod_LoadHL2Model);
		return true;
	}
	return false;
}
