
// This code written in 2010 by Ashley Rose Hale (LadyHavoc) (darkplacesengine gmail com), and placed into public domain.

#include <stdlib.h>
#include <string.h>
#include "bih.h"

static int BIH_BuildNode(bih_t *bih, int numchildren, int *leaflist, float *totalmins, float *totalmaxs)
{
	int i;
	int j;
	int longestaxis;
	int axis = 0;
	int nodenum;
	int front = 0;
	int back = 0;
	bih_node_t *node;
	bih_leaf_t *child;
	float splitdist;
	float d;
	float mins[3];
	float maxs[3];
	float size[3];
	float frontmins[3];
	float frontmaxs[3];
	float backmins[3];
	float backmaxs[3];
	// calculate bounds of children
	child = bih->leafs + leaflist[0];
	mins[0] = child->mins[0];
	mins[1] = child->mins[1];
	mins[2] = child->mins[2];
	maxs[0] = child->maxs[0];
	maxs[1] = child->maxs[1];
	maxs[2] = child->maxs[2];
	for (i = 1;i < numchildren;i++)
	{
		child = bih->leafs + leaflist[i];
		if (mins[0] > child->mins[0]) mins[0] = child->mins[0];
		if (mins[1] > child->mins[1]) mins[1] = child->mins[1];
		if (mins[2] > child->mins[2]) mins[2] = child->mins[2];
		if (maxs[0] < child->maxs[0]) maxs[0] = child->maxs[0];
		if (maxs[1] < child->maxs[1]) maxs[1] = child->maxs[1];
		if (maxs[2] < child->maxs[2]) maxs[2] = child->maxs[2];
	}
	size[0] = maxs[0] - mins[0];
	size[1] = maxs[1] - mins[1];
	size[2] = maxs[2] - mins[2];
	// provide bounds to caller
	totalmins[0] = mins[0];
	totalmins[1] = mins[1];
	totalmins[2] = mins[2];
	totalmaxs[0] = maxs[0];
	totalmaxs[1] = maxs[1];
	totalmaxs[2] = maxs[2];
	// if we run out of nodes it's the caller's fault, but don't crash
	if (bih->numnodes == bih->maxnodes)
	{
		if (!bih->error)
			bih->error = BIHERROR_OUT_OF_NODES;
		return 0;
	}
	nodenum = bih->numnodes++;
	node = bih->nodes + nodenum;
	// store bounds for node
	node->mins[0] = mins[0];
	node->mins[1] = mins[1];
	node->mins[2] = mins[2];
	node->maxs[0] = maxs[0];
	node->maxs[1] = maxs[1];
	node->maxs[2] = maxs[2];
	node->front = 0;
	node->back = 0;
	node->frontmin = 0;
	node->backmax = 0;
	memset(node->children, -1, sizeof(node->children));
	// check if there are few enough children to store an unordered node
	if (numchildren <= BIH_MAXUNORDEREDCHILDREN)
	{
		node->type = BIH_UNORDERED;
		for (j = 0;j < numchildren;j++)
			node->children[j] = leaflist[j];
		return nodenum;
	}
	// pick longest axis
	longestaxis = 0;
	if (size[0] < size[1]) longestaxis = 1;
	if (size[longestaxis] < size[2]) longestaxis = 2;
	// iterate possible split axis choices, starting with the longest axis, if
	// all fail it means all children have the same bounds and we simply split
	// the list in half because each node can only have two children.
	for (j = 0;j < 3;j++)
	{
		// pick an axis
		axis = (longestaxis + j) % 3;
		// sort children into front and back lists
		splitdist = (node->mins[axis] + node->maxs[axis]) * 0.5f;
		front = 0;
		back = 0;
		for (i = 0;i < numchildren;i++)
		{
			child = bih->leafs + leaflist[i];
			d = (child->mins[axis] + child->maxs[axis]) * 0.5f;
			if (d < splitdist)
				bih->leafsortscratch[back++] = leaflist[i];
			else
				leaflist[front++] = leaflist[i];
		}
		// now copy the back ones into the space made in the leaflist for them
		if (back)
			memcpy(leaflist + front, bih->leafsortscratch, back*sizeof(leaflist[0]));
		// if both sides have some children, it's good enough for us.
		if (front && back)
			break;
	}
	if (j == 3)
	{
		// somewhat common case: no good choice, divide children arbitrarily
		axis = 0;
		back = numchildren >> 1;
		front = numchildren - back;
	}

	// we now have front and back children divided in leaflist...
	node->type = (bih_nodetype_t)((int)BIH_SPLITX + axis);
	node->front = BIH_BuildNode(bih, front, leaflist, frontmins, frontmaxs);
	node->frontmin = frontmins[axis];
	node->back = BIH_BuildNode(bih, back, leaflist + front, backmins, backmaxs);
	node->backmax = backmaxs[axis];
	return nodenum;
}

