#include "quakedef.h"
#ifdef MAP_DOOM
#include "glquake.h"
#include "shader.h"



char *va2(char *buffer, size_t buffersize, const char *format, ...)
{
	va_list		argptr;

	va_start (argptr, format);
	buffer[--buffersize] = 0;
	vsnprintf (buffer, buffersize, format, argptr);
	va_end (argptr);

	return buffer;
}

int SignbitsForPlane (mplane_t *out);
int	PlaneTypeForNormal ( vec3_t normal );

//coded from file specifications provided by:
//Matthew S Fell (msfell@aol.com)
//Unofficial Doom Specs

//(aol suck)


//assumptions:
//1. That there is a node, and thus two ssectors.
//2. That the user doesn't want textures...
//3. That all segs ssectors for a single sector are all the same.
//4. That ALL sectors are fully enclosed, and not made of two areas.
//5. That no sectors are inside out.

/*FIXME:
we need to do a bsp2prt type thing (walk nodes and determine actual leaf/ssector shapes based upon those).
build sector geometry based upon this.
this is because flats in doom were implemented using a flood-fill algorithm and thus omits various unecessary inner edges, while 3d rendering apis all need tri-soup instead.
attempting to generate sane volumes from most doom maps is doomed to failure because quite often the sector values on linedefs is just buggy, resulting in some really whacky polygons that cannot be souped in any meaningful way.
this may still result in a mess of floor polygons outside the world, so be sure to draw those last, for early-z.
*/

enum {
	THING_PLAYER		= 1,
	THING_PLAYER2		= 2,
	THING_PLAYER3		= 3,
	THING_PLAYER4		= 4,
	THING_DMSPAWN		= 11,

//we need to balance weapons according to ammo types.
	THING_WCHAINSAW		= 2005,	//-> quad
	THING_WSHOTGUN1		= 2001,	//-> ng
	THING_WSHOTGUN2		= 82,	//-> sng
	THING_WCHAINGUN		= 2002,	//-> ssg
	THING_WROCKETL		= 2003,	//-> lightning
	THING_WPLASMA		= 2004,	//-> grenade
	THING_WBFG			= 2006	//-> rocket
} THING_TYPES;

//thing flags
//skill/dm is appears in rather than quake's excuded in.
#define THING_EASY			1
#define THING_MEDIUM		2
#define THING_HARD			4
#define THING_DEAF			8
#define	THING_DEATHMATCH	16
//other bits are ignored



typedef struct {
	short xpos;
	short ypos;
	short angle;
	unsigned short type;
	unsigned short flags;
} dthing_t;

typedef struct {
	short xpos;
	short ypos;
} ddoomvertex_t;

typedef struct {
	float xpos;
	float ypos;
} mdoomvertex_t;

typedef struct {
	unsigned short vert[2];
	unsigned short flags;
	short types;
	short tag;
	unsigned short sidedef[2]; //(0xffff is none for sidedef[1])
} dlinedef_t;
#define LINEDEF_IMPASSABLE		1
#define	LINEDEF_BLOCKMONSTERS	2
#define LINEDEF_TWOSIDED		4
#define LINEDEF_UPPERUNPEGGED	8
#define LINEDEF_LOWERUNPEGGED	16
#define LINEDEF_SECRET			32	//seen as singlesided on automap, does nothing else.
#define LINEDEF_BLOCKSOUND		64
#define LINEDEF_NOTONMAP		128	//doesn't appear on automap.
#define LINEDEF_STARTONMAP		256
//others are ignored.

typedef struct {
	short texx;
	short texy;
	char uppertex[8];
	char lowertex[8];
	char middletex[8];
	unsigned short sector;
} dsidedef_t;
typedef struct {
	float texx;
	float texy;
	int uppertex;
	int lowertex;
	int middletex;
	unsigned short sector;
} msidedef_t;

typedef struct {	//figure out which linedef to use and throw the rest away.
	unsigned short	vert[2];
	short	angle;
	unsigned short	linedef;
	short	direction;
	short	offset;
} dseg_t;

typedef struct {
	unsigned short	vert[2];
	unsigned short	linedef;
	short	direction;
	unsigned short Partner;	//the one on the other side of the owner's linedef
} dgl_seg1_t;

typedef struct {
	unsigned int	vert[2];
	unsigned short	linedef;
	short	direction;
	unsigned int Partner;	//the one on the other side of the owner's linedef
} dgl_seg3_t;

typedef struct {
	unsigned short segcount;
	unsigned short first;
} dssector_t;

typedef struct {
	struct msector_s *sector;
	unsigned short segcount;
	unsigned short first;
} mssector_t;

typedef struct {
	short x;
	short y;
	short dx;
	short dy;
	short y1upper;
	short y1lower;
	short x1lower;
	short x1upper;
	short y2upper;
	short y2lower;
	short x2lower;
	short x2upper;
	unsigned short node1;
	unsigned short node2;
} ddoomnode_t;
#define NODE_IS_SSECTOR	0x8000

typedef struct {
	short floorheight;
	short ceilingheight;
	char floortexture[8];
	char ceilingtexture[8];
	short lightlevel;
	short specialtype;
	short tag;
} dsector_t;

typedef struct msector_s {
	int visframe;
	int floortex;
	int ceilingtex;

	short floorheight;
	short ceilingheight;

	qbyte lightlev;
	qbyte pad;
	int numflattris;
	short tag;
	short specialtype;

	unsigned short *flats;
} msector_t;


typedef struct {
	short xorg;
	short yorg;
	short columns;
	short rows;
} blockmapheader_t;

typedef struct
{
	char name[16];
	shader_t *shader;
	unsigned short width;
	unsigned short height;
	batch_t batch;
	mesh_t *meshptr;
	mesh_t mesh;
	int maxverts;
	int maxindicies;
} doomtexture_t;

typedef struct doommap_s
{
	model_t			*model;

	ddoomnode_t		*node;
	plane_t			*nodeplane;
	unsigned int	numnodes;

	mssector_t		*ssector;	//aka: leafs
	unsigned int	numssectors;

	msector_t		*sector;
	unsigned int	numsectors;

	dthing_t		*thing;
	unsigned int	numthings;

	mdoomvertex_t	*vertexes;
	unsigned int	numvertexes;

	dgl_seg3_t		*seg;
	unsigned int	numsegs;

	dlinedef_t		*linedef;
	plane_t			*lineplane;
	unsigned int	 numlinedefs;

	msidedef_t		*sidedef;
	unsigned int	numsidedefs;

	blockmapheader_t *blockmap;
	unsigned short	*blockmapofs;

	unsigned int	vertexsglbase;

	doomtexture_t	*textures;
	unsigned int	numtextures;
} doommap_t;

void Doom_SetModelFunc(model_t *mod);

////////////////////////////////////////////////////////////////////////////////////////////
//physics

/*walk the bsp tree*/
msector_t *Doom_SectorNearPoint(doommap_t *dm, vec3_t p)
{
	ddoomnode_t *node;
	plane_t *plane;
	int num;
	float d;
	num = dm->numnodes-1;
	while (1)
	{
		if (num & NODE_IS_SSECTOR)
		{
			num -= NODE_IS_SSECTOR;
			return dm->ssector[num].sector;
		}

		node = dm->node + num;
		plane = dm->nodeplane + num;
		
//		if (plane->type < 3)
//			d = p[plane->type] - plane->dist;
//		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->node2;
		else
			num = node->node1;
	}
	
	return NULL;
}

int Doom_PointContents(model_t *model, vec3_t axis[3], vec3_t p)
{
	doommap_t *dm = model->meshinfo;
	msector_t *sec = Doom_SectorNearPoint(dm, p);
	if (p[2] < sec->floorheight)
		return FTECONTENTS_SOLID;
	if (p[2] > sec->ceilingheight)
		return FTECONTENTS_SOLID;
	return FTECONTENTS_EMPTY;
}

