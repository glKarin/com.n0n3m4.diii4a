/*
mod_terrain_create terrorgen; edit maps/terrorgen.hmp; map terrorgen
you can use mod_terrain_convert to generate+save the entire map for redistribution to people without this particular plugin version, ensuring longevity.
(this paticular command was meant to load+save the entire map, once mod_terrain_savever 2 is default...)

FIXME: no way to speciffy which gen plugin to use for a particular map
*/

#include "../plugin.h"
#include "glquake.h"
#include "com_mesh.h"
#include "gl_terrain.h"

#ifdef TERRAIN
#define GENHIGHTSCALE 1024.0

static plugterrainfuncs_t *terr;
static plugmodfuncs_t *modfuncs;

struct rndctx_s
{
	unsigned int x, y, z, w;
};
unsigned int myrand(struct rndctx_s *ctx)
{	//ripped from wikipedia (originally called xorshift128)
	unsigned int t = ctx->x ^ (ctx->x << 11);
	ctx->x = ctx->y; ctx->y = ctx->z; ctx->z = ctx->w;
	return ctx->w = ctx->w ^ (ctx->w >> 19) ^ t ^ (t >> 8);
}
float TerrorGen_PredCRandom(heightmap_t *hm, int x, int y)
{
	int seed = atoi(hm->seed);
	struct rndctx_s rctx = {x*3421 ^ y*35231, y*23423, seed, seed+seed*4321+x*432+y*423+x*y}; //overflows are fine
	unsigned int r = myrand(&rctx);
	return ((r & 0xffffff) / (float)0x800000)-1;
}

float TerrorGen_CRandom(struct rndctx_s *rctx)
{
	unsigned int r = myrand(rctx);
	return ((r & 0xffffff) / (float)0x800000)-1;
}


//the four corners are defined in advance.
//size is a power-of-two, h must be sized pow(size+1,2)
static void GenHeights(heightmap_t *hm, struct rndctx_s *rctx, float *h, int size, int sx, int sy, float scale)
{
	int stride = size+1;
	int x, y;
	//borders are left, right, top, bottom
	while(size > 1)
	{
		int m = size/2;

		//find central points (diamond pattern)
		for (x = 0; x < stride-1; x += size)
		{
			for (y = 0; y < stride-1; y += size)
			{
				float mid = (h[(x)+(y)*stride] + h[(x+size)+(y)*stride] + h[(x)+(y+size)*stride] + h[(x+size)+(y+size)*stride])/4;
				mid += TerrorGen_CRandom(rctx)*scale;
				h[(x+m) + (y+m)*stride] = mid;
			}
		}

		//find square points (square pattern)
		for (x = 0; x < stride-1; x += size)
		{
			for (y = 0; y < stride-1; y += size)
			{
				float mid;
				//left side
				mid = h[(x)+(y)*stride];
				mid += h[(x)+(y+size)*stride];
				if (x != 0) //not an outer edges
				{
					mid += h[(x-m)+(y+m)*stride];
					mid += h[(x+m)+(y+m)*stride];
					mid = mid/4 + TerrorGen_CRandom(rctx)*scale;
				}
				else	//outer edge doesn't look inwards, because any neighbouring block would not know it
					mid = mid/2 + TerrorGen_PredCRandom(hm, sx+x, sy+y+m)*scale;
				h[(x) + (y+m)*stride] = mid;

				//right
				mid = h[(x+size)+(y)*stride];
				mid += h[(x+size)+(y+size)*stride];
				if (x+size != stride-1) //not an outer edges
				{
					mid += h[(x+size-m)+(y+m)*stride];
					mid += h[(x+size+m)+(y+m)*stride];
					mid = mid/4 + TerrorGen_CRandom(rctx)*scale;
				}
				else	//outer edge doesn't look inwards, because any neighbouring block would not know it
					mid = mid/2 + TerrorGen_PredCRandom(hm, sx+x+size, sy+y+m)*scale;
				h[(x+size) + (y+m)*stride] = mid;

				//top
				mid = h[(x)+(y)*stride];
				mid += h[(x+size)+(y)*stride];
				if (y != 0) //not an outer edges
				{
					mid += h[(x+m)+(y-m)*stride];
					mid += h[(x+m)+(y+m)*stride];
					mid = mid/4 + TerrorGen_CRandom(rctx)*scale;
				}
				else	//outer edge doesn't look inwards, because any neighbouring block would not know it
					mid = mid/2 + TerrorGen_PredCRandom(hm, sx+x+m, sy+y)*scale;
				h[(x+m) + (y)*stride] = mid;

				//bottom
				mid = h[(x)+(y+size)*stride];
				mid += h[(x+size)+(y+size)*stride];
				if (y+size != stride-1) //not an outer edges
				{
					mid += h[(x+m)+(y+size-m)*stride];
					mid += h[(x+m)+(y+size+m)*stride];
					mid = mid/4 + TerrorGen_CRandom(rctx)*scale;
				}
				else	//outer edge doesn't look inwards, because any neighbouring block would not know it
					mid = mid/2 + TerrorGen_PredCRandom(hm, sx+x+m, sy+y+size)*scale;
				h[(x+m) + (y+size)*stride] = mid;
			}
		}
		size = m;
		scale /= 2;
	}
}



