/*
Copyright (C) 2011 Id Software, Inc.

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
/*
this file deals with qc builtins to apply custom skeletal blending (skeletal objects extension), as well as the logic required to perform realtime ragdoll, if I ever implement that.
*/

/*
skeletal objects are just a set of bone poses.
they are separate from the entity they are attached to, and must be created/destroyed separately.
typically the bones are all stored relative to their parent.
qc must use skel_build to copy animation data from the file format into the skeletal object for rendering, but can build different bones using different animations or can override explicit bones.

ragdoll file is a description of the joints in the ragdoll.
a skeletal object, built from a doll instead of a model, has a series of physics objects created at key points (ie: not face).
these objects are absolute
qc must build the skeletal object still, which fills the skeletal object from the physics objects instead of animation data, for bones that have solid objects.
*/

#include "quakedef.h"

#if defined(RAGDOLL) || defined(SKELETALOBJECTS)

#include "pr_common.h"
#include "com_mesh.h"

#define MAX_SKEL_OBJECTS 1024

#ifdef RAGDOLL
/*this is the description of the ragdoll, it is how the doll flops around*/
typedef struct doll_s
{
	char *name;
	int uses;
	model_t *model;
	struct doll_s *next;

	qboolean drawn:1;
	int refanim; //-1 for skining pose. otherwise the first pose of the specified anim. probaly 0.
	float refanimtime; //usually just 0
	int numdefaultanimated;
	int numbodies;
	int numjoints;
	int numbones;
	rbebodyinfo_t *body;
	rbejointinfo_t *joint;
	struct
	{
		//easy lookup table for bone->body.
		//most of these will be -1, which means 'import from animation object'
		int bodyidx;
	} *bone;
} doll_t;

typedef struct
{
	rbebody_t odebody;

//	int ownerent;	/*multiple of 12*/
//	int flags;

//	float moment[12];
	float animstrength;
	float animmatrix[12];
} body_t;
#endif

/*this is the skeletal object*/
typedef struct skelobject_s
{
	int inuse;

	int modelindex; //for vid_restarts
	model_t *model;
	world_t *world; /*be it ssqc or csqc*/
	skeltype_t type;

	unsigned int numbones;
	float *bonematrix;

#ifdef RAGDOLL
	int numanimated;
	struct skelobject_s *animsource;
	unsigned int numbodies;
	body_t *body;
	int numjoints;
	rbejoint_t *joint;
	doll_t *doll;
	wedict_t *entity;	//only valid for dolls.
#endif
} skelobject_t;

#ifdef RAGDOLL
static doll_t *dolllist;
#endif
static skelobject_t skelobjects[MAX_SKEL_OBJECTS];
static int numskelobjectsused;

//copes with src==dst to convert rel->abs
static void skel_copy_toabs(skelobject_t *skelobjdst, skelobject_t *skelobjsrc, int startbone, int endbone)
{
	int maxbones;
	galiasbone_t *boneinfo = Mod_GetBoneInfo(skelobjsrc->model, &maxbones);
	if (!boneinfo)
		return;
	endbone = min(endbone, maxbones);
	if (skelobjsrc->type == SKEL_ABSOLUTE)
	{
		if (skelobjsrc != skelobjdst)
		{
			while(startbone < endbone)
			{
				Vector4Copy(skelobjsrc->bonematrix+12*startbone+0, skelobjdst->bonematrix+12*startbone+0);
				Vector4Copy(skelobjsrc->bonematrix+12*startbone+4, skelobjdst->bonematrix+12*startbone+4);
				Vector4Copy(skelobjsrc->bonematrix+12*startbone+8, skelobjdst->bonematrix+12*startbone+8);
				startbone++;
			}
		}
	}
	else
	{
		if (skelobjsrc != skelobjdst)
		{
			while(startbone < endbone)
			{
				if (boneinfo[startbone].parent >= 0)
				{
					Matrix3x4_Multiply(skelobjsrc->bonematrix+12*startbone, skelobjdst->bonematrix+12*boneinfo[startbone].parent, skelobjdst->bonematrix+12*startbone);
				}
				else
				{
					Vector4Copy(skelobjsrc->bonematrix+12*startbone+0, skelobjdst->bonematrix+12*startbone+0);
					Vector4Copy(skelobjsrc->bonematrix+12*startbone+4, skelobjdst->bonematrix+12*startbone+4);
					Vector4Copy(skelobjsrc->bonematrix+12*startbone+8, skelobjdst->bonematrix+12*startbone+8);
				}
				startbone++;
			}
		}
		else
		{
			float tmpmat[12];
			while(startbone < endbone)
			{
				if (boneinfo[startbone].parent >= 0)
				{
					Matrix3x4_Multiply(skelobjsrc->bonematrix+12*startbone, skelobjdst->bonematrix+12*boneinfo[startbone].parent, tmpmat);
					memcpy(skelobjdst->bonematrix+12*startbone, tmpmat, sizeof(tmpmat));
				}
				startbone++;
			}
		}
	}

	skelobjdst->type = SKEL_ABSOLUTE;
}
static void bonemat_fromidentity(float *out)
{
	out[0] = 1;
	out[1] = 0;
	out[2] = 0;
	out[3] = 0;

	out[4] = 0;
	out[5] = 1;
	out[6] = 0;
	out[7] = 0;

	out[8] = 0;
	out[9] = 0;
	out[10] = 1;
	out[11] = 0;
}
static void bonemat_fromqcvectors(float *out, const pvec_t vx[3], const pvec_t vy[3], const pvec_t vz[3], const pvec_t t[3])
{
	out[0] = vx[0];
	out[1] = -vy[0];
	out[2] = vz[0];
	out[3] = t[0];
	out[4] = vx[1];
	out[5] = -vy[1];
	out[6] = vz[1];
	out[7] = t[1];
	out[8] = vx[2];
	out[9] = -vy[2];
	out[10] = vz[2];
	out[11] = t[2];
}
static void bonemat_fromaxisorg(float *out, vec3_t axis[3], const float t[3])
{
	out[0] = axis[0][0];
	out[1] = axis[1][0];
	out[2] = axis[2][0];
	out[3] = t[0];
	out[4] = axis[0][1];
	out[5] = axis[1][1];
	out[6] = axis[2][1];
	out[7] = t[1];
	out[8] = axis[0][2];
	out[9] = axis[1][2];
	out[10]= axis[2][2];
	out[11]= t[2];
}
static void bonemat_fromentity(world_t *w, wedict_t *ed, float *trans)
{
	vec3_t d[3], a;
	model_t *mod;
	mod = w->Get_CModel(w, ed->v->modelindex);
	a[0] = ed->v->angles[0];
	a[1] = ed->v->angles[1];
	a[2] = ed->v->angles[2];
	if (!mod || mod->type == mod_alias)
	{
		a[0] *= r_meshpitch.value;
		a[2] *= r_meshroll.value;
	}
	AngleVectors(a, d[0], d[1], d[2]);
	bonemat_fromqcvectors(trans, d[0], d[1], d[2], ed->v->origin);
}
static void bonemat_toqcvectors(const float *in, pvec_t vx[3], pvec_t vy[3], pvec_t vz[3], pvec_t t[3])
{
	vx[0] = in[0];
	vx[1] = in[4];
	vx[2] = in[8];
	vy[0] = -in[1];
	vy[1] = -in[5];
	vy[2] = -in[9];
	vz[0] = in[2];
	vz[1] = in[6];
	vz[2] = in[10];
	t [0] = in[3];
	t [1] = in[7];
	t [2] = in[11];
}
static void bonemat_toaxisorg(const float *src, vec3_t axis[3], float t[3])
{
	axis[0][0] = src[0];
	axis[0][1] = src[4];
	axis[0][2] = src[8];
	axis[1][0] = src[1];
	axis[1][1] = src[5];
	axis[1][2] = src[9];
	axis[2][0] = src[2];
	axis[2][1] = src[6];
	axis[2][2] = src[10];
	t[0] = src[3];
	t[1] = src[7];
	t[2] = src[11];
}

static void bonematident_toqcvectors(float vx[3], float vy[3], float vz[3], float t[3])
{
	vx[0] = 1;
	vx[1] = 0;
	vx[2] = 0;
	vy[0] = -0;
	vy[1] = -1;
	vy[2] = -0;
	vz[0] = 0;
	vz[1] = 0;
	vz[2] = 1;
	t [0] = 0;
	t [1] = 0;
	t [2] = 0;
}


static qboolean pendingkill; /*states that there is a skel waiting to be killed*/
#ifdef RAGDOLL
static void rag_uninstanciate(skelobject_t *sko);
static int rag_finddollbody(doll_t *d, char *bodyname)
{
	int i;
	for (i = 0; i < d->numbodies; i++)
	{
		if (!strcmp(d->body[i].name, bodyname))
			return i;
	}
	return -1;
}
static int rag_finddolljoint(doll_t *d, char *name)
{
	int i;
	for (i = 0; i < d->numjoints; i++)
	{
		if (!strcmp(d->joint[i].name, name))
			return i;
	}
	return -1;
}