/*
fixme:
use q2-style bsp collision using the trisoup for flats collisions.
use blockmap for walls
*/
qboolean Doom_Trace(model_t *model, int hulloverride, framestate_t *framestate, vec3_t axis[3], vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, qboolean iscapsule, unsigned int contentstype, trace_t *trace)
{
	doommap_t *dm = model->meshinfo;
#if 1
#define TRACESTEP	16
	unsigned short *linedefs;
	dlinedef_t *ld;
	int bmi, obmi;
	vec3_t delta;
	msector_t *sec1 = Doom_SectorNearPoint(dm, start);
	vec3_t p1, pointonplane, ofs;
	float d1, d2, c1, c2, planedist;
	plane_t *lp;
	mdoomvertex_t *v1, *v2;
	int j;
	float p1f, p2f;

	float clipfrac;
#define	DIST_EPSILON	(0.03125)

//	Con_Printf("%i\n", sec1);

	if (start[2] < sec1->floorheight-mins[2])	//whoops, started outside... ?
	{
		trace->fraction = 0;
		trace->allsolid = trace->startsolid = true;
		trace->endpos[0] = start[0];
		trace->endpos[1] = start[1];
		trace->endpos[2] = start[2];	//yeah, we do mean this - startsolid
//		if (IS_NAN(trace->endpos[2]))
//			Con_Printf("Nanny\n");
		trace->plane.normal[0] = 0;
		trace->plane.normal[1] = 0;
		trace->plane.normal[2] = 1;
		trace->plane.dist = sec1->floorheight-mins[2];

		return false;
	}
	if (start[2] > sec1->ceilingheight-maxs[2])	//whoops, started outside... ?
	{
		trace->fraction = 0;
		trace->allsolid = trace->startsolid = true;
		trace->endpos[0] = start[0];
		trace->endpos[1] = start[1];
		trace->endpos[2] = start[2];
		trace->plane.normal[0] = 0;
		trace->plane.normal[1] = 0;
		trace->plane.normal[2] = -1;
		trace->plane.dist = -(sec1->ceilingheight-maxs[2]);
		return false;
	}

	obmi = -1;
	VectorSubtract(end, start, delta);
	p2f = Length(delta)+DIST_EPSILON;
	if (IS_NAN(p2f) || p2f > 100000)
		p2f = 100000;
	VectorNormalize(delta);

	trace->endpos[0] = end[0];
	trace->endpos[1] = end[1];
	trace->endpos[2] = end[2];

	VectorCopy(start, p1);
	p1f = 0;

	trace->fraction = 1;
	while(1)
	{
		bmi = ((int)p1[0] - dm->blockmap->xorg)/128 + (((int)p1[1] - dm->blockmap->yorg)/128)*dm->blockmap->columns;
//		Con_Printf("%i of %i ", bmi, dm->blockmap->rows*dm->blockmap->columns);
		if (bmi >= 0 && bmi < dm->blockmap->rows*dm->blockmap->columns)
		if (bmi != obmi)
		{
#if 0
			short dummy;
			linedefs = &dummy;
			for (dummy = 0; dummy < dm->numlinedefs; dummy++)
#else
			for(linedefs = (short*)dm->blockmap + dm->blockmapofs[bmi]+1; *linedefs != 0xffff; linedefs++)
#endif
			{
				ld = dm->linedef + *linedefs;
				if (ld->sidedef[1] != 0xffff)
				{
					if (dm->sector[dm->sidedef[ld->sidedef[0]].sector].floorheight == dm->sector[dm->sidedef[ld->sidedef[1]].sector].floorheight &&
						dm->sector[dm->sidedef[ld->sidedef[0]].sector].ceilingheight == dm->sector[dm->sidedef[ld->sidedef[1]].sector].ceilingheight)
						continue;
				}
				
				lp = dm->lineplane + *linedefs;

				if (1)
				{	//figure out how far to move the plane out by
					for (j=0 ; j<2 ; j++)
					{
						if (lp->normal[j] < 0)
							ofs[j] = maxs[j];
						else
							ofs[j] = mins[j];
					}
					ofs[2] = 0;
					planedist = lp->dist - DotProduct (ofs, lp->normal);
				}
				else
					planedist = lp->dist;

				d1 = DotProduct(lp->normal, start) - (planedist);
				d2 = DotProduct(lp->normal, end) - (planedist);
				if (d1 > 0 && d2 > 0)
					continue;	//both points on the front side.
				if (d1 < 0)	//start on back side
				{
					if (ld->sidedef[1] != 0xffff)	//two sided (optimisation)
					{
						planedist = -planedist+lp->dist;
						if (/*d1 < planedist*-1 &&*/ d1 > planedist*2)
						{	//right, we managed to end up just on the other side of a wall's plane.
							v1 = &dm->vertexes[ld->vert[0]];
							v2 = &dm->vertexes[ld->vert[1]];
							if (!(d1 - d2))
								continue;
							if (d1<0)	//back to front.
								c1 = (d1+DIST_EPSILON) / (d1 - d2);
							else
								c1 = (d1-DIST_EPSILON) / (d1 - d2);
							c2 = 1-c1;
							pointonplane[0] = start[0]*c2 + end[0]*c1;
/*							if (pointonplane[0] > v1->xpos+DIST_EPSILON*2+hull->clip_maxs[0] && pointonplane[0] > v2->xpos+DIST_EPSILON*2+hull->clip_maxs[0])
								continue;
							if (pointonplane[0] < v1->xpos-DIST_EPSILON*2+hull->clip_mins[0] && pointonplane[0] < v2->xpos-DIST_EPSILON*2+hull->clip_mins[0])
								continue;
*/							pointonplane[1] = start[1]*c2 + end[1]*c1;
/*							if (pointonplane[1] > v1->ypos+DIST_EPSILON*2+hull->clip_maxs[1] && pointonplane[1] > v2->ypos+DIST_EPSILON*2+hull->clip_maxs[1])
								continue;
							if (pointonplane[1] < v1->ypos-DIST_EPSILON*2+hull->clip_mins[1] && pointonplane[1] < v2->ypos-DIST_EPSILON*2+hull->clip_mins[1])
								continue;
*/
							pointonplane[2] = start[2]*c2 + end[2]*c1;

							Con_Printf("Started in wall\n");
							j = dm->sidedef[ld->sidedef[d1 < planedist]].sector;
							//yup, we are in the thing
							//prevent ourselves from entering the back-sector's floor/ceiling at the point of impact
							if (pointonplane[2] < dm->sector[j].floorheight-mins[2])	//whoops, started outside... ?
							{
								Con_Printf("Started in floor\n");
								trace->allsolid = trace->startsolid = false;
								trace->endpos[2] = dm->sector[j].floorheight-mins[2];
								trace->fraction = fabs(trace->endpos[2] - start[2]) / fabs(end[2] - start[2]);
								trace->endpos[0] = start[0]+delta[0]*trace->fraction*p2f;
								trace->endpos[1] = start[1]+delta[1]*trace->fraction*p2f;
						//		if (IS_NAN(trace->endpos[2]))
						//			Con_Printf("Nanny\n");
								trace->plane.normal[0] = 0;
								trace->plane.normal[1] = 0;
								trace->plane.normal[2] = 1;
								trace->plane.dist = dm->sector[j].floorheight-mins[2];

								continue;
							}
							if (pointonplane[2] > dm->sector[j].ceilingheight-maxs[2])	//whoops, started outside... ?
							{
								Con_Printf("Started in ceiling\n");
								trace->allsolid = trace->startsolid = false;
								trace->endpos[0] = pointonplane[0];
								trace->endpos[1] = pointonplane[1];
								trace->endpos[2] = dm->sector[j].ceilingheight-maxs[2];
								trace->fraction = fabs(trace->endpos[2] - start[2]) / fabs(end[2] - start[2]);
								trace->plane.normal[0] = 0;
								trace->plane.normal[1] = 0;
								trace->plane.normal[2] = -1;
								trace->plane.dist = -(dm->sector[j].ceilingheight-maxs[2]);
								continue;
							}
						}
					}
					if (d2 < 0)
						continue;	//both points on the reverse side.
				}

				//line crosses plane.

				v1 = &dm->vertexes[ld->vert[0]];
				v2 = &dm->vertexes[ld->vert[1]];

				if (d1<0)	//back to front.
				{
					if (ld->sidedef[1] == 0xffff)
						continue;	//hack to allow them to pass
					c1 = (d1+DIST_EPSILON) / (d1 - d2);
				}
				else
					c1 = (d1-DIST_EPSILON) / (d1 - d2);
				c2 = 1-c1;
				pointonplane[0] = start[0]*c2 + end[0]*c1;
				if (pointonplane[0] > v1->xpos+DIST_EPSILON*2+maxs[0] && pointonplane[0] > v2->xpos+DIST_EPSILON*2+maxs[0])
					continue;
				if (pointonplane[0] < v1->xpos-DIST_EPSILON*2+mins[0] && pointonplane[0] < v2->xpos-DIST_EPSILON*2+mins[0])
					continue;
				pointonplane[1] = start[1]*c2 + end[1]*c1;
				if (pointonplane[1] > v1->ypos+DIST_EPSILON*2+maxs[1] && pointonplane[1] > v2->ypos+DIST_EPSILON*2+maxs[1])
					continue;
				if (pointonplane[1] < v1->ypos-DIST_EPSILON*2+mins[1] && pointonplane[1] < v2->ypos-DIST_EPSILON*2+mins[1])
					continue;
				pointonplane[2] = start[2]*c2 + end[2]*c1;

				if (ld->flags & LINEDEF_IMPASSABLE || ld->sidedef[1] == 0xffff)	//unconditionally unpassable.
				{	//unconditionally clipped.
				}
				else
				{	//ensure that the side we are passing on to passes the clip (no ceiling/floor clips happened first)
					msector_t *sec2;

					if (d1<0)
						sec2 = &dm->sector[dm->sidedef[ld->sidedef[1]].sector];
					else
						sec2 = &dm->sector[dm->sidedef[ld->sidedef[0]].sector];

					if (pointonplane[2] < sec2->floorheight-mins[2])
					{	//hit the floor first.
						c1 = fabs(sec1->floorheight-mins[2] - start[2]);
						c2 = fabs(end[2] - start[2]);
						if (!c2)
							c1 = 1;
						else
							c1 = (c1-DIST_EPSILON) / c2;
						if (trace->fraction > c1)
						{
//							Con_Printf("Hit floor\n");
							trace->fraction = c1;
							trace->allsolid = trace->startsolid = true;
							trace->endpos[0] = start[0] + trace->fraction*(end[0]-start[0]);
							trace->endpos[1] = start[1] + trace->fraction*(end[1]-start[1]);
							trace->endpos[2] = start[2] + trace->fraction*(end[2]-start[2]);
							trace->plane.normal[0] = 0;
							trace->plane.normal[1] = 0;
							trace->plane.normal[2] = 1;
							trace->plane.dist = sec1->floorheight-mins[2];
						}
						continue;
					}

					if (pointonplane[2] > sec2->ceilingheight-maxs[2])
					{	//hit the floor first.
						c1 = fabs((sec1->ceilingheight-maxs[2]) - start[2]);
						c2 = fabs(end[2] - start[2]);
						if (!c2)
							c1 = 1;
						else
							c1 = (c1-DIST_EPSILON) / c2;


						if (trace->fraction > c1)
						{
//							Con_Printf("Hit ceiling\n");
							trace->fraction = c1;
							trace->allsolid = trace->startsolid = true;
							trace->endpos[0] = start[0] + trace->fraction*(end[0]-start[0]);
							trace->endpos[1] = start[1] + trace->fraction*(end[1]-start[1]);
							trace->endpos[2] = start[2] + trace->fraction*(end[2]-start[2]);
							trace->plane.normal[0] = 0;
							trace->plane.normal[1] = 0;
							trace->plane.normal[2] = -1;
							trace->plane.dist = -(sec1->ceilingheight-maxs[2]);
						}
						continue;
					}

					if (d1<0)
						sec2 = &dm->sector[dm->sidedef[ld->sidedef[0]].sector];
					else
						sec2 = &dm->sector[dm->sidedef[ld->sidedef[1]].sector];

					if(sec2->ceilingheight == sec2->floorheight)
						sec2->ceilingheight += 64;

					if (pointonplane[2] > sec2->floorheight-mins[2] &&
						pointonplane[2] < sec2->ceilingheight-maxs[2])
					{
//						Con_Printf("Two sided passed\n");
						continue;
					}

//					Con_Printf("blocked by two sided line\n");
//					sec2->floorheight--;
				}

				if (d1<0)	//back to front.
					c1 = (d1+DIST_EPSILON) / (d1 - d2);
				else
					c1 = (d1-DIST_EPSILON) / (d1 - d2);


				clipfrac = c1;

				if (clipfrac < 0)
					clipfrac = 0;
				if (clipfrac > 1)
					clipfrac = 1;

				if (trace->fraction > clipfrac)
				{
					trace->fraction = clipfrac;
					VectorMA(pointonplane, 0, lp->normal, trace->endpos);
					VectorMA(trace->endpos, -0.1, delta, trace->endpos);
//					if (IS_NAN(trace->endpos[2]))
//						Con_Printf("Buggy clipping\n");
					VectorCopy(lp->normal, trace->plane.normal);
					trace->plane.dist = planedist;
//					if (IS_NAN(trace->plane.normal[2]))
//						Con_Printf("Buggy clipping\n");

					if (clipfrac)
					Con_Printf("Clip Wall %f\n", clipfrac);
				}
			}
			
			obmi = bmi;
		}

		p1f += TRACESTEP;
		if (p1f >= p2f)
			break;

		VectorMA(p1, TRACESTEP, delta, p1);
	}

//	VectorMA(start, p2f*trace->fraction, delta, p2);

	if (end[2] != start[2])
	{
		if (sec1 == Doom_SectorNearPoint(dm, trace->endpos))	//special test.
		{
			if (end[2] <= sec1->floorheight-mins[2])	//whoops, started outside... ?
			{
				p1f = fabs(sec1->floorheight-mins[2] - start[2]);
				p2f = fabs(end[2] - start[2]);
				if (!p2f)
					c1 = 1;
				else
					c1 = (p1f-DIST_EPSILON) / p2f;
				if (trace->fraction > c1)
				{
					trace->fraction = c1;
					trace->allsolid = trace->startsolid = false;
					trace->endpos[0] = start[0] + trace->fraction*(end[0]-start[0]);
					trace->endpos[1] = start[1] + trace->fraction*(end[1]-start[1]);
					trace->endpos[2] = start[2] + trace->fraction*(end[2]-start[2]);
					trace->plane.normal[0] = 0;
					trace->plane.normal[1] = 0;
					trace->plane.normal[2] = 1;
					trace->plane.dist = sec1->floorheight-mins[2];
				}

//				if (IS_NAN(trace->endpos[2]))
//					Con_Printf("Nanny\n");
			}
			if (end[2] >= sec1->ceilingheight-maxs[2])	//whoops, started outside... ?
			{
				p1f = fabs(sec1->ceilingheight-maxs[2] - start[2]);
				p2f = fabs(end[2] - start[2]);
				if (!p2f)
					c1 = 1;
				else
					c1 = (p1f-DIST_EPSILON) / p2f;
				if (trace->fraction > c1)
				{
					trace->fraction = c1;
					trace->allsolid = trace->startsolid = false;
					trace->endpos[0] = start[0] + trace->fraction*(end[0]-start[0]);
					trace->endpos[1] = start[1] + trace->fraction*(end[1]-start[1]);
					trace->endpos[2] = start[2] + trace->fraction*(end[2]-start[2]);
					trace->plane.normal[0] = 0;
					trace->plane.normal[1] = 0;
					trace->plane.normal[2] = -1;
					trace->plane.dist = -(sec1->ceilingheight-maxs[2]);
				}

//				if (IS_NAN(trace->endpos[2]))
//					Con_Printf("Nanny\n");
			}
		}
	}

	//we made it all the way through. yay.

	trace->allsolid = trace->startsolid = false;
//Con_Printf("total = %f\n", trace->fraction);
#endif
	return trace->fraction==1;
}

