static void TerrorGen_GenerateOne(heightmap_t *hm, struct rndctx_s *rctx, int sx, int sy, hmsection_t *s, float tl,float tr,float bl,float br)
{
	int x,y,i;
	qbyte *lm;

	s->heights[0] = tl;
	s->heights[SECTHEIGHTSIZE-1] = tr;
	s->heights[0+(SECTHEIGHTSIZE-1)*SECTHEIGHTSIZE] = bl;
	s->heights[SECTHEIGHTSIZE-1+(SECTHEIGHTSIZE-1)*SECTHEIGHTSIZE] = br;
	GenHeights(hm, rctx, s->heights, SECTHEIGHTSIZE-1, sx*(SECTHEIGHTSIZE-1), sy*(SECTHEIGHTSIZE-1), GENHIGHTSCALE/16);

	s->flags |= TSF_RELIGHT;

	//pick the textures to blend between. I'm just hardcoding shit here. this is meant to be some sort example.
	Q_strlcpy(s->texname[0], "city4_2", sizeof(s->texname[0]));
	Q_strlcpy(s->texname[1], "ground1_2", sizeof(s->texname[1]));
	Q_strlcpy(s->texname[2], "ground1_8", sizeof(s->texname[2]));
	Q_strlcpy(s->texname[3], "ground1_1", sizeof(s->texname[3]));

	for (y = 0, i=0; y < SECTHEIGHTSIZE; y++)
	for (x = 0; x < SECTHEIGHTSIZE; x++, i++)
	{
		//calculate where it is in worldspace, if that's useful to you.
//		float wx = hm->sectionsize*(sx + x/(float)(SECTHEIGHTSIZE-1));
//		float wy = hm->sectionsize*(sy + y/(float)(SECTHEIGHTSIZE-1));

		//calculate the RGBA tint. these are floats, so you can oversaturate.
		s->colours[i][0] = 1;
		s->colours[i][1] = 1;
		s->colours[i][2] = 1;
		s->colours[i][3] = 1;
	}

	//make sure there's lightmap storage available
	terr->InitLightmap(s, /*fill with default values*/true);
	lm = terr->GetLightmap(s, 0, /*flag as edited*/true);
	if (lm)
	{	//pleaseworkpleaseworkpleasework
		for (y = 0; y < SECTTEXSIZE; y++, lm += (HMLMSTRIDE)*4)
		for (x = 0; x < SECTTEXSIZE; x++)
		{
			//calculate where it is in worldspace, if that's useful to you.
			float wx = hm->sectionsize*(sx + x/(float)(SECTTEXSIZE-1));
			float wy = hm->sectionsize*(sy + y/(float)(SECTTEXSIZE-1));

			//calc which texture to use
			//adds to 1, with texture[3] taking the remainder.
			lm[x*4+0] = max(0, 255 - 255*fabs(wx/1024));
			lm[x*4+1] = max(0, 255 - 255*fabs(wy/1024));
			lm[x*4+2] = min(lm[x*4+0],lm[x*4+1]);
			lm[x*4+0] -= lm[x*4+2];
			lm[x*4+1] -= lm[x*4+2];

			//logically: lm[x*4+3] = 255-(lm[x*4+0]+lm[x*4+1]+lm[x*4+2]);
			//however, the fourth channel is actually used as a lighting multiplier.
			lm[x*4+3] = 255;
		}
	}

/*	//insert the occasional mesh...
	if ((sx&3) == 0 && (sy&3) == 0)
	{
		vec3_t ang, org, axis[3];
		org[0] = hm->sectionsize*sx;
		org[1] = hm->sectionsize*sy;
		org[2] = 128;
		VectorClear(ang);
		ang[0] = sy*12.5;	//lul
		ang[1] = sx*12.5;
		modfuncs->AngleVectors(ang, axis[0], axis[1], axis[2]);
		VectorNegate(axis[1],axis[1]);	//axis[1] needs to be left, not right. silly quakeisms.

		//obviously you can insert mdls instead... preferably do that!
		terr->AddMesh(hm, TGS_NOLOAD, NULL, "maps/dm4.bsp", org, axis, 1);
	}*/
}