typedef struct {
	qboolean errors;
	doll_t *d;
	int numbones;
	galiasbone_t *bones;

	rbebodyinfo_t *body;
	rbejointinfo_t *joint;

	rbebodyinfo_t defbody;
	rbejointinfo_t defjoint;
} dollcreatectx_t;
static dollcreatectx_t *rag_createdoll(model_t *mod, const char *fname, int numbones)
{
	int i;
	dollcreatectx_t *ctx;
	int nummodbones = Mod_GetNumBones(mod, false);
	if (nummodbones != numbones || !numbones)
		return NULL;

	ctx = malloc(sizeof(*ctx));

	ctx->bones = Mod_GetBoneInfo(mod, &ctx->numbones);
	ctx->errors = 0;

	memset(&ctx->defbody, 0, sizeof(ctx->defbody));
	ctx->defbody.animate = true;
	ctx->defbody.geomshape = GEOMTYPE_BOX;
	ctx->defbody.dimensions[0] = 4;
	ctx->defbody.dimensions[1] = 4;
	ctx->defbody.dimensions[2] = 4;
	ctx->defbody.mass = 1;
	ctx->defbody.orient = false;

	memset(&ctx->defjoint, 0, sizeof(ctx->defjoint));
	ctx->defjoint.startenabled = true;
	ctx->defjoint.axis[1] = 1;
	ctx->defjoint.axis2[2] = 1;
	ctx->defjoint.ERP = 0.2;	//use ODE defaults
	ctx->defjoint.ERP2 = 0.2;
	ctx->defjoint.CFM = 1e-5;
	ctx->defjoint.CFM2 = 1e-5;

	ctx->d = BZ_Malloc(sizeof(*ctx->d));
	ctx->d->next = dolllist;
	ctx->d->name = strdup(fname);
	ctx->d->model = mod;
	ctx->d->refanim = -1;	//skin pose.
	ctx->d->numbodies = 0;
	ctx->d->body = NULL;
	ctx->d->numjoints = 0;
	ctx->d->uses = 0;
	ctx->d->joint = NULL;

	ctx->d->numbones = min(numbones, ctx->numbones);
	ctx->d->bone = BZ_Malloc(sizeof(*ctx->d->bone) * ctx->d->numbones);
	for (i = 0; i < ctx->d->numbones; i++)
		ctx->d->bone[i].bodyidx = -1;

	return ctx;
}
//returns true if the command was recognised. false if the command is for something else.
static qboolean rag_dollline(dollcreatectx_t *ctx, int linenum)
{
	int i;
	int argc;
	char *cmd, *val;
	doll_t *d = ctx->d;

	argc = Cmd_Argc();
	cmd = Cmd_Argv(0);
	val = Cmd_Argv(1);

	if (!argc)
	{
	}
	else if (argc == 2 && !stricmp(cmd, "refpose") && !strcmp(val, "skin"))
		ctx->d->refanim = -1;
	else if (argc == 3 && !stricmp(cmd, "refpose"))
	{
		ctx->d->refanim = atoi(val);
		ctx->d->refanimtime = atoi(Cmd_Argv(2));
	}
	//create a new body
	else if (argc == 3 && !stricmp(cmd, "body"))
	{
		int boneidx = Mod_TagNumForName(d->model, Cmd_Argv(2), 0)-1;
		ctx->joint = NULL;
		ctx->body = NULL;
		if (boneidx >= 0)
		{
			d->body = BZ_Realloc(d->body, sizeof(*d->body)*(d->numbodies+1));
			ctx->body = &d->body[d->numbodies];
			d->bone[boneidx].bodyidx = d->numbodies;
			d->numbodies++;

			*ctx->body = ctx->defbody;
			Q_strncpyz(ctx->body->name, Cmd_Argv(1), sizeof(ctx->body->name));
			ctx->body->bone = boneidx;
		}
		else if (!ctx->errors++)
			Con_Printf("^[Unable to create body \"%s\" because bone \"%s\" does not exist in %s\\edit\\%s:%i^]\n", Cmd_Argv(1), Cmd_Argv(2), d->model?d->model->name:"<NOMODEL>", d->name, linenum);
	}
	//create a new joint
	else if (argc >= 2 && !stricmp(cmd, "joint"))
	{
		char *name;
		ctx->joint = NULL;
		ctx->body = NULL;
		d->joint = BZ_Realloc(d->joint, sizeof(*d->joint)*(d->numjoints+1));
		ctx->joint = &d->joint[d->numjoints];
		*ctx->joint = ctx->defjoint;
		Q_strncpyz(ctx->joint->name, Cmd_Argv(1), sizeof(ctx->joint->name));
		name = Cmd_Argv(2);
		ctx->joint->body1 = *name?rag_finddollbody(d, name):-1;
		if (*name && ctx->joint->body1 < 0)
		{
			if (!ctx->errors++)
				Con_Printf("^[Joint \"%s\" joins invalid body \"%s\"\\edit\\%s:%i^]\n", ctx->joint->name, name, d->name, linenum);
			return true;
		}
		name = Cmd_Argv(3);
		ctx->joint->body2 = *name?rag_finddollbody(d, name):-1;
		if (*name && (ctx->joint->body2 < 0 || ctx->joint->body2 == ctx->joint->body1))
		{
			if (!ctx->errors++)
			{
				if (ctx->joint->body2 == ctx->joint->body1)
					Con_Printf("^[Joint \"%s\" joins body \"%s\" to itself\\edit\\%s:%i^]\n", ctx->joint->name, name, d->name, linenum);
				else
					Con_Printf("^[Joint \"%s\" joins invalid body \"%s\"\\edit\\%s:%i^]\n", ctx->joint->name, name, d->name, linenum);
			}
			return true;
		}
		ctx->joint->bonepivot = d->body[(ctx->joint->body2 >= 0)?ctx->joint->body2:ctx->joint->body1].bone;	//default the pivot object to the bone of the second object.

		if (ctx->joint->body1 >= 0 || ctx->joint->body2 >= 0)
			d->numjoints++;
		else if (!ctx->errors++)
			Con_Printf("^[Joint property \"%s\" not recognised\\edit\\%s:%i^]\n", ctx->joint->name, d->name, linenum);
	}
	else if (argc == 2 && !stricmp(cmd, "updatebody"))
	{
		ctx->joint = NULL;
		ctx->body = NULL;
		if (!strcmp(val, "default"))
			ctx->body = &ctx->defbody;
		else
		{
			i = rag_finddollbody(d, val);
			if (i >= 0)
				ctx->body = &d->body[i];
			else if (!ctx->errors++)
				Con_Printf("^[Cannot update body \"%s\"\\edit\\%s:%i^]\n", ctx->body->name, d->name, linenum);
		}
	}
	else if (argc == 2 && !stricmp(cmd, "updatejoint"))
	{
		ctx->joint = NULL;
		ctx->body = NULL;
		if (!strcmp(val, "default"))
			ctx->joint = &ctx->defjoint;
		else
		{
			i = rag_finddolljoint(d, val);
			if (i >= 0)
				ctx->joint = &d->joint[i];
			else if (!ctx->errors++)
				Con_Printf("^[Cannot update joint \"%s\"\\edit\\%s:%i^]\n", ctx->joint->name, d->name, linenum);
		}
	}

	//body properties
	else if (ctx->body && argc == 2 && !stricmp(cmd, "shape"))
	{
		if (!stricmp(val, "box"))
			ctx->body->geomshape = GEOMTYPE_BOX;
		else if (!stricmp(val, "sphere"))
			ctx->body->geomshape = GEOMTYPE_SPHERE;
		else if (!stricmp(val, "cylinder"))
			ctx->body->geomshape = GEOMTYPE_CYLINDER;
		else if (!stricmp(val, "capsule"))
			ctx->body->geomshape = GEOMTYPE_CAPSULE;
		else if (!ctx->errors++)
			Con_Printf("^[Joint shape \"%s\" not recognised\\edit\\%s:%i^]\n", val, d->name, linenum);
	}
	else if (ctx->body && argc == 2 && !stricmp(cmd, "animate"))
		ctx->body->animate = atof(val);
	else if (ctx->body && argc == 2 && !stricmp(cmd, "draw"))
		ctx->body->draw = atoi(val);
	else if (ctx->body && argc == 2 && !stricmp(cmd, "mass"))
		ctx->body->mass = atof(val);
	else if (ctx->body && argc == 2 && (!stricmp(cmd, "dimensions") || !stricmp(cmd, "size")))
		ctx->body->dimensions[0] = ctx->body->dimensions[1] = ctx->body->dimensions[2] = atof(val);
	else if (ctx->body && argc == 3 && (!stricmp(cmd, "dimensions") || !stricmp(cmd, "size")))
	{
		ctx->body->dimensions[0] = ctx->body->dimensions[1] = atof(val);
		ctx->body->dimensions[2] = atoi(Cmd_Argv(2));
	}
	else if (ctx->body && argc == 4 && (!stricmp(cmd, "dimensions") || !stricmp(cmd, "size")))
	{
		ctx->body->dimensions[0] = atof(val);
		ctx->body->dimensions[1] = atof(Cmd_Argv(2));
		ctx->body->dimensions[2] = atof(Cmd_Argv(3));
	}
	else if (ctx->body && argc == 4 && !stricmp(cmd, "offset"))
	{
		vec3_t ang;
		ctx->body->isoffset = true;
		ctx->body->relmatrix[3] = atof(val);
		ctx->body->relmatrix[7] = atof(Cmd_Argv(2));
		ctx->body->relmatrix[11] = atof(Cmd_Argv(3));
		ang[0] = atof(Cmd_Argv(4));
		ang[1] = atof(Cmd_Argv(5));
		ang[2] = atof(Cmd_Argv(6));
		AngleVectorsFLU(ang, &ctx->body->relmatrix[0], &ctx->body->relmatrix[4], &ctx->body->relmatrix[8]);
		Matrix3x4_Invert_Simple(ctx->body->relmatrix, ctx->body->inverserelmatrix);
	}

	//joint properties
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "type"))
	{
		if (!stricmp(val, "fixed"))
			ctx->joint->type = JOINTTYPE_FIXED;
		else if (!stricmp(val, "point"))
			ctx->joint->type = JOINTTYPE_POINT;
		else if (!stricmp(val, "hinge"))
			ctx->joint->type = JOINTTYPE_HINGE;
		else if (!stricmp(val, "slider"))
			ctx->joint->type = JOINTTYPE_SLIDER;
		else if (!stricmp(val, "universal"))
			ctx->joint->type = JOINTTYPE_UNIVERSAL;
		else if (!stricmp(val, "hinge2"))
			ctx->joint->type = JOINTTYPE_HINGE2;
	}
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "draw"))
		ctx->joint->draw = atoi(val);
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "enabled"))
		ctx->joint->startenabled = atoi(val)?true:false;
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "ERP"))
		ctx->joint->ERP = atof(val);
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "ERP2"))
		ctx->joint->ERP2 = atof(val);
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "CFM"))
		ctx->joint->CFM = atof(val);
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "CFM2"))
		ctx->joint->CFM2 = atof(val);
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "FMax"))
		ctx->joint->FMax = atof(val);
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "FMax2"))
		ctx->joint->FMax2 = atof(val);
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "HiStop"))
		ctx->joint->HiStop = atof(val);
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "HiStop2"))
		ctx->joint->HiStop2 = atof(val);
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "LoStop"))
		ctx->joint->LoStop = atof(val);
	else if (ctx->joint && argc == 2 && !stricmp(cmd, "LoStop2"))
		ctx->joint->LoStop2 = atof(val);
	else if (ctx->joint && argc == 4 && !stricmp(cmd, "axis"))
	{
		ctx->joint->axis[0] = atof(val);
		ctx->joint->axis[1] = atof(Cmd_Argv(2));
		ctx->joint->axis[2] = atof(Cmd_Argv(3));
	}
	else if (ctx->joint && argc == 4 && !stricmp(cmd, "axis2"))
	{
		ctx->joint->axis2[0] = atof(val);
		ctx->joint->axis2[1] = atof(Cmd_Argv(2));
		ctx->joint->axis2[2] = atof(Cmd_Argv(3));
	}
	else if (ctx->joint && argc == 4 && !stricmp(cmd, "offset"))
	{
		ctx->joint->offset[0] = atof(val);
		ctx->joint->offset[1] = atof(Cmd_Argv(2));
		ctx->joint->offset[2] = atof(Cmd_Argv(3));
	}
	else if (ctx->joint && ctx->joint != &ctx->defjoint && (argc == 2 || argc == 5) && !stricmp(cmd, "pivot"))
	{
		//the origin is specified in base-frame model space
		//we need to make it relative to the joint's bodies
		char *bone = val;
		i = Mod_TagNumForName(d->model, bone, 0)-1;
		if (argc > 2)
		{
			ctx->joint->offset[0] = atof(Cmd_Argv(2));
			ctx->joint->offset[1] = atof(Cmd_Argv(3));
			ctx->joint->offset[2] = atof(Cmd_Argv(4));
		}
		if (i >= 0)
		{
			ctx->joint->bonepivot = i;
			//Matrix3x4_Multiply(omat, bones[i].inverse, joint->orgmatrix);
		}
		else if (!ctx->errors++)
			Con_Printf("^[Directive \"%s\" not understood or invalid\\edit\\%s:%i^]\n", cmd, d->name, linenum);
	}
	else
		return false;

	return true;
};
static doll_t *rag_finishdoll(dollcreatectx_t *ctx)
{
	doll_t *d = ctx->d;
	int i;

	d->drawn = false;
	for (i = 0; i < d->numbodies; i++)
	{
		if (d->body[i].draw)
		{
			d->drawn = true;
			break;
		}
	}
	if (i == d->numbodies)
	for (i = 0; i < d->numjoints; i++)
	{
		if (d->joint[i].draw)
		{
			d->drawn = true;
			break;
		}
	}

	free(ctx);

	if (!d->numbodies)
	{
		rag_freedoll(d);
		return NULL;
	}
	return d;
};

doll_t *rag_createdollfromstring(model_t *mod, const char *fname, int numbones, const char *file)
{
	int linenum = 0;
	dollcreatectx_t *ctx;
	ctx = rag_createdoll(mod, fname, numbones);
	if (!ctx)
		return NULL;
	
	while(file && *file)
	{
		linenum++;
		file = Cmd_TokenizeString(file, false, false);

		if (!rag_dollline(ctx, linenum))
			if (!ctx->errors++)
				Con_Printf("^[Directive \"%s\" not understood or invalid\\edit\\%s:%i^]\n", Cmd_Argv(0), ctx->d->name, linenum);
	}
	return rag_finishdoll(ctx);
}

static doll_t *rag_loaddoll(model_t *mod, char *fname, int numbones)
{
	doll_t *d;
	void *fptr = NULL;

	for (d = dolllist; d; d = d->next)
	{
		if (d->model == mod)
			if (!strcmp(d->name, fname))
				return d;
	}

	FS_LoadFile(fname, &fptr);
	if (!fptr)
	{
#ifndef SERVERONLY
		CL_CheckOrEnqueDownloadFile(fname, NULL, 0);
#endif
		return NULL;
	}

	d = rag_createdollfromstring(mod, fname, numbones, fptr);
	FS_FreeFile(fptr);

	if (d)
	{
		d->next = dolllist;
		dolllist = d;
	}
	return d;
}
void rag_freedoll(doll_t *doll)
{
	int i;
	if (doll->uses)
	{
		for (i = 0; i < numskelobjectsused; i++)
		{
			if (skelobjects[i].doll == doll)
			{
				rag_uninstanciate(&skelobjects[i]);
				if (!doll->uses)
					break;
			}
		}
	}
	BZ_Free(doll->bone);
	BZ_Free(doll->body);
	BZ_Free(doll->joint);
	free(doll->name);
	BZ_Free(doll);
}

void rag_uninstanciateall(void)
{
	int i;
	for (i = 0; i < numskelobjectsused; i++)
	{
		rag_uninstanciate(&skelobjects[i]);
	}
}
void rag_flushdolls(qboolean force)
{
	doll_t *d, **link;
	if (force)
		rag_uninstanciateall();
	for (link = &dolllist; *link; )
	{
		d = *link;
		if (!d->uses)
		{
			*link = d->next;
			rag_freedoll(d);
		}
		else
			link = &(*link)->next;
	}
}