#ifndef SERVERONLY
qbyte doompalette[768];
static qboolean paletteloaded;

void Doom_LoadPalette(void)
{
	char *file;
	int greyscale;
	if (!paletteloaded)
	{
		paletteloaded = true;
		file = FS_LoadMallocFile("wad/playpal", NULL);
		if (file)
		{
			memcpy(doompalette, file, 768);
			Z_Free(file);
		}
		else
		{
			for (greyscale = 0; greyscale < 256; greyscale++)
			{
				doompalette[greyscale*3+0] = greyscale;
				doompalette[greyscale*3+1] = greyscale;
				doompalette[greyscale*3+2] = greyscale;
			}
		}
	}
}
#endif
int Doom_LoadFlat(doommap_t *dm, char *flatname)
{
#ifndef SERVERONLY
	char texname[64];
	int texnum;

	sprintf(texname, "flats/%-.8s", flatname);

	for (texnum = 0; texnum < dm->numtextures; texnum++)
	{
		if (!strcmp(dm->textures[texnum].name, texname))
			return texnum;
	}
	
	dm->textures = BZ_Realloc(dm->textures, sizeof(*dm->textures)*((dm->numtextures+16)&~15));
	memset(dm->textures + dm->numtextures, 0, sizeof(dm->textures[dm->numtextures]));
	dm->numtextures++;

	Q_strncpyz(dm->textures[texnum].name, texname, sizeof(dm->textures[texnum].name));

	dm->textures[texnum].width = 64;
	dm->textures[texnum].height = 64;

	return texnum;
#else
	return 0;
#endif
}

#ifndef SERVERONLY
static void R_DrawWall(doommap_t *dm, int texnum, int s, int t, float x1, float y1, float z1, float x2, float y2, float z2, qboolean unpegged, unsigned int colour4b)
{
	doomtexture_t *tex = dm->textures+texnum;
	mesh_t *mesh = &tex->mesh;
	float len = sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
	float s1, s2;
	float t1, t2;
	unsigned int col;

	s1 = s/tex->width;
	s2 = s1 + len/tex->width;

	if (unpegged)
	{
		t2 = t/tex->height;
		t1 = t2 - (z2-z1)/tex->height;
	}
	else
	{
		t1 = t/tex->height;
		t2 = t1 + (z2-z1)/tex->height;
	}
	t1 = 0;
	t2 = 1;

	if (mesh->numvertexes+4 > tex->maxverts)
	{
		if (mesh->numvertexes+4 > MAX_INDICIES)
			BE_DrawMesh_Single(tex->shader, mesh, NULL, 0);
		else
		{
			tex->maxverts = mesh->numvertexes+4;
			mesh->colors4b_array = BZ_Realloc(mesh->colors4b_array, sizeof(*mesh->colors4b_array) * tex->maxverts);
			mesh->xyz_array = BZ_Realloc(mesh->xyz_array, sizeof(*mesh->xyz_array) * tex->maxverts);
			mesh->st_array = BZ_Realloc(mesh->st_array, sizeof(*mesh->st_array) * tex->maxverts);
		}
	}
	if (mesh->numindexes+6 > tex->maxindicies)
	{
		tex->maxindicies = mesh->numindexes+6;
		mesh->indexes = BZ_Realloc(mesh->indexes, sizeof(*mesh->indexes) * tex->maxindicies);
	}

	col = colour4b * 0x01010101;
	((unsigned char*)&col)[3] = 0xff;
	*(unsigned int*)mesh->colors4b_array[mesh->numvertexes+0] = col;
	*(unsigned int*)mesh->colors4b_array[mesh->numvertexes+1] = col;
	*(unsigned int*)mesh->colors4b_array[mesh->numvertexes+2] = col;
	*(unsigned int*)mesh->colors4b_array[mesh->numvertexes+3] = col;
	VectorSet(mesh->xyz_array[mesh->numvertexes+0], x1, y1, z1);
	VectorSet(mesh->xyz_array[mesh->numvertexes+1], x1, y1, z2);
	VectorSet(mesh->xyz_array[mesh->numvertexes+2], x2, y2, z2);
	VectorSet(mesh->xyz_array[mesh->numvertexes+3], x2, y2, z1);
	Vector2Set(mesh->st_array[mesh->numvertexes+0], s1, t2);
	Vector2Set(mesh->st_array[mesh->numvertexes+1], s1, t1);
	Vector2Set(mesh->st_array[mesh->numvertexes+2], s2, t1);
	Vector2Set(mesh->st_array[mesh->numvertexes+3], s2, t2);

	mesh->indexes[mesh->numindexes+0] = mesh->numvertexes+0;
	mesh->indexes[mesh->numindexes+1] = mesh->numvertexes+1;
	mesh->indexes[mesh->numindexes+2] = mesh->numvertexes+2;

	mesh->indexes[mesh->numindexes+3] = mesh->numvertexes+0;
	mesh->indexes[mesh->numindexes+4] = mesh->numvertexes+2;
	mesh->indexes[mesh->numindexes+5] = mesh->numvertexes+3;

	mesh->numvertexes += 4;
	mesh->numindexes += 6;
}

static void R_DrawFlats(doommap_t *dm, int floortexnum, int floorheight, int ceiltexnum, int ceilheight, int numverts, unsigned short *verts, unsigned int colour4b)
{
	mesh_t *mesh;
	unsigned int col;
	unsigned int v, i;

	//floor
	{
		doomtexture_t *floortex = dm->textures + floortexnum;
		mesh = &floortex->mesh;
		if (mesh->numvertexes+numverts > floortex->maxverts)
		{
			if (mesh->numvertexes+numverts > MAX_INDICIES)
			{
				BE_DrawMesh_Single(floortex->shader, mesh, NULL, 0);
				mesh->numvertexes = 0;
				mesh->numindexes = 0;
			}
			else
			{
				floortex->maxverts = mesh->numvertexes+numverts;
				mesh->colors4b_array = BZ_Realloc(mesh->colors4b_array, sizeof(*mesh->colors4b_array) * floortex->maxverts);
				mesh->xyz_array = BZ_Realloc(mesh->xyz_array, sizeof(*mesh->xyz_array) * floortex->maxverts);
				mesh->st_array = BZ_Realloc(mesh->st_array, sizeof(*mesh->st_array) * floortex->maxverts);
			}
		}
		if (mesh->numindexes+numverts > floortex->maxindicies)
		{
			floortex->maxindicies = mesh->numindexes+numverts;
			mesh->indexes = BZ_Realloc(mesh->indexes, sizeof(*mesh->indexes) * floortex->maxindicies);
		}

		col = colour4b * 0x01010101;
		((unsigned char*)&col)[3] = 0xff;

		for (i = 0; i < numverts; i++)
		{
			v = verts[i];
			VectorSet(mesh->xyz_array[mesh->numvertexes+i], dm->vertexes[v].xpos, dm->vertexes[v].ypos, floorheight);
			Vector2Set(mesh->st_array[mesh->numvertexes+i], dm->vertexes[v].xpos/64.0f, dm->vertexes[v].ypos/64.0f);
			*(unsigned int*)mesh->colors4b_array[mesh->numvertexes+i] = col;
		}

		for (i = 0; i < numverts; i++)
		{
			mesh->indexes[mesh->numindexes+i] = mesh->numvertexes+i;
		}

		mesh->numvertexes += numverts;
		mesh->numindexes += numverts;

//		if (floortex->shader)
//			BE_DrawMesh_Single(floortex->shader, mesh, NULL, 0);
	}

	//ceiling
	{
		doomtexture_t *ceiltex = dm->textures + ceiltexnum;
		mesh = &ceiltex->mesh;
		if (mesh->numvertexes+numverts > ceiltex->maxverts)
		{
			if (mesh->numvertexes+numverts > MAX_INDICIES)
			{
				BE_DrawMesh_Single(ceiltex->shader, mesh, NULL, 0);
				mesh->numvertexes = 0;
				mesh->numindexes = 0;
			}
			else
			{
				ceiltex->maxverts = mesh->numvertexes+numverts;
				mesh->colors4b_array = BZ_Realloc(mesh->colors4b_array, sizeof(*mesh->colors4b_array) * ceiltex->maxverts);
				mesh->xyz_array = BZ_Realloc(mesh->xyz_array, sizeof(*mesh->xyz_array) * ceiltex->maxverts);
				mesh->st_array = BZ_Realloc(mesh->st_array, sizeof(*mesh->st_array) * ceiltex->maxverts);
			}
		}
		if (mesh->numindexes+numverts > ceiltex->maxindicies)
		{
			ceiltex->maxindicies = mesh->numindexes+numverts;
			mesh->indexes = BZ_Realloc(mesh->indexes, sizeof(*mesh->indexes) * ceiltex->maxindicies);
		}

		col = colour4b * 0x01010101;
		((unsigned char*)&col)[3] = 0xff;

		for (i = 0; i < numverts; i++)
		{
			v = verts[numverts-1-i];
			VectorSet(mesh->xyz_array[mesh->numvertexes+i], dm->vertexes[v].xpos, dm->vertexes[v].ypos, ceilheight);
			Vector2Set(mesh->st_array[mesh->numvertexes+i], dm->vertexes[v].xpos/64.0f, dm->vertexes[v].ypos/64.0f);
			*(unsigned int*)mesh->colors4b_array[mesh->numvertexes+i] = col;
		}

		for (i = 0; i < numverts; i++)
		{
			mesh->indexes[mesh->numindexes+i] = mesh->numvertexes+i;
		}

		mesh->numvertexes += numverts;
		mesh->numindexes += numverts;

//		if (ceiltex->shader)
//			BE_DrawMesh_Single(ceiltex->shader, mesh, NULL, 0);
	}
}

