#include "../plugin.h"
#include "../engine/common/com_mesh.h"
static plugfsfuncs_t *filefuncs;
static plugmodfuncs_t *modfuncs;

//Utility functions. silly plugins.
float Length(const vec3_t v) {return sqrt(DotProduct(v,v));}
float RadiusFromBounds (const vec3_t mins, const vec3_t maxs)
{
	int		i;
	vec3_t	corner;
	for (i=0 ; i<3 ; i++)
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	return Length(corner);
}



struct fstream_s
{
	void *start;
	size_t len;
	size_t ofs;

	size_t numbones;
	galiasbone_t *bones;
	float *basepose;
};
struct lod_s
{
	float coverage;
	const char *name;
	unsigned short numtex;
	const char *tex[64];
};

static qboolean ReadEOF(struct fstream_s *f)
{
	return (f->ofs >= f->len);
}
static qbyte ReadByte(struct fstream_s *f)
{
	if (f->ofs+1 > f->len)
	{
		f->ofs++;
		return 0;
	}
	return ((qbyte*)f->start)[f->ofs++];
}
static const char *ReadBytes(struct fstream_s *f, size_t len)
{
	f->ofs+=len;
	if (f->ofs > f->len)
		return NULL;
	return &((const char*)f->start)[f->ofs-len];
}
static const char *ReadString(struct fstream_s *f)
{
	size_t len;
	for (len = 0; f->ofs+len < f->len && ((qbyte*)f->start)[f->ofs+len]; len++)
		;
	len++; //for the null
	f->ofs += len;
	return &((const qbyte*)f->start)[f->ofs-len];
}
static unsigned short ReadUInt16(struct fstream_s *f)
{
	unsigned short r;
	r = ReadByte(f);
	r|= ReadByte(f) << 8;
	return r;
}
static short ReadSInt16(struct fstream_s *f)
{
	return (signed short)ReadUInt16(f);
}
static unsigned int ReadUInt32(struct fstream_s *f)
{
	unsigned int r;
	r = ReadByte(f);
	r|= ReadByte(f) << 8;
	r|= ReadByte(f) << 16;
	r|= ReadByte(f) << 24;
	return r;
}
static float ReadFloat(struct fstream_s *f)
{
	union {
		unsigned int u;
		float f;
	} r;
	r.u = ReadByte(f);
	r.u|= ReadByte(f) << 8;
	r.u|= ReadByte(f) << 16;
	r.u|= ReadByte(f) << 24;
	return r.f;
}
static qboolean Mod_XModel_LoadPart (struct model_s *mod, struct fstream_s *f)
{
	unsigned short ver = ReadUInt16(f);
	unsigned short b, nboner = ReadUInt16(f);
	unsigned short nbonea = ReadUInt16(f);
	float rel[12];
	switch(ver)
	{
	case 0x0e:
		break;
	case 0x14:
		break;
	default:
		Con_Printf(CON_ERROR"%s: Unknown version %#x\n", mod->name, ver);
		return false;
	}
//	Con_Printf("%s: version %x rb:%i ab:%i\n", mod->name, ver, nboner, nbonea);
	f->numbones = nbonea+nboner;
	f->basepose = plugfuncs->GMalloc(&mod->memgroup, sizeof(*f->basepose)*12 * f->numbones);
	f->bones = plugfuncs->GMalloc(&mod->memgroup, sizeof(*f->bones) * f->numbones);

	for (b = 0; b < nbonea; b++)
	{	//root bones, with identity position. for some reason.
		f->bones[b].parent = -1;

		VectorClear(f->bones[b].ref.org);
		Vector4Set(f->bones[b].ref.quat, 0, 0, 0, 1);
		VectorSet(f->bones[b].ref.scale, 1, 1, 1);
		modfuncs->GenMatrixPosQuat4Scale(f->bones[b].ref.org, f->bones[b].ref.quat, f->bones[b].ref.scale, f->basepose + b*12);
	}
	for (; b < f->numbones; b++)
	{
		f->bones[b].parent = ReadByte(f);

		if (f->bones[b].parent >= b)
			Con_Printf(CON_ERROR"b%i (%s) has parent %i\n", b, f->bones[b].name, f->bones[b].parent);

		f->bones[b].ref.org[0] = ReadFloat(f);
		f->bones[b].ref.org[1] = ReadFloat(f);
		f->bones[b].ref.org[2] = ReadFloat(f);

		f->bones[b].ref.quat[0] = ReadSInt16(f)/32767.0f;
		f->bones[b].ref.quat[1] = ReadSInt16(f)/32767.0f;
		f->bones[b].ref.quat[2] = ReadSInt16(f)/32767.0f;
		f->bones[b].ref.quat[3] = 1.0-DotProduct(f->bones[b].ref.quat,f->bones[b].ref.quat);	//reconstruct the w part.
		if (f->bones[b].ref.quat[3]>0)
			f->bones[b].ref.quat[3] = sqrt(f->bones[b].ref.quat[3]);
		else
			f->bones[b].ref.quat[3] = 0;

		VectorSet(f->bones[b].ref.scale, 1, 1, 1);

		modfuncs->GenMatrixPosQuat4Scale(f->bones[b].ref.org, f->bones[b].ref.quat, f->bones[b].ref.scale, rel);
		modfuncs->ConcatTransforms((void*)(f->basepose + f->bones[b].parent*12), (void*)rel, (void*)(f->basepose + b*12));

//		Con_Printf("b%i: p:%i, [%f %f %f] [%f %f %f %f]\n", b, f->bones[b].parent, pos[0],pos[1],pos[2], quat[0],quat[1],quat[2],quat[3]);
	}
	for (b = 0; b < f->numbones; b++)
	{
		vec3_t mins, maxs;
		const char *n = ReadString(f);
		if (ver >= 0x14)
		{	//omitted.
			VectorClear(mins);
			VectorClear(maxs);
		}
		else
		{
			mins[0] = ReadFloat(f);
			mins[1] = ReadFloat(f);
			mins[2] = ReadFloat(f);
			maxs[0] = ReadFloat(f);
			maxs[1] = ReadFloat(f);
			maxs[2] = ReadFloat(f);
		}

		//hack... I assume the (game)code does a skel_setbone so this doesn't actually matter?
		if (!strcmp(n, "torso_stabilizer"))
			Vector4Set(f->bones[b].ref.quat, 0,0,0,1);

		Q_strlcpy(f->bones[b].name, n, sizeof(f->bones[b].name));
		modfuncs->M3x4_Invert(f->basepose+12*b, f->bones[b].inverse);

//		if (ver >= 0x14)
//			Con_Printf("b%i: %-20s\n", b, n);
//		else
//			Con_Printf("b%i: %-20s [%f %f %f] [%f %f %f]\n", b, n, mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);
	}
	for (b = 0; b < f->numbones; b++)
		f->bones[b].group = ReadByte(f);	//presumed.
	return true;
}
static qboolean Mod_XModel_LoadSurfs (struct model_s *mod, struct fstream_s *f, struct lod_s *lod, float mincov)
{
	galiasinfo_t *surf;
	galiasskin_t *skins;
	skinframe_t *skinframe;
	unsigned short ver = ReadUInt16(f);
	unsigned short n, nsurfs = ReadUInt16(f);
	switch(ver)
	{
	case 0x0e:
		break;
	case 0x14:
		break;
	default:
		Con_Printf("%s: Unknown version %#x\n", mod->name, ver);
		return false;
	}

	if (!nsurfs)
		return true;	//nothing to do...

	surf = plugfuncs->GMalloc(&mod->memgroup,	sizeof(*surf)*nsurfs +
												sizeof(*skins)*nsurfs +
												sizeof(*skinframe)*nsurfs);
	skins = (galiasskin_t*)(surf+nsurfs);
	skinframe = (skinframe_t*)(skins+nsurfs);
	for (n = 0; n < nsurfs; n++)
	{
		surf[n].numbones = f->numbones;
		surf[n].ofsbones = f->bones;
		surf[n].baseframeofs = f->basepose;

		surf[n].mindist = mincov;
		surf[n].maxdist = lod->coverage;
		surf[n].shares_verts = n;
		surf[n].shares_bones = 0;
		surf[n].nextsurf = &surf[n+1];
		surf[n].ofsskins = &skins[n];
		surf[n].numskins = 1;
		skins[n].skinspeed = 0.1;
		skins[n].frame = &skinframe[n];
		skins[n].numframes = 1;
		Q_snprintf(surf[n].surfacename, sizeof(surf[n].surfacename), "%s/%i", lod->name, n);
		Q_strlcpy(skins[n].name, lod->name, sizeof(skins[n].name));
		if (n < lod->numtex)
			Q_snprintf(skinframe[n].shadername, sizeof(skinframe[n].shadername), (ver==0x0e)?"skins/%s":"%s", lod->tex[n]);
	}
	surf[nsurfs-1].nextsurf = mod->meshinfo;
	mod->meshinfo = surf;
	for(surf = surf[nsurfs-1].nextsurf; surf; surf = surf->nextsurf)
		surf->shares_verts += nsurfs;
	surf = mod->meshinfo;

	for (n = 0; n < nsurfs; n++, surf++)
	{
		int flags = ReadByte(f);
		int v, nverts = ReadUInt16(f);
		int t, ntris = ReadUInt16(f);
		int r, runs = (ver==0xe)?ReadUInt16(f):0;
		unsigned short wcount = 0;
		qbyte run;
		unsigned int idx1, idx2, idx3;

		unsigned short boneidx = ReadUInt16(f);
		qboolean boney = boneidx == (unsigned short)~0u;
		int exweights = 0;
		unsigned short *vw = NULL;
//		unsigned short bunk2;

		vec3_t norm;
		vec4_t xyzw;
		const float *matrix;

		(void)flags;

		if (boney)
		{
			if (ver == 0x0e)
				exweights = ReadUInt16(f);
			/*bunk2 =*/ ReadUInt16(f); //seems to be vaguely related to vert/triangle counts.
		}

		surf->numverts = nverts;

		if (ver == 0xe)
		{	//cod1
			surf->ofs_indexes		= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_indexes		) * ntris*3);
			for(t = 0, r = 0; r < runs; r++)
			{
				run = ReadByte(f);
				idx1 = ReadUInt16(f);
				idx2 = ReadUInt16(f);
				run-=2;

				for (;;)
				{	//strip
					if(!run--) break;
					idx3 = ReadUInt16(f);
					if (idx1 != idx2 && idx1 != idx3 && idx2 != idx3 && t < ntris)
					{
						surf->ofs_indexes[t*3+0] = idx1;
						surf->ofs_indexes[t*3+1] = idx2;
						surf->ofs_indexes[t*3+2] = idx3;
						t++;
					}
					idx1 = idx2;
					idx2 = idx3;

					//alternating triangles flip the order
					if(!run--) break;
					idx3 = ReadUInt16(f);
					if (idx1 != idx2 && idx1 != idx3 && idx2 != idx3 && t < ntris)
					{
						surf->ofs_indexes[t*3+2] = idx1;
						surf->ofs_indexes[t*3+1] = idx2;
						surf->ofs_indexes[t*3+0] = idx3;
						t++;
					}
					idx1 = idx2;
					idx2 = idx3;
				}
			}

			if (ntris != t)
			{
				Con_Printf(CON_ERROR"Expected %i tris, got %i from %i runs\n", ntris, t, r);
				return false;
			}
			surf->numindexes = t*3;

			//lazy and a bit slower.
			surf->ofs_st_array		= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_st_array		) * surf->numverts);
			surf->ofs_skel_xyz		= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_xyz		) * surf->numverts);
			surf->ofs_skel_norm		= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_norm	) * surf->numverts);
			if (boney || f->numbones>0)
			{
				surf->ofs_skel_idx		= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_idx		) * surf->numverts);
				surf->ofs_skel_weight	= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_weight	) * surf->numverts);
				vw = memset(alloca(sizeof(*vw)*nverts), 0, sizeof(*vw)*nverts);
			}
			boneidx = min(boneidx,f->numbones-1);

			for (v = 0; v < nverts; v++)
			{
				norm[0]	= ReadFloat(f);
				norm[1]	= ReadFloat(f);
				norm[2]	= ReadFloat(f);
				surf->ofs_st_array[v][0]	= ReadFloat(f);
				surf->ofs_st_array[v][1]	= ReadFloat(f);
				if (boney)
				{
					wcount = ReadUInt16(f);
					boneidx = ReadUInt16(f);
					boneidx = min(boneidx,f->numbones-1);
				}

				xyzw[0]	= ReadFloat(f);
				xyzw[1]	= ReadFloat(f);
				xyzw[2]	= ReadFloat(f);
				if (wcount)
				{
					xyzw[3] = ReadFloat(f);
					VectorScale(xyzw, xyzw[3], xyzw);
				}
				else
					xyzw[3]	= 1;

				if (surf->ofs_skel_idx)
				{
					vw[v] = wcount;	//urgh.
					surf->ofs_skel_idx[v][0] = surf->ofs_skel_idx[v][1] = surf->ofs_skel_idx[v][2] = surf->ofs_skel_idx[v][3] = boneidx;	//set all of them, cache might thank us.
					surf->ofs_skel_weight[v][0] = xyzw[3];
				}

				//calculate the correct position in the base pose
				matrix = f->basepose + boneidx*12;
				surf->ofs_skel_xyz[ v][0] = xyzw[0] * matrix[0] + xyzw[1] * matrix[1] + xyzw[2] * matrix[ 2] + xyzw[3] * matrix[ 3];
				surf->ofs_skel_xyz[ v][1] = xyzw[0] * matrix[4] + xyzw[1] * matrix[5] + xyzw[2] * matrix[ 6] + xyzw[3] * matrix[ 7];
				surf->ofs_skel_xyz[ v][2] = xyzw[0] * matrix[8] + xyzw[1] * matrix[9] + xyzw[2] * matrix[10] + xyzw[3] * matrix[11];
				surf->ofs_skel_norm[v][0] = norm[0] * matrix[0] + norm[1] * matrix[1] + norm[2] * matrix[ 2];
				surf->ofs_skel_norm[v][1] = norm[0] * matrix[4] + norm[1] * matrix[5] + norm[2] * matrix[ 6];
				surf->ofs_skel_norm[v][2] = norm[0] * matrix[8] + norm[1] * matrix[9] + norm[2] * matrix[10];
			}
			if (vw)
			{
				float lowestv;
				int lowesti, j;
				for (v = 0; v < nverts; v++)
				{
					for (; vw[v] > 0; vw[v]--)
					{
						if (--exweights < 0)
							break;
						boneidx = ReadUInt16(f);
						boneidx = min(boneidx,f->numbones-1);
						xyzw[0]	= ReadFloat(f);
						xyzw[1]	= ReadFloat(f);
						xyzw[2]	= ReadFloat(f);
						xyzw[3] = ReadFloat(f);
						VectorScale(xyzw, xyzw[3], xyzw);

						matrix = f->basepose + boneidx*12;
						surf->ofs_skel_xyz[ v][0] += xyzw[0] * matrix[0] + xyzw[1] * matrix[1] + xyzw[2] * matrix[ 2] + xyzw[3] * matrix[ 3];
						surf->ofs_skel_xyz[ v][1] += xyzw[0] * matrix[4] + xyzw[1] * matrix[5] + xyzw[2] * matrix[ 6] + xyzw[3] * matrix[ 7];
						surf->ofs_skel_xyz[ v][2] += xyzw[0] * matrix[8] + xyzw[1] * matrix[9] + xyzw[2] * matrix[10] + xyzw[3] * matrix[11];

						lowesti = 0;
						lowestv = surf->ofs_skel_weight[v][0];
						for (j = 1; j < countof(surf->ofs_skel_idx[v]); j++)
						{
							if (surf->ofs_skel_weight[v][j] < lowestv)
								lowestv = surf->ofs_skel_weight[v][lowesti=j];
						}
						if (lowestv < xyzw[3])
						{
							surf->ofs_skel_idx[v][lowesti] = boneidx;
							surf->ofs_skel_weight[v][lowesti] = xyzw[3];
						}
					}
					//compensate for any missing weights
					xyzw[3] = surf->ofs_skel_weight[v][0]+surf->ofs_skel_weight[v][1]+surf->ofs_skel_weight[v][2]+surf->ofs_skel_weight[v][3];
					if (xyzw[3]>0)
						Vector4Scale(surf->ofs_skel_weight[v], 1/xyzw[3], surf->ofs_skel_weight[v]);
				}
			}
			if (exweights)
			{	//something was misread
				surf->numverts = 0;
				surf->numindexes = 0;
				return false;
			}

			//compute the tangents that are not stored in the file.
			surf->ofs_skel_svect	= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_svect	) * surf->numverts);
			surf->ofs_skel_tvect	= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_tvect	) * surf->numverts);
			modfuncs->AccumulateTextureVectors(surf->ofs_skel_xyz, surf->ofs_st_array, surf->ofs_skel_norm, surf->ofs_skel_svect, surf->ofs_skel_tvect, surf->ofs_indexes, surf->numindexes, false);
			modfuncs->NormaliseTextureVectors(surf->ofs_skel_norm, surf->ofs_skel_svect, surf->ofs_skel_tvect, surf->numverts, false);
		}
		else if (ver == 0x14)
		{	//cod2
			surf->ofs_st_array		= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_st_array		) * surf->numverts);
			surf->ofs_skel_xyz		= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_xyz		) * surf->numverts);
			surf->ofs_skel_norm		= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_norm	) * surf->numverts);
			surf->ofs_skel_svect	= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_svect	) * surf->numverts);
			surf->ofs_skel_tvect	= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_tvect	) * surf->numverts);
			surf->ofs_rgbaub		= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_rgbaub		) * surf->numverts);

			if (boney || f->numbones)
			{
				surf->ofs_skel_idx		= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_idx		) * surf->numverts);
				surf->ofs_skel_weight	= plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_skel_weight	) * surf->numverts);
			}
			boneidx = min(boneidx,f->numbones-1);

			for (v = 0; v < nverts; v++)
			{
				surf->ofs_skel_norm[v][0]	= ReadFloat(f);
				surf->ofs_skel_norm[v][1]	= ReadFloat(f);
				surf->ofs_skel_norm[v][2]	= ReadFloat(f);
				surf->ofs_rgbaub[v][0]		= ReadByte(f);
				surf->ofs_rgbaub[v][1]		= ReadByte(f);
				surf->ofs_rgbaub[v][2]		= ReadByte(f);
				surf->ofs_rgbaub[v][3]		= ReadByte(f);
				surf->ofs_st_array[v][0]	= ReadFloat(f);
				surf->ofs_st_array[v][1]	= ReadFloat(f);
				surf->ofs_skel_svect[v][0]	= ReadFloat(f);
				surf->ofs_skel_svect[v][1]	= ReadFloat(f);
				surf->ofs_skel_svect[v][2]	= ReadFloat(f);
				surf->ofs_skel_tvect[v][0]	= ReadFloat(f);
				surf->ofs_skel_tvect[v][1]	= ReadFloat(f);
				surf->ofs_skel_tvect[v][2]	= ReadFloat(f);
				if (boney)
				{
					int w;
					wcount = ReadByte(f);
					boneidx = ReadUInt16(f);
					boneidx = min(boneidx,f->numbones-1);
					xyzw[0]	= ReadFloat(f);
					xyzw[1]	= ReadFloat(f);
					xyzw[2]	= ReadFloat(f);
					xyzw[3] = wcount?ReadByte(f)/255.f:1;
					VectorScale(xyzw, xyzw[3], xyzw);
					matrix = f->basepose + boneidx*12;
					surf->ofs_skel_xyz[ v][0] = xyzw[0] * matrix[0] + xyzw[1] * matrix[1] + xyzw[2] * matrix[ 2] + xyzw[3] * matrix[ 3];
					surf->ofs_skel_xyz[ v][1] = xyzw[0] * matrix[4] + xyzw[1] * matrix[5] + xyzw[2] * matrix[ 6] + xyzw[3] * matrix[ 7];
					surf->ofs_skel_xyz[ v][2] = xyzw[0] * matrix[8] + xyzw[1] * matrix[9] + xyzw[2] * matrix[10] + xyzw[3] * matrix[11];

					surf->ofs_skel_idx[v][0] = surf->ofs_skel_idx[v][1] = surf->ofs_skel_idx[v][2] = surf->ofs_skel_idx[v][3] = boneidx;
					surf->ofs_skel_weight[v][0] = xyzw[3]; surf->ofs_skel_weight[v][1] = surf->ofs_skel_weight[v][2] = surf->ofs_skel_weight[v][3] = 0;

					for (w = 1; w <= wcount; w++)
					{
						boneidx = ReadUInt16(f);
						boneidx = min(boneidx,f->numbones-1);
						xyzw[0] = ReadFloat(f);
						xyzw[1] = ReadFloat(f);
						xyzw[2] = ReadFloat(f);
						xyzw[3] = ReadUInt16(f)/65535.f;
						VectorScale(xyzw, xyzw[3], xyzw);
						matrix = f->basepose + boneidx*12;
						surf->ofs_skel_xyz[ v][0] += xyzw[0] * matrix[0] + xyzw[1] * matrix[1] + xyzw[2] * matrix[ 2] + xyzw[3] * matrix[ 3];
						surf->ofs_skel_xyz[ v][1] += xyzw[0] * matrix[4] + xyzw[1] * matrix[5] + xyzw[2] * matrix[ 6] + xyzw[3] * matrix[ 7];
						surf->ofs_skel_xyz[ v][2] += xyzw[0] * matrix[8] + xyzw[1] * matrix[9] + xyzw[2] * matrix[10] + xyzw[3] * matrix[11];
						if (w < 4)
						{
							surf->ofs_skel_idx[v][w] = boneidx;
							surf->ofs_skel_weight[v][w] = xyzw[3];
						}
					}

					//compensate for any missing weights
					xyzw[3] = surf->ofs_skel_weight[v][0]+surf->ofs_skel_weight[v][1]+surf->ofs_skel_weight[v][2]+surf->ofs_skel_weight[v][3];
					if (xyzw[3]>0&&xyzw[3]!=1)
						Vector4Scale(surf->ofs_skel_weight[v], 1/xyzw[3], surf->ofs_skel_weight[v]);
				}
				else
				{
					surf->ofs_skel_xyz[v][0]	= ReadFloat(f);
					surf->ofs_skel_xyz[v][1]	= ReadFloat(f);
					surf->ofs_skel_xyz[v][2]	= ReadFloat(f);

					if (surf->ofs_skel_idx)
					{
						surf->ofs_skel_idx[v][0] = surf->ofs_skel_idx[v][1] = surf->ofs_skel_idx[v][2] = surf->ofs_skel_idx[v][3] = boneidx;
						surf->ofs_skel_weight[v][0] = 1; surf->ofs_skel_weight[v][1] = surf->ofs_skel_weight[v][2] = surf->ofs_skel_weight[v][3] = 0;
					}
				}
			}

			//indexes moved to AFTER. also triangles instead of weird strips. much nicer.
			surf->numindexes = ntris*3;
			surf->ofs_indexes = plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf->ofs_indexes		) * surf->numindexes);
			for (v = 0; v < ntris*3; v++)
				surf->ofs_indexes[v] = ReadUInt16(f);
		}
	}
	return true;
}