#if 0
static void skel_integrate(pubprogfuncs_t *prinst, skelobject_t *sko, skelobject_t *skelobjsrc, float ft, float mmat[12])
{
	trace_t t;
	vec3_t npos, opos, wnpos, wopos;
	vec3_t move;
	float wantmat[12];
	world_t *w = prinst->parms->user;
	body_t *b;
	float gravity = 800;
	int bone, bno;
	int boffs;
	galiasbone_t *boneinfo = Mod_GetBoneInfo(sko->model);
	if (!boneinfo)
		return;

	for (bone = 0, bno = 0, b = sko->body; bone < sko->numbones; bone++)
	{
		boffs = bone*12;
		/*if this bone is positioned using a physical body...*/
		if (bno < sko->numbodies && b->jointo == boffs)
		{
			if (skelobjsrc)
			{
				/*attempt to move to target*/
				if (boneinfo[bone].parent >= 0)
				{
					Matrix3x4_Multiply(skelobjsrc->bonematrix+boffs, sko->bonematrix+12*boneinfo[bone].parent, wantmat);
				}
				else
				{
					Vector4Copy(skelobjsrc->bonematrix+boffs+0, wantmat+0);
					Vector4Copy(skelobjsrc->bonematrix+boffs+4, wantmat+4);
					Vector4Copy(skelobjsrc->bonematrix+boffs+8, wantmat+8);
				}
				b->vel[0] = (wantmat[3 ] - sko->bonematrix[b->jointo + 3 ])/ft;
				b->vel[1] = (wantmat[7 ] - sko->bonematrix[b->jointo + 7 ])/ft;
				b->vel[2] = (wantmat[11] - sko->bonematrix[b->jointo + 11])/ft;
			}
			else
			{
				/*handle gravity*/
				b->vel[2] = b->vel[2] - gravity * ft / 2;
			}

			VectorScale(b->vel, ft, move);

			opos[0] = sko->bonematrix[b->jointo + 3 ];
			opos[1] = sko->bonematrix[b->jointo + 7 ];
			opos[2] = sko->bonematrix[b->jointo + 11];
			npos[0] = opos[0] + move[0];
			npos[1] = opos[1] + move[1];
			npos[2] = opos[2] + move[2];

			Matrix3x4_RM_Transform3(mmat, opos, wopos);
			Matrix3x4_RM_Transform3(mmat, npos, wnpos);

			t = World_Move(w, wopos, vec3_origin, vec3_origin, wnpos, MOVE_NOMONSTERS, w->edicts);
			if (t.startsolid)
				t.fraction = 1;
			else if (t.fraction < 1)
			{
				/*clip the velocity*/
				float backoff = -DotProduct (b->vel, t.plane.normal) * 1.9; /*teehee, bouncy corpses*/
				VectorMA(b->vel, backoff, t.plane.normal, b->vel);
			}
			if (skelobjsrc)
			{
				VectorCopy(wantmat+0, sko->bonematrix+b->jointo+0);
				VectorCopy(wantmat+4, sko->bonematrix+b->jointo+4);
				VectorCopy(wantmat+8, sko->bonematrix+b->jointo+8);
			}

			sko->bonematrix[b->jointo + 3 ] += move[0] * t.fraction;
			sko->bonematrix[b->jointo + 7 ] += move[1] * t.fraction;
			sko->bonematrix[b->jointo + 11] += move[2] * t.fraction;

			if (!skelobjsrc)
				b->vel[2] = b->vel[2] - gravity * ft / 2;

			b++;
			bno++;
		}
		else if (skelobjsrc)
		{
			/*directly copy animation skeleton*/
			if (boneinfo[bone].parent >= 0)
			{
				Matrix3x4_Multiply(skelobjsrc->bonematrix+boffs, sko->bonematrix+12*boneinfo[bone].parent, sko->bonematrix+boffs);
			}
			else
			{
				Vector4Copy(skelobjsrc->bonematrix+boffs+0, sko->bonematrix+boffs+0);
				Vector4Copy(skelobjsrc->bonematrix+boffs+4, sko->bonematrix+boffs+4);
				Vector4Copy(skelobjsrc->bonematrix+boffs+8, sko->bonematrix+boffs+8);
			}
		}
		else
		{
			/*retain the old relation*/
			/*FIXME*/
		}
	}
	/*debugging*/
#if 0
	/*draw points*/
	for (bone = 0; p < sko->numbones; bone++)
	{
		opos[0] = sko->bonematrix[bone*12 + 3 ];
		opos[1] = sko->bonematrix[bone*12 + 7 ];
		opos[2] = sko->bonematrix[bone*12 + 11];
		Matrix3x4_RM_Transform3(mmat, opos, wopos);
		P_RunParticleEffectTypeString(wopos, vec3_origin, 1, "ragdolltest");
	}
#endif
}
#endif
#endif

static void skel_generateragdoll_f_bones(vfsfile_t *f, galiasbone_t *bones, int numbones, int parent, int indent)
{
	int i, j;
	for (i = 0; i < numbones; i++)
	{
		if (bones[i].parent == parent)
		{
			VFS_PUTS(f, "//");
			for (j = 0; j < indent; j++)
				VFS_PUTS(f, "\t");
			VFS_PUTS(f, va("%i %s\n", i, bones[i].name));
			skel_generateragdoll_f_bones(f, bones, numbones, i, indent+1);
		}
	}
}
void skel_generateragdoll_f(void)
{
	char *modname = Cmd_Argv(1);
	char *outname;
	model_t *mod = Mod_ForName(modname, MLV_SILENT);
	galiasbone_t *bones;
	vfsfile_t *f;
	int i;
	int numbones;
	if (!mod)
	{
		Con_Printf("Cannot open %s\n", modname);
		return;
	}
	bones = Mod_GetBoneInfo(mod, &numbones);
	if (!bones || numbones < 1)
	{
		Con_Printf("Model %s has no bones\n", modname);
		return;
	}
	outname = va("%s.doll", mod->name);
	
	f = FS_OpenVFS(outname, "wb", FS_GAMEONLY);
	VFS_PUTS(f, va("//basic ragdoll info for model %s\n", mod->name));
	VFS_PUTS(f, va("//generated with: %s %s\n", Cmd_Argv(0), Cmd_Args()));
	VFS_PUTS(f, "//this file will need editing by hand\n");
	VFS_PUTS(f, "//use the flush command to reload this file\n");

	//print background bone info.
	VFS_PUTS(f, "\n//bones are as follows:\n");
	skel_generateragdoll_f_bones(f, bones, numbones, -1, 0);

	//print background frame info.
	VFS_PUTS(f, "\n//frames are as follows:\n");
	for (i = 0; i < 32768; i++)
	{
		char *fname;
		int numframes;
		float duration;
		qboolean loop;
		int act;
		if (!Mod_FrameInfoForNum(mod, 0, i, &fname, &numframes, &duration, &loop, &act))
			break;
		VFS_PUTS(f, va("//%i %s (%i frames) (%f secs)%s", i, fname, numframes, duration, loop?" (loop)":""));
	}
	if (i == 0)
		VFS_PUTS(f, "//NO FRAME INFO\n");

	VFS_PUTS(f, "\n//reference pose that offsets are defined in terms of\n");
	if (mod->type == mod_halflife)
		VFS_PUTS(f, "refpose 0 0.0 //use first anim's first pose\n");
	else
		VFS_PUTS(f, "refpose skin\n");

	//print background frame info.
	VFS_PUTS(f, "\n//skins are as follows:\n");
	for (i = 0; i < 32768; i++)
	{
		const char *sname;
		sname = Mod_SkinNameForNum(mod, 0, i);
		if (!sname)
			break;
		VFS_PUTS(f, va("//%i %s", i, sname));
	}
	if (i == 0)
		VFS_PUTS(f, "//NO SKIN INFO\n");

	VFS_PUTS(f, "\nupdatebody default\n");
	VFS_PUTS(f, "\tshape box	//one of box, sphere, cylinder, capsule\n");
	VFS_PUTS(f, "\tdimensions 8 8 8\n");
	VFS_PUTS(f, "\tdraw 1		//1 for visualising debug, 0 for release\n");
	VFS_PUTS(f, "\tanimate 1	//0 will always be limp\n");
	VFS_PUTS(f, "\n");

	//FIXME: only write out the bodies specified as arguments, and with no //
	for (i = 0; i < numbones; i++)
		VFS_PUTS(f, va("//body \"b_%s\" \"%s\"\n", bones[i].name, bones[i].name));
	VFS_PUTS(f, "\n");

	VFS_PUTS(f, "updatejoint default\n");
	VFS_PUTS(f, "\ttype hinge	//one of fixed, point, hinge, slider, universal, hinge2\n");
	VFS_PUTS(f, "\t//histop 1\n");
	VFS_PUTS(f, "\t//lostop -1\n");
	VFS_PUTS(f, "\t//histop2 1\n");
	VFS_PUTS(f, "\t//lostop2 -1\n");
	VFS_PUTS(f, "\t//erp\n");
	VFS_PUTS(f, "\t//erp2\n");
	VFS_PUTS(f, "\t//cfm\n");
	VFS_PUTS(f, "\t//cfm2\n");
	VFS_PUTS(f, "\t//fmax\n");
	VFS_PUTS(f, "\t//fmax2\n");
	VFS_PUTS(f, "\n");

	for (i = 0; i < numbones; i++)
	{
		if (bones[i].parent >= 0)	//don't generate joints for root bones. FIXME: also skip omitted bodies from argument list.
		{
			VFS_PUTS(f, va("//joint j_%s b_%s b_%s\n", bones[i].name, bones[bones[i].parent].name, bones[i].name));
			//FIXME: calc the extents in each pose and set the hi/lo stops.
		}
	}
	VFS_PUTS(f, "\n");

	VFS_CLOSE(f);
}
void skel_info_f(void)
{
	int i;
	for (i = 0; i < numskelobjectsused; i++)
	{
		if (skelobjects[i].world)
		{
#if defined(HAVE_CLIENT) && defined(CSQC_DAT)
			extern world_t csqc_world;
#endif
			Con_Printf("doll %i:\n", i);
#ifndef CLIENTONLY
			if (skelobjects[i].world == &sv.world)
				Con_Printf(" SSQC\n");
#endif
#if defined(HAVE_CLIENT) && defined(CSQC_DAT)
			if (skelobjects[i].world == &csqc_world)
				Con_Printf(" CSQC\n");
#endif
			Con_Printf(" type: %s\n", (skelobjects[i].type == SKEL_RELATIVE)?"parentspace":"modelspace");
			if (skelobjects[i].model)
				Con_Printf(" model: %s\n", skelobjects[i].model->name);
			Con_Printf(" bone count: %i\n", skelobjects[i].numbones);
#ifdef RAGDOLL
			if (skelobjects[i].doll)
			{
				Con_Printf(" ragdoll: %s%s\n", skelobjects[i].doll->name, ((skelobjects[i].doll == skelobjects[i].model->dollinfo)?" (model default)":""));
				Con_Printf(" phys bodies: %i\n", skelobjects[i].doll->numbodies);
			}
			if (skelobjects[i].entity)
				Con_Printf(" entity: %i (%s)\n", skelobjects[i].entity->entnum, skelobjects[i].world->progs->StringToNative(skelobjects[i].world->progs, skelobjects[i].entity->v->classname));
#endif
		}
	}
}

/*models were purged. reset any pointers to them*/
void skel_reload(void)
{
	int i;
	for (i = 0; i < countof(skelobjects); i++)
	{
		if (!skelobjects[i].model)
			continue;
		if (skelobjects[i].modelindex && skelobjects[i].world)
			skelobjects[i].model = skelobjects[i].world->Get_CModel(skelobjects[i].world, skelobjects[i].modelindex);
		else
			skelobjects[i].model = NULL;
	}
}

/*destroys all skeletons*/
void skel_reset(world_t *world)
{
	int i;

	for (i = 0; i < countof(skelobjects); i++)
	{
		if (skelobjects[i].world == world)
		{
#ifdef RAGDOLL
			rag_uninstanciate(&skelobjects[i]);
#endif
			skelobjects[i].numbones = 0;
			skelobjects[i].inuse = false;
			skelobjects[i].bonematrix = NULL;
			skelobjects[i].world = NULL;
		}
	}

	while (numskelobjectsused && !skelobjects[numskelobjectsused-1].inuse)
		numskelobjectsused--;
#ifdef RAGDOLL
	rag_flushdolls(false);
#endif
}

/*deletes any skeletons marked for deletion*/
void skel_dodelete(world_t *world)
{
	int skelidx;
	if (!pendingkill)
		return;

	pendingkill = false;
	for (skelidx = 0; skelidx < numskelobjectsused; skelidx++)
	{
		if (skelobjects[skelidx].inuse == 2)
		{
#ifdef RAGDOLL
			rag_uninstanciate(&skelobjects[skelidx]);
#endif
			skelobjects[skelidx].inuse = 0;
		}
	}

	while (numskelobjectsused && !skelobjects[numskelobjectsused-1].inuse)
		numskelobjectsused--;
}

static skelobject_t *skel_create(world_t *world, int bonecount)
{
	unsigned int skelidx;
	//invalid if the bonecount is not set...
	if (bonecount <= 0 || bonecount > MAX_BONES)
		return NULL;

	for (skelidx = 0; skelidx < numskelobjectsused; skelidx++)
	{
		if (!skelobjects[skelidx].inuse && skelobjects[skelidx].numbones == bonecount && skelobjects[skelidx].world == world)
		{
			skelobjects[skelidx].inuse = 1;
			return &skelobjects[skelidx];
		}
	}

	for (skelidx = 0; skelidx <= MAX_SKEL_OBJECTS; skelidx++)
	{
		if (!skelobjects[skelidx].inuse &&
			(!skelobjects[skelidx].numbones || skelobjects[skelidx].numbones == bonecount) &&
			(!skelobjects[skelidx].world || skelobjects[skelidx].world == world))
		{
			if (!skelobjects[skelidx].numbones)
			{
				skelobjects[skelidx].numbones = bonecount;
				skelobjects[skelidx].bonematrix = (float*)PR_AddString(world->progs, "", sizeof(float)*12*bonecount, false);
			}
			skelobjects[skelidx].world = world;
			if (numskelobjectsused <= skelidx)
				numskelobjectsused = skelidx + 1;
			skelobjects[skelidx].modelindex = 0;
			skelobjects[skelidx].model = NULL;
			skelobjects[skelidx].inuse = 1;
			return &skelobjects[skelidx];
		}
	}

	return NULL;
}
static skelobject_t *skel_get(world_t *world, int skelidx)
{
	skelidx--;
	if ((unsigned int)skelidx >= numskelobjectsused)
		return NULL;
	if (skelobjects[skelidx].inuse != 1)
		return NULL;
	return &skelobjects[skelidx];
}

void skel_lookup(world_t *world, int skelidx, framestate_t *fte_restrict out)
{
	skelobject_t *sko = skel_get(world, skelidx);
	if (sko && sko->inuse)
	{
		out->skeltype = sko->type;
		out->bonecount = sko->numbones;
		out->bonestate = sko->bonematrix;
	}
}