static void R_DrawSSector(doommap_t *dm, unsigned int ssec)
{
	short v0, v1;
	int sd;
	dlinedef_t *ld;
	int seg;
	msector_t *sec, *sec2;

	for (seg = dm->ssector[ssec].first + dm->ssector[ssec].segcount-1; seg >= dm->ssector[ssec].first; seg--)
		if (dm->seg[seg].linedef != 0xffff)
			break;
	sec = dm->sector + dm->sidedef[dm->linedef[dm->seg[seg].linedef].sidedef[dm->seg[seg].direction]].sector;

	if (sec->visframe != r_visframecount)
	{
		R_DrawFlats(dm, sec->floortex, sec->floorheight, sec->ceilingtex, sec->ceilingheight, sec->numflattris*3, sec->flats, sec->lightlev);

		sec->visframe = r_visframecount;
	}
	for (seg = dm->ssector[ssec].first + dm->ssector[ssec].segcount-1; seg >= dm->ssector[ssec].first; seg--)
	{
		if (dm->seg[seg].linedef == 0xffff)
			continue;

		v0 = dm->seg[seg].vert[0];
		v1 = dm->seg[seg].vert[1];
		if (v0==v1)
			continue;
		ld = dm->linedef + dm->seg[seg].linedef;
		sd = ld->sidedef[dm->seg[seg].direction];

		if (ld->sidedef[1] != 0xffff)	//we can see through this linedef
		{
			sec2 = dm->sector + dm->sidedef[ld->sidedef[1-dm->seg[seg].direction]].sector;

			if (sec->floorheight < sec2->floorheight)
			{
				R_DrawWall(dm,
					dm->sidedef[sd].lowertex, 
					dm->sidedef[ld->sidedef[1-dm->seg[seg].direction]].texx,
					dm->sidedef[ld->sidedef[1-dm->seg[seg].direction]].texy,
					dm->vertexes[v0].xpos, dm->vertexes[v0].ypos, sec->floorheight,
					dm->vertexes[v1].xpos, dm->vertexes[v1].ypos, sec2->floorheight, ld->flags & LINEDEF_LOWERUNPEGGED, sec->lightlev);
			}

			if (sec->ceilingheight > sec2->ceilingheight)
			{
				R_DrawWall(dm,
					dm->sidedef[sd].uppertex, 
					dm->sidedef[ld->sidedef[1-dm->seg[seg].direction]].texx,
					dm->sidedef[ld->sidedef[1-dm->seg[seg].direction]].texy,
					dm->vertexes[v0].xpos, dm->vertexes[v0].ypos, sec2->ceilingheight,
					dm->vertexes[v1].xpos, dm->vertexes[v1].ypos, sec->ceilingheight, ld->flags & LINEDEF_UPPERUNPEGGED, sec->lightlev);
			}

			if (dm->sidedef[sd].middletex)
			{
				R_DrawWall(dm,
					dm->sidedef[sd].middletex, 
					dm->sidedef[ld->sidedef[dm->seg[seg].direction]].texx,
					dm->sidedef[ld->sidedef[dm->seg[seg].direction]].texy,
					dm->vertexes[v1].xpos, dm->vertexes[v1].ypos, (sec2->ceilingheight < sec->ceilingheight)?sec2->ceilingheight:sec->ceilingheight,
					dm->vertexes[v0].xpos, dm->vertexes[v0].ypos, (sec2->floorheight > sec->floorheight)?sec2->floorheight:sec->floorheight, false, sec->lightlev);
			}
		}
		else
		{	//solid wall, draw full wall.

			R_DrawWall(dm,
				dm->sidedef[sd].middletex, 
				dm->sidedef[ld->sidedef[dm->seg[seg].direction]].texx,
				dm->sidedef[ld->sidedef[dm->seg[seg].direction]].texy,
				dm->vertexes[v0].xpos, dm->vertexes[v0].ypos, sec->floorheight,
				dm->vertexes[v1].xpos, dm->vertexes[v1].ypos, sec->ceilingheight, false, sec->lightlev);
		}
	}
}

mplane_t	frustum2d[2];
static int Box2DOnPlaneSide (short emins[2], short emaxs[2], mplane_t *p)
{
	float	dist1, dist2;
	int		sides;

// general case
	switch (p->signbits)
	{
	case 0:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1];
		break;
	case 1:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1];
		break;
	case 2:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1];
		break;
	case 3:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1];
		break;
	case 4:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1];
		break;
	case 5:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1];
		break;
	case 6:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1];
		break;
	case 7:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
//		BOPS_Error ();
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

#ifdef PARANOID
if (sides == 0)
	Sys_Error ("Box2DOnPlaneSide: sides==0");
#endif

	return sides;
}
static qboolean R_Cull2DBox (short mins_x, short mins_y, short maxs_x, short maxs_y)
{
	short mins[2], maxs[2];
	int		i;
//return false;
	mins[0] = mins_x;
	mins[1] = mins_y;
	maxs[0] = maxs_x;
	maxs[1] = maxs_y;

	for (i=0 ; i<2 ; i++)
		if (Box2DOnPlaneSide (mins, maxs, &frustum2d[i]) == 2)
			return true;
	return false;
}

void R_Set2DFrustum (void)
{
	int		i;
	vec3_t vpn, vright, vup, viewang;

	if ((int)r_novis.value & 4)
		return;

	viewang[0] = 0;
	viewang[1] = r_refdef.viewangles[1];
	viewang[2] = 0;
	AngleVectors (viewang, vpn, vright, vup);

/*	if (r_refdef.fov_x == 90) 
	{
		// front side is visible

		VectorAdd (vpn, vright, frustum2d[0].normal);
		VectorSubtract (vpn, vright, frustum2d[1].normal);
	}
	else*/
	{

		// rotate VPN right by FOV_X/2 degrees
		RotatePointAroundVector( frustum2d[0].normal, vup, vpn, -(90-r_refdef.fov_x / 2 ) );
		// rotate VPN left by FOV_X/2 degrees
		RotatePointAroundVector( frustum2d[1].normal, vup, vpn, 90-r_refdef.fov_x / 2 );
	}

	for (i=0 ; i<2 ; i++)
	{
		frustum2d[i].type = PLANE_ANYZ;
		frustum2d[i].dist = DotProduct (r_origin, frustum2d[i].normal);
		frustum2d[i].signbits = SignbitsForPlane (&frustum2d[i]);
	}
}


static void R_RecursiveDoomNode(doommap_t *dm, unsigned int node)
{
	if (node & NODE_IS_SSECTOR)
	{
		R_DrawSSector(dm, node & ~NODE_IS_SSECTOR);		

		return;
	}

	if (!R_Cull2DBox(dm->node[node].x1lower, dm->node[node].y1lower, dm->node[node].x1upper, dm->node[node].y1upper)||1)
		R_RecursiveDoomNode(dm, dm->node[node].node1);
	if (!R_Cull2DBox(dm->node[node].x2lower, dm->node[node].y2lower, dm->node[node].x2upper, dm->node[node].y2upper)||1)
		R_RecursiveDoomNode(dm, dm->node[node].node2);
}

void R_DoomWorld(void)
{
	model_t *mod = cl.worldmodel;
	doommap_t *dm = mod->meshinfo;
	int texnum;
	doomtexture_t *t;
	if (!dm->node || !dm->numnodes)
		return;	//err... buggy

	for (texnum = 0; texnum < dm->numtextures; texnum++)	//a hash table might be a good plan.
	{
		t = &dm->textures[texnum];
		t->mesh.numindexes = 0;
		t->mesh.numvertexes = 0;
	}
	r_visframecount++;
	R_RecursiveDoomNode(dm, dm->numnodes-1);

	memset(mod->batches, 0, sizeof(mod->batches));
	for (texnum = 0; texnum < dm->numtextures; texnum++)	//a hash table might be a good plan.
	{
		t = &dm->textures[texnum];
		if (t->mesh.numindexes && t->shader)
		{
			t->batch.next = mod->batches[t->shader->sort];
			mod->batches[t->shader->sort] = &t->batch;

			BE_DrawMesh_Single(t->shader, &t->mesh, NULL, 0);
		}
	}
}
#endif


//find the first ssector, go through it's list/
//grab the lines into multiple arrays.
//make sure all arrays are looped fully. If not, error out.
//if we have two arrays, we have a hole in the middle.
//with multiple arrays, from the second onwards
//	grab two adjacent verts and find the nearest point in any other array, that is also on the positive side.
//	One of the two should be an extreeme, and the external point should be in the direction that the angle points at.
//		none found = error
//	create a triangle from these points, fix array links.
//	move on to next spare array.
//we now have a concave polygon with no holes.
//pick a point, follow along the walls making a triangle fan, until an angle of > 180, throw out fan, rebuild arrays.
//at new point, start a new fan. Be prepared to not be able to generate one.

