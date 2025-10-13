#ifndef GLQUAKE
#define GLQUAKE	//this is shit.
#endif
#include "quakedef.h"
#include "../plugin.h"
#include "com_mesh.h"

#ifdef SKELETALMODELS

extern plugmodfuncs_t *modfuncs;
extern plugfsfuncs_t *filefuncs;

#define IQM_MAGIC "INTERQUAKEMODEL"
#define IQM_VERSION2 2

struct iqmheader
{
	char magic[16];
	unsigned int version;
	unsigned int filesize;
	unsigned int flags;
	unsigned int num_text, ofs_text;
	unsigned int num_meshes, ofs_meshes;
	unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
	unsigned int num_triangles, ofs_triangles, ofs_adjacency;
	unsigned int num_joints, ofs_joints;
	unsigned int num_poses, ofs_poses;
	unsigned int num_anims, ofs_anims;
	unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
	unsigned int num_comment, ofs_comment;
	unsigned int num_extensions, ofs_extensions;
};
struct iqmmesh
{
	unsigned int name;
	unsigned int material;
	unsigned int first_vertex, num_vertexes;
	unsigned int first_triangle, num_triangles;
};

enum
{
	IQM_POSITION     = 0,
	IQM_TEXCOORD     = 1,
	IQM_NORMAL       = 2,
	IQM_TANGENT      = 3,
	IQM_BLENDINDEXES = 4,
	IQM_BLENDWEIGHTS = 5,
	IQM_COLOR        = 6,
	IQM_CUSTOM       = 0x10
};

enum
{
	IQM_BYTE   = 0,
	IQM_UBYTE  = 1,
	IQM_SHORT  = 2,
	IQM_USHORT = 3,
	IQM_INT    = 4,
	IQM_UINT   = 5,
	IQM_HALF   = 6,
	IQM_FLOAT  = 7,
	IQM_DOUBLE = 8,
};

struct iqmtriangle
{
	unsigned int vertex[3];
};

struct iqmjoint2
{
	unsigned int name;
	int parent;
	float translate[3], rotate[4], scale[3];
};

struct iqmpose2
{
	int parent;
	unsigned int mask;
	float channeloffset[10];
	float channelscale[10];
};

struct iqmanim
{
	unsigned int name;
	unsigned int first_frame, num_frames;
	float framerate;
	unsigned int flags;
};

enum
{
	IQM_LOOP = 1<<0
};

struct iqmvertexarray
{
	unsigned int type;
	unsigned int flags;
	unsigned int format;
	unsigned int size;
	unsigned int offset;
};

struct iqmbounds
{
	float bbmin[3], bbmax[3];
	float xyradius, radius;
};

struct iqmextension
{
	unsigned int name;
	unsigned int num_data, ofs_data;
	unsigned int ofs_extensions; // pointer to next extension. wtf is up with this? how is this not redundant due to ofs_data?
};