int BIH_Build(bih_t *bih, int numleafs, bih_leaf_t *leafs, int maxnodes, bih_node_t *nodes, int *temp_leafsort, int *temp_leafsortscratch)
{
	int i;

	memset(bih, 0, sizeof(*bih));
	bih->numleafs = numleafs;
	bih->leafs = leafs;
	bih->leafsort = temp_leafsort;
	bih->leafsortscratch = temp_leafsortscratch;
	bih->numnodes = 0;
	bih->maxnodes = maxnodes;
	bih->nodes = nodes;

	// clear things we intend to rebuild
	memset(bih->nodes, 0, sizeof(bih->nodes[0]) * bih->maxnodes);
	for (i = 0;i < bih->numleafs;i++)
		bih->leafsort[i] = i;

	bih->rootnode = BIH_BuildNode(bih, bih->numleafs, bih->leafsort, bih->mins, bih->maxs);
	return bih->error;
}

static void BIH_GetTriangleListForBox_Node(const bih_t *bih, int nodenum, int maxtriangles, int *trianglelist_idx, int *trianglelist_surf, int *numtrianglespointer, const float *mins, const float *maxs)
{
	int axis;
	bih_node_t *node;
	bih_leaf_t *leaf;
	for(;;)
	{
		node = bih->nodes + nodenum;
		// check if this is an unordered node (which holds an array of leaf numbers)
		if (node->type == BIH_UNORDERED)
		{
			for (axis = 0;axis < BIH_MAXUNORDEREDCHILDREN && node->children[axis] >= 0;axis++)
			{
				leaf = bih->leafs + node->children[axis];
				if (mins[0] > leaf->maxs[0] || maxs[0] < leaf->mins[0]
				 || mins[1] > leaf->maxs[1] || maxs[1] < leaf->mins[1]
				 || mins[2] > leaf->maxs[2] || maxs[2] < leaf->mins[2])
					continue;
				switch(leaf->type)
				{
				case BIH_RENDERTRIANGLE:
					if (*numtrianglespointer >= maxtriangles)
					{
						++*numtrianglespointer; // so the caller can detect overflow
						break;
					}
					if(trianglelist_surf)
						trianglelist_surf[*numtrianglespointer] = leaf->surfaceindex;
					trianglelist_idx[*numtrianglespointer] = leaf->itemindex;
					++*numtrianglespointer;
					break;
				default:
					break;
				}
			}
			return;
		}
		// splitting node
		axis = node->type - BIH_SPLITX;
		if (mins[axis] < node->backmax)
		{
			if (maxs[axis] > node->frontmin)
				BIH_GetTriangleListForBox_Node(bih, node->front, maxtriangles, trianglelist_idx, trianglelist_surf, numtrianglespointer, mins, maxs);
			nodenum = node->back;
			continue;
		}
		if (maxs[axis] > node->frontmin)
		{
			nodenum = node->front;
			continue;
		}
		// fell between the child groups, nothing here
		return;
	}
}

int BIH_GetTriangleListForBox(const bih_t *bih, int maxtriangles, int *trianglelist_idx, int *trianglelist_surf, const float *mins, const float *maxs)
{
	int numtriangles = 0;
	BIH_GetTriangleListForBox_Node(bih, bih->rootnode, maxtriangles, trianglelist_idx, trianglelist_surf, &numtriangles, mins, maxs);
	return numtriangles;
}