#define MAX_REGIONS		256
#define MAX_POLYVERTS	(MAX_FLATTRIS*3)
#define MAX_FLATTRIS	1024

//buffer to hold tris
static unsigned short indexes[MAX_POLYVERTS];
static unsigned int numindexes;

typedef struct {
	int vertex[MAX_POLYVERTS];
	unsigned int numverts;
	float angle;
} conectedregion_t;
static conectedregion_t polyregions[MAX_REGIONS];	//we need to be able to join them as we go.
static unsigned int regions;

//throw out duplicates.
static void Triangulate_AddLine(int v1, int v2)	//order makes a difference
{
	int r, v;
	int beginingof = -1;
	int endof = -1;
	int freer = -1;

	for (r = 0; r < regions; r++)
	{
		if (!polyregions[r].numverts)
		{
			freer = r;
			continue;
		}
		if (polyregions[r].vertex[0] == v2)
			beginingof = r;
		if (polyregions[r].vertex[polyregions[r].numverts-1] == v1)
			endof = r;

		for (v = polyregions[r].numverts-2; v >= 0; v--)
			if (polyregions[r].vertex[v] == v1 && polyregions[r].vertex[v+1] == v2)
				return;	//whoops. Duplicate line.
	}
	if (beginingof >= 0 && endof >= 0)
	{	//merge two regions. Copy one onto the end of the other.
		if (beginingof == endof)
		{	//close up
			if (polyregions[endof].numverts+1 >= MAX_POLYVERTS)
			{
				Con_Printf(CON_WARNING "WARNING: Map region is too large.\n");
				return;
			}
			polyregions[endof].vertex[polyregions[endof].numverts] = v2;
			polyregions[endof].numverts++;
		}
		else
		{
			if (polyregions[endof].numverts+polyregions[beginingof].numverts >= MAX_POLYVERTS)
			{
				Con_Printf(CON_WARNING "WARNING: Map region is too large.\n");
				return;
			}
			memcpy(polyregions[endof].vertex + polyregions[endof].numverts,
				polyregions[beginingof].vertex,
				sizeof(polyregions[beginingof].vertex[0])*polyregions[beginingof].numverts);
			polyregions[endof].numverts += polyregions[beginingof].numverts;
			polyregions[beginingof].numverts = 0;
		}
	}
	else if (beginingof >= 0)
	{	//insert into
		if (polyregions[beginingof].numverts+1 >= MAX_POLYVERTS)
		{
			Con_Printf(CON_WARNING "WARNING: Map region is too large.\n");
			return;
		}

		memmove(polyregions[beginingof].vertex + 1,
			polyregions[beginingof].vertex,
			sizeof(polyregions[beginingof].vertex[0])*polyregions[beginingof].numverts);
		polyregions[beginingof].vertex[0] = v1;
		polyregions[beginingof].numverts++;
	}
	else if (endof >= 0)
	{	//stick outselves on the end
		if (polyregions[endof].numverts+1 >= MAX_POLYVERTS)
		{
			Con_Printf(CON_WARNING "WARNING: Map region is too large.\n");
			return;
		}
		polyregions[endof].vertex[polyregions[endof].numverts] = v2;
		polyregions[endof].numverts++;
	}
	else
	{	//new region.
		if (freer < 0)
		{
			freer = regions++;
			if (regions > MAX_REGIONS)
			{
				Con_Printf(CON_WARNING "WARNING: Too many regions. Sector is too chaotic/complicated.\n");
				freer = 0;
				regions = 1;
			}
		}

		polyregions[freer].numverts = 2;
		polyregions[freer].vertex[0] = v1;
		polyregions[freer].vertex[1] = v2;
	}
}

static unsigned short *Triangulate_Finish(doommap_t *dm, int *numtris, unsigned short *old, int oldindexcount)
{
	unsigned short *out;
	unsigned int v1, v2, v3, v;
	unsigned int r, v2s, f;
	float a1;
	float a2;
	for (r = 0; r < regions; r++)
	{
		if (!polyregions[r].numverts)
			continue;

		if (polyregions[r].vertex[0] != polyregions[r].vertex[polyregions[r].numverts-1])
		{
			Con_Printf("Sector is not enclosed\n");
			polyregions[r].vertex[polyregions[r].numverts] = polyregions[r].vertex[0];
			polyregions[r].numverts++;

			/*
			*numtris = 0;
			regions = 0;


			return NULL;*/
		}

		polyregions[r].angle = 0;
		polyregions[r].numverts--;//start == end
		for (v = 0; v < polyregions[r].numverts; v++)
		{
			v1 = polyregions[r].vertex[v];
			v2 = polyregions[r].vertex[(v+1)%(polyregions[r].numverts)];
			v3 = polyregions[r].vertex[(v+2)%(polyregions[r].numverts)];
			a1 = atan2(dm->vertexes[v3].ypos - dm->vertexes[v2].ypos, dm->vertexes[v3].xpos - dm->vertexes[v2].xpos);
			a2 = atan2(dm->vertexes[v1].ypos - dm->vertexes[v2].ypos, dm->vertexes[v1].xpos - dm->vertexes[v2].xpos);
			polyregions[r].angle += fabs(a1 - a2);
		}
	}

	//FIXME: inner loops should find the nearest point in a forwards direction from one of the extreeme points.

	//angle should be either (numverts-2)*PI	//inner loop
	//or PI*numverts+2*PI						//outer loop
	//unfortuantly it's rarly either of them...

	for (r = 0; r < regions; r++)
	{
		if (polyregions[r].numverts<3)
			continue;
		v1 = polyregions[r].vertex[0];
		v2 = polyregions[r].vertex[1];
		v2s = 1;
		f=0;
		for (v = 2; polyregions[r].numverts>=3; )
		{	//build a triangle fan.
			if (numindexes+3 > MAX_POLYVERTS)
			{
				Con_Printf(CON_WARNING "WARNING: Sector is too big for triangulation\n");
				break;
			}
			v3 = polyregions[r].vertex[v];

			a1 = atan2(dm->vertexes[v3].ypos - dm->vertexes[v2].ypos, dm->vertexes[v3].xpos - dm->vertexes[v2].xpos);
			a2 = atan2(dm->vertexes[v1].ypos - dm->vertexes[v2].ypos, dm->vertexes[v1].xpos - dm->vertexes[v2].xpos);
			if (fabs(a1-a2) > M_PI+0.01)	//this would be a reflex angle then.;.
			{
/*				indexes[numindexes++] = 0;
				indexes[numindexes++] = v2;
				indexes[numindexes++] = 1;
*/
				v1 = v2;
				v2 = v3;
				v2s = v;
				v=(v+1)%polyregions[r].numverts;
				f++;
				if (f >= 1000)
				{	//infinate loop - shouldn't happen. must have got the angle stuff wrong.
					Con_Printf(CON_WARNING "WARNING: Failed to triangulate polygon\n");
					break;
				}
				continue;
			}

			//FIXME: make sure v1 -> v3 doesn't clip any same-region lines.

			indexes[numindexes++] = v1;
			indexes[numindexes++] = v2;
			indexes[numindexes++] = v3;
			memmove(polyregions[r].vertex+v2s, polyregions[r].vertex+v2s+1, (polyregions[r].numverts-v2s)*sizeof(polyregions[r].vertex[0]));
			polyregions[r].numverts--;
			v=(v)%polyregions[r].numverts;
			v2 = v3;
			v2s = v;
			polyregions[r].vertex[polyregions[r].numverts] = 0;
		}
	}

	if (!numindexes)
	{
		Con_Printf(CON_WARNING "Warning: Sector is empty\n");

		*numtris = 0;
		regions = 0;

		return NULL;
	}

	out = BZ_Realloc(old, sizeof(*out)*(numindexes+oldindexcount*3));
	memcpy(out+oldindexcount*3, indexes, sizeof(*out)*numindexes);

	*numtris = numindexes/3+oldindexcount;
	regions = 0;
	numindexes = 0;

	return out;
}

static void Triangulate_Sectors(doommap_t *dm, dsector_t *sectorl, qboolean glbspinuse)
{
	int seg, nsec;
	int i, sec=-1;

	if (glbspinuse)
	{
		for (i = 0; i < dm->numssectors; i++)
		{	//only do linedefs.
			for (seg = dm->ssector[i].first; seg < dm->ssector[i].first + dm->ssector[i].segcount; seg++)
				if (dm->seg[seg].linedef != 0xffff)
					break;

			if (seg == dm->ssector[i].first + dm->ssector[i].segcount)	//throw a fit.
			{
				Con_Printf("SubSector %i has absolutly no walls\n", i);
				continue;
			}
				
			nsec = dm->sidedef[dm->linedef[dm->seg[seg].linedef].sidedef[dm->seg[seg].direction]].sector;
			if (sec != nsec)
			{
				if (sec>=0)
					dm->sector[sec].flats = Triangulate_Finish(dm, &dm->sector[sec].numflattris, dm->sector[sec].flats, dm->sector[sec].numflattris);
				sec = nsec;
			}
			for (seg = dm->ssector[i].first; seg < dm->ssector[i].first + dm->ssector[i].segcount; seg++)
			{	//ignore direction, it's do do with the intersection rather than the draw direction.
				Triangulate_AddLine(dm->seg[seg].vert[0], dm->seg[seg].vert[1]);
			}
		}
		if (sec>=0)
			dm->sector[sec].flats = Triangulate_Finish(dm, &dm->sector[sec].numflattris, dm->sector[sec].flats, dm->sector[sec].numflattris);
	}
	else
	{
		for (sec = 0; sec < dm->numsectors; sec++)
		{
			for (i = 0; i < dm->numlinedefs; i++)
			{
				if (dm->sidedef[dm->linedef[i].sidedef[0]].sector == sec)
					Triangulate_AddLine(dm->linedef[i].vert[0], dm->linedef[i].vert[1]);
				if (dm->linedef[i].sidedef[1] != 0xffff && dm->sidedef[dm->linedef[i].sidedef[1]].sector == sec)
					Triangulate_AddLine(dm->linedef[i].vert[1], dm->linedef[i].vert[0]);
			}
			dm->sector[sec].flats = Triangulate_Finish(dm, &dm->sector[sec].numflattris, dm->sector[sec].flats, dm->sector[sec].numflattris);
		}
	}

	/*
	for (i = 0; i < ssectorsc; i++)
	{	//only do linedefs.
		seg = dm->ssector[i].first;
		nsec = dm->sidedef[dm->linedef[dm->seg[seg].linedef].sidedef[dm->seg[seg].direction]].sector;
		if (sec != nsec)
		{
			if (sec>=0)
				dm->sector[sec].flats = Triangulate_Finish(&dm->sector[sec].numflattris);
			sec = nsec;
		}
		for (seg = dm->ssector[i].first; seg < dm->ssector[i].first + dm->ssector[i].segcount; seg++)
		{	//ignore direction, it's do do with the intersection rather than the draw direction.
			Triangulate_AddLine(dm->seg[seg].vert[0], dm->seg[seg].vert[1]);
		}
	}
	if (sec>=0)
		dm->sector[sec].flats = Triangulate_Finish(&dm->sector[sec].numflattris);
	*/

	for (i = 0; i < dm->numsectors; i++)
	{
		dm->sector[i].ceilingtex = Doom_LoadFlat(dm, sectorl[i].ceilingtexture);
		dm->sector[i].floortex = Doom_LoadFlat(dm, sectorl[i].floortexture);
		dm->sector[i].lightlev = sectorl[i].lightlevel;
		dm->sector[i].specialtype = sectorl[i].specialtype;
		dm->sector[i].tag = sectorl[i].tag;
		dm->sector[i].ceilingheight = sectorl[i].ceilingheight;
		dm->sector[i].floorheight = sectorl[i].floorheight;
	}
}