struct iqmext_fte_mesh
{
	unsigned int contents;		//default CONTENTS_BODY
	unsigned int surfaceflags;	//propagates to trace_surfaceflags
	unsigned int surfaceid;		//the body reported to qc via trace_surface
	unsigned int geomset;
	unsigned int geomid;
	float	mindist;
	float	maxdist;
};
struct iqmext_fte_event
{
	unsigned int anim;
	float timestamp;
	unsigned int evcode;
	unsigned int evdata_str;	//stringtable
};
static void CrossProduct_ (const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}
static void Bone_To_PosQuat4(const float *matrix, float *pos, float *quat4, float *scale)
{	//I originally ripped this function out of DP. tweaked slightly.
	//http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
	float origininvscale = 1;
	float origin[3];
	float quat[4];
	float quatscale;

	float trace = matrix[0*4+0] + matrix[1*4+1] + matrix[2*4+2];

	origin[0] = matrix[0*4+0];
	origin[1] = matrix[1*4+0];
	origin[2] = matrix[2*4+0];
	scale [0] = sqrt(DotProduct(origin,origin));
	origin[0] = matrix[0*4+1];
	origin[1] = matrix[1*4+1];
	origin[2] = matrix[2*4+1];
	scale [1] = sqrt(DotProduct(origin,origin));
	origin[1] = matrix[0*4+2];
	origin[1] = matrix[1*4+2];
	origin[2] = matrix[2*4+2];
	scale [2] = sqrt(DotProduct(origin,origin));

	origin[0] = matrix[0*4+3];
	origin[1] = matrix[1*4+3];
	origin[2] = matrix[2*4+3];
	if(trace > 0)
	{
		float r = sqrt(1.0f + trace), inv = 0.5f / r;
		quat[0] = (matrix[2*4+1] - matrix[1*4+2]) * inv;
		quat[1] = (matrix[0*4+2] - matrix[2*4+0]) * inv;
		quat[2] = (matrix[1*4+0] - matrix[0*4+1]) * inv;
		quat[3] = 0.5f * r;
	}
	else if(matrix[0*4+0] > matrix[1*4+1] && matrix[0*4+0] > matrix[2*4+2])
	{
		float r = sqrt(1.0f + matrix[0*4+0] - matrix[1*4+1] - matrix[2*4+2]), inv = 0.5f / r;
		quat[0] = 0.5f * r;
		quat[1] = (matrix[1*4+0] + matrix[0*4+1]) * inv;
		quat[2] = (matrix[0*4+2] + matrix[2*4+0]) * inv;
		quat[3] = (matrix[2*4+1] - matrix[1*4+2]) * inv;
	}
	else if(matrix[1*4+1] > matrix[2*4+2])
	{
		float r = sqrt(1.0f + matrix[1*4+1] - matrix[0*4+0] - matrix[2*4+2]), inv = 0.5f / r;
		quat[0] = (matrix[1*4+0] + matrix[0*4+1]) * inv;
		quat[1] = 0.5f * r;
		quat[2] = (matrix[2*4+1] + matrix[1*4+2]) * inv;
		quat[3] = (matrix[0*4+2] - matrix[2*4+0]) * inv;
	}
	else
	{
		float r = sqrt(1.0f + matrix[2*4+2] - matrix[0*4+0] - matrix[1*4+1]), inv = 0.5f / r;
		quat[0] = (matrix[0*4+2] + matrix[2*4+0]) * inv;
		quat[1] = (matrix[2*4+1] + matrix[1*4+2]) * inv;
		quat[2] = 0.5f * r;
		quat[3] = (matrix[1*4+0] - matrix[0*4+1]) * inv;
	}
	// normalize quaternion so that it is unit length
	quatscale = quat[0]*quat[0]+quat[1]*quat[1]+quat[2]*quat[2]+quat[3]*quat[3];
	if (quatscale)
		quatscale = (quat[3] >= 0 ? -1.0f : 1.0f) / sqrt(quatscale);

	// use a negative scale on the quat because the above function produces a
	// positive quat[3] and canonical quaternions have negative quat[3]
	VectorScale(origin, origininvscale, pos);
	Vector4Scale(quat, quatscale, quat4);
}
void Mod_ExportIQM(char *fname, int flags, galiasinfo_t *mesh)
{
	int i, j, k;
	vfsfile_t *f;
	galiasinfo_t *m;
	qbyte *data = NULL;
	char *otext;
	struct iqmvertexarray *ovarr;
	struct iqmtriangle *otri;
	struct iqmmesh *omesh;
	struct iqmjoint2 *ojoint = NULL;
	struct iqmanim *oanim = NULL;
	struct iqmpose2 *opose = NULL;
	struct
	{
		float min[10], max[10], scale[10];
		int flags;
	} *poseinfo = NULL, *pi;	//per bone
	struct
	{	//pos3, quat4, scale3
		float posquatscale[10];	//raw values, used to calibrate ranges
	} *posedata = NULL, *pd;	//per bone*joint
	vecV_t *ivert;
	vec2_t *ist;
	vec3_t *overt;
	vec3_t *onorm = NULL;
	vec4_t *otang = NULL;
	vec2_t *ost;
	bone_vec4_t *oboneidx = NULL;
	byte_vec4_t *oboneweight = NULL;
	unsigned short *oposedata = NULL;
	struct iqmheader hdr = {IQM_MAGIC, IQM_VERSION2}, *oh;
	hdr.flags = flags;
	hdr.num_vertexarrays = 4;
	hdr.num_triangles = 0;
	hdr.ofs_adjacency = 0;	//noone actually uses this...
	hdr.num_poses = 0;
//	hdr.ofs_poses = 0;
	hdr.num_anims = 0;
//	hdr.ofs_anims = 0;
	hdr.num_frames = 0;
	hdr.num_framechannels = 0;
//	hdr.ofs_frames = 0;
//	hdr.ofs_bounds = 0;
	hdr.num_comment = 0;
//	hdr.ofs_comment = 0;
	hdr.num_extensions = 0;
//	hdr.ofs_extensions = 0;


	hdr.num_joints = mesh->numbones;
	if (hdr.num_joints)
	{
		float *matrix;
		hdr.num_vertexarrays+= 2;
		hdr.num_anims = mesh->numanimations;
		for (i = 0; i < hdr.num_anims; i++)
		{
			hdr.num_text += strlen(mesh->ofsanimations[i].name)+1;
			hdr.num_frames += mesh->ofsanimations[i].numposes;
		}
		if (hdr.num_frames)
		{
			poseinfo = malloc(sizeof(*poseinfo)*hdr.num_joints);
			hdr.num_poses = hdr.num_joints;
			posedata = malloc(sizeof(*posedata)*hdr.num_joints*hdr.num_poses);
			//pull out the raw data and convert to the quats that we need
			for (i = 0, pd = posedata; i < hdr.num_anims; i++)
				for (j = 0, matrix = mesh->ofsanimations[i].boneofs; j < mesh->ofsanimations[i].numposes; j++)
					for (k = 0; k < hdr.num_joints; k++)
					{
						Bone_To_PosQuat4(matrix, &pd->posquatscale[0], &pd->posquatscale[3], &pd->posquatscale[7]);
						pd++;
						matrix+=12;
					}
			//now figure out each poseinfo's min+max
			for (i = 0, pi = poseinfo; i < hdr.num_joints; i++, pi++)
				for (j = 0, pd = posedata+i; j < hdr.num_poses; j++, pd+=hdr.num_joints)
					for (k = 0; k < 10; k++)
					{
						if (!i || pd->posquatscale[k] < pi->min[k])
							pi->min[k] = pd->posquatscale[k];
						if (!i || pi[i].max[k] < pd->posquatscale[k])
							pi->max[k] = pd->posquatscale[k];
					}
			//figure out the offset+range+flags
			for (i = 0, pi = poseinfo; i < hdr.num_joints; i++, pi++)
				for (k = 0; k < 10; k++)
				{
					pi->scale[k] = pi->max[k]-pi->min[k];
					if (pi->scale[k] < 1e-10f)
						;	//total range is tiny and won't make any real difference, drop this channel for a small saving.
					else
					{
						pi->scale[k] /= 0xffffu;	//compensate for the datatype's max
						pi->flags |= 1u<<k;
						hdr.num_framechannels++;
					}
				}
			hdr.num_framechannels *= hdr.num_frames; //there'll be one for each pose*channel*frame
		}
	}
	hdr.num_text += hdr.num_joints*32; //gah

	//count needed data
	for (m = mesh; m; m = m->nextsurf)
	{
		//can't handle the surface if its verts are weird.
		if (m->shares_verts && m->shares_verts != hdr.num_meshes)
			continue;
		//can only handle one set of bones.
		if (m->shares_bones != 0)
			continue;
		//and must have the same number of bones.
		if (hdr.num_joints != m->numbones)
			continue;

		hdr.num_text += strlen(m->surfacename)+1;
		if (m->ofsskins && m->ofsskins->frame)
			hdr.num_text += strlen(m->ofsskins->frame->shadername)+1;
		hdr.num_triangles += m->numindexes/3;
		hdr.num_vertexes += m->numverts;
		hdr.num_meshes++;
	}

	//allocate our output buffer
#define ALLOCSPACE	hdr.filesize = 0; \
						ALLOC(oh, sizeof(*oh));	\
						ALLOC(otext, sizeof(*otext)*hdr.num_text);	\
						ALLOC(ovarr, sizeof(*ovarr)*hdr.num_vertexarrays); \
						ALLOC(otri, sizeof(*otri)*hdr.num_triangles); \
						ALLOC(overt, sizeof(*overt)*hdr.num_vertexes); \
						ALLOC(ost, sizeof(*ost)*hdr.num_vertexes);	\
						if (mesh->ofs_skel_norm) {ALLOC(onorm, sizeof(*onorm)*hdr.num_vertexes);} \
						if (mesh->ofs_skel_svect && mesh->ofs_skel_tvect) {ALLOC(otang, sizeof(*otang)*hdr.num_vertexes);} \
						if (hdr.num_joints) {ALLOC(oboneweight, sizeof(*oboneweight)*hdr.num_vertexes);} \
						if (hdr.num_joints) {ALLOC(oboneidx, sizeof(*oboneidx)*hdr.num_vertexes);} \
						if (hdr.num_joints) {ALLOC(ojoint, sizeof(*ojoint)*hdr.num_joints);} \
						if (hdr.num_anims) {ALLOC(oanim, sizeof(*oanim)*hdr.num_anims);} \
						if (hdr.num_poses) {ALLOC(opose, sizeof(*opose)*hdr.num_poses);} \
						if (hdr.num_framechannels) {ALLOC(oposedata, sizeof(*oposedata)*hdr.num_framechannels);} \
						ALLOC(omesh, sizeof(*omesh)*hdr.num_meshes);
#define ALLOC(p,s) p=(void*)(data+hdr.filesize);hdr.filesize+=s;
	ALLOCSPACE;	//figure out how much space we need
	data = malloc(hdr.filesize);
	memset(data, 0xFE, hdr.filesize);
	ALLOCSPACE;	//and assign everything to the right offsets.
#undef ALLOC
#undef ALLOCSPACE

	//copy over the preliminary header
	*oh = hdr;

#define hdr hdr

	if (omesh) oh->ofs_meshes = (qbyte*)omesh-data;
	if (otext) oh->ofs_text = (qbyte*)otext-data;
	if (ovarr) oh->ofs_vertexarrays = (qbyte*)ovarr-data;
	if (otri) oh->ofs_triangles = (qbyte*)otri-data;
	if (ojoint) oh->ofs_joints = (qbyte*)ojoint-data;
	if (opose) oh->ofs_poses = (qbyte*)opose-data;
	if (oposedata) oh->ofs_frames = (qbyte*)oposedata-data;
	if (oanim) oh->ofs_anims = (qbyte*)oanim-data;

	//set up vertex array data. we might add some padding here, in case the extra data isn't availble.
	memset(ovarr, 0, sizeof(*ovarr)*oh->num_vertexarrays);
	oh->num_vertexarrays=0;
	ovarr->type = IQM_POSITION;
	ovarr->flags = 0;
	ovarr->format = IQM_FLOAT;
	ovarr->size = 3;
	ovarr->offset = (qbyte*)overt - data;
	ovarr++;
	oh->num_vertexarrays++;

	ovarr->type = IQM_TEXCOORD;
	ovarr->flags = 0;
	ovarr->format = IQM_FLOAT;
	ovarr->size = 2;
	ovarr->offset = (qbyte*)ost - data;
	ovarr++;
	oh->num_vertexarrays++;

	if (onorm)
	{
		ovarr->type = IQM_NORMAL;
		ovarr->flags = 0;
		ovarr->format = IQM_FLOAT;
		ovarr->size = 3;
		ovarr->offset = (qbyte*)onorm - data;
		ovarr++;
		oh->num_vertexarrays++;
	}
	if (otang)
	{
		ovarr->type = IQM_TANGENT;
		ovarr->flags = 0;
		ovarr->format = IQM_FLOAT;
		ovarr->size = 4;
		ovarr->offset = (qbyte*)otang - data;
		ovarr++;
		oh->num_vertexarrays++;
	}
	if (oboneidx)
	{
		ovarr->type = IQM_BLENDINDEXES;
		ovarr->flags = 0;
		ovarr->format = (MAX_BONES>65536)?IQM_UINT:((MAX_BONES>256)?IQM_USHORT:IQM_UBYTE);
		ovarr->size = 4;
		ovarr->offset = (qbyte*)oboneidx - data;
		ovarr++;
		oh->num_vertexarrays++;
	}
	if (oboneweight)
	{
		ovarr->type = IQM_BLENDWEIGHTS;
		ovarr->flags = 0;
		ovarr->format = IQM_BYTE;
		ovarr->size = 4;
		ovarr->offset = (qbyte*)oboneweight - data;
		ovarr++;
		oh->num_vertexarrays++;
	}
	/*if (orgba)
	{
		ovarr->type = IQM_COLOR;
		ovarr->flags = 0;
		ovarr->format = IQM_FLOAT;
		ovarr->size = 4;
		ovarr->offset = (qbyte*)orgba - data;
		ovarr++;
		oh->num_vertexarrays++;
	}*/

	if (ojoint)
	{
		for (i = 0; i < hdr.num_joints; i++)
		{
			ojoint[i].parent = mesh->ofsbones[i].parent;
			ojoint[i].name = (qbyte*)otext-(data+oh->ofs_text);
			strcpy(otext, mesh->ofsbones[i].name);
			otext += strlen(otext)+1;

			Bone_To_PosQuat4(mesh->ofsbones[i].inverse, ojoint[i].translate, ojoint[i].rotate, ojoint[i].scale);
		}
	}
	if (opose)
	{
		int c;
		for (i = 0, pi=poseinfo; i < hdr.num_joints; i++, pi++)
		{
			opose[i].parent = mesh->ofsbones[i].parent;
			opose[i].mask = pi->flags;
			for (k = 0; k < 10; k++)
			{
				opose[i].channeloffset[k] = pi->min[k];
				opose[i].channelscale[k] = pi->scale[k];
			}

			for (j = 0, pd = posedata+i; j < hdr.num_frames; j++, pd+=hdr.num_joints)
			{
				for (k = 0; k < 10; k++)
				{
					if (opose[i].mask & (1<<k))
					{
						c = (pd->posquatscale[k]-pi->min[k])/pi->scale[k];
						c = bound(0, c, 0xffff);	//clamp it just in case (floats can be annoying)
						*oposedata++ = c;
					}
				}
			}
		}
	}
	if (oposedata)
	{
		for (i = 0; i < hdr.num_joints; i++)
		{
			opose[i].parent = mesh->ofsbones[i].parent;
			opose[i].mask = poseinfo[i].flags;
			for (k = 0; k < 10; k++)
			{
				opose[i].channeloffset[k] = poseinfo[i].min[k];
				opose[i].channelscale[k] = poseinfo[i].scale[k];
			}
		}
	}

	hdr.num_frames = 0;
	for (i = 0; i < hdr.num_anims; i++, oanim++)
	{
		oanim->first_frame = hdr.num_frames;
		oanim->num_frames = mesh->ofsanimations[i].numposes;
		oanim->framerate = mesh->ofsanimations[i].rate;
		oanim->flags = mesh->ofsanimations[i].loop?IQM_LOOP:0;
		oanim->name = (qbyte*)otext-(data+oh->ofs_text);
		strcpy(otext, mesh->ofsanimations[i].name);
		otext += strlen(otext)+1;
		hdr.num_frames += mesh->ofsanimations[i].numposes;
	}
	oh->num_anims = i;
	oh->num_frames = hdr.num_frames;

	//count needed data
	hdr.num_triangles = 0;
	hdr.num_vertexes = 0;
	for (m = mesh; m; m = m->nextsurf)
	{
		//can't handle the surface if its verts are weird.
		if (m->shares_verts && m->shares_verts != hdr.num_meshes)
			continue;
		//can only handle one set of bones.
		if (m->shares_bones != 0)
			continue;
		if (hdr.num_joints != m->numbones)
			continue;

		omesh->name = (qbyte*)otext-(data+oh->ofs_text);
		strcpy(otext, m->surfacename);
		otext += strlen(otext)+1;

		omesh->material = (qbyte*)otext-(data+oh->ofs_text);
		if (m->ofsskins && m->ofsskins->frame)
			strcpy(otext, m->ofsskins->frame->shadername);
		else
			strcpy(otext, "");
		otext += strlen(otext)+1;

		omesh->first_vertex = hdr.num_vertexes;
		omesh->num_vertexes = m->numverts;
		omesh->first_triangle = hdr.num_triangles;
		omesh->num_triangles = m->numindexes/3;

		if (m->ofs_skel_xyz)
			ivert = m->ofs_skel_xyz;
#ifdef NONSKELETALMODELS
		else if (m->numanimations && m->ofsanimations->numposes)
			ivert = m->ofsanimations->poseofs->ofsverts;
#endif
		else
			ivert = NULL;
		if (ivert)
		{
			for (i = 0; i < omesh->num_vertexes; i++)
				VectorCopy (ivert[i], overt[i]);
		}
		else
		{
			for (i = 0; i < omesh->num_vertexes; i++)
				VectorClear (overt[i]);

		}
		overt += i;

		if (oboneidx)
		{
			bone_vec4_t *iidx = m->ofs_skel_idx;
			vec4_t *iweight = m->ofs_skel_weight;
			for (i = 0; i < omesh->num_vertexes; i++)
			{
				Vector4Copy(iidx[i], oboneidx[i]);
				Vector4Scale(iweight[i], 255, oboneweight[i]);
			}
			oboneidx += i;
			oboneweight += i;
		}

		if (ost)
		{
			ist = m->ofs_st_array;
			for (i = 0; i < omesh->num_vertexes; i++)
				Vector2Copy(ist[i], ost[i]);
			ost += i;
		}

		if (onorm)
		{
			vec3_t *inorm, *isdir, *itdir, t;
			if (m->ofs_skel_norm)
			{
				inorm = m->ofs_skel_norm;
				isdir = m->ofs_skel_svect;
				itdir = m->ofs_skel_tvect;
			}
#ifdef NONSKELETALMODELS
			else if (m->numanimations && m->ofsanimations->numposes)
			{
				inorm = m->ofsanimations->poseofs->ofsnormals;
				isdir = m->ofsanimations->poseofs->ofssvector;
				itdir = m->ofsanimations->poseofs->ofstvector;
			}
#endif
			else
			{
				inorm = NULL;
				isdir = NULL;
				itdir = NULL;
			}
			if (otang)
			{
				if (inorm && isdir && itdir)
				{
					for (i = 0; i < omesh->num_vertexes; i++)
					{
						VectorCopy (isdir[i], otang[i]);
						CrossProduct_(isdir[i], inorm[i], t);
						otang[i][3] = DotProduct(itdir[i], t)<0;	//fourth part is simply a flag that says which direction the bitangent is in, should otherwise be a nice crossproduct result.
					}
				}
				else
				{
					for (i = 0; i < omesh->num_vertexes; i++)
					{
						VectorClear (otang[i]);
						otang[i][3] = 0;
					}
				}
				otang += i;
			}
			if (inorm)
			{
				for (i = 0; i < omesh->num_vertexes; i++)
					VectorCopy (ivert[i], onorm[i]);
			}
			else
			{
				for (i = 0; i < omesh->num_vertexes; i++)
					VectorClear (onorm[i]);
			}
			otang += i;
			onorm += i;
		}

		for (i = 0; i < omesh->num_triangles; i++)
		{
			otri[i].vertex[0] = m->ofs_indexes[i*3+0]+hdr.num_vertexes;
			otri[i].vertex[1] = m->ofs_indexes[i*3+1]+hdr.num_vertexes;
			otri[i].vertex[2] = m->ofs_indexes[i*3+2]+hdr.num_vertexes;
		}
		otri += i;

		hdr.num_vertexes += omesh->num_vertexes;
		hdr.num_triangles += omesh->num_triangles;
	}

	//and write it out
	f = filefuncs->OpenVFS(fname, "wb", FS_GAMEONLY);
	if (f)
	{
		VFS_WRITE(f, oh, oh->filesize);
		VFS_CLOSE(f);
	}
	free(data);
	free(poseinfo);
#undef hdr
}
#endif