struct codbone_s
{	//source data is the number of samples, the pose index for each sample and THEN the actual samples at the specified sparse poses.
//	const char *name;
	unsigned int numquats;
	qboolean flip;
	struct
	{
		unsigned short ts;
		signed short v[3];
	} *quats;
	unsigned int numcoords;
	struct
	{
		unsigned int ts;
		vec3_t v;
	} *coord;
};

static void Mod_XAnim_LoadQuats (struct model_s *mod, struct fstream_s *f, struct codbone_s *b, unsigned int maxts, qboolean flipquat, qboolean zonly)
{
	unsigned int i, n = ReadUInt16(f);
	b->flip = flipquat;
	if (n)
	{
		b->numquats = n;
		b->quats = plugfuncs->GMalloc(&mod->memgroup, sizeof(*b->quats)*n);
		if (n == 1 || n == maxts)
			for (i = 0; i < n; i++)
				b->quats[i].ts = i;
		else if (maxts > 0xff)
			for (i = 0; i < n; i++)
				b->quats[i].ts = ReadUInt16(f);
		else
			for (i = 0; i < n; i++)
				b->quats[i].ts = ReadByte(f);

		for (i = 0; i < n; i++)
		{
			b->quats[i].v[0] = zonly?0:ReadSInt16(f);
			b->quats[i].v[1] = zonly?0:ReadSInt16(f);
			b->quats[i].v[2] =         ReadSInt16(f);
		}
	}
}
static void Mod_XAnim_LoadCoords (struct model_s *mod, struct fstream_s *f, struct codbone_s *b, unsigned int maxts)
{
	unsigned int i, n = ReadUInt16(f);
	if (n)
	{
		b->numcoords = n;
		b->coord = plugfuncs->GMalloc(&mod->memgroup, sizeof(*b->coord)*n);
		if (n == 1 || n == maxts)
			for (i = 0; i < n; i++)
				b->coord[i].ts = i;
		else if (maxts > 0xff)
			for (i = 0; i < n; i++)
				b->coord[i].ts = ReadUInt16(f);
		else
			for (i = 0; i < n; i++)
				b->coord[i].ts = ReadByte(f);

		for (i = 0; i < n; i++)
		{
			b->coord[i].v[0] = ReadFloat(f);
			b->coord[i].v[1] = ReadFloat(f);
			b->coord[i].v[2] = ReadFloat(f);
		}
	}
}
static float *QDECL Mod_XAnim_GetRawBones(const struct galiasinfo_s *animmesh, const struct galiasanimation_s *a, float time, float *bonematrixstorage, const struct galiasbone_s *boneinf, int numbones)
{
	const struct codbone_s *bone = (const void*)(a+1);
	size_t b, i, j;
	int ts;
	float frac;
	galiasrefpose_t ref;
	time *= a->rate;
	ts = time;
	frac = time - ts;	//FIXME: negative time!
	if (a->loop)
	{
		ts %= a->numposes;
		if (ts < 0)
			ts += a->numposes;
	}
	ts = bound(0, ts, a->numposes);
	if (!boneinf)
	{	//our own bones?!? that'll be a horrible mess, but if you're really asking for that...
		boneinf = animmesh->ofsbones;
		numbones = min(numbones,animmesh->numbones);
	}

	for (b = 0; b < numbones; b++, boneinf++)
	{
		ref = boneinf->ref;
		for (j = 0; j < animmesh->numbones; j++)
		{
			if (!strcmp(boneinf->name, animmesh->ofsbones[j].name))
			{	//okay this is the one we want. replace the reference pose with the animated data.
				bone = (const struct codbone_s*)(a+1) + j;
				for (i = 0; i < bone->numquats; i++)
					if (ts < bone->quats[i].ts)
						break;	//we flew too close to the sun!
				if (!i--)
					;	//use the value from the model.
				else if (i < bone->numquats-1 && bone->quats[i+1].ts == ts+1)
				{
					vec4_t oq,nq;
					VectorScale(bone->quats[i].v, 1.f/32767, oq);
					oq[3] = 1 - DotProduct(oq,oq);
					if (oq[3] > 0)
						oq[3] = sqrt(oq[3]);

					VectorScale(bone->quats[i+1].v, 1.f/32767, nq);
					nq[3] = 1 - DotProduct(nq,nq);
					if (nq[3] > 0)
						nq[3] = sqrt(nq[3]);

					modfuncs->QuaternionSlerp(oq, nq, frac, ref.quat);
				}
				else
				{
					VectorScale(bone->quats[i].v, 1.f/32767, ref.quat);
					ref.quat[3] = 1 - DotProduct(ref.quat,ref.quat);
					if (ref.quat[3] > 0)
						ref.quat[3] = sqrt(ref.quat[3]);
				}

				for (i = 0; i < bone->numcoords; i++)
					if (ts < bone->coord[i].ts)
						break;
				if (!i--)
					;	//use the value from the model.
				else if (i < bone->numcoords-1 && bone->coord[i+1].ts == ts+1)
					VectorInterpolate(bone->coord[i].v, frac, bone->coord[i+1].v, ref.org);
				else
					VectorCopy(bone->coord[i].v, ref.org);

				break;	//don't look for others.
			}
		}
		modfuncs->GenMatrixPosQuat4Scale(ref.org, ref.quat, ref.scale, bonematrixstorage + b*12);
	}
	return bonematrixstorage;
}
static int Mod_XAnim_CompareEvents (const void *av, const void *bv)
{
	const galiasevent_t *a=av;
	const galiasevent_t *b=bv;
	return b->timestamp-a->timestamp;
}
static qboolean Mod_XAnim_Load (struct model_s *mod, void *buffer, size_t fsize)
{
	struct fstream_s f = {buffer, fsize};
	unsigned short ver = ReadUInt16(&f);
	unsigned short numposes = ReadUInt16(&f);
	unsigned short numabones=0, numrbones = ReadUInt16(&f);
	unsigned char flags = ReadByte(&f);
	unsigned short framerate = ReadUInt16(&f);
	unsigned int i;
	const qbyte *flip, *tiny;
	struct codbone_s *bone;
	galiasinfo_t *surf;
	galiasanimation_t *anim;
	galiasbone_t *gbones;
	int numev;
	switch(ver)
	{
	case 0x0e:	//cod1
		break;
	case 0x14:
		break;
	default:
		Con_Printf(CON_ERROR"%s: unknown version %x\n", mod->name, ver);
		return false;
	}
//	Con_Printf(CON_DEBUG"Poses:%i bones:%i flags:%i rate:%i\n", numposes, numrbones, flags, framerate);

	if (flags & 2)
		numabones = 1;

	mod->type = mod_alias;
	mod->fromgame = fg_new;
	mod->meshinfo = surf = plugfuncs->GMalloc(&mod->memgroup, sizeof(*surf) + sizeof(*anim) + sizeof(*bone)*(numabones+numrbones));
	surf->numbones = numabones+numrbones;
	surf->ofsbones = gbones = plugfuncs->GMalloc(&mod->memgroup, sizeof(*gbones)*(numabones+numrbones));

	surf->numanimations = 1;
	surf->ofsanimations = anim = (void*)(surf+1);

	anim->loop = !!(flags&1);
	anim->numposes = numposes;
	anim->skeltype = SKEL_RELATIVE;	//no parents info stored here, so these are screwy unless you're importing them into a model that DOES provide the parents info for you.
	bone = (void*)(anim+1);
	anim->rate = framerate;
	Q_strlcpy(anim->name, mod->name, sizeof(anim->name));
	anim->GetRawBones = Mod_XAnim_GetRawBones;

	for (i = 0; i < numabones; i++)
	{
		gbones[i].parent = -1;
		VectorClear(gbones[i].ref.org);
		Vector4Set(gbones[i].ref.quat,0,0,0,1);
		VectorSet(gbones[i].ref.scale,1,1,1);
		Q_strlcpy(gbones[i].name, "tag_origin", sizeof(gbones[i].name));

		Mod_XAnim_LoadQuats(mod, &f, &bone[i], numposes, false, true);
		Mod_XAnim_LoadCoords(mod, &f, &bone[i], numposes);
	}
	flip = ReadBytes(&f, (numrbones+7)>>3);
	tiny = ReadBytes(&f, (numrbones+7)>>3);
	if (anim->loop)
		numposes++;	//is this the right place?
	for (i = 0; i < numrbones; i++)
	{
		const char *n = ReadString(&f);
		gbones[numabones+i].parent = -1;	//no information. oh noes.
		VectorClear(gbones[i].ref.org);
		Vector4Set(gbones[i].ref.quat,0,0,0,1);
		VectorSet(gbones[i].ref.scale,1,1,1);
		Q_strlcpy(gbones[numabones+i].name, n, sizeof(gbones[numabones+i].name));
//		Con_Printf(CON_DEBUG"Part %i = %s (%i %i)\n", numabones+i, bone[numabones+i].name, !!(flip[i>>3]&(1u<<(i&7))), !!(tiny[i>>3]&(1u<<(i&7))));
	}
	for (i = 0; i < numrbones; i++)
	{
//		Con_Printf(CON_WARNING"%s:\n", gbones[numabones+i].name);
		Mod_XAnim_LoadQuats(mod, &f, &bone[numabones+i], numposes, !!(flip[i>>3]&(1u<<(i&7))), !!(tiny[i>>3]&(1u<<(i&7))));
		Mod_XAnim_LoadCoords(mod, &f, &bone[numabones+i], numposes);
	}
	if (anim->loop)
		numposes--;

	numev = ReadByte(&f);
	if (numev)
	{
		anim->events = plugfuncs->GMalloc(&mod->memgroup, sizeof(*anim->events)*(numev));
		for (i = 0; i < numev; i++)
		{
			const char *txt = ReadString(&f);
			anim->events[i].code = 0;	//this format doesn't provide event ids, just pure strings.
			anim->events[i].data = strcpy(plugfuncs->GMalloc(&mod->memgroup, strlen(txt)+1), txt);

			anim->events[i].timestamp = ReadUInt16(&f) / (float)numposes;
		}
		qsort(anim->events, numev, sizeof(*anim->events), Mod_XAnim_CompareEvents);	//make sure they're sorted by timestamps.
		for (i = 0; i < numev-1; i++)
			anim->events[i].next = &anim->events[i+1];
	}

	if (f.ofs != f.len)
		Con_Printf(CON_WARNING"Misread %s (%u bytes of %u)\n", mod->name, (unsigned)f.ofs, (unsigned)f.len);
	return true;
}
qboolean QDECL Mod_XModel_Load (struct model_s *mod, void *buffer, size_t fsize)
{
	struct fstream_s f = {buffer, fsize};
	struct fstream_s pf = {NULL,0};
	unsigned short ver;
	unsigned i;
	unsigned int clod, clodnsurf;
	unsigned short t;
	struct lod_s lod[4];
	float mincov = 0;
	int nlod;

	//I fucking hate this.
	if (!strncmp(mod->publicname, "xanim/", 6))	//anims are not linked in any way, we treat them as separate precachable models so that gamecode can selectively load them.
		return Mod_XAnim_Load(mod, buffer, fsize);
	if (!strncmp(mod->publicname, "xmodelparts/", 12))	//loaded as part of models. don't get confused.
		return false;
	if (!strncmp(mod->publicname, "xmodelsurfs/", 12))	//loaded as part of models. don't get confused.
		return false;
	//"xmodels/" is okay though!

	ver = ReadUInt16(&f);
	switch(ver)
	{
	case 0x0e:	//cod1
		nlod = 3;
		break;
	case 0x14:	//cod2
		/*type =*/ ReadByte(&f);
		nlod = 4;
		break;
	default:
		Con_Printf(CON_ERROR"%s: Unknown version %x\n", mod->name, ver);
		return false;
	}
	mod->mins[0] = ReadFloat(&f);
	mod->mins[1] = ReadFloat(&f);
	mod->mins[2] = ReadFloat(&f);
	mod->maxs[0] = ReadFloat(&f);
	mod->maxs[1] = ReadFloat(&f);
	mod->maxs[2] = ReadFloat(&f);
	for (i = 0; i < nlod; i++)
	{
		lod[i].coverage = ReadFloat(&f);	//coverage
		lod[i].name = ReadString(&f);
	}
	clod = ReadUInt32(&f);
	clodnsurf = ReadUInt32(&f);
	(void)clod;
	for (i = 0; i < clodnsurf && !ReadEOF(&f); i++)
	{
		unsigned int t, ntris = ReadUInt32(&f);
		for (t = 0; t < ntris && !ReadEOF(&f); t++)
		{
			vec4_t p, sd, td;
			//plane
			p[0] = ReadFloat(&f);
			p[1] = ReadFloat(&f);
			p[2] = ReadFloat(&f);
			p[3] = ReadFloat(&f);

			//?splane?
			sd[0] = ReadFloat(&f);
			sd[1] = ReadFloat(&f);
			sd[2] = ReadFloat(&f);
			sd[3] = ReadFloat(&f);

			//?tplane?
			td[0] = ReadFloat(&f);
			td[1] = ReadFloat(&f);
			td[2] = ReadFloat(&f);
			td[3] = ReadFloat(&f);

			(void)p;
			(void)sd;
			(void)td;

//			Con_Printf("%i: %f %f %f %f : %f %f %f %f : %f %f %f %f\n", t, p[0],p[1],p[2],p[3], sd[0],sd[1],sd[2],sd[3], td[0],td[1],td[2],td[3]);
			//dunno how to use this for collision... a triangle has 3 side faces AND a front!
		}
		//bounds
		ReadFloat(&f);
		ReadFloat(&f);
		ReadFloat(&f);
		ReadFloat(&f);
		ReadFloat(&f);
		ReadFloat(&f);
		/*boneidx = */ ReadUInt32(&f);
		/*contents = */ ReadUInt32(&f);
		/*surfaceflags = */ ReadUInt32(&f);
	}

	pf.ofs = 0;
	pf.start = filefuncs->LoadFile(va("xmodelparts/%s", lod[0].name), &pf.len);
	if (pf.start)
	{
		if (!ReadEOF(&pf) && Mod_XModel_LoadPart(mod, &pf))
		{
			if (pf.ofs != pf.len)
				Con_Printf(CON_WARNING"Misread xmodelparts/%s (%u bytes of %u)\n", lod[0].name, (unsigned)f.ofs, (unsigned)f.len);
		}
		else
			return false;
		plugfuncs->Free(pf.start);
	}

	for (i = 0; i < nlod; i++)
	{
		if (!*lod[i].name)
			break;
		lod[i].numtex = ReadUInt16(&f);
		for (t = 0; t < lod[i].numtex; t++)
			lod[i].tex[t] = ReadString(&f);
		pf.ofs = 0;
		pf.start = filefuncs->LoadFile(va("xmodelsurfs/%s", lod[i].name), &pf.len);
		if (pf.start)
		{
			if (!ReadEOF(&pf) && Mod_XModel_LoadSurfs(mod, &pf, &lod[i], mincov))
			{
				if (pf.ofs != pf.len)
					Con_Printf(CON_WARNING"Misread xmodelsurfs/%s (%u bytes of %u)\n", lod[i].name, (unsigned)f.ofs, (unsigned)f.len);
			}
			else
				return false;
			mincov = lod[i].coverage;
			plugfuncs->Free(pf.start);
		}
	}


	mod->type = mod_alias;
	mod->fromgame = fg_new;
	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);
