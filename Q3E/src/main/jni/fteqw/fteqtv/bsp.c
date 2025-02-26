/*
Copyright (C) 1996-1997 Id Software, Inc.

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

#include "qtv.h"

#define	MAX_MAP_LEAFS		32767

typedef struct {
	float planen[3];
	float planedist;
	int child[2];
} node_t;
struct bsp_s {
	unsigned int fullchecksum;
	unsigned int visedchecksum;
	node_t *nodes;
	unsigned char *pvslump;
	unsigned char **pvsofs;

	unsigned char decpvs[(MAX_MAP_LEAFS+7)/8];	//decompressed pvs
	int pvsbytecount;



	int numintermissionspots;
	intermission_t intermissionspot[8];
};

static const intermission_t nullintermissionspot = {{0}};


typedef struct
{
	int			contents;
	int			visofs;				// -1 = no visibility info

	short		mins[3];			// for frustum culling
	short		maxs[3];

	unsigned short		firstmarksurface;
	unsigned short		nummarksurfaces;
#define NUM_AMBIENTS 4
	unsigned char		ambient_level[NUM_AMBIENTS];
} dleaf_t;
typedef struct
{
	unsigned int			planenum;
	short		children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for sphere culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} dnode_t;
typedef struct
{
	float	normal[3];
	float	dist;
	unsigned int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;
typedef struct
{
	int		fileofs, filelen;
} lump_t;
#define LUMP_ENTITIES		0
#define	LUMP_PLANES		1
#define	LUMP_VISIBILITY	4
#define	LUMP_NODES		5
#define	LUMP_LEAFS		10
#define	HEADER_LUMPS	15
typedef struct
{
	int			version;	
	lump_t		lumps[HEADER_LUMPS];
} dheader_t;



void DecompressVis(unsigned char *in, unsigned char *out, int bytecount)
{
	int c;
	unsigned char *end;

	for (end = out + bytecount; out < end; )
	{
		c = *in;
		if (!c)
		{	//a 0 is always followed by the count of 0s.
			c = in[1];
			in += 2;
			for (; c > 4; c-=4)
			{
				*(unsigned int*)out = 0;
				out+=4;
			}
			for (; c; c--)
				*out++ = 0;
		}
		else
		{
			in++;
			*out++ = c;
		}
	}
}

void BSP_LoadEntities(bsp_t *bsp, char *entitydata)
{
	char *v;
	char key[2048];
	char value[2048];

	enum {et_random, et_startspot, et_primarystart, et_intermission} etype;

	float org[3];
	float angles[3];

	qboolean foundstartspot = false;
	float startspotorg[3];
	float startspotangles[3];

	//char *COM_ParseToken (char *data, char *out, int outsize, const char *punctuation)
	while (entitydata)
	{
		entitydata = COM_ParseToken(entitydata, key, sizeof(key), NULL);
		if (!entitydata)
			break;

		if (!strcmp(key, "{"))
		{
			org[0] = 0;
			org[1] = 0;
			org[2] = 0;

			angles[0] = 0;
			angles[1] = 0;
			angles[2] = 0;
			etype = et_random;

			for(;;)
			{

				if(!entitydata)
				{
					printf("unexpected eof in bsp entities section\n");
					return;
				}

				entitydata = COM_ParseToken(entitydata, key, sizeof(key), NULL);
				if (!strcmp(key, "}"))
					break;
				
				entitydata = COM_ParseToken(entitydata, value, sizeof(value), NULL);

				if (!strcmp(key, "origin"))
				{
					v = value;
					v = COM_ParseToken(v, key, sizeof(key), NULL);
					org[0] = atof(key);
					v = COM_ParseToken(v, key, sizeof(key), NULL);
					org[1] = atof(key);
					v = COM_ParseToken(v, key, sizeof(key), NULL);
					org[2] = atof(key);
				}

				if (!strcmp(key, "angles") || !strcmp(key, "angle") || !strcmp(key, "mangle"))
				{
					v = value;
					v = COM_ParseToken(v, key, sizeof(key), NULL);
					angles[0] = atof(key);
					v = COM_ParseToken(v, key, sizeof(key), NULL);
					if (v)
					{
						angles[1] = atof(key);
						v = COM_ParseToken(v, key, sizeof(key), NULL);
						angles[2] = atof(key);
					}
					else
					{
						angles[1] = angles[0];
						angles[0] = 0;
						angles[2] = 0;
					}
				}


				if (!strcmp(key, "classname"))
				{
					if (!strcmp(value, "info_player_start"))
						etype = et_primarystart;
					if (!strcmp(value, "info_deathmatch_start"))
						etype = et_startspot;
					if (!strcmp(value, "info_intermission"))
						etype = et_intermission;
				}
			}

			switch (etype)
			{
			case et_random: //a random (unknown) entity
				break;
			case et_primarystart:	//a single player start
				memcpy(startspotorg, org, sizeof(startspotorg));
				memcpy(startspotangles, angles, sizeof(startspotangles));
				foundstartspot = true;
				break;
			case et_startspot:
				if (!foundstartspot)
				{
					memcpy(startspotorg, org, sizeof(startspotorg));
					memcpy(startspotangles, angles, sizeof(startspotangles));
					foundstartspot = true;
				}
				break;
			case et_intermission:
				if (bsp->numintermissionspots < sizeof(bsp->intermissionspot)/sizeof(bsp->intermissionspot[0]))
				{
					bsp->intermissionspot[bsp->numintermissionspots].pos[0] = org[0];
					bsp->intermissionspot[bsp->numintermissionspots].pos[1] = org[1];
					bsp->intermissionspot[bsp->numintermissionspots].pos[2] = org[2];

					bsp->intermissionspot[bsp->numintermissionspots].angle[0] = angles[0];
					bsp->intermissionspot[bsp->numintermissionspots].angle[1] = angles[1];
					bsp->intermissionspot[bsp->numintermissionspots].angle[2] = angles[2];
					bsp->numintermissionspots++;
				}
				break;
			}
		}
		else
		{
			printf("data not expected here\n");
			return;
		}
	}

	if (foundstartspot && !bsp->numintermissionspots)
	{
		bsp->intermissionspot[bsp->numintermissionspots].pos[0] = startspotorg[0];
		bsp->intermissionspot[bsp->numintermissionspots].pos[1] = startspotorg[1];
		bsp->intermissionspot[bsp->numintermissionspots].pos[2] = startspotorg[2];

		bsp->intermissionspot[bsp->numintermissionspots].angle[0] = startspotangles[0];
		bsp->intermissionspot[bsp->numintermissionspots].angle[1] = startspotangles[1];
		bsp->intermissionspot[bsp->numintermissionspots].angle[2] = startspotangles[2];
		bsp->numintermissionspots++;
	}
}

bsp_t *BSP_LoadModel(cluster_t *cluster, char *gamedir, char *bspname)
{
	unsigned char *data;
	unsigned int size;
	char *entdata;

	dheader_t *header;
	dplane_t *planes;
	dnode_t *nodes;
	dleaf_t *leaf;

	int numnodes, i;
	int numleafs;
	unsigned int chksum;

	bsp_t *bsp;

	data = FS_ReadFile(gamedir, bspname, &size);
	if (!data)
	{
		Sys_Printf(cluster, "Couldn't open bsp file \"%s\" (gamedir \"%s\")\n", bspname, gamedir);
		return NULL;
	}


	header = (dheader_t*)data;
	if (size < sizeof(dheader_t) || data[0] != 29)
	{
		free(data);
		Sys_Printf(cluster, "BSP not version 29 (%s in %s)\n", bspname, gamedir);
		return NULL;
	}

	for (i = 0; i < HEADER_LUMPS; i++)
	{
		if (LittleLong(header->lumps[i].fileofs) + LittleLong(header->lumps[i].filelen) > size)
		{
			free(data);
			Sys_Printf(cluster, "BSP appears truncated (%s in gamedir %s)\n", bspname, gamedir);
			return NULL;
		}
	}

	planes = (dplane_t*)(data+LittleLong(header->lumps[LUMP_PLANES].fileofs));
	nodes = (dnode_t*)(data+LittleLong(header->lumps[LUMP_NODES].fileofs));
	leaf = (dleaf_t*)(data+LittleLong(header->lumps[LUMP_LEAFS].fileofs));

	entdata = (char*)(data+LittleLong(header->lumps[LUMP_ENTITIES].fileofs));

	numnodes = LittleLong(header->lumps[LUMP_NODES].filelen)/sizeof(dnode_t);
	numleafs = LittleLong(header->lumps[LUMP_LEAFS].filelen)/sizeof(dleaf_t);

	bsp = malloc(sizeof(bsp_t) + sizeof(node_t)*numnodes + LittleLong(header->lumps[LUMP_VISIBILITY].filelen) + sizeof(unsigned char *)*numleafs);
	bsp->numintermissionspots = 0;
	if (bsp)
	{
		bsp->fullchecksum = 0;
		bsp->visedchecksum = 0;
		for (i = 0; i < HEADER_LUMPS; i++)
		{
			if (i == LUMP_ENTITIES)
				continue;	//entities never appear in any checksums

			chksum = Com_BlockChecksum(data + LittleLong(header->lumps[i].fileofs), LittleLong(header->lumps[i].filelen));
			bsp->fullchecksum ^= chksum;
			if (i == LUMP_VISIBILITY || i == LUMP_LEAFS || i == LUMP_NODES)
				continue;
			bsp->visedchecksum ^= chksum;
		}
		bsp->nodes = (node_t*)(bsp+1);
		bsp->pvsofs = (unsigned char**)(bsp->nodes+numnodes);
		bsp->pvslump = (unsigned char*)(bsp->pvsofs+numleafs);

		bsp->pvsbytecount = (numleafs+7)/8;

		for (i = 0; i < numnodes; i++)
		{
			bsp->nodes[i].child[0] = LittleShort(nodes[i].children[0]);
			bsp->nodes[i].child[1] = LittleShort(nodes[i].children[1]);
			bsp->nodes[i].planedist = planes[LittleLong(nodes[i].planenum)].dist;
			bsp->nodes[i].planen[0] = planes[LittleLong(nodes[i].planenum)].normal[0];
			bsp->nodes[i].planen[1] = planes[LittleLong(nodes[i].planenum)].normal[1];
			bsp->nodes[i].planen[2] = planes[LittleLong(nodes[i].planenum)].normal[2];
		}
		memcpy(bsp->pvslump, data+LittleLong(header->lumps[LUMP_VISIBILITY].fileofs), LittleLong(header->lumps[LUMP_VISIBILITY].filelen));

		for (i = 0; i < numleafs; i++)
		{
			if (leaf[i].visofs < 0)
				bsp->pvsofs[i] = NULL;
			else
				bsp->pvsofs[i] = bsp->pvslump+leaf[i].visofs;
		}
	}

	BSP_LoadEntities(bsp, entdata);

	free(data);

	return bsp;
}

void BSP_Free(bsp_t *bsp)
{
	free(bsp);
}

int BSP_SphereLeafNums_r(bsp_t *bsp, int first, int maxleafs, unsigned short *list, float *pos, float radius)
{
	node_t *node;
	float dot;
	int rn;
	int numleafs = 0;

	if (!bsp)
		return 0;

	for(rn = first;rn >= 0;)
	{
		node = &bsp->nodes[rn];
		dot = (node->planen[0]*pos[0] + node->planen[1]*pos[1] + node->planen[2]*pos[2]) - node->planedist;
		if (dot < -radius)
			rn = node->child[1];
		else if (dot > radius)
			rn = node->child[0];
		else
		{
			rn = BSP_SphereLeafNums_r(bsp, node->child[0], maxleafs-numleafs, list+numleafs, pos, radius);
			if (rn < 0)
				return -1;	//ran out, so don't use pvs for this entity.
			else
				numleafs += rn;
			rn = node->child[1];	//both sides
		}
	}

	rn = -1-rn;

	if (rn <= 0)
		;	//leaf 0 has no pvs info, so don't add it.
	else if (maxleafs>numleafs)
	{
		list[numleafs] = rn-1;
		numleafs++;
	}
	else
		return -1;	//there are just too many

	return numleafs;
}

unsigned int BSP_Checksum(bsp_t *bsp)
{
	if (!bsp)
		return 0;
	return bsp->visedchecksum;
}

int BSP_SphereLeafNums(bsp_t *bsp, int maxleafs, unsigned short *list, float x, float y, float z, float radius)
{
	float pos[3];
	pos[0] = x;
	pos[1] = y;
	pos[2] = z;
	return BSP_SphereLeafNums_r(bsp, 0, maxleafs, list, pos, radius);
}

int BSP_LeafNum(bsp_t *bsp, float x, float y, float z)
{
	node_t *node;
	float dot;
	int rn;

	if (!bsp)
		return 0;

	for(rn = 0;rn >= 0;)
	{
		node = &bsp->nodes[rn];
		dot = node->planen[0]*x + node->planen[1]*y + node->planen[2]*z;
		rn = node->child[(dot-node->planedist) <= 0];
	}

	return -1-rn;
}

qboolean BSP_Visible(bsp_t *bsp, int leafcount, unsigned short *list)
{
	int i;
	if (!bsp)
		return true;

	if (leafcount < 0)	//too many, so pvs was switched off.
		return true;

	for (i = 0; i < leafcount; i++)
	{
		if (bsp->decpvs[list[i]>>3] & (1<<(list[i]&7)))
			return true;
	}
	return false;
}

void BSP_SetupForPosition(bsp_t *bsp, float x, float y, float z)
{
	int leafnum;
	if (!bsp)
		return;

	leafnum = BSP_LeafNum(bsp, x, y, z);
	DecompressVis(bsp->pvsofs[leafnum], bsp->decpvs, bsp->pvsbytecount);
}

const intermission_t *BSP_IntermissionSpot(bsp_t *bsp)
{
	int spotnum;
	if (bsp)
	{
		if (bsp->numintermissionspots>0)
		{
			spotnum = rand()%bsp->numintermissionspots;
			return &bsp->intermissionspot[spotnum];
		}
	}
	return &nullintermissionspot;
}