void skel_updateentbounds(world_t *w, wedict_t *ent)
{/*
	float radius[MAX_BONES];
	float maxr = 0;
	size_t i, numbones;
	skelobject_t *skel = skel_get(w, ent->xv->skeletonindex);
	galiasbone_t *bones;
	if (!skel)
		return;
	bones = Mod_GetBoneInfo(skel->model, &numbones);
	if (!skel || numbones != skel->numbones)
		return;
	if (skel->type == SKEL_RELATIVE)
	{
		for (i = 0; i < skel->numbones; i++)
		{
			radius[i] = skel->bonematrix[i*12+3]*skel->bonematrix[i*12+3]+skel->bonematrix[i*12+7]*skel->bonematrix[i*12+7]+skel->bonematrix[i*12+11]*skel->bonematrix[i*12+11];
			if (bones[i].parent >= 0)
				radius[i] += radius[bones[i].parent];
			if (maxr < radius[i] + bones[i].radius)
				maxr = radius[i] + bones[i].radius;
		}
		for (i = 0; i < 3; i++)
		{
			if (ent->v->absmin[i] > env->v->origin-maxr)
				ent->v->absmin[i] = env->v->origin-maxr;
			if (ent->v->absmax[i] < env->v->origin+maxr)
				ent->v->absmax[i] = env->v->origin+maxr;
		}
	}*/
}

void QCBUILTIN PF_skel_mmap(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *world = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	skelobject_t *sko = skel_get(world, skelidx);
	if (!sko || sko->world != world)
		G_INT(OFS_RETURN) = 0;
	else
		G_INT(OFS_RETURN) = (char*)sko->bonematrix - prinst->stringtable;
}

#ifdef RAGDOLL
//may not poke the skeletal object bone data.
static void rag_uninstanciate(skelobject_t *sko)
{
	int i;
	if (!sko->doll)
		return;

	if (!sko->world || !sko->world->rbe)
	{
		sko->numbodies = sko->numjoints = 0;
		Con_Printf(CON_ERROR "ERROR: Uninstanciating ragdoll from invalid world\n");
	}

	for (i = 0; i < sko->numbodies; i++)
	{
		sko->world->rbe->RagDestroyBody(sko->world, &sko->body[i].odebody);
	}
	BZ_Free(sko->body);
	sko->body = NULL;
	sko->numbodies = 0;

	for (i = 0; i < sko->numjoints; i++)
	{
		sko->world->rbe->RagDestroyJoint(sko->world, &sko->joint[i]);
	}
	BZ_Free(sko->joint);
	sko->joint = NULL;
	sko->numjoints = 0;

	sko->doll->uses--;
	sko->doll = NULL;
}
static void rag_genbodymatrix(skelobject_t *sko, rbebodyinfo_t *dollbody, float *emat, float *result)
{
	float tmp[12];
	float *bmat;
	int bone = dollbody->bone;
	bmat = sko->bonematrix + bone*12;

	if (dollbody->isoffset)
	{
		R_ConcatTransforms((void*)dollbody->relmatrix, (void*)bmat, (void*)tmp);
		bmat = tmp;
	}

	R_ConcatTransforms((void*)emat, (void*)bmat, (void*)result);

	if (dollbody->orient)
	{
		float peer[12];
		bone = dollbody->orientpeer;
		bmat = sko->bonematrix + bone*12;
		
		R_ConcatTransforms((void*)emat, (void*)bmat, (void*)peer);
	}

	//FIXME: handle biasing it to point away from the parent bone towards an orientation bone
}
static qboolean rag_animate(skelobject_t *sko, doll_t *doll, float *emat)
{
	//drive the various animated bodies to their updated positions
	int i;
	for (i = 0; i < sko->numbodies; i++)
	{
		if (!sko->body[i].animstrength)
			continue;
		rag_genbodymatrix(sko, &doll->body[i], emat, sko->body[i].animmatrix);
	}
	return true;
}
static qboolean rag_instanciate(skelobject_t *sko, doll_t *doll, float *emat, wedict_t *ent)
{
	int i;
	float *bmat;
	float bodymat[12], worldmat[12];
	vec3_t aaa2[3];
	int numbones;
	galiasbone_t *bones = Mod_GetBoneInfo(sko->model, &numbones);
	int bone;
	rbebody_t *body1, *body2;
	rbejointinfo_t *j;
	float *absolutes;
	sko->numbodies = doll->numbodies;
	sko->body = BZ_Malloc(sizeof(*sko->body) * sko->numbodies);
	sko->doll = doll;
	doll->uses++;
	sko->numanimated = 0;

	if (bones && doll->refanim)
	{
		framestate_t fstate = {0};
		float *relatives = alloca(sizeof(float)*12*numbones*2);
		fstate.g[FS_REG].frame[0] = doll->refanim;			//which anim we're using as the reference
		fstate.g[FS_REG].frametime[0] = doll->refanimtime;	//first pose of the anim
		fstate.g[FS_REG].lerpweight[0] = 1;
		fstate.g[FS_REG].endbone = numbones;

		absolutes = relatives+numbones*12;
		numbones = Mod_GetBoneRelations(sko->model, 0, numbones, bones, &fstate, relatives);
		for (i = 0; i < numbones; i++)
		{	//compute the absolutes. not gonna make a bg3 reference here.
			if (bones[i].parent>=0)
				R_ConcatTransforms((void*)(absolutes+12*bones[i].parent), (void*)(relatives+12*i), (void*)(absolutes+12*i));
			else
				memcpy(absolutes+12*i, relatives+12*i, sizeof(float)*12);
		}
	}
	else absolutes = NULL;

	for (i = 0; i < sko->numbodies; i++)
	{
		memset(&sko->body[i], 0, sizeof(sko->body[i]));

		sko->body[i].animstrength = doll->body[i].animate;
		if (sko->body[i].animstrength)
			sko->numanimated++;

		//spawn the body in the base pose, so we can add joints etc (also ignoring the entity matrix, we'll fix all that up later).
		if (absolutes)	//we have a reference pose
			memcpy(bodymat, absolutes+12*doll->body[i].bone, sizeof(float)*12);
		else if (1)
			Matrix3x4_Invert_Simple(bones[doll->body[i].bone].inverse, bodymat);
		else
			rag_genbodymatrix(sko, &doll->body[i], emat, bodymat);
		if (!sko->world->rbe->RagCreateBody(sko->world, &sko->body[i].odebody, &doll->body[i], bodymat, ent))
			return false;
	}
	sko->numjoints = doll->numjoints;
	sko->joint = BZ_Malloc(sizeof(*sko->joint) * sko->numjoints);
	memset(sko->joint, 0, sizeof(*sko->joint) * sko->numjoints);
	for(i = 0; i < sko->numjoints; i++)
	{
		j = &doll->joint[i];
		body1 = j->body1>=0?&sko->body[j->body1].odebody:NULL;
		body2 = j->body2>=0?&sko->body[j->body2].odebody:NULL;

		bone = j->bonepivot;
		bmat = sko->bonematrix + bone*12;

		if (absolutes)	//we have a reference pose
			memcpy(worldmat, absolutes+12*doll->body[i].bone, sizeof(float)*12);
		else if (1)
		{	//FIXME: j->offset isn't actually used?!?
			Matrix3x4_Invert_Simple(bones[j->bonepivot].inverse, worldmat);
		}
		else
		{
			memcpy(bodymat, bmat, sizeof(bodymat));
			bodymat[3] += j->offset[0];
			bodymat[3+4] += j->offset[1];
			bodymat[3+8] += j->offset[2];
			R_ConcatTransforms((void*)emat, (void*)bodymat, (void*)worldmat);
		}
		aaa2[0][0] = worldmat[3];
		aaa2[0][1] = worldmat[3+4];
		aaa2[0][2] = worldmat[3+8];
		VectorNormalize2(j->axis, aaa2[1]);
		VectorNormalize2(j->axis2, aaa2[2]);

		sko->world->rbe->RagCreateJoint(sko->world, &sko->joint[i], j, body1, body2, aaa2);

		sko->world->rbe->RagEnableJoint(&sko->joint[i], j->startenabled);
	}

	//now the joints have all their various properties, move the bones to their real positions.
	//this might result in the body flying across the room...
	for (i = 0; i < sko->numbodies; i++)
	{
		rag_genbodymatrix(sko, &doll->body[i], emat, bodymat);
		sko->world->rbe->RagMatrixToBody(&sko->body[i].odebody, bodymat);
	}

	sko->doll->numdefaultanimated = sko->numanimated;
	return true;
}

void CLQ1_AddOrientedCube(shader_t *shader, vec3_t mins, vec3_t maxs, float *matrix, float r, float g, float b, float a);
void CLQ1_AddOrientedCylinder(shader_t *shader, float radius, float height, qboolean capsule, float *matrix, float r, float g, float b, float a);
void CLQ1_DrawLine(shader_t *shader, vec3_t v1, vec3_t v2, float r, float g, float b, float a);
static void rag_derive(skelobject_t *sko, skelobject_t *asko, float *emat)
{
	doll_t *doll = sko->doll;
	float *bmat = sko->bonematrix;
	float *amat = asko?asko->bonematrix:NULL;
	int numbones;
	galiasbone_t *bones = Mod_GetBoneInfo(sko->model, &numbones);
	int i;
	float invemat[12];
	float bodymat[12], rel[12];

	Matrix3x4_Invert(emat, invemat);

#ifndef SERVERONLY
	if (doll->drawn)
	{
		float rad;
		vec3_t mins, maxs;
		shader_t *debugshader = NULL;
		shader_t *lineshader = NULL;
		vec3_t start, end;
		for (i = 0; i < sko->numbodies; i++)
		{
			if (!doll->body[i].draw)
				continue;

			if (!debugshader)
				debugshader = R_RegisterShader("boneshader", SUF_NONE,
					"{\n"
					"polygonoffset\n"
					"{\n"
					"map $whiteimage\n"
					"blendfunc add\n"
					"rgbgen vertex\n"
					"alphagen vertex\n"
					"}\n"
					"}\n");
			
			sko->world->rbe->RagMatrixFromBody(sko->world, &sko->body[i].odebody, bodymat);

			switch(doll->body[i].geomshape)
			{
			default:
			case GEOMTYPE_BOX:
				VectorScale(doll->body[i].dimensions, -0.5, mins);
				VectorScale(doll->body[i].dimensions, 0.5, maxs);
				CLQ1_AddOrientedCube(debugshader, mins, maxs, bodymat, 0.2, 0.2, 0.2, 1);
				break;
			case GEOMTYPE_CYLINDER:
				rad = (doll->body[i].dimensions[0] + doll->body[i].dimensions[1])*0.5;
				CLQ1_AddOrientedCylinder(debugshader, rad, doll->body[i].dimensions[2], false, bodymat, 0.2, 0.2, 0.2, 1);
				break;
			case GEOMTYPE_CAPSULE:
				rad = (doll->body[i].dimensions[0] + doll->body[i].dimensions[1])*0.5;
				CLQ1_AddOrientedCylinder(debugshader, rad, doll->body[i].dimensions[2], true, bodymat, 0.2, 0.2, 0.2, 1);
				break;
			case GEOMTYPE_SPHERE:
				rad = (doll->body[i].dimensions[0] + doll->body[i].dimensions[1] + doll->body[i].dimensions[2])/3;
				CLQ1_AddOrientedCylinder(debugshader, rad, rad, true, bodymat, 0.2, 0.2, 0.2, 1);
				break;
			}
		}
		mins[0] = mins[1] = mins[2] = -1;
		maxs[0] = maxs[1] = maxs[2] = 1;
		for (i = 0; i < doll->numjoints; i++)
		{
			if (!doll->joint[i].draw)
				continue;
			sko->world->rbe->RagMatrixFromJoint(&sko->joint[i], &doll->joint[i], bodymat);
	//		CLQ1_AddOrientedCube(debugshader, mins, maxs, bodymat, 0, 0.2, 0, 1);

			if (!lineshader)
				lineshader = R_RegisterShader("lineshader", SUF_NONE,
					"{\n"
					"polygonoffset\n"
					"{\n"
					"map $whiteimage\n"
					"blendfunc add\n"
					"rgbgen vertex\n"
					"alphagen vertex\n"
					"}\n"
					"}\n");
			start[0] = bodymat[3];
			start[1] = bodymat[7];
			start[2] = bodymat[11];
			end[0] = bodymat[3] + bodymat[8]*4;
			end[1] = bodymat[7] + bodymat[9]*4;
			end[2] = bodymat[11] + bodymat[10]*4;
			CLQ1_DrawLine(lineshader, start, end, 0, 1, 1, 1);
			start[0] = bodymat[3] + bodymat[4]*-2;
			start[1] = bodymat[7] + bodymat[5]*-2;
			start[2] = bodymat[11] + bodymat[6]*-2;
			end[0] = bodymat[3] + bodymat[4]*2;
			end[1] = bodymat[7] + bodymat[5]*2;
			end[2] = bodymat[11] + bodymat[6]*2;
			CLQ1_DrawLine(lineshader, start, end, 1, 1, 0, 1);
		}
	}
#endif
	for (i = 0; i < doll->numbones; i++)
	{
		if (doll->bone[i].bodyidx >= 0)
		{
			//bones with a body are given an absolute pose matching that body.
			sko->world->rbe->RagMatrixFromBody(sko->world, &sko->body[doll->bone[i].bodyidx].odebody, bodymat);
			if (doll->body[doll->bone[i].bodyidx].isoffset)
			{
				float tmp[12];
				R_ConcatTransforms((void*)doll->body[doll->bone[i].bodyidx].inverserelmatrix, (void*)bodymat, (void*)tmp);
				R_ConcatTransforms((void*)invemat, (void*)tmp, (void*)((float*)bmat+i*12));
			}
			else
				//that body matrix is in world space, so transform to model space for our result
				R_ConcatTransforms((void*)invemat, (void*)bodymat, (void*)((float*)bmat+i*12));
		}
		else if (amat)	//FIXME: don't do this when the bone has an unanimated child body.
		{
			//this bone has no joint object, use the anim sko's relative pose info instead
			if (bones[i].parent >= 0)
				R_ConcatTransforms((void*)(bmat + bones[i].parent*12), (void*)((float*)amat+i*12), (void*)((float*)bmat+i*12));
			else
				memcpy((void*)((float*)bmat+i*12), (void*)((float*)amat+i*12), sizeof(float)*12);
		}
		else
		{
			//copy from the base pose
			Matrix3x4_Invert_Simple(bones[i].inverse, bodymat);
			//that's the absolute pose...
			if (bones[i].parent >= 0)
			{
				R_ConcatTransforms((void*)(bones[bones[i].parent].inverse), (void*)bodymat, (void*)rel);
				R_ConcatTransforms((void*)(bmat + bones[i].parent*12), (void*)rel, (void*)((float*)bmat+i*12));
			}
			else
			{
				//its all absolute when its the root.
				memcpy((void*)((float*)bmat+i*12), bodymat, sizeof(float)*12);
			}
		}
	}

	//if it wasn't before, it definitely is now.
	sko->type = SKEL_ABSOLUTE;
}