//	if (mod->meshinfo)
//		modfuncs->BIH_BuildAlias(mod, mod->meshinfo);
	return !!mod->meshinfo;
}

static qboolean XMODEL_Init(void)
{
	filefuncs = plugfuncs->GetEngineInterface(plugfsfuncs_name, sizeof(*filefuncs));
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	if (modfuncs && modfuncs->version != MODPLUGFUNCS_VERSION)
		modfuncs = NULL;

	if (modfuncs && filefuncs)
	{
		modfuncs->RegisterModelFormatMagic("XMODEL", "\x0e\0",2, Mod_XModel_Load);	//cod1
		modfuncs->RegisterModelFormatMagic("XMODEL", "\x14\0",2, Mod_XModel_Load);	//cod2
		return true;
	}
	return false;
}


qboolean CODBSP_Init(void);
qboolean STypes_Init(void);
qboolean IWI_Init(void);
//qboolean IWFF_Init(void);

qboolean Plug_Init(void)
{
	qboolean somethingisokay = false;
	char plugname[128];
	strcpy(plugname, "cod");
	plugfuncs->GetPluginName(0, plugname, sizeof(plugname));

	if (!STypes_Init())	Con_Printf(CON_ERROR"%s: Shader Types support unavailable\n",	plugname);	else	somethingisokay = true;
	if (!IWI_Init())	Con_Printf(CON_ERROR"%s: IWI support unavailable\n",			plugname);	else	somethingisokay = true;
	if (!XMODEL_Init())	Con_Printf(CON_ERROR"%s: XModel support unavailable\n",			plugname);	else	somethingisokay = true;
	if (!CODBSP_Init())	Con_Printf(CON_ERROR"%s: CODBSP support unavailable\n",			plugname);	else	somethingisokay = true;
//	if (!IWFF_Init())	Con_Printf(CON_ERROR"%s: FF support unavailable\n",				plugname);	else	somethingisokay = true;
	return somethingisokay;
}