#ifndef SERVERONLY
static void *textures1;
static void *textures2;
static char *pnames;
static void Doom_LoadTextureInfos(void)
{
	textures1 = FS_LoadMallocFile("wad/texture1", NULL);
	textures2 = FS_LoadMallocFile("wad/texture2", NULL);
	pnames = FS_LoadMallocFile("wad/pnames", NULL);
}

typedef struct {
	char name[8];
	short always0_0;
	short always0_1;
	short width;
	short height;
	short always0_2;
	short always0_3;
	short componantcount;
} ddoomtexture_t;
typedef struct {
	short xoffset;
	short yoffset;
	unsigned short patchnum;
	unsigned short always_1;
	unsigned short always_0;
} ddoomtexturecomponant_t;

typedef struct {
	short width;
	short height;
	short xpos;
	short ypos;
} doomimage_t;

static void Doom_ExtractPName(unsigned int *out, doomimage_t *di, size_t imgsize, int outwidth, int outheight, int x, int y)
{
	unsigned int *colpointers;
	int c, fr, rc, extra;
	unsigned char *data, *coldata;
	extern qbyte		gammatable[256];

	if (!di)
		return;

	data = (char *)di;


//	out += x/*+di->xpos*/;
//	out += (y/*+di->ypos*/)*outwidth;

	colpointers = (unsigned int*)(data+sizeof(doomimage_t));
	for (c = 0; c < di->width; c++)
	{
		if (c+x < 0)
			continue;
		if (c+x >= outwidth)
			break;

		if (colpointers[c] >= imgsize)
			break;
		coldata = data + colpointers[c];
		while(1)
		{
			fr = *coldata++;
			if (fr == 255)
				break;

			rc = *coldata++;

			coldata++;	//one not drawn, on each side

			fr+=y;

			if (fr<0)
			{
				coldata += -fr;	//plus
				rc -= -fr;
				fr = 0;
			}

			if ((fr+rc) > outheight)
			{
				extra = rc - (outheight - fr) +1;
				rc = outheight - fr;
				if (rc < 0)
					break;
			}
			else
				extra = 1;

			while(rc)
			{
				out[c+x + fr*outwidth] = (gammatable[doompalette[*coldata*3]]) + (gammatable[doompalette[*coldata*3+1]]<<8) + (gammatable[doompalette[*coldata*3+2]]<<16) + (255<<24);
				coldata++;
				fr++;
				rc--;
			}

			coldata+=extra; //one not drawn, on each side
		}
	}
}

static texid_t Doom_LoadPatchFromTexWad(char *name, void *texlump, unsigned short *width, unsigned short *height, qboolean *hasalpha)
{
	char patch[32] = "patches/";
	unsigned int *tex;
	ddoomtexture_t *tx;
	ddoomtexturecomponant_t *tc;
	texid_t result;
	int i;
	int count;

	count = *(int *)texlump;
	tex = (int *)texlump+1;

	for (i = 0; i < count; i++)
	{
		tx = (ddoomtexture_t*)((unsigned char*)texlump + *tex);
		if (!strncmp(tx->name, name, 8))
		{
			tex = BZ_Malloc(tx->width*tx->height*4);
			memset(tex, 0, tx->width*tx->height*4);
			*width = tx->width;
			*height = tx->height;
			tc = (ddoomtexturecomponant_t*)(tx+1);
			for (i = 0; i < tx->componantcount; i++, tc++)
			{
				doomimage_t *img;
				size_t imgsize;
				strncpy(patch+8, pnames+4+8*tc->patchnum, 8);
				Q_strlwr(patch+8);
				patch[16] = '\0';
				Q_strncatz(patch, ".pat", sizeof(patch));

				img = (doomimage_t *)FS_LoadMallocFile(patch, &imgsize);
				Doom_ExtractPName(tex, img, imgsize, tx->width, tx->height, tc->xoffset, tc->yoffset);
				BZ_Free(img);
			}

			*hasalpha = false;
			for (i = 0; i < tx->width * tx->height; i++)
			{
				if (!(tex[i] & 0xff000000))
				{
					*hasalpha = true;
					break;
				}
			}

			result = R_LoadTexture32(name, tx->width, tx->height, tex, 0);
			BZ_Free(tex);
			return result;
		}

		tex++;
	}

	return r_nulltex;
}
static int Doom_LoadPatch(doommap_t *dm, char *name)
{
	qboolean hasalpha = false;
	int texnum;
	size_t nlen = strnlen(name, 8);

	for (texnum = 0; texnum < dm->numtextures; texnum++)	//a hash table might be a good plan.
	{
		if(!memcmp(name, dm->textures[texnum].name, nlen) && !dm->textures[texnum].name[nlen])
		{
			return texnum;
		}
	}
	//couldn't find it.
//	texnum = dm->numtextures;

	dm->textures = BZ_Realloc(dm->textures, sizeof(*dm->textures)*((dm->numtextures+16)&~15));
	memset(dm->textures + dm->numtextures, 0, sizeof(dm->textures[dm->numtextures]));
	dm->numtextures++;

	memcpy(dm->textures[texnum].name, name, nlen);
	dm->textures[texnum].name[nlen] = 0;

	return texnum;
}

static void Doom_LoadShaders(void *ctx, void *data, size_t a, size_t b)
{
	model_t *mod = ctx;
	doommap_t *dm = mod->meshinfo;
	texnums_t tn;
	qboolean hasalpha = false;
	qboolean isflat;
	int texnum;
	char tmp[MAX_QPATH];

	for (texnum = 0; texnum < dm->numtextures; texnum++)	//a hash table might be a good plan.
	{
		isflat = !strncmp(dm->textures[texnum].name, "flats/", 6);
		memset(&tn, 0, sizeof(tn));
		if (isflat)
		{
			void *file = FS_LoadMallocFile(va2(tmp, sizeof(tmp), "%s.raw", dm->textures[texnum].name), NULL);
			if (file)
			{
				tn.base = Image_GetTexture(dm->textures[texnum].name, NULL, 0, file, doompalette, 64, 64, TF_8PAL24);
				Z_Free(file);
			}
			dm->textures[texnum].width = 64;
			dm->textures[texnum].height = 64;
		}
		else
		{
			if (textures1 && !TEXVALID(tn.base))
				tn.base = Doom_LoadPatchFromTexWad(dm->textures[texnum].name, textures1, &dm->textures[texnum].width, &dm->textures[texnum].height, &hasalpha);
			if (textures2 && !TEXVALID(tn.base))
				tn.base = Doom_LoadPatchFromTexWad(dm->textures[texnum].name, textures2, &dm->textures[texnum].width, &dm->textures[texnum].height, &hasalpha);
		}
		if (!TEXVALID(tn.base))
		{
			dm->textures[texnum].width = 64;
			dm->textures[texnum].height = 64;
			hasalpha = false;
		}

		if (hasalpha)
			dm->textures[texnum].shader = R_RegisterShader(dm->textures[texnum].name, SUF_NONE, "{\n{\nmap $diffuse\nrgbgen vertex\nalphagen vertex\nalphafunc ge128\n}\n}\n");
		else
			dm->textures[texnum].shader = R_RegisterShader(dm->textures[texnum].name, SUF_NONE, "{\n{\nmap $diffuse\nrgbgen vertex\nalphagen vertex\n}\n}\n");

		R_BuildDefaultTexnums(&tn, dm->textures[texnum].shader);
	}
};

static void Doom_Purge (struct model_s *mod)
{
	int texnum;
	doommap_t *dm = mod->meshinfo;
	for (texnum = 0; texnum < dm->numtextures; texnum++)
	{
		BZ_Free(dm->textures[texnum].mesh.colors4b_array);
		BZ_Free(dm->textures[texnum].mesh.st_array);
		BZ_Free(dm->textures[texnum].mesh.xyz_array);
		BZ_Free(dm->textures[texnum].mesh.indexes);
	}
	BZ_Free(dm->textures);
	dm->textures = NULL;
}
#endif
static void CleanWalls(doommap_t *dm, dsidedef_t *sidedefsl)
{
	int i;
	char texname[64];
	char lastmiddle[9]="-";
	char lastlower[9]="-";
	char lastupper[9]="-";
	int lastmidtex=0, lastuptex=0, lastlowtex=0;
	dm->sidedef = BZ_Malloc(dm->numsidedefs * sizeof(*dm->sidedef));
	for (i = 0; i < dm->numsidedefs; i++)
	{
#if 1//def GLQUAKE
		strncpy(texname, sidedefsl[i].middletex, 8);
		texname[8] = '\0';
		if (!strcmp(texname, "-"))
			dm->sidedef[i].middletex = 0;
		else
		{
			if (!strncmp(texname, lastmiddle, 8))
				dm->sidedef[i].middletex = lastmidtex;
			else
			{
				strncpy(lastmiddle, texname, 8);
				dm->sidedef[i].middletex = lastmidtex = Doom_LoadPatch(dm, texname);
			}
		}

		strncpy(texname, sidedefsl[i].lowertex, 8);
		texname[8] = '\0';
		if (!strcmp(texname, "-"))
			dm->sidedef[i].lowertex = 0;
		else
		{
			if (!strncmp(texname, lastlower, 8))
				dm->sidedef[i].lowertex = lastlowtex;
			else
			{
				strncpy(lastlower, texname, 8);
				dm->sidedef[i].lowertex = lastlowtex = Doom_LoadPatch(dm, texname);
			}
		}

		strncpy(texname, sidedefsl[i].uppertex, 8);
		texname[8] = '\0';
		if (!strcmp(texname, "-"))
			dm->sidedef[i].uppertex = 0;
		else
		{
			if (!strncmp(texname, lastupper, 8))
				dm->sidedef[i].uppertex = lastuptex;
			else
			{
				strncpy(lastupper, texname, 8);
				dm->sidedef[i].uppertex = lastuptex = Doom_LoadPatch(dm, texname);
			}
		}
#endif
		dm->sidedef[i].sector = sidedefsl[i].sector;
		dm->sidedef[i].texx = sidedefsl[i].texx;
		dm->sidedef[i].texy = sidedefsl[i].texy;
	}
}

