/*
Copyright (c) 2022 Ashley Rose Hale (LadyHavoc)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <math.h>
#include "convex.h"

void convex_builder_initialize(convex_builder_state_t* b, float epsilon)
{
	b->numcorners = 0;
	b->numfaces = 0;
	b->epsilon = 0.0f;
}

// this is a variant of QuickHull that relies on the caller to provide points
// in a reasonable order - the result will be the same regardless of point order
// but it's more efficient if the furthest points are provided first
//
// this could be a little more efficient if we kept track of edges during the
// build, but I think it may be more numerically stable this way
void convex_builder_add_point(convex_builder_state_t* b, float x, float y, float z)
{
	int i, j, l;
	convex_corner_t corner;
	unsigned char removedcorner[CONVEX_MAX_CORNERS];
	unsigned char removedface[CONVEX_MAX_FACES];

	// we can't add any new points after max generations is reached
	if (b->numcorners > CONVEX_MAX_CORNERS - 1 || b->numfaces > CONVEX_MAX_FACES - b->numcorners - 2)
		return;

	// make a corner struct with the same layout we expect to use for vector ops
	corner.x = x;
	corner.y = y;
	corner.z = z;
	corner.w = 1.0f;

	float epsilon = b->epsilon;

	// add the new corner to the bounding box
	if (b->numcorners == 0)
	{
		b->extents[0][0] = corner.x;
		b->extents[0][1] = corner.y;
		b->extents[0][2] = corner.z;
		b->extents[1][0] = corner.x;
		b->extents[1][1] = corner.y;
		b->extents[1][2] = corner.z;
	}
	else
	{
		if (b->extents[0][0] > corner.x)
			b->extents[0][0] = corner.x;
		if (b->extents[0][1] > corner.y)
			b->extents[0][1] = corner.y;
		if (b->extents[0][2] > corner.z)
			b->extents[0][2] = corner.z;
		if (b->extents[1][0] < corner.x)
			b->extents[1][0] = corner.x;
		if (b->extents[1][1] < corner.y)
			b->extents[1][1] = corner.y;
		if (b->extents[1][2] < corner.z)
			b->extents[1][2] = corner.z;
	}

	if (b->numfaces > 0)
	{
		// determine which faces will be inside the resulting solid
		for (i = 0; i < b->numfaces; i++)
		{
			convex_face_t* f = b->faces + i;
			// face will be removed if it places this corner outside the solid
			removedface[i] = (f->x * corner.x + f->y * corner.y + f->z * corner.z + f->w * corner.w) > epsilon;
		}

		// scan for removed faces
		for (i = 0; i < b->numfaces; i++)
			if (removedface[i])
				break;

		// exit early if point is completely inside the solid
		if (i == b->numfaces)
			return;

		// garbage collect the removed faces
		for (j = i + 1; j < b->numfaces; j++)
			if (!removedface[j])
				b->faces[i++] = b->faces[j];
		b->numfaces = i;
	}

	// iterate active corners to create replacement faces using the new corner
	for (i = 0; i < b->numcorners; i++)
	{
		convex_corner_t ca = b->corners[i];
		for (j = 0; j < b->numcorners; j++)
		{
			// using the same point twice would make a degenerate plane
			if (i == j)
				continue;
			convex_corner_t cb = b->corners[j];
			// calculate the edge directions
			convex_corner_t d, e;
			convex_face_t face;
			d.x = ca.x - cb.x;
			d.y = ca.y - cb.y;
			d.z = ca.z - cb.z;
			d.w = 0.0f;
			e.x = corner.x - cb.x;
			e.y = corner.y - cb.y;
			e.z = corner.z - cb.z;
			e.w = 0.0f;
			// cross product to produce a normal; this is not unit length,
			// its length is the volume of the triangle *2
			face.x = d.y * e.z - d.z * e.y;
			face.y = d.z * e.x - d.x * e.z;
			face.z = d.x * e.y - d.y * e.x;
			float len2 = face.x * face.x + face.y * face.y + face.z * face.z;
			if (len2 == 0.0f)
			{
				// we can't do anything with a degenerate plane
				continue;
			}
			// normalize the plane normal
			float inv = 1.0f / sqrt(len2);
			face.x *= inv;
			face.y *= inv;
			face.z *= inv;
			face.w = -(corner.x * face.x + corner.y * face.y + corner.z * face.z);
			// flip the face if it's backwards (not facing center)
			if ((b->extents[0][0] + b->extents[1][0]) * 0.5f * face.x + (b->extents[0][1] + b->extents[1][1]) * 0.5f * face.y + (b->extents[0][2] + b->extents[1][2]) * 0.5f * face.z + face.w > 0.0f)
			{
				face.x *= -1.0f;
				face.y *= -1.0f;
				face.z *= -1.0f;
				face.w *= -1.0f;
			}
			// discard the proposed face if it slices through the solid
			for (l = 0; l < b->numcorners; l++)
			{
				convex_corner_t cl = b->corners[l];
				if (cl.x * face.x + cl.y * face.y + cl.z * face.z + face.w > epsilon)
					break;
			}
			if (l < b->numcorners)
				continue;
			// add the new face
			b->faces[b->numfaces++] = face;
		}
	}

	// discard any corners that are no longer on the surface of the solid
	for (i = 0; i < b->numcorners; i++)
	{
		convex_corner_t ca = b->corners[i];
		for (j = 0; j < b->numfaces; j++)
		{
			const convex_face_t *f = b->faces + j;
			if (ca.x * f->x + ca.y * f->y + ca.z * f->z + ca.w * f->w > -epsilon)
				break;
		}
		// if we didn't find any face that uses this corner, remove the corner
		removedcorner[i] = (j == b->numfaces);
	}

	// scan for removed corners and remove them
	for (i = 0; i < b->numcorners; i++)
		if (removedcorner[i])
			break;
	for (j = i + 1;j < b->numcorners;j++)
		if (!removedcorner[j])
			b->corners[i++] = b->corners[j];
	b->numcorners = i;

	// add the new corner
	b->corners[b->numcorners++] = corner;
}

int convex_builder_get_planes4f(convex_builder_state_t* b, float* outplanes4f, int maxplanes, int positivew)
{
	int i;
	int n = b->numfaces < maxplanes ? b->numfaces : maxplanes;
	if (positivew)
	{
		for (i = 0; i < n; i++)
		{
			const convex_face_t* f = b->faces + i;
			outplanes4f[i * 4 + 0] = f->x;
			outplanes4f[i * 4 + 1] = f->y;
			outplanes4f[i * 4 + 2] = f->z;
			outplanes4f[i * 4 + 3] = f->w * -1.0f;
		}
	}
	else
	{
		for (i = 0; i < n; i++)
		{
			const convex_face_t* f = b->faces + i;
			outplanes4f[i * 4 + 0] = f->x;
			outplanes4f[i * 4 + 1] = f->y;
			outplanes4f[i * 4 + 2] = f->z;
			outplanes4f[i * 4 + 3] = f->w;
		}
	}
	return b->numfaces;
}

int convex_builder_get_points3f(convex_builder_state_t *b, float* outpoints3f, int maxpoints)
{
	int i;
	int n = b->numcorners < maxpoints ? b->numcorners : maxpoints;
	for (i = 0; i < n; i++)
	{
		const convex_corner_t* c = b->corners + i;
		outpoints3f[i * 3 + 0] = c->x;
		outpoints3f[i * 3 + 1] = c->y;
		outpoints3f[i * 3 + 2] = c->z;
	}
	return b->numcorners;
}
