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

// This module is a variant of the QuickHull algorithm intended to create hulls
// (brushes, aka n-sided polytopes or hulls) from a series of points provided by
// the caller

#pragma once

#ifndef CONVEX_H

enum convex_enums
{
	CONVEX_MAX_CORNERS = 256,
	CONVEX_MAX_FACES = 1024,
};

typedef struct convex_corner_s
{
	float x;
	float y;
	float z;
	float w; // 1.0f
}
convex_corner_t;

typedef struct convex_face_s
{
	// plane equation: a * x + b * y + c * z + d * w = 0.0f
	float x;
	float y;
	float z;
	float w;
}
convex_face_t;

typedef struct convex_builder_state_s
{
	// axially aligned bounding box
	float extents[2][3];

	int numcorners;
	convex_corner_t corners[CONVEX_MAX_CORNERS];

	int numfaces;
	convex_face_t faces[CONVEX_MAX_FACES];

	// we consider points to be equivalent if they are within this distance
	// suggested value is maxextent / 1048576.0f, which is a way of saying 
	// 'equivalent within 20 bits of precision'
	float epsilon;
}
convex_builder_state_t;

// set up a builer state to receive points
void convex_builder_initialize(convex_builder_state_t* b, float epsilon);

// this is a variant of QuickHull that relies on the caller to provide points
// in a reasonable order - the result will be the same regardless of point order
// but it's more efficient if the furthest points are provided first
//
// this could be a little more efficient if we kept track of edges during the
// build, but I think it may be more numerically stable this way
void convex_builder_add_point(convex_builder_state_t* b, float x, float y, float z);

// returns computed faces in array of vec4
// positivew=0 is for plane equations of the form a*x+b*y+c*z+w, which is the
// internal format
// positivew=1 is for plane equations of the form a*x+b*y+c*z-w, which tend to
// be less friendly in terms of vector ops
int convex_builder_get_planes4f(convex_builder_state_t* b, float* outplanes4f, int maxplanes, int positivew);

// returns the points as an array of vec3
// internal format is vec4, so this is just repacking the data
int convex_builder_get_points3f(convex_builder_state_t* b, float* outpoints3f, int maxpoints);

#endif