//called each physics frame to update the body velocities for animation
void rag_doallanimations(world_t *world)
{
	int i, j;
	doll_t *doll;
	skelobject_t *sko;
	for (i = 0; i < numskelobjectsused; i++)
	{
		sko = &skelobjects[i];
		if (sko->world != world)
			continue;
		doll = sko->doll;
		if (!doll || !sko->numanimated)
			continue;

		for (j = 0; j < sko->numbodies; j++)
		{
			if (!sko->body[j].animstrength)
				continue;
			sko->world->rbe->RagMatrixToBody(&sko->body[j].odebody, sko->body[j].animmatrix);
		}
	}
}

#ifndef SERVERONLY
void rag_removedeltaent(lerpents_t *le)
{
	extern world_t csqc_world;
	int skelidx = le->skeletalobject;
	skelobject_t *skelobj;

	if (!skelidx)
		return;
	le->skeletalobject = 0;

	skelobj = skel_get(&csqc_world, skelidx);
	if (skelobj)
	{
		skelobj->inuse = 2;	//2 means don't reuse yet.
		skelobj->modelindex = 0;
		skelobj->model = NULL;
		pendingkill = true;
	}
}

//we received some pos+quat data from the network
//store it into some skeletal object associated with the entity for the renderer to use as needed
void rag_lerpdeltaent(lerpents_t *le, unsigned int bonecount, short *newstate, float frac, short *oldstate)
{
	extern world_t csqc_world;
	int i;
	float sc;
	vec3_t pos;
	vec4_t quat, quat1, quat2;
	vec3_t scale = {1,1,1};
	skelobject_t *sko;
	if (le->skeletalobject)
	{
		sko = skel_get(&csqc_world, le->skeletalobject);
		if (sko->numbones != bonecount)
		{	//unusable, discard it and create a new one.
			sko->inuse = 2;	//2 means don't reuse yet.
			sko->modelindex = 0;
			sko->model = NULL;
			pendingkill = true;
			sko = NULL;
		}
	}
	else
		sko = NULL;

	if (!sko || sko->inuse != 1)
	{
		sko = skel_create(&csqc_world, bonecount);
		if (!sko)
			return;	//couldn't get one, ran out of memory or something?
		sko->modelindex = 0;
		sko->model = NULL;
		sko->type = SKEL_RELATIVE;
		le->skeletalobject = (sko - skelobjects) + 1;
	}

	if (!newstate)
	{	//shouldn't happen
		Con_Printf("invalid networked bone state\n");
		rag_removedeltaent(le);
		return;
	}
	if (frac == 1 || !oldstate)
	{
		for (i = 0; i < bonecount; i++, newstate += 7)
		{
			sc = (newstate[6] > 0)?-1.0/32767:1.0/32767;
			VectorScale(newstate, 1.0/64, pos);
			Vector4Scale(newstate+3, sc, quat);
			GenMatrixPosQuat4Scale(pos, quat, scale, sko->bonematrix+i*12);
		}
	}
	else
	{
		for (i = 0; i < bonecount; i++, oldstate += 7, newstate += 7)
		{	//this is annoying because of the quat sign
			sc = 1.0/64 * (1-frac);
			VectorScale(oldstate, sc, pos);
			sc = (oldstate[6] > 0)?-1.0/32767:1.0/32767;
			Vector4Scale(oldstate+3, sc, quat1);

			sc = 1.0/64 * frac;
			VectorMA(pos, sc, newstate, pos);
			sc = (newstate[6] > 0)?-1.0/32767:1.0/32767;
			Vector4Scale(newstate+3, sc, quat2);

			QuaternionSlerp(quat1, quat2, frac, quat);

			GenMatrixPosQuat4Scale(pos, quat, scale, sko->bonematrix+i*12);
		}
	}
}

void rag_updatedeltaent(world_t *w, entity_t *ent, lerpents_t *le)
{
	model_t *mod = ent->model;
	skelobject_t *sko;
	float emat[12];
	skelobject_t skorel = {0};
	float relmat[MAX_BONES*12];
	skorel.bonematrix = relmat;
	skorel.type = SKEL_RELATIVE;

	if (mod->dollinfo)
	{
		if (!w->rbe)
			return;

		if (!le->skeletalobject)
		{
			sko = skel_create(w, Mod_GetNumBones(mod, false));
			if (!sko)
				return;	//couldn't get one, ran out of memory or something?
			sko->modelindex = 0;
			sko->model = mod;
			sko->type = SKEL_RELATIVE;
			le->skeletalobject = (sko - skelobjects) + 1;
		}
		else
		{
			sko = skel_get(w, le->skeletalobject);
			if (!sko)
			{
				le->skeletalobject = 0;
				return;	//couldn't get one, ran out of memory or something?
			}
		}

		skorel.numbones = sko->numbones;

		//FIXME: provide some way for the animation to auto-trigger ragdoll (so framegroups can work automagically)
		if ((ent->framestate.g[FS_REG].frame[0] & 0x8000) || (ent->framestate.g[FS_REG].frame[1] & 0x8000))
			sko->numanimated = 0;
		else if (sko->doll)
			sko->numanimated = sko->doll->numdefaultanimated;
		Mod_GetBoneRelations(mod, 0, skorel.numbones, NULL, &ent->framestate, skorel.bonematrix);
		skorel.modelindex = sko->modelindex;
		skorel.model = sko->model;
		if (sko->numanimated || sko->doll != mod->dollinfo)
		{
//			sko->type = SKEL_ABSOLUTE;
//			Alias_ForceConvertBoneData(skorel.type, skorel.bonematrix, skorel.numbones, bones, sko->type, sko->bonematrix, sko->numbones);
			skel_copy_toabs(sko, &skorel, 0, sko->numbones);
		}

		bonemat_fromaxisorg(emat, ent->axis, ent->origin);

		if (sko->doll != mod->dollinfo)
		{
			rag_uninstanciate(sko);
			rag_instanciate(sko, mod->dollinfo, emat, NULL);
		}
		if (sko->numanimated)
			rag_animate(sko, sko->doll, emat);

		rag_derive(sko, sko->numanimated?&skorel:NULL, emat);

		ent->framestate.bonestate = sko->bonematrix;
		ent->framestate.bonecount = sko->numbones;
		ent->framestate.skeltype = sko->type;
	}
	else if (le->skeletalobject)
	{
		sko = skel_get(w, le->skeletalobject);
		if (!sko)
		{
			le->skeletalobject = 0;
			return;	//couldn't get one, ran out of memory or something?
		}

		ent->framestate.bonestate = sko->bonematrix;
		ent->framestate.bonecount = sko->numbones;
		ent->framestate.skeltype = sko->type;
	}
}
#endif
#endif
#endif

#ifdef SKELETALOBJECTS
//update a skeletal object to track its ragdoll/apply a ragdoll to a skeletal object.
void QCBUILTIN PF_skel_ragedit(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//do we want to be able to generate a ragdoll object with this function too?
#ifdef RAGDOLL
	world_t *w = prinst->parms->user;
	wedict_t *wed = (wedict_t*)G_EDICT(prinst, OFS_PARM0);
	const char *ragname = PR_GetStringOfs(prinst, OFS_PARM1);
	int parentskel = G_FLOAT(OFS_PARM2);
	int skelidx;
	skelobject_t *sko, *psko;
	doll_t *doll;
	float emat[12];

	pvec3_t d[3], a;
	//fixme: respond to renderflags&USEAXIS? scale?
	a[0] = wed->v->angles[0] * r_meshpitch.value; /*mod_alias bug*/
	a[1] = wed->v->angles[1];
	a[2] = wed->v->angles[2] * r_meshroll.value; /*hexen2 bug*/
	AngleVectors(a, d[0], d[1], d[2]);
	bonemat_fromqcvectors(emat, d[0], d[1], d[2], wed->v->origin);
	skelidx = wed->xv->skeletonindex;

	G_FLOAT(OFS_RETURN) = 0;

	//the parent skeletal object must be relative, if specified.
	psko = skel_get(w, parentskel);
	if (psko && psko->type != SKEL_RELATIVE)
		return;

	sko = skel_get(w, skelidx);
	if (!sko)
	{
		Con_DPrintf("PF_skel_ragedit: invalid skeletal object\n");
		return;
	}

	if (!sko->world->rbe)
	{
		Con_DPrintf("PF_skel_ragedit: rigid body system not enabled\n");
		return;
	}

	if (sko->doll)	//use the current doll
		doll = sko->doll;
	else if (sko->model) //
		doll = sko->model->dollinfo;
	else
		doll = NULL;

	if (*ragname)
	{
		char *cmd;

		ragname = Cmd_TokenizeString(ragname, false, false);
		cmd = Cmd_Argv(0);
		if (!stricmp(cmd, "enablejoint"))
		{
			int idx;
			int enable = atoi(Cmd_Argv(2));
			if (!sko->doll)
			{
				skel_copy_toabs(sko, psko?psko:sko, 0, sko->numbones);
				if (!doll || !rag_instanciate(sko, doll, emat, wed))
				{
					Con_Printf("enablejoint: doll not instanciated yet\n");
					return;
				}
			}
			idx = rag_finddolljoint(sko->doll, Cmd_Argv(1));
	
			if (idx >= 0)
			{
				sko->world->rbe->RagEnableJoint(&sko->joint[idx], enable);
				G_FLOAT(OFS_RETURN) = 1;
			}
			else
			{
				Con_Printf("enablejoint: %s is not defined as a ragdoll joint\n", Cmd_Argv(1));
				G_FLOAT(OFS_RETURN) = 0;
			}
			return;
		}
		else if (!stricmp(cmd, "animatebody"))
		{
			int body;
			float strength = atof(Cmd_Argv(2));
			if (!sko->doll)
			{
				skel_copy_toabs(sko, psko?psko:sko, 0, sko->numbones);
				if (!doll || !rag_instanciate(sko, doll, emat, wed))
				{
					Con_Printf("animatebody: doll not instanciated yet\n");
					return;
				}
			}	
			body = rag_finddollbody(sko->doll, Cmd_Argv(1));
			if (body >= 0)
			{
				if (sko->body[body].animstrength)
					sko->numanimated--;
				sko->body[body].animstrength = strength;
				if (sko->body[body].animstrength)
					sko->numanimated++;
			}
			else
				Con_Printf("animatebody: %s is not defined as a ragdoll body\n", Cmd_Argv(1));
			G_FLOAT(OFS_RETURN) = sko->numanimated;
			return;
		}
		else if (!stricmp(cmd, "animate"))
		{
			float strength = atof(Cmd_Argv(1));
			int i;
			if (!sko->doll)
			{
				skel_copy_toabs(sko, psko?psko:sko, 0, sko->numbones);
				if (!doll || !rag_instanciate(sko, doll, emat, wed))
				{
					Con_Printf("animate: doll not instanciated yet\n");
					return;
				}
			}
			sko->numanimated = 0;

			for (i = 0; i < sko->numbodies; i++)
			{
				sko->body[i].animstrength = sko->doll->body[i].animate * strength;
				if (sko->body[i].animstrength)
					sko->numanimated++;
			}

			if (sko->numanimated)
			{
				//make sure the animation target is valid.
				skel_copy_toabs(sko, psko?psko:sko, 0, sko->numbones);
				rag_animate(sko, sko->doll, emat);
			}
			G_FLOAT(OFS_RETURN) = 1;
			return;
		}
		else if (!stricmp(cmd, "doll"))
			doll = sko->model?rag_loaddoll(sko->model, Cmd_Argv(1), sko->numbones):NULL;
		else if (!stricmp(cmd, "dollstring"))
			doll = sko->model?rag_createdollfromstring(sko->model, "", sko->numbones, ragname):NULL;
		else if (!stricmp(cmd, "cleardoll"))
			doll = NULL;
		else
		{
			Con_Printf("PF_skel_ragedit: Unsupported command.\n");
			return;
		}
	}

	if (sko->doll != doll)
	{
		rag_uninstanciate(sko);
		if (!doll)
		{
			G_FLOAT(OFS_RETURN) = 1;	//technically success.
			return;
		}
		skel_copy_toabs(sko, psko?psko:sko, 0, sko->numbones);
		if (!rag_instanciate(sko, doll, emat, wed))
		{
			rag_uninstanciate(sko);
			Con_DPrintf("PF_skel_ragedit: unable to instanciate objects\n");
			G_FLOAT(OFS_RETURN) = 0;
			return;
		}
		if (sko->numanimated)
			rag_animate(sko, doll, emat);
	}
	else if (!doll)
	{
		G_FLOAT(OFS_RETURN) = !!*ragname;	//technically success.
		return;
	}
	else if (sko->numanimated)
	{
		skel_copy_toabs(sko, psko?psko:sko, 0, sko->numbones);
		rag_animate(sko, doll, emat);
	}

	if (psko == sko)
	{
		Con_Printf("PF_skel_ragedit: cannot use the same skeleton for animation source\n");
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}
	rag_derive(sko, psko, emat);
	G_FLOAT(OFS_RETURN) = 1;
#endif
}