#define GENBLOCKSIZE 16
static qboolean QDECL TerrorGen_GenerateBlock(heightmap_t *hm, int sx, int sy, unsigned int tgsflags)
{
	float h[(GENBLOCKSIZE+1)*(GENBLOCKSIZE+1)];
	hmsection_t *sect[GENBLOCKSIZE*GENBLOCKSIZE];
	int mx = sx & ~(GENBLOCKSIZE-1);
	int my = sy & ~(GENBLOCKSIZE-1);
	struct rndctx_s rctx;
	int i;

	if (!terr->GenerateSections(hm, mx, my, GENBLOCKSIZE, sect))
		return false;

	for (i = 0; i < countof(h); i++)
		h[i] = -1024;

	//generate global height values
	h[           0+           0*(GENBLOCKSIZE+1)] = 0;//TerrorGen_PredRandom(hm, mx             , my             )*GENHIGHTSCALE*4;
	h[GENBLOCKSIZE+           0*(GENBLOCKSIZE+1)] = 0;//TerrorGen_PredRandom(hm, mx+GENBLOCKSIZE, my             )*GENHIGHTSCALE*4;
	h[             GENBLOCKSIZE*(GENBLOCKSIZE+1)] = 0;//TerrorGen_PredRandom(hm, mx             , my+GENBLOCKSIZE)*GENHIGHTSCALE*4;
	h[GENBLOCKSIZE+GENBLOCKSIZE*(GENBLOCKSIZE+1)] = 0;//TerrorGen_PredRandom(hm, mx+GENBLOCKSIZE, my+GENBLOCKSIZE)*GENHIGHTSCALE*4;
	rctx.x = mx*4142^mx*523423;
	rctx.y = mx*4323;
	rctx.z = mx*234;
	rctx.w = 2535;
	GenHeights(hm, &rctx, h, GENBLOCKSIZE, (mx-CHUNKBIAS)*SECTHEIGHTSIZE, (my-CHUNKBIAS)*SECTHEIGHTSIZE, GENHIGHTSCALE);

	for (sy = 0; sy < GENBLOCKSIZE; sy++)
	{
		for (sx = 0; sx < GENBLOCKSIZE; sx++)
		{
			if (!sect[sx + sy*GENBLOCKSIZE])
				continue;	//already in memory.

			//in case we skipped a section...
			rctx.x = (mx*4141^mx*523423) + sx*4231 + sy*539;
			rctx.y = mx*4323;
			rctx.z = mx*231+sy;
			rctx.w = 253553;

			TerrorGen_GenerateOne(hm, &rctx, mx+sx-CHUNKBIAS, my+sy-CHUNKBIAS, sect[sx + sy*GENBLOCKSIZE],
					h[(sx  )+(sy  )*(GENBLOCKSIZE+1)],
					h[(sx+1)+(sy  )*(GENBLOCKSIZE+1)],
					h[(sx  )+(sy+1)*(GENBLOCKSIZE+1)],
					h[(sx+1)+(sy+1)*(GENBLOCKSIZE+1)]);
			terr->FinishedSection(sect[sx + sy*GENBLOCKSIZE], true);
		}
	}
	return true;
}

static qboolean TerrorGen_Shutdown(void)
{	//if its still us, make sure there's no dangling pointers.
	if (terr->AutogenerateSection == TerrorGen_GenerateBlock)
		terr->AutogenerateSection = NULL;
	return true;
}
qboolean Plug_Init(void)
{
	modfuncs = plugfuncs->GetEngineInterface(plugmodfuncs_name, sizeof(*modfuncs));
	if (modfuncs && modfuncs->version < MODPLUGFUNCS_VERSION)
		modfuncs = NULL;
	terr = plugfuncs->GetEngineInterface(plugterrainfuncs_name, sizeof(*terr));
	if (!terr)
		return false;
	if (!plugfuncs->ExportFunction("Shutdown", TerrorGen_Shutdown))
		return false;

	terr->AutogenerateSection = TerrorGen_GenerateBlock;
	return true;
}
#else
qboolean Plug_Init(void)
{
	return false;
}
#endif