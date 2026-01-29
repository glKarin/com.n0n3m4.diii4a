#include "quakedef.h"

#include "com_bih.h"
#include "com_mesh.h"

#ifdef __F16C__
#ifdef _MSC_VER
	#include <intrin.h>
#else
	#include <x86intrin.h>
#endif
#endif

//#include <x86intrin.h>

//small helper to aid compiling.
#if defined(SKELETALMODELS) || defined(MD3MODELS)
	#define SKELORTAGS
#endif

qboolean		r_loadbumpmapping;
extern cvar_t r_noframegrouplerp;
cvar_t r_lerpmuzzlehack						= CVARF  ("r_lerpmuzzlehack", "1", CVAR_ARCHIVE);
#ifdef MD1MODELS
cvar_t mod_h2holey_bugged					= CVARD ("mod_h2holey_bugged", "0", "Hexen2's holey-model flag uses index 0 as transparent (and additionally 255 in gl, due to a bug). GLQuake engines tend to have bugs that use ONLY index 255, resulting in a significant compatibility issue that can be resolved only with this shitty cvar hack.");
cvar_t mod_halftexel						= CVARD ("mod_halftexel", "1", "Offset texture coords by a half-texel, for compatibility with glquake and the majority of engine forks.");
cvar_t mod_nomipmap							= CVARD ("mod_nomipmap", "0", "Disables the use of mipmaps on quake1 mdls, consistent with its original software renderer.");
#endif
#ifdef MODELFMT_OBJ
cvar_t mod_obj_orientation					= CVARD("mod_obj_orientation", "1", "Controls how the model's axis are interpreted.\n0: x=forward, z=up (Quake)\n1: x=forward, y=up\n2: z=forward, y=up");
#endif
#ifdef MD5MODELS
cvar_t mod_md5_singleanimation				= CVARD("mod_md5_singleanimation", "1", "When loading an md5mesh file, also attempt to load an .md5anim file, and unpack it into individual poses. Use 0 for mods that will be precaching their own md5anims for use with skeletal objects.");
#endif
static void QDECL r_meshpitch_callback(cvar_t *var, char *oldvalue)
{
	if (!strcmp(var->string, "-1") || !strcmp(var->string, "1"))
		return;
	if (var->value <= 0)
		Cvar_ForceSet(var, "-1");
	else
		Cvar_ForceSet(var, "1");
}
#ifndef HAVE_LEGACY
cvar_t r_meshpitch							= CVARCD	("r_meshpitch", "1", r_meshpitch_callback, "Specifies the direction of the pitch angle on mesh models formats, also affects gamecode, so do not change from its default.");
#else
cvar_t r_meshpitch							= CVARCD	("r_meshpitch", "-1", r_meshpitch_callback, "Specifies the direction of the pitch angle on mesh models formats, Quake compatibility requires -1.");
#endif
cvar_t r_meshroll							= CVARCD	("r_meshroll", "1", r_meshpitch_callback, "Specifies the direction of the roll angle on mesh models formats, also affects gamecode, so do not change from its default.");
cvar_t dpcompat_skinfiles					= CVARD	("dpcompat_skinfiles", "0", "When set, uses a nodraw shader for any unmentioned surfaces.");

#ifdef HAVE_CLIENT
static void Mod_UpdateCRC(void *ctx, void *data, size_t a, size_t b)
{
	char st[40];
	Q_snprintfz(st, sizeof(st), "%d", (int) a);
	if (strcmp(st, InfoBuf_ValueForKey(&cls.userinfo[0], ctx)))
	{
		InfoBuf_SetKey(&cls.userinfo[0], ctx, st);
		if (cls.state >= ca_connected && (cls.protocol == CP_QUAKEWORLD || (cls.fteprotocolextensions2 & PEXT2_PREDINFO)))
			CL_SendClientCommand(true, "setinfo %s %s", (char*)ctx, st);
	}
}
#endif

//Common loader function.
qboolean Mod_DoCRC(model_t *mod, char *buffer, int buffersize)
{
#ifdef HAVE_CLIENT
	//we've got to have this bit
	if (mod->engineflags & MDLF_DOCRC)
	{
		unsigned int crc = CalcHashInt(&hash_crc16, buffer, buffersize);
		if (!(mod->engineflags & MDLF_PLAYER))
		{	//eyes
			mod->tainted = (crc != 6967);
		}
		COM_AddWork(WG_MAIN, Mod_UpdateCRC, (mod->engineflags & MDLF_PLAYER) ? pmodel_name : emodel_name, NULL, crc, 0);
	}
	return Ruleset_FileLoaded(mod->publicname, buffer, buffersize);
#else
	return true;
#endif
}

extern cvar_t gl_part_flame, r_fullbrightSkins, r_fb_models;
extern cvar_t r_noaliasshadows;
//extern cvar_t r_skin_overlays;
extern cvar_t mod_md3flags;


#ifdef HAVE_CLIENT
typedef struct
{
	char *name;
	float furthestallowedextremety;	//this field is the combined max-min square, added together
									//note that while this allows you to move models about a little, you cannot resize the visible part
} clampedmodel_t;

//these should be rounded up slightly.
//really this is only to catch spiked models. This doesn't prevent more visible models, just bigger ones.
static clampedmodel_t clampedmodel[] = {
	{"maps/b_bh100.bsp", 3440},
	{"progs/player.mdl", 22497},
	{"progs/eyes.mdl", 755},
	{"progs/gib1.mdl", 374},
	{"progs/gib2.mdl", 1779},
	{"progs/gib3.mdl", 2066},
	{"progs/bolt2.mdl", 1160},
	{"progs/end1.mdl", 764},
	{"progs/end2.mdl", 981},
	{"progs/end3.mdl", 851},
	{"progs/end4.mdl", 903},
	{"progs/g_shot.mdl", 3444},
	{"progs/g_nail.mdl", 2234},
	{"progs/g_nail2.mdl", 3660},
	{"progs/g_rock.mdl", 3441},
	{"progs/g_rock2.mdl", 3660},
	{"progs/g_light.mdl", 2698},
	{"progs/invisibl.mdl", 196},
	{"progs/quaddama.mdl", 2353},
	{"progs/invulner.mdl", 2746},
	{"progs/suit.mdl", 3057},
	{"progs/missile.mdl", 416},
	{"progs/grenade.mdl", 473},
	{"progs/spike.mdl", 112},
	{"progs/s_spike.mdl", 112},
	{"progs/backpack.mdl", 1117},
	{"progs/armor.mdl", 2919},
	{"progs/s_bubble.spr", 100},
	{"progs/s_explod.spr", 1000},

	//and now TF models
#ifdef warningmsg
#pragma warningmsg("FIXME: these are placeholders")
#endif
	{"progs/disp.mdl", 3000},
	{"progs/tf_flag.mdl", 3000},
	{"progs/tf_stan.mdl", 3000},
	{"progs/turrbase.mdl", 3000},
	{"progs/turrgun.mdl", 3000}
};
#endif








void QDECL Mod_AccumulateTextureVectors(vecV_t *const vc, vec2_t *const tc, vec3_t *nv, vec3_t *sv, vec3_t *tv, const index_t *idx, int numidx, qboolean calcnorms)
{
	int i;
	const float *v0, *v1, *v2;
	const float *tc0, *tc1, *tc2;

	vec3_t d1, d2;
	float td1, td2;

	vec3_t norm, t, s;
	vec3_t temp;

	for (i = 0; i < numidx; i += 3)
	{
		//this is the stuff we're working from
		v0 = vc[idx[i+0]];
		v1 = vc[idx[i+1]];
		v2 = vc[idx[i+2]];
		tc0 = tc[idx[i+0]];
		tc1 = tc[idx[i+1]];
		tc2 = tc[idx[i+2]];

		//calc perpendicular directions
		VectorSubtract(v1, v0, d1);
		VectorSubtract(v2, v0, d2);

		//calculate s as the pependicular of the t dir
		td1 = tc1[1] - tc0[1];
		td2 = tc2[1] - tc0[1];
		s[0] = td1 * d2[0] - td2 * d1[0];
		s[1] = td1 * d2[1] - td2 * d1[1];
		s[2] = td1 * d2[2] - td2 * d1[2];

		//calculate t as the pependicular of the s dir
		td1 = tc1[0] - tc0[0];
		td2 = tc2[0] - tc0[0];
		t[0] = td1 * d2[0] - td2 * d1[0];
		t[1] = td1 * d2[1] - td2 * d1[1];
		t[2] = td1 * d2[2] - td2 * d1[2];

		//the surface might be a back face and thus textured backwards
		//calc the normal twice and compare.
		norm[0] = d2[1] * d1[2] - d2[2] * d1[1];
		norm[1] = d2[2] * d1[0] - d2[0] * d1[2];
		norm[2] = d2[0] * d1[1] - d2[1] * d1[0];
		CrossProduct(t, s, temp);
		if (DotProduct(temp, norm) < 0)
		{
			VectorNegate(s, s);
			VectorNegate(t, t);
		}

		//and we're done, accumulate the result
		if (calcnorms)
		{
			VectorAdd(nv[idx[i+0]], norm, nv[idx[i+0]]);
			VectorAdd(nv[idx[i+1]], norm, nv[idx[i+1]]);
			VectorAdd(nv[idx[i+2]], norm, nv[idx[i+2]]);
		}

		VectorAdd(sv[idx[i+0]], s, sv[idx[i+0]]);
		VectorAdd(sv[idx[i+1]], s, sv[idx[i+1]]);
		VectorAdd(sv[idx[i+2]], s, sv[idx[i+2]]);

		VectorAdd(tv[idx[i+0]], t, tv[idx[i+0]]);
		VectorAdd(tv[idx[i+1]], t, tv[idx[i+1]]);
		VectorAdd(tv[idx[i+2]], t, tv[idx[i+2]]);
	}
}

void Mod_AccumulateMeshTextureVectors(mesh_t *m)
{
	Mod_AccumulateTextureVectors(m->xyz_array, m->st_array, m->normals_array, m->snormals_array, m->tnormals_array, m->indexes, m->numindexes, false);
}

void QDECL Mod_NormaliseTextureVectors(vec3_t *n, vec3_t *s, vec3_t *t, int v, qboolean calcnorms)
{
	int i;
	float f;
	vec3_t tmp;

	for (i = 0; i < v; i++)
	{
		if (calcnorms)
			VectorNormalize(n[i]);

		//strip away any variance against the normal to keep it perpendicular, then normalize
		f = -DotProduct(s[i], n[i]);
		VectorMA(s[i], f, n[i], tmp);
		VectorNormalize2(tmp, s[i]);

		f = -DotProduct(t[i], n[i]);
		VectorMA(t[i], f, n[i], tmp);
		VectorNormalize2(tmp, t[i]);
	}
}



#ifdef SKELETALMODELS

/*like GenMatrixPosQuat4Scale, but guess the quat.w*/
static void GenMatrixPosQuat3Scale(vec3_t const pos, vec3_t const quat3, vec3_t const scale, float result[12])
{
	vec4_t quat4;
	float term = 1 - DotProduct(quat3, quat3);
	if (term < 0)
		quat4[3] = 0;
	else
		quat4[3] = - (float) sqrt(term);
	VectorCopy(quat3, quat4);
	GenMatrixPosQuat4Scale(pos, quat4, scale, result);
}

#ifdef MD5MODELS
static void GenMatrix(float x, float y, float z, float qx, float qy, float qz, float result[12])
{
	float qw;
	{	//figure out qw
		float term = 1 - (qx*qx) - (qy*qy) - (qz*qz);
		if (term < 0)
			qw = 0;
		else
			qw = - (float) sqrt(term);
	}

	{	//generate the matrix
		/*
		float xx      = qx * qx;
		float xy      = qx * qy;
		float xz      = qx * qz;
		float xw      = qx * qw;
		float yy      = qy * qy;
		float yz      = qy * qz;
		float yw      = qy * qw;
		float zz      = qz * qz;
		float zw      = qz * qw;
		result[0*4+0]  = 1 - 2 * ( yy + zz );
		result[0*4+1]  =     2 * ( xy - zw );
		result[0*4+2]  =     2 * ( xz + yw );
		result[0*4+3]  =     x;
		result[1*4+0]  =     2 * ( xy + zw );
		result[1*4+1]  = 1 - 2 * ( xx + zz );
		result[1*4+2]  =     2 * ( yz - xw );
		result[1*4+3]  =     y;
		result[2*4+0]  =     2 * ( xz - yw );
		result[2*4+1]  =     2 * ( yz + xw );
		result[2*4+2] = 1 - 2 * ( xx + yy );
		result[2*4+3]  =     z;
		*/

		   float xx, xy, xz, xw, yy, yz, yw, zz, zw;
		   float x2, y2, z2;
		   x2 = qx + qx;
		   y2 = qy + qy;
		   z2 = qz + qz;

		   xx = qx * x2;   xy = qx * y2;   xz = qx * z2;
		   yy = qy * y2;   yz = qy * z2;   zz = qz * z2;
		   xw = qw * x2;   yw = qw * y2;   zw = qw * z2;

		   result[0*4+0] = 1.0f - (yy + zz);
		   result[1*4+0] = xy + zw;
		   result[2*4+0] = xz - yw;

		   result[0*4+1] = xy - zw;
		   result[1*4+1] = 1.0f - (xx + zz);
		   result[2*4+1] = yz + xw;

		   result[0*4+2] = xz + yw;
		   result[1*4+2] = yz - xw;
		   result[2*4+2] = 1.0f - (xx + yy);

		   result[0*4+3]  =     x;
		   result[1*4+3]  =     y;
		   result[2*4+3]  =     z;
	}
}
#endif

#ifdef PSKMODELS
static void PSKGenMatrix(float x, float y, float z, float qx, float qy, float qz, float qw, float result[12])
{
	float xx, xy, xz, xw, yy, yz, yw, zz, zw;
	float x2, y2, z2;
	x2 = qx + qx;
	y2 = qy + qy;
	z2 = qz + qz;

	xx = qx * x2;   xy = qx * y2;   xz = qx * z2;
	yy = qy * y2;   yz = qy * z2;   zz = qz * z2;
	xw = qw * x2;   yw = qw * y2;   zw = qw * z2;

	result[0*4+0] = 1.0f - (yy + zz);
	result[1*4+0] = xy + zw;
	result[2*4+0] = xz - yw;

	result[0*4+1] = xy - zw;
	result[1*4+1] = 1.0f - (xx + zz);
	result[2*4+1] = yz + xw;

	result[0*4+2] = xz + yw;
	result[1*4+2] = yz - xw;
	result[2*4+2] = 1.0f - (xx + yy);

	result[0*4+3]  =     x;
	result[1*4+3]  =     y;
	result[2*4+3]  =     z;
}
#endif

/*transforms some skeletal vecV_t values*/
static void Alias_TransformVerticies_V(const float *bonepose, int vertcount, boneidx_t *bidx, float *weights, float *xyzin, float *fte_restrict xyzout)
{
#if 1
	int i, j;
	const float *matrix, *matrix1;
	float mat[12];
	for (i = 0; i < vertcount; i++, bidx+=4, weights+=4)
	{
		matrix = &bonepose[12*bidx[0]];
		if (weights[1])
		{
			matrix1 = &bonepose[12*bidx[1]];
			for (j = 0; j < 12; j++)
				mat[j] = (weights[0] * matrix[j]) + (weights[1] * matrix1[j]);
			if (weights[2])
			{
				matrix = &bonepose[12*bidx[2]];
				for (j = 0; j < 12; j++)
					mat[j] += weights[2] * matrix[j];
				if (weights[3])
				{
					matrix = &bonepose[12*bidx[3]];
					for (j = 0; j < 12; j++)
						mat[j] += weights[3] * matrix[j];
				}
			}
			matrix = mat;
		}
		//NOTE: else we assume that weights[0] is 1.

		xyzout[0] = (xyzin[0] * matrix[0] + xyzin[1] * matrix[1] + xyzin[2] * matrix[ 2] + matrix[ 3]);
		xyzout[1] = (xyzin[0] * matrix[4] + xyzin[1] * matrix[5] + xyzin[2] * matrix[ 6] + matrix[ 7]);
		xyzout[2] = (xyzin[0] * matrix[8] + xyzin[1] * matrix[9] + xyzin[2] * matrix[10] + matrix[11]);
		xyzout+=sizeof(vecV_t)/sizeof(vec_t);
		xyzin+=sizeof(vecV_t)/sizeof(vec_t);
	}
#else
	int i;
	const float *matrix;
	for (i = 0; i < vertcount; i++, bidx+=4, weights+=4)
	{
		matrix = &bonepose[12*bidx[0]];
		xyzout[0] = weights[0] * (xyzin[0] * matrix[0] + xyzin[1] * matrix[1] + xyzin[2] * matrix[ 2] + matrix[ 3]);
		xyzout[1] = weights[0] * (xyzin[0] * matrix[4] + xyzin[1] * matrix[5] + xyzin[2] * matrix[ 6] + matrix[ 7]);
		xyzout[2] = weights[0] * (xyzin[0] * matrix[8] + xyzin[1] * matrix[9] + xyzin[2] * matrix[10] + matrix[11]);

		if (weights[1])
		{
			matrix = &bonepose[12*bidx[1]];
			xyzout[0] += weights[1] * (xyzin[0] * matrix[0] + xyzin[1] * matrix[1] + xyzin[2] * matrix[ 2] + matrix[ 3]);
			xyzout[1] += weights[1] * (xyzin[0] * matrix[4] + xyzin[1] * matrix[5] + xyzin[2] * matrix[ 6] + matrix[ 7]);
			xyzout[2] += weights[1] * (xyzin[0] * matrix[8] + xyzin[1] * matrix[9] + xyzin[2] * matrix[10] + matrix[11]);

			if (weights[2])
			{
				matrix = &bonepose[12*bidx[2]];
				xyzout[0] += weights[2] * (xyzin[0] * matrix[0] + xyzin[1] * matrix[1] + xyzin[2] * matrix[ 2] + matrix[ 3]);
				xyzout[1] += weights[2] * (xyzin[0] * matrix[4] + xyzin[1] * matrix[5] + xyzin[2] * matrix[ 6] + matrix[ 7]);
				xyzout[2] += weights[2] * (xyzin[0] * matrix[8] + xyzin[1] * matrix[9] + xyzin[2] * matrix[10] + matrix[11]);

				if (weights[3])
				{
					matrix = &bonepose[12*bidx[3]];
					xyzout[0] += weights[3] * (xyzin[0] * matrix[0] + xyzin[1] * matrix[1] + xyzin[2] * matrix[ 2] + matrix[ 3]);
					xyzout[1] += weights[3] * (xyzin[0] * matrix[4] + xyzin[1] * matrix[5] + xyzin[2] * matrix[ 6] + matrix[ 7]);
					xyzout[2] += weights[3] * (xyzin[0] * matrix[8] + xyzin[1] * matrix[9] + xyzin[2] * matrix[10] + matrix[11]);
				}
			}
		}
		xyzout+=sizeof(vecV_t)/sizeof(vec_t);
		xyzin+=sizeof(vecV_t)/sizeof(vec_t);
	}
#endif
}

/*transforms some skeletal vecV_t values*/
static void Alias_TransformVerticies_VN(const float *bonepose, int vertcount, const boneidx_t *bidx, float *weights,
										const float *xyzin, float *fte_restrict xyzout,
										const float *normin, float *fte_restrict normout)
{
	int i, j;
	const float *matrix, *matrix1;
	float mat[12];
	for (i = 0; i < vertcount; i++, 
		xyzout+=sizeof(vecV_t)/sizeof(vec_t), xyzin+=sizeof(vecV_t)/sizeof(vec_t),
		normout+=sizeof(vec3_t)/sizeof(vec_t), normin+=sizeof(vec3_t)/sizeof(vec_t),
		bidx+=4, weights+=4)
	{
		matrix = &bonepose[12*bidx[0]];
		if (weights[1])
		{
			matrix1 = &bonepose[12*bidx[1]];
			for (j = 0; j < 12; j++)
				mat[j] = (weights[0] * matrix[j]) + (weights[1] * matrix1[j]);
			if (weights[2])
			{
				matrix = &bonepose[12*bidx[2]];
				for (j = 0; j < 12; j++)
					mat[j] += weights[2] * matrix[j];
				if (weights[3])
				{
					matrix = &bonepose[12*bidx[3]];
					for (j = 0; j < 12; j++)
						mat[j] += weights[3] * matrix[j];
				}
			}
			matrix = mat;
		}

		xyzout[0] = (xyzin[0] * matrix[0] + xyzin[1] * matrix[1] + xyzin[2] * matrix[ 2] + matrix[ 3]);
		xyzout[1] = (xyzin[0] * matrix[4] + xyzin[1] * matrix[5] + xyzin[2] * matrix[ 6] + matrix[ 7]);
		xyzout[2] = (xyzin[0] * matrix[8] + xyzin[1] * matrix[9] + xyzin[2] * matrix[10] + matrix[11]);

		normout[0] = (normin[0] * matrix[0] + normin[1] * matrix[1] + normin[2] * matrix[ 2]);
		normout[1] = (normin[0] * matrix[4] + normin[1] * matrix[5] + normin[2] * matrix[ 6]);
		normout[2] = (normin[0] * matrix[8] + normin[1] * matrix[9] + normin[2] * matrix[10]);
	}
}

/*transforms some skeletal vecV_t values*/
static void Alias_TransformVerticies_VNST(const float *bonepose, int vertcount, const boneidx_t *bidx, const float *weights,
										const float *xyzin, float *fte_restrict xyzout,
										const float *normin, float *fte_restrict normout,
										const float *sdirin, float *fte_restrict sdirout,
										const float *tdirin, float *fte_restrict tdirout)
{
	int i, j;
	const float *matrix, *matrix1;
	float mat[12];
	for (i = 0; i < vertcount; i++, bidx+=4, weights+=4)
	{
		matrix = &bonepose[12*bidx[0]];
		if (weights[1])
		{
			matrix1 = &bonepose[12*bidx[1]];
			for (j = 0; j < 12; j++)
				mat[j] = (weights[0] * matrix[j]) + (weights[1] * matrix1[j]);
			if (weights[2])
			{
				matrix = &bonepose[12*bidx[2]];
				for (j = 0; j < 12; j++)
					mat[j] += weights[2] * matrix[j];
				if (weights[3])
				{
					matrix = &bonepose[12*bidx[3]];
					for (j = 0; j < 12; j++)
						mat[j] += weights[3] * matrix[j];
				}
			}
			matrix = mat;
		}

		xyzout[0] = (xyzin[0] * matrix[0] + xyzin[1] * matrix[1] + xyzin[2] * matrix[ 2] + matrix[ 3]);
		xyzout[1] = (xyzin[0] * matrix[4] + xyzin[1] * matrix[5] + xyzin[2] * matrix[ 6] + matrix[ 7]);
		xyzout[2] = (xyzin[0] * matrix[8] + xyzin[1] * matrix[9] + xyzin[2] * matrix[10] + matrix[11]);
		xyzout+=sizeof(vecV_t)/sizeof(vec_t);
		xyzin+=sizeof(vecV_t)/sizeof(vec_t);

		normout[0] = (normin[0] * matrix[0] + normin[1] * matrix[1] + normin[2] * matrix[ 2]);
		normout[1] = (normin[0] * matrix[4] + normin[1] * matrix[5] + normin[2] * matrix[ 6]);
		normout[2] = (normin[0] * matrix[8] + normin[1] * matrix[9] + normin[2] * matrix[10]);
		normout+=sizeof(vec3_t)/sizeof(vec_t);
		normin+=sizeof(vec3_t)/sizeof(vec_t);

		sdirout[0] = (sdirin[0] * matrix[0] + sdirin[1] * matrix[1] + sdirin[2] * matrix[ 2]);
		sdirout[1] = (sdirin[0] * matrix[4] + sdirin[1] * matrix[5] + sdirin[2] * matrix[ 6]);
		sdirout[2] = (sdirin[0] * matrix[8] + sdirin[1] * matrix[9] + sdirin[2] * matrix[10]);
		sdirout+=sizeof(vec3_t)/sizeof(vec_t);
		sdirin+=sizeof(vec3_t)/sizeof(vec_t);

		tdirout[0] = (tdirin[0] * matrix[0] + tdirin[1] * matrix[1] + tdirin[2] * matrix[ 2]);
		tdirout[1] = (tdirin[0] * matrix[4] + tdirin[1] * matrix[5] + tdirin[2] * matrix[ 6]);
		tdirout[2] = (tdirin[0] * matrix[8] + tdirin[1] * matrix[9] + tdirin[2] * matrix[10]);
		tdirout+=sizeof(vec3_t)/sizeof(vec_t);
		tdirin+=sizeof(vec3_t)/sizeof(vec_t);
	}
}

//converts one entire frame to another skeleton type
//only writes to destbuffer if absolutely needed
static const float *Alias_ConvertBoneData(skeltype_t sourcetype, const float *sourcedata, size_t bonecount, galiasbone_t *bones, skeltype_t desttype, float *destbuffer, float *destbufferalt, size_t destbonecount)
{
	int i;
	if (sourcetype == desttype)
		return sourcedata;

	//everything can be converted up to SKEL_INVERSE_ABSOLUTE and back.
	//this means that everything can be converted to everything else, but it might take lots of individual transforms.
	//a->ia
	//r->a->ia
	//a->r
	//ia->ir
	//ir->ia
	//r->a->ia->ir
	//a->ia->ir

	if (bonecount > destbonecount || bonecount > MAX_BONES)
		Sys_Error("Alias_ConvertBoneData: too many bones %"PRIuSIZE">%"PRIuSIZE"\n", bonecount, destbonecount);

	//r(->a)->ia(->ir)
	if (desttype == SKEL_INVERSE_RELATIVE && sourcetype == SKEL_RELATIVE)
	{
		//for this conversion, we need absolute data.
		//this is not an efficient operation.
		sourcedata = Alias_ConvertBoneData(sourcetype, sourcedata, bonecount, bones, SKEL_ABSOLUTE, destbuffer, destbufferalt, destbonecount);
		sourcetype = SKEL_INVERSE_ABSOLUTE;
	}
	//ir->ia(->a->r)
	//ir->ia(->a)
	//a->ia(->ir)
	if ((desttype == SKEL_ABSOLUTE && sourcetype == SKEL_INVERSE_RELATIVE) ||
		(desttype == SKEL_RELATIVE && sourcetype == SKEL_INVERSE_RELATIVE) ||
		(desttype == SKEL_INVERSE_RELATIVE && sourcetype == SKEL_ABSOLUTE))
	{
		//for this conversion, we need absolute data.
		//this is not an efficient operation.
		sourcedata = Alias_ConvertBoneData(sourcetype, sourcedata, bonecount, bones, SKEL_INVERSE_ABSOLUTE, destbuffer, destbufferalt, destbonecount);
		sourcetype = SKEL_INVERSE_ABSOLUTE;
	}

	//r->a
	//r->a(->ia)
	//ir->ia
	if ((sourcetype == SKEL_RELATIVE && (desttype == SKEL_ABSOLUTE || desttype == SKEL_INVERSE_ABSOLUTE)) ||
		(sourcetype == SKEL_INVERSE_RELATIVE && desttype == SKEL_INVERSE_ABSOLUTE))
	{
		float *dest = (sourcedata == destbuffer)?destbufferalt:destbuffer;
		/*needs to be an absolute skeleton*/
		for (i = 0; i < bonecount; i++)
		{
			if (bones[i].parent >= 0)
				R_ConcatTransforms((const void*)(dest + bones[i].parent*12), (const void*)(sourcedata+i*12), (void*)(dest+i*12));
			else
			{
				Vector4Copy(sourcedata+i*12+0, dest+i*12+0);
				Vector4Copy(sourcedata+i*12+4, dest+i*12+4);
				Vector4Copy(sourcedata+i*12+8, dest+i*12+8);
			}
		}
		sourcedata = dest;
		if (sourcetype == SKEL_INVERSE_RELATIVE)
			sourcetype = SKEL_INVERSE_ABSOLUTE;
		else
			sourcetype = SKEL_ABSOLUTE;
	}

	//ia->a(->r)
	//ia->a
	if ((desttype == SKEL_RELATIVE || desttype == SKEL_ABSOLUTE) && sourcetype == SKEL_INVERSE_ABSOLUTE)
	{
		float iim[12];
		float *dest = (sourcedata == destbuffer)?destbufferalt:destbuffer;
		for (i = 0; i < bonecount; i++)
		{
			Matrix3x4_Invert_Simple(bones[i].inverse, iim);
			R_ConcatTransforms((const void*)(sourcedata + i*12), (const void*)iim, (void*)(dest + i*12));
		}
		sourcedata = dest;
		sourcetype = SKEL_ABSOLUTE;
	}

	//ia->ir
	//a->r
	if ((desttype == SKEL_RELATIVE && sourcetype == SKEL_ABSOLUTE) ||
		(desttype == SKEL_INVERSE_RELATIVE && sourcetype == SKEL_INVERSE_ABSOLUTE))
	{
		float ip[12];
		float *dest = (sourcedata == destbuffer)?destbufferalt:destbuffer;
		for (i = 0; i < bonecount; i++)
		{
			if (bones[i].parent >= 0)
			{
				Matrix3x4_Invert_Simple(sourcedata+bones[i].parent*12, ip);
				R_ConcatTransforms((const void*)ip, (const void*)(sourcedata+i*12), (void*)(dest+i*12));
			}
			else
			{
				Vector4Copy(sourcedata+i*12+0, dest+i*12+0);
				Vector4Copy(sourcedata+i*12+4, dest+i*12+4);
				Vector4Copy(sourcedata+i*12+8, dest+i*12+8);
			}
		}
		sourcedata = dest;
		if (sourcetype == SKEL_INVERSE_ABSOLUTE)
			sourcetype = SKEL_INVERSE_RELATIVE;
		else
			sourcetype = SKEL_RELATIVE;
	}

	//a->ia
	if (desttype == SKEL_INVERSE_ABSOLUTE && sourcetype == SKEL_ABSOLUTE)
	{
		float *dest = (sourcedata == destbuffer)?destbufferalt:destbuffer;
		for (i = 0; i < bonecount; i++)
			R_ConcatTransforms((const void*)(sourcedata + i*12), (const void*)(bones[i].inverse), (void*)(dest + i*12));
		sourcedata = dest;
		sourcetype = SKEL_INVERSE_ABSOLUTE;
	}
	
	if (sourcetype == SKEL_IDENTITY)
	{	//we can 'convert' identity matricies to anything. but we only want to do this when everything else is bad, because there really is no info here
		float *dest = (sourcedata == destbuffer)?destbufferalt:destbuffer;
		memset(dest, 0, bonecount*12*sizeof(float));
		for (i = 0; i < bonecount; i++)
		{	//is this right? does it matter?
			dest[i*12+0] = 1;
			dest[i*12+5] = 1;
			dest[i*12+10] = 1;
		}
		sourcedata = dest;
		sourcetype = desttype;	//panic
	}

	if (sourcetype != desttype)
		Sys_Error("Alias_ConvertBoneData: %i->%i not supported\n", (int)sourcetype, (int)desttype);

	return sourcedata;
}
/*
converts the bone data from source to dest.
uses parent bone info, so don't try to offset for a first bone.
ALWAYS writes dest. Don't force it if you don't want to waste cycles when no conversion is actually needed.
destbonecount is to catch errors, its otherwise ignored for now. no identity padding.
*/
void QDECL Alias_ForceConvertBoneData(skeltype_t sourcetype, const float *sourcedata, size_t bonecount, galiasbone_t *bones, skeltype_t desttype, float *destbuffer, size_t destbonecount)
{
	float altbuffer[MAX_BONES*12];
	const float *buf = Alias_ConvertBoneData(sourcetype, sourcedata, bonecount, bones, desttype, destbuffer, altbuffer, destbonecount);
	if (buf != destbuffer)
	{
		//Alias_ConvertBoneData successfully managed to avoid doing any work. bah.
		memcpy(destbuffer, buf, bonecount*12*sizeof(float));
	}
}

#endif





#if 1

struct
{
	int numcoords;
	vecV_t *coords;

	int numnorm;
	vec3_t *norm;

	int bonegroup;
	int vertgroup;
	entity_t *ent;

#ifdef SKELETALMODELS
	boneidx_t *bonemap; //force the renderer to forget the current entity when this changes
	float gpubones[MAX_BONES*12]; //temp storage for multi-surface models with too many bones.
	float boneposebuffer1[MAX_BONES*12];
	float boneposebuffer2[MAX_BONES*12];
	skeltype_t bonecachetype;
	const float *usebonepose;
	int bonecount;
#endif
	qboolean usebones;

	vecV_t *acoords1;
	vecV_t *acoords2;
	vec3_t *anorm;
	vec3_t *anorms;
	vec3_t *anormt;
	float lerp;

	vbo_t vbo;
	vbo_t *vbop;
} meshcache;

//#define SSE_INTRINSICS
#ifdef SSE_INTRINSICS
#include <xmmintrin.h>
#endif

#ifndef SERVERONLY
#ifdef D3DQUAKE
void R_LightArraysByte_BGR(const entity_t *entity, vecV_t *coords, byte_vec4_t *colours, int vertcount, vec3_t *normals, qboolean usecolourmod)
{
	int i;
	int c;
	float l;

	byte_vec4_t ambientlightb;
	byte_vec4_t shadelightb;
	const float *lightdir = entity->light_dir;

	for (i = 0; i < 3; i++)
	{
		l = entity->light_avg[2-i]*255;
		ambientlightb[i] = bound(0, l, 255);
		l = entity->light_range[2-i]*255;
		shadelightb[i] = bound(0, l, 255);
	}

	if (!normals || (ambientlightb[0] == shadelightb[0] && ambientlightb[1] == shadelightb[1] && ambientlightb[2] == shadelightb[2]))
	{
		for (i = vertcount-1; i >= 0; i--)
		{
			*(int*)colours[i] = *(int*)ambientlightb;
//			colours[i][0] = ambientlightb[0];
//			colours[i][1] = ambientlightb[1];
//			colours[i][2] = ambientlightb[2];
		}
	}
	else
	{
		for (i = vertcount-1; i >= 0; i--)
		{
			l = DotProduct(normals[i], lightdir);
			c = l*shadelightb[0];
			c += ambientlightb[0];
			colours[i][0] = bound(0, c, 255);
			c = l*shadelightb[1];
			c += ambientlightb[1];
			colours[i][1] = bound(0, c, 255);
			c = l*shadelightb[2];
			c += ambientlightb[2];
			colours[i][2] = bound(0, c, 255);
		}
	}
}
#endif

void R_LightArrays(const entity_t *entity, vecV_t *coords, avec4_t *colours, int vertcount, vec3_t *normals, float scale, qboolean colormod)
{
	extern cvar_t r_vertexdlights;
	int i;
	float l;

	//float *lightdir = currententity->light_dir; //unused variable

	if (!normals || (!entity->light_range[0] && !entity->light_range[1] && !entity->light_range[2]))
	{
		vec3_t val;
		VectorCopy(entity->light_avg, val);
		if (colormod)
			VectorMul(val, entity->shaderRGBAf, val);

		for (i = vertcount-1; i >= 0; i--)
		{
			VectorCopy(val, colours[i]);
		}
	}
	else
	{
		avec3_t la, lr;
		VectorScale(entity->light_avg, scale, la);
		VectorScale(entity->light_range, scale, lr);
		if (colormod)
		{
			VectorMul(la, entity->shaderRGBAf, la);
			VectorMul(lr, entity->shaderRGBAf, lr);
		}
#ifdef SSE_INTRINSICS
		__m128 va, vs, vl, vr;
		va = _mm_load_ps(ambientlight);
		vs = _mm_load_ps(shadelight);
		va.m128_f32[3] = 0;
		vs.m128_f32[3] = 1;
#endif
		/*dotproduct will return a value between 1 and -1, so increase the ambient to be correct for normals facing away from the light*/
		for (i = vertcount-1; i >= 0; i--)
		{
			l = DotProduct(normals[i], entity->light_dir);
	#ifdef SSE_INTRINSICS
			if (l < 0)
			{
				_mm_storeu_ps(colours[i], va);
				//stomp on colour[i][3] (will be set to 1)
			}
			else
			{
				vl = _mm_load1_ps(&l);
				vr = _mm_mul_ss(va,vl);
				vr = _mm_add_ss(vr,vs);

				_mm_storeu_ps(colours[i], vr);
				//stomp on colour[i][3] (will be set to 1)
			}
	#else
			if (l < 0)
			{	//don't over-shade the dark side of the mesh.
				colours[i][0] = la[0];
				colours[i][1] = la[1];
				colours[i][2] = la[2];
			}
			else
			{
				colours[i][0] = l*lr[0]+la[0];
				colours[i][1] = l*lr[1]+la[1];
				colours[i][2] = l*lr[2]+la[2];
			}
	#endif
		}
	}

	if (r_vertexdlights.ival && r_dynamic.ival > 0 && normals)
	{
		unsigned int lno, v;
		vec3_t dir, rel;
		float dot, d, a;
		//don't include world lights
		for (lno = rtlights_first; lno < RTL_FIRST; lno++)
		{
			if (cl_dlights[lno].radius && (cl_dlights[lno].flags & LFLAG_LIGHTMAP))
			{
				VectorSubtract (cl_dlights[lno].origin,
								entity->origin,
								dir);
				if (Length(dir)>cl_dlights[lno].radius+256)	//far out man!
					continue;

				rel[0] = -DotProduct(dir, entity->axis[0]);
				rel[1] = -DotProduct(dir, entity->axis[1]);
				rel[2] = -DotProduct(dir, entity->axis[2]);

				for (v = 0; v < vertcount; v++)
				{
					VectorSubtract(coords[v], rel, dir);
					dot = DotProduct(dir, normals[v]);
					if (dot>0)
					{
						d = DotProduct(dir, dir);
						a = 1/d;
						if (a>0)
						{
							a *= 10000000*dot/sqrt(d);
							colours[v][0] += a*cl_dlights[lno].color[0];
							colours[v][1] += a*cl_dlights[lno].color[1];
							colours[v][2] += a*cl_dlights[lno].color[2];
						}
					}
				}
			}
		}
	}
}
#ifdef NONSKELETALMODELS
static void R_LerpFrames(mesh_t *mesh, galiaspose_t *p1, galiaspose_t *p2, float lerp, float expand, float lerpcutoff)
{
	extern cvar_t r_nolerp; // r_nolightdir is unused
	float blerp = 1-lerp;
	int i;
	vecV_t *p1v = p1->ofsverts, *p2v = p2->ofsverts;
	vec3_t *p1n = p1->ofsnormals, *p2n = p2->ofsnormals;
	vec3_t *p1s = p1->ofssvector, *p2s = p2->ofssvector;
	vec3_t *p1t = p1->ofstvector, *p2t = p2->ofstvector;

	mesh->snormals_array = blerp>0.5?p2s:p1s;		//never lerp
	mesh->tnormals_array = blerp>0.5?p2t:p1t;		//never lerp
	mesh->colors4f_array[0] = NULL;	//not generated

	if (p1v == p2v || r_nolerp.value || !blerp)
	{
		mesh->normals_array = p1n;
		mesh->snormals_array = p1s;
		mesh->tnormals_array = p1t;

		if (expand)
		{
			vecV_t *oxyz = mesh->xyz_array;
			for (i = 0; i < mesh->numvertexes; i++)
			{
				oxyz[i][0] = p1v[i][0] + p1n[i][0]*expand;
				oxyz[i][1] = p1v[i][1] + p1n[i][1]*expand;
				oxyz[i][2] = p1v[i][2] + p1n[i][2]*expand;
			}
			return;
		}
		else
			mesh->xyz_array = p1v;
	}
	else
	{
		vecV_t *oxyz = mesh->xyz_array;
		vec3_t *onorm = mesh->normals_array;
		if (lerpcutoff)
		{
			vec3_t d;
			lerpcutoff *= lerpcutoff;
			for (i = 0; i < mesh->numvertexes; i++)
			{
				VectorSubtract(p2v[i], p1v[i], d);
				if (DotProduct(d, d) > lerpcutoff)
				{
					//just use the current frame if we're over the lerp threshold.
					//these verts are considered to have teleported.
					onorm[i][0] = p2n[i][0];
					onorm[i][1] = p2n[i][1];
					onorm[i][2] = p2n[i][2];

					oxyz[i][0] = p2v[i][0];
					oxyz[i][1] = p2v[i][1];
					oxyz[i][2] = p2v[i][2];
				}
				else
				{
					onorm[i][0] = p1n[i][0]*lerp + p2n[i][0]*blerp;
					onorm[i][1] = p1n[i][1]*lerp + p2n[i][1]*blerp;
					onorm[i][2] = p1n[i][2]*lerp + p2n[i][2]*blerp;

					oxyz[i][0] = p1v[i][0]*lerp + p2v[i][0]*blerp;
					oxyz[i][1] = p1v[i][1]*lerp + p2v[i][1]*blerp;
					oxyz[i][2] = p1v[i][2]*lerp + p2v[i][2]*blerp;
				}
			}
		}
		else
		{
			for (i = 0; i < mesh->numvertexes; i++)
			{
				onorm[i][0] = p1n[i][0]*lerp + p2n[i][0]*blerp;
				onorm[i][1] = p1n[i][1]*lerp + p2n[i][1]*blerp;
				onorm[i][2] = p1n[i][2]*lerp + p2n[i][2]*blerp;

				oxyz[i][0] = p1v[i][0]*lerp + p2v[i][0]*blerp;
				oxyz[i][1] = p1v[i][1]*lerp + p2v[i][1]*blerp;
				oxyz[i][2] = p1v[i][2]*lerp + p2v[i][2]*blerp;
			}
		}

		if (expand)
		{
			for (i = 0; i < mesh->numvertexes; i++)
			{
				oxyz[i][0] += onorm[i][0]*expand;
				oxyz[i][1] += onorm[i][1]*expand;
				oxyz[i][2] += onorm[i][2]*expand;
			}
		}
	}
}
#endif
#endif
#endif

#ifdef SKELETALMODELS
/*
	returns the up-to-8 skeletal bone poses to blend together.
	return value is the number of blends that are actually live.
*/
typedef struct
{
	skeltype_t	skeltype;	//the skeletal type of this bone block. all blocks should have the same result or the whole thing is unusable or whatever.
	int			firstbone;	//first bone of interest
	int			endbone;	//the first bone of the next group (ie: if first is 0, this is the count)
	int			lerpcount;	//number of pose+frac entries.
	float		frac[FRAME_BLENDS*2];	//weight of this animation (1 if lerpcount is 1)
	float		*pose[FRAME_BLENDS*2];	//pointer to the raw frame data for bone 0.
	void		*needsfree[FRAME_BLENDS*2];
} skellerps_t;
static qboolean Alias_BuildSkelLerps(skellerps_t *lerps, const struct framestateregion_s *fs, const galiasbone_t *boneinf, int numbones, const galiasinfo_t *inf)
{
	int frame1;	//signed, because frametime might be negative...
	int frame2;
	float mlerp;	//minor lerp, poses within a group.
	int l = 0;
	galiasanimation_t *g;
	unsigned int b;
	float totalweight = 0, dropweight = 0;
#ifndef SERVERONLY
	extern cvar_t r_nolerp;
#endif

	lerps->skeltype = SKEL_IDENTITY;	//sometimes nothing else is valid.

	for (b = 0; b < FRAME_BLENDS; b++)
	{
		if (fs->lerpweight[b])
		{
			unsigned int frame = fs->frame[b];
			float time = fs->frametime[b];
			if (frame >= inf->numanimations)
			{
				if (inf->numanimations)
					frame = 0;
				else
				{
					dropweight += fs->lerpweight[b];
					continue;//frame = (unsigned)frame%inf->groups;
				}
			}

			g = &inf->ofsanimations[frame];

			if (lerps->skeltype == SKEL_IDENTITY)
				lerps->skeltype = g->skeltype;
			else if (lerps->skeltype != g->skeltype)
			{
				dropweight += fs->lerpweight[b];
				continue;	//oops, can't cope with mixed blend types
			}

			if (g->GetRawBones)
			{
				lerps->frac[l] = fs->lerpweight[b];
				lerps->needsfree[l] = BZ_Malloc(sizeof(float)*12*numbones);
				lerps->pose[l] = g->GetRawBones(inf, g, time, lerps->needsfree[l], boneinf, numbones);
				if (lerps->pose[l])
					l++;
				else
					dropweight += lerps->frac[l];
				continue;
			}
			if (!g->numposes)
			{
				dropweight += fs->lerpweight[b];
				continue;	//err...
			}

			mlerp = time*g->rate;
			frame1=floor(mlerp);
			frame2=frame1+1;
			mlerp-=frame1;
			if (g->loop)
			{	//loop normally.
				frame1=frame1%g->numposes;
				frame2=frame2%g->numposes;
				if (frame1 < 0)
					frame1 += g->numposes;
				if (frame2 < 0)
					frame2 += g->numposes;
			}
			else
			{
				frame1=bound(0, frame1, g->numposes-1);
				frame2=bound(0, frame2, g->numposes-1);
			}

			if (frame1 == frame2)
				mlerp = 0;
			else if (r_noframegrouplerp.ival)
				mlerp = (mlerp>0.5)?1:0;
			lerps->frac[l] = (1-mlerp)*fs->lerpweight[b];
			if (lerps->frac[l]>0)
			{
				totalweight += lerps->frac[l];
				lerps->needsfree[l] = NULL;
				lerps->pose[l++] = (float*)g->boneofs + inf->numbones*12*frame1;
			}
			lerps->frac[l] = (mlerp)*fs->lerpweight[b];
			if (lerps->frac[l]>0)
			{
				totalweight += lerps->frac[l];
				lerps->needsfree[l] = NULL;
				lerps->pose[l++] = (float*)g->boneofs + inf->numbones*12*frame2;
			}
		}
	}

#ifndef SERVERONLY
	if (r_nolerp.ival && l > 1)
	{	//when lerping is completely disabled, find the strongest influence
		frame1 = 0;
		mlerp = lerps->frac[0];
		for (b = 1; b < l; b++)
		{
			if (lerps->frac[b] > mlerp)
			{
				frame1 = b;
				mlerp = lerps->frac[b];
			}
		}
		lerps->frac[0] = totalweight+dropweight;
		lerps->pose[0] = lerps->pose[frame1];
		l = 1;
	}
	else
#endif
		if (l && totalweight && dropweight)
	{	//don't rescale if some animation got dropped.
		totalweight = (totalweight+dropweight) / totalweight;
		for (b = 0; b < l; b++)
		{
			lerps->frac[b] *= totalweight;
		}
	}

	lerps->lerpcount = l;
	return l > 0;
}
/*
finds the various blend info. returns number of bone blocks used.
*/
static int Alias_FindRawSkelData(galiasinfo_t *inf, const framestate_t *fstate, skellerps_t *lerps, size_t firstbone, size_t lastbone, const galiasbone_t *boneinfo)
{
	int bonegroup;
	int cbone = 0;
	int endbone;
	int numbonegroups=0;

	if (lastbone > inf->numbones)
		lastbone = inf->numbones;

	for (bonegroup = 0; bonegroup < FS_COUNT; bonegroup++)
	{
		endbone = fstate->g[bonegroup].endbone;
		if (bonegroup == FS_COUNT-1)
			endbone = MAX_BONES;

		if (cbone <= firstbone || endbone > lastbone)
		{
			lerps->firstbone = max(cbone, firstbone);
			lerps->endbone = max(lerps->firstbone, min(endbone, lastbone));
			if (lerps->firstbone == lerps->endbone)
				continue;

			if (!inf->numanimations || !Alias_BuildSkelLerps(lerps, &fstate->g[bonegroup], boneinfo, lerps->endbone, inf))	//if there's no animations in this model, use the base pose instead.
			{
				if (!inf->baseframeofs)
					continue;	//nope, not happening.
				lerps->skeltype = SKEL_ABSOLUTE;
				lerps->needsfree[0] = NULL;
				lerps->frac[0] = 1;
				lerps->pose[0] = inf->baseframeofs;
				lerps->lerpcount = 1;
			}
			numbonegroups++;
			lerps++;
		}
		cbone = endbone;
	}
	return numbonegroups;
}
/*
	retrieves the raw bone data for a current frame state.
	ignores poses that don't match the desired skeltype
	ignores skeletal objects.
	return value is the lastbone argument, or less if the model simply doesn't have that many bones.
	_always_ writes into result
*/
static int Alias_BlendBoneData(galiasinfo_t *inf, const framestate_t *fstate, float *result, skeltype_t skeltype, int firstbone, int lastbone, const galiasbone_t *boneinfo)
{
	skellerps_t lerps[FS_COUNT], *lerp;
	size_t bone, endbone = 0;
	size_t numgroups = Alias_FindRawSkelData(inf, fstate, lerps, firstbone, lastbone, boneinfo);

	float *pose, *matrix;
	int k, b;

	for (lerp = lerps; numgroups--; lerp++)
	{
		if (lerp->skeltype == skeltype)
		{
			bone = lerp->firstbone;
			endbone = lerp->endbone;
			if (lerp->lerpcount == 1 && lerp->frac[0] == 1)
				memcpy(result+bone*12, lerp->pose[0]+bone*12, (endbone-bone)*12*sizeof(float));
			else
			{
				//blend each influence
				for (; bone < endbone; bone++)
				{
					pose = result + 12*bone;
					//set up the per-bone transform matrix
					matrix = lerps->pose[0] + bone*12;
					for (k = 0;k < 12;k++)
						pose[k] = matrix[k] * lerp->frac[0];
					for (b = 1;b < lerp->lerpcount;b++)
					{
						matrix = lerps->pose[b] + bone*12;

						for (k = 0;k < 12;k++)
							pose[k] += matrix[k] * lerp->frac[b];
					}
				}
			}
		}
		for (k = 0; k < lerp->lerpcount; k++)
			BZ_Free(lerp->needsfree[k]);
	}
	return endbone;
}

/*retrieves the bone data.
only writes targetbuffer if needed. the return value is the only real buffer result.
assumes that all blended types are the same. probably buggy, but meh.
*/
static const float *Alias_GetBoneInformation(galiasinfo_t *inf, const framestate_t *framestate, skeltype_t targettype, float *targetbuffer, float *targetbufferalt, size_t numbones, const galiasbone_t *boneinfo)
{
	skellerps_t lerps[FS_COUNT], *lerp;
	size_t numgroups;
	size_t bone, endbone;
	const float *ret;

	lerps[0].skeltype = SKEL_IDENTITY; //just in case.
#ifdef SKELETALOBJECTS
	if (framestate->bonestate && framestate->bonecount >= numbones)
	{
		lerps[0].skeltype = framestate->skeltype;
		lerps[0].firstbone = 0;
		lerps[0].endbone = framestate->bonecount;
		lerps[0].pose[0] = framestate->bonestate;
		lerps[0].frac[0] = 1;
		lerps[0].needsfree[0] = NULL;
		lerps[0].lerpcount = 1;
		numgroups = 1;
	}
	else
#endif
	{
		numgroups = Alias_FindRawSkelData(inf, framestate, lerps, 0, numbones, boneinfo);
	}

	//try to return data in-place.
	if (numgroups==1 && lerps[0].lerpcount == 1)
	{
		ret = Alias_ConvertBoneData(lerps[0].skeltype, lerps[0].pose[0], min(lerps[0].endbone, inf->numbones), inf->ofsbones, targettype, targetbuffer, targetbufferalt, numbones);
		if (ret == lerps[0].needsfree[0])
		{	//bum
			memcpy(targetbuffer, ret, sizeof(float)*min(lerps[0].endbone, numbones)*12);
			ret = targetbuffer;
		}
		BZ_Free(lerps[0].needsfree[0]);
		return ret;
	}

	for (lerp = lerps; numgroups--; lerp++)
	{
		bone = lerp->firstbone;
		endbone = lerp->endbone;
		switch(lerp->lerpcount)
		{
		case 2:
			{
				int k;
				float *out = targetbuffer + bone*12;
				float *pose1 = lerp->pose[0] + bone*12, *pose2 = lerp->pose[1] + bone*12;
				float frac1 = lerp->frac[0], frac2 = lerp->frac[1];
				for (; bone < endbone; bone++, out+=12, pose1+=12, pose2+=12)
				{
					for (k = 0; k < 12; k++)	//please please unroll!
						out[k] = (pose1[k]*frac1) + (frac2*pose2[k]);
				}
				BZ_Free(lerp->needsfree[0]);
				BZ_Free(lerp->needsfree[1]);
			}
			break;
		case 3:
			{
				int k;
				float *out = targetbuffer + bone*12;
				float *pose1 = lerp->pose[0] + bone*12, *pose2 = lerp->pose[1] + bone*12, *pose3 = lerp->pose[2] + bone*12;
				float frac1 = lerp->frac[0], frac2 = lerp->frac[1], frac3 = lerp->frac[2];
				for (; bone < endbone; bone++, out+=12, pose1+=12, pose2+=12, pose3+=12)
				{
					for (k = 0; k < 12; k++)	//please please unroll!
						out[k] = (pose1[k]*frac1) + (frac2*pose2[k]) + (pose3[k]*frac3);
				}
				BZ_Free(lerp->needsfree[0]);
				BZ_Free(lerp->needsfree[1]);
				BZ_Free(lerp->needsfree[2]);
			}
			break;
		case 4:
			{
				int k;
				float *out = targetbuffer + bone*12;
				float *pose1 = lerp->pose[0] + bone*12, *pose2 = lerp->pose[1] + bone*12, *pose3 = lerp->pose[2] + bone*12, *pose4 = lerp->pose[3] + bone*12;
				float frac1 = lerp->frac[0], frac2 = lerp->frac[1], frac3 = lerp->frac[2], frac4 = lerp->frac[3];
				for (; bone < endbone; bone++, out+=12, pose1+=12, pose2+=12, pose3+=12, pose4+=12)
				{
					for (k = 0; k < 12; k++)	//please please unroll!
						out[k] = (pose1[k]*frac1) + (frac2*pose2[k]) + (pose3[k]*frac3) + (frac4*pose4[k]);
				}
				BZ_Free(lerp->needsfree[0]);
				BZ_Free(lerp->needsfree[1]);
				BZ_Free(lerp->needsfree[2]);
				BZ_Free(lerp->needsfree[3]);
			}
			break;
		case 0:
		case 1:	//the weight will usually be 1, which won't take this path.
		default:
			{	//the generic slow path.
				int k, i, b;
				float *out, *pose, frac;
				for (i = 0; i < lerp->lerpcount; i++)
				{
					out = targetbuffer + bone*12;
					pose = lerp->pose[i] + bone*12;
					frac = lerp->frac[i];
					if (!i)
					{	//first influence shouldn't add, saving us a memcpy.
						for (b = bone; b < endbone; b++, out+=12, pose+=12)
							for (k = 0; k < 12; k++)
								out[k] = (pose[k]*frac);
					}
					else
					{
						for (b = bone; b < endbone; b++, out+=12, pose+=12)
							for (k = 0; k < 12; k++)
								out[k] += (pose[k]*frac);
					}
					BZ_Free(lerp->needsfree[i]);
				}
			}
			break;
		}
	}

	return Alias_ConvertBoneData(lerps[0].skeltype, targetbuffer, inf->numbones, inf->ofsbones, targettype, targetbuffer, targetbufferalt, numbones);
}

static void Alias_BuildSkeletalMesh(mesh_t *mesh, framestate_t *framestate, galiasinfo_t *inf)
{
	boneidx_t *fte_restrict bidx = inf->ofs_skel_idx[0];
	float *fte_restrict weight = inf->ofs_skel_weight[0];
	const float *morphweights;

	if (meshcache.bonecachetype != SKEL_INVERSE_ABSOLUTE)
		meshcache.usebonepose = Alias_GetBoneInformation(inf, framestate, meshcache.bonecachetype=SKEL_INVERSE_ABSOLUTE, meshcache.boneposebuffer1, meshcache.boneposebuffer2, inf->numbones, NULL);

	morphweights = inf->AnimateMorphs?inf->AnimateMorphs(inf, framestate, alloca(sizeof(*morphweights)*inf->nummorphs)):NULL;
	if (morphweights)
	{
		size_t m,v;
		float w;
		vecV_t *xyz = alloca(sizeof(*xyz)*inf->numverts), *inxyz;
		vec3_t *norm = alloca(sizeof(*norm)*inf->numverts), *innorm;
		vec3_t *sdir = alloca(sizeof(*sdir)*inf->numverts), *insdir;
		vec3_t *tdir = alloca(sizeof(*tdir)*inf->numverts), *intdir;
		memcpy(xyz, inf->ofs_skel_xyz, sizeof(*xyz)*inf->numverts);
		memcpy(norm, inf->ofs_skel_norm, sizeof(*norm)*inf->numverts);
		memcpy(sdir, inf->ofs_skel_svect, sizeof(*sdir)*inf->numverts);
		memcpy(tdir, inf->ofs_skel_tvect, sizeof(*tdir)*inf->numverts);
		for (m = 0; m < inf->nummorphs; m++)
		{
			if (morphweights[m] <= 0)
				continue;
			inxyz = inf->ofs_skel_xyz + (m+1)*inf->numverts;
			innorm = inf->ofs_skel_norm + (m+1)*inf->numverts;
			insdir = inf->ofs_skel_svect + (m+1)*inf->numverts;
			intdir = inf->ofs_skel_tvect + (m+1)*inf->numverts;
			w = morphweights[m];
			for (v = 0; v < inf->numverts; v++)
			{
				VectorMA(xyz[v], w, inxyz[v], xyz[v]);
				VectorMA(norm[v], w, innorm[v], norm[v]);
				VectorMA(sdir[v], w, insdir[v], sdir[v]);
				VectorMA(tdir[v], w, intdir[v], tdir[v]);
			}
		}

		//right, now do the bones thing.
		Alias_TransformVerticies_VNST(meshcache.usebonepose, inf->numverts, bidx, weight,
				xyz[0], mesh->xyz_array[0],
				norm[0], mesh->normals_array[0],
				sdir[0], mesh->snormals_array[0],
				tdir[0], mesh->tnormals_array[0]
				);
	}
	else if ((1))
		Alias_TransformVerticies_VNST(meshcache.usebonepose, inf->numverts, bidx, weight, 
				inf->ofs_skel_xyz[0], mesh->xyz_array[0],
				inf->ofs_skel_norm[0], mesh->normals_array[0],
				inf->ofs_skel_svect[0], mesh->snormals_array[0],
				inf->ofs_skel_tvect[0], mesh->tnormals_array[0]
				);
	else
		Alias_TransformVerticies_VN(meshcache.usebonepose, inf->numverts, bidx, weight,
				inf->ofs_skel_xyz[0], mesh->xyz_array[0],
				inf->ofs_skel_norm[0], mesh->normals_array[0]
				);
}

#if defined(MD5MODELS) || defined(ZYMOTICMODELS) || defined(DPMMODELS)
static int QDECL sortweights(const void *v1, const void *v2)	//helper for Alias_BuildGPUWeights
{
	const galisskeletaltransforms_t *w1=v1, *w2=v2;
	if (w1->vertexindex - w2->vertexindex)
		return w1->vertexindex - w2->vertexindex;
	if (w1->org[3] > w2->org[3])
		return -1;
	if (w1->org[3] < w2->org[3])
		return 1;
	return 0;
}
//takes old-style vertex transforms and tries to generate something more friendly for GPUs, limited to only 4 influences per vertex
static void Alias_BuildGPUWeights(model_t *mod, galiasinfo_t *inf, size_t num_trans, galisskeletaltransforms_t *trans, qboolean calcnorms)
{
	size_t i, j, v;
	double strength;
	const float *matrix, *basepose;
	const galisskeletaltransforms_t *t;

	float buffer[MAX_BONES*12];
	float bufferalt[MAX_BONES*12];

	//first sort the weights by the verticies, then by strength. this is probably already done, but whatever.
	qsort(trans, num_trans, sizeof(*trans), sortweights);

	inf->ofs_skel_xyz = ZG_Malloc(&mod->memgroup, sizeof(*inf->ofs_skel_xyz) * inf->numverts);
	inf->ofs_skel_norm = ZG_Malloc(&mod->memgroup, sizeof(*inf->ofs_skel_norm) * inf->numverts);
	inf->ofs_skel_svect = ZG_Malloc(&mod->memgroup, sizeof(*inf->ofs_skel_svect) * inf->numverts);
	inf->ofs_skel_tvect = ZG_Malloc(&mod->memgroup, sizeof(*inf->ofs_skel_tvect) * inf->numverts);
	inf->ofs_skel_idx = ZG_Malloc(&mod->memgroup, sizeof(*inf->ofs_skel_idx) * inf->numverts);
	inf->ofs_skel_weight = ZG_Malloc(&mod->memgroup, sizeof(*inf->ofs_skel_weight) * inf->numverts);

	//make sure we have a base pose for all animations to be relative to. first animation's first pose if there isn't an explicit one
	basepose = inf->baseframeofs;
	if (!basepose)
	{
		if (!inf->numanimations || !inf->ofsanimations[0].boneofs)
		{
			Con_DPrintf("Alias_BuildGPUWeights: no base pose\n");
			return;	//its fucked jim
		}
		basepose = Alias_ConvertBoneData(inf->ofsanimations[0].skeltype, inf->ofsanimations[0].boneofs, inf->numbones, inf->ofsbones, SKEL_ABSOLUTE, buffer, bufferalt, MAX_BONES);
	}

	//make sure we have bone inversions
	for (i = 0; i < inf->numbones; i++)
	{
		Matrix3x4_Invert_Simple(basepose+i*12, inf->ofsbones[i].inverse);
	}

	//validate the indicies, because we can.
	for (i = 0; i < inf->numindexes; i++)
	{
		if (inf->ofs_indexes[i] >= inf->numverts)
			Con_DPrintf("Alias_BuildGPUWeights: bad index\n");
	}

	//walk the (sorted) transforms and calculate the proper position for each vert, and the strongest influences too.
	for (i = 0; i < num_trans; i++)
	{
		t = &trans[i];
		v = t->vertexindex;
		if (v >= inf->numverts || t->boneindex >= inf->numbones)
		{
			Con_DPrintf("Alias_BuildGPUWeights: bad vertex\n");
			continue;
		}
		matrix = basepose + t->boneindex*12;

		//calculate the correct position in the base pose
		inf->ofs_skel_xyz[v][0] += t->org[0] * matrix[0] + t->org[1] * matrix[1] + t->org[2] * matrix[ 2] + t->org[3] * matrix[ 3];
		inf->ofs_skel_xyz[v][1] += t->org[0] * matrix[4] + t->org[1] * matrix[5] + t->org[2] * matrix[ 6] + t->org[3] * matrix[ 7];
		inf->ofs_skel_xyz[v][2] += t->org[0] * matrix[8] + t->org[1] * matrix[9] + t->org[2] * matrix[10] + t->org[3] * matrix[11];
#ifndef SERVERONLY
		if (!calcnorms)
		{
			inf->ofs_skel_norm[v][0] += t->normal[0] * matrix[0] + t->normal[1] * matrix[1] + t->normal[2] * matrix[ 2];
			inf->ofs_skel_norm[v][1] += t->normal[0] * matrix[4] + t->normal[1] * matrix[5] + t->normal[2] * matrix[ 6];
			inf->ofs_skel_norm[v][2] += t->normal[0] * matrix[8] + t->normal[1] * matrix[9] + t->normal[2] * matrix[10];
		}
#endif

		//we sorted them so we're guarenteed to see the highest influences first.
		for (j = 0; j < 4; j++)
		{
			if (!inf->ofs_skel_weight[v][j])
			{
				inf->ofs_skel_weight[v][j] = t->org[3];
				for (; j < 4; j++)	//be nicer on cache, if necessary
					inf->ofs_skel_idx[v][j] = t->boneindex;
				break;
			}
		}
	}

	//weights should add up to 1, but might not if we had too many influences. make sure they don't move, at least in their base pose.
	for (v = 0; v < inf->numverts; v++)
	{
		strength = inf->ofs_skel_weight[v][0] + inf->ofs_skel_weight[v][1] + inf->ofs_skel_weight[v][2] + inf->ofs_skel_weight[v][3];
		if (strength && strength != 1)
		{
			strength = 1/strength;
			Vector4Scale(inf->ofs_skel_weight[v], strength, inf->ofs_skel_weight[v]);
		}
	}

#ifndef SERVERONLY
	Mod_AccumulateTextureVectors(inf->ofs_skel_xyz, inf->ofs_st_array, inf->ofs_skel_norm, inf->ofs_skel_svect, inf->ofs_skel_tvect, inf->ofs_indexes, inf->numindexes, calcnorms);
	Mod_NormaliseTextureVectors(inf->ofs_skel_norm, inf->ofs_skel_svect, inf->ofs_skel_tvect, inf->numverts, calcnorms);
#endif
}
#endif

#ifndef SERVERONLY
static void Alias_DrawSkeletalBones(galiasbone_t *bones, float const*bonepose, int bonecount, int basebone)
{
	scenetris_t *t;
	int flags = BEF_NODLIGHT|BEF_NOSHADOWS|BEF_LINES;
	shader_t *shader;
	int i, p;
	extern	entity_t	*currententity;
	index_t *indexes;
	vecV_t *verts;
	vec4_t *colours;
	vec2_t *texcoords;
	int numindexes = 0;
	mesh_t bonemesh, *m;
	batch_t b;

	//this shader lookup might get pricy.
	shader = R_RegisterShader("shader_draw_line", SUF_NONE,
		"{\n"
			"program defaultfill\n"
			"{\n"
				"map $whiteimage\n"
				"rgbgen exactvertex\n"
				"alphagen vertex\n"
				"blendfunc blend\n"
			"}\n"
		"}\n");

	if (cl_numstris && cl_stris[cl_numstris-1].shader == shader && cl_stris[cl_numstris-1].flags == flags)
		t = &cl_stris[cl_numstris-1];
	else
	{
		if (cl_numstris == cl_maxstris)
		{
			cl_maxstris += 8;
			cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
		}
		t = &cl_stris[cl_numstris++];
		t->shader = shader;
		t->numidx = 0;
		t->numvert = 0;
		t->firstidx = cl_numstrisidx;
		t->firstvert = cl_numstrisvert;
		t->flags = flags;
	}
	if (cl_numstrisvert + bonecount*2 > cl_maxstrisvert)
		cl_stris_ExpandVerts(cl_numstrisvert + bonecount*2);
	if (cl_maxstrisidx < cl_numstrisidx+bonecount*2)
	{
		cl_maxstrisidx = cl_numstrisidx+bonecount*2;
		cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
	}

	verts = alloca(sizeof(*verts)*bonecount);
	colours = alloca(sizeof(*colours)*bonecount);
	texcoords = alloca(sizeof(*texcoords)*bonecount);
	indexes = alloca(sizeof(*indexes)*bonecount*2);
	numindexes = 0;

	for (i = 0; i < bonecount; i++)
	{
		//fixme: transform by model matrix
		verts[i][0] = bonepose[i*12+3];
		verts[i][1] = bonepose[i*12+7];
		verts[i][2] = bonepose[i*12+11];
		texcoords[i][0] = 0;
		texcoords[i][1] = 0;
		colours[i][0] = (i < basebone)?0:1;
		colours[i][1] = (i < basebone)?0:0;
		colours[i][2] = (i < basebone)?1:0;
		colours[i][3] = 1;

		p = bones[i].parent;
		if (p < 0)
			p = i;
		indexes[numindexes++] = i;
		indexes[numindexes++] = p;
	}

	memset(&bonemesh, 0, sizeof(bonemesh));
	bonemesh.indexes = indexes;
	bonemesh.st_array = texcoords;
	bonemesh.lmst_array[0] = texcoords;
	bonemesh.xyz_array = verts;
	bonemesh.colors4f_array[0] = colours;
	bonemesh.numindexes = numindexes;
	bonemesh.numvertexes = bonecount;
	m = &bonemesh;

//FIXME: We should use the skybox clipping code and split the sphere into 6 sides.
	memset(&b, 0, sizeof(b));
	b.flags = flags;
	b.meshes = 1;
	b.firstmesh = 0;
	b.mesh = &m;
	b.ent = currententity;
	b.shader = shader;
	b.skin = NULL;
	b.texture = NULL;
	b.vbo = NULL;
	BE_SubmitBatch(&b);
}
#endif	//!SERVERONLY
#endif	//SKELETALMODELS

void Alias_FlushCache(void)
{
	meshcache.ent = NULL;
}

void Alias_Shutdown(void)
{
	if (meshcache.norm)
		BZ_Free(meshcache.norm);
	meshcache.norm = NULL;
	meshcache.numnorm = 0;

	if (meshcache.coords)
		BZ_Free(meshcache.coords);
	meshcache.coords = NULL;
	meshcache.numcoords = 0;
}

qboolean Alias_GAliasBuildMesh(mesh_t *mesh, vbo_t **vbop, galiasinfo_t *inf, int surfnum, entity_t *e, qboolean usebones)
{
#ifndef SERVERONLY
	extern cvar_t r_nolerp;
#endif

#ifdef SKELETALMODELS
	qboolean bytecolours = false;
#endif

	if (!inf->numanimations)
	{
#ifdef SKELETALMODELS
		if (inf->ofs_skel_xyz)
		{}
		else
#endif
		{
			Con_DPrintf("Model with no frames (%s)\n", e->model->name);
			return false;
		}
	}

	if (meshcache.numnorm < inf->numverts)
	{
		if (meshcache.norm)
			BZ_Free(meshcache.norm);
		meshcache.norm = BZ_Malloc(sizeof(*meshcache.norm)*inf->numverts*3);
		meshcache.numnorm = inf->numverts;
	}
	if (meshcache.numcoords < inf->numverts)
	{
		if (meshcache.coords)
			BZ_Free(meshcache.coords);
		meshcache.coords = BZ_Malloc(sizeof(*meshcache.coords)*inf->numverts);
		memset(meshcache.coords, 0, sizeof(*meshcache.coords)*inf->numverts);	//vecV_t is often uninitialised.
		meshcache.numcoords = inf->numverts;
	}

	mesh->numvertexes = inf->numverts;
	mesh->indexes = inf->ofs_indexes;
	mesh->numindexes = inf->numindexes;
	mesh->numbones = 0;

#ifndef SERVERONLY
	mesh->colors4f_array[0] = inf->ofs_rgbaf;
	mesh->colors4b_array = inf->ofs_rgbaub;
#ifdef SKELETALMODELS
	bytecolours = !!inf->ofs_rgbaub;
#endif
	mesh->st_array = inf->ofs_st_array;
	mesh->lmst_array[0] = inf->ofs_lmst_array; //some formats allow for two.
#endif
	mesh->trneighbors = inf->ofs_trineighbours;

#if defined(SKELETALMODELS) && !defined(SERVERONLY)
	if (!inf->numbones)
		usebones = false;
	else if (inf->ofs_skel_xyz && !inf->ofs_skel_weight)
		usebones = false;
	else if (e->fatness || !inf->ofs_skel_idx || (!inf->mappedbones && inf->numbones > sh_config.max_gpu_bones) || inf->nummorphs)
#endif
		usebones = false;

	if (meshcache.ent == e)
	{
		if (meshcache.vertgroup == inf->shares_verts && meshcache.ent == e && usebones == meshcache.usebones)
		{
			mesh->xyz_blendw[0] = meshcache.lerp;
			mesh->xyz_blendw[1] = 1-meshcache.lerp;
			mesh->xyz_array = meshcache.acoords1;
			mesh->xyz2_array = meshcache.acoords2;
			mesh->normals_array = meshcache.anorm;
			mesh->snormals_array = meshcache.anorms;
			mesh->tnormals_array = meshcache.anormt;
			if (vbop)
			{
				*vbop = meshcache.vbop;
				meshcache.vbo.indicies = inf->vboindicies;
				meshcache.vbo.indexcount = inf->numindexes;
			}

#ifndef SKELETALMODELS
			return false;
		}
	}
#else
			if (usebones)
			{
				mesh->bonenums = inf->ofs_skel_idx;
				mesh->boneweights = inf->ofs_skel_weight;
				mesh->bones = meshcache.usebonepose;
				mesh->numbones = inf->numbones;
			}	
#ifndef SERVERONLY
			if (meshcache.bonemap != inf->bonemap)
			{
				meshcache.bonemap = inf->bonemap;
				BE_SelectEntity(e);
			}
			if (inf->mappedbones)
			{
				int i;
				for (i = 0; i < inf->mappedbones; i++)
					memcpy(meshcache.gpubones + i*12, meshcache.usebonepose + inf->bonemap[i]*12, sizeof(float)*12);
				meshcache.vbo.numbones = inf->mappedbones;
				meshcache.vbo.bones = meshcache.gpubones;
			}
#endif
			return false;	//don't generate the new vertex positions. We still have them all.
		}
		if (meshcache.bonegroup != inf->shares_bones)
		{
			meshcache.usebonepose = NULL;
			meshcache.bonecachetype = -1;
		}
	}
	else
	{
		meshcache.usebonepose = NULL;
		meshcache.bonecachetype = -1;
	}
	meshcache.bonegroup = inf->shares_bones;
#endif
	meshcache.vertgroup = inf->shares_verts;
	meshcache.ent = e;


#if defined(_DEBUG) && FRAME_BLENDS == 4
	if (!e->framestate.g[FS_REG].lerpweight[0] && !e->framestate.g[FS_REG].lerpweight[1] && !e->framestate.g[FS_REG].lerpweight[2] && !e->framestate.g[FS_REG].lerpweight[3])
		Con_Printf("Entity with no lerp info\n");
#endif


#ifndef SERVERONLY
	mesh->trneighbors = inf->ofs_trineighbours;
	mesh->normals_array = meshcache.norm;
	mesh->snormals_array = meshcache.norm+meshcache.numnorm;
	mesh->tnormals_array = meshcache.norm+meshcache.numnorm*2;
#endif
	mesh->xyz_array = meshcache.coords;

//we don't support meshes with one pose skeletal and another not.
//we don't support meshes with one group skeletal and another not.

#ifdef SKELETALMODELS
	meshcache.vbop = NULL;
	if (vbop)
		*vbop = NULL;
	if (inf->ofs_skel_xyz && !inf->ofs_skel_weight)
	{
		//if we have skeletal xyz info, but no skeletal weights, then its a partial model that cannot possibly be animated.
		meshcache.usebonepose = NULL;
		meshcache.bonecachetype = -1;
		mesh->xyz_array = inf->ofs_skel_xyz;
		mesh->xyz2_array = NULL;
		mesh->normals_array = inf->ofs_skel_norm;
		mesh->snormals_array = inf->ofs_skel_svect;
		mesh->tnormals_array = inf->ofs_skel_tvect;

		if (vbop)
		{
			meshcache.vbo.indicies = inf->vboindicies;
			meshcache.vbo.indexcount = inf->numindexes;
			meshcache.vbo.vertcount = inf->numverts;
			meshcache.vbo.texcoord = inf->vbotexcoords;
			meshcache.vbo.lmcoord[0] = inf->vbolmtexcoords;
			meshcache.vbo.coord = inf->vbo_skel_verts;
			memset(&meshcache.vbo.coord2, 0, sizeof(meshcache.vbo.coord2));
			meshcache.vbo.normals = inf->vbo_skel_normals;
			meshcache.vbo.svector = inf->vbo_skel_svector;
			meshcache.vbo.tvector = inf->vbo_skel_tvector;
			meshcache.vbo.colours[0] = inf->vborgba;
			meshcache.vbo.colours_bytes = bytecolours;
			memset(&meshcache.vbo.bonenums, 0, sizeof(meshcache.vbo.bonenums));
			memset(&meshcache.vbo.boneweights, 0, sizeof(meshcache.vbo.boneweights));
			meshcache.vbo.numbones = 0;
			meshcache.vbo.bones = NULL;
			if (meshcache.vbo.indicies.sysptr)
				*vbop = meshcache.vbop = &meshcache.vbo;
		}
	}
	else if (inf->numbones)
	{
		mesh->xyz2_array = NULL;	//skeltal animations blend bones, not verticies.

		if (!usebones)
		{
			if (inf->numindexes)
			{
				//software bone animation
				//there are two ways to animate a skeleton
				Alias_BuildSkeletalMesh(mesh, &e->framestate, inf);

#ifdef PEXT_FATNESS
				if (e->fatness)
				{
					int i;
					for (i = 0; i < mesh->numvertexes; i++)
					{
						VectorMA(mesh->xyz_array[i], e->fatness, mesh->normals_array[i], meshcache.coords[i]);
					}
					mesh->xyz_array = meshcache.coords;
				}
#endif
			}
			else
			{
				if (meshcache.bonecachetype != SKEL_ABSOLUTE)
					meshcache.usebonepose = Alias_GetBoneInformation(inf, &e->framestate, meshcache.bonecachetype=SKEL_ABSOLUTE, meshcache.boneposebuffer1, meshcache.boneposebuffer2, inf->numbones, NULL);
#ifndef SERVERONLY
				if (inf->shares_bones != surfnum && qrenderer)
					Alias_DrawSkeletalBones(inf->ofsbones, (const float *)meshcache.usebonepose, inf->numbones, e->framestate.g[0].endbone);
#endif
			}
		}
		else
		{
			if (meshcache.bonecachetype != SKEL_INVERSE_ABSOLUTE)
				meshcache.usebonepose = Alias_GetBoneInformation(inf, &e->framestate, meshcache.bonecachetype=SKEL_INVERSE_ABSOLUTE, meshcache.boneposebuffer1, meshcache.boneposebuffer2, inf->numbones, NULL);

			//hardware bone animation
			mesh->xyz_array = inf->ofs_skel_xyz;
			mesh->normals_array = inf->ofs_skel_norm;
			mesh->snormals_array = inf->ofs_skel_svect;
			mesh->tnormals_array = inf->ofs_skel_tvect;

			if (vbop)
			{
				meshcache.vbo.indicies = inf->vboindicies;
				meshcache.vbo.indexcount = inf->numindexes;
				meshcache.vbo.vertcount = inf->numverts;
				meshcache.vbo.texcoord = inf->vbotexcoords;
				meshcache.vbo.lmcoord[0] = inf->vbolmtexcoords;
				meshcache.vbo.coord = inf->vbo_skel_verts;
				memset(&meshcache.vbo.coord2, 0, sizeof(meshcache.vbo.coord2));
				meshcache.vbo.normals = inf->vbo_skel_normals;
				meshcache.vbo.svector = inf->vbo_skel_svector;
				meshcache.vbo.tvector = inf->vbo_skel_tvector;
				meshcache.vbo.colours[0] = inf->vborgba;
				meshcache.vbo.colours_bytes = bytecolours;
				meshcache.vbo.bonenums = inf->vbo_skel_bonenum;
				meshcache.vbo.boneweights = inf->vbo_skel_bweight;
				meshcache.vbo.numbones = inf->numbones;
				meshcache.vbo.bones = meshcache.usebonepose;
				if (meshcache.vbo.indicies.sysptr)
					*vbop = meshcache.vbop = &meshcache.vbo;
			}
		}
	}
	else
#endif
	{
#ifndef NONSKELETALMODELS
		memset(mesh, 0, sizeof(*mesh));
		*vbop = NULL;
		return false;
#else
#ifndef SERVERONLY
		float lerpcutoff;
#endif
		galiasanimation_t *g1, *g2;
		int frame1;
		int frame2;
		float lerp;
		float fg1time;
		//float fg2time;
		static float printtimer;

#if defined(_DEBUG) && FRAME_BLENDS != 2
		if (e->framestate.g[FS_REG].lerpweight[2] || e->framestate.g[FS_REG].lerpweight[3])
			Con_ThrottlePrintf(&printtimer, 1, "Alias_GAliasBuildMesh(%s): non-skeletal animation only supports two animations\n", e->model->name);
#endif

		//FIXME: replace most of this logic with Alias_BuildSkelLerps

		frame1 = e->framestate.g[FS_REG].frame[0];
		frame2 = e->framestate.g[FS_REG].frame[1];
		lerp = e->framestate.g[FS_REG].lerpweight[1];	//FIXME
		fg1time = e->framestate.g[FS_REG].frametime[0];
		//fg2time = e->framestate.g[FS_REG].frametime[1];

		if (frame1 < 0)
		{
			Con_ThrottlePrintf(&printtimer, 1, "Negative frame (%s)\n", e->model->name);
			frame1 = 0;
		}
		if (frame2 < 0)
		{
			Con_ThrottlePrintf(&printtimer, 1, "Negative frame (%s)\n", e->model->name);
			frame2 = frame1;
		}
		if (frame1 >= inf->numanimations)
		{
			Con_ThrottlePrintf(&printtimer, 1, "Too high frame %i (%s)\n", frame1, e->model->name);
			frame1 %= inf->numanimations;
		}
		if (frame2 >= inf->numanimations)
		{
 			Con_ThrottlePrintf(&printtimer, 1, "Too high frame %i (%s)\n", frame2, e->model->name);
			frame2 %= inf->numanimations;
		}

		if (lerp <= 0)
			frame2 = frame1;
		else  if (lerp >= 1)
			frame1 = frame2;

		g1 = &inf->ofsanimations[frame1];
		g2 = &inf->ofsanimations[frame2];

		if (!inf->numanimations || !g1->numposes || !g2->numposes)
		{
			Con_ThrottlePrintf(&printtimer, 1, "Invalid animation data on entity with model %s\n", e->model->name);
			//no animation data. panic!
			memset(mesh, 0, sizeof(*mesh));
			*vbop = NULL;
			return false;
		}

		if (g1 == g2)	//lerping within group is only done if not changing group
		{
			lerp = fg1time*g1->rate;
			frame1=floor(lerp);
			frame2=frame1+1;
			lerp-=frame1;
			if (r_noframegrouplerp.ival || (e->model->engineflags&MDLF_NOLERP))
				lerp = 0;
			if (g1->loop)
			{
				frame1=frame1%g1->numposes;if (frame1 < 0)frame1 += g1->numposes;
				frame2=frame2%g1->numposes;if (frame2 < 0)frame2 += g1->numposes;
			}
			else
			{
				frame1=bound(0, frame1, g1->numposes-1);
				frame2=bound(0, frame2, g1->numposes-1);
			}
		}
		else	//don't bother with a four way lerp. Yeah, this will produce jerkyness with models with just framegroups.
		{
			if (e->model->engineflags&MDLF_NOLERP)
			{
				if (lerp > 0.5)
					g2 = g1;
				else
					g1 = g2;
			}
			//FIXME: find the two poses with the strongest influence.
			frame1=0;
			frame2=0;
		}

#ifndef SERVERONLY
		lerpcutoff = inf->lerpcutoff * r_lerpmuzzlehack.value;
		if (Sh_StencilShadowsActive() || e->fatness || lerpcutoff)
		{
			memset(&meshcache.vbo.coord2, 0, sizeof(meshcache.vbo.coord2));
			mesh->xyz2_array = NULL;
			mesh->xyz_blendw[0] = 1;
			mesh->xyz_blendw[1] = 0;
			R_LerpFrames(mesh,	&g1->poseofs[frame1], &g2->poseofs[frame2], 1-lerp, e->fatness, lerpcutoff);
		}
		else
#endif
		{
			galiaspose_t *p1 = &g1->poseofs[frame1];
#ifndef SERVERONLY
			galiaspose_t *p2 = &g2->poseofs[frame2];
#endif

			meshcache.vbo.indicies = inf->vboindicies;
			meshcache.vbo.indexcount = inf->numindexes;
			meshcache.vbo.vertcount = inf->numverts;
			meshcache.vbo.texcoord = inf->vbotexcoords;
			meshcache.vbo.lmcoord[0] = inf->vbolmtexcoords;

#ifdef SKELETALMODELS
			memset(&meshcache.vbo.bonenums, 0, sizeof(meshcache.vbo.bonenums));
			memset(&meshcache.vbo.boneweights, 0, sizeof(meshcache.vbo.boneweights));
			meshcache.vbo.numbones = 0;
			meshcache.vbo.bones = 0;
#endif

#ifdef SERVERONLY
			mesh->xyz_array = p1->ofsverts;
			mesh->xyz2_array = NULL;
#else
			mesh->normals_array = p1->ofsnormals;
			mesh->snormals_array = p1->ofssvector;
			mesh->tnormals_array = p1->ofstvector;

			meshcache.vbo.normals = p1->vbonormals;
			meshcache.vbo.svector = p1->vbosvector;
			meshcache.vbo.tvector = p1->vbotvector;
			memset(&meshcache.vbo.colours[0], 0, sizeof(meshcache.vbo.colours[0]));

			if (p1 == p2 || r_nolerp.ival)
			{
				meshcache.vbo.coord = p1->vboverts;
				memset(&meshcache.vbo.coord2, 0, sizeof(meshcache.vbo.coord2));
				mesh->xyz_array = p1->ofsverts;
				mesh->xyz2_array = NULL;
			}
			else
			{
				meshcache.vbo.coord = p1->vboverts;
				meshcache.vbo.coord2 = p2->vboverts;
				mesh->xyz_blendw[0] = 1-lerp;
				mesh->xyz_blendw[1] = lerp;
				mesh->xyz_array = p1->ofsverts;
				mesh->xyz2_array = p2->ofsverts;
			}
#endif
			if (vbop && meshcache.vbo.indicies.sysptr)
				*vbop = meshcache.vbop = &meshcache.vbo;
		}
#endif
	}

	meshcache.vbo.vao = 0;
	meshcache.vbo.vaodynamic = ~0;
	meshcache.vbo.vaoenabled = 0;
	meshcache.acoords1 = mesh->xyz_array;
	meshcache.acoords2 = mesh->xyz2_array;
	meshcache.anorm = mesh->normals_array;
	meshcache.anorms = mesh->snormals_array;
	meshcache.anormt = mesh->tnormals_array;
	meshcache.lerp = mesh->xyz_blendw[0];
	if (vbop)
		meshcache.vbop = *vbop;

#ifdef SKELETALMODELS
	meshcache.usebones = usebones;
	if (usebones)
	{
		mesh->bonenums = inf->ofs_skel_idx;
		mesh->boneweights = inf->ofs_skel_weight;
		mesh->bones = meshcache.usebonepose;
		mesh->numbones = inf->numbones;
#ifndef SERVERONLY
		if (meshcache.bonemap != inf->bonemap)
		{
			meshcache.bonemap = inf->bonemap;
			BE_SelectEntity(e);
		}
		if (inf->mappedbones)
		{
			int i;
			for (i = 0; i < inf->mappedbones; i++)
				memcpy(meshcache.gpubones + i*12, meshcache.usebonepose + inf->bonemap[i]*12, sizeof(float)*12);
			meshcache.vbo.numbones = inf->mappedbones;
			meshcache.vbo.bones = meshcache.gpubones;
		}
#endif
	}
#endif

	return true;	//to allow the mesh to be dlighted.
}

#ifndef SERVERONLY
//used by the modelviewer.
//mode 0: wireframe
//mode 1: normal pegs
//mode 2: 2d projection.
void Mod_AddSingleSurface(entity_t *ent, int surfaceidx, shader_t *shader, int mode)
{
	scenetris_t *t;
	vecV_t *posedata = NULL;
	vec3_t *normdata = NULL, tmp;
	int i, j;

	batch_t *batches[SHADER_SORT_COUNT], *b;
	int s;
	mesh_t *m;
	unsigned int meshidx;

	if (!ent->model || ent->model->loadstate != MLS_LOADED)
		return;

	memset(batches, 0, sizeof(batches));
	r_refdef.frustum_numplanes = 0;
	switch(ent->model->type)
	{
	case mod_alias:
		R_GAlias_GenerateBatches(ent, batches);
		break;

#ifdef HALFLIFEMODELS
	case mod_halflife:
		R_HalfLife_GenerateBatches(ent, batches);
		break;
#endif
	default:
		return;
	}

	for (s = 0; s < countof(batches); s++)
	{
		if (!batches[s])
			continue;
		for (b = batches[s]; b; b = b->next)
		{
			if (b->buildmeshes)
				b->buildmeshes(b);

			for (meshidx = b->firstmesh; meshidx < b->meshes; meshidx++)
			{
				if (surfaceidx < 0)
				{	//only draw meshes that have an actual contents value (collision data)
					//FIXME: implement.
				}
				else
				{	//only draw the mesh that's actually selected.
					if (b->user.alias.surfrefs[meshidx] != surfaceidx)
						continue;
				}

				m = b->mesh[meshidx];

				if (mode == 2 && m->st_array)
				{	//2d wireframe (using texture coords instead of modelspace)
					if (cl_numstris == cl_maxstris)
					{
						cl_maxstris+=8;
						cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
					}
					t = &cl_stris[cl_numstris++];
					t->shader = shader;
					t->flags = BEF_LINES;
					t->firstidx = cl_numstrisidx;
					t->firstvert = cl_numstrisvert;
					if (t->flags&BEF_LINES)
						t->numidx = m->numindexes*2;
					else
						t->numidx = m->numindexes;
					t->numvert = m->numvertexes;

					if (cl_numstrisidx+t->numidx > cl_maxstrisidx)
					{
						cl_maxstrisidx=cl_numstrisidx+t->numidx;
						cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
					}
					if (cl_numstrisvert+m->numvertexes > cl_maxstrisvert)
						cl_stris_ExpandVerts(cl_numstrisvert+m->numvertexes);
					for (i = 0; i < m->numvertexes; i++)
					{
						VectorMA(vec3_origin,	m->st_array[i][0],	ent->axis[0], tmp);
						VectorMA(tmp,			m->st_array[i][1],	ent->axis[1], tmp);
						VectorMA(tmp,			0,					ent->axis[2], tmp);
						VectorMA(ent->origin,	ent->scale,			tmp,		  cl_strisvertv[t->firstvert+i]);

						Vector2Set(cl_strisvertt[t->firstvert+i], 0.5, 0.5);
						Vector4Set(cl_strisvertc[t->firstvert+i], 1, 1, 1, 0.1);
					}
					if (t->flags&BEF_LINES)
					{
						for (i = 0; i < m->numindexes; i+=3)
						{
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+0];
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+1];
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+1];
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+2];
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+2];
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+0];
						}
					}
					else
					{
						for (i = 0; i < m->numindexes; i++)
							cl_strisidx[cl_numstrisidx+i] = m->indexes[i];
						cl_numstrisidx += m->numindexes;
					}
					cl_numstrisvert += m->numvertexes;
					continue;
				}

				posedata = m->xyz_array;
				normdata = (mode==1)?m->normals_array:NULL;
#ifdef SKELETALMODELS
				if (m->numbones)
				{	//intended shader might have caused it to use skeletal stuff.
					//we're too lame for that though.
					posedata = alloca(m->numvertexes*sizeof(vecV_t));
					if (normdata)
					{
						normdata = alloca(m->numvertexes*sizeof(vec3_t));
						Alias_TransformVerticies_VN(m->bones, m->numvertexes, m->bonenums[0], m->boneweights[0],	m->xyz_array[0], posedata[0], m->normals_array[0], normdata[0]);
					}
					else
						Alias_TransformVerticies_V(m->bones, m->numvertexes, m->bonenums[0], m->boneweights[0],		m->xyz_array[0], posedata[0]);
				}
				else
#endif
				{
					if (m->xyz_blendw[1] == 1 && m->xyz2_array)
						posedata = m->xyz2_array;
					else if (m->xyz_blendw[0] != 1 && m->xyz2_array)
					{
						posedata = alloca(m->numvertexes*sizeof(vecV_t));
						for (i = 0; i < m->numvertexes; i++)
						{
							for (j = 0; j < 3; j++)
								posedata[i][j] =	m->xyz_array[i][j] * m->xyz_blendw[0] +
													m->xyz2_array[i][j] * m->xyz_blendw[1];
						}
					}
					else
						posedata = m->xyz_array;
				}
				if (normdata)
				{	//show small pegs at each vertex
					if (cl_numstris == cl_maxstris)
					{
						cl_maxstris+=8;
						cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
					}
					t = &cl_stris[cl_numstris++];
					t->shader = shader;
					t->flags = BEF_LINES;
					t->firstidx = cl_numstrisidx;
					t->firstvert = cl_numstrisvert;
					t->numidx = t->numvert = m->numvertexes*2;

					if (cl_numstrisidx+t->numidx > cl_maxstrisidx)
					{
						cl_maxstrisidx=cl_numstrisidx+t->numidx;
						cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
					}
					if (cl_numstrisvert+t->numvert > cl_maxstrisvert)
						cl_stris_ExpandVerts(cl_numstrisvert+t->numvert);
					for (i = 0; i < m->numvertexes; i++)
					{
						VectorMA(vec3_origin,	posedata[i][0], ent->axis[0], tmp);
						VectorMA(tmp,			posedata[i][1], ent->axis[1], tmp);
						VectorMA(tmp,			posedata[i][2], ent->axis[2], tmp);
						VectorMA(ent->origin,	ent->scale,		tmp,		  cl_strisvertv[t->firstvert+i*2+0]);

						VectorMA(tmp,			normdata[i][0], ent->axis[0], tmp);
						VectorMA(tmp,			normdata[i][1], ent->axis[1], tmp);
						VectorMA(tmp,			normdata[i][2], ent->axis[2], tmp);
						VectorMA(ent->origin,	ent->scale,		tmp,		  cl_strisvertv[t->firstvert+i*2+1]);

						Vector2Set(cl_strisvertt[t->firstvert+i*2+0], 0.0, 0.0);
						Vector2Set(cl_strisvertt[t->firstvert+i*2+1], 1.0, 1.0);
						Vector4Set(cl_strisvertc[t->firstvert+i*2+0], 0, 0, 1, 1);
						Vector4Set(cl_strisvertc[t->firstvert+i*2+1], 0, 0, 1, 1);

						cl_strisidx[cl_numstrisidx+i*2+0] = i*2+0;
						cl_strisidx[cl_numstrisidx+i*2+1] = i*2+1;
					}
					cl_numstrisidx += i*2;
					cl_numstrisvert += i*2;
				}
				else if (mode == 0)
				{	//regular wireframe
					if (cl_numstris == cl_maxstris)
					{
						cl_maxstris+=8;
						cl_stris = BZ_Realloc(cl_stris, sizeof(*cl_stris)*cl_maxstris);
					}
					t = &cl_stris[cl_numstris++];
					t->shader = shader;
					t->flags = 0;//BEF_LINES;
					t->firstidx = cl_numstrisidx;
					t->firstvert = cl_numstrisvert;
					if (t->flags&BEF_LINES)
						t->numidx = m->numindexes*2;
					else
						t->numidx = m->numindexes;
					t->numvert = m->numvertexes;

					if (cl_numstrisidx+t->numidx > cl_maxstrisidx)
					{
						cl_maxstrisidx=cl_numstrisidx+t->numidx;
						cl_strisidx = BZ_Realloc(cl_strisidx, sizeof(*cl_strisidx)*cl_maxstrisidx);
					}
					if (cl_numstrisvert+m->numvertexes > cl_maxstrisvert)
						cl_stris_ExpandVerts(cl_numstrisvert+m->numvertexes);
					for (i = 0; i < m->numvertexes; i++)
					{
						VectorMA(vec3_origin,	posedata[i][0], ent->axis[0], tmp);
						VectorMA(tmp,			posedata[i][1], ent->axis[1], tmp);
						VectorMA(tmp,			posedata[i][2], ent->axis[2], tmp);
						VectorMA(ent->origin,	ent->scale,		tmp,		  cl_strisvertv[t->firstvert+i]);

						Vector2Set(cl_strisvertt[t->firstvert+i], 0.5, 0.5);
						Vector4Set(cl_strisvertc[t->firstvert+i], 1, 1, 1, 0.1);
					}
					if (t->flags&BEF_LINES)
					{
						for (i = 0; i < m->numindexes; i+=3)
						{
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+0];
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+1];
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+1];
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+2];
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+2];
							cl_strisidx[cl_numstrisidx++] = m->indexes[i+0];
						}
					}
					else
					{
						for (i = 0; i < m->numindexes; i++)
							cl_strisidx[cl_numstrisidx+i] = m->indexes[i];
						cl_numstrisidx += m->numindexes;
					}
					cl_numstrisvert += m->numvertexes;
				}
			}
		}
	}
}
#endif


static float PlaneNearest(const vec3_t normal, const vec3_t mins, const vec3_t maxs)
{
	float result;
#if 0
	result  = fabs(normal[0] * maxs[0]);
	result += fabs(normal[1] * maxs[1]);
	result += fabs(normal[2] * maxs[2]);
#elif 0
	result  = normal[0] * ((normal[0] > 0)?-16:16);
	result += normal[1] * ((normal[1] > 0)?-16:16);
	result += normal[2] * ((normal[2] > 0)?-24:32);
#else
	result  = normal[0] * ((normal[0] > 0)?mins[0]:maxs[0]);
	result += normal[1] * ((normal[1] > 0)?mins[1]:maxs[1]);
	result += normal[2] * ((normal[2] > 0)?mins[2]:maxs[2]);
#endif
	return result;
}

void CLQ1_DrawLine(shader_t *shader, vec3_t v1, vec3_t v2, float r, float g, float b, float a);
static qboolean Mod_Trace_Trisoup(vecV_t *posedata, index_t *indexes, int numindexes, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, trace_t *fte_restrict trace)
{
	qboolean impacted = false;
	int i, j;

	float *p1, *p2, *p3;
	vec3_t edge1, edge2, edge3;
	vec3_t normal;
	vec3_t edgenormal;

	float planedist;
	float diststart, distend;
	float mn,mx;
	float extend;

	float frac;

	vec3_t impactpoint;

	for (i = 0; i < numindexes; i+=3)
	{
		p1 = posedata[indexes[i+0]];
		p2 = posedata[indexes[i+1]];
		p3 = posedata[indexes[i+2]];

#if 0
		{
			shader_t *lineshader = R_RegisterShader("lineshader", SUF_NONE,
						"{\n"
							"polygonoffset\n"
							"{\n"
								"map $whiteimage\n"
								"blendfunc add\n"
								"rgbgen vertex\n"
								"alphagen vertex\n"
							"}\n"
						"}\n");
			VectorAdd(p1, r_refdef.pvsorigin, edge1);
			VectorAdd(p2, r_refdef.pvsorigin, edge2);
			VectorAdd(p3, r_refdef.pvsorigin, edge3);
			CLQ1_DrawLine(lineshader, edge1, edge2, 0, 1, 0, 1);
			CLQ1_DrawLine(lineshader, edge2, edge3, 0, 1, 0, 1);
			CLQ1_DrawLine(lineshader, edge3, edge1, 0, 1, 0, 1);
		}
#endif

		VectorSubtract(p1, p2, edge1);
		VectorSubtract(p3, p2, edge2);
		CrossProduct(edge1, edge2, normal);
		VectorNormalize(normal);

		//degenerate triangle
		if (!normal[0] && !normal[1] && !normal[2])
			continue;

		//debugging
//		if (normal[2] != 1)
//			continue;

#define	DIST_EPSILON	(0.03125)
#define DIST_SOLID		(3/8.0)	//the plane must be at least this thick, or player prediction will try jittering through it to correct the player's origin
		extend = PlaneNearest(normal, mins, maxs);
		planedist = DotProduct(p1, normal)-extend;
		diststart = DotProduct(start, normal);
		if (diststart/*+extend+DIST_SOLID*/ < planedist)
			continue;	//start on back side (or slightly inside).
		distend = DotProduct(end, normal);
		if (distend > planedist)
			continue;	//end on front side.

		//figure out the precise frac
		if (diststart > planedist)
		{
			//if we're not stuck inside it
			if (distend >= diststart)
				continue;	//trace moves away from or along the surface. don't block the trace if we're sliding along the front of it.
		}
		frac = (diststart - planedist) / (diststart-distend);
		if (frac >= trace->truefraction)	//already found one closer.
			continue;

		//an impact outside of the surface's bounding box (expanded by the trace bbox) is not a valid impact.
		//this solves extrusion issues.
		for (j = 0; j < 3; j++)
		{
			impactpoint[j] = start[j] + frac*(end[j] - start[j]);
			//make sure the impact point is within the triangle's bbox.
			//primarily, this serves to prevent the edge extruding off to infinity or so
			mx = mn = p1[j];
			if (mn > p2[j])
				mn = p2[j];
			if (mx < p2[j])
				mx = p2[j];
			if (mn > p3[j])
				mn = p3[j];
			if (mx < p3[j])
				mx = p3[j];
			mx-=mins[j]-DIST_EPSILON;
			mn-=maxs[j]+DIST_EPSILON;
			if (impactpoint[j] > mx)
				break;
			if (impactpoint[j] < mn)
				break;
		}
		if (j < 3)
			continue;


		//make sure the impact point is actually within the triangle
		CrossProduct(edge1, normal, edgenormal);
		VectorNormalize(edgenormal);
		if (DotProduct(impactpoint, edgenormal) > DotProduct(p2, edgenormal)-PlaneNearest(edgenormal, mins, maxs)+DIST_EPSILON)
			continue;

		CrossProduct(normal, edge2, edgenormal);
		VectorNormalize(edgenormal);
		if (DotProduct(impactpoint, edgenormal) > DotProduct(p3, edgenormal)-PlaneNearest(edgenormal, mins, maxs)+DIST_EPSILON)
			continue;

		VectorSubtract(p1, p3, edge3);
		CrossProduct(normal, edge3, edgenormal);
		VectorNormalize(edgenormal);
		if (DotProduct(impactpoint, edgenormal) > DotProduct(p1, edgenormal)-PlaneNearest(edgenormal, mins, maxs)+DIST_EPSILON)
			continue;

		//okay, its a valid impact
		trace->truefraction = frac;

		//move back from the impact point. this should keep the point slightly outside of the solid triangle.
		frac = (diststart - (planedist+DIST_EPSILON)) / (diststart-distend);
		if (frac < 0)
		{	//we're inside, apparently
			trace->startsolid = trace->allsolid = (diststart < planedist);
			trace->fraction = 0;
			VectorCopy(start, trace->endpos);
		}
		else
		{
			//we made progress
			trace->fraction = frac;
			trace->endpos[0] = start[0] + frac*(end[0] - start[0]);
			trace->endpos[1] = start[1] + frac*(end[1] - start[1]);
			trace->endpos[2] = start[2] + frac*(end[2] - start[2]);
		}
		VectorCopy(normal, trace->plane.normal);
		trace->plane.dist = planedist;
		trace->triangle_id = 1+i/3;
		impacted = true;

//		if (fabs(normal[0]) != 1 && fabs(normal[1]) != 1 && fabs(normal[2]) != 1)
//			Con_Printf("Non-axial impact\n");

#if 0
		{
			shader_t *lineshader = R_RegisterShader("lineshader", SUF_NONE,
						"{\n"
							"polygonoffset\n"
							"{\n"
								"map $whiteimage\n"
								"blendfunc add\n"
								"rgbgen vertex\n"
								"alphagen vertex\n"
							"}\n"
						"}\n");
			VectorAdd(p1, r_refdef.pvsorigin, edge1);
			VectorAdd(p2, r_refdef.pvsorigin, edge2);
			VectorAdd(p3, r_refdef.pvsorigin, edge3);
			CLQ1_DrawLine(lineshader, edge1, edge2, 0, 1, 0, 1);
			CLQ1_DrawLine(lineshader, edge2, edge3, 0, 1, 0, 1);
			CLQ1_DrawLine(lineshader, edge3, edge1, 0, 1, 0, 1);
		}
#endif
	}
	return impacted;
}

//The whole reason why model loading is supported in the server.
static qboolean Mod_Trace(model_t *model, int forcehullnum, const framestate_t *framestate, const vec3_t axis[3], const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, qboolean capsule, unsigned int contentsmask, trace_t *trace)
{
	galiasinfo_t *mod = Mod_Extradata(model);

//	float temp;

	vecV_t *posedata = NULL;
	index_t *indexes;
	int surfnum = 0;
#ifdef SKELETALMODELS
	int cursurfnum = -1, curbonesurf = -1;
	float buffer[MAX_BONES*12];
	float bufferalt[MAX_BONES*12];
	const float *bonepose = NULL;
#endif

	vec3_t start_l, end_l;

	if (axis)
	{
		start_l[0] = DotProduct(start, axis[0]);
		start_l[1] = DotProduct(start, axis[1]);
		start_l[2] = DotProduct(start, axis[2]);
		end_l[0] = DotProduct(end, axis[0]);
		end_l[1] = DotProduct(end, axis[1]);
		end_l[2] = DotProduct(end, axis[2]);
	}
	else
	{
		VectorCopy(start, start_l);
		VectorCopy(end, end_l);
	}

	trace->fraction = trace->truefraction = 1;

	for(; mod; mod = mod->nextsurf, surfnum++)
	{
		if (!(mod->contents & contentsmask))
			continue;	//this mesh isn't solid to the trace

		indexes = mod->ofs_indexes;
#ifdef SKELETALMODELS
		if (mod->ofs_skel_xyz)
		{
			if (!mod->ofs_skel_idx || !framestate || !mod->numbones)
				posedata = mod->ofs_skel_xyz;	//if there's no weights, don't try animating anything.
			else if (mod->shares_verts != cursurfnum || !posedata)
			{
				cursurfnum = mod->shares_verts;
				if (curbonesurf != mod->shares_bones)
				{
					curbonesurf = mod->shares_bones;
					bonepose = Alias_GetBoneInformation(mod, framestate, SKEL_INVERSE_ABSOLUTE, buffer, bufferalt, MAX_BONES, NULL);
				}
				posedata = alloca(mod->numverts*sizeof(vecV_t));
				Alias_TransformVerticies_V(bonepose, mod->numverts, mod->ofs_skel_idx[0], mod->ofs_skel_weight[0], mod->ofs_skel_xyz[0], posedata[0]);
			}
			//else posedata = posedata;
		}
		else 
#endif
#ifndef NONSKELETALMODELS
			continue;
#else
		if (!mod->numanimations)
			continue;
		else
		{
			galiaspose_t *pose;
			galiasanimation_t *group = mod->ofsanimations;
			if (framestate)
				group += framestate->g[FS_REG].frame[0] % mod->numanimations;
			//FIXME: no support for frame blending.
			if (!group->numposes)
				continue;
			pose = group->poseofs;
			if (!pose)
				continue;	//error...
			if (framestate)
				pose += (int)(framestate->g[FS_REG].frametime[0] * group->rate)%group->numposes;
			posedata = pose->ofsverts;
		}
#endif

		if (Mod_Trace_Trisoup(posedata, indexes, mod->numindexes, start_l, end_l, mins, maxs, trace))
		{
			trace->contents = mod->contents;
			trace->surface = &mod->csurface;
			trace->surface_id = mod->surfaceid;
			trace->bone_id = 0;
#ifdef SKELETALMODELS
			if (mod->ofs_skel_weight)
			{	//fixme: would be better to consider the distance to the vertex too. cartesian coord stuff etc.
				unsigned int v, w, i;
				float bw = 0;
				for (i = 0; i < 3; i++)
				{
					for (v = indexes[(trace->triangle_id-1)*3+i], w = 0; w < 4; w++)
					{
						if (bw < mod->ofs_skel_weight[v][w])
						{
							bw = mod->ofs_skel_weight[v][w];
							trace->bone_id = 1 + mod->ofs_skel_idx[v][w];
						}
					}
				}
			}
#endif
			if (axis)
			{
				vec3_t iaxis[3];
				vec3_t norm;
				Matrix3x3_RM_Invert_Simple((const void *)axis, iaxis);
				VectorCopy(trace->plane.normal, norm);
				trace->plane.normal[0] = DotProduct(norm, iaxis[0]);
				trace->plane.normal[1] = DotProduct(norm, iaxis[1]);
				trace->plane.normal[2] = DotProduct(norm, iaxis[2]);

//				frac = traceinfo.truefraction;
				/*
				diststart = DotProduct(traceinfo.start, trace->plane.normal);
				distend = DotProduct(traceinfo.end, trace->plane.normal);
				if (diststart == distend)
					frac = 0;
				else
				{
					frac = (diststart - trace->plane.dist) / (diststart-distend);
					if (frac < 0)
						frac = 0;
					else if (frac > 1)
						frac = 1;
				}*/

				/*okay, this is where it hits this plane*/
				trace->endpos[0] = start[0] + trace->fraction*(end[0] - start[0]);
				trace->endpos[1] = start[1] + trace->fraction*(end[1] - start[1]);
				trace->endpos[2] = start[2] + trace->fraction*(end[2] - start[2]);
			}
		}
	}

	trace->allsolid = false;

	return trace->fraction != 1;
}

static unsigned int Mod_Mesh_PointContents(struct model_s *model, const vec3_t axis[3], const vec3_t p)
{	//trisoup doesn't have any actual volumes, thus we can't report anything...
	return 0;
}
static int Mod_Mesh_ClusterForPoint(struct model_s *model, const vec3_t point, int *areaout)
{	//trisoup doesn't have any actual pvs, thus we can't report anything...
	if (areaout)
		*areaout = 0;
	return -1;
}
static qbyte *Mod_Mesh_ClusterPVS(struct model_s *model, int cluster, pvsbuffer_t *pvsbuffer, pvsmerge_t merge)
{	//trisoup doesn't have any actual pvs, thus we can't report anything...
	return NULL;
}
static unsigned int Mod_Mesh_FatPVS(struct model_s *model, const vec3_t org, pvsbuffer_t *pvsbuffer, qboolean merge)
{	//trisoup doesn't have any actual pvs, thus we can't report anything...
	return 0;
}
qboolean Mod_Mesh_EdictInFatPVS(struct model_s *model, const struct pvscache_s *edict, const qbyte *pvs, const int *areas)
{	//trisoup doesn't have any actual pvs, thus we always report visible
	return true;
}
void Mod_Mesh_FindTouchedLeafs(struct model_s *model, struct pvscache_s *ent, const vec3_t cullmins, const vec3_t cullmaxs)
{
}
static void Mod_Mesh_LightPointValues(struct model_s *model, const vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{	//trisoup doesn't have any actual pvs, thus we can't report anything...
	VectorSet(res_diffuse, 255,255,255);
	VectorSet(res_ambient, 128,128,128);
	VectorSet(res_dir, 0,0,1);
}
static void Mod_SetMeshModelFuncs(model_t *mod, qboolean isstatic)
{
	if (!isstatic)
	{
		mod->funcs.NativeTrace = Mod_Trace;
		return;
	}

//	void (*PurgeModel) (struct model_s *mod);

	mod->funcs.PointContents = Mod_Mesh_PointContents;
//	unsigned int (*BoxContents)		(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p, const vec3_t mins, const vec3_t maxs);

	//deals with whatever is native for the bsp (gamecode is expected to distinguish this).
	mod->funcs.NativeTrace = Mod_Trace;
//	unsigned int (*NativeContents)(struct model_s *model, int hulloverride, const framestate_t *framestate, const vec3_t axis[3], const vec3_t p, const vec3_t mins, const vec3_t maxs);

	mod->funcs.FatPVS = Mod_Mesh_FatPVS;
	mod->funcs.EdictInFatPVS = Mod_Mesh_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs = Mod_Mesh_FindTouchedLeafs;

	mod->funcs.LightPointValues = Mod_Mesh_LightPointValues;
//	void (*StainNode)			(struct mnode_s *node, float *parms);
//	void (*MarkLights)			(struct dlight_s *light, dlightbitmask_t bit, struct mnode_s *node);

	mod->funcs.ClusterForPoint = Mod_Mesh_ClusterForPoint;
	mod->funcs.ClusterPVS = Mod_Mesh_ClusterPVS;
//	qbyte *(*ClustersInSphere)	(struct model_s *model, const vec3_t point, float radius, pvsbuffer_t *pvsbuffer, const qbyte *fte_restrict unionwith);

	BIH_BuildAlias(mod, mod->meshinfo);
}


static void Mod_ClampModelSize(model_t *mod)
{
#ifndef SERVERONLY
	int i;

	float rad=0, axis;
	axis = (mod->maxs[0] - mod->mins[0]);
	rad += axis*axis;
	axis = (mod->maxs[1] - mod->mins[1]);
	rad += axis*axis;
	axis = (mod->maxs[2] - mod->mins[2]);
	rad += axis*axis;

	mod->tainted = false;
	if (mod->engineflags & MDLF_DOCRC)
	{
		if (!strcmp(mod->name, "progs/eyes.mdl"))
		{	//this is checked elsewhere to make sure the crc matches (this is to make sure the crc check was actually called)
			if (mod->type != mod_alias || mod->fromgame != fg_quake || mod->flags)
				mod->tainted = true;
		}
	}

	mod->clampscale = 1;
	for (i = 0; i < sizeof(clampedmodel)/sizeof(clampedmodel[0]); i++)
	{
		if (!strcmp(mod->name, clampedmodel[i].name))
		{
			if (rad > clampedmodel[i].furthestallowedextremety)
			{
				axis = clampedmodel[i].furthestallowedextremety;
				mod->clampscale = axis/rad;
				Con_DPrintf("\"%s\" will be clamped.\n", mod->name);
			}
			return;
		}
	}

//	Con_DPrintf("Don't know what size to clamp \"%s\" to (size:%f).\n", mod->name, rad);
#endif
}

#ifdef NONSKELETALMODELS
#ifndef SERVERONLY
static int R_FindTriangleWithEdge (index_t *indexes, int numtris, int start, int end, int ignore)
{
	int i;
	int match, count;

	count = 0;
	match = -1;

	for (i = 0; i < numtris; i++, indexes += 3)
	{
		if ( (indexes[0] == start && indexes[1] == end)
			|| (indexes[1] == start && indexes[2] == end)
			|| (indexes[2] == start && indexes[0] == end) ) {
			if (i != ignore)
				match = i;
			count++;
		} else if ( (indexes[1] == start && indexes[0] == end)
			|| (indexes[2] == start && indexes[1] == end)
			|| (indexes[0] == start && indexes[2] == end) ) {
			count++;
		}
	}

	// detect edges shared by three triangles and make them seams
	if (count > 2)
		match = -1;

	return match;
}
#endif

static void Mod_CompileTriangleNeighbours(model_t *loadmodel, galiasinfo_t *galias)
{
#ifndef SERVERONLY
	if (Sh_StencilShadowsActive())
	{
		int i, *n;
		index_t *index;
		index_t *indexes = galias->ofs_indexes;
		int numtris = galias->numindexes/3;
		int *neighbours;
		neighbours = ZG_Malloc(&loadmodel->memgroup, sizeof(int)*numtris*3);
		galias->ofs_trineighbours = neighbours;

		for (i = 0, index = indexes, n = neighbours; i < numtris; i++, index += 3, n += 3)
		{
			n[0] = R_FindTriangleWithEdge (indexes, numtris, index[1], index[0], i);
			n[1] = R_FindTriangleWithEdge (indexes, numtris, index[2], index[1], i);
			n[2] = R_FindTriangleWithEdge (indexes, numtris, index[0], index[2], i);
		}
	}
#endif
}
#endif

#define MAX_FRAMEINFO_POSES 256

typedef struct
{
	unsigned int poses[MAX_FRAMEINFO_POSES];
	qboolean posesarray;
	unsigned int firstpose;
	unsigned int posecount;
	float fps;
	qboolean loop;
	int action;
	galiasevent_t *events;
	float actionweight;
	char name[MAX_QPATH];
} frameinfo_t;
static frameinfo_t *ParseFrameInfo(model_t *mod, int *numgroups)
{
	const char *modelname = mod->name;

	int count = 0;
	int maxcount = 0;
	char *line, *eol;
	char *file;
	frameinfo_t *frames = NULL;
	char fname[MAX_QPATH];
	char tok[MAX_FRAMEINFO_POSES * 4];
	size_t fsize;
	com_tokentype_t ttype;
	json_t *rootjson;
	Q_snprintfz(fname, sizeof(fname), "%s.framegroups", modelname);
	line = file = FS_LoadMallocFile(fname, &fsize);
	if (!file)
		return NULL;

	rootjson = JSON_Parse(file);	//must be a fully valid json file, so any space-separated tokens from the dp format will return NULL here just fine.
	if (rootjson)
	{
		json_t *framegroups = JSON_FindChild(rootjson, "framegroups");
		maxcount = JSON_GetCount(framegroups);
		frames = realloc(frames, sizeof(*frames)*maxcount);
		for(count = 0; count < maxcount; count++)
		{
			galiasevent_t *ev, **link;
			char eventdata[65536];
			unsigned int posecount;
			json_t *arr, *c;
			json_t *in = JSON_GetIndexed(framegroups, count);
			if (!in)
				break;	//erk? shouldn't really happen. not an issue though.

			frames[count].firstpose = JSON_GetInteger(in, "firstpose", 0);
			frames[count].posecount = JSON_GetInteger(in, "numposes", 1);
			frames[count].posesarray = false;
			arr = JSON_FindChild(in, "poses");
			if (arr)
			{	//override with explicit poses, if specified.
				for (posecount = 0; posecount < countof(frames[count].poses); posecount++)
				{
					c = JSON_GetIndexed(arr, posecount);
					if (!c)	//ran out of elements...
						break;
					frames[count].poses[posecount] = JSON_GetUInteger(c, NULL, 0);
				}
				if (posecount>0)
				{
					frames[count].posecount = posecount;
					frames[count].posesarray = true;
				}
			}

			frames[count].fps = JSON_GetFloat(in, "fps", 20);
			if (frames[count].fps <= 0)
				frames[count].fps = 10;
			frames[count].loop = JSON_GetUInteger(in, "loop", false);

			Q_snprintfz(frames[count].name,sizeof(frames[count].name), "%s[%d]", fname, count);
			JSON_GetString(in, "name", frames[count].name, sizeof(frames[count].name), NULL);

			frames[count].action = JSON_GetInteger(in, "action", -1);
			frames[count].actionweight = JSON_GetFloat(in, "actionweight", 0);
			frames[count].events = NULL;

			arr = JSON_FindChild(in, "events");
			for (posecount = 0; (c=JSON_GetIndexed(arr, posecount))!=NULL; posecount++)
			{
				*eventdata = 0;
				JSON_GetString(c, "value", eventdata,sizeof(eventdata), NULL);
				ev = ZG_Malloc(&mod->memgroup, sizeof(*ev) + strlen(eventdata)+1);
				ev->code = JSON_GetInteger(c, "code", 0);
				ev->timestamp = JSON_GetFloat(c, "timestamp", JSON_GetFloat(c, "pose", 0)/frames[count].fps);
				ev->data = strcpy((char*)(ev+1), eventdata);

				link = &frames[count].events;
				while (*link && (*link)->timestamp <= ev->timestamp)
					link = &(*link)->next;
				ev->next = *link;
				*link = ev;
			}
		}

		mod->flags = JSON_GetUInteger(rootjson, "modelflags", mod->flags);


		JSON_Destroy(rootjson);
	}
	else while(line && *line)
	{
		unsigned int posecount = 0;

		eol = strchr(line, '\n');
		if (eol)
			*eol = 0;
			
		if (count == maxcount)
		{
			maxcount += 32;
			frames = realloc(frames, sizeof(*frames)*maxcount);
		}

		line = COM_ParseOut(line, tok, sizeof(tok));
		// Check if firstpose is actually a sequence of comma separated poses, e.g.: 42,43,44,43,42
		if (strchr(tok, ','))
		{
			char pose[64], *ptok = tok;

			for (; posecount < MAX_FRAMEINFO_POSES; posecount++)
			{
				ptok = COM_ParseStringSetSep(ptok, ',', pose, sizeof(pose));
				if (!pose[0])
					break;
				frames[count].poses[posecount] = atoi(pose);
			}
		}
		frames[count].posesarray = !!posecount;
		frames[count].firstpose = posecount ? 0 : atoi(tok);
		line = COM_ParseOut(line, tok, sizeof(tok));
		frames[count].posecount = posecount ? posecount : atoi(tok);
		line = COM_ParseOut(line, tok, sizeof(tok));
		frames[count].fps = atof(tok);
		line = COM_ParseOut(line, tok, sizeof(tok));
		if (!strcmp(tok, "true") || !strcmp(tok, "yes") || !strcmp(tok, "on"))
			frames[count].loop = true;
		else
			frames[count].loop = !!atoi(tok);

		frames[count].events = NULL;
		frames[count].action = -1;
		frames[count].actionweight = 0;
		Q_snprintfz(frames[count].name, sizeof(frames[count].name), "groupified_%d_anim", count);	//to match DP. frameforname cares.

		line = COM_ParseType(line, tok, sizeof(tok), &ttype);
		if (ttype != TTP_EOF)
		{
			Q_strncpyz(frames[count].name, tok, sizeof(frames[count].name));
			line = COM_ParseType(line, tok, sizeof(tok), &ttype);
		}
		if (ttype != TTP_EOF)
		{
			frames[count].action = atoi(tok);
			line = COM_ParseType(line, tok, sizeof(tok), &ttype);
		}
		if (ttype != TTP_EOF)
			frames[count].actionweight = atoi(tok);

		if (frames[count].posecount>0 && frames[count].fps)
			count++;

		if (eol)
			line = eol+1;
		else
			break;
	}
	BZ_Free(file);

	*numgroups = count;
	if (!count)
	{
		free(frames);
		frames = NULL;
	}
	return frames;
}

//parses a foo.mdl.events file and inserts the events into the relevant animations
static void Mod_InsertEvent(zonegroup_t *mem, galiasanimation_t *anims, unsigned int numanimations, unsigned int eventanimation, float eventpose, int eventcode, const char *eventdata)
{
	galiasevent_t *ev, **link;
	if (eventanimation >= numanimations)
	{
		Con_Printf("Mod_InsertEvent: invalid frame index\n");
		return;
	}
	ev = ZG_Malloc(mem, sizeof(*ev) + strlen(eventdata)+1);
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
	line = file = FS_LoadMallocFile(fname, &fsize);
	if (!file)
		return false;
	while(line && *line)
	{
		eol = strchr(line, '\n');
		if (eol)
			*eol = 0;

		line = COM_ParseOut(line, tok, sizeof(tok));
		anim = strtoul(tok, NULL, 0);
		line = COM_ParseOut(line, tok, sizeof(tok));
		pose = strtod(tok, NULL);
		line = COM_ParseOut(line, tok, sizeof(tok));
		eventcode = (long)strtol(tok, NULL, 0);
		line = COM_ParseOut(line, tok, sizeof(tok));
		Mod_InsertEvent(mem, anims, numanimations, anim, pose, eventcode, tok);

		if (eol)
			line = eol+1;
		else
			break;
	}
	BZ_Free(file);
	return true;
}

static void Mod_DefaultMesh(galiasinfo_t *galias, const char *name, unsigned int index)
{
	Q_strncpyz(galias->surfacename, name, sizeof(galias->surfacename));
	Q_strncpyz(galias->csurface.name, COM_SkipPath(name), sizeof(galias->csurface.name));
	galias->contents = FTECONTENTS_BODY;
	galias->geomset = ~0;	//invalid set = always visible
	galias->geomid = 0;
	galias->mindist = 0;
	galias->maxdist = 0;
	galias->surfaceid = index+1;
}

void Mod_DestroyMesh(galiasinfo_t *galias)
{
#ifndef SERVERONLY
	if (!qrenderer || !BE_VBO_Destroy)
		return;
	while(galias)
	{
		BE_VBO_Destroy(&galias->vbotexcoords, galias->vbomem);
		BE_VBO_Destroy(&galias->vboindicies, galias->ebomem);
		galias = galias->nextsurf;
	}
#endif
}

#ifndef SERVERONLY
static void Mod_GenerateMeshVBO(model_t *mod, galiasinfo_t *galias)
//vec3_t *vc, vec2_t *tc, vec3_t *nv, vec3_t *sv, vec3_t *tv, index_t *idx, int numidx, int numverts)
{
#ifdef NONSKELETALMODELS
	int i, p;
	galiasanimation_t *group = galias->ofsanimations;
#endif
	int vbospace = 0;
	vbobctx_t vboctx;

	//don't fail on dedicated servers
	if (!BE_VBO_Begin || !galias->numverts)
		return;

	//determine the amount of space we need for our vbos.
	if (galias->ofs_st_array)
		vbospace += sizeof(*galias->ofs_st_array) * galias->numverts;
	if (galias->ofs_lmst_array)
		vbospace += sizeof(*galias->ofs_lmst_array) * galias->numverts;
	if (galias->ofs_rgbaf)
		vbospace += sizeof(*galias->ofs_rgbaf) * galias->numverts;
	else if (galias->ofs_rgbaub)
		vbospace += sizeof(*galias->ofs_rgbaub) * galias->numverts;
#ifdef SKELETALMODELS
	if (galias->ofs_skel_xyz)
		vbospace += sizeof(*galias->ofs_skel_xyz) * galias->numverts;
	if (galias->ofs_skel_norm)
		vbospace += sizeof(*galias->ofs_skel_norm) * galias->numverts;
	if (galias->ofs_skel_svect)
		vbospace += sizeof(*galias->ofs_skel_svect) * galias->numverts;
	if (galias->ofs_skel_tvect)
		vbospace += sizeof(*galias->ofs_skel_tvect) * galias->numverts;
	if (galias->ofs_skel_idx)
		vbospace += sizeof(*galias->ofs_skel_idx) * galias->numverts;
	if (galias->ofs_skel_weight)
		vbospace += sizeof(*galias->ofs_skel_weight) * galias->numverts;
#endif
#ifdef NONSKELETALMODELS
	for (i = 0; i < galias->numanimations; i++)
	{
		if (group[i].poseofs)
			vbospace += group[i].numposes * galias->numverts * (sizeof(vecV_t)+sizeof(vec3_t)*3);
	}
#endif

	BE_VBO_Begin(&vboctx, vbospace);
	if (galias->ofs_st_array)
		BE_VBO_Data(&vboctx, galias->ofs_st_array, sizeof(*galias->ofs_st_array) * galias->numverts, &galias->vbotexcoords);
	if (galias->ofs_lmst_array)
		BE_VBO_Data(&vboctx, galias->ofs_lmst_array, sizeof(*galias->ofs_lmst_array) * galias->numverts, &galias->vbolmtexcoords);
	if (galias->ofs_rgbaf)
		BE_VBO_Data(&vboctx, galias->ofs_rgbaf, sizeof(*galias->ofs_rgbaf) * galias->numverts, &galias->vborgba);
	else if (galias->ofs_rgbaub)
		BE_VBO_Data(&vboctx, galias->ofs_rgbaub, sizeof(*galias->ofs_rgbaub) * galias->numverts, &galias->vborgba);
#ifdef SKELETALMODELS
	if (galias->ofs_skel_xyz)
		BE_VBO_Data(&vboctx, galias->ofs_skel_xyz, sizeof(*galias->ofs_skel_xyz) * galias->numverts, &galias->vbo_skel_verts);
	if (galias->ofs_skel_norm)
		BE_VBO_Data(&vboctx, galias->ofs_skel_norm, sizeof(*galias->ofs_skel_norm) * galias->numverts, &galias->vbo_skel_normals);
	if (galias->ofs_skel_svect)
		BE_VBO_Data(&vboctx, galias->ofs_skel_svect, sizeof(*galias->ofs_skel_svect) * galias->numverts, &galias->vbo_skel_svector);
	if (galias->ofs_skel_tvect)
		BE_VBO_Data(&vboctx, galias->ofs_skel_tvect, sizeof(*galias->ofs_skel_tvect) * galias->numverts, &galias->vbo_skel_tvector);
	if (!galias->mappedbones && galias->numbones > sh_config.max_gpu_bones && galias->ofs_skel_idx && sh_config.max_gpu_bones)
	{	//if we're using gpu bones, then its possible that we're trying to load a model with more bones than the gpu supports
		//to work around this (and get performance back), each surface has a gpu->cpu table so that bones not used on a mesh don't cause it to need to use a software fallback
		qboolean *seen = alloca(sizeof(*seen) * galias->numbones);
		int j, k;
		memset(seen, 0, sizeof(*seen) * galias->numbones);
		for (j = 0; j < galias->numverts; j++)
			for (k = 0; k < 4; k++)
			{
				if (galias->ofs_skel_weight[j][k])
					seen[galias->ofs_skel_idx[j][k]] = true;
			}

		for (j = 0, k = 0; j < galias->numbones; j++)
		{
			if (seen[j])
				k++;
		}
		if (k < sh_config.max_gpu_bones)
		{	//okay, we can hardware accelerate that.
			galias->bonemap = ZG_Malloc(&mod->memgroup, sizeof(*galias->bonemap)*sh_config.max_gpu_bones);
			galias->mappedbones = 0;
			for (j = 0; j < galias->numbones; j++)
			{
				if (seen[j])
					galias->bonemap[galias->mappedbones++] = j;
			}
		}
		else
			Con_DPrintf(CON_WARNING"PERF: \"%s\":\"%s\" exceeds gpu bone limit and will be software-skinned - %i > %i\n", mod->name, galias->surfacename, k, sh_config.max_gpu_bones);
	}
	if (galias->mappedbones)
	{
		boneidx_t *remaps = alloca(sizeof(*remaps) * galias->numbones);
		bone_vec4_t *bones = alloca(sizeof(*bones) * galias->numverts);
		int j, k;

		//our remap table is gpu->cpu, but we need cpu->gpu here
		for (j = 0; j < galias->numbones; j++)
			remaps[j] = 0;	//errors.
		for (j = 0; j < galias->mappedbones; j++)
			remaps[galias->bonemap[j]] = j;
		//now remap them all
		for (j = 0; j < galias->numverts; j++)
			for (k = 0; k < 4; k++)
				bones[j][k] = remaps[galias->ofs_skel_idx[j][k]];

		//and we can upload
		if (galias->ofs_skel_idx)
			BE_VBO_Data(&vboctx, bones, sizeof(*bones) * galias->numverts, &galias->vbo_skel_bonenum);
		if (galias->ofs_skel_weight)
			BE_VBO_Data(&vboctx, galias->ofs_skel_weight, sizeof(*galias->ofs_skel_weight) * galias->numverts, &galias->vbo_skel_bweight);
	}
	else
	{
		if (galias->ofs_skel_idx)
			BE_VBO_Data(&vboctx, galias->ofs_skel_idx, sizeof(*galias->ofs_skel_idx) * galias->numverts, &galias->vbo_skel_bonenum);
		if (galias->ofs_skel_weight)
			BE_VBO_Data(&vboctx, galias->ofs_skel_weight, sizeof(*galias->ofs_skel_weight) * galias->numverts, &galias->vbo_skel_bweight);
	}
#endif
#ifdef NONSKELETALMODELS
	for (i = 0; i < galias->numanimations; i++)
	{
		galiaspose_t *pose = group[i].poseofs;
		if (pose)
			for (p = 0; p < group[i].numposes; p++, pose++)
			{
				BE_VBO_Data(&vboctx, pose->ofsverts, sizeof(*pose->ofsverts) * galias->numverts, &pose->vboverts);
				BE_VBO_Data(&vboctx, pose->ofsnormals, sizeof(*pose->ofsnormals) * galias->numverts, &pose->vbonormals);
				if (pose->ofssvector)
					BE_VBO_Data(&vboctx, pose->ofssvector, sizeof(*pose->ofssvector) * galias->numverts, &pose->vbosvector);
				if (pose->ofstvector)
					BE_VBO_Data(&vboctx, pose->ofstvector, sizeof(*pose->ofstvector) * galias->numverts, &pose->vbotvector);
			}
	}
#endif
	BE_VBO_Finish(&vboctx, galias->ofs_indexes, sizeof(*galias->ofs_indexes) * galias->numindexes, &galias->vboindicies, &galias->vbomem, &galias->ebomem);
}
#endif


#ifdef NONSKELETALMODELS
//called for non-skeletal model formats.
static void Mod_BuildTextureVectors(galiasinfo_t *galias)
//vec3_t *vc, vec2_t *tc, vec3_t *nv, vec3_t *sv, vec3_t *tv, index_t *idx, int numidx, int numverts)
{
#ifndef SERVERONLY
	int i, p;
	galiasanimation_t *group;
	galiaspose_t *pose;
	vecV_t *vc;
	vec3_t *nv, *sv, *tv;
	vec2_t *tc;
	index_t *idx;

	//don't fail on dedicated servers
	if (!qrenderer || !BE_VBO_Begin)
		return;

	idx = galias->ofs_indexes;
	tc = galias->ofs_st_array;
	group = galias->ofsanimations;

	for (i = 0; i < galias->numanimations; i++, group++)
	{
		pose = group->poseofs;
		for (p = 0; p < group->numposes; p++, pose++)
		{
			vc = pose->ofsverts;
			nv = pose->ofsnormals;
			if (pose->ofssvector != 0 && pose->ofstvector != 0)
			{
				sv = pose->ofssvector;
				tv = pose->ofstvector;

				Mod_AccumulateTextureVectors(vc, tc, nv, sv, tv, idx, galias->numindexes, false);
				Mod_NormaliseTextureVectors(nv, sv, tv, galias->numverts, false);
			}
		}
	}
#endif
}
#endif

#ifndef SERVERONLY
//looks for foo.md3_0.skin files, for dp compat
//also try foo_0.skin, because people appear to use that too. *sigh*.
static int Mod_CountSkinFiles(model_t *mod)
{
	int i;
	char skinfilename[MAX_QPATH];
	char *modelname = mod->name;
	//try and add numbered skins, and then try fixed names.
	for (i = 0; ; i++)
	{
		Q_snprintfz(skinfilename, sizeof(skinfilename), "%s_%i.skin", modelname, i);
		if (!COM_FCheckExists(skinfilename))
		{
			COM_StripExtension(modelname, skinfilename, sizeof(skinfilename));
			Q_snprintfz(skinfilename+strlen(skinfilename), sizeof(skinfilename)-strlen(skinfilename), "_%i.skin", i);
			if (!COM_FCheckExists(skinfilename))
				break;
		}
	}
	return i;
}

void Mod_LoadAliasShaders(model_t *mod)
{
	galiasinfo_t *ai = mod->meshinfo;
	galiasskin_t *s;
	skinframe_t *f;
	int i, j, k;
	int numskins;

	unsigned int loadflags;
	unsigned int imageflags;
	char basename[32];
	char alttexpath[MAX_QPATH];
	uploadfmt_t skintranstype;
	char *slash;
#ifdef MD1MODELS
#ifdef HEXEN2
	if (mod->flags & MFH2_TRANSPARENT)
		skintranstype = TF_H2_T7G1;	//hexen2
	else
#endif
		if (mod->flags & MFH2_HOLEY)
	{
		//in hexen2, the official value is 0.
		//hexen2's GL renderer ONLY also has a bug that ADDITIONALLY translates index 255. this is the 'normal' behaviour.
		//quakespasm has a bug that translates ONLY 255 and ignores 0. massive fuckup, when index 0 is more commonly used (and is also a dupe anyway, with q1's palette).
		skintranstype = mod_h2holey_bugged.ival?TF_TRANS8:TF_H2_TRANS8_0;	//hexen2
	}
#ifdef HEXEN2
	else if (mod->flags & MFH2_SPECIAL_TRANS)
		skintranstype = TF_H2_T4A4;	//hexen2
#endif
	else
#endif
		skintranstype = TF_SOLID8;

	COM_FileBase(mod->name, basename, sizeof(basename));

	imageflags = 0;
	if (mod->engineflags & MDLF_NOTREPLACEMENTS)
	{
		ruleset_allow_sensitive_texture_replacements.flags |= CVAR_RENDERERLATCH;
		if (!ruleset_allow_sensitive_texture_replacements.ival)
			imageflags |= IF_NOREPLACE;
	}
#ifdef MD1MODELS
	if (mod->fromgame == fg_quake && mod_nomipmap.ival)
		imageflags |= IF_NOMIPMAP;
#endif

	slash = COM_SkipPath(mod->name);
	if (slash != mod->name && slash-mod->name < sizeof(alttexpath))
	{
		slash--;
		memcpy(alttexpath, mod->name, slash-mod->name);
		Q_strncpyz(alttexpath+(slash-mod->name), ":models", sizeof(alttexpath)-(slash-mod->name));	//fuhquake compat
		slash++;
	}
	else
	{
		slash = mod->name;
		strcpy(alttexpath, "models");	//fuhquake compat
	}



	for (ai = mod->meshinfo, numskins = 0; ai; ai = ai->nextsurf)
	{
		if (numskins < ai->numskins)
			numskins = ai->numskins;
		Mod_GenerateMeshVBO(mod, ai);	//FIXME: shares verts
	}
	for (i = 0; i < numskins; i++)
	{
		skinid_t skinid;
		skinfile_t *skinfile;
		char *filedata;
		char skinfilename[MAX_QPATH];
		char *modelname = mod->name;

		skinid = 0;
		skinfile = NULL;
		if (qrenderer != QR_NONE)
		{
			Q_snprintfz(skinfilename, sizeof(skinfilename), "%s_%i.skin", modelname, i);
			filedata = FS_LoadMallocFile(skinfilename, NULL);
			if (!filedata)
			{
				COM_StripExtension(modelname, skinfilename, sizeof(skinfilename));
				Q_snprintfz(skinfilename+strlen(skinfilename), sizeof(skinfilename)-strlen(skinfilename), "_%i.skin", i);
				filedata = FS_LoadMallocFile(skinfilename, NULL);
			}
			if (filedata)
			{
				skinid = Mod_ReadSkinFile(skinfilename, filedata);
				Z_Free(filedata);
				skinfile = Mod_LookupSkin(skinid);
			}
		}

		for (ai = mod->meshinfo; ai; ai = ai->nextsurf)
		{
			if (i >= ai->numskins)
				continue;

			s = ai->ofsskins+i;
			for (j = 0, f = s->frame; j < s->numframes; j++, f++)
			{
				if (j == 0 && skinfile)
				{
					//check if this skinfile has a mapping.
					for (k = 0; k < skinfile->nummappings; k++)
					{
						if (!strcmp(ai->surfacename, skinfile->mappings[k].surface))
						{
							skinfile->mappings[k].shader->uses++;	//so it doesn't blow up when the skin gets freed.
							f->shader = skinfile->mappings[k].shader;
							f->texnums = skinfile->mappings[k].texnums;
							skinfile->mappings[k].needsfree = 0;	//don't free any composed texture. it'll live on as part of the model.
							break;
						}
					}
				}
				else
					f->shader = NULL;
				if (!f->shader)
				{
					if (!f->defaultshader)
					{
						if ((ai->csurface.flags & 0x80) || dpcompat_skinfiles.ival)	//nodraw
							f->shader = R_RegisterShader(f->shadername, SUF_NONE,	"{\nsurfaceparm nodraw\nsurfaceparm nodlight\nsurfaceparm nomarks\nsurfaceparm noshadows\n}\n");
						else
							f->shader = R_RegisterSkin(mod, f->shadername);
					}
					else
						f->shader = R_RegisterShader(f->shadername, SUF_NONE, f->defaultshader);
				}

				if (f->texels)
				{
					loadflags = SHADER_HASPALETTED | SHADER_HASDIFFUSE | SHADER_HASGLOSS | SHADER_HASTOPBOTTOM;
					if (r_fb_models.ival)
						loadflags |= SHADER_HASFULLBRIGHT;
					if (r_loadbumpmapping)
						loadflags |= SHADER_HASNORMALMAP;
					R_BuildLegacyTexnums(f->shader, basename, alttexpath, loadflags, imageflags, skintranstype, s->skinwidth, s->skinheight, f->texels, host_basepal);
				}
				else
					R_BuildDefaultTexnums(&f->texnums, f->shader, 0);
			}
		}
		Mod_WipeSkin(skinid, false);
	}
}
#endif












//Q1 model loading
#ifdef MD1MODELS
#define NUMVERTEXNORMALS	162
extern float	r_avertexnormals[NUMVERTEXNORMALS][3];
// mdltype 0 = q1, 1 = qtest, 2 = rapo/h2

static void Q1MDL_LoadPose(galiasinfo_t *galias, dmdl_t *pq1inmodel, vecV_t *verts, vec3_t *normals, vec3_t *svec, vec3_t *tvec, dtrivertx_t *pinframe, int *seamremaps, int mdltype, unsigned int bbox[6])
{
	int j;
#ifdef _DEBUG
	bbox[0] = bbox[1] = bbox[2] = ~0;
	bbox[3] = bbox[4] = bbox[5] = 0;
#endif

#ifdef HEXEN2
	if (mdltype == 2)
	{
		for (j = 0; j < galias->numverts; j++)
		{
			verts[j][0] = pinframe[seamremaps[j]].v[0]*pq1inmodel->scale[0]+pq1inmodel->scale_origin[0];
			verts[j][1] = pinframe[seamremaps[j]].v[1]*pq1inmodel->scale[1]+pq1inmodel->scale_origin[1];
			verts[j][2] = pinframe[seamremaps[j]].v[2]*pq1inmodel->scale[2]+pq1inmodel->scale_origin[2];
#ifndef SERVERONLY
			VectorCopy(r_avertexnormals[pinframe[seamremaps[j]].lightnormalindex], normals[j]);
#endif
		}
	}
	else
#endif
	{
		for (j = 0; j < pq1inmodel->numverts; j++)
		{
#ifdef _DEBUG
			bbox[0] = min(bbox[0], pinframe[j].v[0]);
			bbox[1] = min(bbox[1], pinframe[j].v[1]);
			bbox[2] = min(bbox[2], pinframe[j].v[2]);
			bbox[3] = max(bbox[3], pinframe[j].v[0]);
			bbox[4] = max(bbox[4], pinframe[j].v[1]);
			bbox[5] = max(bbox[5], pinframe[j].v[2]);
#endif
			verts[j][0] = pinframe[j].v[0]*pq1inmodel->scale[0]+pq1inmodel->scale_origin[0];
			verts[j][1] = pinframe[j].v[1]*pq1inmodel->scale[1]+pq1inmodel->scale_origin[1];
			verts[j][2] = pinframe[j].v[2]*pq1inmodel->scale[2]+pq1inmodel->scale_origin[2];
#ifndef SERVERONLY
			VectorCopy(r_avertexnormals[pinframe[j].lightnormalindex], normals[j]);
#endif
			if (seamremaps[j] != j)
			{
				VectorCopy(verts[j], verts[seamremaps[j]]);
#ifndef SERVERONLY
				VectorCopy(normals[j], normals[seamremaps[j]]);
#endif
			}
		}
	}
}
static void Q1MDL_LoadPoseQF16(galiasinfo_t *galias, dmdl_t *pq1inmodel, vecV_t *verts, vec3_t *normals, vec3_t *svec, vec3_t *tvec, dtrivertx_t *pinframe, int *seamremaps, int mdltype)
{
	//quakeforge's MD16 format has regular 8bit stuff, trailed by an extra low-order set of the verts providing the extra 8bits of precision.
	//its worth noting that the model could be rendered using the high-order parts only, if your software renderer only supports that or whatever.
	dtrivertx_t *pinframelow =  pinframe + pq1inmodel->numverts;
	int j;
	vec3_t exscale;
	VectorScale(pq1inmodel->scale, 1.0/256, exscale);
	for (j = 0; j < pq1inmodel->numverts; j++)
	{
		verts[j][0] = pinframe[j].v[0]*pq1inmodel->scale[0] + pinframelow[j].v[0]*exscale[0] + pq1inmodel->scale_origin[0];
		verts[j][1] = pinframe[j].v[1]*pq1inmodel->scale[1] + pinframelow[j].v[1]*exscale[1] + pq1inmodel->scale_origin[1];
		verts[j][2] = pinframe[j].v[2]*pq1inmodel->scale[2] + pinframelow[j].v[2]*exscale[2] + pq1inmodel->scale_origin[2];
#ifndef SERVERONLY
		VectorCopy(r_avertexnormals[pinframe[j].lightnormalindex], normals[j]);
#endif
		if (seamremaps[j] != j)
		{
			VectorCopy(verts[j], verts[seamremaps[j]]);
#ifndef SERVERONLY
			VectorCopy(normals[j], normals[seamremaps[j]]);
#endif
		}
	}
}
static void *Q1MDL_LoadFrameGroup (galiasinfo_t *galias, dmdl_t *pq1inmodel, model_t *loadmodel, daliasframetype_t *pframetype, int *seamremaps, int mdltype)
{
	galiaspose_t *pose;
	galiasanimation_t *frame = galias->ofsanimations;
	dtrivertx_t		*pinframe;
	daliasframe_t *frameinfo;
	int				i, k;
	daliasgroup_t *ingroup;
	daliasinterval_t *intervals;
	float sinter;

	vec3_t *normals, *svec, *tvec;
	vecV_t *verts;
	int aliasframesize = (mdltype == 1) ? sizeof(daliasframe_t)-16 : sizeof(daliasframe_t);

	unsigned int bbox[6];

#ifdef SERVERONLY
	normals = NULL;
	svec = NULL;
	tvec = NULL;
#endif

	for (i = 0; i < pq1inmodel->numframes; i++)
	{
		frame->action = -1;
		frame->actionweight = 0;
		switch(LittleLong(pframetype->type))
		{
		case ALIAS_SINGLE:
			frameinfo = (daliasframe_t*)((char *)(pframetype+1)); // qtest aliasframe is a subset
			pinframe = (dtrivertx_t*)((char*)frameinfo+aliasframesize);
#ifndef SERVERONLY
			pose = (galiaspose_t *)ZG_Malloc(&loadmodel->memgroup, sizeof(galiaspose_t) + (sizeof(vecV_t)+sizeof(vec3_t)*3)*galias->numverts);
#else
			pose = (galiaspose_t *)ZG_Malloc(&loadmodel->memgroup, sizeof(galiaspose_t) + (sizeof(vecV_t))*galias->numverts);
#endif
			frame->poseofs = pose;
			frame->numposes = 1;
			galias->numanimations++;

			if (mdltype == 1)
				frame->name[0] = '\0';
			else
				Q_strncpyz(frame->name, frameinfo->name, sizeof(frame->name));

			verts = (vecV_t *)(pose+1);
			pose->ofsverts = verts;
#ifndef SERVERONLY
			normals = (vec3_t*)&verts[galias->numverts];
			svec = &normals[galias->numverts];
			tvec = &svec[galias->numverts];
			pose->ofsnormals = normals;
			pose->ofssvector = svec;
			pose->ofstvector = tvec;
#endif

			if (mdltype & 16)
			{
				Q1MDL_LoadPoseQF16(galias, pq1inmodel, verts, normals, svec, tvec, pinframe, seamremaps, mdltype);
				pframetype = (daliasframetype_t *)&pinframe[pq1inmodel->numverts*2];
			}
			else
			{
				Q1MDL_LoadPose(galias, pq1inmodel, verts, normals, svec, tvec, pinframe, seamremaps, mdltype, bbox);
				pframetype = (daliasframetype_t *)&pinframe[pq1inmodel->numverts];

#ifdef _DEBUG
				if ((bbox[3] > frameinfo->bboxmax.v[0] || bbox[4] > frameinfo->bboxmax.v[1] || bbox[5] > frameinfo->bboxmax.v[2] ||
					bbox[0] < frameinfo->bboxmin.v[0] || bbox[1] < frameinfo->bboxmin.v[1] || bbox[2] < frameinfo->bboxmin.v[2]) && !galias->warned)
#else
				if (galias->numverts && pinframe[0].v[2] > frameinfo->bboxmax.v[2] && !galias->warned)
#endif
				{
					Con_DPrintf(CON_WARNING"%s has incorrect frame bounds\n", loadmodel->name);
					galias->warned = true;
				}
			}


//			GL_GenerateNormals((float*)verts, (float*)normals, (int *)((char *)galias + galias->ofs_indexes), galias->numindexes/3, galias->numverts);

			break;

		case ALIAS_GROUP:
		case ALIAS_GROUP_SWAPPED: // prerelease
			ingroup = (daliasgroup_t *)(pframetype+1);

			frame->numposes = LittleLong(ingroup->numframes);
#ifdef SERVERONLY
			pose = (galiaspose_t *)ZG_Malloc(&loadmodel->memgroup, frame->numposes*(sizeof(galiaspose_t) + sizeof(vecV_t)*galias->numverts));
			verts = (vecV_t *)(pose+frame->numposes);
#else
			pose = (galiaspose_t *)ZG_Malloc(&loadmodel->memgroup, frame->numposes*(sizeof(galiaspose_t) + (sizeof(vecV_t)+sizeof(vec3_t)*3)*galias->numverts));
			verts = (vecV_t *)(pose+frame->numposes);
			normals = (vec3_t*)&verts[galias->numverts];
			svec = &normals[galias->numverts];
			tvec = &svec[galias->numverts];
#endif

			frame->poseofs = pose;
			frame->loop = true;
			galias->numanimations++;

			intervals = (daliasinterval_t *)(ingroup+1);
			if (frame->numposes == 0)
				frame->rate = 10;
			else
			{
				sinter = LittleFloat(intervals->interval);
				if (sinter <= 0)
					sinter = 0.1;
				frame->rate = 1/sinter;
			}

			pinframe = (dtrivertx_t *)(intervals+frame->numposes);
			for (k = 0; k < frame->numposes; k++)
			{
				pose->ofsverts = verts;
#ifndef SERVERONLY
				pose->ofsnormals = normals;
				pose->ofssvector = svec;
				pose->ofstvector = tvec;
#endif

				frameinfo = (daliasframe_t*)pinframe;
				pinframe = (dtrivertx_t *)((char *)frameinfo + aliasframesize);

				if (k == 0)
				{
					if (mdltype == 1)
						frame->name[0] = '\0';
					else
						Q_strncpyz(frame->name, frameinfo->name, sizeof(frame->name));
				}

				if (mdltype & 16)
				{
					Q1MDL_LoadPoseQF16(galias, pq1inmodel, verts, normals, svec, tvec, pinframe, seamremaps, mdltype);
					pinframe += pq1inmodel->numverts*2;
				}
				else
				{
					Q1MDL_LoadPose(galias, pq1inmodel, verts, normals, svec, tvec, pinframe, seamremaps, mdltype, bbox);

#ifdef _DEBUG
					if ((bbox[3] > frameinfo->bboxmax.v[0] || bbox[4] > frameinfo->bboxmax.v[1] || bbox[5] > frameinfo->bboxmax.v[2] ||
						bbox[0] < frameinfo->bboxmin.v[0] || bbox[1] < frameinfo->bboxmin.v[1] || bbox[2] < frameinfo->bboxmin.v[2] ||
#else
					if (galias->numverts && (pinframe[0].v[2] > frameinfo->bboxmax.v[2] ||
#endif
						frameinfo->bboxmin.v[0] < ingroup->bboxmin.v[0] || frameinfo->bboxmin.v[1] < ingroup->bboxmin.v[1] || frameinfo->bboxmin.v[2] < ingroup->bboxmin.v[2] ||
						frameinfo->bboxmax.v[0] > ingroup->bboxmax.v[0] || frameinfo->bboxmax.v[1] > ingroup->bboxmax.v[1] || frameinfo->bboxmax.v[2] > ingroup->bboxmax.v[2]) && !galias->warned)
					{
						Con_DPrintf(CON_WARNING"%s has incorrect frame bounds\n", loadmodel->name);
						galias->warned = true;
					}
					pinframe += pq1inmodel->numverts;
				}

#ifndef SERVERONLY
				verts = (vecV_t*)&tvec[galias->numverts];
				normals = (vec3_t*)&verts[galias->numverts];
				svec = &normals[galias->numverts];
				tvec = &svec[galias->numverts];
#else
				verts = &verts[galias->numverts];
#endif
				pose++;
			}

//			GL_GenerateNormals((float*)verts, (float*)normals, (int *)((char *)galias + galias->ofs_indexes), galias->numindexes/3, galias->numverts);

			pframetype = (daliasframetype_t *)pinframe;
			break;
		default:
			Con_Printf(CON_ERROR "Bad frame type in %s\n", loadmodel->name);
			return NULL;
		}
		frame++;
	}
	return pframetype;
}

//greatly reduced version of Q1_LoadSkins
//just skips over the data
static void *Q1MDL_LoadSkins_SV (galiasinfo_t *galias, dmdl_t *pq1inmodel, daliasskintype_t *pskintype, unsigned int skintranstype)
{
	int i;
	int s;
	int *count;
	float *intervals;
	qbyte *data;

	s = pq1inmodel->skinwidth*pq1inmodel->skinheight;
	for (i = 0; i < pq1inmodel->numskins; i++)
	{
		switch(LittleLong(pskintype->type))
		{
		case ALIAS_SKIN_SINGLE:
			pskintype = (daliasskintype_t *)((char *)(pskintype+1)+s);
			break;

		default:
			count = (int *)(pskintype+1);
			intervals = (float *)(count+1);
			data = (qbyte *)(intervals + LittleLong(*count));
			data += s*LittleLong(*count);
			pskintype = (daliasskintype_t *)data;
			break;
		}
	}
	galias->numskins=pq1inmodel->numskins;
	return pskintype;
}

#ifndef SERVERONLY
static cvar_t dpcompat_nofloodfill = CVARD("dpcompat_nofloodfill", "0", "Disables the q1mdl floodfill. Setting this to 1 may result in blue seams on the vanilla quake models.");
/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes - Ed
=================
*/

typedef struct
{
	short		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
do{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}while(0)

static void Mod_FloodFillSkin( qbyte *skin, int skinwidth, int skinheight )
{
	qbyte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if (dpcompat_nofloodfill.ival || skinwidth > 0x7fffu || skinheight > 0x7fffu)
		return;

	if (filledcolor == -1)
	{
		filledcolor = 0;
		// attempt to find opaque black
		for (i = 0; i < 256; ++i)
			if (d_8to24rgbtable[i] == (255 << 0)) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledcolor;
		qbyte		*pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP( -1, -1, 0 );
		if (x < skinwidth - 1)	FLOODFILL_STEP( 1, 1, 0 );
		if (y > 0)				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if (y < skinheight - 1)	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[x + skinwidth * y] = fdc;
	}
}

static void *Q1MDL_LoadSkins_GL (galiasinfo_t *galias, dmdl_t *pq1inmodel, model_t *loadmodel, daliasskintype_t *pskintype, uploadfmt_t skintranstype)
{
	skinframe_t *frames;
	int i;
	int s, t;
	float sinter;
	daliasskingroup_t *count;
	daliasskininterval_t *intervals;
	qbyte *data, *saved;
	galiasskin_t *outskin = galias->ofsskins;

	s = pq1inmodel->skinwidth*pq1inmodel->skinheight;
	for (i = 0; i < pq1inmodel->numskins; i++)
	{
		switch(LittleLong(pskintype->type))
		{
		case ALIAS_SKIN_SINGLE:
			outskin->skinwidth = pq1inmodel->skinwidth;
			outskin->skinheight = pq1inmodel->skinheight;

//but only preload it if we have no replacement.
			outskin->numframes=1;

			//the actual texture gets loaded after the shader.
			frames = ZG_Malloc(&loadmodel->memgroup, sizeof(*frames)+s);
			saved = (qbyte*)(frames+1);
			frames[0].texels = saved;
			memcpy(saved, pskintype+1, s);
			if (i == 0) //Vanilla bug: ONLY skin 0 is flood-filled (the vanilla code operates on a cached 'skin' variable that does NOT get updated between skins reflooding skin 0). We still don't like flood fills either. Hexen2 has the same issue.
				Mod_FloodFillSkin(saved, outskin->skinwidth, outskin->skinheight);

			Q_snprintfz(frames[0].shadername, sizeof(frames[0].shadername), "%s_%i.lmp", loadmodel->name, i);
			frames[0].shader = NULL;
			frames[0].defaultshader = NULL;
			outskin->frame = frames;

			switch(skintranstype)
			{
			default:	//urk
			case TF_SOLID8:
				frames[0].defaultshader = NULL;	//default skin...
				break;
			case TF_H2_T7G1:
				frames[0].defaultshader =
					"{\n"
//						"program defaultskin\n"
						"{\n"
							"map $diffuse\n"
							"blendfunc gl_src_alpha gl_one_minus_src_alpha\n"
							"alphagen entity\n"
							"rgbgen lightingDiffuse\n"
							"depthwrite\n"
						"}\n"
					"}\n";
				break;
			case TF_H2_TRANS8_0:
				frames[0].defaultshader =
					"{\n"
						"glslprogram defaultskin#MASK=0.5#MASKLT\n"
						"{\n"
							"map $diffuse\n"
							"blendfunc gl_src_alpha gl_one_minus_src_alpha\n"
							"alphafunc ge128\n"
							"rgbgen lightingDiffuse\n"
							"alphagen entity\n"
							"depthwrite\n"
						"}\n"
						"{\n"
							"map $loweroverlay\n"
							"rgbgen bottomcolor\n"
							"blendfunc gl_src_alpha gl_one\n"
						"}\n"
						"{\n"
							"map $upperoverlay\n"
							"rgbgen topcolor\n"
							"blendfunc gl_src_alpha gl_one\n"
						"}\n"
						"{\n"
							"map $fullbright\n"
							"blendfunc add\n"
						"}\n"
					"}\n";
				break;
			case TF_H2_T4A4:
				frames[0].defaultshader =
					"{\n"
//						"program defaultskin\n"
						"cull disable\n"
						"{\n"
							"map $diffuse\n"
							"blendfunc gl_one_minus_src_alpha gl_src_alpha\n"
							"alphagen entity\n"
							"rgbgen lightingDiffuse\n"
							"depthwrite\n"
						"}\n"
						"{\n"
							"map $loweroverlay\n"
							"rgbgen bottomcolor\n"
							"blendfunc gl_src_alpha gl_one\n"
						"}\n"
						"{\n"
							"map $upperoverlay\n"
							"rgbgen topcolor\n"
							"blendfunc gl_src_alpha gl_one\n"
						"}\n"
						"{\n"
							"map $fullbright\n"
							"blendfunc add\n"
						"}\n"
					"}\n";
				break;
			}

			pskintype = (daliasskintype_t *)((char *)(pskintype+1)+s);
			break;

		default:
			outskin->skinwidth = pq1inmodel->skinwidth;
			outskin->skinheight = pq1inmodel->skinheight;
			count = (daliasskingroup_t*)(pskintype+1);
			intervals = (daliasskininterval_t *)(count+1);
			outskin->numframes = LittleLong(count->numskins);
			data = (qbyte *)(intervals + outskin->numframes);
			frames = ZG_Malloc(&loadmodel->memgroup, sizeof(*frames)*outskin->numframes);
			outskin->frame = frames;
			sinter = LittleFloat(intervals[0].interval);
			if (sinter <= 0)
				sinter = 0.1;
			outskin->skinspeed = 1/sinter;

			for (t = 0; t < outskin->numframes; t++,data+=s)
			{
				frames[t].texels = ZG_Malloc(&loadmodel->memgroup, s);
				memcpy(frames[t].texels, data, s);
				//other engines apparently don't flood fill. because flood filling is horrible, we won't either.
				//Mod_FloodFillSkin(frames[t].texels, outskin->skinwidth, outskin->skinheight);

				Q_snprintfz(frames[t].shadername, sizeof(frames[t].shadername), "%s_%i_%i.lmp", loadmodel->name, i, t);
				frames[t].defaultshader = NULL;
			}
			pskintype = (daliasskintype_t *)data;
			break;
		}
		outskin++;
	}
	galias->numskins=pq1inmodel->numskins;
	return pskintype;
}
#endif

static void Mesh_HandleFramegroupsFile(model_t *mod, galiasinfo_t *galias)
{	//use ONLY with vertex models.
	unsigned int numanims, a, p, g, oldnumanims = galias->numanimations, targpose;
	galiasanimation_t *o, *oldanims = galias->ofsanimations, *frame;
	frameinfo_t *framegroups = ParseFrameInfo(mod, &numanims);
	if (framegroups)
	{
		galias->ofsanimations = o = ZG_Malloc(&mod->memgroup, sizeof(*galias->ofsanimations) * numanims);
		for (a = 0; a < numanims; a++, o++)
		{
			o->poseofs = ZG_Malloc(&mod->memgroup, sizeof(*o->poseofs) * framegroups[a].posecount);
			for (p = 0; p < framegroups[a].posecount; p++)
			{
				if (framegroups[a].posesarray)
					targpose = framegroups[a].poses[p];
				else
					targpose = framegroups[a].firstpose + p;
				for (g = 0, frame = oldanims; g < oldnumanims; g++, frame++)
				{
					if (targpose < frame->numposes)
						break;
					targpose -= frame->numposes;
				}
				if (g == oldnumanims)
					break;
				o->poseofs[p] = frame->poseofs[targpose];
			}
			o->numposes = p;
			o->rate = framegroups[a].fps;
			o->loop = framegroups[a].loop;
			o->events = framegroups[a].events;
			o->action = framegroups[a].action;
			o->actionweight = framegroups[a].actionweight;
			Q_strncpyz(o->name, framegroups[a].name, sizeof(o->name));
		}
		galias->numanimations = numanims;
		free(framegroups);
	}
}

static qboolean QDECL Mod_LoadQ1Model (model_t *mod, void *buffer, size_t fsize)
{
#ifndef SERVERONLY
	vec2_t *st_array;
	int j;
	float halftexel = mod_halftexel.ival?0.5:0;
#endif
	int version;
	int i, onseams;
	dstvert_t *pinstverts;
	dtriangle_t *pinq1triangles;
	int *seamremap;
	index_t *indexes;
	daliasskintype_t *skinstart;
	uploadfmt_t skintranstype;
	qboolean sixteenbit;

	int size;
	unsigned int hdrsize;
	void *end;
	qboolean qtest = false;
#ifdef HEXEN2
	dh2triangle_t *pinh2triangles;
	qboolean rapo = false;
#endif
	galiasinfo_t *galias;
	dmdl_t *pq1inmodel = (dmdl_t *)buffer;

	hdrsize = sizeof(dmdl_t) - sizeof(int);

	mod->engineflags |= MDLF_NEEDOVERBRIGHT;

	sixteenbit = pq1inmodel->ident == LittleLong(('6'<<24)+('1'<<16)+('D'<<8)+'M');	//quakeforge's 16bit mdls

	version = LittleLong(pq1inmodel->version);
	if (version == QTESTALIAS_VERSION && !sixteenbit)
	{
		hdrsize = (size_t)&((dmdl_t*)NULL)->flags;
		qtest = true;
	}
#ifdef HEXEN2
	else if (version == 50 && !sixteenbit)
	{
		hdrsize = sizeof(dmdl_t);
		rapo = true;
	}
#endif
	else if (version != ALIAS_VERSION)
	{
		Con_Printf (CON_ERROR "%s has wrong version number (%i should be %i)\n",
				 mod->name, version, ALIAS_VERSION);
		return false;
	}

	seamremap = (int*)pq1inmodel;	//I like overloading locals.

	i = hdrsize/4 - 1;

	for (; i >= 0; i--)
		seamremap[i] = LittleLong(seamremap[i]);

	if (pq1inmodel->numframes < 1 ||
		pq1inmodel->numskins < 1 ||
		pq1inmodel->numtris < 1 ||
		pq1inmodel->numverts < 3 ||
		pq1inmodel->skinheight < 1 ||
		pq1inmodel->skinwidth < 1)
	{
		Con_Printf(CON_ERROR "Model %s has an invalid quantity\n", mod->name);
		return false;
	}

	if (qtest)
		mod->flags = 0; // Qtest has no flags in header
	else
		mod->flags = pq1inmodel->flags;

	size = sizeof(galiasinfo_t)
#ifndef SERVERONLY
		+ pq1inmodel->numskins*sizeof(galiasskin_t)
#endif
		+ pq1inmodel->numframes*sizeof(galiasanimation_t);

	galias = ZG_Malloc(&mod->memgroup, size);
	Mod_DefaultMesh(galias, mod->name, 0);
	galias->ofsanimations = (galiasanimation_t*)(galias+1);
#ifndef SERVERONLY
	galias->ofsskins = (galiasskin_t*)(galias->ofsanimations+pq1inmodel->numframes);
#endif
	galias->nextsurf = 0;

	mod->numframes = pq1inmodel->numframes;

//skins
	skinstart = (daliasskintype_t *)((char*)pq1inmodel+hdrsize);

#ifdef HEXEN2
	if( mod->flags & MFH2_TRANSPARENT )
		skintranstype = TF_H2_T7G1;	//hexen2
	else
#endif
	 if( mod->flags & MFH2_HOLEY )
		skintranstype = TF_H2_TRANS8_0;	//hexen2
#ifdef HEXEN2
	else if( mod->flags & MFH2_SPECIAL_TRANS )
		skintranstype = TF_H2_T4A4;	//hexen2
#endif
	else
		skintranstype = TF_SOLID8;

	switch(qrenderer)
	{
	default:
#ifndef SERVERONLY
		pinstverts = (dstvert_t *)Q1MDL_LoadSkins_GL(galias, pq1inmodel, mod, skinstart, skintranstype);
		break;
#endif
	case QR_NONE:
		pinstverts = (dstvert_t *)Q1MDL_LoadSkins_SV(galias, pq1inmodel, skinstart, skintranstype);
		break;
	}

#ifdef HEXEN2
	if (rapo)
	{
		/*each triangle can use one coord and one st, for each vert, that's a lot of combinations*/
#ifdef SERVERONLY
		/*separate st + vert lists*/
		pinh2triangles = (dh2triangle_t *)&pinstverts[pq1inmodel->num_st];

		seamremap = BZ_Malloc(sizeof(*seamremap)*pq1inmodel->numtris*3);

		galias->numverts = pq1inmodel->numverts;
		galias->numindexes = pq1inmodel->numtris*3;
		indexes = ZG_Malloc(&mod->memgroup, galias->numindexes*sizeof(*indexes));
		galias->ofs_indexes = indexes;
		for (i = 0; i < pq1inmodel->numverts; i++)
			seamremap[i] = i;
		for (i = 0; i < pq1inmodel->numtris; i++)
		{
			indexes[i*3+0] = LittleShort(pinh2triangles[i].vertindex[0]);
			indexes[i*3+1] = LittleShort(pinh2triangles[i].vertindex[1]);
			indexes[i*3+2] = LittleShort(pinh2triangles[i].vertindex[2]);
		}
#else
		int t, v, k;
		int *stremap;
		/*separate st + vert lists*/
		pinh2triangles = (dh2triangle_t *)&pinstverts[pq1inmodel->num_st];

		seamremap = BZ_Malloc(sizeof(int)*pq1inmodel->numtris*6);
		stremap = seamremap + pq1inmodel->numtris*3;

		/*output the indicies as we figure out which verts we want*/
		galias->numindexes = pq1inmodel->numtris*3;
		indexes = ZG_Malloc(&mod->memgroup, galias->numindexes*sizeof(*indexes));
		galias->ofs_indexes = indexes;
		for (i = 0; i < pq1inmodel->numtris; i++)
		{
			for (j = 0; j < 3; j++)
			{
				v = LittleShort(pinh2triangles[i].vertindex[j]);
				t = LittleShort(pinh2triangles[i].stindex[j]);
				if (pinstverts[t].onseam && !pinh2triangles[i].facesfront)
					t += pq1inmodel->num_st;
			 	for (k = 0; k < galias->numverts; k++) /*big fatoff slow loop*/
				{
					if (stremap[k] == t && seamremap[k] == v)
						break;
				}
				if (k == galias->numverts)
				{
					galias->numverts++;
					stremap[k] = t;
					seamremap[k] = v;
				}
				indexes[i*3+j] = k;
			}
		}

		st_array = ZG_Malloc(&mod->memgroup, sizeof(*st_array)*(galias->numverts));
		galias->ofs_st_array = st_array;
		/*generate our st_array now we know which vertexes we want*/
		for (k = 0; k < galias->numverts; k++)
		{
			if (stremap[k] > pq1inmodel->num_st)
			{	/*onseam verts? shrink the index, and add half a texture width to the s coord*/
				st_array[k][0] = 0.5+(LittleLong(pinstverts[stremap[k]-pq1inmodel->num_st].s)+halftexel)/(float)pq1inmodel->skinwidth;
				st_array[k][1] = (LittleLong(pinstverts[stremap[k]-pq1inmodel->num_st].t)+halftexel)/(float)pq1inmodel->skinheight;
			}
			else
			{
				st_array[k][0] = (LittleLong(pinstverts[stremap[k]].s)+halftexel)/(float)pq1inmodel->skinwidth;
				st_array[k][1] = (LittleLong(pinstverts[stremap[k]].t)+halftexel)/(float)pq1inmodel->skinheight;
			}
		}
#endif
		end = &pinh2triangles[pq1inmodel->numtris];

		if (Q1MDL_LoadFrameGroup(galias, pq1inmodel, mod, (daliasframetype_t *)end, seamremap, 2) == NULL)
		{
			BZ_Free(seamremap);
			ZG_FreeGroup(&mod->memgroup);
			return false;
		}

		BZ_Free(seamremap);
	}
	else
#endif
	{
		/*onseam means +=skinwidth/2
		verticies that are marked as onseam potentially generate two output verticies.
		the triangle chooses which side based upon its 'onseam' field.
		*/

		//count number of verts that are onseam.
		for (onseams=0,i = 0; i < pq1inmodel->numverts; i++)
		{
			if (pinstverts[i].onseam)
				onseams++;
		}
		seamremap = BZ_Malloc(sizeof(*seamremap)*pq1inmodel->numverts);

		galias->numverts = pq1inmodel->numverts+onseams;

		//st
#ifndef SERVERONLY
		st_array = ZG_Malloc(&mod->memgroup, sizeof(*st_array)*(pq1inmodel->numverts+onseams));
		galias->ofs_st_array = st_array;
		for (j=pq1inmodel->numverts,i = 0; i < pq1inmodel->numverts; i++)
		{
			st_array[i][0] = (LittleLong(pinstverts[i].s)+halftexel)/(float)pq1inmodel->skinwidth;
			st_array[i][1] = (LittleLong(pinstverts[i].t)+halftexel)/(float)pq1inmodel->skinheight;

			if (pinstverts[i].onseam)
			{
				if (pinstverts[i].onseam != 0x20 && !galias->warned)
				{
					Con_DPrintf(CON_WARNING "Model %s has an invalid seam flag, which may crash software-rendered engines\n", mod->name);
					//1 == ALIAS_LEFT_CLIP
					galias->warned = true;
				}
				st_array[j][0] = st_array[i][0]+0.5;
				st_array[j][1] = st_array[i][1];
				seamremap[i] = j;
				j++;
			}
			else
				seamremap[i] = i;
		}
#else
		for (i = 0; i < pq1inmodel->numverts; i++)
		{
			seamremap[i] = i;
		}
#endif

		//trianglelists;
		pinq1triangles = (dtriangle_t *)&pinstverts[pq1inmodel->numverts];

		galias->numindexes = pq1inmodel->numtris*3;
		indexes = ZG_Malloc(&mod->memgroup, galias->numindexes*sizeof(*indexes));
		galias->ofs_indexes = indexes;
		for (i=0 ; i<pq1inmodel->numtris ; i++)
		{
			unsigned int v1 = LittleLong(pinq1triangles[i].vertindex[0]);
			unsigned int v2 = LittleLong(pinq1triangles[i].vertindex[1]);
			unsigned int v3 = LittleLong(pinq1triangles[i].vertindex[2]);
			if (v1 >= pq1inmodel->numverts || v2 >= pq1inmodel->numverts || v3 >= pq1inmodel->numverts)
			{
				Con_DPrintf(CON_ERROR"%s has invalid triangle (%u %u %u > %u)\n", mod->name, v1, v2, v3, pq1inmodel->numverts);
				v1 = v2 = v3 = 0;
			}
			if (!pinq1triangles[i].facesfront)
			{
				indexes[i*3+0] = seamremap[v1];
				indexes[i*3+1] = seamremap[v2];
				indexes[i*3+2] = seamremap[v3];
			}
			else
			{
				indexes[i*3+0] = v1;
				indexes[i*3+1] = v2;
				indexes[i*3+2] = v3;
			}
		}
		end = &pinq1triangles[pq1inmodel->numtris];

		//frames
		if (Q1MDL_LoadFrameGroup(galias, pq1inmodel, mod, (daliasframetype_t *)end, seamremap, (sixteenbit?16:0) | (qtest?1:0)) == NULL)
		{
			BZ_Free(seamremap);
			ZG_FreeGroup(&mod->memgroup);
			return false;
		}
		BZ_Free(seamremap);
	}


	Mod_CompileTriangleNeighbours(mod, galias);
	Mod_BuildTextureVectors(galias);

#if 0	//fast (somewhat inaccurate) way. some exporters will bias all geometry weirdly (often ensuring 0 0 0 is in the bounds, resulting in unfortunate biases.)
	VectorCopy (pq1inmodel->scale_origin, mod->mins);
	VectorMA (mod->mins, 255, pq1inmodel->scale, mod->maxs);
#else
	ClearBounds(mod->mins, mod->maxs);
	for (i = 0; i < galias->numanimations; i++)
	{
		galiasanimation_t *a = galias->ofsanimations;
		vecV_t *v;
		size_t j, k;
		for (j = 0; j < a->numposes; j++)
		{
			v = a->poseofs[j].ofsverts;
			for (k = 0; k < galias->numverts; k++)
				AddPointToBounds(v[k], mod->mins, mod->maxs);
		}
	}
	if (mod->maxs[0] < mod->mins[0])	//no points? o.O
		AddPointToBounds(vec3_origin, mod->mins, mod->maxs);
#endif

	mod->type = mod_alias;
	Mod_ClampModelSize(mod);
	Mesh_HandleFramegroupsFile(mod, galias);
	Mod_ParseModelEvents(mod, galias->ofsanimations, galias->numanimations);

	mod->numframes = galias->numanimations;
	mod->meshinfo = galias;

	mod->funcs.NativeTrace = Mod_Trace;

	if (!strcmp(mod->name, "progs/v_shot.mdl"))
		galias->lerpcutoff = 20;
	else if (!strcmp(mod->name, "progs/v_shot2.mdl"))
		galias->lerpcutoff = 20;
	else if (!strcmp(mod->name, "progs/v_nail.mdl"))
		galias->lerpcutoff = 7;
	else if (!strcmp(mod->name, "progs/v_nail2.mdl"))
		galias->lerpcutoff = 6;
	else if (!strcmp(mod->name, "progs/v_rock.mdl"))
		galias->lerpcutoff = 30;
	else if (!strcmp(mod->name, "progs/v_rock2.mdl"))
		galias->lerpcutoff = 30;
	else if (!strcmp(mod->name, "progs/v_light.mdl"))
		galias->lerpcutoff = 30;
#ifdef HEXEN2
	if ((mod->flags == MF_ROCKET) && !strncmp(mod->name, "models/sflesh", 13))
		mod->flags = MFH2_SPIDERBLOOD;
#endif

	return true;
}

static int Mod_ReadFlagsFromMD1(char *name, int md3version)
{
	int result = 0;
	size_t fsize;
	dmdl_t				*pinmodel;
	char fname[MAX_QPATH];
	COM_StripExtension(name, fname, sizeof(fname));
	COM_DefaultExtension(fname, ".mdl", sizeof(fname));

	if (!strcmp(name, fname))	//md3 renamed as mdl
	{
		COM_StripExtension(name, fname, sizeof(fname));	//seeing as the md3 is named over the mdl,
		COM_DefaultExtension(fname, ".md1", sizeof(fname));//read from a file with md1 (one, not an ell)
	}

	pinmodel = (dmdl_t *)FS_LoadMallocFile(fname, &fsize);
	if (pinmodel)
	{
		if (fsize >= sizeof(dmdl_t) && !memcmp(&pinmodel->ident, IDPOLYHEADER))
			if (LittleLong(pinmodel->version) == ALIAS_VERSION)
				result = LittleLong(pinmodel->flags);
		BZ_Free(pinmodel);
	}
	return result;
}
#else
int Mod_ReadFlagsFromMD1(char *name, int md3version)
{
	return 0;
}
#endif

#ifdef MD2MODELS

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Q2 model loading

typedef struct
{
	float		scale[3];	// multiply qbyte verts by this
	float		translate[3];	// then add this
	char		name[16];	// frame name from grabbing
	dtrivertx_t	verts[1];	// variable sized
} dmd2aliasframe_t;

//static galiasinfo_t *galias;
//static md2_t *pq2inmodel;
#define Q2NUMVERTEXNORMALS	162
extern vec3_t	bytedirs[Q2NUMVERTEXNORMALS];

static void Q2MD2_LoadSkins(galiasinfo_t *galias, model_t *mod, md2_t *pq2inmodel, char *skins)
{
#ifndef SERVERONLY
	int i;
	skinframe_t *frames;
	galiasskin_t *outskin = galias->ofsskins;

	for (i = 0; i < LittleLong(pq2inmodel->num_skins); i++, outskin++)
	{
		frames = ZG_Malloc(&mod->memgroup, sizeof(*frames));
		outskin->frame = frames;
		outskin->numframes=1;

		COM_CleanUpPath(skins);	//blooming tanks.
		Q_strncpyz(frames->shadername, skins, sizeof(frames->shadername));

		outskin->skinwidth = 0;
		outskin->skinheight = 0;
		outskin->skinspeed = 0;

		skins += MD2MAX_SKINNAME;
	}
#endif
	galias->numskins = LittleLong(pq2inmodel->num_skins);

	/*
#ifndef SERVERONLY
	outskin = (galiasskin_t *)((char *)galias + galias->ofsskins);
	outskin += galias->numskins - 1;
	if (galias->numskins)
	{
		if (*(shader_t**)((char *)outskin + outskin->ofstexnums))
			return;

		galias->numskins--;
	}
#endif
	*/
}

#define MD2_MAX_TRIANGLES 4096
static qboolean QDECL Mod_LoadQ2Model (model_t *mod, void *buffer, size_t fsize)
{
#ifndef SERVERONLY
	dmd2stvert_t *pinstverts;
	vec2_t *st_array;
	vec3_t *normals;
#endif
	md2_t *pq2inmodel;

	int version;
	int i, j;
	dmd2triangle_t *pintri;
	index_t *indexes;
	int numindexes;

	vec3_t min;
	vec3_t max;

	galiaspose_t *pose;
	galiasanimation_t *poutframe;
	dmd2aliasframe_t *pinframe;
	int framesize;
	vecV_t *verts;

	int		indremap[MD2_MAX_TRIANGLES*3];
	unsigned short		ptempindex[MD2_MAX_TRIANGLES*3], ptempstindex[MD2_MAX_TRIANGLES*3];

	int numverts;

	int size;
	galiasinfo_t *galias;

	mod->engineflags |= MDLF_NEEDOVERBRIGHT;

	pq2inmodel = (md2_t *)buffer;

	version = LittleLong (pq2inmodel->version);
	if (version != MD2ALIAS_VERSION)
	{
		Con_Printf (CON_ERROR "%s has wrong version number (%i should be %i)\n",
				 mod->name, version, MD2ALIAS_VERSION);
		return false;
	}

	if (LittleLong(pq2inmodel->num_frames) < 1 ||
		LittleLong(pq2inmodel->num_skins) < 0 ||
		LittleLong(pq2inmodel->num_tris) < 1 ||
		LittleLong(pq2inmodel->num_xyz) < 3 ||
		LittleLong(pq2inmodel->num_st) < 3 ||
		LittleLong(pq2inmodel->skinheight) < 1 ||
		LittleLong(pq2inmodel->skinwidth) < 1)
	{
		Con_Printf(CON_ERROR "Model %s has an invalid quantity\n", mod->name);
		return false;
	}

	mod->flags = 0;

	mod->numframes = LittleLong(pq2inmodel->num_frames);

	size = sizeof(galiasinfo_t)
#ifndef SERVERONLY
		+ LittleLong(pq2inmodel->num_skins)*sizeof(galiasskin_t)
#endif
		+ LittleLong(pq2inmodel->num_frames)*sizeof(galiasanimation_t);

	galias = ZG_Malloc(&mod->memgroup, size);
	Mod_DefaultMesh(galias, mod->name, 0);
	galias->ofsanimations = (galiasanimation_t*)(galias+1);
#ifndef SERVERONLY
	galias->ofsskins = (galiasskin_t*)(galias->ofsanimations + LittleLong(pq2inmodel->num_frames));
#endif
	galias->nextsurf = 0;

//skins
	Q2MD2_LoadSkins(galias, mod, pq2inmodel, ((char *)pq2inmodel+LittleLong(pq2inmodel->ofs_skins)));

	//trianglelists;
	pintri = (dmd2triangle_t *)((char *)pq2inmodel + LittleLong(pq2inmodel->ofs_tris));


	for (i=0 ; i<LittleLong(pq2inmodel->num_tris) ; i++, pintri++)
	{
		for (j=0 ; j<3 ; j++)
		{
			ptempindex[i*3+j] = ( unsigned short )LittleShort ( pintri->xyz_index[j] );
			ptempstindex[i*3+j] = ( unsigned short )LittleShort ( pintri->st_index[j] );
		}
	}

	numindexes = galias->numindexes = LittleLong(pq2inmodel->num_tris)*3;
	indexes = ZG_Malloc(&mod->memgroup, galias->numindexes*sizeof(*indexes));
	galias->ofs_indexes = indexes;
	memset ( indremap, -1, sizeof(indremap) );
	numverts=0;

	for ( i = 0; i < numindexes; i++ )
	{
		if ( indremap[i] != -1 ) {
			continue;
		}

		for ( j = 0; j < numindexes; j++ )
		{
			if ( j == i ) {
				continue;
			}

			if ( (ptempindex[i] == ptempindex[j]) && (ptempstindex[i] == ptempstindex[j]) ) {
				indremap[j] = i;
			}
		}
	}

	// count unique vertexes
	for ( i = 0; i < numindexes; i++ )
	{
		if ( indremap[i] != -1 ) {
			continue;
		}

		indexes[i] = numverts++;
		indremap[i] = i;
	}

//	Con_DPrintf ( "%s: remapped %i verts to %i\n", mod->name, LittleLong(pq2inmodel->num_xyz), numverts );

	galias->numverts = numverts;

	// remap remaining indexes
	for ( i = 0; i < numindexes; i++ )
	{
		if ( indremap[i] != i ) {
			indexes[i] = indexes[indremap[i]];
		}
	}

// s and t vertices
#ifndef SERVERONLY
	pinstverts = ( dmd2stvert_t * ) ( ( qbyte * )pq2inmodel + LittleLong (pq2inmodel->ofs_st) );
	st_array = ZG_Malloc(&mod->memgroup, sizeof(*st_array)*(numverts));
	galias->ofs_st_array = st_array;

	for (j=0 ; j<numindexes; j++)
	{
		st_array[indexes[j]][0] = (float)(((double)LittleShort (pinstverts[ptempstindex[indremap[j]]].s) + 0.5f) /LittleLong(pq2inmodel->skinwidth));
		st_array[indexes[j]][1] = (float)(((double)LittleShort (pinstverts[ptempstindex[indremap[j]]].t) + 0.5f) /LittleLong(pq2inmodel->skinheight));
	}
#endif

	//frames
	ClearBounds ( mod->mins, mod->maxs );

	poutframe = galias->ofsanimations;
	framesize = LittleLong (pq2inmodel->framesize);
	for (i=0 ; i<LittleLong(pq2inmodel->num_frames) ; i++)
	{
		size = sizeof(galiaspose_t) + sizeof(vecV_t)*numverts;
#ifndef SERVERONLY
		size += 3*sizeof(vec3_t)*numverts;
#endif
		pose = (galiaspose_t *)ZG_Malloc(&mod->memgroup, size);
		poutframe->poseofs = pose;
		poutframe->numposes = 1;
		galias->numanimations++;

		verts = (vecV_t *)(pose+1);
		pose->ofsverts = verts;
#ifndef SERVERONLY
		normals = (vec3_t*)&verts[galias->numverts];
		pose->ofsnormals = normals;

		pose->ofssvector = &normals[galias->numverts];
		pose->ofstvector = &normals[galias->numverts*2];
#endif


		pinframe = ( dmd2aliasframe_t * )( ( qbyte * )pq2inmodel + LittleLong (pq2inmodel->ofs_frames) + i * framesize );
		Q_strncpyz(poutframe->name, pinframe->name, sizeof(poutframe->name));

		for (j=0 ; j<3 ; j++)
		{
			pose->scale[j] = LittleFloat (pinframe->scale[j]);
			pose->scale_origin[j] = LittleFloat (pinframe->translate[j]);
		}

		for (j=0 ; j<numindexes; j++)
		{
			// verts are all 8 bit, so no swapping needed
			verts[indexes[j]][0] = pose->scale_origin[0]+pose->scale[0]*pinframe->verts[ptempindex[indremap[j]]].v[0];
			verts[indexes[j]][1] = pose->scale_origin[1]+pose->scale[1]*pinframe->verts[ptempindex[indremap[j]]].v[1];
			verts[indexes[j]][2] = pose->scale_origin[2]+pose->scale[2]*pinframe->verts[ptempindex[indremap[j]]].v[2];
#ifndef SERVERONLY
			VectorCopy(bytedirs[pinframe->verts[ptempindex[indremap[j]]].lightnormalindex], normals[indexes[j]]);
#endif
		}

//		Mod_AliasCalculateVertexNormals ( numindexes, poutindex, numverts, poutvertex, qfalse );

		VectorCopy ( pose->scale_origin, min );
		VectorMA ( pose->scale_origin, 255, pose->scale, max );

//		poutframe->radius = RadiusFromBounds ( min, max );

//		mod->radius = max ( mod->radius, poutframe->radius );
		AddPointToBounds ( min, mod->mins, mod->maxs );
		AddPointToBounds ( max, mod->mins, mod->maxs );

//		GL_GenerateNormals((float*)verts, (float*)normals, indexes, numindexes/3, numverts);

		poutframe++;
	}



	Mod_CompileTriangleNeighbours(mod, galias);
	Mod_BuildTextureVectors(galias);
	/*
	VectorCopy (pq2inmodel->scale_origin, mod->mins);
	VectorMA (mod->mins, 255, pq2inmodel->scale, mod->maxs);
	*/

	Mod_ClampModelSize(mod);
	Mesh_HandleFramegroupsFile(mod, galias);
	Mod_ParseModelEvents(mod, galias->ofsanimations, galias->numanimations);

	mod->meshinfo = galias;
	mod->numframes = galias->numanimations;
	mod->type = mod_alias;

	mod->funcs.NativeTrace = Mod_Trace;
	return true;
}

#endif

#ifdef MODELFMT_MDX

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Kingpin model loading
//basically md2, but with toggleable subobjects and object bounding boxes (fixme: use those for hitmesh instead of the mesh itself)

typedef struct
{
	float		scale[3];	// multiply qbyte verts by this
	float		translate[3];	// then add this
	char		name[16];	// frame name from grabbing
	dtrivertx_t	verts[1];	// variable sized
} dmdxframe_t;

typedef struct
{
	int cmd;
	int obj;
	struct dmdxcommandvert_s
	{
		float s, t;
		int origvert;
	} vert[1];
} dmdxcommand_t;

typedef struct
{
   int magic;
   int version;
   int skinWidth;
   int skinHeight;
   int frameSize;
   int numSkins;
   int numVertices;
   int numTriangles_Unusable;
   int numGlCommands;
   int numFrames;
   int numSfxDefines_Unsupported;
   int numSfxEntries_Unsupported;
   int numSubObjects;
   int offsetSkins;
   int offsetTriangles_Unusable;
   int offsetFrames;
   int offsetGlCommands;
   int offsetVertexInfo_Whatever;
   int offsetSfxDefines_Unsupported;
   int offsetSfxEntries_Unsupported;
   int offsetBBoxFrames_HitmeshInstead;
   int offsetMaybeST_Bugged;
   int offsetEnd;
} dmdxheader_t;

#define MDX_IDENT "IDPX",4
#define MDX_VERSION 4

#define Q2NUMVERTEXNORMALS	162
extern vec3_t	bytedirs[Q2NUMVERTEXNORMALS];

#ifndef SERVERONLY
static void MDX_LoadSkins(galiasinfo_t *galias, model_t *mod, dmdxheader_t *pinmodel, char *skins)
{
	int i;
	skinframe_t *frames;
	galiasskin_t *outskin = galias->ofsskins;

	for (i = 0; i < LittleLong(pinmodel->numSkins); i++, outskin++)
	{
		frames = ZG_Malloc(&mod->memgroup, sizeof(*frames));
		outskin->frame = frames;
		outskin->numframes=1;

		COM_CleanUpPath(skins);	//blooming tanks.
		Q_strncpyz(frames->shadername, skins, sizeof(frames->shadername));

		outskin->skinwidth = 0;
		outskin->skinheight = 0;
		outskin->skinspeed = 0;

		skins += MD2MAX_SKINNAME;
	}
	galias->numskins = LittleLong(pinmodel->numSkins);
}
#endif

#define MDX_MAX_TRIANGLES 4096
#define MDX_MAX_VERTS MDX_MAX_TRIANGLES*3
static int MDX_MatchVert(struct dmdxcommandvert_s *list, int *count, struct dmdxcommandvert_s *newvert)
{
	int i;
	for (i = 0; i < *count; i++)
	{
		if (list[i].origvert == newvert->origvert &&
			list[i].s == newvert->s &&
			list[i].t == newvert->t)
			return i;	//its a dupe!
	}
	if (i == MDX_MAX_VERTS)
		return 0;
	list[i] = *newvert;
	*count+=1;
	return i;
}

static qboolean QDECL Mod_LoadKingpinModel (model_t *mod, void *buffer, size_t fsize)
{
#ifndef SERVERONLY
	vec2_t *st_array;
	vec3_t *normals;
#endif
	dmdxheader_t *pinmodel;

	int version;
	int i, j, subobj;
	index_t *indexes;

	vec3_t min;
	vec3_t max;

	galiaspose_t *pose;
	galiasanimation_t *poutframe;
	dmdxframe_t *pinframe;
	int framesize;
	vecV_t *verts;
	dmdxcommand_t *pincmd, *pincmdend;

	struct dmdxcommandvert_s tmpvert[MDX_MAX_VERTS];
	int numverts;
	struct
	{
		int newidx[3];
	} tri[MDX_MAX_TRIANGLES];
	int numtri;

	int size;
	galiasinfo_t *galias, *root;

	mod->engineflags |= MDLF_NEEDOVERBRIGHT;

	pinmodel = (dmdxheader_t *)buffer;

	if (fsize < sizeof(*pinmodel))
	{
		Con_Printf (CON_ERROR "%s is truncated\n", mod->name);
		return false;
	}
	version = LittleLong (pinmodel->version);
	if (version != MDX_VERSION)
	{
		Con_Printf (CON_ERROR "%s has wrong version number (%i should be %i)\n",
				 mod->name, version, MDX_VERSION);
		return false;
	}
	if (LittleLong(pinmodel->offsetEnd) != fsize)
	{
		Con_Printf (CON_ERROR "%s is truncated\n", mod->name);
		return false;
	}

	if (LittleLong(pinmodel->numFrames) < 1 ||
		LittleLong(pinmodel->numSkins) < 0 ||
		LittleLong(pinmodel->numTriangles_Unusable) < 1 ||
		LittleLong(pinmodel->numVertices) < 3 ||
		LittleLong(pinmodel->skinHeight) < 1 ||
		LittleLong(pinmodel->skinWidth) < 1 ||
		LittleLong(pinmodel->numSubObjects) < 1)
	{
		Con_Printf(CON_ERROR "Model %s has an invalid quantity\n", mod->name);
		return false;
	}

	mod->flags = 0;

	mod->numframes = LittleLong(pinmodel->numFrames);

	size = sizeof(galiasinfo_t)*pinmodel->numSubObjects
		+ LittleLong(pinmodel->numFrames)*sizeof(galiasanimation_t)*pinmodel->numSubObjects
#ifndef SERVERONLY
		+ LittleLong(pinmodel->numSkins)*sizeof(galiasskin_t)
#endif
		;

	root = galias = ZG_Malloc(&mod->memgroup, size);

#ifndef SERVERONLY
//skins
	galias->ofsskins = (galiasskin_t*)((galiasanimation_t*)(galias+pinmodel->numSubObjects) + LittleLong(pinmodel->numFrames)*pinmodel->numSubObjects);
	MDX_LoadSkins(galias, mod, pinmodel, ((char *)pinmodel+LittleLong(pinmodel->offsetSkins)));
#else
	galias->numskins = LittleLong(pinmodel->numSkins);
#endif

	ClearBounds (mod->mins, mod->maxs);
	for (subobj = 0; subobj < pinmodel->numSubObjects; subobj++)
	{
		galias = &root[subobj];
		Mod_DefaultMesh(galias, mod->name, subobj);
		galias->ofsanimations = (galiasanimation_t*)(root+pinmodel->numSubObjects) + subobj*pinmodel->numFrames;
		galias->ofsskins = root->ofsskins;
		galias->numskins = root->numskins;
		galias->shares_verts = subobj;
		if (subobj > 0)
			root[subobj-1].nextsurf = galias;

		//process the strips+fans, and split the verts into the appropriate submesh
		pincmd = (dmdxcommand_t *)((char *)pinmodel + LittleLong(pinmodel->offsetGlCommands));
		pincmdend = (dmdxcommand_t *)((char *)pinmodel + LittleLong(pinmodel->offsetGlCommands) + LittleLong(pinmodel->numGlCommands)*4);
		numverts = 0;
		numtri = 0;
		while (pincmd < pincmdend)
		{
			int n = LittleLong(pincmd->cmd);
			if (!n)
				break; //no more commands
			if (n < 0)
			{	//fan club
				n = -n;
				if (n > 2 && LittleLong(pincmd->obj) == subobj)
				{
					int first = MDX_MatchVert(tmpvert, &numverts, &pincmd->vert[0]);
					int prev = MDX_MatchVert(tmpvert, &numverts, &pincmd->vert[1]);
					for (i = 2; i < n && numtri < countof(tri); i++)
					{
						tri[numtri].newidx[0] = first;
						tri[numtri].newidx[1] = prev;
						tri[numtri].newidx[2] = prev = MDX_MatchVert(tmpvert, &numverts, &pincmd->vert[i]);
						numtri++;
					}
				}
			}
			else
			{	//stripper
				if (n > 2 && LittleLong(pincmd->obj) == subobj)
				{
					int first = MDX_MatchVert(tmpvert, &numverts, &pincmd->vert[0]);
					int prev = MDX_MatchVert(tmpvert, &numverts, &pincmd->vert[1]);
					for (i = 2; i < n && numtri < countof(tri); i++)
					{
						tri[numtri].newidx[2] = MDX_MatchVert(tmpvert, &numverts, &pincmd->vert[i]);
						if (i&1)
						{
							tri[numtri].newidx[0] = prev;
							tri[numtri].newidx[1] = first;
						}
						else
						{
							tri[numtri].newidx[0] = first;
							tri[numtri].newidx[1] = prev;
						}
						first = prev;
						prev = tri[numtri].newidx[2];

						if (tri[numtri].newidx[0] == tri[numtri].newidx[1] ||
							tri[numtri].newidx[0] == tri[numtri].newidx[2] ||
							tri[numtri].newidx[1] == tri[numtri].newidx[2])
							continue;	//degenerate... I doubt we'll see any though.
						numtri++;
					}
				}
			}
			pincmd = (dmdxcommand_t*)&pincmd->vert[n];
		}
		for (i = 0; i < numverts; i++)
		{	//might as well byteswap that stuff now...
			tmpvert[i].origvert = LittleLong(tmpvert[i].origvert);
			tmpvert[i].s = LittleFloat(tmpvert[i].s);
			tmpvert[i].t = LittleFloat(tmpvert[i].t);
		}

		galias->numverts = numverts;
		galias->numindexes = numtri*3;
		indexes = ZG_Malloc(&mod->memgroup, galias->numindexes*sizeof(*indexes));
		galias->ofs_indexes = indexes;

		for (i = 0; i < numtri; i++, indexes+=3)
		{
			indexes[0] = tri[i].newidx[0];
			indexes[1] = tri[i].newidx[1];
			indexes[2] = tri[i].newidx[2];
		}

		// s and t vertices
#ifndef SERVERONLY
		st_array = ZG_Malloc(&mod->memgroup, sizeof(*st_array)*(numverts));
		galias->ofs_st_array = st_array;

		for (j=0 ; j<numverts; j++)
		{
			st_array[j][0] = tmpvert[j].s;
			st_array[j][1] = tmpvert[j].t;
		}
#endif

		//frames
		poutframe = galias->ofsanimations;
		framesize = LittleLong (pinmodel->frameSize);

		size = sizeof(galiaspose_t) + sizeof(vecV_t)*numverts;
#ifndef SERVERONLY
		size += 3*sizeof(vec3_t)*numverts;
#endif
		size *= pinmodel->numFrames;
		pose = (galiaspose_t *)ZG_Malloc(&mod->memgroup, size);
		verts = (vecV_t*)(pose+pinmodel->numFrames);
#ifndef SERVERONLY
		normals = (vec3_t*)(verts+pinmodel->numFrames*numverts);
#endif

		for (i=0 ; i<LittleLong(pinmodel->numFrames) ; i++)
		{
			poutframe->poseofs = pose;
			poutframe->numposes = 1;
			poutframe->action = -1;
			poutframe->actionweight = 0;
			galias->numanimations++;

#ifndef SERVERONLY
			pose->ofsnormals = normals;
			pose->ofssvector = &normals[galias->numverts];
			pose->ofstvector = &normals[galias->numverts*2];
#endif

			pinframe = ( dmdxframe_t * )( ( qbyte * )pinmodel + LittleLong (pinmodel->offsetFrames) + i * framesize );
			Q_strncpyz(poutframe->name, pinframe->name, sizeof(poutframe->name));

			for (j=0 ; j<3 ; j++)
			{
				pose->scale[j] = LittleFloat (pinframe->scale[j]);
				pose->scale_origin[j] = LittleFloat (pinframe->translate[j]);
			}

			pose->ofsverts = verts;
			for (j=0 ; j<numverts; j++)
			{
				// verts are all 8 bit, so no swapping needed
				verts[j][0] = pose->scale_origin[0]+pose->scale[0]*pinframe->verts[tmpvert[j].origvert].v[0];
				verts[j][1] = pose->scale_origin[1]+pose->scale[1]*pinframe->verts[tmpvert[j].origvert].v[1];
				verts[j][2] = pose->scale_origin[2]+pose->scale[2]*pinframe->verts[tmpvert[j].origvert].v[2];
#ifndef SERVERONLY
				VectorCopy(bytedirs[pinframe->verts[tmpvert[j].origvert].lightnormalindex], normals[j]);
#endif
			}

			VectorCopy ( pose->scale_origin, min );
			VectorMA ( pose->scale_origin, 255, pose->scale, max );

			AddPointToBounds ( min, mod->mins, mod->maxs );
			AddPointToBounds ( max, mod->mins, mod->maxs );

			poutframe++;
			pose++;
			verts += numverts;
#ifndef SERVERONLY
			normals += numverts*3;
#endif
		}

		Mod_CompileTriangleNeighbours(mod, galias);
		Mod_BuildTextureVectors(galias);
	}

	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
	Mod_ClampModelSize(mod);
	Mesh_HandleFramegroupsFile(mod, galias);
	Mod_ParseModelEvents(mod, root->ofsanimations, root->numanimations);

	mod->meshinfo = root;
	mod->numframes = root->numanimations;
	mod->type = mod_alias;

	mod->funcs.NativeTrace = Mod_Trace;

	return true;
}
#endif







int Mod_GetNumBones(model_t *model, qboolean allowtags)
{
#ifdef SKELORTAGS
	if (model && model->type == mod_alias)
	{
		galiasinfo_t *inf = Mod_Extradata(model);
#ifdef SKELETALMODELS
		if (inf->numbones)
			return inf->numbones;
		else
#endif
#ifdef MD3MODELS
			if (allowtags)
			return inf->numtags;
#endif
		return 0;
	}
#endif
#ifdef HALFLIFEMODELS
	if (model && model->type == mod_halflife)
		return HLMDL_GetNumBones(model, allowtags);
#endif
	return 0;
}

int Mod_GetBoneRelations(model_t *model, int firstbone, int lastbone, const galiasbone_t *boneinfo, const framestate_t *fstate, float *result)
{
#ifdef SKELETALMODELS
	if (model && model->type == mod_alias)
		return Alias_BlendBoneData(Mod_Extradata(model), fstate, result, SKEL_RELATIVE, firstbone, lastbone, boneinfo);
#endif
#ifdef HALFLIFEMODELS
	if (model && model->type == mod_halflife)
		return HLMDL_GetBoneData(model, firstbone, lastbone, fstate, result);
#endif
	return 0;
}

galiasbone_t *Mod_GetBoneInfo(model_t *model, int *numbones)
{
#ifdef SKELETALMODELS
	if (model && model->type == mod_alias)
	{
		galiasinfo_t *inf = Mod_Extradata(model);
		*numbones = inf->numbones;
		return inf->ofsbones;
	}
#endif
#ifdef HALFLIFEMODELS
	if (model && model->type == mod_halflife)
	{
		hlmodel_t *hlmod = Mod_Extradata(model);
		*numbones = hlmod->header->numbones;
		return hlmod->compatbones;
	}
#endif
	*numbones = 0;
	return NULL;
}

int Mod_GetBoneParent(model_t *model, int bonenum)
{
#ifdef SKELETALMODELS
	if (model && model->type == mod_alias)
	{
		galiasbone_t *bone;
		galiasinfo_t *inf;
		inf = Mod_Extradata(model);


		bonenum--;
		if ((unsigned int)bonenum >= inf->numbones)
			return 0;	//no parent
		bone = inf->ofsbones;
		return bone[bonenum].parent+1;
	}
#endif
#ifdef HALFLIFEMODELS
	if (model && model->type == mod_halflife)
		return HLMDL_GetBoneParent(model, bonenum-1)+1;
#endif
	return 0;
}

const char *Mod_GetBoneName(model_t *model, int bonenum)
{
#ifdef SKELETALMODELS
	if (model && model->type == mod_alias)
	{
		galiasbone_t *bone;
		galiasinfo_t *inf;
		inf = Mod_Extradata(model);


		bonenum--;
		if ((unsigned int)bonenum >= inf->numbones)
			return 0;	//no parent
		bone = inf->ofsbones;
		return bone[bonenum].name;
	}
#endif
#ifdef HALFLIFEMODELS
	if (model && model->type == mod_halflife)
		return HLMDL_GetBoneName(model, bonenum-1);
#endif
	return 0;
}

qboolean Mod_GetTag(model_t *model, int tagnum, framestate_t *fstate, float *result)
{
#ifdef HALFLIFEMODELS
	if (model && model->type == mod_halflife)
	{
		int numbones = Mod_GetNumBones(model, true);
		if (tagnum > 0 && tagnum <= numbones)
		{
			float relatives[MAX_BONES*12];
			float tempmatrix[12];			//flipped between this and bonematrix
			float *matrix;	//the matrix for a single bone in a single pose.
			float *lerps = relatives;
			int k;

			if (tagnum <= 0 || tagnum > numbones)
				return false;
			tagnum--;	//tagnum 0 is 'use my angles/org'

			//data comes from skeletal object, if possible
			if (fstate->bonestate)
			{
				numbones = fstate->bonecount;
				lerps = fstate->bonestate;

				if (tagnum >= numbones)
					return false;

				if (fstate->skeltype == SKEL_ABSOLUTE)
				{	//can just directly read it, woo.
					memcpy(result, fstate->bonestate + 12 * tagnum, 12*sizeof(*result));
					return true;
				}
			}
			else //try getting the data from the frame state
			{
				numbones = Mod_GetBoneRelations(model, 0, tagnum+1, NULL, fstate, relatives);
				lerps = relatives;
			}

			//set up the identity matrix
			for (k = 0;k < 12;k++)
				result[k] = 0;
			result[0] = 1;
			result[5] = 1;
			result[10] = 1;
			if (tagnum >= numbones)
				tagnum = HLMDL_GetAttachment(model, tagnum-numbones, result);
			while(tagnum >= 0)
			{
				//set up the per-bone transform matrix
				matrix = lerps + tagnum*12;
				memcpy(tempmatrix, result, sizeof(tempmatrix));
				R_ConcatTransforms((void*)matrix, (void*)tempmatrix, (void*)result);
				tagnum = Mod_GetBoneParent(model, tagnum+1)-1;
			}

			return true;
		}
	}
#endif

#ifdef SKELORTAGS
	if (model && model->type == mod_alias)
	{
		galiasinfo_t *inf = Mod_Extradata(model);
#ifdef SKELETALMODELS
		if (inf->numbones)
		{
			galiasbone_t *bone = inf->ofsbones;

			float tempmatrix[12];			//flipped between this and bonematrix
			float *matrix;	//the matrix for a single bone in a single pose.
			float m[12];	//combined interpolated version of 'matrix'.
			int b, k;	//counters

			int numbonegroups = 0;
			skellerps_t lerps[FS_COUNT], *lerp;

			if (tagnum <= 0 || tagnum > inf->numbones)
				return false;
			tagnum--;	//tagnum 0 is 'use my angles/org'

			//data comes from skeletal object, if possible
			if (!numbonegroups && fstate->bonestate)
			{
				if (tagnum >= fstate->bonecount)
					return false;

				if (fstate->skeltype == SKEL_ABSOLUTE)
				{	//can just directly read it, woo.
					memcpy(result, fstate->bonestate + 12 * tagnum, 12*sizeof(*result));
					return true;
				}

				lerps[0].pose[0] = fstate->bonestate;
				lerps[0].frac[0] = 1;
				lerps[0].needsfree[0] = 0;
				lerps[0].lerpcount = 1;
				lerps[0].firstbone = 0;
				lerps[0].endbone = fstate->bonecount;
				lerps[0].skeltype = fstate->skeltype;
				numbonegroups = 1;
			}

			//try getting the data from the frame state
			if (!numbonegroups)
				numbonegroups = Alias_FindRawSkelData(inf, fstate, lerps, 0, inf->numbones, NULL);

			//try base pose?
			if (!numbonegroups && inf->baseframeofs)
			{
				lerps[0].pose[0] = inf->baseframeofs;
				lerps[0].frac[0] = 1;
				lerps[0].needsfree[0] = 0;
				lerps[0].lerpcount = 1;
				lerps[0].firstbone = 0;
				lerps[0].endbone = inf->numbones;
				lerps[0].skeltype = SKEL_ABSOLUTE;
				numbonegroups = 1;
			}

			//make sure it was all okay.
			if (!numbonegroups || tagnum >= lerps[numbonegroups-1].endbone)
				return false;

			//set up the identity matrix
			for (k = 0;k < 12;k++)
				result[k] = 0;
			result[0] = 1;
			result[5] = 1;
			result[10] = 1;
			while(tagnum >= 0)
			{
				for (lerp = lerps; tagnum < lerp->firstbone; lerp++)
					;
				//set up the per-bone transform matrix
				matrix = lerp->pose[0] + tagnum*12;
				for (k = 0;k < 12;k++)
					m[k] = matrix[k] * lerp->frac[0];
				for (b = 1;b < lerp->lerpcount;b++)
				{
					matrix = lerp->pose[b] + tagnum*12;
					for (k = 0;k < 12;k++)
						m[k] += matrix[k] * lerp->frac[b];
				}

				if (lerp->skeltype == SKEL_ABSOLUTE)
				{
					memcpy(result, m, sizeof(tempmatrix));
					break;
				}

				memcpy(tempmatrix, result, sizeof(tempmatrix));
				R_ConcatTransforms((void*)m, (void*)tempmatrix, (void*)result);

				tagnum = bone[tagnum].parent;
			}

			for (b = 0; b < numbonegroups; b++)
				for (k = 0; k < lerps[b].lerpcount; k++)
					BZ_Free(lerps[b].needsfree[k]);
			return true;
		}
#endif
#ifdef MD3MODELS
		if (inf->numtags)
		{
			md3tag_t *t1, *t2;

			int frame1, frame2;
			//float f1time, f2time;	//tags/md3s don't support framegroups.
			float f2ness;

#if FRAME_BLENDS != 2
			if (fstate->g[FS_REG].lerpweight[2] || fstate->g[FS_REG].lerpweight[3])
			{
				static float printtimer;
				Con_ThrottlePrintf(&printtimer, 1, "Mod_GetTag(%s): non-skeletal animation only supports two animations\n", model->name);
			}
#endif

			frame1 = fstate->g[FS_REG].frame[0];
			frame2 = fstate->g[FS_REG].frame[1];
			//f1time = fstate->g[FS_REG].frametime[0];
			//f2time = fstate->g[FS_REG].frametime[1];
			f2ness = fstate->g[FS_REG].lerpweight[1];

			if (tagnum <= 0 || tagnum > inf->numtags)
				return false;
			if (frame1 < 0)
				return false;
			if (frame1 >= inf->numtagframes)
				frame1 = inf->numtagframes - 1;
			if (frame2 < 0 || frame2 >= inf->numtagframes)
				frame2 = frame1;
			tagnum--;	//tagnum 0 is 'use my angles/org'

			t1 = inf->ofstags;
			t1 += tagnum;
			t1 += inf->numtags*frame1;

			t2 = inf->ofstags;
			t2 += tagnum;
			t2 += inf->numtags*frame2;

			if (t1 == t2)
			{
				result[0]	= t1->ang[0][0];
				result[1]	= t1->ang[0][1];
				result[2]	= t1->ang[0][2];
				result[3]	= t1->org[0];
				result[4]	= t1->ang[1][0];
				result[5]	= t1->ang[1][1];
				result[6]	= t1->ang[1][2];
				result[7]	= t1->org[1];
				result[8]	= t1->ang[2][0];
				result[9]	= t1->ang[2][1];
				result[10]	= t1->ang[2][2];
				result[11]	= t1->org[2];
			}
			else
			{
				float f1ness = 1-f2ness;
				result[0]	= t1->ang[0][0]*f1ness	+ t2->ang[0][0]*f2ness;
				result[1]	= t1->ang[0][1]*f1ness	+ t2->ang[0][1]*f2ness;
				result[2]	= t1->ang[0][2]*f1ness	+ t2->ang[0][2]*f2ness;
				result[3]	= t1->org[0]*f1ness		+ t2->org[0]*f2ness;
				result[4]	= t1->ang[1][0]*f1ness	+ t2->ang[1][0]*f2ness;
				result[5]	= t1->ang[1][1]*f1ness	+ t2->ang[1][1]*f2ness;
				result[6]	= t1->ang[1][2]*f1ness	+ t2->ang[1][2]*f2ness;
				result[7]	= t1->org[1]*f1ness		+ t2->org[1]*f2ness;
				result[8]	= t1->ang[2][0]*f1ness	+ t2->ang[2][0]*f2ness;
				result[9]	= t1->ang[2][1]*f1ness	+ t2->ang[2][1]*f2ness;
				result[10]	= t1->ang[2][2]*f1ness	+ t2->ang[2][2]*f2ness;
				result[11]	= t1->org[2]*f1ness		+ t2->org[2]*f2ness;
			}

			VectorNormalize(result);
			VectorNormalize(result+4);
			VectorNormalize(result+8);

			return true;
		}
#endif
	}
#endif
	return false;
}

int Mod_TagNumForName(model_t *model, const char *name, int firsttag)
{
#ifdef SKELORTAGS
	int i;
	galiasinfo_t *inf;

	if (!model)
		return 0;
	if (model->loadstate != MLS_LOADED)
	{
		if (model->loadstate == MLS_NOTLOADED)
			Mod_LoadModel(model, MLV_SILENT);
		if (model->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(model, &model->loadstate, MLS_LOADING);
		if (model->loadstate != MLS_LOADED)
			return 0;
	}

#ifdef HALFLIFEMODELS
	if (model->type == mod_halflife)
		return HLMDL_BoneForName(model, name);
#endif
	if (model->type == mod_alias)
	{
		inf = Mod_Extradata(model);

#ifdef SKELETALMODELS
		if (inf->numbones)
		{
			galiasbone_t *b;
			b = inf->ofsbones;
			for (i = firsttag; i < inf->numbones; i++)
			{
				if (!strcmp(b[i].name, name))
					return i+1;
			}
		}
#endif
#ifdef MD3MODELS
		if (inf->numtags)
		{
			md3tag_t *t = inf->ofstags;
			for (i = firsttag; i < inf->numtags; i++)
			{
				if (!strcmp(t[i].name, name))
					return i+1;
			}
		}
#endif
		return 0;
	}
#endif
	return 0;
}

int Mod_FrameNumForName(model_t *model, int surfaceidx, const char *name)
{
	galiasanimation_t *group;
	galiasinfo_t *inf;
	int i;

	if (!model)
		return -1;
#ifdef HALFLIFEMODELS
	if (model->type == mod_halflife)
		return HLMDL_FrameForName(model, name);
#endif
	if (model->type != mod_alias)
		return 0;

	inf = Mod_Extradata(model);

	while(surfaceidx-->0 && inf)
		inf = inf->nextsurf;
	if (inf)
	{
		group = inf->ofsanimations;
		for (i = 0; i < inf->numanimations; i++, group++)
		{
			if (!strcmp(group->name, name))
				return i;
		}
	}
	return -1;
}

int Mod_FrameNumForAction(model_t *model, int surfaceidx, int actionid)
{
	galiasanimation_t *group;
	galiasinfo_t *inf;
	int i;
	float weight;

	if (!model)
		return -1;
#ifdef HALFLIFEMODELS
	if (model->type == mod_halflife)
		return HLMDL_FrameForAction(model, actionid);
#endif
	if (model->type != mod_alias)
		return -1;

	inf = Mod_Extradata(model);

	while(surfaceidx-->0 && inf)
		inf = inf->nextsurf;
	if (inf)
	{
		for (i = 0, weight = 0, group = inf->ofsanimations; i < inf->numanimations; i++, group++)
		{
			if (group->action == actionid)
				weight += group->actionweight;
		}
		weight *= frandom();
		for (i = 0, group = inf->ofsanimations; i < inf->numanimations; i++, group++)
		{
			if (group->action == actionid)
			{
				if (weight <= group->actionweight)
					return i;
				weight -= group->actionweight;
			}
		}
	}
	return -1;
}


qboolean Mod_GetModelEvent(model_t *model, int animation, int eventidx, float *timestamp, int *eventcode, char **eventdata)
{
	if (!model)
		return false;
	if (model->type == mod_alias)
	{
		galiasinfo_t *inf = Mod_Extradata(model);
		if (inf && animation >= 0 && animation < inf->numanimations)
		{
			galiasanimation_t *anim = inf->ofsanimations + animation;
			galiasevent_t *ev = anim->events;
			while (eventidx-->0 && ev)
				ev = ev->next;
			if (ev)
			{
				*timestamp = ev->timestamp;
				*eventcode = ev->code;
				*eventdata = ev->data;
				return true;
			}
		}
	}
#ifdef HALFLIFEMODELS
	if (model->type == mod_halflife)
		return HLMDL_GetModelEvent(model, animation, eventidx, timestamp, eventcode, eventdata);
#endif
	return false;
}

#ifndef SERVERONLY
int Mod_SkinNumForName(model_t *model, int surfaceidx, const char *name)
{
	int i;
	galiasinfo_t *inf;
	galiasskin_t *skin;

	if (!model || model->loadstate != MLS_LOADED)
	{
		if (model && model->loadstate == MLS_NOTLOADED)
			Mod_LoadModel(model, MLV_SILENT);
		if (model && model->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(model, &model->loadstate, MLS_LOADING);
		if (!model || model->loadstate != MLS_LOADED || model->type != mod_alias)
			return -1;
	}
	inf = Mod_Extradata(model);

	while(surfaceidx-->0 && inf)
		inf = inf->nextsurf;
	if (inf)
	{
		skin = inf->ofsskins;
		for (i = 0; i < inf->numskins; i++, skin++)
		{
			if (!strcmp(skin->name, name))
				return i;
		}
	}
	return -1;
}
#endif

const char *Mod_FrameNameForNum(model_t *model, int surfaceidx, int num)
{
	galiasanimation_t *group;
	galiasinfo_t *inf;

	if (!model || model->loadstate != MLS_LOADED)
	{
		if (model && model->loadstate == MLS_NOTLOADED)
			Mod_LoadModel(model, MLV_SILENT);
		if (model && model->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(model, &model->loadstate, MLS_LOADING);
		if (!model || model->loadstate != MLS_LOADED)
			return NULL;
	}
	if (model->type == mod_alias)
	{
		inf = Mod_Extradata(model);

		while(surfaceidx-->0 && inf)
			inf = inf->nextsurf;

		if (!inf || (unsigned)num >= (unsigned)inf->numanimations)
			return NULL;
		group = inf->ofsanimations;
		return group[num].name;
	}
#ifdef HALFLIFEMODELS
	if (model->type == mod_halflife)
		return HLMDL_FrameNameForNum(model, surfaceidx, num);
#endif
	return NULL;
}

qboolean Mod_FrameInfoForNum(model_t *model, int surfaceidx, int num, char **name, int *numframes, float *duration, qboolean *loop, int *act)
{
	galiasanimation_t *group;
	galiasinfo_t *inf;

	if (!model || model->loadstate != MLS_LOADED)
	{
		if (model && model->loadstate == MLS_NOTLOADED)
			Mod_LoadModel(model, MLV_SILENT);
		if (model && model->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(model, &model->loadstate, MLS_LOADING);
		if (!model || model->loadstate != MLS_LOADED)
			return false;
	}
	if (model->type == mod_alias)
	{
		inf = Mod_Extradata(model);

		while(surfaceidx-->0 && inf)
			inf = inf->nextsurf;

		if (!inf || num >= inf->numanimations)
			return false;
		group = inf->ofsanimations;
		group += num;

		*name = group->name;
		*numframes = group->numposes;
		*loop = group->loop;
		*duration = group->numposes/group->rate;
		*act = group->action;
		return true;
	}
#ifdef HALFLIFEMODELS
	if (model->type == mod_halflife)
		return HLMDL_FrameInfoForNum(model, surfaceidx, num, name, numframes, duration, loop, act);
#endif
	return false;
}

#ifndef SERVERONLY
shader_t *Mod_ShaderForSkin(model_t *model, int surfaceidx, int num, float time, texnums_t **out_texnums)
{
	galiasinfo_t *inf;
	galiasskin_t *skin;
	skinframe_t *skinframe;

	*out_texnums = NULL;

	if (!model || model->loadstate != MLS_LOADED)
	{
		if (model && model->loadstate == MLS_NOTLOADED)
			Mod_LoadModel(model, MLV_SILENT);
		if (model && model->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(model, &model->loadstate, MLS_LOADING);
		if (!model || model->loadstate != MLS_LOADED)
			return NULL;
	}

	switch(model->type)
	{
	case mod_brush:
		if (surfaceidx < model->numtextures && !num)
			return model->textures[surfaceidx]->shader;
		return NULL;

	case mod_alias:
		inf = Mod_Extradata(model);

		while(surfaceidx-->0 && inf)
			inf = inf->nextsurf;

		if (!inf || num >= inf->numskins)
			return NULL;
		skin = inf->ofsskins;
		skin += num;
		skinframe = skin->frame;
		if (skin->numframes)
			skinframe += (int)(time*skin->skinspeed)%skin->numframes;
		*out_texnums = &skinframe->texnums;
		return skinframe->shader;
	default:
		return NULL;
	}
}
#endif
const char *Mod_SkinNameForNum(model_t *model, int surfaceidx, int num)
{
#ifdef SERVERONLY
	return NULL;
#else
	galiasinfo_t *inf;
	galiasskin_t *skin;

	if (!model || model->loadstate != MLS_LOADED)
	{
		if (model && model->loadstate == MLS_NOTLOADED)
			Mod_LoadModel(model, MLV_SILENT);
		if (model && model->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(model, &model->loadstate, MLS_LOADING);
		if (!model || model->loadstate != MLS_LOADED)
			return NULL;
	}

	switch(model->type)
	{
	case mod_brush:
		if (surfaceidx < model->numtextures && !num)
			return "";
		return NULL;
	case mod_alias:
		inf = Mod_Extradata(model);

		while(surfaceidx-->0 && inf)
			inf = inf->nextsurf;
		if (!inf || num >= inf->numskins)
			return NULL;
		skin = inf->ofsskins;
	//	if (!*skin[num].name)
	//		return skin[num].frame[0].shadername;
	//	else
			return skin[num].name;
	default:
		return NULL;
	}
#endif
}

const char *Mod_SurfaceNameForNum(model_t *model, int num)
{
#ifdef SERVERONLY
	return NULL;
#else
	galiasinfo_t *inf;

	if (!model || model->loadstate != MLS_LOADED)
	{
		if (model && model->loadstate == MLS_NOTLOADED)
			Mod_LoadModel(model, MLV_SILENT);
		if (model && model->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(model, &model->loadstate, MLS_LOADING);
		if (!model || model->loadstate != MLS_LOADED)
			return NULL;
	}

	switch(model->type)
	{
	case mod_brush:
		if (model->type == mod_brush && num < model->numtextures)
			return model->textures[num]->name;
		return NULL;
	case mod_halflife:
		return NULL;
	case mod_alias:
		inf = Mod_Extradata(model);

		while(num-->0 && inf)
			inf = inf->nextsurf;
		if (inf)
			return inf->surfacename;
		else
			return NULL;
	case mod_sprite:
	case mod_dummy:
	case mod_heightmap:
	default:
		return NULL;
	}
#endif
}

float Mod_GetFrameDuration(model_t *model, int surfaceidx, int frameno)
{
	galiasinfo_t *inf;
	galiasanimation_t *group;

#ifdef HALFLIFEMODELS
	if (model && model->type == mod_halflife) {
		int unused;
		float duration;
		char *name;
		qboolean loop;
		int act;
		HLMDL_FrameInfoForNum(model, surfaceidx, frameno, &name, &unused, &duration, &loop, &act);
		return duration;
	}
#endif
	
	if (!model || model->type != mod_alias)
		return 0;

	inf = Mod_Extradata(model);
	
	while(surfaceidx-->0 && inf)
		inf = inf->nextsurf;

	if (inf)
	{
		group = inf->ofsanimations;
		if (frameno >= 0 && frameno < inf->numanimations)
		{
			group += frameno;
			return group->numposes/group->rate;
		}
	}
	return 0;
}

int Mod_GetFrameCount(struct model_s *model)
{
	if (!model)
		return 0;
	return model->numframes;
}


#ifdef MD3MODELS

//structures from Tenebrae
typedef struct {
	int			ident;
	int			version;

	char		name[64];

	int			flags;	//Does anyone know what these are?

	int			numFrames;
	int			numTags;
	int			numSurfaces;

	int			numSkins;

	int			ofsFrames;
	int			ofsTags;
	int			ofsSurfaces;
	int			ofsEnd;
} md3Header_t;

//then has header->numFrames of these at header->ofs_Frames
typedef struct md3Frame_s {
	vec3_t		bounds[2];
	vec3_t		localOrigin;
	float		radius;
	char		name[16];
} md3Frame_t;

//there are header->numSurfaces of these at header->ofsSurfaces, following from ofsEnd
typedef struct {
	int		ident;				//

	char	name[64];	// polyset name

	int		flags;
	int		numFrames;			// all surfaces in a model should have the same

	int		numShaders;			// all surfaces in a model should have the same
	int		numVerts;

	int		numTriangles;
	int		ofsTriangles;

	int		ofsShaders;			// offset from start of md3Surface_t
	int		ofsSt;				// texture coords are common for all frames
	int		ofsXyzNormals;		// numVerts * numFrames

	int		ofsEnd;				// next surface follows
} md3Surface_t;

//at surf+surf->ofsXyzNormals
typedef struct {
	short		xyz[3];
	qbyte		latlong[2];
} md3XyzNormal_t;

//surf->numTriangles at surf+surf->ofsTriangles
typedef struct {
	int			indexes[3];
} md3Triangle_t;

//surf->numVerts at surf+surf->ofsSt
typedef struct {
	float		s;
	float		t;
} md3St_t;

typedef struct {
	char			name[64];
	int				shaderIndex;
} md3Shader_t;
//End of Tenebrae 'assistance'

//q3 loads foo.md3, foo_1.md3, foo_2.md3 etc
static galiasinfo_t *Mod_LoadQ3ModelLod(model_t *mod, int *surfcount, void *buffer, size_t fsize)
{
#ifndef SERVERONLY
	galiasskin_t	*skin;
	skinframe_t	*frames;
	float lat, lng;
	md3St_t			*inst;
	vec3_t *normals;
	vec3_t *svector;
	vec3_t *tvector;
	vec2_t *st_array;
	md3Shader_t		*inshader;
	int externalskins;
#endif
//	int version;
	int s, i, j, d;

	index_t *indexes;

	vec3_t min;
	vec3_t max;

	galiaspose_t *pose;
	galiasanimation_t *group;

	vecV_t *verts;

	md3Triangle_t	*intris;
	md3XyzNormal_t	*invert;
	galiasinfo_t	*first = NULL;
	galiasinfo_t	**surflink = &first;


	int size;

	md3Header_t		*header;
	md3Surface_t	*surf;
	galiasinfo_t	*galias;
	unsigned int	numposes, numverts;

	frameinfo_t		*framegroups;
	unsigned int	numgroups;

	header = buffer;

//	if (header->version != sdfs)
//		Sys_Error("GL_LoadQ3Model: Bad version\n");

#ifndef SERVERONLY
	externalskins = Mod_CountSkinFiles(mod);
#endif

	framegroups = ParseFrameInfo(mod, &numgroups);

	ClearBounds(min, max);

	surf = (md3Surface_t *)((qbyte *)header + LittleLong(header->ofsSurfaces));
	for (s = 0; s < LittleLong(header->numSurfaces); s++, *surfcount+=1)
	{
		if (memcmp(&surf->ident, MD3_IDENT))
			Con_Printf(CON_WARNING "Warning: md3 sub-surface doesn't match ident\n");

		if (!framegroups)
			numgroups = LittleLong(header->numFrames);
		numposes = LittleLong(surf->numFrames);
		numverts = LittleLong(surf->numVerts);

		size = sizeof(galiasinfo_t) + sizeof(galiasanimation_t)*numgroups;
		galias = ZG_Malloc(&mod->memgroup, size);
		Mod_DefaultMesh(galias, surf->name, s);
		galias->ofsanimations = group = (galiasanimation_t*)(galias+1);	//frame groups
		galias->numanimations = numgroups;
		galias->numverts = numverts;
		galias->numindexes = LittleLong(surf->numTriangles)*3;
		galias->shares_verts = *surfcount; //with itself, so no sharing.

		if (!first)
			first = galias;
		*surflink = galias;
		surflink = &galias->nextsurf;
		galias->nextsurf = NULL;

		//load the texcoords
#ifndef SERVERONLY
		st_array = ZG_Malloc(&mod->memgroup, sizeof(vec2_t)*galias->numindexes);
		galias->ofs_st_array = st_array;
		inst = (md3St_t*)((qbyte*)surf + LittleLong(surf->ofsSt));
		for (i = 0; i < galias->numverts; i++)
		{
			st_array[i][0] = LittleFloat(inst[i].s);
			st_array[i][1] = LittleFloat(inst[i].t);
		}
#endif

		//load the index data
		indexes = ZG_Malloc(&mod->memgroup, sizeof(*indexes)*galias->numindexes);
		galias->ofs_indexes = indexes;
		intris = (md3Triangle_t *)((qbyte*)surf + LittleLong(surf->ofsTriangles));
		for (i = 0; i < LittleLong(surf->numTriangles); i++)
		{
			indexes[i*3+0] = LittleLong(intris[i].indexes[0]);
			indexes[i*3+1] = LittleLong(intris[i].indexes[1]);
			indexes[i*3+2] = LittleLong(intris[i].indexes[2]);

			if (indexes[i*3+0] >= numverts || indexes[i*3+1] >= numverts || indexes[i*3+2] >= numverts)
			{
				Con_Printf(CON_WARNING "Warning: surface %s has invalid vertex indexes\n", galias->surfacename);
				indexes[i*3+0] = indexes[i*3+1] = indexes[i*3+2] = 0;
			}
		}

		//figure out where we're putting the pose data
		size = sizeof(galiaspose_t) + sizeof(vecV_t)*numverts;
#ifndef SERVERONLY
		size += 3*sizeof(vec3_t)*numverts;
#endif
		size *= numposes;
		pose = (galiaspose_t *)ZG_Malloc(&mod->memgroup, size);
		verts = (vecV_t*)(pose+numposes);
#ifndef SERVERONLY
		normals = (vec3_t*)(verts + numverts*numposes);
		svector = (vec3_t*)(normals + numverts*numposes);
		tvector = (vec3_t*)(svector + numverts*numposes);
#endif

		//load in that per-pose data
		invert = (md3XyzNormal_t *)((qbyte*)surf + LittleLong(surf->ofsXyzNormals));
		for (i = 0; i < numposes; i++)
		{
			for (j = 0; j < numverts; j++)
			{
#ifndef SERVERONLY
				lat = (float)invert[j].latlong[0] * (2 * M_PI)*(1.0 / 255.0);
				lng = (float)invert[j].latlong[1] * (2 * M_PI)*(1.0 / 255.0);
				normals[j][0] = cos ( lng ) * sin ( lat );
				normals[j][1] = sin ( lng ) * sin ( lat );
				normals[j][2] = cos ( lat );
#endif
				for (d = 0; d < 3; d++)
				{
					verts[j][d] = LittleShort(invert[j].xyz[d])/64.0f;
					if (verts[j][d]<min[d])
						min[d] = verts[j][d];
					if (verts[j][d]>max[d])
						max[d] = verts[j][d];
				}
			}

			pose->scale[0] = 1;
			pose->scale[1] = 1;
			pose->scale[2] = 1;

			pose->scale_origin[0] = 0;
			pose->scale_origin[1] = 0;
			pose->scale_origin[2] = 0;

			invert += LittleLong(surf->numVerts);

			pose->ofsverts = verts;
			verts += numverts;
#ifndef SERVERONLY
			pose->ofsnormals = normals;
			normals += numverts;
			pose->ofssvector = svector;
			svector += numverts;
			pose->ofstvector = tvector;
			tvector += numverts;
#endif
			pose++;
		}
		pose -= numposes;

		if (framegroups)
		{	//group the poses into animations.
			for (i = 0; i < numgroups; i++)
			{
				Q_snprintfz(group->name, sizeof(group->name), "%s", framegroups[i].name);

				if (framegroups[i].posesarray)
				{
					unsigned int p, targpose;
					group->poseofs = ZG_Malloc(&mod->memgroup, sizeof(*group->poseofs) * framegroups[i].posecount);
					group->numposes = 0;
					for (p = 0; p < framegroups[i].posecount; p++)
					{
						targpose = framegroups[i].poses[p];
						if (targpose < numposes)
							group->poseofs[group->numposes++] = pose[targpose];
					}
				}
				else
				{
					int first = framegroups[i].firstpose, count = framegroups[i].posecount;
					if (first >= numposes)	//bound the numbers.
						first = numposes-1;
					if (first < 0)
						first = 0;
					if (count > numposes-first)
						count = numposes-first;
					if (count < 0)
						count = 0;
					group->poseofs = pose + first;
					group->numposes = count;
				}

				group->rate = framegroups[i].fps;
				group->loop = framegroups[i].loop;
				group->events = framegroups[i].events;
				group->action = framegroups[i].action;
				group->actionweight = framegroups[i].actionweight;
				group++;
			}
		}
		else
		{	//raw poses, no animations.
			for (i = 0; i < numgroups; i++)
			{
				Q_snprintfz(group->name, sizeof(group->name), "frame%i", i);
				group->numposes = 1;
				group->rate = 1;
				group->poseofs = pose + i;
				group->loop = false;
				group->events = NULL;
				group->action = -1;
				group->actionweight = 0;
				group++;
			}
		}

#ifndef SERVERONLY
		if (externalskins<LittleLong(surf->numShaders))
			externalskins = LittleLong(surf->numShaders);
		if (externalskins)
		{
			skin = ZG_Malloc(&mod->memgroup, (externalskins)*((sizeof(galiasskin_t)+sizeof(skinframe_t))));
			galias->ofsskins = skin;
			frames = (skinframe_t *)(skin + externalskins);
			inshader = (md3Shader_t *)((qbyte *)surf + LittleLong(surf->ofsShaders));
			for (i = 0; i < externalskins; i++)
			{
				skin->numframes = 1;
				skin->frame = &frames[i];
				skin->skinwidth = 0;
				skin->skinheight = 0;
				skin->skinspeed = 0;

				if (i >= LittleLong(surf->numShaders))
					Q_strncpyz(frames->shadername, "", sizeof(frames->shadername));	//this shouldn't be possible
				else
				{
					Q_strncpyz(frames->shadername, inshader->name, sizeof(frames->shadername));
					Q_strncpyz(skin->name, inshader->name, sizeof(frames->shadername));
				}

				inshader++;
				skin++;
			}
			galias->numskins = i;
		}
#endif

		Mod_CompileTriangleNeighbours (mod, galias);
		Mod_BuildTextureVectors(galias);

		//if the surfacename is eg _1 then strip that off.
		//this works around q3data requiring different surface names in higher lod levels (and matches q3).
		size = strlen(galias->surfacename);
		if (size>2 && galias->surfacename[size-2] == '_')
			galias->surfacename[size-2] = 0;

		surf = (md3Surface_t *)((qbyte *)surf + LittleLong(surf->ofsEnd));
	}

	AddPointToBounds(min, mod->mins, mod->maxs);
	AddPointToBounds(max, mod->mins, mod->maxs);
	free(framegroups);
	return first;
}
static qboolean QDECL Mod_LoadQ3Model(model_t *mod, void *buffer, size_t fsize)
{
//	int version;

	galiasinfo_t	*root;
	md3Header_t		*header;

	int lod;
	int surfcount = 0;

	header = buffer;

//	if (header->version != sdfs)
//		Sys_Error("GL_LoadQ3Model: Bad version\n");

	root = NULL;

	ClearBounds(mod->mins, mod->maxs);

	mod->meshinfo = Mod_LoadQ3ModelLod(mod, &surfcount, buffer, fsize);
	if (mod->meshinfo)
	{
		const char *ext = COM_GetFileExtension(mod->name, NULL);
		if (*ext == '.' && ext-mod->name < MAX_QPATH)
		{
			galiasinfo_t *sublod;
			sublod = mod->meshinfo;
			for(lod = 1; lod > 0; lod++)
			{
				size_t s;
				char *f;
				char basename[MAX_QPATH];
				char lodname[MAX_QPATH];
				memcpy(basename, mod->name, ext-mod->name);
				basename[ext-mod->name] = 0;
				Q_snprintfz(lodname, sizeof(lodname), "%s_%i%s", basename, lod, ext);
				f = FS_LoadMallocFile(lodname, &s);
				if (!f)
					break;
				root = sublod;
				sublod = Mod_LoadQ3ModelLod(mod, &surfcount, f, s);
				if (sublod)
				{
					for(;;)
					{
						root->maxdist = lod;
						if (!root->nextsurf)
							break;
						root = root->nextsurf;
					}
					root->nextsurf = sublod;
					for (root = sublod; root; root = root->nextsurf)
						root->mindist = lod;
					mod->maxlod = lod+1;
				}
				else
					lod = -1;
				FS_FreeFile(f);
			}
		}
	}

	root = mod->meshinfo;
	if (!root)
	{
		root = ZG_Malloc(&mod->memgroup, sizeof(galiasinfo_t));
		Mod_DefaultMesh(root, mod->name, 0);
	}

	root->numtagframes = LittleLong(header->numFrames);
	root->numtags = LittleLong(header->numTags);
	root->ofstags = ZG_Malloc(&mod->memgroup, LittleLong(header->numTags)*sizeof(md3tag_t)*LittleLong(header->numFrames));

	{
		md3tag_t *src;
		md3tag_t *dst;
		int s, i, j;

		src = (md3tag_t *)((char*)header+LittleLong(header->ofsTags));
		dst = root->ofstags;
		for(i=0;i<LittleLong(header->numTags)*LittleLong(header->numFrames);i++)
		{
			memcpy(dst->name, src->name, sizeof(dst->name));
			for(j=0;j<3;j++)
			{
				dst->org[j] = LittleFloat(src->org[j]);
			}

			for(j=0;j<3;j++)
			{
				for(s=0;s<3;s++)
				{
					dst->ang[j][s] = LittleFloat(src->ang[j][s]);
				}
			}

			src++;
			dst++;
		}
	}

	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);

#ifndef SERVERONLY
	if (mod_md3flags.value)
		mod->flags = LittleLong(header->flags);
	else
#endif
		mod->flags = 0;
	if (!mod->flags)
		mod->flags = Mod_ReadFlagsFromMD1(mod->name, 0);

	Mod_ClampModelSize(mod);
	Mod_ParseModelEvents(mod, root->ofsanimations, root->numanimations);

	mod->type = mod_alias;
	mod->numframes = root->numanimations;
	mod->meshinfo = root;

	mod->funcs.NativeTrace = Mod_Trace;

	return true;
}
#endif










#ifdef ZYMOTICMODELS


typedef struct zymlump_s
{
	int start;
	int length;
} zymlump_t;

typedef struct zymtype1header_s
{
	char id[12]; // "ZYMOTICMODEL", length 12, no termination
	int type; // 0 (vertex morph) 1 (skeletal pose) or 2 (skeletal scripted)
	int filesize; // size of entire model file
	float mins[3], maxs[3], radius; // for clipping uses
	int numverts;
	int numtris;
	int numsurfaces;
	int numbones; // this may be zero in the vertex morph format (undecided)
	int numscenes; // 0 in skeletal scripted models

// skeletal pose header
	// lump offsets are relative to the file
	zymlump_t lump_scenes; // zymscene_t scene[numscenes]; // name and other information for each scene (see zymscene struct)
	zymlump_t lump_poses; // float pose[numposes][numbones][6]; // animation data
	zymlump_t lump_bones; // zymbone_t bone[numbones];
	zymlump_t lump_vertbonecounts; // int vertbonecounts[numvertices]; // how many bones influence each vertex (separate mainly to make this compress better)
	zymlump_t lump_verts; // zymvertex_t vert[numvertices]; // see vertex struct
	zymlump_t lump_texcoords; // float texcoords[numvertices][2];
	zymlump_t lump_render; // int renderlist[rendersize]; // sorted by shader with run lengths (int count), shaders are sequentially used, each run can be used with glDrawElements (each triangle is 3 int indices)
	zymlump_t lump_surfnames; // char shadername[numsurfaces][32]; // shaders used on this model
	zymlump_t lump_trizone; // byte trizone[numtris]; // see trizone explanation
} zymtype1header_t;

typedef struct zymbone_s
{
	char name[32];
	int flags;
	int parent; // parent bone number
} zymbone_t;

typedef struct zymscene_s
{
	char name[32];
	float mins[3], maxs[3], radius; // for clipping
	float framerate; // the scene will animate at this framerate (in frames per second)
	int flags;
	int start, length; // range of poses
} zymscene_t;
#define ZYMSCENEFLAG_NOLOOP 1

typedef struct zymvertex_s
{
	int bonenum;
	float origin[3];
} zymvertex_t;

//this can generate multiple meshes (one for each shader).
//but only one set of transforms are ever generated.
static qboolean QDECL Mod_LoadZymoticModel(model_t *mod, void *buffer, size_t fsize)
{
#ifndef SERVERONLY
	galiasskin_t *skin;
	skinframe_t *skinframe;
	int skinfiles;
	int j;
#endif

	int i;

	zymtype1header_t *header;
	galiasinfo_t *root;

	size_t numtransforms;
	galisskeletaltransforms_t *transforms;
	zymvertex_t	*intrans;

	galiasbone_t *bone;
	zymbone_t *inbone;
	int v;
	float multiplier;
	float *matrix, *inmatrix;

	vec2_t *stcoords;
	vec2_t *inst;

	int *vertbonecounts;

	galiasanimation_t *grp;
	zymscene_t *inscene;

	int *renderlist, count;
	index_t *indexes;

	char *surfname;

	header = buffer;

	if (memcmp(header->id, "ZYMOTICMODEL", 12))
	{
		Con_Printf("Mod_LoadZymoticModel: %s, doesn't appear to BE a zymotic!\n", mod->name);
		return false;
	}

	if (BigLong(header->type) != 1)
	{
		Con_Printf("Mod_LoadZymoticModel: %s, only type 1 is supported\n", mod->name);
		return false;
	}

	for (i = 0; i < sizeof(zymtype1header_t)/4; i++)
		((int*)header)[i] = BigLong(((int*)header)[i]);

	if (!header->numverts)
	{
		Con_Printf("Mod_LoadZymoticModel: %s, no vertexes\n", mod->name);
		return false;
	}

	if (!header->numsurfaces)
	{
		Con_Printf("Mod_LoadZymoticModel: %s, no surfaces\n", mod->name);
		return false;
	}

	VectorCopy(header->mins, mod->mins);
	VectorCopy(header->maxs, mod->maxs);

	root = ZG_Malloc(&mod->memgroup, sizeof(galiasinfo_t)*header->numsurfaces);

	numtransforms = header->lump_verts.length/sizeof(zymvertex_t);
	transforms = Z_Malloc(numtransforms*sizeof(*transforms));

	vertbonecounts = (int *)((char*)header + header->lump_vertbonecounts.start);
	intrans = (zymvertex_t *)((char*)header + header->lump_verts.start);

	vertbonecounts[0] = BigLong(vertbonecounts[0]);
	multiplier = 1.0f / vertbonecounts[0];
	for (i = 0, v=0; i < numtransforms; i++)
	{
		while(!vertbonecounts[v])
		{
			v++;
			if (v == header->numverts)
			{
				Con_Printf("Mod_LoadZymoticModel: %s, too many transformations\n", mod->name);
				return false;
			}
			vertbonecounts[v] = BigLong(vertbonecounts[v]);
			multiplier = 1.0f / vertbonecounts[v];
		}
		transforms[i].vertexindex = v;
		transforms[i].boneindex = BigLong(intrans[i].bonenum);
		transforms[i].org[0] = multiplier*BigFloat(intrans[i].origin[0]);
		transforms[i].org[1] = multiplier*BigFloat(intrans[i].origin[1]);
		transforms[i].org[2] = multiplier*BigFloat(intrans[i].origin[2]);
		transforms[i].org[3] = multiplier*1;
		vertbonecounts[v]--;
	}
	if (intrans != (zymvertex_t *)((char*)header + header->lump_verts.start))
	{
		Con_Printf(CON_ERROR "%s, Vertex transforms list appears corrupt.\n", mod->name);
		return false;
	}
	if (vertbonecounts != (int *)((char*)header + header->lump_vertbonecounts.start))
	{
		Con_Printf(CON_ERROR "%s, Vertex bone counts list appears corrupt.\n", mod->name);
		return false;
	}

	root->numverts = v+1;

	root->numbones = header->numbones;
	bone = ZG_Malloc(&mod->memgroup, numtransforms*sizeof(*transforms));
	inbone = (zymbone_t*)((char*)header + header->lump_bones.start);
	for (i = 0; i < root->numbones; i++)
	{
		Q_strncpyz(bone[i].name, inbone[i].name, sizeof(bone[i].name));
		bone[i].parent = BigLong(inbone[i].parent);
	}
	root->ofsbones = bone;

	renderlist = (int*)((char*)header + header->lump_render.start);
	for (i = 0;i < header->numsurfaces; i++)
	{
		count = BigLong(*renderlist++);
		count *= 3;
		indexes = ZG_Malloc(&mod->memgroup, count*sizeof(*indexes));
		root[i].ofs_indexes = indexes;
		root[i].numindexes = count;
		while(count)
		{	//invert
			indexes[count-1] = BigLong(renderlist[count-3]);
			indexes[count-2] = BigLong(renderlist[count-2]);
			indexes[count-3] = BigLong(renderlist[count-1]);
			count-=3;
		}
		renderlist += root[i].numindexes;
	}
	if (renderlist != (int*)((char*)header + header->lump_render.start + header->lump_render.length))
	{
		Con_Printf(CON_ERROR "%s, render list appears corrupt.\n", mod->name);
		return false;
	}

	grp = ZG_Malloc(&mod->memgroup, sizeof(*grp)*header->numscenes*header->numsurfaces);
	matrix = ZG_Malloc(&mod->memgroup, header->lump_poses.length);
	inmatrix = (float*)((char*)header + header->lump_poses.start);
	for (i = 0; i < header->lump_poses.length/4; i++)
		matrix[i] = BigFloat(inmatrix[i]);
	inscene = (zymscene_t*)((char*)header + header->lump_scenes.start);
	surfname = ((char*)header + header->lump_surfnames.start);

	stcoords = ZG_Malloc(&mod->memgroup, root[0].numverts*sizeof(vec2_t));
	inst = (vec2_t *)((char *)header + header->lump_texcoords.start);
	for (i = 0; i < header->lump_texcoords.length/8; i++)
	{
		stcoords[i][0] = BigFloat(inst[i][0]);
		stcoords[i][1] = 1-BigFloat(inst[i][1]);	//hmm. upside down skin coords?
	}

#ifndef SERVERONLY
	skinfiles = Mod_CountSkinFiles(mod);
	if (skinfiles < 1)
		skinfiles = 1;

	skin = ZG_Malloc(&mod->memgroup, (sizeof(galiasskin_t)+sizeof(skinframe_t))*skinfiles*header->numsurfaces);
	skinframe = (skinframe_t*)(skin+skinfiles*header->numsurfaces);
#endif

	for (i = 0; i < header->numsurfaces; i++, surfname+=32)
	{
		Mod_DefaultMesh(&root[i], surfname, i);
		root[i].numanimations = header->numscenes;
		root[i].ofsanimations = grp;

#ifdef SERVERONLY
		root[i].numskins = 1;
#else
		root[i].ofs_st_array = stcoords;
		root[i].numskins = skinfiles;
		root[i].ofsskins = skin;

		for (j = 0; j < skinfiles; j++)
		{
			skin->skinwidth = 1;
			skin->skinheight = 1;
			skin->skinspeed = 10; /*something to avoid div-by-0*/
			skin->numframes = 1;	//non-sequenced skins.
			skin->frame = skinframe;
			skin++;

			Q_strncpyz(skinframe->shadername, surfname, sizeof(skinframe->shadername));
			skinframe++;
		}
#endif
	}


	for (i = 0; i < header->numscenes; i++, grp++, inscene++)
	{
		Q_strncpyz(grp->name, inscene->name, sizeof(grp->name));

		grp->skeltype = SKEL_RELATIVE;
		grp->rate = BigFloat(inscene->framerate);
		grp->loop = !(BigLong(inscene->flags) & ZYMSCENEFLAG_NOLOOP);
		grp->numposes = BigLong(inscene->length);
		grp->boneofs = matrix + BigLong(inscene->start)*12*root->numbones;
		grp->action = -1;
		grp->actionweight = 0;
	}

	if (inscene != (zymscene_t*)((char*)header + header->lump_scenes.start+header->lump_scenes.length))
	{
		Con_Printf(CON_ERROR "%s, scene list appears corrupt.\n", mod->name);
		return false;
	}

	Alias_BuildGPUWeights(mod, root, numtransforms, transforms, true);

	for (i = 0; i < header->numsurfaces-1; i++)
		root[i].nextsurf = &root[i+1];
	for (i = 1; i < header->numsurfaces; i++)
	{
		root[i].ofs_skel_xyz = root[0].ofs_skel_xyz;
		root[i].ofs_skel_norm = root[0].ofs_skel_norm;
		root[i].ofs_skel_svect = root[0].ofs_skel_svect;
		root[i].ofs_skel_tvect = root[0].ofs_skel_tvect;
		root[i].ofs_skel_idx = root[0].ofs_skel_idx;
		root[i].ofs_skel_weight = root[0].ofs_skel_weight;

		root[i].shares_verts = 0;
		root[i].numbones = root[0].numbones;
		root[i].numverts = root[0].numverts;

		root[i].ofsbones = root[0].ofsbones;
	}

	Z_Free(transforms);

	mod->flags = Mod_ReadFlagsFromMD1(mod->name, 0);	//file replacement - inherit flags from any defunc mdl files.

	Mod_ClampModelSize(mod);
	Mod_ParseModelEvents(mod, root->ofsanimations, root->numanimations);


	mod->meshinfo = root;
	mod->numframes = root->numanimations;
	mod->type = mod_alias;

	mod->funcs.NativeTrace = Mod_Trace;

	return true;
}
#endif //ZYMOTICMODELS


///////////////////////////////////////////////////////////////
//psk
#ifdef PSKMODELS
/*Typedefs copied from DarkPlaces*/
extern cvar_t dpcompat_psa_ungroup;

typedef struct pskchunk_s
{
	// id is one of the following:
	// .psk:
	// ACTRHEAD (recordsize = 0, numrecords = 0)
	// PNTS0000 (recordsize = 12, pskpnts_t)
	// VTXW0000 (recordsize = 16, pskvtxw_t)
	// FACE0000 (recordsize = 12, pskface_t)
	// MATT0000 (recordsize = 88, pskmatt_t)
	// REFSKELT (recordsize = 120, pskboneinfo_t)
	// RAWWEIGHTS (recordsize = 12, pskrawweights_t)
	// .psa:
	// ANIMHEAD (recordsize = 0, numrecords = 0)
	// BONENAMES (recordsize = 120, pskboneinfo_t)
	// ANIMINFO (recordsize = 168, pskaniminfo_t)
	// ANIMKEYS (recordsize = 32, pskanimkeys_t)
	char id[20];
	// in .psk always 0x1e83b9
	// in .psa always 0x2e
	int version;
	int recordsize;
	int numrecords;
} pskchunk_t;

typedef struct pskpnts_s
{
	float origin[3];
} pskpnts_t;

typedef struct pskvtxw_s
{
	unsigned short pntsindex; // index into PNTS0000 chunk
	unsigned char unknown1[2]; // seems to be garbage
	float texcoord[2];
	unsigned char mattindex; // index into MATT0000 chunk
	unsigned char unknown2; // always 0?
	unsigned char unknown3[2]; // seems to be garbage
} pskvtxw_t;

typedef struct pskface_s
{
	unsigned short vtxwindex[3]; // triangle
	unsigned char mattindex; // index into MATT0000 chunk
	unsigned char unknown; // seems to be garbage
	unsigned int group; // faces seem to be grouped, possibly for smoothing?
} pskface_t;

typedef struct pskmatt_s
{
	char name[64];
	int unknown[6]; // observed 0 0 0 0 5 0
} pskmatt_t;

typedef struct pskpose_s
{
	float quat[4];
	float origin[3];
	float unknown; // probably a float, always seems to be 0
	float size[3];
} pskpose_t;

typedef struct pskboneinfo_s
{
	char name[64];
	int unknown1;
	int numchildren;
	int parent; // root bones have 0 here
	pskpose_t basepose;
} pskboneinfo_t;

typedef struct pskrawweights_s
{
	float weight;
	int pntsindex;
	int boneindex;
} pskrawweights_t;

typedef struct pskaniminfo_s
{
	char name[64];
	char group[64];
	int numbones;
	int unknown1;
	int unknown2;
	int unknown3;
	float unknown4;
	float playtime; // not really needed
	float fps; // frames per second
	int unknown5;
	int firstframe;
	int numframes;
	// firstanimkeys = (firstframe + frameindex) * numbones
} pskaniminfo_t;

typedef struct pskanimkeys_s
{
	float origin[3];
	float quat[4];
	float frametime;
} pskanimkeys_t;


static qboolean QDECL Mod_LoadPSKModel(model_t *mod, void *buffer, size_t fsize)
{
	pskchunk_t *chunk;
	unsigned int pos = 0;
	unsigned int i, j;
	qboolean fail = false;
	char basename[MAX_QPATH];
	char psaname[MAX_QPATH];

	galiasinfo_t *gmdl;
#ifndef SERVERONLY
	vec2_t *stcoord;
	galiasskin_t *skin;
	skinframe_t *sframes;
#endif
	galiasbone_t *bones;
	galiasanimation_t *group;
	float *animmatrix, *basematrix;
	index_t *indexes;
	int bonemap[MAX_BONES];
	char *e;
	size_t psasize;
	void *psabuffer;

	pskpnts_t *pnts = NULL;
	pskvtxw_t *vtxw = NULL;
	pskface_t *face = NULL;
	pskmatt_t *matt = NULL;
	pskboneinfo_t *boneinfo = NULL;
	pskrawweights_t *rawweights = NULL;
	unsigned int num_pnts, num_vtxw=0, num_face=0, num_matt = 0, num_boneinfo=0, num_rawweights=0;

	pskaniminfo_t *animinfo = NULL;
	pskanimkeys_t *animkeys = NULL;
	unsigned int num_animinfo=0, num_animkeys=0;

	vecV_t *skel_xyz;
	vec3_t *skel_norm, *skel_svect, *skel_tvect;
	bone_vec4_t *skel_idx;
	vec4_t *skel_weights;

	/*load the psk*/
	while (pos < fsize && !fail)
	{
		chunk = (pskchunk_t*)((char*)buffer + pos);
		chunk->version = LittleLong(chunk->version);
		chunk->recordsize = LittleLong(chunk->recordsize);
		chunk->numrecords = LittleLong(chunk->numrecords);

		pos += sizeof(*chunk);

		if (!strcmp("ACTRHEAD", chunk->id) && chunk->recordsize == 0 && chunk->numrecords == 0)
		{
		}
		else if (!strcmp("PNTS0000", chunk->id) && chunk->recordsize == sizeof(pskpnts_t))
		{
			num_pnts = chunk->numrecords;
			pnts = (pskpnts_t*)((char*)buffer + pos);
			pos += chunk->recordsize * chunk->numrecords;

			for (i = 0; i < num_pnts; i++)
			{
				pnts[i].origin[0] = LittleFloat(pnts[i].origin[0]);
				pnts[i].origin[1] = LittleFloat(pnts[i].origin[1]);
				pnts[i].origin[2] = LittleFloat(pnts[i].origin[2]);
			}
		}
		else if (!strcmp("VTXW0000", chunk->id) && chunk->recordsize == sizeof(pskvtxw_t))
		{
			num_vtxw = chunk->numrecords;
			vtxw = (pskvtxw_t*)((char*)buffer + pos);
			pos += chunk->recordsize * chunk->numrecords;

			for (i = 0; i < num_vtxw; i++)
			{
				vtxw[i].pntsindex = LittleShort(vtxw[i].pntsindex);
				vtxw[i].texcoord[0] = LittleFloat(vtxw[i].texcoord[0]);
				vtxw[i].texcoord[1] = LittleFloat(vtxw[i].texcoord[1]);
			}
		}
		else if (!strcmp("FACE0000", chunk->id) && chunk->recordsize == sizeof(pskface_t))
		{
			num_face = chunk->numrecords;
			face = (pskface_t*)((char*)buffer + pos);
			pos += chunk->recordsize * chunk->numrecords;

			for (i = 0; i < num_face; i++)
			{
				face[i].vtxwindex[0] = LittleShort(face[i].vtxwindex[0]);
				face[i].vtxwindex[1] = LittleShort(face[i].vtxwindex[1]);
				face[i].vtxwindex[2] = LittleShort(face[i].vtxwindex[2]);
			}
		}
		else if (!strcmp("MATT0000", chunk->id) && chunk->recordsize == sizeof(pskmatt_t))
		{
			num_matt = chunk->numrecords;
			matt = (pskmatt_t*)((char*)buffer + pos);
			pos += chunk->recordsize * chunk->numrecords;
		}
		else if (!strcmp("REFSKELT", chunk->id) && chunk->recordsize == sizeof(pskboneinfo_t))
		{
			num_boneinfo = chunk->numrecords;
			boneinfo = (pskboneinfo_t*)((char*)buffer + pos);
			pos += chunk->recordsize * chunk->numrecords;

			for (i = 0; i < num_boneinfo; i++)
			{
				boneinfo[i].parent = LittleLong(boneinfo[i].parent);
				boneinfo[i].basepose.origin[0] = LittleFloat(boneinfo[i].basepose.origin[0]);
				boneinfo[i].basepose.origin[1] = LittleFloat(boneinfo[i].basepose.origin[1]);
				boneinfo[i].basepose.origin[2] = LittleFloat(boneinfo[i].basepose.origin[2]);
				boneinfo[i].basepose.quat[0] = LittleFloat(boneinfo[i].basepose.quat[0]);
				boneinfo[i].basepose.quat[1] = LittleFloat(boneinfo[i].basepose.quat[1]);
				boneinfo[i].basepose.quat[2] = LittleFloat(boneinfo[i].basepose.quat[2]);
				boneinfo[i].basepose.quat[3] = LittleFloat(boneinfo[i].basepose.quat[3]);
				boneinfo[i].basepose.size[0] = LittleFloat(boneinfo[i].basepose.size[0]);
				boneinfo[i].basepose.size[1] = LittleFloat(boneinfo[i].basepose.size[1]);
				boneinfo[i].basepose.size[2] = LittleFloat(boneinfo[i].basepose.size[2]);

				/*not sure if this is needed, but mimic DP*/
				if (i)
				{
					boneinfo[i].basepose.quat[0] *= -1;
					boneinfo[i].basepose.quat[2] *= -1;
				}
				boneinfo[i].basepose.quat[1] *= -1;
			}
		}
		else if (!strcmp("RAWWEIGHTS", chunk->id) && chunk->recordsize == sizeof(pskrawweights_t))
		{
			num_rawweights = chunk->numrecords;
			rawweights = (pskrawweights_t*)((char*)buffer + pos);
			pos += chunk->recordsize * chunk->numrecords;

			for (i = 0; i < num_rawweights; i++)
			{
				rawweights[i].boneindex = LittleLong(rawweights[i].boneindex);
				rawweights[i].pntsindex = LittleLong(rawweights[i].pntsindex);
				rawweights[i].weight = LittleFloat(rawweights[i].weight);
			}
		}
		else
		{
			Con_Printf(CON_ERROR "%s has unsupported chunk %s of %i size with version %i.\n", mod->name, chunk->id, chunk->recordsize, chunk->version);
			fail = true;
		}
	}

	if (!num_matt)
		fail = true;

	if (!pnts || !vtxw || !face || !matt || !boneinfo || !rawweights)
		fail = true;

	/*attempt to load a psa file. don't die if we can't find one*/
	COM_StripExtension(mod->name, psaname, sizeof(psaname));
	Q_strncatz(psaname, ".psa", sizeof(psaname));
	buffer = NULL;//test
	psabuffer = FS_LoadMallocFile(psaname, &psasize);
	if (psabuffer)
	{
		pos = 0;
		while (pos < psasize && !fail)
		{
			chunk = (pskchunk_t*)((char*)psabuffer + pos);
			chunk->version = LittleLong(chunk->version);
			chunk->recordsize = LittleLong(chunk->recordsize);
			chunk->numrecords = LittleLong(chunk->numrecords);

			pos += sizeof(*chunk);

			if (!strcmp("ANIMHEAD", chunk->id) && chunk->recordsize == 0 && chunk->numrecords == 0)
			{
			}
			else if (!strcmp("BONENAMES", chunk->id) && chunk->recordsize == sizeof(pskboneinfo_t))
			{
				/*parsed purely to ensure that the bones match the main model*/
				pskboneinfo_t *animbones = (pskboneinfo_t*)((char*)psabuffer + pos);
				pos += chunk->recordsize * chunk->numrecords;
				if (num_boneinfo != chunk->numrecords)
				{
					fail = true;
					Con_Printf("PSK/PSA bone counts do not match\n");
				}
				else
				{
					for (i = 0; i < num_boneinfo; i++)
					{
						/*assumption: 1:1 mapping will be common*/
						if (!strcmp(boneinfo[i].name, animbones[i].name))
							bonemap[i] = i;
						else
						{
							/*non 1:1 mapping*/
							for (j = 0; j < chunk->numrecords; j++)
							{
								if (!strcmp(boneinfo[i].name, animbones[j].name))
								{
									bonemap[i] = j;
									break;
								}
							}
							if (j == chunk->numrecords)
							{
								fail = true;
								Con_Printf("PSK bone %s does not exist in PSA %s\n", boneinfo[i].name, basename);
								break;
							}
						}
					}
				}
			}
			else if (!strcmp("ANIMINFO", chunk->id) && chunk->recordsize == sizeof(pskaniminfo_t))
			{
				num_animinfo = chunk->numrecords;
				animinfo = (pskaniminfo_t*)((char*)psabuffer + pos);
				pos += chunk->recordsize * chunk->numrecords;

				for (i = 0; i < num_animinfo; i++)
				{
					animinfo[i].firstframe = LittleLong(animinfo[i].firstframe);
					animinfo[i].numframes = LittleLong(animinfo[i].numframes);
					animinfo[i].numbones = LittleLong(animinfo[i].numbones);
					animinfo[i].fps = LittleFloat(animinfo[i].fps);
					animinfo[i].playtime = LittleFloat(animinfo[i].playtime);
				}
			}
			else if (!strcmp("ANIMKEYS", chunk->id) && chunk->recordsize == sizeof(pskanimkeys_t))
			{
				num_animkeys = chunk->numrecords;
				animkeys = (pskanimkeys_t*)((char*)psabuffer + pos);
				pos += chunk->recordsize * chunk->numrecords;

				for (i = 0; i < num_animkeys; i++)
				{
					animkeys[i].origin[0] = LittleFloat(animkeys[i].origin[0]);
					animkeys[i].origin[1] = LittleFloat(animkeys[i].origin[1]);
					animkeys[i].origin[2] = LittleFloat(animkeys[i].origin[2]);
					animkeys[i].quat[0] = LittleFloat(animkeys[i].quat[0]);
					animkeys[i].quat[1] = LittleFloat(animkeys[i].quat[1]);
					animkeys[i].quat[2] = LittleFloat(animkeys[i].quat[2]);
					animkeys[i].quat[3] = LittleFloat(animkeys[i].quat[3]);

					/*not sure if this is needed, but mimic DP*/
					if (i%num_boneinfo)
					{
						animkeys[i].quat[0] *= -1;
						animkeys[i].quat[2] *= -1;
					}
					animkeys[i].quat[1] *= -1;
				}
			}
			else if (!strcmp("SCALEKEYS", chunk->id) && chunk->recordsize == 16)
			{
				pos += chunk->recordsize * chunk->numrecords;
			}
			else
			{
				Con_Printf(CON_ERROR "%s has unsupported chunk %s of %i size with version %i.\n", va("%s.psa", basename), chunk->id, chunk->recordsize, chunk->version);
				fail = true;
			}
		}
		if (fail)
		{
			animinfo = NULL;
			num_animinfo = 0;
			animkeys = NULL;
			num_animkeys = 0;
			fail = false;
		}
	}

	if (fail)
	{
		BZ_Free(psabuffer);
		return false;
	}

	gmdl = ZG_Malloc(&mod->memgroup, sizeof(*gmdl)*num_matt);

	/*bones!*/
	bones = ZG_Malloc(&mod->memgroup, sizeof(galiasbone_t) * num_boneinfo);
	for (i = 0; i < num_boneinfo; i++)
	{
		Q_strncpyz(bones[i].name, boneinfo[i].name, sizeof(bones[i].name));
		e = bones[i].name + strlen(bones[i].name);
		while(e > bones[i].name && e[-1] == ' ')
			*--e = 0;
		bones[i].parent = boneinfo[i].parent;
		if (i == 0 && bones[i].parent == 0)
			bones[i].parent = -1;
		else if (bones[i].parent >= i || bones[i].parent < -1)
		{
			Con_Printf("Invalid bones\n");
			break;
		}
	}

	basematrix = ZG_Malloc(&mod->memgroup, num_boneinfo*sizeof(float)*12);
	for (i = 0; i < num_boneinfo; i++)
	{
		float tmp[12];
		PSKGenMatrix(
			boneinfo[i].basepose.origin[0], boneinfo[i].basepose.origin[1], boneinfo[i].basepose.origin[2],
			boneinfo[i].basepose.quat[0],   boneinfo[i].basepose.quat[1],   boneinfo[i].basepose.quat[2], boneinfo[i].basepose.quat[3],
			tmp);
		if (bones[i].parent < 0)
			memcpy(basematrix + i*12, tmp, sizeof(float)*12);
		else
			R_ConcatTransforms((void*)(basematrix + bones[i].parent*12), (void*)tmp, (void*)(basematrix+i*12));
	}

	for (i = 0; i < num_boneinfo; i++)
	{
		Matrix3x4_Invert_Simple(basematrix+i*12, bones[i].inverse);
	}

	skel_xyz = ZG_Malloc(&mod->memgroup, sizeof(*skel_xyz) * num_vtxw);
	skel_norm = ZG_Malloc(&mod->memgroup, sizeof(*skel_norm) * num_vtxw);
	skel_svect = ZG_Malloc(&mod->memgroup, sizeof(*skel_svect) * num_vtxw);
	skel_tvect = ZG_Malloc(&mod->memgroup, sizeof(*skel_tvect) * num_vtxw);
	skel_idx = ZG_Malloc(&mod->memgroup, sizeof(*skel_idx) * num_vtxw);
	skel_weights = ZG_Malloc(&mod->memgroup, sizeof(*skel_weights) * num_vtxw);
	for (j = 0; j < 4; j++)
		skel_idx[i][j] = ~0;
	for (i = 0; i < num_vtxw; i++)
	{
		float t;
		for (j = 0; j < num_rawweights; j++)
		{
			if (rawweights[j].pntsindex == vtxw[i].pntsindex)
			{
				int in, lin = -1;
				float liv = rawweights[j].weight;
				for (in = 0; in < 4; in++)
				{
					if (liv > skel_weights[i][in])
					{
						liv = skel_weights[i][in];
						lin = in;
						if (!liv)
							break;
					}
				}
				if (lin >= 0)
				{
					skel_idx[i][lin] = rawweights[j].boneindex;
					skel_weights[i][lin] = rawweights[j].weight;
				}
			}
		}
		t = 0;
		for (j = 0; j < 4; j++)
			t += skel_weights[i][j];
		if (t != 1)
			for (j = 0; j < 4; j++)
				skel_weights[i][j] *= 1/t;

		skel_xyz[i][0] = pnts[vtxw[i].pntsindex].origin[0];
		skel_xyz[i][1] = pnts[vtxw[i].pntsindex].origin[1];
		skel_xyz[i][2] = pnts[vtxw[i].pntsindex].origin[2];
	}

#ifndef SERVERONLY
	/*st coords, all share the same list*/
	stcoord = ZG_Malloc(&mod->memgroup, sizeof(vec2_t)*num_vtxw);
	for (i = 0; i < num_vtxw; i++)
	{
		stcoord[i][0] = vtxw[i].texcoord[0];
		stcoord[i][1] = vtxw[i].texcoord[1];
	}
#endif

	/*allocate faces in a single block, as we at least know an upper bound*/
	indexes = ZG_Malloc(&mod->memgroup, sizeof(index_t)*num_face*3);

	if (animinfo && animkeys)
	{
		int numgroups = 0;
		frameinfo_t *frameinfo = ParseFrameInfo(mod, &numgroups);
		if (numgroups)
		{
			/*externally supplied listing of frames. ignore all framegroups in the model and use only the pose info*/
			group = ZG_Malloc(&mod->memgroup, sizeof(galiasanimation_t)*numgroups + num_animkeys*sizeof(float)*12);
			animmatrix = (float*)(group+numgroups);
			for (j = 0; j < numgroups; j++)
			{
				/*bound check*/
				if (frameinfo[j].posesarray)
					Con_Printf(CON_WARNING"Mod_LoadPSKModel(%s): framegroup[%i] poses array not suppported\n", mod->name, j);
				if (frameinfo[j].firstpose+frameinfo[j].posecount > num_animkeys)
					frameinfo[j].posecount = num_animkeys - frameinfo[j].firstpose;
				if (frameinfo[j].firstpose >= num_animkeys)
				{
					frameinfo[j].firstpose = 0;
					frameinfo[j].posecount = 1;
				}

				group[j].boneofs = animmatrix + 12*num_boneinfo*frameinfo[j].firstpose;
				group[j].numposes = frameinfo[j].posecount;
				if (*frameinfo[j].name)
					Q_snprintfz(group[j].name, sizeof(group[j].name), "%s", frameinfo[j].name);
				else
					Q_snprintfz(group[j].name, sizeof(group[j].name), "frame_%i", j);
				group[j].loop = frameinfo[j].loop;
				group[j].rate = frameinfo[j].fps;
				group[j].skeltype = SKEL_RELATIVE;
				group[j].events = frameinfo[j].events;
				group[j].action = frameinfo[j].action;
				group[j].actionweight = frameinfo[j].actionweight;
			}
			num_animinfo = numgroups;
		}
#ifndef HAVE_LEGACY
		else if (dpcompat_psa_ungroup.ival)
		{
			/*unpack each frame of each animation to be a separate framegroup*/
			unsigned int iframe;	/*individual frame count*/
			iframe = 0;
			for (i = 0; i < num_animinfo; i++)
				iframe += animinfo[i].numframes;
			group = ZG_Malloc(&mod->memgroup, sizeof(galiasanimation_t)*iframe + num_animkeys*sizeof(float)*12);
			animmatrix = (float*)(group+iframe);
			iframe = 0;
			for (j = 0; j < num_animinfo; j++)
			{
				for (i = 0; i < animinfo[j].numframes; i++)
				{
					group[iframe].boneofs = animmatrix + 12*num_boneinfo*(animinfo[j].firstframe+i);
					group[iframe].numposes = 1;
					Q_snprintfz(group[iframe].name, sizeof(group[iframe].name), "%s_%i", animinfo[j].name, i);
					group[iframe].loop = true;
					group[iframe].rate = animinfo[j].fps;
					group[iframe].skeltype = SKEL_RELATIVE;
					group[iframe].action = -1;
					group[iframe].actionweight = 0;
					iframe++;
				}
			}
			num_animinfo = iframe;
		}
#endif
		else
		{
			/*keep each framegroup as a group*/
			group = ZG_Malloc(&mod->memgroup, sizeof(galiasanimation_t)*num_animinfo + num_animkeys*sizeof(float)*12);
			animmatrix = (float*)(group+num_animinfo);
			for (i = 0; i < num_animinfo; i++)
			{
				group[i].boneofs = animmatrix + 12*num_boneinfo*animinfo[i].firstframe;
				group[i].numposes = animinfo[i].numframes;
				Q_strncpyz(group[i].name, animinfo[i].name, sizeof(group[i].name));
				group[i].loop = true;
				group[i].rate = animinfo[i].fps;
				group[i].skeltype = SKEL_RELATIVE;
				group[i].action = -1;
				group[i].actionweight = 0;
			}
		}
		for (j = 0; j < num_animkeys; j += num_boneinfo)
		{
			pskanimkeys_t *sb;
			for (i = 0; i < num_boneinfo; i++)
			{
				sb = &animkeys[j + bonemap[i]];
				PSKGenMatrix(
					sb->origin[0], sb->origin[1], sb->origin[2],
					sb->quat[0],   sb->quat[1],   sb->quat[2],   sb->quat[3],
					animmatrix + (j+i)*12);
			}
		}
		free(frameinfo);
	}
	else
	{
		num_animinfo = 1;
		/*build a base pose*/
		group = ZG_Malloc(&mod->memgroup, sizeof(galiasanimation_t) + num_boneinfo*sizeof(float)*12);
		animmatrix = basematrix;
		group->boneofs = animmatrix;
		group->numposes = 1;
		strcpy(group->name, "base");
		group->loop = true;
		group->rate = 10;
		group->skeltype = SKEL_ABSOLUTE;
		group->action = -1;
		group->actionweight = 0;
	}


#ifndef SERVERONLY
	//client builds need skin info
	skin = ZG_Malloc(&mod->memgroup, num_matt * (sizeof(galiasskin_t) + sizeof(skinframe_t)));
	sframes = (skinframe_t*)(skin + num_matt);
	for (i = 0; i < num_matt; i++, skin++)
	{
		skin->frame = &sframes[i];
		skin->numframes = 1;
		skin->skinspeed = 10;
		Q_strncpyz(skin->name, matt[i].name, sizeof(skin->name));
		Q_strncpyz(sframes[i].shadername, matt[i].name, sizeof(sframes[i].shadername));
		sframes[i].shader = NULL;

		gmdl[i].ofsskins = skin;
		gmdl[i].numskins = 1;

		gmdl[i].ofs_st_array = stcoord;
		gmdl[i].numverts = num_vtxw;
#else
	//server-only builds
	for (i = 0; i < num_matt; i++)
	{
#endif
		//common to all builds
		Mod_DefaultMesh(&gmdl[i], mod->name, i);

		gmdl[i].ofsanimations = group;
		gmdl[i].numanimations = num_animinfo;

		gmdl[i].numindexes = 0;
		for (j = 0; j < num_face; j++)
		{
			if (face[j].mattindex%num_matt == i)
			{
				indexes[gmdl[i].numindexes+0] = face[j].vtxwindex[0];
				indexes[gmdl[i].numindexes+1] = face[j].vtxwindex[1];
				indexes[gmdl[i].numindexes+2] = face[j].vtxwindex[2];
				gmdl[i].numindexes += 3;
			}
		}
		gmdl[i].ofs_indexes = indexes;
		indexes += gmdl[i].numindexes;

		//all surfaces share bones
		gmdl[i].baseframeofs = basematrix;
		gmdl[i].ofsbones = bones;
		gmdl[i].numbones = num_boneinfo;
		gmdl[i].shares_bones = 0;

		//all surfaces share verts, but not indexes.
		gmdl[i].ofs_skel_idx = skel_idx;
		gmdl[i].ofs_skel_weight = skel_weights;
		gmdl[i].ofs_skel_xyz = skel_xyz;
		gmdl[i].ofs_skel_norm = skel_norm;
		gmdl[i].ofs_skel_svect = skel_svect;
		gmdl[i].ofs_skel_tvect = skel_tvect;
		gmdl[i].shares_verts = 0;

		gmdl[i].nextsurf = (i != num_matt-1)?&gmdl[i+1]:NULL;
	}

#ifndef SERVERONLY
	Mod_AccumulateTextureVectors(skel_xyz, stcoord, skel_norm, skel_svect, skel_tvect, gmdl[0].ofs_indexes, indexes-gmdl[0].ofs_indexes, true);
	Mod_NormaliseTextureVectors(skel_norm, skel_svect, skel_tvect, num_vtxw, true);
#endif

	BZ_Free(psabuffer);
	if (fail)
	{
		return false;
	}


//	mod->radius = Alias_CalculateSkeletalNormals(gmdl);

	mod->mins[0] = mod->mins[1] = mod->mins[2] = -mod->radius;
	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = mod->radius;

	mod->flags = Mod_ReadFlagsFromMD1(mod->name, 0);	//file replacement - inherit flags from any defunc mdl files.

	Mod_ClampModelSize(mod);
	Mod_ParseModelEvents(mod, gmdl->ofsanimations, gmdl->numanimations);


	mod->meshinfo = gmdl;
	mod->numframes = gmdl->numanimations;
	mod->type = mod_alias;
	mod->funcs.NativeTrace = Mod_Trace;
	return true;
}

#endif





//////////////////////////////////////////////////////////////
//dpm
#ifdef DPMMODELS

// header for the entire file
typedef struct dpmheader_s
{
	char id[16]; // "DARKPLACESMODEL\0", length 16
	unsigned int type; // 2 (hierarchical skeletal pose)
	unsigned int filesize; // size of entire model file
	float mins[3], maxs[3], yawradius, allradius; // for clipping uses

	// these offsets are relative to the file
	unsigned int num_bones;
	unsigned int num_meshs;
	unsigned int num_frames;
	unsigned int ofs_bones; // dpmbone_t bone[num_bones];
	unsigned int ofs_meshs; // dpmmesh_t mesh[num_meshs];
	unsigned int ofs_frames; // dpmframe_t frame[num_frames];
} dpmheader_t;

// there may be more than one of these
typedef struct dpmmesh_s
{
	// these offsets are relative to the file
	char shadername[32]; // name of the shader to use
	unsigned int num_verts;
	unsigned int num_tris;
	unsigned int ofs_verts; // dpmvertex_t vert[numvertices]; // see vertex struct
	unsigned int ofs_texcoords; // float texcoords[numvertices][2];
	unsigned int ofs_indices; // unsigned int indices[numtris*3]; // designed for glDrawElements (each triangle is 3 unsigned int indices)
	unsigned int ofs_groupids; // unsigned int groupids[numtris]; // the meaning of these values is entirely up to the gamecode and modeler
} dpmmesh_t;

// if set on a bone, it must be protected from removal
#define DPMBONEFLAG_ATTACHMENT 1

// one per bone
typedef struct dpmbone_s
{
	// name examples: upperleftarm leftfinger1 leftfinger2 hand, etc
	char name[32];
	// parent bone number
	signed int parent;
	// flags for the bone
	unsigned int flags;
} dpmbone_t;

// a bonepose matrix is intended to be used like this:
// (n = output vertex, v = input vertex, m = matrix, f = influence)
// n[0] = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2] + f * m[0][3];
// n[1] = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2] + f * m[1][3];
// n[2] = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2] + f * m[2][3];
typedef struct dpmbonepose_s
{
	float matrix[3][4];
} dpmbonepose_t;

// immediately followed by bone positions for the frame
typedef struct dpmframe_s
{
	// name examples: idle_1 idle_2 idle_3 shoot_1 shoot_2 shoot_3, etc
	char name[32];
	float mins[3], maxs[3], yawradius, allradius;
	int ofs_bonepositions; // dpmbonepose_t bonepositions[bones];
} dpmframe_t;

// one or more of these per vertex
typedef struct dpmbonevert_s
{
	float origin[3]; // vertex location (these blend)
	float influence; // influence fraction (these must add up to 1)
	float normal[3]; // surface normal (these blend)
	unsigned int bonenum; // number of the bone
} dpmbonevert_t;

// variable size, parsed sequentially
typedef struct dpmvertex_s
{
	unsigned int numbones;
	// immediately followed by 1 or more dpmbonevert_t structures
} dpmvertex_t;

static qboolean QDECL Mod_LoadDarkPlacesModel(model_t *mod, void *buffer, size_t fsize)
{
#ifndef SERVERONLY
	galiasskin_t *skin;
	skinframe_t *skinframe;
	int skinfiles;
	float *inst;
	float *outst;
#endif

	int i, j, k;

	dpmheader_t *header;
	galiasinfo_t *root, *m;
	dpmmesh_t *mesh;
	dpmvertex_t *vert;
	dpmbonevert_t *bonevert;

	galisskeletaltransforms_t *transforms, *firsttransform;

	galiasbone_t *outbone;
	dpmbone_t *inbone;

	float *outposedata;
	galiasanimation_t *outgroups;
	float *inposedata;
	dpmframe_t *inframes;

	unsigned int *index;	index_t *outdex;	// groan...

	int numtransforms;

	int numgroups;
	frameinfo_t *framegroups;

	header = buffer;

	if (memcmp(header->id, "DARKPLACESMODEL\0", 16))
	{
		Con_Printf(CON_ERROR "Mod_LoadDarkPlacesModel: %s, doesn't appear to be a darkplaces model!\n", mod->name);
		return false;
	}

	if (BigLong(header->type) != 2)
	{
		Con_Printf(CON_ERROR "Mod_LoadDarkPlacesModel: %s, only type 2 is supported\n", mod->name);
		return false;
	}

	for (i = 0; i < sizeof(dpmheader_t)/4; i++)
		((int*)header)[i] = BigLong(((int*)header)[i]);

	if (!header->num_bones)
	{
		Con_Printf(CON_ERROR "Mod_LoadDarkPlacesModel: %s, no bones\n", mod->name);
		return false;
	}
	if (!header->num_frames)
	{
		Con_Printf(CON_ERROR "Mod_LoadDarkPlacesModel: %s, no frames\n", mod->name);
		return false;
	}
	if (!header->num_meshs)
	{
		Con_Printf(CON_ERROR "Mod_LoadDarkPlacesModel: %s, no surfaces\n", mod->name);
		return false;
	}


	VectorCopy(header->mins, mod->mins);
	VectorCopy(header->maxs, mod->maxs);

	root = ZG_Malloc(&mod->memgroup, sizeof(galiasinfo_t)*header->num_meshs);

	outbone = ZG_Malloc(&mod->memgroup, sizeof(galiasbone_t)*header->num_bones);
	inbone = (dpmbone_t*)((char*)buffer + header->ofs_bones);
	for (i = 0; i < header->num_bones; i++)
	{
		outbone[i].parent = BigLong(inbone[i].parent);
		if (outbone[i].parent >= i || outbone[i].parent < -1)
		{
			Con_Printf(CON_ERROR "Mod_LoadDarkPlacesModel: bad bone index in %s\n", mod->name);
			return false;
		}

		Q_strncpyz(outbone[i].name, inbone[i].name, sizeof(outbone[i].name));
		//throw away the flags.
	}

	framegroups = ParseFrameInfo(mod, &numgroups);
	if (!framegroups)
	{	//use the dpm's poses directly.
		numgroups = header->num_frames;
		outgroups = ZG_Malloc(&mod->memgroup, sizeof(galiasanimation_t)*numgroups + sizeof(float)*header->num_frames*header->num_bones*12);
		outposedata = (float*)(outgroups+numgroups);

		inframes = (dpmframe_t*)((char*)buffer + header->ofs_frames);
		for (i = 0; i < numgroups; i++)
		{
			inframes[i].ofs_bonepositions = BigLong(inframes[i].ofs_bonepositions);
			inframes[i].allradius = BigLong(inframes[i].allradius);
			inframes[i].yawradius = BigLong(inframes[i].yawradius);
			inframes[i].mins[0] = BigLong(inframes[i].mins[0]);
			inframes[i].mins[1] = BigLong(inframes[i].mins[1]);
			inframes[i].mins[2] = BigLong(inframes[i].mins[2]);
			inframes[i].maxs[0] = BigLong(inframes[i].maxs[0]);
			inframes[i].maxs[1] = BigLong(inframes[i].maxs[1]);
			inframes[i].maxs[2] = BigLong(inframes[i].maxs[2]);

			Q_strncpyz(outgroups[i].name, inframes[i].name, sizeof(outgroups[i].name));

			outgroups[i].rate = 10;
			outgroups[i].numposes = 1;
			outgroups[i].skeltype = SKEL_RELATIVE;
			outgroups[i].boneofs = outposedata;
			outgroups[i].action = -1;
			outgroups[i].actionweight = 0;

			inposedata = (float*)((char*)buffer + inframes[i].ofs_bonepositions);
			for (j = 0; j < header->num_bones*12; j++)
				*outposedata++ = BigFloat(*inposedata++);
		}
	}
	else
	{	//we read a .framegroups file to remap everything.
		outgroups = ZG_Malloc(&mod->memgroup, sizeof(galiasanimation_t)*numgroups + sizeof(float)*header->num_frames*header->num_bones*12);
		outposedata = (float*)(outgroups+numgroups);

		inframes = (dpmframe_t*)((char*)buffer + header->ofs_frames);
		for (i = 0; i < header->num_frames; i++)
		{
			inposedata = (float*)((char*)buffer + BigLong(inframes[i].ofs_bonepositions));
			for (j = 0; j < header->num_bones*12; j++)
				*outposedata++ = BigFloat(*inposedata++);
		}
		outposedata -= header->num_frames*header->num_bones*12;

		for (i = 0; i < numgroups; i++)
		{
			int firstpose = framegroups[i].firstpose, numposes = framegroups[i].posecount;
			if (firstpose >= header->num_frames)
				firstpose = header->num_frames-1;
			if (firstpose < 0)
				firstpose = 0;
			if (numposes < 0)
				numposes = 0;
			if (firstpose + numposes > header->num_frames)
				numposes = header->num_frames - firstpose;
			if (framegroups[i].posesarray)
				Con_Printf(CON_WARNING"Mod_LoadDarkPlacesModel(%s): No support for explicit pose lists\n", mod->name);
			outgroups[i].skeltype = SKEL_RELATIVE;
			outgroups[i].boneofs = outposedata + firstpose*header->num_bones*12;
			outgroups[i].numposes = numposes;
			outgroups[i].loop = framegroups[i].loop;
			outgroups[i].rate = framegroups[i].fps;
			outgroups[i].events = framegroups[i].events;
			outgroups[i].action = framegroups[i].action;
			outgroups[i].actionweight = framegroups[i].actionweight;
			Q_strncpyz(outgroups[i].name, framegroups[i].name, sizeof(outgroups[i].name));
		}
	}


#ifndef SERVERONLY
	skinfiles = Mod_CountSkinFiles(mod);
	if (skinfiles < 1)
		skinfiles = 1;
#endif

	mesh = (dpmmesh_t*)((char*)buffer + header->ofs_meshs);
	for (i = 0; i < header->num_meshs; i++, mesh++)
	{
			m = &root[i];
		Mod_DefaultMesh(m, mesh->shadername, i);
		if (i < header->num_meshs-1)
			m->nextsurf = &root[i+1];
		m->shares_bones = 0;

		m->ofsbones = outbone;
		m->numbones = header->num_bones;

		m->numanimations = numgroups;
		m->ofsanimations = outgroups;

#ifdef SERVERONLY
		m->numskins = 1;
#else
		m->numskins = skinfiles;

		skin = ZG_Malloc(&mod->memgroup, (sizeof(galiasskin_t)+sizeof(skinframe_t))*skinfiles);
		skinframe = (skinframe_t*)(skin+skinfiles);
		for (j = 0; j < skinfiles; j++, skinframe++)
		{
			skin[j].numframes = 1;	//non-sequenced skins.
			skin[j].frame = skinframe;
			skin[j].skinwidth = 1;
			skin[j].skinheight = 1;
			skin[j].skinspeed = 10; /*something to avoid div by 0*/

			if (!j)
			{
				Q_strncpyz(skin[j].name, mesh->shadername, sizeof(skin[j].name));
				Q_strncpyz(skinframe->shadername, mesh->shadername, sizeof(skin[j].name));
			}
			else
			{
				Q_strncpyz(skin[j].name, "", sizeof(skin[j].name));
				Q_strncpyz(skinframe->shadername, "", sizeof(skin[j].name));
			}

		}

		m->ofsskins = skin;
#endif


		mesh->num_verts = BigLong(mesh->num_verts);
		mesh->num_tris = BigLong(mesh->num_tris);
		mesh->ofs_verts = BigLong(mesh->ofs_verts);
		mesh->ofs_texcoords = BigLong(mesh->ofs_texcoords);
		mesh->ofs_indices = BigLong(mesh->ofs_indices);
		mesh->ofs_groupids = BigLong(mesh->ofs_groupids);

		index = (unsigned int*)((char*)buffer + mesh->ofs_indices);
		outdex = ZG_Malloc(&mod->memgroup, mesh->num_tris*3*sizeof(index_t));
		m->ofs_indexes = outdex;
		m->numindexes = mesh->num_tris*3;
		for (j = 0; j < mesh->num_tris; j++, index += 3, outdex += 3)
		{
			outdex[0] = BigLong(index[2]);
			outdex[1] = BigLong(index[1]);
			outdex[2] = BigLong(index[0]);
		}


		numtransforms = 0;
		//count and byteswap the transformations
		vert = (dpmvertex_t*)((char *)buffer+mesh->ofs_verts);
		for (j = 0; j < mesh->num_verts; j++)
		{
			vert->numbones = BigLong(vert->numbones);
			numtransforms += vert->numbones;
			bonevert = (dpmbonevert_t*)(vert+1);
			vert = (dpmvertex_t*)(bonevert+vert->numbones);
		}

		m = &root[i];
		m->shares_verts = i;
		m->shares_bones = 0;
#ifndef SERVERONLY
		outst = ZG_Malloc(&mod->memgroup, mesh->num_verts*sizeof(vec2_t));
		m->ofs_st_array = (vec2_t*)outst;
		m->numverts = mesh->num_verts;
		inst = (float*)((char*)buffer + mesh->ofs_texcoords);
		for (j = 0; j < mesh->num_verts; j++, outst+=2, inst+=2)
		{
			outst[0] = BigFloat(inst[0]);
			outst[1] = BigFloat(inst[1]);
		}
#endif


		firsttransform = transforms = Z_Malloc(numtransforms*sizeof(galisskeletaltransforms_t));
		//build the transform list.
		vert = (dpmvertex_t*)((char *)buffer+mesh->ofs_verts);
		for (j = 0; j < mesh->num_verts; j++)
		{
			bonevert = (dpmbonevert_t*)(vert+1);
			for (k = 0; k < vert->numbones; k++, bonevert++, transforms++)
			{
				transforms->boneindex = BigLong(bonevert->bonenum);
				transforms->vertexindex = j;
				transforms->org[0] = BigFloat(bonevert->origin[0]);
				transforms->org[1] = BigFloat(bonevert->origin[1]);
				transforms->org[2] = BigFloat(bonevert->origin[2]);
				transforms->org[3] = BigFloat(bonevert->influence);
#ifndef SERVERONLY
				transforms->normal[0] = BigFloat(bonevert->normal[0]);
				transforms->normal[1] = BigFloat(bonevert->normal[1]);
				transforms->normal[2] = BigFloat(bonevert->normal[2]);
#endif
			}
			vert = (dpmvertex_t*)bonevert;
		}
		Alias_BuildGPUWeights(mod, m, numtransforms, firsttransform, false);
		Z_Free(firsttransform);
	}

	mod->flags = Mod_ReadFlagsFromMD1(mod->name, 0);	//file replacement - inherit flags from any defunc mdl files.

	Mod_ClampModelSize(mod);
	Mod_ParseModelEvents(mod, root->ofsanimations, root->numanimations);

	mod->meshinfo = root;
	mod->numframes = root->numanimations;
	mod->type = mod_alias;
	mod->funcs.NativeTrace = Mod_Trace;

	free(framegroups);

	return true;
}
#endif	//DPMMODELS


#ifdef INTERQUAKEMODELS
#define IQM_MAGIC "INTERQUAKEMODEL"
#define IQM_VERSION1 1
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

struct iqmjoint1
{
	unsigned int name;
	int parent;
	float translate[3], rotate[3], scale[3];
};
struct iqmjoint2
{
	unsigned int name;
	int parent;
	float translate[3], rotate[4], scale[3];
};

struct iqmpose1
{
	int parent;
	unsigned int mask;
	float channeloffset[9];
	float channelscale[9];
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
//skin lump is made of 3 parts
struct iqmext_fte_skin
{
	unsigned int numskinframes;
	unsigned int nummeshskins;
	//unsigned int numskins[nummeshes];
	//iqmext_fte_skin_skinframe[numskinframes];
	//iqmext_fte_skin_meshskin mesh0[numskins[0]];
	//iqmext_fte_skin_meshskin mesh1[numskins[1]]; etc
};
struct iqmext_fte_skin_skinframe
{	//as many as needed
	unsigned int material_idx;
	unsigned int shadertext_idx;
};
struct iqmext_fte_skin_meshskin
{
	unsigned int firstframe;	//index into skinframes
	unsigned int countframes;	//skinframes
	float interval;
};
/*struct iqmext_fte_shader
{
	unsigned int material;
	unsigned int defaultshaderbody;
	unsigned int ofs_data;		//rgba or 8bit, depending on whether a palette is specified
	unsigned int width;			//if ofs_data
	unsigned int height;		//if ofs_data
	unsigned int ofs_palette;	//rgba*256. colourmapped if exactly matches quake's palette
};
//struct iqmext_fte_ragdoll; // pure text
struct iqmext_fte_bone
{
	unsigned int bonegroup;
};*/

/*
galisskeletaltransforms_t *IQM_ImportTransforms(int *resultcount, int inverts, float *vpos, float *tcoord, float *vnorm, float *vtang, unsigned char *vbone, unsigned char *vweight)
{
	galisskeletaltransforms_t *t, *r;
	unsigned int num_t = 0;
	unsigned int v, j;
	for (v = 0; v < inverts*4; v++)
	{
		if (vweight[v])
			num_t++;
	}
	t = r = Hunk_Alloc(sizeof(*r)*num_t);
	for (v = 0; v < inverts; v++)
	{
		for (j = 0; j < 4; j++)
		{
			if (vweight[(v<<2)+j])
			{
				t->boneindex = vbone[(v<<2)+j];
				t->vertexindex = v;
				VectorScale(vpos, vweight[(v<<2)+j]/255.0, t->org);
				VectorScale(vnorm, vweight[(v<<2)+j]/255.0, t->normal);
				t++;
			}
		}
	}
	return r;
}
*/

static qboolean IQM_ImportArray4Bone(const qbyte *fte_restrict base, const struct iqmvertexarray *fte_restrict src, bone_vec4_t *fte_restrict out, size_t count, unsigned int maxval)
{
	size_t i;
	unsigned int j;
	unsigned int sz = LittleLong(src->size);
	unsigned int fmt = LittleLong(src->format);
	unsigned int offset = LittleLong(src->offset);
	qboolean invalid = false;
	maxval = min(MAX_BONES,maxval);	//output is bytes.
	if (!offset)
	{
		sz = 0;
		fmt = IQM_UBYTE;
	}
	switch(fmt)
	{
	default:
		sz = 0;
		invalid = true;
		break;
	case IQM_BYTE:	//FIXME: should be signed, but this makes no sense for our uses
	case IQM_UBYTE:
		{
			const qbyte *in = (const qbyte*)(base+offset);
			/*if (sz == 4)
				memcpy(out, in, count * sizeof(*out));	//the fast path.
			else*/ for (i = 0; i < count; i++)
			{
				for (j = 0; j < 4 && j < sz; j++)
				{
					if (in[i*sz+j] >= maxval)
					{
						out[i][j] = 0;
						invalid = true;
					}
					else
						out[i][j] = in[i*sz+j];
				}
			}
		}
		break;
	case IQM_SHORT://FIXME: should be signed, but this makes no sense for our uses
	case IQM_USHORT:
		{
			const unsigned short *in = (const unsigned short*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < 4 && j < sz; j++)
				{
					if (in[i*sz+j] >= maxval)
					{
						out[i][j] = 0;
						invalid = true;
					}
					else
						out[i][j] = in[i*sz+j];
				}
			}
		}
		break;
	case IQM_INT://FIXME: should be signed, but this makes no sense for our uses
	case IQM_UINT:
		{
			const unsigned int *in = (const unsigned int*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < 4 && j < sz; j++)
				{
					if (in[i*sz+j] >= maxval)
					{
						out[i][j] = 0;
						invalid = true;
					}
					else
						out[i][j] = in[i*sz+j];
				}
			}
		}
		break;
	//float types don't really make sense
	}

	//if there were not enough elements, pad it.
	if (sz < 4)
	{
		for (i = 0; i < count; i++)
		{
			for (j = sz; j < 4; j++)
				out[i][j] = 0;
		}
	}

	return !invalid;
}
static void IQM_ImportArrayF(const qbyte *fte_restrict base, const struct iqmvertexarray *fte_restrict src, float *fte_restrict out, size_t e, size_t count, float *def)
{
	size_t i;
	unsigned int j;
	unsigned int sz = LittleLong(src->size);
	unsigned int fmt = LittleLong(src->format);
	unsigned int offset = LittleLong(src->offset);
	if (!offset)
	{
		sz = 0;
		fmt = IQM_FLOAT;
	}
	switch(fmt)
	{
	default:
		sz = 0;
		break;
	case IQM_BYTE:	//negatives not handled properly
		{
			const signed char *in = (const signed char*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < e && j < sz; j++)
					out[i*e+j] = in[i*sz+j] * (1.0/127);
			}
		}
		break;
	case IQM_UBYTE:
		{
			const qbyte *in = (const qbyte*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < e && j < sz; j++)
					out[i*e+j] = in[i*sz+j] * (1.0/255);
			}
		}
		break;
	case IQM_SHORT:	//negatives not handled properly
		{
			const signed short *in = (const signed short*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < e && j < sz; j++)
					out[i*e+j] = in[i*sz+j] * (1.0/32767);
			}
		}
		break;
	case IQM_USHORT:
		{
			const unsigned short *in = (const unsigned short*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < e && j < sz; j++)
					out[i*e+j] = in[i*sz+j] * (1.0/65535);
			}
		}
		break;
	case IQM_INT:	//negatives not handled properly
		{
			const signed int *in = (const signed int*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < e && j < sz; j++)
					out[i*e+j] = in[i*sz+j]/(float)0x7fffffff;
			}
		}
		break;
	case IQM_UINT:
		{
			const unsigned int *in = (const unsigned int*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < e && j < sz; j++)
					out[i*e+j] = in[i*sz+j]/(float)0xffffffffu;
			}
		}
		break;

	case IQM_HALF:
#ifdef __F16C__
		{	//x86 intrinsics
			const unsigned short *in = (const unsigned short*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < e && j < sz; j++)
					out[i*e+j] = _cvtsh_ss(in[i*sz+j]);
			}
		}
#elif 0
		{
			const _Float16 *in = (const _Float16*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < e && j < sz; j++)
					out[i*e+j] = in[i*sz+j];
			}
		}
#else
		sz = 0;
#endif
		break;
	case IQM_FLOAT:
		{
			const float *in = (const float*)(base+offset);
			if (e == sz)
				memcpy(out, in, e * sizeof(float) * count);
			else for (i = 0; i < count; i++)
			{
				for (j = 0; j < e && j < sz; j++)
					out[i*e+j] = in[i*sz+j];
			}
		}
		break;
	case IQM_DOUBLE:
		{
			const double *in = (const double*)(base+offset);
			for (i = 0; i < count; i++)
			{
				for (j = 0; j < e && j < sz; j++)
					out[i*e+j] = in[i*sz+j];
			}
		}
		break;
	}

	//if there were not enough elements, pad it.
	if (sz < e)
	{
		for (i = 0; i < count; i++)
		{
			for (j = sz; j < e; j++)
				out[i*e+j] = def[j];
		}
	}
}

static const void *IQM_FindExtension(const char *buffer, size_t buffersize, const char *extname, int index, size_t *extsize)
{
	const struct iqmheader *h = (const struct iqmheader *)buffer;
	const char *strings = buffer + h->ofs_text;
	const struct iqmextension *ext;
	int i;
	for (i = 0, ext = (const struct iqmextension*)(buffer + h->ofs_extensions); i < h->num_extensions; i++, ext = (const struct iqmextension*)(buffer + ext->ofs_extensions))
	{
		if ((const char*)ext > buffer+buffersize || ext->name > h->num_text || ext->ofs_data+ext->num_data>buffersize)
			break;
		if (!Q_strcasecmp(strings + ext->name, extname) && index-->=0)
		{
			*extsize = ext->num_data;
			return buffer + ext->ofs_data;
		}
	}
	*extsize = 0;
	return NULL;
}

static void Mod_CleanWeights(const char *modelname, size_t numverts, vec4_t *oweight, bone_vec4_t *oindex)
{	//some IQMs lack weight values, apparently.
	int j, v;
	qboolean problemfound = false;
	for (v = 0; v < numverts; v++)
	{
		float t = oweight[v][0]+oweight[v][1]+oweight[v][2]+oweight[v][3];
		if (!t)
		{
			problemfound = true;
			Vector4Set(oweight[v], 1, 0, 0, 0);
		}
		else if (t < 0.99 || t > 1.01)
			Vector4Scale(oweight[v], 1/t, oweight[v]);

		//compact any omitted weights...
		for(j = 3; j > 0; )
		{
			if (oweight[v][j] && !oweight[v][j-1])
			{
				problemfound = true;
				oweight[v][j-1] = oweight[v][j];
				oindex[v][j-1] = oindex[v][j];
				oweight[v][j] = 0;
				j++; //bubble back up
			}
			else
				j--;
		}
	}
	if (problemfound)
		Con_DPrintf(CON_ERROR"%s has invalid vertex weights. Verticies will probably be attached to the wrong bones\n", modelname);
}

struct iqmstrings_s
{
	const char *base;
	size_t size;
};
static const char *Mod_IQMString(struct iqmstrings_s *strings, int offset)
{
	if (offset < 0 || offset >= strings->size)
		return "<BADSTRING>";
	return strings->base + offset;
}
static galiasinfo_t *Mod_ParseIQMMeshModel(model_t *mod, const char *buffer, size_t fsize)
{
	const struct iqmheader *h = (const struct iqmheader *)buffer;
	const struct iqmmesh *mesh;
	const struct iqmvertexarray *varray;
	const struct iqmtriangle *tris;
	const struct iqmanim *anim;
	const struct iqmext_fte_mesh *ftemesh;
	const struct iqmext_fte_event *fteevents;
	struct iqmstrings_s strings;

	const unsigned int *fteskincount;
	const struct iqmext_fte_skin_meshskin *fteskins;
	const struct iqmext_fte_skin_skinframe *fteskinframes;

	unsigned int i, j, t, numtris, numverts, firstvert, firsttri;
	size_t extsize;

	const float *vtang = NULL;
	struct iqmvertexarray vpos = {0}, vnorm = {0}, vtcoord = {0}, vbone = {0}, vweight = {0}, vrgba = {0};
	unsigned int type, fmt, size, offset;
	const unsigned short *framedata;
	vec4_t defaultcolour = {1,1,1,1};
	vec4_t defaultweight = {0,0,0,0};
	vec4_t defaultvert = {0,0,0,1};

	const struct iqmbounds	*inbounds;

	int memsize;
	qbyte *obase=NULL;
	vecV_t *opos=NULL;
	vec3_t *onorm1=NULL, *onorm2=NULL, *onorm3=NULL;
	vec4_t *oweight=NULL;
	bone_vec4_t *oindex=NULL;
	float *opose=NULL,*oposebase=NULL;
	vec2_t *otcoords = NULL;
	vec4_t *orgbaf = NULL;


	galiasinfo_t *gai=NULL;
#ifndef SERVERONLY
	int skinfiles;
#endif
	galiasanimation_t *fgroup=NULL;
	galiasbone_t *bones = NULL;
	index_t *idx;
	qboolean noweights;
	frameinfo_t *framegroups;
	int numgroups;
	qboolean fuckedevents = false;

	if (memcmp(h->magic, IQM_MAGIC, sizeof(h->magic)))
	{
		Con_Printf("%s: format not recognised\n", mod->name);
		return NULL;
	}
	if (h->version != IQM_VERSION1 && h->version != IQM_VERSION2)
	{
		Con_Printf("%s: unsupported IQM version\n", mod->name);
		return NULL;
	}
	if (h->filesize != fsize)
	{
		Con_Printf("%s: size (%u != %"PRIuSIZE")\n", mod->name, h->filesize, fsize);
		return NULL;
	}

	varray = (const struct iqmvertexarray*)(buffer + h->ofs_vertexarrays);
	for (i = 0; i < h->num_vertexarrays; i++)
	{
		type = LittleLong(varray[i].type);
		fmt = LittleLong(varray[i].format);
		size = LittleLong(varray[i].size);
		offset = LittleLong(varray[i].offset);
		if (type == IQM_POSITION)
			vpos = varray[i];
		else if (type == IQM_TEXCOORD)
			vtcoord = varray[i];
		else if (type == IQM_NORMAL)
			vnorm = varray[i];
		else if (type == IQM_TANGENT && fmt == IQM_FLOAT && size == 4) /*yup, 4, extra is side, for the bitangent*/
			vtang = (const float*)(buffer + offset);
		else if (type == IQM_BLENDINDEXES)
			vbone = varray[i];
		else if (type == IQM_BLENDWEIGHTS)
			vweight = varray[i];
		else if (type == IQM_COLOR)
			vrgba = varray[i];
		else
			Con_Printf("Unrecognised iqm info (type=%i, fmt=%i, size=%i)\n", type, fmt, size);
	}

	/*if (!h->num_meshes)
	{
		Con_Printf("%s: IQM has no meshes\n", mod->name);
		return NULL;
	}*/

	//a mesh must contain vertex coords or its not much of a mesh.
	//we also require texcoords because we can.
	//we don't require normals
	//we don't require weights, but such models won't animate.
	if (h->num_vertexes > 0 && (!vpos.offset || !vtcoord.offset))
	{
		Con_Printf("%s is missing vertex array data\n", mod->name);
		return NULL;
	}
	noweights = !vbone.offset || !vweight.offset;
	if (!h->num_meshes)
		noweights = true;
	else if (noweights)
	{
		if (h->num_frames || h->num_anims || h->num_joints)
		{
			Con_Printf("%s: animated IQM lacks bone weights\n", mod->name);
			return NULL;
		}
	}

	if (h->num_joints > MAX_BONES)
	{
		Con_Printf("%s: IQM has %u joints, max supported is %u.\n", mod->name, h->num_joints, MAX_BONES);
		return NULL;
	}
	if (h->num_poses > MAX_BONES)
	{
		Con_Printf("%s: IQM has %u bones, max supported is %u.\n", mod->name, h->num_joints, MAX_BONES);
		return NULL;
	}
	if (h->num_joints && h->num_poses && h->num_joints != h->num_poses)
	{
		Con_Printf("%s: IQM has mismatched joints (%i vs %i).\n", mod->name, h->num_joints, h->num_poses);
		return NULL;
	}
	if (h->num_meshes && !noweights && !h->num_joints)
	{
		Con_Printf("%s: mesh IQM has no joints.\n", mod->name);
		return NULL;
	}
	if (h->num_frames && !h->num_poses)
	{
		Con_Printf("%s: animated IQM has no poses.\n", mod->name);
		return NULL;
	}

	strings.base = buffer + h->ofs_text;
	strings.size = h->num_text;

	/*try to completely disregard all the info the creator carefully added to their model...*/
	numgroups = 0;
	framegroups = NULL;
	if (!numgroups)
		framegroups = ParseFrameInfo(mod, &numgroups);
	if (!numgroups && h->num_anims)
	{
		/*use the model's framegroups*/
		numgroups = h->num_anims;
		framegroups = malloc(sizeof(*framegroups)*numgroups);

		anim = (const struct iqmanim*)(buffer + h->ofs_anims);
		for (i = 0; i < numgroups; i++)
		{
			framegroups[i].posesarray = false;
			framegroups[i].firstpose = LittleLong(anim[i].first_frame);
			framegroups[i].posecount = LittleLong(anim[i].num_frames);
			framegroups[i].fps = LittleFloat(anim[i].framerate);
			framegroups[i].loop = !!(LittleLong(anim[i].flags) & IQM_LOOP);
			framegroups[i].events = NULL;
			framegroups[i].action = -1;
			framegroups[i].actionweight = 0;
			Q_strncpyz(framegroups[i].name, Mod_IQMString(&strings, anim[i].name), sizeof(fgroup[i].name));
		}
	}
	else
		fuckedevents = true;	//we're not using the animation data from the model, so ignore any events because they won't make sense any more.
	if (!numgroups && !noweights && h->num_joints)
	{	/*base frame only*/
		numgroups = 1;
		framegroups = malloc(sizeof(*framegroups));
		framegroups->posesarray = false;
		framegroups->firstpose = -1;
		framegroups->posecount = 1;
		framegroups->fps = 10;
		framegroups->loop = 1;
		framegroups->events = NULL;
		framegroups->action = -1;
		framegroups->actionweight = 0;
		strcpy(framegroups->name, "base");
	}

	mesh = (const struct iqmmesh*)(buffer + h->ofs_meshes);

#ifndef SERVERONLY
	skinfiles = Mod_CountSkinFiles(mod);
	if (skinfiles < 1)
		skinfiles = 1;	//iqms have 1 skin and one skin only and always. make sure its loaded.
#endif

	/*allocate a nice big block of memory and figure out where stuff is*/
	/*run through twice, so things are consistant*/
#define dalloc(o,count) do{o = (void*)(obase+memsize); memsize += sizeof(*o)*(count);}while(0)
	for (i = 0, memsize = 0, obase = NULL; i < 2; i++)
	{
		if (i)
			obase = ZG_Malloc(&mod->memgroup, memsize);
		memsize = 0;
		dalloc(gai, max(1,h->num_meshes));
		dalloc(bones, h->num_joints);
		dalloc(opos, h->num_vertexes);
		dalloc(onorm1, h->num_vertexes);
		dalloc(onorm2, h->num_vertexes);
		dalloc(onorm3, h->num_vertexes);
		if (!noweights)
		{
			dalloc(oindex, h->num_vertexes);
			dalloc(oweight, h->num_vertexes);
		}
		else
		{
			oindex = NULL;
			oweight = NULL;
		}
#ifndef SERVERONLY
		if (vtcoord.offset)
			dalloc(otcoords, h->num_vertexes);
		else
			otcoords = NULL;
		if (vrgba.offset)
			dalloc(orgbaf, h->num_vertexes);
		else
			orgbaf = NULL;
#endif
		dalloc(fgroup, numgroups);
		dalloc(oposebase, 12*h->num_joints);
		dalloc(opose, 12*(h->num_poses*h->num_frames));
	}
#undef dalloc

//no code to load animations or bones
	framedata = (const unsigned short*)(buffer + h->ofs_frames);

	/*Version 1 supports only normalized quaternions, version 2 uses complete quaternions. Some struct sizes change for this, otherwise functionally identical.*/
	if (h->version == IQM_VERSION1)
	{
		const struct iqmpose1 *p, *ipose = (const struct iqmpose1*)(buffer + h->ofs_poses);
		const struct iqmjoint1 *ijoint = (const struct iqmjoint1*)(buffer + h->ofs_joints);
		vec3_t pos;
		vec4_t quat;
		vec3_t scale;
		float mat[12];

		//joint info (mesh)
		for (i = 0; i < h->num_joints; i++)
		{
			Q_strncpyz(bones[i].name, Mod_IQMString(&strings, ijoint[i].name), sizeof(bones[i].name));
			bones[i].parent = ijoint[i].parent;

			GenMatrixPosQuat3Scale(ijoint[i].translate, ijoint[i].rotate, ijoint[i].scale, mat);

			if (ijoint[i].parent >= 0)
				Matrix3x4_Multiply(mat, &oposebase[ijoint[i].parent*12], &oposebase[i*12]);
			else
				memcpy(&oposebase[i*12], mat, sizeof(mat));
			Matrix3x4_Invert_Simple(&oposebase[i*12], bones[i].inverse);
		}

		//pose info (anim)
		for (i = 0; i < h->num_frames; i++)
		{
			for (j = 0, p = ipose; j < h->num_poses; j++, p++)
			{
				pos[0]   = p->channeloffset[0]; if (p->mask &   1) pos[0]   += *framedata++ * p->channelscale[0];
				pos[1]   = p->channeloffset[1]; if (p->mask &   2) pos[1]   += *framedata++ * p->channelscale[1];
				pos[2]   = p->channeloffset[2]; if (p->mask &   4) pos[2]   += *framedata++ * p->channelscale[2];
				quat[0]  = p->channeloffset[3]; if (p->mask &   8) quat[0]  += *framedata++ * p->channelscale[3];
				quat[1]  = p->channeloffset[4]; if (p->mask &  16) quat[1]  += *framedata++ * p->channelscale[4];
				quat[2]  = p->channeloffset[5]; if (p->mask &  32) quat[2]  += *framedata++ * p->channelscale[5];
				scale[0] = p->channeloffset[6]; if (p->mask &  64) scale[0] += *framedata++ * p->channelscale[6];
				scale[1] = p->channeloffset[7]; if (p->mask & 128) scale[1] += *framedata++ * p->channelscale[7];
				scale[2] = p->channeloffset[8]; if (p->mask & 256) scale[2] += *framedata++ * p->channelscale[8];

				quat[3] = -sqrt(max(1.0 - pow(VectorLength(quat),2), 0.0));

				GenMatrixPosQuat3Scale(pos, quat, scale, &opose[(i*h->num_poses+j)*12]);
			}
		}

		if (framedata != (const unsigned short*)(buffer + h->ofs_frames) + h->num_framechannels*h->num_frames)
			Con_Printf("%s: Incorrect number of framechannels found\n", mod->name);
	}
	else
	{
		const struct iqmpose2 *p, *ipose = (const struct iqmpose2*)(buffer + h->ofs_poses);
		const struct iqmjoint2 *ijoint = (const struct iqmjoint2*)(buffer + h->ofs_joints);
		vec3_t pos;
		vec4_t quat;
		vec3_t scale;
		float mat[12];
		int fc;

		//joint info (mesh)
		for (i = 0; i < h->num_joints; i++)
		{
			Q_strncpyz(bones[i].name, Mod_IQMString(&strings, ijoint[i].name), sizeof(bones[i].name));

			GenMatrixPosQuat4Scale(ijoint[i].translate, ijoint[i].rotate, ijoint[i].scale, mat);

			if (ijoint[i].parent >= 0 && ijoint[i].parent < i)
			{
				bones[i].parent = ijoint[i].parent;
				Matrix3x4_Multiply(mat, &oposebase[ijoint[i].parent*12], &oposebase[i*12]);
			}
			else
			{
				memcpy(&oposebase[i*12], mat, sizeof(mat));
				bones[i].parent = -1;
			}
			Matrix3x4_Invert_Simple(&oposebase[i*12], bones[i].inverse);
		}

		for (fc = 0, j = 0, p = ipose; j < h->num_poses; j++, p++)
		{
			for (i = 0; i < 10; i++)
				if (p->mask & (1<<i))
					fc++;
		}
		if (fc != h->num_framechannels)
			Con_Printf("%s: Incorrect number of framechannels found (%i, expected %i)\n", mod->name, h->num_framechannels, fc);
		else

		//pose info (anim)
		for (i = 0; i < h->num_frames; i++)
		{
			for (j = 0, p = ipose; j < h->num_poses; j++, p++)
			{
				pos[0]   = p->channeloffset[0]; if (p->mask &   1) pos[0]   += *framedata++ * p->channelscale[0];
				pos[1]   = p->channeloffset[1]; if (p->mask &   2) pos[1]   += *framedata++ * p->channelscale[1];
				pos[2]   = p->channeloffset[2]; if (p->mask &   4) pos[2]   += *framedata++ * p->channelscale[2];
				quat[0]  = p->channeloffset[3]; if (p->mask &   8) quat[0]  += *framedata++ * p->channelscale[3];
				quat[1]  = p->channeloffset[4]; if (p->mask &  16) quat[1]  += *framedata++ * p->channelscale[4];
				quat[2]  = p->channeloffset[5]; if (p->mask &  32) quat[2]  += *framedata++ * p->channelscale[5];
				quat[3]  = p->channeloffset[6]; if (p->mask &  64) quat[3]  += *framedata++ * p->channelscale[6];
				scale[0] = p->channeloffset[7]; if (p->mask & 128) scale[0] += *framedata++ * p->channelscale[7];
				scale[1] = p->channeloffset[8]; if (p->mask & 256) scale[1] += *framedata++ * p->channelscale[8];
				scale[2] = p->channeloffset[9]; if (p->mask & 512) scale[2] += *framedata++ * p->channelscale[9];

				GenMatrixPosQuat4Scale(pos, quat, scale, &opose[(i*h->num_poses+j)*12]);
			}
		}
	}

	//now generate the animations.
	for (i = 0; i < numgroups; i++)
	{
		if (framegroups[i].posesarray)
			Con_Printf(CON_WARNING"Mod_ParseIQMMeshModel(%s): framegroup[%i] poses array not suppported\n", mod->name, i);
		if (framegroups[i].firstpose + framegroups[i].posecount > h->num_frames)
			framegroups[i].posecount = h->num_frames - framegroups[i].firstpose;
		if (framegroups[i].firstpose >= h->num_frames)
		{
			//invalid/basepose.
			fgroup[i].skeltype = SKEL_ABSOLUTE;
			fgroup[i].boneofs = oposebase;
			fgroup[i].numposes = 1;
		}
		else
		{
			fgroup[i].skeltype = SKEL_RELATIVE;
			fgroup[i].boneofs = opose + framegroups[i].firstpose*12*h->num_poses;
			fgroup[i].numposes = framegroups[i].posecount;
		}

		fgroup[i].loop = framegroups[i].loop;
		fgroup[i].rate = framegroups[i].fps;
		Q_strncpyz(fgroup[i].name, framegroups[i].name, sizeof(fgroup[i].name));
	
		if (fgroup[i].rate <= 0)
			fgroup[i].rate = 10;

		fgroup[i].events = framegroups[i].events;
		fgroup[i].action = framegroups[i].action;
		fgroup[i].actionweight = framegroups[i].actionweight;
	}
	free(framegroups);

	if (fuckedevents)
	{
		fteevents = NULL;
		extsize = 0;
	}
	else
		fteevents = IQM_FindExtension(buffer, fsize, "FTE_EVENT", 0, &extsize);
	if (fteevents && !(extsize % sizeof(*fteevents)))
	{
		galiasevent_t *oevent, **link;
		extsize /= sizeof(*fteevents);
		oevent = ZG_Malloc(&mod->memgroup, sizeof(*oevent)*extsize);
		for (; extsize>0; extsize--, fteevents++,oevent++)
		{
			oevent->timestamp = fteevents->timestamp;
			oevent->code = fteevents->evcode;
			oevent->data = ZG_Malloc(&mod->memgroup, strlen(Mod_IQMString(&strings, fteevents->evdata_str))+1);
			strcpy(oevent->data, Mod_IQMString(&strings, fteevents->evdata_str));
			link = &fgroup[fteevents->anim].events;
			while (*link && (*link)->timestamp <= oevent->timestamp)
				link = &(*link)->next;
			oevent->next = *link;
			*link = oevent;
		}
	}

	//determine the bounds
	inbounds = (const struct iqmbounds*)(buffer + h->ofs_bounds);
	if (h->ofs_bounds)
	{
		for (i = 0; i < h->num_frames; i++)
		{
			vec3_t mins, maxs;
			mins[0] = LittleFloat(inbounds[i].bbmin[0]);
			mins[1] = LittleFloat(inbounds[i].bbmin[1]);
			mins[2] = LittleFloat(inbounds[i].bbmin[2]);
			AddPointToBounds(mins, mod->mins, mod->maxs);
			maxs[0] = LittleFloat(inbounds[i].bbmax[0]);
			maxs[1] = LittleFloat(inbounds[i].bbmax[1]);
			maxs[2] = LittleFloat(inbounds[i].bbmax[2]);
			AddPointToBounds(maxs, mod->mins, mod->maxs);
		}
	}



	ftemesh = IQM_FindExtension(buffer, fsize, "FTE_MESH", 0, &extsize);
	if (!extsize || extsize != sizeof(*ftemesh)*h->num_meshes)
		ftemesh = NULL;	//erk.

	fteskincount = IQM_FindExtension(buffer, fsize, "FTE_SKINS", 0, &extsize);
	if (extsize >= sizeof(unsigned int)*(2+h->num_meshes) && extsize == sizeof(struct iqmext_fte_skin) + sizeof(unsigned int)*h->num_meshes + LittleLong(fteskincount[0])*sizeof(*fteskinframes) + LittleLong(fteskincount[1])*sizeof(*fteskins))
	{
		unsigned int numskinframes = LittleLong(*fteskincount++);
		/*unsigned int numskins = LittleLong(* */fteskincount++;

		fteskinframes = (const struct iqmext_fte_skin_skinframe*)(fteskincount+h->num_meshes);
		fteskins = (const struct iqmext_fte_skin_meshskin*)(fteskinframes+numskinframes);
	}
	else fteskincount = NULL, fteskins = NULL, fteskinframes = NULL;

	for (i = 0; i < max(1, h->num_meshes); i++)
	{
		if (h->num_meshes)
		{
			Mod_DefaultMesh(&gai[i], Mod_IQMString(&strings, mesh[i].name), i);
			firstvert = LittleLong(mesh[i].first_vertex);
			numverts = LittleLong(mesh[i].num_vertexes);
			numtris = LittleLong(mesh[i].num_triangles);
			firsttri = LittleLong(mesh[i].first_triangle);
		}
		else
		{	//animation-only models still need a place-holder mesh.
			firstvert = 0;
			numverts = 0;
			numtris = 0;
			firsttri = 0;
		}

		gai[i].nextsurf = &gai[i+1];

		/*animation info*/
		gai[i].shares_bones = 0;
		gai[i].numbones = h->num_joints?h->num_joints:h->num_poses;
		gai[i].ofsbones = bones;
		gai[i].numanimations = numgroups;
		gai[i].ofsanimations = fgroup;

#ifndef SERVERONLY
		/*colours*/
		gai[i].ofs_rgbaf = orgbaf?(orgbaf+firstvert):NULL;
		gai[i].ofs_rgbaub = NULL;
		/*texture coords*/
		gai[i].ofs_st_array = (otcoords+firstvert);
		/*skins*/
		if (h->num_meshes)
		{
			galiasskin_t *skin;
			skinframe_t *skinframe;
			unsigned int iqmskins = fteskincount?LittleLong(*fteskincount++):0;
			gai[i].numskins = max(iqmskins,skinfiles);
			gai[i].ofsskins = skin = ZG_Malloc(&mod->memgroup, sizeof(*gai[i].ofsskins)*gai[i].numskins);

			for (j = 0; j < gai[i].numskins; j++, skin++)
			{
				const struct iqmext_fte_skin_skinframe *sf;
				if (j < iqmskins)
				{
					sf = fteskinframes + LittleLong(fteskins->firstframe);
					fteskins++;
				}
				else
					sf = NULL;

				skin->skinwidth = 1;
				skin->skinheight = 1;
				Q_snprintfz(gai[i].ofsskins[j].name, sizeof(gai[i].ofsskins[j].name), "%i", j);
				if (sf)
				{
					skin->skinspeed = 1.0/LittleFloat(fteskins[-1].interval); /*something to avoid div by 0*/
					skin->numframes = LittleLong(fteskins[-1].countframes);	//non-sequenced skins.

					if (!skin->numframes)
						continue;

					skin->frame = ZG_Malloc(&mod->memgroup, sizeof(*skin->frame)*skin->numframes);
					for (t = 0; t < skin->numframes; t++, sf++)
					{
						Q_strncpyz(skin->frame[t].shadername, Mod_IQMString(&strings, sf->material_idx), sizeof(skin->frame[t].shadername));
						if (sf->shadertext_idx && sf->shadertext_idx<h->num_text)
						{
							const char *stxt = Mod_IQMString(&strings, sf->shadertext_idx);
							skin->frame[t].defaultshader = strcpy(ZG_Malloc(&mod->memgroup, strlen(stxt)+1), stxt);
						}
					}
				}
				else
				{
					skin->skinspeed = 10;	//something to avoid div by 0
					skin->numframes = 1;	//non-sequenced skins.

					skin->frame = skinframe = ZG_Malloc(&mod->memgroup, sizeof(*skin->frame)*skin->numframes);
					Q_strncpyz(skinframe->shadername, Mod_IQMString(&strings, mesh[i].material), sizeof(skinframe->shadername));
				}
			}
			gai[i].numskins = skin-gai[i].ofsskins;
		}
#endif

		tris = (const struct iqmtriangle*)(buffer + LittleLong(h->ofs_triangles));
		tris += firsttri;
		idx = ZG_Malloc(&mod->memgroup, sizeof(*idx)*numtris*3);
		gai[i].ofs_indexes = idx;
		for (t = 0; t < numtris; t++)
		{
			unsigned int a,b,c;
			a = LittleLong(tris[t].vertex[0]) - firstvert;
			b = LittleLong(tris[t].vertex[1]) - firstvert;
			c = LittleLong(tris[t].vertex[2]) - firstvert;
			if (a > MAX_INDICIES || b > MAX_INDICIES || c > MAX_INDICIES)
				continue;	//we can't handle this triangle properly.
			*idx++ = a;
			*idx++ = b;
			*idx++ = c;
		}
		gai[i].numindexes = idx - gai[i].ofs_indexes;
		if (gai[i].numindexes != numtris*3)
			Con_Printf("%s(%s|%s): Dropped %u of %u triangles due to index size limit\n", mod->name, gai[i].surfacename,Mod_IQMString(&strings, mesh[i].material), numtris-gai[i].numindexes/3, numtris);

		/*verts*/
		gai[i].shares_verts = i;
		gai[i].numverts = numverts;
		gai[i].ofs_skel_xyz = (opos+firstvert);
		gai[i].ofs_skel_norm = (onorm1+firstvert);
		gai[i].ofs_skel_svect = (onorm2+firstvert);
		gai[i].ofs_skel_tvect = (onorm3+firstvert);
		gai[i].ofs_skel_idx = oindex?(oindex+firstvert):NULL;
		gai[i].ofs_skel_weight = oweight?(oweight+firstvert):NULL;

		if (ftemesh)
		{	//if we have extensions, then lets use them!
			gai[i].geomset = ftemesh[i].geomset;
			gai[i].geomid = ftemesh[i].geomid;
			gai[i].contents = ftemesh[i].contents;
			gai[i].csurface.flags = ftemesh[i].surfaceflags;
			gai[i].surfaceid = ftemesh[i].surfaceid;
			gai[i].mindist = ftemesh[i].mindist;
			gai[i].maxdist = ftemesh[i].maxdist;

			mod->maxlod = max(mod->maxlod, gai[i].mindist);
			mod->maxlod = max(mod->maxlod, gai[i].maxdist);
		}
	}
	gai[i-1].nextsurf = NULL;
	if (!noweights)
	{
		if (!IQM_ImportArray4Bone(buffer, &vbone, oindex, h->num_vertexes, h->num_joints))
			Con_DPrintf(CON_WARNING "Invalid bone indexes detected inside %s\n", mod->name);
		IQM_ImportArrayF(buffer, &vweight, (float*)oweight, 4, h->num_vertexes, defaultweight);
		Mod_CleanWeights(mod->name, h->num_vertexes, oweight, oindex);
	}

	if (otcoords)
		IQM_ImportArrayF(buffer, &vtcoord, (float*)otcoords, 2, h->num_vertexes, defaultweight);
	if (orgbaf)
		IQM_ImportArrayF(buffer, &vrgba, (float*)orgbaf, 4, h->num_vertexes, defaultcolour);

	IQM_ImportArrayF(buffer, &vnorm, (float*)onorm1, 3, h->num_vertexes, defaultcolour);
	IQM_ImportArrayF(buffer, &vpos, (float*)opos, sizeof(opos[0])/sizeof(float), h->num_vertexes, defaultvert);

	if (!h->ofs_bounds || !h->num_frames)
		for (i = 0; i < h->num_vertexes; i++)
			AddPointToBounds(opos[i], mod->mins, mod->maxs);

	if (vnorm.offset && vtang)
	{
		for (i = 0; i < h->num_vertexes; i++)
		{
			VectorCopy(vtang+i*4, onorm2[i]);
			if(LittleFloat(vtang[i*4 + 3]) < 0)
				CrossProduct(onorm2[i], onorm1[i], onorm3[i]);
			else
				CrossProduct(onorm1[i], onorm2[i], onorm3[i]);
		}
	}
#ifndef SERVERONLY	//hopefully dedicated servers won't need this too often...
	else if (h->num_vertexes)
	{	//make something up
		for (i = 0; i < h->num_meshes; i++)
		{
			Mod_AccumulateTextureVectors(gai[i].ofs_skel_xyz, gai[i].ofs_st_array, gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].ofs_indexes, gai[i].numindexes, false);
		}
		for (i = 0; i < h->num_meshes; i++)
		{
			Mod_NormaliseTextureVectors(gai[i].ofs_skel_norm, gai[i].ofs_skel_svect, gai[i].ofs_skel_tvect, gai[i].numverts, false);
		}
	}
#endif

	return gai;
}

static qboolean QDECL Mod_LoadInterQuakeModel(model_t *mod, void *buffer, size_t fsize)
{
	galiasinfo_t *root;
	struct iqmheader *h = (struct iqmheader *)buffer;

	ClearBounds(mod->mins, mod->maxs);

	root = Mod_ParseIQMMeshModel(mod, buffer, fsize);
	if (!root)
	{
		return false;
	}

	mod->flags = h->flags;

	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);

	Mod_ClampModelSize(mod);
	Mod_ParseModelEvents(mod, root->ofsanimations, root->numanimations);

	mod->numframes = root->numanimations;
	mod->meshinfo = root;
	mod->type = mod_alias;
	Mod_SetMeshModelFuncs(mod, !root->ofs_skel_idx&&!root->ofs_skel_weight);

	return true;
}
#endif





#ifdef MD5MODELS

static qboolean Mod_ParseMD5Anim(model_t *mod, char *buffer, galiasinfo_t *prototype, void**poseofs, galiasanimation_t *gat)
{
#define MD5ERROR0PARAM(x) do{ Con_Printf(CON_ERROR x "\n"); return false; }while(0)
#define MD5ERROR1PARAM(x, y) do{ Con_Printf(CON_ERROR x "\n", y); return false; }while(0)
#define EXPECT(x) do{buffer = COM_ParseOut(buffer, token, sizeof(token)); if (strcmp(token, x)) MD5ERROR1PARAM("MD5ANIM: expected %s", x);}while(0)
	unsigned int i, j;

	galiasanimation_t grp;

	unsigned int parent;
	unsigned int numframes;
	unsigned int numjoints;
	float framespersecond;
	unsigned int numanimatedparts;
	galiasbone_t *bonelist;

	unsigned char *boneflags;
	unsigned int *firstanimatedcomponents;

	float *animatedcomponents;
	float *baseframe;	//6 components.
	float *posedata;
	float tx, ty, tz, qx, qy, qz;
	int fac, flags;
	float f;
	char token[8192];

	EXPECT("MD5Version");
	EXPECT("10");

	EXPECT("commandline");
	buffer = COM_ParseOut(buffer, token, sizeof(token));

	EXPECT("numFrames");
	buffer = COM_ParseOut(buffer, token, sizeof(token));
	numframes = atoi(token);

	EXPECT("numJoints");
	buffer = COM_ParseOut(buffer, token, sizeof(token));
	numjoints = atoi(token);

	EXPECT("frameRate");
	buffer = COM_ParseOut(buffer, token, sizeof(token));
	framespersecond = atof(token);

	EXPECT("numAnimatedComponents");
	buffer = COM_ParseOut(buffer, token, sizeof(token));
	numanimatedparts = atoi(token);

	firstanimatedcomponents = BZ_Malloc(sizeof(int)*numjoints);
	animatedcomponents = BZ_Malloc(sizeof(float)*numanimatedparts);
	boneflags = BZ_Malloc(sizeof(unsigned char)*numjoints);
	baseframe = BZ_Malloc(sizeof(float)*12*numjoints);

	*poseofs = posedata = ZG_Malloc(&mod->memgroup, sizeof(float)*12*numjoints*numframes);

	if (!prototype->baseframeofs)
		 prototype->baseframeofs = posedata;

	if (prototype->numbones)
	{
		if (prototype->numbones != numjoints)
			MD5ERROR0PARAM("MD5ANIM: number of bones doesn't match");
		bonelist = prototype->ofsbones;
	}
	else
	{
		bonelist = ZG_Malloc(&mod->memgroup, sizeof(galiasbone_t)*numjoints);
		prototype->ofsbones = bonelist;
	}

	EXPECT("hierarchy");
	EXPECT("{");
	for (i = 0; i < numjoints; i++, bonelist++)
	{
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		if (prototype->numbones)
		{
			if (strcmp(bonelist->name, token))
				MD5ERROR1PARAM("MD5ANIM: bone name doesn't match (%s)", token);
		}
		else
			Q_strncpyz(bonelist->name, token, sizeof(bonelist->name));
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		parent = atoi(token);
		if (prototype->numbones)
		{
			if (bonelist->parent != parent)
				MD5ERROR1PARAM("MD5ANIM: bone name doesn't match (%s)", token);
		}
		else
			bonelist->parent = parent;

		buffer = COM_ParseOut(buffer, token, sizeof(token));
		boneflags[i] = atoi(token);
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		firstanimatedcomponents[i] = atoi(token);
	}
	EXPECT("}");

	if (!prototype->numbones)
		prototype->numbones = numjoints;

	EXPECT("bounds");
	EXPECT("{");
	for (i = 0; i < numframes; i++)
	{
		EXPECT("(");
		buffer = COM_ParseOut(buffer, token, sizeof(token));f=atoi(token);
		if (f < mod->mins[0]) mod->mins[0] = f;
		buffer = COM_ParseOut(buffer, token, sizeof(token));f=atoi(token);
		if (f < mod->mins[1]) mod->mins[1] = f;
		buffer = COM_ParseOut(buffer, token, sizeof(token));f=atoi(token);
		if (f < mod->mins[2]) mod->mins[2] = f;
		EXPECT(")");
		EXPECT("(");
		buffer = COM_ParseOut(buffer, token, sizeof(token));f=atoi(token);
		if (f > mod->maxs[0]) mod->maxs[0] = f;
		buffer = COM_ParseOut(buffer, token, sizeof(token));f=atoi(token);
		if (f > mod->maxs[1]) mod->maxs[1] = f;
		buffer = COM_ParseOut(buffer, token, sizeof(token));f=atoi(token);
		if (f > mod->maxs[2]) mod->maxs[2] = f;
		EXPECT(")");
	}
	EXPECT("}");

	EXPECT("baseframe");
	EXPECT("{");
	for (i = 0; i < numjoints; i++)
	{
		EXPECT("(");
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		baseframe[i*6+0] = atof(token);
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		baseframe[i*6+1] = atof(token);
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		baseframe[i*6+2] = atof(token);
		EXPECT(")");
		EXPECT("(");
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		baseframe[i*6+3] = atof(token);
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		baseframe[i*6+4] = atof(token);
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		baseframe[i*6+5] = atof(token);
		EXPECT(")");
	}
	EXPECT("}");

	for (i = 0; i < numframes; i++)
	{
		EXPECT("frame");
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		if (atoi(token) != i)
			MD5ERROR1PARAM("MD5ANIM: expected frame %i", i);
		EXPECT("{");
		for (j = 0; j < numanimatedparts; j++)
		{
			buffer = COM_ParseOut(buffer, token, sizeof(token));
			animatedcomponents[j] = atof(token);
		}
		EXPECT("}");

		for (j = 0; j < numjoints; j++)
		{
			fac = firstanimatedcomponents[j];
			flags = boneflags[j];

			if (flags&1)
				tx = animatedcomponents[fac++];
			else
				tx = baseframe[j*6+0];
			if (flags&2)
				ty = animatedcomponents[fac++];
			else
				ty = baseframe[j*6+1];
			if (flags&4)
				tz = animatedcomponents[fac++];
			else
				tz = baseframe[j*6+2];
			if (flags&8)
				qx = animatedcomponents[fac++];
			else
				qx = baseframe[j*6+3];
			if (flags&16)
				qy = animatedcomponents[fac++];
			else
				qy = baseframe[j*6+4];
			if (flags&32)
				qz = animatedcomponents[fac++];
			else
				qz = baseframe[j*6+5];

			GenMatrix(tx, ty, tz, qx, qy, qz, posedata+12*(j+numjoints*i));
		}
	}

	BZ_Free(firstanimatedcomponents);
	BZ_Free(animatedcomponents);
	BZ_Free(boneflags);
	BZ_Free(baseframe);

	memset(&grp, 0, sizeof(grp));
	Q_strncpyz(grp.name, "", sizeof(grp.name));
	grp.skeltype = SKEL_RELATIVE;
	grp.numposes = numframes;
	grp.rate = framespersecond;
	grp.loop = true;
	grp.boneofs = *poseofs;

	*gat = grp;
	return true;
#undef MD5ERROR0PARAM
#undef MD5ERROR1PARAM
#undef EXPECT
}

static galiasinfo_t *Mod_ParseMD5MeshModel(model_t *mod, char *buffer, char *modname)
{
#define MD5ERROR0PARAM(x) do{ Con_Printf(CON_ERROR x "\n"); return NULL; }while(0)
#define MD5ERROR1PARAM(x, y) do{ Con_Printf(CON_ERROR x "\n", y); return NULL; }while(0)
#define EXPECT(x) do{buffer = COM_ParseOut(buffer, token, sizeof(token)); if (strcmp(token, x)) Sys_Error("MD5MESH: expected %s", x);}while(0)
	int numjoints = 0;
	int nummeshes = 0;
	qboolean foundjoints = false;
	int i;

	galiasbone_t *bones = NULL;
	galiasanimation_t *pose = NULL;
	galiasinfo_t *inf, *root, *lastsurf;
	float *posedata;
#ifndef SERVERONLY
	galiasskin_t *skin;
	skinframe_t *frames;
#endif
	char *filestart = buffer;
	char token[1024];

	float x, y, z, qx, qy, qz;

	buffer = COM_ParseOut(buffer, token, sizeof(token));
	if (strcmp(token, "MD5Version"))
		MD5ERROR0PARAM("MD5 model without MD5Version identifier first");

	buffer = COM_ParseOut(buffer, token, sizeof(token));
	if (atoi(token) != 10)
		MD5ERROR0PARAM("MD5 model with unsupported MD5Version");


	root = ZG_Malloc(&mod->memgroup, sizeof(galiasinfo_t));
	lastsurf = NULL;

	for(;;)
	{
		buffer = COM_ParseOut(buffer, token, sizeof(token));
		if (!buffer)
			break;

		if (!strcmp(token, "numFrames"))
		{
			void *poseofs;
			galiasanimation_t *grp = ZG_Malloc(&mod->memgroup, sizeof(galiasanimation_t));
			Mod_ParseMD5Anim(mod, filestart, root, &poseofs, grp);
			root->ofsanimations = grp;
			root->numanimations = 1;
			grp->poseofs = poseofs;
			return root;
		}
		else if (!strcmp(token, "commandline"))
		{	//we don't need this
			buffer = strchr(buffer, '\"');
			buffer = strchr((char*)buffer+1, '\"')+1;
//			buffer = COM_Parse(buffer);
		}
		else if (!strcmp(token, "numJoints"))
		{
			if (numjoints)
				MD5ERROR0PARAM("MD5MESH: numJoints was already declared");
			buffer = COM_ParseOut(buffer, token, sizeof(token));
			numjoints = atoi(token);
			if (numjoints <= 0)
				MD5ERROR0PARAM("MD5MESH: Needs some joints");
		}
		else if (!strcmp(token, "numMeshes"))
		{
			if (nummeshes)
				MD5ERROR0PARAM("MD5MESH: numMeshes was already declared");
			buffer = COM_ParseOut(buffer, token, sizeof(token));
			nummeshes = atoi(token);
			if (nummeshes <= 0)
				MD5ERROR0PARAM("MD5MESH: Needs some meshes");
		}
		else if (!strcmp(token, "joints"))
		{
			if (foundjoints)
				MD5ERROR0PARAM("MD5MESH: Duplicate joints section");
			foundjoints=true;
			if (!numjoints)
				MD5ERROR0PARAM("MD5MESH: joints section before (or without) numjoints");

			bones = ZG_Malloc(&mod->memgroup, sizeof(*bones) * numjoints);
			pose = ZG_Malloc(&mod->memgroup, sizeof(galiasanimation_t));
			posedata = ZG_Malloc(&mod->memgroup, sizeof(float)*12 * numjoints);
			pose->skeltype = SKEL_ABSOLUTE;
			pose->rate = 1;
			pose->numposes = 1;
			pose->boneofs = posedata;

			root->baseframeofs = posedata;

			Q_strncpyz(pose->name, "base", sizeof(pose->name));

			EXPECT("{");
			//"name" parent (x y z) (s t u)
			//stu are a normalized quaternion, which we will convert to a 3*4 matrix for no apparent reason

			for (i = 0; i < numjoints; i++)
			{
				buffer = COM_ParseOut(buffer, token, sizeof(token));
				Q_strncpyz(bones[i].name, token, sizeof(bones[i].name));
				buffer = COM_ParseOut(buffer, token, sizeof(token));
				bones[i].parent = atoi(token);
				if (bones[i].parent >= i)
					MD5ERROR0PARAM("MD5MESH: joints parent's must be lower");
				if ((bones[i].parent < 0 && i) || (!i && bones[i].parent!=-1))
					MD5ERROR0PARAM("MD5MESH: Only the root joint may have a negative parent");

				EXPECT("(");
				buffer = COM_ParseOut(buffer, token, sizeof(token));
				x = atof(token);
				buffer = COM_ParseOut(buffer, token, sizeof(token));
				y = atof(token);
				buffer = COM_ParseOut(buffer, token, sizeof(token));
				z = atof(token);
				EXPECT(")");
				EXPECT("(");
				buffer = COM_ParseOut(buffer, token, sizeof(token));
				qx = atof(token);
				buffer = COM_ParseOut(buffer, token, sizeof(token));
				qy = atof(token);
				buffer = COM_ParseOut(buffer, token, sizeof(token));
				qz = atof(token);
				EXPECT(")");
				GenMatrix(x, y, z, qx, qy, qz, posedata+i*12);
			}
			EXPECT("}");
		}
		else if (!strcmp(token, "mesh"))
		{
			int numverts = 0;
			int numweights = 0;
			int numtris = 0;

			int num;
			int vnum;

			int numusableweights = 0;
			int *firstweightlist = NULL;
			int *numweightslist = NULL;

			galisskeletaltransforms_t *trans;
#ifdef HAVE_CLIENT
			float *stcoord = NULL;
			unsigned int numskins;
#endif
			index_t *indexes = NULL;
			float w;

			vec4_t *rawweight = NULL;
			int *rawweightbone = NULL;


			if (!nummeshes)
				MD5ERROR0PARAM("MD5MESH: mesh section before (or without) nummeshes");
			if (!foundjoints || !bones || !pose)
				MD5ERROR0PARAM("MD5MESH: mesh must come after joints");

			if (!lastsurf)
			{
				lastsurf = root;
				inf = root;
			}
			else
			{
				inf = ZG_Malloc(&mod->memgroup, sizeof(*inf));
				inf->shares_verts = lastsurf->shares_verts+1;
				lastsurf->nextsurf = inf;
				lastsurf = inf;
			}
			Mod_DefaultMesh(inf, COM_SkipPath(mod->name), 0);

			inf->shares_bones = 0;
			inf->ofsbones = bones;
			inf->numbones = numjoints;
			inf->numanimations = 1;
			inf->ofsanimations = pose;
			inf->baseframeofs = pose->boneofs;

#ifdef HAVE_CLIENT
			skin = ZG_Malloc(&mod->memgroup, sizeof(*skin));
			frames = ZG_Malloc(&mod->memgroup, sizeof(*frames));
			inf->numskins = 1;
			inf->ofsskins = skin;
			skin->numframes = 1;
			skin->skinspeed = 1;
			skin->frame = frames;
			numskins = 0;
#endif
			EXPECT("{");
			for(;;)
			{
				buffer = COM_ParseOut(buffer, token, sizeof(token));
				if (!buffer)
					MD5ERROR0PARAM("MD5MESH: unexpected eof");

				if (!strcmp(token, "shader"))
				{
					buffer = COM_ParseOut(buffer, token, sizeof(token));
#ifdef HAVE_CLIENT
					if (!strchr(token, '/'))
					{	//hack to try to deal with the rerelease using md5s for things that don't actually support skins properly.
						char texbase[MAX_QPATH];
						char texname[MAX_QPATH];
						Q_strncpyz(texbase, mod->name, sizeof(frames[0].shadername));
						*COM_SkipPath(texbase) = 0;
						Q_strncatz(texbase, token, sizeof(texbase));
						for (numskins = 0; ; numskins++)
						{
							Q_snprintfz(texname, sizeof(texname), "%s_%02d_%02d.lmp", texbase, numskins, 0);
							if (!COM_FCheckExists(texname))
								break;
						}
						if (numskins)
						{
							skin = ZG_Malloc(&mod->memgroup, sizeof(*skin)*numskins);
							inf->numskins = numskins;
							inf->ofsskins = skin;
							for (num = 0; num < numskins; num++, skin++)
							{
								skin->skinspeed = 5;	//match bsp anim speed.
								for (skin->numframes = 1; ; skin->numframes++)
								{
									Q_snprintfz(texname, sizeof(texname), "%s_%02d_%02d.lmp", texbase, num, skin->numframes);
									if (!COM_FCheckExists(texname))
										break;
								}

								frames = ZG_Malloc(&mod->memgroup, sizeof(*frames)*skin->numframes);
								skin->frame = frames;
								for (vnum = 0; vnum < skin->numframes; vnum++)
								{
									size_t fsize=0, w, h;
									qbyte *img;
									Q_snprintfz(frames[vnum].shadername, sizeof(frames[vnum].shadername), "%s_%02d_%02d.lmp", texbase, num, vnum);

									//extra stuff to make sure we can colourmap the 8bit data without needing _upper etc images.
									img = FS_LoadMallocGroupFile(&mod->memgroup, frames[vnum].shadername, &fsize, false);
									if (img && fsize >= 8)
									{
										w = (img[0]<<0)|(img[1]<<8)|(img[2]<<16)|(img[3]<<24);
										h = (img[4]<<0)|(img[5]<<8)|(img[6]<<16)|(img[7]<<24);
										if (fsize == 8+w*h && (vnum == 0 || (w==skin->skinwidth&&h==skin->skinheight)))
										{
											skin->skinwidth = w;
											skin->skinheight = h;
											frames[vnum].texels = img+8;
										}
										else
											ZG_Free(&mod->memgroup, img);	//something's screwy, don't leave the wasted memory lying around.
									}
								}
							}
						}
					}
#ifdef MD2MODELS
					if (!numskins && strcmp(mod->publicname, mod->name))
					{	//try grabbing the names from an original md2 file.
						size_t sz;
						md2_t *md2 = FS_LoadMallocFile(mod->publicname, &sz);
						if (md2)
						{
							if (!memcmp(md2, MD2IDALIASHEADER) && md2->version == LittleLong(MD2ALIAS_VERSION))
							{
								char *p;
								const char *str = (const char*)md2 + LittleLong(md2->ofs_skins);
								numskins = LittleLong(md2->num_skins);
								if (str + numskins*MD2MAX_SKINNAME>(char*)md2+sz)
									numskins = 0;

								skin = ZG_Malloc(&mod->memgroup, sizeof(*skin)*numskins);
								inf->numskins = numskins;
								inf->ofsskins = skin;
								for (num = 0; num < numskins; num++, skin++, str += MD2MAX_SKINNAME)
								{
									skin->skinspeed = 5;	//match bsp anim speed.
									skin->numframes = 1;

									frames = ZG_Malloc(&mod->memgroup, sizeof(*frames)*skin->numframes);
									skin->frame = frames;

									p = COM_SkipPath(mod->publicname);
									if (!strncmp(str, mod->publicname, p-mod->publicname))
									{
										Q_snprintfz(frames[0].shadername, sizeof(frames[0].shadername), "%s", mod->name);
										*COM_SkipPath(frames[0].shadername) = 0;
										Q_strncatz(frames[0].shadername, str+(p-mod->publicname), sizeof(frames[0].shadername));
									}
									else
										Q_snprintfz(frames[0].shadername, sizeof(frames[0].shadername), "%s", str);
								}
							}
							FS_FreeFile(md2);
						}
					}
#endif
					if (!numskins)
						//FIXME: we probably want to support multiple skins some time
						Q_strncpyz(frames[0].shadername, token, sizeof(frames[0].shadername));
#endif
				}
				else if (!strcmp(token, "numverts"))
				{
					if (numverts)
						MD5ERROR0PARAM("MD5MESH: numverts was already specified");
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					numverts = atoi(token);
					if (numverts < 0)
						MD5ERROR0PARAM("MD5MESH: numverts cannot be negative");

					firstweightlist = Z_Malloc(sizeof(*firstweightlist) * numverts);
					numweightslist = Z_Malloc(sizeof(*numweightslist) * numverts);
#ifdef HAVE_CLIENT
					stcoord = ZG_Malloc(&mod->memgroup, sizeof(float)*2*numverts);
					inf->ofs_st_array = (vec2_t*)stcoord;
					inf->numverts = numverts;
#endif
				}
				else if (!strcmp(token, "vert"))
				{	//vert num ( s t ) firstweight numweights

					buffer = COM_ParseOut(buffer, token, sizeof(token));
					num = atoi(token);
					if (num < 0 || num >= numverts)
						MD5ERROR0PARAM("MD5MESH: vertex out of range");

					EXPECT("(");
					buffer = COM_ParseOut(buffer, token, sizeof(token));
#ifdef HAVE_CLIENT
					if (!stcoord)
						MD5ERROR0PARAM("MD5MESH: vertex out of range");
					stcoord[num*2+0] = atof(token);
#endif
					buffer = COM_ParseOut(buffer, token, sizeof(token));
#ifdef HAVE_CLIENT
					stcoord[num*2+1] = atof(token);
#endif
					EXPECT(")");
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					firstweightlist[num] = atoi(token);
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					numweightslist[num] = atoi(token);

					numusableweights += numweightslist[num];
				}
				else if (!strcmp(token, "numtris"))
				{
					if (numtris)
						MD5ERROR0PARAM("MD5MESH: numtris was already specified");
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					numtris = atoi(token);
					if (numtris < 0)
						MD5ERROR0PARAM("MD5MESH: numverts cannot be negative");

					indexes = ZG_Malloc(&mod->memgroup, sizeof(int)*3*numtris);
					inf->ofs_indexes = indexes;
					inf->numindexes = numtris*3;
				}
				else if (!strcmp(token, "tri"))
				{
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					num = atoi(token);
					if (num < 0 || num >= numtris)
						MD5ERROR0PARAM("MD5MESH: vertex out of range");

					buffer = COM_ParseOut(buffer, token, sizeof(token));
					indexes[num*3+0] = atoi(token);
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					indexes[num*3+1] = atoi(token);
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					indexes[num*3+2] = atoi(token);
				}
				else if (!strcmp(token, "numweights"))
				{
					if (numweights)
						MD5ERROR0PARAM("MD5MESH: numweights was already specified");
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					numweights = atoi(token);

					rawweight = Z_Malloc(sizeof(*rawweight)*numweights);
					rawweightbone = Z_Malloc(sizeof(*rawweightbone)*numweights);
				}
				else if (!strcmp(token, "weight"))
				{
					//weight num bone scale ( x y z )
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					num = atoi(token);
					if (num < 0 || num >= numweights)
						MD5ERROR0PARAM("MD5MESH: weight out of range");

					buffer = COM_ParseOut(buffer, token, sizeof(token));
					rawweightbone[num] = atoi(token);
					if (rawweightbone[num] < 0 || rawweightbone[num] >= numjoints)
						MD5ERROR0PARAM("MD5MESH: weight specifies bad bone");
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					w = atof(token);

					EXPECT("(");
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					rawweight[num][0] = w*atof(token);
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					rawweight[num][1] = w*atof(token);
					buffer = COM_ParseOut(buffer, token, sizeof(token));
					rawweight[num][2] = w*atof(token);
					EXPECT(")");
					rawweight[num][3] = w;
				}
				else if (!strcmp(token, "}"))
					break;
				else
					MD5ERROR1PARAM("MD5MESH: Unrecognised token inside mesh (%s)", token);

			}

			trans = Z_Malloc(sizeof(*trans)*numusableweights);

			for (num = 0, vnum = 0; num < numverts; num++)
			{
				if (numweightslist[num] <= 0)
					MD5ERROR0PARAM("MD5MESH: weights not set on vertex");
				while(numweightslist[num])
				{
					trans[vnum].vertexindex = num;
					trans[vnum].boneindex = rawweightbone[firstweightlist[num]];
					trans[vnum].org[0] = rawweight[firstweightlist[num]][0];
					trans[vnum].org[1] = rawweight[firstweightlist[num]][1];
					trans[vnum].org[2] = rawweight[firstweightlist[num]][2];
					trans[vnum].org[3] = rawweight[firstweightlist[num]][3];
					vnum++;
					firstweightlist[num]++;
					numweightslist[num]--;
				}
			}

			Alias_BuildGPUWeights(mod, inf, vnum, trans, true);
			Z_Free(trans);


			for (i = 0; i < inf->numverts; i++)
				AddPointToBounds(inf->ofs_skel_xyz[i], mod->mins, mod->maxs);

			if (firstweightlist)
				Z_Free(firstweightlist);
			if (numweightslist)
				Z_Free(numweightslist);
			if (rawweight)
				Z_Free(rawweight);
			if (rawweightbone)
				Z_Free(rawweightbone);
		}
		else
			MD5ERROR1PARAM("Unrecognised token in MD5 model (%s)", token);
	}

	if (!lastsurf)
		MD5ERROR0PARAM("MD5MESH: No meshes");

//	Alias_CalculateSkeletalNormals(root);

	return root;
#undef MD5ERROR0PARAM
#undef MD5ERROR1PARAM
#undef EXPECT
}

static qboolean QDECL Mod_LoadMD5MeshModel(model_t *mod, void *buffer, size_t fsize)
{
	galiasinfo_t *root;
	char animname[MAX_QPATH];
	void *animfile;

	root = Mod_ParseMD5MeshModel(mod, buffer, mod->name);
	if (root == NULL)
	{
		return false;
	}

	if (mod_md5_singleanimation.ival && root->numanimations==1 && root->ofsanimations[0].skeltype==SKEL_ABSOLUTE)	//make sure there's only the base pose...
	{
		COM_StripAllExtensions(mod->name, animname, sizeof(animname));
		Q_strncatz(animname, ".md5anim", sizeof(animname));
		animfile = FS_LoadMallocFile(animname, NULL);
		if (animfile)	//FIXME: make non fatal somehow..
		{
			galiasinfo_t *surf;
			void *np = NULL;
			galiasanimation_t ng;
			if (Mod_ParseMD5Anim(mod, animfile, root, &np, &ng) && ng.numposes>0)
			{
				galiasanimation_t *a;
				int i;
				root->numanimations = ng.numposes;
				root->ofsanimations = ZG_Malloc(&mod->memgroup, ng.numposes*sizeof(*root->ofsanimations));

				//pull out each frame individually
				for (i = 0; i < ng.numposes; i++)
				{
					a = &root->ofsanimations[i];
					a->skeltype = ng.skeltype;
					a->loop = false;
					a->numposes = 1;
					a->rate = 10;
					a->boneofs = (float*)np + i*12*root->numbones;
					Q_snprintfz(a->name, sizeof(a->name), "%s_%i", animname, i);
				}

				for(surf = root;(surf = surf->nextsurf);)
				{
					surf->ofsanimations = root->ofsanimations;
					surf->numanimations = root->numanimations;
				}
			}

			Z_Free(animfile);
		}
	}

	mod->flags = Mod_ReadFlagsFromMD1(mod->name, 0);	//file replacement - inherit flags from any defunc mdl files.

	Mod_ClampModelSize(mod);
	Mod_ParseModelEvents(mod, root->ofsanimations, root->numanimations);

	mod->type = mod_alias;
	mod->numframes = root->numanimations;
	mod->meshinfo = root;

	mod->funcs.NativeTrace = Mod_Trace;
	return true;
}

/*
EXTERNALANIM

//File that specifies md5 model/anim stuff.

model test/imp.md5mesh

group test/idle1.md5anim
clampgroup test/idle1.md5anim
frames test/idle1.md5anim

*/
static qboolean QDECL Mod_LoadCompositeAnim(model_t *mod, void *buffer, size_t fsize)
{
	int i;

	char *file;
	galiasinfo_t *root = NULL, *surf;
	int numgroups = 0;
	galiasanimation_t *grouplist = NULL;
	galiasanimation_t *newgroup = NULL;
	float **poseofs;
	char com_token[8192];

	buffer = COM_Parse(buffer);
	if (strcmp(com_token, "EXTERNALANIM"))
	{
		Con_Printf (CON_ERROR "EXTERNALANIM: header is not compleate (%s)\n", mod->name);
		return false;
	}

	buffer = COM_Parse(buffer);
	if (!strcmp(com_token, "model"))
	{
		buffer = COM_Parse(buffer);
		file = COM_LoadTempMoreFile(com_token, NULL);

		if (!file)	//FIXME: make non fatal somehow..
		{
			Con_Printf(CON_ERROR "Couldn't open %s (from %s)\n", com_token, mod->name);
			return false;
		}

		root = Mod_ParseMD5MeshModel(mod, file, mod->name);
		if (root == NULL)
		{
			return false;
		}
		newgroup = root->ofsanimations;

		grouplist = BZ_Malloc(sizeof(galiasanimation_t)*(numgroups+root->numanimations));
		memcpy(grouplist, newgroup, sizeof(galiasanimation_t)*(numgroups+root->numanimations));
		poseofs = BZ_Malloc(sizeof(galiasanimation_t)*(numgroups+root->numanimations));
		for (i = 0; i < root->numanimations; i++)
		{
			grouplist[numgroups] = newgroup[i];
			poseofs[numgroups] = newgroup[i].boneofs;
			numgroups++;
		}
	}
	else
	{
		Con_Printf (CON_ERROR "EXTERNALANIM: model must be defined immediatly after the header\n");
		return false;
	}

	for (;;)
	{
		buffer = COM_Parse(buffer);
		if (!buffer)
			break;

		if (!strcmp(com_token, "group"))
		{
			grouplist = BZ_Realloc(grouplist, sizeof(galiasanimation_t)*(numgroups+1));
			poseofs = BZ_Realloc(poseofs, sizeof(*poseofs)*(numgroups+1));
			buffer = COM_Parse(buffer);
			file = COM_LoadTempMoreFile(com_token, NULL);
			if (file)	//FIXME: make non fatal somehow..
			{
				char namebkup[MAX_QPATH];
				Q_strncpyz(namebkup, com_token, sizeof(namebkup));
				if (!Mod_ParseMD5Anim(mod, file, root, (void**)&poseofs[numgroups], &grouplist[numgroups]))
				{
					return false;
				}
				Q_strncpyz(grouplist[numgroups].name, namebkup, sizeof(grouplist[numgroups].name));
				numgroups++;
			}
		}
		else if (!strcmp(com_token, "clampgroup"))
		{
			grouplist = BZ_Realloc(grouplist, sizeof(galiasanimation_t)*(numgroups+1));
			poseofs = BZ_Realloc(poseofs, sizeof(*poseofs)*(numgroups+1));
			buffer = COM_Parse(buffer);
			file = COM_LoadTempMoreFile(com_token, NULL);
			if (file)	//FIXME: make non fatal somehow..
			{
				char namebkup[MAX_QPATH];
				Q_strncpyz(namebkup, com_token, sizeof(namebkup));
				if (!Mod_ParseMD5Anim(mod, file, root, (void**)&poseofs[numgroups], &grouplist[numgroups]))
				{
					return false;
				}
				Q_strncpyz(grouplist[numgroups].name, namebkup, sizeof(grouplist[numgroups].name));
				grouplist[numgroups].loop = false;
				numgroups++;
			}
		}
		else if (!strcmp(com_token, "frames"))
		{
			galiasanimation_t ng;
			void *np;

			buffer = COM_Parse(buffer);
			file = COM_LoadTempMoreFile(com_token, NULL);
			if (file)	//FIXME: make non fatal somehow..
			{
				char namebkup[MAX_QPATH];
				Q_strncpyz(namebkup, com_token, sizeof(namebkup));
				if (!Mod_ParseMD5Anim(mod, file, root, &np, &ng))
				{
					return false;
				}

				grouplist = BZ_Realloc(grouplist, sizeof(galiasanimation_t)*(numgroups+ng.numposes));
				poseofs = BZ_Realloc(poseofs, sizeof(*poseofs)*(numgroups+ng.numposes));

				//pull out each frame individually
				for (i = 0; i < ng.numposes; i++)
				{
					grouplist[numgroups].skeltype = ng.skeltype;
					grouplist[numgroups].loop = false;
					grouplist[numgroups].numposes = 1;
					grouplist[numgroups].rate = 24;
					poseofs[numgroups] = (float*)np + i*12*root->numbones;
					Q_snprintfz(grouplist[numgroups].name, sizeof(grouplist[numgroups].name), "%s%i", namebkup, i);
					Q_strncpyz(grouplist[numgroups].name, namebkup, sizeof(grouplist[numgroups].name));
					grouplist[numgroups].loop = false;
					numgroups++;
				}
			}
		}
		else
		{
			Con_Printf(CON_ERROR "EXTERNALANIM: unrecognised token (%s)\n", mod->name);
			return false;
		}
	}

	newgroup = grouplist;
	grouplist = ZG_Malloc(&mod->memgroup, sizeof(galiasanimation_t)*numgroups);
	for(surf = root;;)
	{
		surf->ofsanimations = grouplist;
		surf->numanimations = numgroups;
		if (!surf->nextsurf)
			break;
		surf = surf->nextsurf;
	}
	for (i = 0; i < numgroups; i++)
	{
		grouplist[i] = newgroup[i];
		grouplist[i].boneofs = poseofs[i];
	}

	mod->flags = Mod_ReadFlagsFromMD1(mod->name, 0);	//file replacement - inherit flags from any defunc mdl files.

	Mod_ClampModelSize(mod);
	Mod_ParseModelEvents(mod, root->ofsanimations, root->numanimations);

	mod->type = mod_alias;
	mod->numframes = root->numanimations;
	mod->meshinfo = root;

	mod->funcs.NativeTrace = Mod_Trace;
	return true;
}

#endif //MD5MODELS

#ifdef MODELFMT_OBJ
#include <ctype.h>
struct objvert { size_t attrib[3]; };

struct objbuf_s { char *ptr; char *end; };
static char *obj_getline(struct objbuf_s *vf, char *buffer, size_t buflen)
{
	char in;
	char *out = buffer;
	size_t len;
	if (buflen <= 1)
		return NULL;
	len = buflen-1;
	while (len > 0)
	{
		if (vf->ptr == vf->end)
		{
			if (len == buflen-1)
				return NULL;
			*out = '\0';
			return buffer;
		}
		in = *vf->ptr++;
		if (in == '\n')
			break;
		*out++ = in;
		len--;
	}
	*out = '\0';

	//if there's a trailing \r, strip it.
	if (out > buffer)
		if (out[-1] == '\r')
			out[-1] = 0;

	return buffer;
}

struct objattrib_s {
	size_t length;
	size_t maxlength;
	float *data;
};
static qboolean parseobjvert(char *s, struct objattrib_s *out, int reorient)
{
	int i;
	float *v;
	char *n;
	if (out->length == out->maxlength)
		Z_ReallocElements((void**)&out->data,&out->maxlength,out->length+1024,3*sizeof(float));
	v = out->data+out->length*3;
	out->length++;
	while(isalpha(*s)) s++;
	for (i = 0; i < 3;)
	{
		v[i] = strtod(s, &n);
		if (n==s)
			return false;
		s = n;
		while(isspace(*s)) s++;
		i++;
		if(!*s) break;
	}
	for (; i < 3; i++)
		v[i] = 0;

	if (reorient)
	{
		if (reorient == 1)
		{	//xzy - DP's orientation.
			float z = v[2];
			v[2] = v[1];
			v[1] = z;
		}
		else
		{	//zxy - match iqmtool (doesn't need negative scaling)
			float z = v[2];
			v[2] = v[1];
			v[1] = v[0];
			v[0] = z;
		}
	}
	return true;
}

static galiasinfo_t *Obj_FinishFace(model_t *mod, galiasinfo_t *m, struct objattrib_s *attribs, struct objvert *vert, size_t numverts, index_t *indexes, size_t *numelements)
{	//this is really lame. not optimised and I don't care.
	qboolean calcnorms = false;
	size_t i;
	if (m && numverts < MAX_INDICIES)
	{
		m->ofs_skel_xyz = ZG_Malloc(&mod->memgroup, sizeof(*m->ofs_skel_xyz)*numverts);
		m->ofs_st_array = ZG_Malloc(&mod->memgroup, sizeof(*m->ofs_st_array)*numverts);
		m->ofs_skel_norm = ZG_Malloc(&mod->memgroup, sizeof(*m->ofs_skel_norm)*numverts);
		m->ofs_skel_svect = ZG_Malloc(&mod->memgroup, sizeof(*m->ofs_skel_svect)*numverts);
		m->ofs_skel_tvect = ZG_Malloc(&mod->memgroup, sizeof(*m->ofs_skel_tvect)*numverts);

		for (i = 0; i < numverts; i++)
		{
			if (vert[i].attrib[0] >= attribs[0].length)
				VectorClear(m->ofs_skel_xyz[i]);
			else
				VectorCopy(attribs[0].data+3*vert[i].attrib[0], m->ofs_skel_xyz[i]);
			AddPointToBounds(m->ofs_skel_xyz[i], mod->mins, mod->maxs);

			if (vert[i].attrib[1] >= attribs[1].length)
				Vector2Clear(m->ofs_st_array[i]);
			else
				Vector2Copy(attribs[1].data+3*vert[i].attrib[1], m->ofs_st_array[i]);
			m->ofs_st_array[i][1] = 1-m->ofs_st_array[i][1]; //flip y coords.

			if (vert[i].attrib[2] >= attribs[2].length)
			{
				VectorClear(m->ofs_skel_norm[i]);
				calcnorms = true;
			}
			else
				VectorCopy(attribs[2].data+3*vert[i].attrib[2], m->ofs_skel_norm[i]);
		}
		m->numverts = i;

		m->ofs_indexes = ZG_Malloc(&mod->memgroup, sizeof(*m->ofs_indexes)**numelements);
		memcpy(m->ofs_indexes, indexes, sizeof(*m->ofs_indexes)**numelements);
		m->numindexes = *numelements;

		//calc tangents.
		Mod_AccumulateTextureVectors(m->ofs_skel_xyz, m->ofs_st_array, m->ofs_skel_norm, m->ofs_skel_svect, m->ofs_skel_tvect, m->ofs_indexes, m->numindexes, calcnorms);
		Mod_NormaliseTextureVectors(m->ofs_skel_norm, m->ofs_skel_svect, m->ofs_skel_tvect, m->numverts, calcnorms);

		*numelements = 0;
	}
	return NULL;
}

static qboolean QDECL Mod_LoadObjModel(model_t *mod, void *buffer, size_t fsize)
{
	struct objbuf_s f = {buffer, (qbyte*)buffer+fsize};
	struct objattrib_s attrib[3] = {{0},{0},{0}};
	char buf[512];
	char *meshname = NULL, *matname = NULL;
	galiasinfo_t *m = NULL, **link = (galiasinfo_t**)&mod->meshinfo;

	size_t numverts = 0;
	size_t maxverts = 0;
	struct objvert *vert = NULL, defaultvert={{-1,-1,-2}};

	size_t numelems = 0;
	size_t maxelems = 0;
	index_t *elem = NULL;

	qboolean badinput = false;
	int meshidx = 0;
	int reorient = mod_obj_orientation.ival;

	ClearBounds(mod->mins, mod->maxs);

	while(!badinput && obj_getline(&f, buf, sizeof(buf)))
	{
		char *c = buf;
		while(isspace(*c)) c++;
		switch(*c)
		{
			case '#': continue;
			case 'v':
				if(isspace(c[1])) badinput |= !parseobjvert(c, &attrib[0], reorient);
				else if(c[1]=='t') badinput |= !parseobjvert(c, &attrib[1], false);
				else if(c[1]=='n') badinput |= !parseobjvert(c, &attrib[2], reorient);
				break;
			case 'g':
			{
				char *name;
				size_t namelen;
				while(isalpha(*c)) c++;
				while(isspace(*c)) c++;
				name = c;
				namelen = strlen(name);
				while(namelen > 0 && isspace(name[namelen-1])) namelen--;
				Z_Free(meshname);
				meshname = Z_Malloc(namelen+1);
				memcpy(meshname, name, namelen);
				meshname[namelen] = 0;

				m = Obj_FinishFace(mod, m, attrib, vert, numverts, elem, &numelems);
				break;
			}
			case 'u':
			{
				char *name;
				size_t namelen;
				if(strncmp(c, "usemtl", 6)) continue;
				while(isalpha(*c)) c++;
				while(isspace(*c)) c++;
				name = c;
				namelen = strlen(name);
				while(namelen > 0 && isspace(name[namelen-1])) namelen--;

				if (!matname || strncmp(matname, name, namelen)||matname[namelen])
				{
					Z_Free(matname);
					matname = Z_Malloc(namelen+1);
					memcpy(matname, name, namelen);
					matname[namelen] = 0;

					m = Obj_FinishFace(mod, m, attrib, vert, numverts, elem, &numelems);
				}
				break;
			}
			case 's':
			{
				if(!isspace(c[1])) continue;
				while(isalpha(*c)) c++;
				while(isspace(*c)) c++;

				//make sure that these verts are not merged, ensuring that they get smoothed.
				//different texture coords will still have discontinuities though.
				defaultvert.attrib[2] = -1-strtol(c, &c, 10);
				break;
			}
			case 'f':
			{
				size_t i, v = 0;
				struct objvert vkey={0};
				index_t first=0, prev=0, cur=0;

				//only generate a new mesh if something actually changed.
				if (!m)
				{
#ifdef HAVE_CLIENT
					galiasskin_t *skin;
					skinframe_t *sframe;
					m = ZG_Malloc(&mod->memgroup, sizeof(*m)+sizeof(*skin)+sizeof(*sframe));
#else
					m = ZG_Malloc(&mod->memgroup, sizeof(*m));
#endif
					*link = m;
					link = &m->nextsurf;
					m->shares_verts = meshidx;
					Mod_DefaultMesh(m, COM_SkipPath(mod->name), meshidx++);

					Q_strncpyz(m->surfacename, meshname?meshname:"", sizeof(m->surfacename));

#ifdef HAVE_CLIENT
					skin = (void*)(m+1);
					sframe = (void*)(skin+1);
					skin->frame = sframe;
					skin->numframes = 1;
					skin->skinspeed = 10;
					Q_strncpyz(skin->name, matname?matname:"", sizeof(skin->name));
					Q_strncpyz(sframe->shadername, matname?matname:"", sizeof(sframe->shadername));
					sframe->shader = NULL;

					m->ofsskins = skin;
					m->numskins = 1;
#endif
				}

				while(isalpha(*c)) c++;
				for(;;)
				{
					while(isspace(*c)) c++;
					if(!*c) break;

					for (i = 0; i < countof(vkey.attrib); )
					{
						char *n;
						long v;
						v = strtol(c, &n, 10);
						if (c == n) {badinput = true; break;} //not a number if we read nothing!
						if (v < 0)
							vkey.attrib[i] = attrib[i].length + v;
						else
							vkey.attrib[i] = v - 1; //0 is index-not-specified.
						i++;
						c = n;
						if(*c!='/') break;
						c++;
					}
					for (; i < countof(vkey.attrib); i++)
						vkey.attrib[i] = defaultvert.attrib[i];

					//figure out the verts, to avoid dupes
					for (cur = 0; cur < numverts; cur++)
					{
						if (vert[cur].attrib[0] == vkey.attrib[0] &&
							vert[cur].attrib[1] == vkey.attrib[1] &&
							vert[cur].attrib[2] == vkey.attrib[2] && vkey.attrib[2]!=-1)
							break;
					}
					if (cur == numverts)
					{
						if (numverts == maxverts)
							Z_ReallocElements((void**)&vert,&maxverts,numverts+1024,sizeof(*vert));
						vert[numverts++] = vkey;
					}

					//spew out the trifan
					if (v == 0)
						first = cur;
					else if (v > 1)
					{
						if (numelems+3 >= maxelems)
						{
							if (numelems >= 65535)
							{
								badinput = true;
								break;	//don't depend upon the OOM killer... it kills everything else too
							}
							Z_ReallocElements((void**)&elem,&maxelems,numelems+1024,sizeof(*elem));
						}
						if (reorient == 1)
						{
							elem[numelems++] = first;
							elem[numelems++] = prev;
							elem[numelems++] = cur;
						}
						else
						{
							elem[numelems++] = cur;
							elem[numelems++] = prev;
							elem[numelems++] = first;
						}
					}
					prev = cur;
					v++;
				}
				break;
			}
		}
	}
	m = Obj_FinishFace(mod, m, attrib, vert, numverts, elem, &numelems);

	Z_Free(vert);
	Z_Free(elem);
	Z_Free(attrib[0].data);
	Z_Free(attrib[1].data);
	Z_Free(attrib[2].data);

	if (badinput)
	{	//fail the load.
		Con_Printf(CON_WARNING "File \"%s\" with .obj extension does not appear to be an .obj file\n", mod->name);
		mod->meshinfo = NULL;
		return false;
	}

	Mod_ClampModelSize(mod);
	Mod_ParseModelEvents(mod, NULL, 0);
	mod->flags = 0;
	mod->type = mod_alias;
	mod->numframes = 0;
	Mod_SetMeshModelFuncs(mod, true);

//	Con_Printf(CON_WARNING "%s: multi-surface obj files are unoptimised\n", mod->name);
	return !!mod->meshinfo;
}
#endif


void Alias_Register(void)
{
#ifdef MD1MODELS
#ifndef SERVERONLY
	Cvar_Register(&dpcompat_nofloodfill, NULL);
#endif
	Mod_RegisterModelFormatMagic(NULL, "Quake1 Model (mdl)",				IDPOLYHEADER,							Mod_LoadQ1Model);
	Mod_RegisterModelFormatMagic(NULL, "QuakeForge 16bit Model",			"MD16",4,								Mod_LoadQ1Model);
#ifdef HEXEN2
	Mod_RegisterModelFormatMagic(NULL, "Hexen2 Model (mdl)",				RAPOLYHEADER,							Mod_LoadQ1Model);
#endif
#endif
#ifdef MD2MODELS
	Mod_RegisterModelFormatMagic(NULL, "Quake2 Model (md2)",				MD2IDALIASHEADER,						Mod_LoadQ2Model);
#endif
#ifdef MODELFMT_MDX
	Mod_RegisterModelFormatMagic(NULL, "Kingpin Model (mdx)",				MDX_IDENT,								Mod_LoadKingpinModel);
#endif
#ifdef MD3MODELS
	Mod_RegisterModelFormatMagic(NULL, "Quake3 Model (md3)",				MD3_IDENT,								Mod_LoadQ3Model);
#endif
#ifdef HALFLIFEMODELS
	Mod_RegisterModelFormatMagic(NULL, "Half-Life Model (mdl)",				"IDST\x0a\0\0\0",8,						Mod_LoadHLModel);
#endif

#ifdef ZYMOTICMODELS
	Mod_RegisterModelFormatMagic(NULL, "Zymotic Model (zym)",				"ZYMOTICMODEL",12,						Mod_LoadZymoticModel);
#endif
#ifdef DPMMODELS
	Mod_RegisterModelFormatMagic(NULL, "DarkPlaces Model (dpm)",			"DARKPLACESMODEL\0",16,					Mod_LoadDarkPlacesModel);
#endif
#ifdef PSKMODELS
	Mod_RegisterModelFormatMagic(NULL, "Unreal Interchange Model (psk)",	"ACTR",4,								Mod_LoadPSKModel);
#endif
#ifdef INTERQUAKEMODELS
	Mod_RegisterModelFormatMagic(NULL, "Inter-Quake Model (iqm)",			"INTERQUAKEMODEL\0",16,					Mod_LoadInterQuakeModel);
#endif
#ifdef MD5MODELS
	Cvar_Register(&mod_md5_singleanimation, NULL);
	Mod_RegisterModelFormatText(NULL, "MD5 Mesh/Anim (md5mesh)",			"MD5Version",							Mod_LoadMD5MeshModel);
	Mod_RegisterModelFormatText(NULL, "External Anim",						"EXTERNALANIM",							Mod_LoadCompositeAnim);
#endif


#ifdef MODELFMT_OBJ
	Mod_RegisterModelFormatText(NULL, "Wavefront Object (obj)",				".obj",									Mod_LoadObjModel);
	Cvar_Register(&mod_obj_orientation, NULL);
#endif

#ifndef SERVERONLY
	Cvar_Register(&dpcompat_skinfiles, NULL);
#endif
}