//float(float modelindex) skel_create (FTE_CSQC_SKELETONOBJECTS)
void QCBUILTIN PF_skel_create (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;

	int numbones;
	skelobject_t *skelobj;
	model_t *model;
	int midx;
	int type;

	midx = G_FLOAT(OFS_PARM0);
	type = (prinst->callargc > 1)?G_FLOAT(OFS_PARM1):SKEL_RELATIVE;

	//default to failure
	G_FLOAT(OFS_RETURN) = 0;

	model = w->Get_CModel(w, midx);
	if (!model)
		return; //no model set, can't get a skeleton

	numbones = Mod_GetNumBones(model, type != SKEL_RELATIVE);
	if (!numbones)
	{
//		isabs = true;
//		numbones = Mod_GetNumBones(model, isabs);
//		if (!numbones)
			return;	//this isn't a skeletal model.
	}

	skelobj = skel_create(w, numbones);
	if (!skelobj)
		return;	//couldn't get one, ran out of memory or something?

	skelobj->modelindex = midx;
	skelobj->model = model;
	skelobj->type = type;

	/*
	for (i = 0; i < numbones; i++)
	{
		galiasbone_t *bones = Mod_GetBoneInfo(skelobj->model);
		Matrix3x4_Invert_Simple(bones[i].inverse, skelobj->bonematrix + i*12);
	}
	skelobj->type = SKOT_ABSOLUTE;
	*/

	G_FLOAT(OFS_RETURN) = (skelobj - skelobjects) + 1;
}

//float(float skel, entity ent, float modelindex, float retainfrac, float firstbone, float lastbone) skel_build (FTE_CSQC_SKELETONOBJECTS)
void QCBUILTIN PF_skel_build(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	wedict_t *ent = (wedict_t*)G_EDICT(prinst, OFS_PARM1);
	int midx = G_FLOAT(OFS_PARM2);
	float retainfrac = G_FLOAT(OFS_PARM3);
	int firstbone = G_FLOAT(OFS_PARM4)-1;
	int lastbone = G_FLOAT(OFS_PARM5)-1;
	float addition = (prinst->callargc>6)?G_FLOAT(OFS_PARM6):1-retainfrac;

	int i, j;
	int numbones;
	framestate_t fstate;
	skelobject_t *skelobj;
	model_t *model;
	galiasbone_t *boneinfo;

	//default to failure
	G_FLOAT(OFS_RETURN) = 0;

	model = w->Get_CModel(w, midx);
	if (!model)
		return; //invalid model, can't get a skeleton

	w->Get_FrameState(w, ent, &fstate);

	//heh... don't copy.
	fstate.bonecount = 0;
	fstate.bonestate = NULL;

	if (!skelidx)
	{
		numbones = Mod_GetNumBones(model, false);
		if (!numbones)
		{
			return;	//this isn't a skeletal model.
		}
		skelobj = skel_create(w, numbones);
	}
	else
		skelobj = skel_get(w, skelidx);
	if (!skelobj)
		return;	//couldn't get one, ran out of memory or something?

	if (skelobj->model)
		boneinfo = Mod_GetBoneInfo(skelobj->model, &numbones);
	else
		boneinfo = NULL;
	numbones = skelobj->numbones;


	if (lastbone < 0)
		lastbone = numbones;
	if (lastbone > numbones)
		lastbone = numbones;
	if (firstbone < 0)
		firstbone = 0;
	if (lastbone < firstbone)
		lastbone = firstbone;

	if (skelobj->type != SKEL_RELATIVE)
	{
		if (firstbone > 0 || lastbone < skelobj->numbones || retainfrac)
		{
			Con_Printf("skel_build on non-relative skeleton\n");
			return;
		}
		skelobj->type = SKEL_RELATIVE;	//entire model will get replaced, convert it.
	}

	if (retainfrac == 0)
	{
		if (addition == 0) /*wipe it*/
			memset(skelobj->bonematrix + firstbone*12, 0, sizeof(float)*12*(lastbone-firstbone));
		else if (addition == 1) /*replace everything*/
			Mod_GetBoneRelations(model, firstbone, lastbone, boneinfo, &fstate, skelobj->bonematrix);
		else
		{
			//scale new
			float relationsbuf[MAX_BONES*12];
			Mod_GetBoneRelations(model, firstbone, lastbone, boneinfo, &fstate, relationsbuf);
			for (i = firstbone; i < lastbone; i++)
			{
				for (j = 0; j < 12; j++)
					skelobj->bonematrix[i*12+j] = addition*relationsbuf[i*12+j];
			}
		}
	}
	else
	{
		float relationsbuf[MAX_BONES*12];

		if (retainfrac != 1)
		{
			//rescale the existing bones
			for (i = firstbone; i < lastbone; i++)
			{
				for (j = 0; j < 12; j++)
					skelobj->bonematrix[i*12+j] *= retainfrac;
			}
		}

		//just add
		Mod_GetBoneRelations(model, firstbone, lastbone, boneinfo, &fstate, relationsbuf);
		if (addition == 1)
		{
			for (i = firstbone; i < lastbone; i++)
			{
				for (j = 0; j < 12; j++)
					skelobj->bonematrix[i*12+j] += relationsbuf[i*12+j];
			}
		}
		else
		{
			for (i = firstbone; i < lastbone; i++)
			{
				for (j = 0; j < 12; j++)
					skelobj->bonematrix[i*12+j] += addition*relationsbuf[i*12+j];
			}
		}
	}

	G_FLOAT(OFS_RETURN) = (skelobj - skelobjects) + 1;
}

typedef struct
{
	int sourcemodelindex;
	int sourceskel;
	int firstbone;
	int lastbone;
	float prescale;	//0 destroys existing data, 1 retains it.
	float scale[4];
	int animation[4];
	float animationtime[4];

	//halflife models
	float subblend[2];
	float controllers[5];
} skelblend_t;
//float(float skel, int numblends, skelblend_t *blenddata, int structsize) skel_build_ptr
void QCBUILTIN PF_skel_build_ptr(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	int numblends = G_INT(OFS_PARM1);
	int blends_qcptr = G_INT(OFS_PARM2);
	int structsize = G_INT(OFS_PARM3);

	skelblend_t *fte_restrict blends = (skelblend_t*)(prinst->stringtable+blends_qcptr);
	int i, j;
	framestate_t fstate;
	skelobject_t *skelobj;

	float relationsbuf[MAX_BONES*12];
	float scale;
	int numbones, firstbone, lastbone;
	model_t *model;
	qboolean noadd;
	const galiasbone_t *boneinfo = NULL;

	//default to failure
	G_FLOAT(OFS_RETURN) = 0;

	memset(&fstate, 0, sizeof(fstate));

	skelobj = skel_get(w, skelidx);
	if (!skelobj)
		return;	//couldn't get one, ran out of memory or something?

	if (structsize < sizeof(skelblend_t))
		return;	//enforce a minimum size...

	for(; numblends --> 0; blends = (skelblend_t*)((char*)blends + structsize))
	{
		noadd = blends->scale[0] == 0 && blends->scale[1] == 0 && blends->scale[2] == 0 && blends->scale[3] == 0;
		if (blends->prescale == 1 && noadd)
			continue;	//does nothing to the model, skip it before wasting too much time.
			
		if (blends->sourceskel)
		{
			skelobject_t *srcskel = skel_get(w, blends->sourceskel);
			if (!srcskel)
				continue;
			model = srcskel->model;
			if (!model)
				continue; //invalid model, can't get a skeleton
			fstate.bonecount = numbones = srcskel->numbones;
			fstate.bonestate = srcskel->bonematrix;
			fstate.skeltype = srcskel->type;
		}
		else
		{
			fstate.bonecount = 0;
			fstate.bonestate = NULL;
			fstate.skeltype = SKEL_RELATIVE;

			if (blends->sourcemodelindex)
				model = w->Get_CModel(w, blends->sourcemodelindex);
			else
				model = w->Get_CModel(w, skelobj->modelindex);
			if (!model)
				continue; //invalid model, can't get a skeleton

			numbones = Mod_GetNumBones(model, false);
			if (!numbones)
				continue;	//this isn't a skeletal model.
		}

		firstbone = blends->firstbone-1;
		lastbone = blends->lastbone-1;

		if (lastbone < 0)
			lastbone = numbones;
		if (lastbone > numbones)
			lastbone = numbones;
		if (firstbone < 0)
			firstbone = 0;
		if (lastbone < firstbone)
			lastbone = firstbone;

		fstate.g[FS_REG].endbone = 0x7fffffff;
		for (i = 0; i < FRAME_BLENDS; i++)
		{
			fstate.g[FS_REG].frame[i] = blends->animation[i];
			fstate.g[FS_REG].frametime[i] = blends->animationtime[i];
			fstate.g[FS_REG].lerpweight[i] = blends->scale[i];
		}
#ifdef HALFLIFEMODELS
		fstate.g[FS_REG].subblendfrac = blends->subblend[0];
		fstate.g[FS_REG].subblend2frac = blends->subblend[1];
		fstate.bonecontrols[0] = blends->controllers[0];
		fstate.bonecontrols[1] = blends->controllers[1];
		fstate.bonecontrols[2] = blends->controllers[2];
		fstate.bonecontrols[3] = blends->controllers[3];
		fstate.bonecontrols[4] = blends->controllers[4];
#endif

		if (skelobj->type != SKEL_RELATIVE)
		{
			if (firstbone > 0 || lastbone < skelobj->numbones || blends->prescale)
			{
				Con_Printf("skel_build on non-relative skeleton\n");
				return;
			}
			skelobj->type = SKEL_RELATIVE;	//entire model will get replaced, convert it.
		}
		if (noadd)
		{
			if (blends->prescale == 0)
				memset(skelobj->bonematrix+firstbone*12, 0, sizeof(float)*12);
			else
			{	//just rescale the existing bones
				scale = blends->prescale;
				for (i = firstbone; i < lastbone; i++)
				{
					for (j = 0; j < 12; j++)
						skelobj->bonematrix[i*12+j] *= scale;
				}
			}
		}
		else if (blends->prescale == 0) //new data only. directly replace the existing data
			Mod_GetBoneRelations(model, firstbone, lastbone, boneinfo, &fstate, skelobj->bonematrix);
		else
		{
			if (blends->prescale != 1)
			{	//rescale the existing bones
				scale = blends->prescale;
				for (i = firstbone; i < lastbone; i++)
				{
					for (j = 0; j < 12; j++)
						skelobj->bonematrix[i*12+j] *= scale;
				}
			}

			Mod_GetBoneRelations(model, firstbone, lastbone, boneinfo, &fstate, relationsbuf);
			for (i = firstbone; i < lastbone; i++)
			{
				for (j = 0; j < 12; j++)
					skelobj->bonematrix[i*12+j] += relationsbuf[i*12+j];
			}
		}
	}

	G_FLOAT(OFS_RETURN) = (skelobj - skelobjects) + 1;
}

//float(float skel) skel_get_numbones (FTE_CSQC_SKELETONOBJECTS)
void QCBUILTIN PF_skel_get_numbones (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	skelobject_t *skelobj;

	skelobj = skel_get(w, skelidx);

	if (!skelobj)
		G_FLOAT(OFS_RETURN) = 0;
	else
		G_FLOAT(OFS_RETURN) = skelobj->numbones;
}

//string(float skel, float bonenum) skel_get_bonename (FTE_CSQC_SKELETONOBJECTS) (returns tempstring)
void QCBUILTIN PF_skel_get_bonename (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	int boneidx = G_FLOAT(OFS_PARM1);
	skelobject_t *skelobj;

	skelobj = skel_get(w, skelidx);

	if (!skelobj)
		G_INT(OFS_RETURN) = 0;
	else
	{
		RETURN_TSTRING(Mod_GetBoneName(skelobj->model, boneidx));
	}
}

//float(float skel, float bonenum) skel_get_boneparent (FTE_CSQC_SKELETONOBJECTS)
void QCBUILTIN PF_skel_get_boneparent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	int boneidx = G_FLOAT(OFS_PARM1);
	skelobject_t *skelobj;

	skelobj = skel_get(w, skelidx);

	if (!skelobj)
		G_FLOAT(OFS_RETURN) = 0;
	else
		G_FLOAT(OFS_RETURN) = Mod_GetBoneParent(skelobj->model, boneidx);
}

//float(float skel, string tagname) skel_find_bone (FTE_CSQC_SKELETONOBJECTS)
void QCBUILTIN PF_skel_find_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	const char *bname = PR_GetStringOfs(prinst, OFS_PARM1);
	skelobject_t *skelobj;

	skelobj = skel_get(w, skelidx);
	if (!skelobj)
		G_FLOAT(OFS_RETURN) = 0;
	else
		G_FLOAT(OFS_RETURN) = Mod_TagNumForName(skelobj->model, bname, 0);
}