void QuakifyThings(doommap_t *dm)
{
	msector_t *sector;
	int spawnflags;
	char *name;
	int i;
	int zpos;
	static char newlump[1024*1024];	//FIXME
	char thingname[MAX_QPATH];
	
	char *ptr = newlump;
	vec3_t point;

	sprintf(ptr,	"{\n"
					"\"classname\" \"worldspawn\"\n"
					"}\n");
	ptr += strlen(ptr);

	for (i = 0; i < dm->numthings; i++)
	{
		float zbias = 24;
		switch(dm->thing[i].type)
		{
		case THING_PLAYER:	//fixme: spit out a coop spawn too.
			name = "info_player_start";
			break;
		case THING_PLAYER2:
		case THING_PLAYER3:
		case THING_PLAYER4:
			name = "info_player_coop";
			break;
		case THING_DMSPAWN:
			name = "info_player_deathmatch";
			break;


		case THING_WCHAINSAW:
			name = "item_artifact_super_damage";
			break;
		case THING_WSHOTGUN1:
			name = "weapon_nailgun";
			break;
		case THING_WSHOTGUN2:
			name = "weapon_supernailgun";
			break;
		case THING_WCHAINGUN:
			name = "weapon_supershotgun";
			break;
		case THING_WROCKETL:
			name = "weapon_rocketlauncher";
			break;
		case THING_WPLASMA:
			name = "weapon_grenadelauncher";
			break;
		case THING_WBFG:
			name = "weapon_lightning";
			break;

		default:
			name = va2(thingname, sizeof(thingname), "thing_%i", dm->thing[i].type);
			zbias = 0;
			break;
		}

		point[0] = dm->thing[i].xpos;
		point[1] = dm->thing[i].ypos;
		point[2] = 0;
		sector = Doom_SectorNearPoint(dm, point);
		zpos = sector->floorheight + zbias;	//things have no z coord, so find the sector they're in

		spawnflags = SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD | SPAWNFLAG_NOT_DEATHMATCH;
		if (dm->thing[i].flags & THING_EASY)
			spawnflags -= SPAWNFLAG_NOT_EASY;
		if (dm->thing[i].flags & THING_MEDIUM)
			spawnflags -= SPAWNFLAG_NOT_MEDIUM;
		if (dm->thing[i].flags & THING_HARD)
			spawnflags -= SPAWNFLAG_NOT_HARD;
		if (dm->thing[i].flags & THING_DEATHMATCH)
			spawnflags -= SPAWNFLAG_NOT_DEATHMATCH;
		if (dm->thing[i].flags & THING_DEAF)
			spawnflags |= 1;

		Q_snprintfz(ptr, newlump+sizeof(newlump)-ptr,	"{\n"
						"\"classname\" \"%s\"\n"
						"\"origin\" \"%i %i %i\"\n"
						"\"spawnflags\" \"%i\"\n"
						"\"angle\" \"%i\"\n"
						"}\n",
							name,
							dm->thing[i].xpos, dm->thing[i].ypos, zpos,
							spawnflags,
							dm->thing[i].angle
						);
		ptr += strlen(ptr);
	}

	Mod_SetEntitiesStringLen(dm->model, newlump, ptr-newlump);
}

void Doom_GeneratePlanes(doommap_t *dm)
{
	vec3_t point, up, line;
	int n;
	up[0] = 0;
	up[1] = 0;
	up[2] = 1;
	line[2] = 0;
	dm->nodeplane = BZ_Malloc(sizeof(*dm->nodeplane)*dm->numnodes);
	dm->lineplane = BZ_Malloc(sizeof(*dm->lineplane)*dm->numlinedefs);
	point[2] = 0;
	for (n = 0; n < dm->numnodes; n++)
	{
		line[0] = dm->node[n].dx;
		line[1] = dm->node[n].dy;
		point[0] = dm->node[n].x;
		point[1] = dm->node[n].y;
		CrossProduct(line, up, dm->nodeplane[n].normal);
		VectorNormalize(dm->nodeplane[n].normal);
		dm->nodeplane[n].dist = DotProduct (point, dm->nodeplane[n].normal);
	}

	for (n = 0; n < dm->numlinedefs; n++)
	{
		point[0] = dm->vertexes[dm->linedef[n].vert[0]].xpos;
		point[1] = dm->vertexes[dm->linedef[n].vert[0]].ypos;
		line[0] = dm->vertexes[dm->linedef[n].vert[1]].xpos-point[0];
		line[1] = dm->vertexes[dm->linedef[n].vert[1]].ypos-point[1];
		CrossProduct(line, up, dm->lineplane[n].normal);
		VectorNormalize(dm->lineplane[n].normal);
		dm->lineplane[n].dist = DotProduct (point, dm->lineplane[n].normal);
	}
}

/*
doom maps have no network limitations, but has +/-32767 map size limits (same as quake bsp)
fte defaults to a +/- 4096 world
a lot of maps are off-centered and can be moved to get them to fit fte's constraints, so if we can, do so
*/
static void MoveWorld(doommap_t *dm)
{
	int v;
	short adj[2];
	short min[2], max[2];
	min[0] = 4096;
	min[1] = 4096;
	max[0] = -4096;
	max[1] = -4096;

	for (v = 0; v < dm->numvertexes; v++)
	{
		if (min[0] > dm->vertexes[v].xpos)
			min[0] = dm->vertexes[v].xpos;
		if (min[1] > dm->vertexes[v].ypos)
			min[1] = dm->vertexes[v].ypos;

		if (max[0] < dm->vertexes[v].xpos)
			max[0] = dm->vertexes[v].xpos;
		if (max[1] < dm->vertexes[v].ypos)
			max[1] = dm->vertexes[v].ypos;
	}

	if (min[0]>=-4096 && max[0]<=4096)
		if (min[1]>=-4096 && max[1]<=4096)
			adj[0] = adj[1] = 0;	//doesn't need adjusting, live with it.

	if (max[0]-min[0]>=8192 || max[1]-min[1]>=8192)
	{
		Con_Printf(CON_WARNING "Warning: Map is too large for the network protocol\n");
		adj[0] = adj[1] = 0;
	}
	else
	{
		adj[0] = (max[0]-4096)&~63;	//don't harm the tiling.
		adj[1] = (max[1]-4096)&~63;
	}

	dm->model->mins[0] = min[0] - adj[0];
	dm->model->mins[1] = min[1] - adj[1];
	dm->model->mins[2] = -32768;

	dm->model->maxs[0] = max[0] - adj[0];
	dm->model->maxs[1] = max[1] - adj[1];
	dm->model->maxs[2] = 32767;

	if (!adj[0] && !adj[1])
		return;

	Con_Printf("Adjusting map (%i %i)\n", -adj[0], -adj[1]);

	for (v = 0; v < dm->numvertexes; v++)
	{
		dm->vertexes[v].xpos -= adj[0];
		dm->vertexes[v].ypos -= adj[1];
	}

	for (v = 0; v < dm->numnodes; v++)
	{
		dm->node[v].x -= adj[0];
		dm->node[v].y -= adj[1];

		dm->node[v].x1lower -= adj[0];
		dm->node[v].x1upper -= adj[1];
		dm->node[v].y1lower -= adj[0];
		dm->node[v].y1upper -= adj[1];

		dm->node[v].x2lower -= adj[0];
		dm->node[v].x2upper -= adj[1];
		dm->node[v].y2lower -= adj[0];
		dm->node[v].y2upper -= adj[1];
	}

	for (v = 0; v < dm->numthings; v++)
	{
		dm->thing[v].xpos -= adj[0];
		dm->thing[v].ypos -= adj[1];
	}

	dm->blockmap->xorg -= adj[0];
	dm->blockmap->yorg -= adj[1];
}


static void Doom_LoadVerticies(doommap_t *dm, char *name)
{
	ddoomvertex_t *std, *gl1;
	int stdc, glc;
	int *gl2;
	int i;
	size_t fsize;
	char tmp[MAX_QPATH];

	std		= (void *)FS_LoadMallocFile	(va2(tmp,sizeof(tmp),"%s.vertexes",	name), &fsize);
	stdc	= fsize/sizeof(*std);

	gl2		= (void *)FS_LoadMallocFile	(va2(tmp,sizeof(tmp),"%s.gl_vert",	name), &fsize);
	if (!gl2)
	{
		glc = 0;
		gl1 = NULL;
	}
	else if (gl2[0] == (('g'<<0)|('N'<<8)|('d'<<16)|('2'<<24)))
	{
		gl2++;
		glc = (fsize-4)/sizeof(int)/2;
		gl1 = NULL;
	}
	else
	{
		glc	= fsize/sizeof(*gl1);
		gl1 = (ddoomvertex_t*)gl2;
	}

	if (stdc)
	{
		dm->numvertexes = stdc + glc;
		dm->vertexes = BZ_Malloc(dm->numvertexes*sizeof(*dm->vertexes));

		dm->vertexsglbase = stdc;

		for (i = 0; i < stdc; i++)
		{
			dm->vertexes[i].xpos = std[i].xpos;
			dm->vertexes[i].ypos = std[i].ypos;
		}
		if (gl1)
		{
			for (i = 0; i < glc; i++)
			{
				dm->vertexes[stdc+i].xpos = gl1[i].xpos;
				dm->vertexes[stdc+i].ypos = gl1[i].ypos;
			}
		}
		else
		{
			for (i = 0; i < glc; i++)
			{
				dm->vertexes[stdc+i].xpos = (float)gl2[i*2] / 0x10000;
				dm->vertexes[stdc+i].ypos = (float)gl2[i*2+1] / 0x10000;
			}
		}
	}
	Z_Free(std);
	Z_Free(gl2);
}