//vector(float skel, float bonenum) skel_get_bonerel (FTE_CSQC_SKELETONOBJECTS) (sets v_forward etc)
void QCBUILTIN PF_skel_get_bonerel (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	int boneidx = G_FLOAT(OFS_PARM1)-1;
	skelobject_t *skelobj = skel_get(w, skelidx);
	if (!skelobj || (unsigned int)boneidx >= skelobj->numbones)
		bonematident_toqcvectors(w->g.v_forward, w->g.v_right, w->g.v_up, G_VECTOR(OFS_RETURN));
	else if (skelobj->type!=SKEL_RELATIVE)
	{
		float tmp[12];
		float invparent[12];
		int parent;
		/*invert the parent, multiply that against the child, we now know the transform required to go from parent to child. woo.*/
		parent = Mod_GetBoneParent(skelobj->model, boneidx+1)-1;
		Matrix3x4_Invert(skelobj->bonematrix+12*parent, invparent);
		Matrix3x4_Multiply(invparent, skelobj->bonematrix+12*boneidx, tmp);
		bonemat_toqcvectors(tmp, w->g.v_forward, w->g.v_right, w->g.v_up, G_VECTOR(OFS_RETURN));
	}
	else
		bonemat_toqcvectors(skelobj->bonematrix+12*boneidx, w->g.v_forward, w->g.v_right, w->g.v_up, G_VECTOR(OFS_RETURN));
}

//vector(float skel, float bonenum) skel_get_boneabs (FTE_CSQC_SKELETONOBJECTS) (sets v_forward etc)
void QCBUILTIN PF_skel_get_boneabs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	int boneidx = G_FLOAT(OFS_PARM1)-1;
	float workingm[12], tempmatrix[3][4];
	int i;
	skelobject_t *skelobj = skel_get(w, skelidx);

	if (!skelobj || (unsigned int)boneidx >= skelobj->numbones)
		bonematident_toqcvectors(w->g.v_forward, w->g.v_right, w->g.v_up, G_VECTOR(OFS_RETURN));
	else if (skelobj->type != SKEL_RELATIVE)
	{
		//can just copy it out
		bonemat_toqcvectors(skelobj->bonematrix + boneidx*12, w->g.v_forward, w->g.v_right, w->g.v_up, G_VECTOR(OFS_RETURN));
	}
	else
	{
		//we need to work out the abs position

		//testme

		//set up an identity matrix
		for (i = 0;i < 12;i++)
			workingm[i] = 0;
		workingm[0] = 1;
		workingm[5] = 1;
		workingm[10] = 1;

		while(boneidx >= 0)
		{
			//copy out the previous working matrix, so we don't stomp on it
			memcpy(tempmatrix, workingm, sizeof(tempmatrix));
			R_ConcatTransforms((void*)(skelobj->bonematrix + boneidx*12), (void*)tempmatrix, (void*)workingm);

			boneidx = Mod_GetBoneParent(skelobj->model, boneidx+1)-1;
		}
		bonemat_toqcvectors(workingm, w->g.v_forward, w->g.v_right, w->g.v_up, G_VECTOR(OFS_RETURN));
	}
}

//void(entity ent, float bonenum, vector org, optional fwd, right, up) skel_set_bone_world (FTE_CSQC_SKELETONOBJECTS2) (reads v_forward etc)
void QCBUILTIN PF_skel_set_bone_world (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	wedict_t *ent = G_WEDICT(prinst, OFS_PARM0);
	unsigned int boneidx = G_FLOAT(OFS_PARM1)-1;
	pvec_t *matrix[3];
	skelobject_t *skelobj;
	float *bone;
	float childworld[12], parentinv[12];

	/*sort out the parameters*/
	if (prinst->callargc == 4)
	{
		pvec3_t d[3], a;
		a[0] = G_VECTOR(OFS_PARM3)[0] * r_meshpitch.value; /*mod_alias bug*/
		a[1] = G_VECTOR(OFS_PARM3)[1];
		a[2] = G_VECTOR(OFS_PARM3)[2];
		AngleVectors(a, d[0], d[1], d[2]);
		bonemat_fromqcvectors(childworld, d[0], d[1], d[2], G_VECTOR(OFS_PARM2));
	}
	else
	{
		if (prinst->callargc > 5)
		{
			matrix[0] = G_VECTOR(OFS_PARM3);
			matrix[1] = G_VECTOR(OFS_PARM4);
			matrix[2] = G_VECTOR(OFS_PARM5);
		}
		else
		{
			matrix[0] = w->g.v_forward;
			matrix[1] = w->g.v_right;
			matrix[2] = w->g.v_up;
		}
		bonemat_fromqcvectors(childworld, matrix[0], matrix[1], matrix[2], G_VECTOR(OFS_PARM2));
	}

	/*make sure the skeletal object is correct*/
	skelobj = skel_get(w, ent->xv->skeletonindex);
	if (!skelobj || boneidx >= skelobj->numbones)
		return;

	/*get the inverse of the parent matrix*/
	{
		float parentabs[12];
		float parentw[12];
		float parentent[12];
		framestate_t fstate;
		w->Get_FrameState(w, ent, &fstate);
		if (skelobj->type == SKEL_ABSOLUTE || !Mod_GetTag(skelobj->model, Mod_GetBoneParent(skelobj->model, boneidx+1), &fstate, parentabs))
		{
			bonemat_fromentity(w, ent, parentw);
		}
		else
		{
			bonemat_fromentity(w, ent, parentent);
			Matrix3x4_Multiply(parentabs, parentent, parentw);
		}
		Matrix3x4_Invert(parentw, parentinv);
	}

	/*calc the result*/
	bone = skelobj->bonematrix+12*boneidx;
	Matrix3x4_Multiply(childworld, parentinv, bone);
}

//void(float skel, float bonenum, vector org) skel_set_bone (FTE_CSQC_SKELETONOBJECTS) (reads v_forward etc)
void QCBUILTIN PF_skel_set_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	unsigned int boneidx = G_FLOAT(OFS_PARM1)-1;
	float *matrix[3];
	skelobject_t *skelobj;
	float *bone;

	if (prinst->callargc > 5)
	{
		matrix[0] = G_VECTOR(OFS_PARM3);
		matrix[1] = G_VECTOR(OFS_PARM4);
		matrix[2] = G_VECTOR(OFS_PARM5);
	}
	else
	{
		matrix[0] = w->g.v_forward;
		matrix[1] = w->g.v_right;
		matrix[2] = w->g.v_up;
	}

	skelobj = skel_get(w, skelidx);
	if (!skelobj || boneidx >= skelobj->numbones)
		return;

	bone = skelobj->bonematrix+12*boneidx;
	bonemat_fromqcvectors(bone, matrix[0], matrix[1], matrix[2], G_VECTOR(OFS_PARM2));
}

//void(float skel, float bonenum, vector org [, vector fwd, vector right, vector up]) skel_mul_bone (FTE_CSQC_SKELETONOBJECTS) (reads v_forward etc)
void QCBUILTIN PF_skel_premul_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	int boneidx = G_FLOAT(OFS_PARM1)-1;
	float temp[3][4];
	float mult[3][4];
	skelobject_t *skelobj;
	if (prinst->callargc > 5)
		bonemat_fromqcvectors((float*)mult, G_VECTOR(OFS_PARM3), G_VECTOR(OFS_PARM4), G_VECTOR(OFS_PARM5), G_VECTOR(OFS_PARM2));
	else
		bonemat_fromqcvectors((float*)mult, w->g.v_forward, w->g.v_right, w->g.v_up, G_VECTOR(OFS_PARM2));

	skelobj = skel_get(w, skelidx);
	if (!skelobj || boneidx >= skelobj->numbones)
		return;

	//this is backwards. it rotates the EXISTING position around the passed position. this makes it hard to work with bones that have existing animation data (even if its a static transform).
	Vector4Copy(skelobj->bonematrix+12*boneidx+0, temp[0]);
	Vector4Copy(skelobj->bonematrix+12*boneidx+4, temp[1]);
	Vector4Copy(skelobj->bonematrix+12*boneidx+8, temp[2]);
	R_ConcatTransforms((void*)mult, (void*)temp, (float(*)[4])(skelobj->bonematrix+12*boneidx));
}
void QCBUILTIN PF_skel_postmul_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	int boneidx = G_FLOAT(OFS_PARM1)-1;
	float temp[3][4];
	float mult[3][4];
	skelobject_t *skelobj;
	if (prinst->callargc > 5)
		bonemat_fromqcvectors((float*)mult, G_VECTOR(OFS_PARM3), G_VECTOR(OFS_PARM4), G_VECTOR(OFS_PARM5), G_VECTOR(OFS_PARM2));
	else
		bonemat_fromqcvectors((float*)mult, w->g.v_forward, w->g.v_right, w->g.v_up, G_VECTOR(OFS_PARM2));

	skelobj = skel_get(w, skelidx);
	if (!skelobj || boneidx >= skelobj->numbones)
		return;

	Vector4Copy(skelobj->bonematrix+12*boneidx+0, temp[0]);
	Vector4Copy(skelobj->bonematrix+12*boneidx+4, temp[1]);
	Vector4Copy(skelobj->bonematrix+12*boneidx+8, temp[2]);
	R_ConcatTransforms((void*)temp, (void*)mult, (float(*)[4])(skelobj->bonematrix+12*boneidx));
}

//void(float skel, float startbone, float endbone, vector org) skel_mul_bone (FTE_CSQC_SKELETONOBJECTS) (reads v_forward etc)
void QCBUILTIN PF_skel_premul_bones (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	unsigned int startbone = G_FLOAT(OFS_PARM1)-1;
	unsigned int endbone = G_FLOAT(OFS_PARM2)-1;
	float temp[3][4];
	float mult[3][4];
	skelobject_t *skelobj;
	if (prinst->callargc > 6)
		bonemat_fromqcvectors((float*)mult, G_VECTOR(OFS_PARM4), G_VECTOR(OFS_PARM5), G_VECTOR(OFS_PARM6), G_VECTOR(OFS_PARM3));
	else
		bonemat_fromqcvectors((float*)mult, w->g.v_forward, w->g.v_right, w->g.v_up, G_VECTOR(OFS_PARM3));

	skelobj = skel_get(w, skelidx);
	if (!skelobj)
		return;

	if (startbone == -1)
		startbone = 0;
	if (endbone == -1)
		endbone = skelobj->numbones;
	else if (endbone > skelobj->numbones)
		endbone = skelobj->numbones;

	while(startbone < endbone)
	{
		Vector4Copy(skelobj->bonematrix+12*startbone+0, temp[0]);
		Vector4Copy(skelobj->bonematrix+12*startbone+4, temp[1]);
		Vector4Copy(skelobj->bonematrix+12*startbone+8, temp[2]);
		R_ConcatTransforms((void*)mult, (void*)temp, (float(*)[4])(skelobj->bonematrix+12*startbone));

		startbone++;
	}
}

//void(float skeldst, float skelsrc, float startbone, float entbone) skel_copybones (FTE_CSQC_SKELETONOBJECTS)
void QCBUILTIN PF_skel_copybones (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skeldst = G_FLOAT(OFS_PARM0);
	int skelsrc = G_FLOAT(OFS_PARM1);
	int startbone = G_FLOAT(OFS_PARM2)-1;
	int endbone = G_FLOAT(OFS_PARM3)-1;

	skelobject_t *skelobjdst;
	skelobject_t *skelobjsrc;

	skelobjdst = skel_get(w, skeldst);
	skelobjsrc = skel_get(w, skelsrc);
	if (!skelobjdst || !skelobjsrc)
		return;
	if (startbone == -1)
		startbone = 0;
	if (endbone == -1)
		endbone = skelobjdst->numbones;
	if (endbone > skelobjdst->numbones)
		endbone = skelobjdst->numbones;
	if (endbone > skelobjsrc->numbones)
		endbone = skelobjsrc->numbones;

	if (skelobjsrc->type == skelobjdst->type)
	{
		while(startbone < endbone)
		{
			Vector4Copy(skelobjsrc->bonematrix+12*startbone+0, skelobjdst->bonematrix+12*startbone+0);
			Vector4Copy(skelobjsrc->bonematrix+12*startbone+4, skelobjdst->bonematrix+12*startbone+4);
			Vector4Copy(skelobjsrc->bonematrix+12*startbone+8, skelobjdst->bonematrix+12*startbone+8);

			startbone++;
		}
	}
	else if (skelobjsrc->type == SKEL_RELATIVE && skelobjdst->type == SKEL_ABSOLUTE)
	{
		/*copy from relative to absolute*/
		skel_copy_toabs(skelobjdst, skelobjsrc, startbone, endbone);
	}
	else
	{
		/*copy from absolute to relative*/
		//FIXME
	}
}

//void(float skel) skel_delete (FTE_CSQC_SKELETONOBJECTS)
void QCBUILTIN PF_skel_delete (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int skelidx = G_FLOAT(OFS_PARM0);
	skelobject_t *skelobj;

	skelobj = skel_get(w, skelidx);
	if (skelobj)
	{
		skelobj->inuse = 2;	//2 means don't reuse yet.
		skelobj->modelindex = 0;
		skelobj->model = NULL;
		pendingkill = true;
	}
}