static void Doom_LoadSSectors(doommap_t *dm, char *name)
{
	dssector_t *in;
	size_t fsize;
	unsigned int i;
	char tmp[MAX_QPATH];
	in	= (void *)FS_LoadMallocFile	(va2(tmp, sizeof(tmp), "%s.gl_ssect",	name), &fsize);
	if (!in)
		in	= (void *)FS_LoadMallocFile	(va2(tmp, sizeof(tmp), "%s.ssectors",	name), &fsize);
	//FIXME: "gNd3" means that it's glbsp version 3.
	dm->numssectors	= fsize/sizeof(*in);

	dm->ssector = Z_Malloc(dm->numssectors * sizeof(*dm->ssector));
	for (i = 0; i < dm->numssectors; i++)
	{
		dm->ssector[i].segcount = in[i].segcount;
		dm->ssector[i].first = in[i].first;
	}
	Z_Free(in);
}
static void Doom_CalcSubsectorSectors(doommap_t *dm)
{	//kinda shitty
	unsigned int num, seg;
	for (num = 0; num < dm->numssectors; num++)
	{
		dm->ssector[num].sector = &dm->sector[dm->sidedef[dm->linedef[dm->seg[dm->ssector[num].first].linedef].sidedef[dm->seg[dm->ssector[num].first].direction]].sector];
		for (seg = dm->ssector[num].first+1; seg < dm->ssector[num].first + dm->ssector[num].segcount; seg++)
			if (dm->seg[seg].linedef != 0xffff)
			{
				dm->ssector[num].sector = &dm->sector[dm->sidedef[dm->linedef[dm->seg[seg].linedef].sidedef[dm->seg[seg].direction]].sector];
				break;
			}
	}
}

static void Doom_LoadSSegs(doommap_t *dm, char *name)
{	//these skirt the subsectors

	void *file;
	dgl_seg3_t	*s3;
	dgl_seg1_t	*s1;
	dseg_t		*s0;
	int i;
	size_t fsize;
	char tmp[MAX_QPATH];

	file	= (void *)FS_LoadMallocFile	(va2(tmp, sizeof(tmp), "%s.gl_segs",	name), &fsize);
	if (!file)
	{
		s0 = (void *)FS_LoadMallocFile	(va2(tmp, sizeof(tmp), "%s.segs",	name), &fsize);
		dm->numsegs	= fsize/sizeof(*s0);

		dm->seg = BZ_Malloc(dm->numsegs * sizeof(*dm->seg));
		for (i = 0; i < dm->numsegs; i++)
		{
			dm->seg[i].vert[0] = s0[i].vert[0];
			dm->seg[i].vert[1] = s0[i].vert[1];
			dm->seg[i].linedef = s0[i].linedef;
			dm->seg[i].direction = s0[i].direction;
			dm->seg[i].Partner = 0xffff;
		}
	}
	else if (*(int *)file == *(int *)"gNd3")
	{
		s3 = file;
		dm->numsegs	= fsize/sizeof(*s3);

		dm->seg = s3;
	}
	else if (!file)
		return;
	else
	{
		s1 = file;
		dm->numsegs	= fsize/sizeof(*s1);

		dm->seg = BZ_Malloc(dm->numsegs * sizeof(*dm->seg));
		for (i = 0; i < dm->numsegs; i++)
		{
			if (s1[i].vert[0] & 0x8000)
				dm->seg[i].vert[0] = (s1[i].vert[0]&0x7fff)+dm->vertexsglbase;
			else
				dm->seg[i].vert[0] = s1[i].vert[0];
			if (s1[i].vert[1] & 0x8000)
				dm->seg[i].vert[1] = (s1[i].vert[1]&0x7fff)+dm->vertexsglbase;
			else
				dm->seg[i].vert[1] = s1[i].vert[1];
			dm->seg[i].linedef = s1[i].linedef;
			dm->seg[i].direction = s1[i].direction;
			if (s1[i].Partner == 0xffff)
				dm->seg[i].Partner = 0xffffffff;
			else
				dm->seg[i].Partner = s1[i].Partner;
		}
	}
}

qboolean QDECL Mod_LoadDoomLevel(model_t *mod, void *buffer, size_t fsize)
{
	int h;
	dsector_t		*sectorl;
	dsidedef_t		*sidedefsl;
	char name[MAX_QPATH];
	char tmp[MAX_QPATH];
	doommap_t *dm;

	int *gl_nodes;

	if (fsize != 4)
	{
		Con_Printf("Wad map %s does actually exist... weird.\n", mod->name);
		return false;
	}

	dm = Z_Malloc(sizeof(*dm));
	dm->model = mod;
	mod->meshinfo = dm;

	COM_StripExtension(mod->name, name, sizeof(name));

	gl_nodes	= (void *)FS_LoadMallocFile	(va2(tmp,sizeof(tmp),"%s.gl_nodes",	name), &fsize);
	if (gl_nodes && fsize>0)
	{
		dm->node = (void *)gl_nodes;
		dm->numnodes = fsize/sizeof(*dm->node);
	}
	else
	{
		gl_nodes=NULL;
		dm->node		= (void *)FS_LoadMallocFile	(va2(tmp,sizeof(tmp),"%s.nodes",		name), &fsize);
		dm->numnodes		= fsize/sizeof(*dm->node);
	}
	sectorl		= (void *)FS_LoadMallocFile	(va2(tmp,sizeof(tmp),"%s.sectors",	name), &fsize);
	dm->numsectors		= fsize/sizeof(*sectorl);
	dm->sector = Z_Malloc(dm->numsectors * sizeof(*dm->sector));

#ifndef SERVERONLY
	dm->numtextures=0;
	Doom_LoadPalette();
#endif


	Doom_LoadVerticies(dm, name);

	Doom_LoadSSegs(dm, name);
	Doom_LoadSSectors(dm, name);

	dm->thing		= (void *)FS_LoadMallocFile	(va2(tmp,sizeof(tmp),"%s.things",	name), &fsize);
	dm->numthings		= fsize/sizeof(*dm->thing);
	dm->linedef	= (void *)FS_LoadMallocFile	(va2(tmp,sizeof(tmp),"%s.linedefs",	name), &fsize);
	dm->numlinedefs	= fsize/sizeof(*dm->linedef);
	sidedefsl	= (void *)FS_LoadMallocFile	(va2(tmp,sizeof(tmp),"%s.sidedefs",	name), &fsize);
	dm->numsidedefs	= fsize/sizeof(*sidedefsl);
	dm->blockmap	= (void *)FS_LoadMallocFile	(va2(tmp,sizeof(tmp),"%s.blockmap",	name), &fsize);

#ifndef SERVERONLY
	Doom_LoadTextureInfos();
#endif
	dm->blockmapofs = (unsigned short*)(dm->blockmap+1);

	if (!dm->node || !sectorl || !dm->seg || !dm->ssector || !dm->thing || !dm->linedef || !sidedefsl || !dm->vertexes)
	{
		Sys_Error("Wad map doesn't contain enough lumps\n");
		dm->node = NULL;
		return false;
	}

	MoveWorld(dm);

	Doom_GeneratePlanes(dm);

	mod->hulls[0].clip_mins[0] = 0;
	mod->hulls[0].clip_mins[1] = 0;
	mod->hulls[0].clip_mins[2] = 0;
	mod->hulls[0].clip_maxs[0] = 0;
	mod->hulls[0].clip_maxs[1] = 0;
	mod->hulls[0].clip_maxs[2] = 0;
	mod->hulls[0].available = true;

	for (h = 1; h < MAX_MAP_HULLSM; h++)
		mod->hulls[h].available = false;

	Doom_SetModelFunc(mod);

	mod->fromgame = fg_doom;
	mod->type = mod_brush;
	mod->nodes = (void*)0x1;
	mod->numclusters = dm->numsectors;

	CleanWalls(dm, sidedefsl);

	Doom_CalcSubsectorSectors(dm);

	Triangulate_Sectors(dm, sectorl, !!gl_nodes);

	QuakifyThings(dm);

	COM_AddWork(WG_MAIN, Doom_LoadShaders, mod, NULL, 0, 0);
	return true;
}

static void Doom_LightPointValues(model_t *model, vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir)
{
	doommap_t *dm = model->meshinfo;
	msector_t *sec;
	sec = Doom_SectorNearPoint(dm, point);

	res_dir[0] = 0;
	res_dir[1] = 1;
	res_dir[2] = 1;
	res_diffuse[0] = sec->lightlev;
	res_diffuse[1] = sec->lightlev;
	res_diffuse[2] = sec->lightlev;
	res_ambient[0] = sec->lightlev;
	res_ambient[1] = sec->lightlev;
	res_ambient[2] = sec->lightlev;
}

//return pvs bits for point
static unsigned int Doom_FatPVS(struct model_s *model, vec3_t org, pvsbuffer_t *pvsbuffer, qboolean merge)
{
	//FIXME: use REJECT lump.
	return 0;
}

//check if an ent is within the given pvs
static qboolean Doom_EdictInFatPVS(struct model_s *model, struct pvscache_s *edict, qbyte *pvsbuffer)
{	//FIXME: use REJECT lump.
	return true;
}

static int Doom_ClusterForPoint(struct model_s *model, vec3_t point)
{
	doommap_t *dm = model->meshinfo;
	return Doom_SectorNearPoint(dm, point) - dm->sector;
}
static qbyte *Doom_ClusterPVS(struct model_s *model, int cluster, pvsbuffer_t *pvsbuffer, pvsmerge_t merge)
{	//FIXME: use REJECT lump.
	return NULL;
}

//generate useful info for correct functioning of Doom_EdictInFatPVS.
static void Doom_FindTouchedLeafs(struct model_s *model, struct pvscache_s *ent, vec3_t cullmins, vec3_t cullmaxs)
{
	//work out the sectors this ent is in for easy pvs.
}

//requires lightmaps - not supported.
static void Doom_StainNode(struct mnode_s *node, float *parms)
{
}

//requires lightmaps - not supported.
static void Doom_MarkLights(struct dlight_s *light, int bit, struct mnode_s *node)
{
}

void Doom_SetModelFunc(model_t *mod)
{
#ifndef SERVERONLY
	mod->funcs.PurgeModel			= Doom_Purge;
#endif
	mod->funcs.FatPVS				= Doom_FatPVS;
	mod->funcs.EdictInFatPVS		= Doom_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs		= Doom_FindTouchedLeafs;
	mod->funcs.ClusterForPoint		= Doom_ClusterForPoint;
	mod->funcs.ClusterPVS			= Doom_ClusterPVS;

	mod->funcs.LightPointValues		= Doom_LightPointValues;
	mod->funcs.StainNode			= Doom_StainNode;
	mod->funcs.MarkLights			= Doom_MarkLights;

//	mod->funcs.LeafPVS)			(struct model_s *model, int num, qbyte *buffer, unsigned int buffersize);

	mod->funcs.NativeTrace			= Doom_Trace;
	mod->funcs.PointContents		= Doom_PointContents;

	//Doom_SetCollisionFuncs(mod);
}

#endif