//vector(entity ent, float tag) gettaginfo (DP_MD3_TAGSINFO)
void QCBUILTIN PF_gettaginfo (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	wedict_t *ent = G_WEDICT(prinst, OFS_PARM0);
	int tagnum = G_FLOAT(OFS_PARM1);
	int tagent = ent->xv->tag_entity;
	int chain = 10;

	float transent[12];
	float transforms[12];
	float result[12];
	float result2[12];

	framestate_t fstate;

	w->Get_FrameState(w, ent, &fstate);
	if (!Mod_GetTag(w->Get_CModel(w, ent->v->modelindex), tagnum, &fstate, transforms))
		bonemat_fromidentity(transforms);

	bonemat_fromentity(w, ent, transent);
	R_ConcatTransforms((void*)transent, (void*)transforms, (void*)result);

	while (tagent && chain --> 0)
	{
		ent = PROG_TO_WEDICT(prinst, tagent);
		w->Get_FrameState(w, ent, &fstate);
		if (!Mod_GetTag(w->Get_CModel(w, ent->v->modelindex), tagnum, &fstate, transforms))
			bonemat_fromidentity(transforms);

		bonemat_fromentity(w, ent, transent);
		R_ConcatTransforms((void*)transforms, (void*)result, (void*)result2);
		R_ConcatTransforms((void*)transent, (void*)result2, (void*)result);

		tagent = ent->xv->tag_entity;
		tagnum = ent->xv->tag_index;
	}

	bonemat_toqcvectors(result, w->g.v_forward, w->g.v_right, w->g.v_up, G_VECTOR(OFS_RETURN));

/*	//extra info for dp compat.
	gettaginfo_parent = parentofbone(tagnum);
	gettaginfo_name = nameofbone(tagnum);
	bonemat_toqcvectors(relbonetransform(fstate, tagnum), gettaginfo_forward, gettaginfo_right, gettaginfo_up, gettaginfo_offset);
*/
}

//writes to axis+origin. returns root entity.
wedict_t *skel_gettaginfo_args (pubprogfuncs_t *prinst, vec3_t axis[3], vec3_t origin, int tagent, int tagnum)
{
	world_t *w = prinst->parms->user;
	wedict_t *ent = NULL;
	int chain = 10;

	float transent[12];
	float transforms[12];
	float result[12];
	float result2[12];
	framestate_t fstate;

	bonemat_fromaxisorg(result, axis, origin);

	while (tagent && chain --> 0)
	{
		ent = PROG_TO_WEDICT(prinst, tagent);
		w->Get_FrameState(w, ent, &fstate);
		if (!Mod_GetTag(w->Get_CModel(w, ent->v->modelindex), tagnum, &fstate, transforms))
			bonemat_fromidentity(transforms);

		bonemat_fromentity(w, ent, transent);
		R_ConcatTransforms((void*)transforms, (void*)result, (void*)result2);
		R_ConcatTransforms((void*)transent, (void*)result2, (void*)result);

		tagent = ent->xv->tag_entity;
		tagnum = ent->xv->tag_index;
	}

	bonemat_toaxisorg(result, axis, origin);
	return ent;
}

//vector(entity ent, string tagname) gettagindex (DP_MD3_TAGSINFO)
void QCBUILTIN PF_gettagindex (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	wedict_t *ent = G_WEDICT(prinst, OFS_PARM0);
	const char *tagname = PR_GetStringOfs(prinst, OFS_PARM1);
	model_t *mod = *tagname?w->Get_CModel(w, ent->v->modelindex):NULL;
	if (mod)
		G_FLOAT(OFS_RETURN) = Mod_TagNumForName(mod, tagname, 0);
	else
		G_FLOAT(OFS_RETURN) = 0;
}

//string(float modidx, float framenum) frametoname
void QCBUILTIN PF_frametoname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int modelindex = G_FLOAT(OFS_PARM0);
	unsigned int animnum = G_FLOAT(OFS_PARM1);
	int surfaceidx = 0;
	model_t *mod = w->Get_CModel(w, modelindex);
	const char *n = Mod_FrameNameForNum(mod, surfaceidx, animnum);

	if (n)
		RETURN_TSTRING(n);
	else
		G_INT(OFS_RETURN) = 0;	//null string (which is also empty in qc)
}

void QCBUILTIN PF_frameforname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int modelindex = G_FLOAT(OFS_PARM0);
	int surfaceidx = 0;
	const char *str = PF_VarString(prinst, 1, pr_globals);
	model_t *mod = w->Get_CModel(w, modelindex);

	if (mod)
		G_FLOAT(OFS_RETURN) = Mod_FrameNumForName(mod, surfaceidx, str);
	else
		G_FLOAT(OFS_RETURN) = -1;
}
void QCBUILTIN PF_frameforaction (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int modelindex = G_FLOAT(OFS_PARM0);
	int surfaceidx = 0;
	int actionid = G_INT(OFS_PARM1);
	model_t *mod = w->Get_CModel(w, modelindex);

	if (mod)
		G_FLOAT(OFS_RETURN) = Mod_FrameNumForAction(mod, surfaceidx, actionid);
	else
		G_FLOAT(OFS_RETURN) = -1;
}
void QCBUILTIN PF_frameduration (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int modelindex = G_FLOAT(OFS_PARM0);
	unsigned int framenum = G_FLOAT(OFS_PARM1);
	int surfaceidx = 0;
	model_t *mod = w->Get_CModel(w, modelindex);

	if (mod)
		G_FLOAT(OFS_RETURN) = Mod_GetFrameDuration(mod, surfaceidx, framenum);
	else
		G_FLOAT(OFS_RETURN) = 0;
}
void QCBUILTIN PF_modelframecount (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	unsigned int modelindex = G_FLOAT(OFS_PARM0);
	model_t *mod = w->Get_CModel(w, modelindex);

	if (mod)
		G_FLOAT(OFS_RETURN) = Mod_GetFrameCount(mod);
	else
		G_FLOAT(OFS_RETURN) = 0;
}

//void(float modidx, float framenum, __inout float basetime, float targettime, void(float timestamp, int code, string data) callback)
void QCBUILTIN PF_processmodelevents (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	int modelindex = G_FLOAT(OFS_PARM0);
	unsigned int frame = G_FLOAT(OFS_PARM1);
	float basetime = G_FLOAT(OFS_PARM2);
	float targettime = G_FLOAT(OFS_PARM3);
	func_t callback = G_INT(OFS_PARM4);
	model_t *mod = w->Get_CModel(w, modelindex);
	float starttime, timestamp;

	if (targettime == basetime)
		return;	//don't refire the same event multiple times.

	//returns all basetime <= eventtime < targettime

	if (mod)
	{
		if (mod->type == mod_alias)
		{	//slightly more optimised path that is kinda redundant, but w/e
			galiasinfo_t *ga = Mod_Extradata(mod);
			galiasanimation_t *anim = ga->ofsanimations + frame;
			galiasevent_t *ev;
			float loopduration;
			if (frame < (unsigned int)ga->numanimations && anim->events)
			{
				if (anim->loop)
				{
					loopduration = anim->rate * anim->numposes;
					starttime = loopduration*(unsigned int)(basetime/loopduration);
				}
				else
					starttime = loopduration = 0;
				for (ev = anim->events; ; )
				{
					//be careful to use as consistent timings as we can
					timestamp = starttime + ev->timestamp;
					if (timestamp >= targettime)
						break;	//this is in the future.
					if (timestamp >= basetime)
					{
						G_FLOAT(OFS_PARM0) = timestamp;
						G_INT(OFS_PARM1) = ev->code;
						G_INT(OFS_PARM2) = PR_TempString(prinst, ev->data);
						PR_ExecuteProgram(prinst, callback);
					}

					ev = ev->next;
					if (!ev)
					{
						if (loopduration)
							ev = anim->events;
						else
							break;	//animation ends here, so no more events possible
						starttime += loopduration;
					}
				}
			}
		}
#ifdef HALFLIFEMODELS
		else	//actually this is a generic version that would work for iqm etc too, but is less efficient due to repeated lookups. oh well.
		{
			int ev, code;
			char *data;
			float loopduration;
			qboolean looping;
			int act;

			if (Mod_FrameInfoForNum(mod, 0, frame, &data, &code, &loopduration, &looping, &act))
			{
				if (looping && loopduration)
					starttime = loopduration*(unsigned int)(basetime/loopduration);
				else
					starttime = loopduration = 0;
				for (ev = 0; ; ev++)
				{
					if (!Mod_GetModelEvent(mod, frame, ev, &timestamp, &code, &data))
					{
						if (looping && Mod_GetModelEvent(mod, frame, 0, &timestamp, &code, &data))
						{
							ev = 0;
							starttime += loopduration;
						}
						else
							break;	//end of anim
					}

					//be careful to use as consistent timings as we can...
					timestamp += starttime;
					if (timestamp >= targettime)
						break;	//this is in the future.
					if (timestamp >= basetime)
					{
						G_FLOAT(OFS_PARM0) = timestamp;
						G_INT(OFS_PARM1) = code;
						G_INT(OFS_PARM2) = PR_TempString(prinst, data);
						PR_ExecuteProgram(prinst, callback);
					}
				}
			}
		}
#endif
	}
	G_FLOAT(OFS_PARM2) = targettime;
}

//float(float modidx, float framenum, __inout float basetime, float targettime, __out int code, __out string data)
void QCBUILTIN PF_getnextmodelevent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	unsigned int modelindex = G_FLOAT(OFS_PARM0);
	unsigned int frame = G_FLOAT(OFS_PARM1);
	float basetime = G_FLOAT(OFS_PARM2);
	float targettime = G_FLOAT(OFS_PARM3);
	model_t *mod = w->Get_CModel(w, modelindex);
	float starttime, timestamp;
	//default return values
	G_FLOAT(OFS_RETURN) = false;
	G_FLOAT(OFS_PARM2) = targettime;
	G_INT(OFS_PARM4) = 0;
	G_INT(OFS_PARM5) = 0;

	if (mod)
	{
		if (mod->type == mod_alias)
		{	//slightly more optimised path that is kinda redundant, but w/e
			galiasinfo_t *ga = Mod_Extradata(mod);
			galiasanimation_t *anim = ga->ofsanimations + frame;
			galiasevent_t *ev;
			float loopduration;
			if (frame >= (unsigned int)ga->numanimations || !anim->events)
				return;
			if (anim->loop)
			{
				loopduration = anim->rate * anim->numposes;
				starttime = loopduration*(unsigned int)(basetime/loopduration);
			}
			else
				starttime = loopduration = 0;
			for (ev = anim->events; ; )
			{
				//be careful to use as consistent timings as we can
				timestamp = starttime + ev->timestamp;
				if (timestamp > targettime)
					break;	//this is in the future.
				if (timestamp > basetime)
				{
					G_FLOAT(OFS_RETURN) = true;
					G_FLOAT(OFS_PARM2) = timestamp;
					G_INT(OFS_PARM4) = ev->code;
					G_INT(OFS_PARM5) = PR_TempString(prinst, ev->data);
					return;
				}

				ev = ev->next;
				if (!ev)
				{
					if (loopduration)
						ev = anim->events;
					else
						return;	//animation ended here, so no more events
					starttime += loopduration;
				}
			}
		}
#ifdef HALFLIFEMODELS
		else	//actually this is a generic version that would work for iqm etc too, but is less efficient due to repeated lookups. oh well.
		{
			int ev, code;
			char *data;
			float loopduration;
			qboolean looping;
			int act;

			if (!Mod_FrameInfoForNum(mod, 0, frame, &data, &code, &loopduration, &looping, &act))
				return; //invalid frame

			if (looping && loopduration)
				starttime = loopduration*(unsigned int)(basetime/loopduration);
			else
				starttime = loopduration = 0;
			for (ev = 0; ; ev++)
			{
				if (!Mod_GetModelEvent(mod, frame, ev, &timestamp, &code, &data))
				{
					if (looping && Mod_GetModelEvent(mod, frame, 0, &timestamp, &code, &data))
					{
						ev = 0;
						starttime += loopduration;
					}
					else
						break;	//end of anim
				}

				//be careful to use as consistent timings as we can...
				timestamp += starttime;
				if (timestamp > targettime)
					break;	//this is in the future.
				if (timestamp > basetime)
				{
					G_FLOAT(OFS_RETURN) = true;
					G_FLOAT(OFS_PARM2) = timestamp;
					G_INT(OFS_PARM4) = code;
					G_INT(OFS_PARM5) = PR_TempString(prinst, data);
					return;
				}
			}
		}
#endif
	}
}
//float(float modidx, float framenum, int idx, __out float timestamp, __out int code, __out string data)
void QCBUILTIN PF_getmodeleventidx (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	unsigned int modelindex = G_FLOAT(OFS_PARM0);
	unsigned int frame = G_FLOAT(OFS_PARM1);
	int eventindex = G_INT(OFS_PARM2);
	model_t *mod = w->Get_CModel(w, modelindex);
	//default return values
	float timestamp = 0;
	int code = 0;
	char *data = NULL;

	G_FLOAT(OFS_RETURN) = Mod_GetModelEvent(mod, frame, eventindex, &timestamp, &code, &data);
	G_FLOAT(OFS_PARM3) = timestamp;
	G_INT(OFS_PARM4) = code;
	if (data)
		G_INT(OFS_PARM5) = PR_TempString(prinst, data);
	else
		G_INT(OFS_PARM5) = 0;
}

//string(float modidx, float skinnum) skintoname
void QCBUILTIN PF_skintoname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	world_t *w = prinst->parms->user;
	unsigned int modelindex = G_FLOAT(OFS_PARM0);
	unsigned int skinnum = G_FLOAT(OFS_PARM1);
	int surfaceidx = 0;
	model_t *mod = w->Get_CModel(w, modelindex);
	const char *n = Mod_SkinNameForNum(mod, surfaceidx, skinnum);

	if (n)
		RETURN_TSTRING(n);
	else
		G_INT(OFS_RETURN) = 0;	//null string (which is also empty in qc)
}
void QCBUILTIN PF_skinforname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifndef SERVERONLY
	world_t *w = prinst->parms->user;
	unsigned int modelindex = G_FLOAT(OFS_PARM0);
	const char *str = PF_VarString(prinst, 1, pr_globals);
	int surfaceidx = 0;
	model_t *mod = w->Get_CModel(w, modelindex);

	if (mod)
		G_FLOAT(OFS_RETURN) = Mod_SkinNumForName(mod, surfaceidx, str);
	else
#endif
		G_FLOAT(OFS_RETURN) = -1;
}
#endif

