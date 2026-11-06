/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../../idlib/precompiled.h"
#pragma hdrstop

#include "AASFile.h"
#include "AASFile_local.h"


//===============================================================
//
//	Environment Sampling
//
//===============================================================

/*
================
idAASFileLocal::EdgeCenter
================
*/
idVec3 idAASFileLocal::EdgeCenter(int edgeNum) const
{
	const aasEdge_t *edge;
	edge = &edges[edgeNum];
	return (vertices[edge->vertexNum[0]] + vertices[edge->vertexNum[1]]) * 0.5f;
}

/*
================
idAASFileLocal::FaceCenter
================
*/
idVec3 idAASFileLocal::FaceCenter(int faceNum) const
{
	int i, edgeNum;
	const aasFace_t *face;
	const aasEdge_t *edge;
	idVec3 center;

	center = vec3_origin;

#ifdef _SPLASHDAMAGE
	edgeNum = faceNum;
	edge = &edges[ abs(edgeNum)];
	center += vertices[ edge->vertexNum[ INTSIGNBITSET(edgeNum)] ];
#else
	face = &faces[faceNum];

	if (face->numEdges > 0) {
		for (i = 0; i < face->numEdges; i++) {
			edgeNum = edgeIndex[ face->firstEdge + i ];
			edge = &edges[ abs(edgeNum)];
			center += vertices[ edge->vertexNum[ INTSIGNBITSET(edgeNum)] ];
		}

		center /= face->numEdges;
	}
#endif

	return center;
}

/*
================
idAASFileLocal::AreaCenter
================
*/
idVec3 idAASFileLocal::AreaCenter(int areaNum) const
{
	int i, faceNum;
	const aasArea_t *area;
	idVec3 center;

	center = vec3_origin;

	area = &areas[areaNum];

#ifdef _SPLASHDAMAGE
	if (area->numEdges > 0) {
		for (i = 0; i < area->numEdges; i++) {
			int edgeNum = edgeIndex[area->firstEdge + i];
			center += EdgeCenter(abs(edgeNum));
		}

		center /= area->numEdges;
	}
#else
	if (area->numFaces > 0) {
		for (i = 0; i < area->numFaces; i++) {
			faceNum = faceIndex[area->firstFace + i];
			center += FaceCenter(abs(faceNum));
		}

		center /= area->numFaces;
	}
#endif

	return center;
}

/*
============
idAASFileLocal::AreaReachableGoal
============
*/
idVec3 idAASFileLocal::AreaReachableGoal(int areaNum) const
{
#ifdef _SPLASHDAMAGE
	return AreaCenter(areaNum);
#else
	int i, faceNum, numFaces;
	const aasArea_t *area;
	idVec3 center;
	idVec3 start, end;
	aasTrace_t trace;

	area = &areas[areaNum];

	if (!(area->flags & (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY)) || (area->flags & AREA_LIQUID)) {
		return AreaCenter(areaNum);
	}

	center = vec3_origin;

	numFaces = 0;

	for (i = 0; i < area->numFaces; i++) {
		faceNum = faceIndex[area->firstFace + i];

		if (!(faces[abs(faceNum)].flags & FACE_FLOOR)) {
			continue;
		}

		center += FaceCenter(abs(faceNum));
		numFaces++;
	}

	if (numFaces > 0) {
		center /= numFaces;
	}

	center[2] += 1.0f;
	end = center;
	end[2] -= 1024;
	Trace(trace, center, end);

	return trace.endpos;
#endif
}

/*
================
idAASFileLocal::EdgeBounds
================
*/
idBounds idAASFileLocal::EdgeBounds(int edgeNum) const
{
	const aasEdge_t *edge;
	idBounds bounds;

	edge = &edges[ abs(edgeNum)];
	bounds[0] = bounds[1] = vertices[ edge->vertexNum[0] ];
	bounds += vertices[ edge->vertexNum[1] ];
	return bounds;
}

/*
================
idAASFileLocal::FaceBounds
================
*/
idBounds idAASFileLocal::FaceBounds(int faceNum) const
{
	int i, edgeNum;
	const aasFace_t *face;
	const aasEdge_t *edge;
	idBounds bounds;

#ifdef _SPLASHDAMAGE
	edgeNum = faceNum;
	bounds.Clear();

	edge = &edges[ abs(edgeNum)];
	bounds.AddPoint(vertices[ edge->vertexNum[ INTSIGNBITSET(edgeNum)] ]);
#else
	face = &faces[faceNum];
	bounds.Clear();

	for (i = 0; i < face->numEdges; i++) {
		edgeNum = edgeIndex[ face->firstEdge + i ];
		edge = &edges[ abs(edgeNum)];
		bounds.AddPoint(vertices[ edge->vertexNum[ INTSIGNBITSET(edgeNum)] ]);
	}
#endif

	return bounds;
}

/*
================
idAASFileLocal::AreaBounds
================
*/
idBounds idAASFileLocal::AreaBounds(int areaNum) const
{
	int i, faceNum;
	const aasArea_t *area;
	idBounds bounds;

	area = &areas[areaNum];
	bounds.Clear();

#ifdef _SPLASHDAMAGE
	for (i = 0; i < area->numEdges; i++) {
		int edgeNum = edgeIndex[area->firstEdge + i];
		bounds += EdgeBounds(abs(edgeNum));
	}
#else
	for (i = 0; i < area->numFaces; i++) {
		faceNum = faceIndex[area->firstFace + i];
		bounds += FaceBounds(abs(faceNum));
	}
#endif

	return bounds;
}

/*
============
idAASFileLocal::PointAreaNum
============
*/
int idAASFileLocal::PointAreaNum(const idVec3 &origin) const
{
	int nodeNum;
	const aasNode_t *node;

	nodeNum = 1;

	do {
		node = &nodes[nodeNum];

#ifdef _SPLASHDAMAGE
		if (planeList[node->planeNum].Distance(origin) <= 0.0f) 
#else
		if (planeList[node->planeNum].Side(origin) == PLANESIDE_BACK) 
#endif
		{
			nodeNum = node->children[1];
		} else {
			nodeNum = node->children[0];
		}

		if (nodeNum < 0) {
			return -nodeNum;
		}
	} while (nodeNum);

	return 0;
}

/*
============
idAASFileLocal::PointReachableAreaNum
============
*/
int idAASFileLocal::PointReachableAreaNum(const idVec3 &origin, const idBounds &searchBounds, const int areaFlags, const int excludeTravelFlags) const
{
#ifdef _SPLASHDAMAGE
	int result;
	int i_v7;
	idVec3 invGravityDir_v8;
	idVec3 originDown_v11;
	idBounds bounds_v12;
	idAASFileLocal::bestReachableArea_t bestReachableArea_v13;
	float maxStepHeight;

	bestReachableArea_v13.v0 = this->settings.boundingBox[1].z - this->settings.boundingBox[0].z;
	bestReachableArea_v13.excludeTravelFlags = excludeTravelFlags;
	bestReachableArea_v13.v1 = 24.0f;
	bestReachableArea_v13.distance1 = idMath::INFINITY;
	bestReachableArea_v13.areaFlags = areaFlags;
	bestReachableArea_v13.distance2 = idMath::INFINITY;
	bestReachableArea_v13.areaNum1 = 0;
	bestReachableArea_v13.areaNum2 = 0;
	idAASFileLocal::PointBestReachableAreaNum(&origin, &bestReachableArea_v13);
	if ( !bestReachableArea_v13.areaNum1 )
	{
		maxStepHeight = this->settings.maxStepHeight;
		invGravityDir_v8 = this->settings.invGravityDir * maxStepHeight;
		originDown_v11 = origin + invGravityDir_v8;
		idAASFileLocal::PointBestReachableAreaNum(&originDown_v11, &bestReachableArea_v13);
	}
	if ( bestReachableArea_v13.v0 > bestReachableArea_v13.distance1 )
		return bestReachableArea_v13.areaNum1;
	i_v7 = 0;
	bounds_v12[0].x = origin.x - 4.0f;
	bounds_v12[0].y = origin.y - 4.0f;
	bounds_v12[1].x = origin.x + 4.0f;
	bounds_v12[1].y = origin.y + 4.0f;
	bounds_v12[1].z = origin.z;
	bounds_v12[0].z = bounds_v12[1].z;
	while ( 1 )
	{
		result = bestReachableArea_v13.areaNum2;
		if ( bestReachableArea_v13.areaNum2 )
			break;
		idAASFileLocal::BoundsBestReachableAreaNum(&bounds_v12, &origin, 1, NULL, &bestReachableArea_v13);
		++i_v7;
		bounds_v12.ExpandSelf(4.0f);
		if ( i_v7 >= 4 )
		{
			result = bestReachableArea_v13.areaNum2;
			if ( !bestReachableArea_v13.areaNum2 )
				return bestReachableArea_v13.areaNum1;
			return result;
		}
	}
	return result;
#else
	int areaList[32], areaNum, i;
	idVec3 start, end, pointList[32];
	aasTrace_t trace;
	idBounds bounds;
	float frac;

	start = origin;

	trace.areas = areaList;
	trace.points = pointList;
	trace.maxAreas = sizeof(areaList) / sizeof(int);
	trace.getOutOfSolid = true;

	areaNum = PointAreaNum(start);

	if (areaNum) {
		if ((areas[areaNum].flags & areaFlags) && ((areas[areaNum].travelFlags & excludeTravelFlags) == 0)) {
			return areaNum;
		}
	} else {
		// trace up
		end = start;
		end[2] += 32.0f;
		Trace(trace, start, end);

		if (trace.numAreas >= 1) {
			if ((areas[0].flags & areaFlags) && ((areas[0].travelFlags & excludeTravelFlags) == 0)) {
				return areaList[0];
			}

			start = pointList[0];
			start[2] += 1.0f;
		}
	}

	// trace down
	end = start;
	end[2] -= 32.0f;
	Trace(trace, start, end);

	if (trace.lastAreaNum) {
		if ((areas[trace.lastAreaNum].flags & areaFlags) && ((areas[trace.lastAreaNum].travelFlags & excludeTravelFlags) == 0)) {
			return trace.lastAreaNum;
		}

		start = trace.endpos;
	}

	// expand bounds until an area is found
	for (i = 1; i <= 12; i++) {
		frac = i * (1.0f / 12.0f);
		bounds[0] = origin + searchBounds[0] * frac;
		bounds[1] = origin + searchBounds[1] * frac;
		areaNum = BoundsReachableAreaNum(bounds, areaFlags, excludeTravelFlags);

		if (areaNum && (areas[areaNum].flags & areaFlags) && ((areas[areaNum].travelFlags & excludeTravelFlags) == 0)) {
			return areaNum;
		}
	}

	return 0;
#endif
}

/*
============
idAASFileLocal::BoundsReachableAreaNum_r
============
*/
int idAASFileLocal::BoundsReachableAreaNum_r(int nodeNum, const idBounds &bounds, const int areaFlags, const int excludeTravelFlags) const
{
	int res;
	const aasNode_t *node;

	while (nodeNum) {
		if (nodeNum < 0) {
			if ((areas[-nodeNum].flags & areaFlags) && ((areas[-nodeNum].travelFlags & excludeTravelFlags) == 0)) {
				return -nodeNum;
			}

			return 0;
		}

		node = &nodes[nodeNum];
		res = bounds.PlaneSide(planeList[node->planeNum]);

		if (res == PLANESIDE_BACK) {
			nodeNum = node->children[1];
		} else if (res == PLANESIDE_FRONT) {
			nodeNum = node->children[0];
		} else {
			nodeNum = BoundsReachableAreaNum_r(node->children[1], bounds, areaFlags, excludeTravelFlags);

			if (nodeNum) {
				return nodeNum;
			}

			nodeNum = node->children[0];
		}
	}

	return 0;
}

/*
============
idAASFileLocal::BoundsReachableAreaNum
============
*/
int idAASFileLocal::BoundsReachableAreaNum(const idBounds &bounds, const int areaFlags, const int excludeTravelFlags) const
{
#ifdef _SPLASHDAMAGE
	return BoundsReachableAreaNum_r(&bounds, 1, areaFlags, excludeTravelFlags);
#else
	return BoundsReachableAreaNum_r(1, bounds, areaFlags, excludeTravelFlags);
#endif
}

/*
============
idAASFileLocal::PushPointIntoAreaNum
============
*/
void idAASFileLocal::PushPointIntoAreaNum(int areaNum, idVec3 &point) const
{
#ifdef _SPLASHDAMAGE
	PushPointIntoArea(areaNum, point);
#else
	int i, faceNum;
	const aasArea_t *area;
	const aasFace_t *face;

	area = &areas[areaNum];

	// push the point to the right side of all area face planes
	for (i = 0; i < area->numFaces; i++) {
		faceNum = faceIndex[area->firstFace + i];
		face = &faces[abs(faceNum)];

		const idPlane &plane = planeList[face->planeNum ^ INTSIGNBITSET(faceNum)];
		float dist = plane.Distance(point);

		// project the point onto the face plane if it is on the wrong side
		if (dist < 0.0f) {
			point -= dist * plane.Normal();
		}
	}
#endif
}

/*
============
idAASFileLocal::Trace
============
*/
#define TRACEPLANE_EPSILON		0.125f

typedef struct aasTraceStack_s {
	idVec3			start;
	idVec3			end;
	int				planeNum;
	int				nodeNum;
} aasTraceStack_t;

bool idAASFileLocal::Trace(aasTrace_t &trace, const idVec3 &start, const idVec3 &end) const
{
#ifdef _SPLASHDAMAGE
	return Trace(&trace, &start, &end);
#else
	int side, nodeNum, tmpPlaneNum;
	double front, back, frac;
	idVec3 cur_start, cur_end, cur_mid, v1, v2;
	aasTraceStack_t tracestack[MAX_AAS_TREE_DEPTH];
	aasTraceStack_t *tstack_p;
	const aasNode_t *node;
	const idPlane *plane;

	trace.numAreas = 0;
	trace.lastAreaNum = 0;
	trace.blockingAreaNum = 0;

	tstack_p = tracestack;
	tstack_p->start = start;
	tstack_p->end = end;
	tstack_p->planeNum = 0;
	tstack_p->nodeNum = 1;		//start with the root of the tree
	tstack_p++;

	while (1) {

		tstack_p--;

		// if the trace stack is empty
		if (tstack_p < tracestack) {
			if (!trace.lastAreaNum) {
				// completely in solid
				trace.fraction = 0.0f;
				trace.endpos = start;
			} else {
				// nothing was hit
				trace.fraction = 1.0f;
				trace.endpos = end;
			}

			trace.planeNum = 0;
			return false;
		}

		// number of the current node to test the line against
		nodeNum = tstack_p->nodeNum;

		// if it is an area
		if (nodeNum < 0) {
			// if can't enter the area
			if ((areas[-nodeNum].flags & trace.flags) || (areas[-nodeNum].travelFlags & trace.travelFlags)) {
				if (!trace.lastAreaNum) {
					trace.fraction = 0.0f;
					v1 = vec3_origin;
				} else {
					v1 = end - start;
					v2 = tstack_p->start - start;
					trace.fraction = v2.Length() / v1.Length();
				}

				trace.endpos = tstack_p->start;
				trace.blockingAreaNum = -nodeNum;
				trace.planeNum = tstack_p->planeNum;
				// always take the plane with normal facing towards the trace start
				plane = &planeList[trace.planeNum];

				if (v1 * plane->Normal() > 0.0f) {
					trace.planeNum ^= 1;
				}

				return true;
			}

			trace.lastAreaNum = -nodeNum;

			if (trace.numAreas < trace.maxAreas) {
				if (trace.areas) {
					trace.areas[trace.numAreas] = -nodeNum;
				}

				if (trace.points) {
					trace.points[trace.numAreas] = tstack_p->start;
				}

				trace.numAreas++;
			}

			continue;
		}

		// if it is a solid leaf
		if (!nodeNum) {
			if (!trace.lastAreaNum) {
				trace.fraction = 0.0f;
				v1 = vec3_origin;
			} else {
				v1 = end - start;
				v2 = tstack_p->start - start;
				trace.fraction = v2.Length() / v1.Length();
			}

			trace.endpos = tstack_p->start;
			trace.blockingAreaNum = 0;	// hit solid leaf
			trace.planeNum = tstack_p->planeNum;
			// always take the plane with normal facing towards the trace start
			plane = &planeList[trace.planeNum];

			if (v1 * plane->Normal() > 0.0f) {
				trace.planeNum ^= 1;
			}

			if (!trace.lastAreaNum && trace.getOutOfSolid) {
				continue;
			} else {
				return true;
			}
		}

		// the node to test against
		node = &nodes[nodeNum];
		// start point of current line to test against node
		cur_start = tstack_p->start;
		// end point of the current line to test against node
		cur_end = tstack_p->end;
		// the current node plane
		plane = &planeList[node->planeNum];

		front = plane->Distance(cur_start);
		back = plane->Distance(cur_end);

		// if the whole to be traced line is totally at the front of this node
		// only go down the tree with the front child
		if (front >= -ON_EPSILON && back >= -ON_EPSILON) {
			// keep the current start and end point on the stack and go down the tree with the front child
			tstack_p->nodeNum = node->children[0];
			tstack_p++;

			if (tstack_p >= &tracestack[MAX_AAS_TREE_DEPTH]) {
				common->Error("idAASFileLocal::Trace: stack overflow\n");
				return false;
			}
		}
		// if the whole to be traced line is totally at the back of this node
		// only go down the tree with the back child
		else if (front < ON_EPSILON && back < ON_EPSILON) {
			// keep the current start and end point on the stack and go down the tree with the back child
			tstack_p->nodeNum = node->children[1];
			tstack_p++;

			if (tstack_p >= &tracestack[MAX_AAS_TREE_DEPTH]) {
				common->Error("idAASFileLocal::Trace: stack overflow\n");
				return false;
			}
		}
		// go down the tree both at the front and back of the node
		else {
			tmpPlaneNum = tstack_p->planeNum;

			// calculate the hit point with the node plane
			// put the cross point TRACEPLANE_EPSILON on the near side
			if (front < 0) {
				frac = (front + TRACEPLANE_EPSILON) / (front - back);
			} else {
				frac = (front - TRACEPLANE_EPSILON) / (front - back);
			}

			if (frac < 0) {
				frac = 0.001f; //0
			} else if (frac > 1) {
				frac = 0.999f; //1
			}

			cur_mid = cur_start + (cur_end - cur_start) * frac;

			// side the front part of the line is on
			side = front < 0;

			// first put the end part of the line on the stack (back side)
			tstack_p->start = cur_mid;
			tstack_p->planeNum = node->planeNum;
			tstack_p->nodeNum = node->children[!side];
			tstack_p++;

			if (tstack_p >= &tracestack[MAX_AAS_TREE_DEPTH]) {
				common->Error("idAASFileLocal::Trace: stack overflow\n");
				return false;
			}

			// now put the part near the start of the line on the stack so we will
			// continue with that part first.
			tstack_p->start = cur_start;
			tstack_p->end = cur_mid;
			tstack_p->planeNum = tmpPlaneNum;
			tstack_p->nodeNum = node->children[side];
			tstack_p++;

			if (tstack_p >= &tracestack[MAX_AAS_TREE_DEPTH]) {
				common->Error("idAASFileLocal::Trace: stack overflow\n");
				return false;
			}
		}
	}

	return false;
#endif
}

/*
============
idAASLocal::AreaContentsTravelFlags
============
*/
int idAASFileLocal::AreaContentsTravelFlags(int areaNum) const
{
	if (areas[areaNum].contents & AREACONTENTS_WATER) {
		return TFL_WATER;
	}

	return TFL_AIR;
}

/*
============
idAASFileLocal::MaxTreeDepth_r
============
*/
void idAASFileLocal::MaxTreeDepth_r(int nodeNum, int &depth, int &maxDepth) const
{
	const aasNode_t *node;

	if (nodeNum <= 0) {
		return;
	}

	depth++;

	if (depth > maxDepth) {
		maxDepth = depth;
	}

	node = &nodes[nodeNum];
	MaxTreeDepth_r(node->children[0], depth, maxDepth);
	MaxTreeDepth_r(node->children[1], depth, maxDepth);

	depth--;
}

/*
============
idAASFileLocal::MaxTreeDepth
============
*/
int idAASFileLocal::MaxTreeDepth(void) const
{
	int depth, maxDepth;

	depth = maxDepth = 0;
	MaxTreeDepth_r(1, depth, maxDepth);
	return maxDepth;
}

#ifdef _SPLASHDAMAGE

/*
============
idAASFileLocal::BoundsReachableAreaNum_r
 * Calc area of reachable in node by bounds
 * @param bounds_a2 bounds
 * @param nodeNum_a3 nodeNum
 * @param areaFlags_a4 has area's flags
 * @param excludeTravelFlags_a5 exclude area's travelFlags
 * @return areaNum, 0 is not found
============
*/
int idAASFileLocal::BoundsReachableAreaNum_r(const idBounds *bounds_a2, int nodeNum_a3, int areaFlags_a4, int excludeTravelFlags_a5) const {

	int areaNum_v5;
	bool isAreaNum_v6;
	const aasNode_t *v8;
	int side_v9;
	int result;
	const aasArea_t *area_v11;

	areaNum_v5 = nodeNum_a3;
	isAreaNum_v6 = nodeNum_a3 < 0;
	if ( !nodeNum_a3 )
		return 0;
	while ( !isAreaNum_v6 )
	{
		v8 = &this->nodes[areaNum_v5];
		side_v9 = bounds_a2->PlaneSide(this->planeList[v8->planeNum], ON_EPSILON);
		if ( side_v9 == PLANESIDE_BACK )
		{
			areaNum_v5 = v8->children[1];
		}
		else
		{
			if ( side_v9 )
			{
				result = idAASFileLocal::BoundsReachableAreaNum_r(bounds_a2, v8->children[1], areaFlags_a4, excludeTravelFlags_a5);
				if ( result )
					return result;
			}
			areaNum_v5 = v8->children[0];
		}
		isAreaNum_v6 = areaNum_v5 < 0;
		if ( !areaNum_v5 )
			return 0;
	}
	area_v11 = &this->areas[-areaNum_v5];
	if ( (area_v11->flags & (unsigned short)areaFlags_a4) != 0 && (area_v11->travelFlags & (unsigned short)excludeTravelFlags_a5) == 0 )
		return -areaNum_v5;
	else
		return 0;
}

/*
============
idAASFileLocal::PushPointIntoArea
 *
 * @param areaNum areaNum, positive
 * @param point origin
 * @return
============
*/
bool idAASFileLocal::PushPointIntoArea( int areaNum, idVec3 &point ) const {
	bool ret_v4;
	const aasArea_t *area_v5;
	int edgeIndex_v6;
	unsigned int edgeIndexAbs_v7;
	const aasEdge_t *edge_list;
	bool noVerticleEdge_v9;
	const aasEdge_t *edge_v10;
	const idVec3 *vertex_v11;
	const idVec3 *edgeVertexLT0_p_x;
	const idVec3 *edgeVertexGE0_v13;
	float rightLength_v14;
	float normalX_v16;
	float normalZ_v17;
	float normalY_v18;
	float normalY_v19;
	float normalZ_v20;
	float normalZ_v21;
	float v22;
	float normalX_v23;
	float normalY_v24;
	float normalX_v25;
	float normalY_v26;
	float v27;
	bool result;
	float v29;
	float rightLengthSqr_v30;
	float rightLength_v31;
	float v32;
	float rightLengthInv_v33;
	float dist_v34;
	char cilpped_v35;
	float normalX_v36;
	float normalY_v37;
	float normalZ_v38;
	float v39;
	float dist_v40;
	float dist_v41;
	float dist_v42;
	char v43;
	idVec3 rightNormalized_v44; // v44 v45 v48
	float normalY_v46;
	float normalZ_v49;
	float v52 = 0.0f;
	int edgeIndexNum_v53;
	const aasArea_t *area_v56;
	float maxDistance_v57;
	idVec3 right_v58; // v58 v59 v60
	idVec3 edgeLine_v61; // v61 v62 v63
	idVec3 vertexLT0ToPoint_v64; // v64 v65 v66
	idVec3 vertexLT0ToGE0_v67; // v67 v68 v69
	idVec3 v70; // v70 v71 v72
	idVec3 vertexGE0ToPoint_v73; // v73 v74 v75
	idVec3 v78; // v78 v79 v80
	idVec3 lastPoint_v81; // 0:^34.12
	idVec3 originPoint_v82; // 0:^90.12

	idVec3 v16_19_20;

	maxDistance_v57 = idMath::INFINITY;
	originPoint_v82 = point;
	lastPoint_v81 = point;
	ret_v4 = false;
	area_v5 = &this->areas[areaNum];
	cilpped_v35 = 0;
	v43 = 0;
	area_v56 = area_v5;
	edgeIndexNum_v53 = 0;
	if ( area_v5->numEdges <= 0 )
		return ret_v4;
	do
	{
		edgeIndex_v6 = this->edgeIndex[edgeIndexNum_v53 + area_v5->firstEdge];
		edgeIndexAbs_v7 = abs(edgeIndex_v6);
		edge_list = this->edges.Ptr();
		noVerticleEdge_v9 = (edge_list[edgeIndexAbs_v7].flags & AAS_EDGE_VERTICAL/* 0x40 */) == 0;
		edge_v10 = &edge_list[edgeIndexAbs_v7];
		if ( !noVerticleEdge_v9 )
			goto LABEL_53;
		vertex_v11 = this->vertices.Ptr();
		edgeVertexLT0_p_x = &vertex_v11[edge_v10->vertexNum[(unsigned int)edgeIndex_v6 >> 31]];
		v78 = *edgeVertexLT0_p_x - originPoint_v82;
		edgeVertexGE0_v13 = &vertex_v11[edge_v10->vertexNum[edgeIndex_v6 >= 0]];
		v29 = v78.LengthSqr();
		if ( maxDistance_v57 > v29 )
		{
			maxDistance_v57 = v29;
			lastPoint_v81 = *edgeVertexLT0_p_x;
		}
		edgeLine_v61 = *edgeVertexGE0_v13 - *edgeVertexLT0_p_x;
		right_v58 = edgeLine_v61.Cross(this->settings.invGravityDir);
		rightNormalized_v44 = right_v58;
		rightLengthSqr_v30 = right_v58.LengthSqr();
		rightLength_v31 = sqrt(rightLengthSqr_v30);
		rightLength_v14 = rightLength_v31;
		if ( rightLength_v31 >= idMath::FLT_EPSILON )
		{
			rightLengthInv_v33 = 1.0f / rightLength_v14;
			rightNormalized_v44 = rightLengthInv_v33 * right_v58;
			v32 = rightLength_v14;
		}
		else
		{
			v32 = 0.0f;
		}
		v16_19_20 = rightNormalized_v44;
		v16_19_20.FixDegenerateNormal();
		if ( 0.0f != v32 )
		{
			v39 = v16_19_20 * *edgeVertexGE0_v13;
			v52 = -v39;
		}
		dist_v34 = point * v16_19_20 + v52;
		v27 = dist_v34;
		if ( dist_v34 < 0.0f ) // in back
		{
			cilpped_v35 = 1;
			v70 = v16_19_20 * v27;
			point = point - v70;
			dist_v34 = 0.0f;
		}
		dist_v40 = fabs(dist_v34);
		if ( dist_v40 < 0.1f ) // in plane
		{
			vertexLT0ToGE0_v67 = *edgeVertexGE0_v13 - *edgeVertexLT0_p_x;
			vertexLT0ToPoint_v64 = point - *edgeVertexLT0_p_x;
			dist_v41 = vertexLT0ToPoint_v64 * vertexLT0ToGE0_v67;
			if ( dist_v41 >= 0.0f )
			{
				vertexGE0ToPoint_v73 = point - *edgeVertexGE0_v13;
				dist_v42 = vertexLT0ToGE0_v67 * vertexGE0ToPoint_v73;
				if ( 0.0f >= dist_v42 )
					v43 = 1;
			}
		}
		ret_v4 = cilpped_v35;
		area_v5 = area_v56;
LABEL_53:
		++edgeIndexNum_v53;
	}
	while ( edgeIndexNum_v53 < area_v5->numEdges );
	if ( !ret_v4 )
		return ret_v4;
	result = ret_v4;
	if ( !v43 )
		point = lastPoint_v81;
	return result;
}

/*
============
idAASFileLocal::TraceHeight
 *
 * @param trace save traced height info
 * @param start start position
 * @param end end position
 * @return
============
*/
bool idAASFileLocal::TraceHeight(aasTraceHeight_t& trace, const idVec3& start, const idVec3& end) const
{
	aasTraceStack_t* tstack_p_v4;
	int nodeNum_v6;
	const aasNode_t* list;
	int nodeNum_v8;
	bool noHeightFlag_v9;
	const aasNode_t* node_v10;
	int numPoints;
	idVec3* points;
	int numPoints_v13;
	idVec3* tracePoint_v14;
	int planeNum;
	const idPlane* plane_v16;
	float front_v20;
	float back_v21;
	int nodeNum_v22;
	int tmpPlaneNum_v23;
	float v24;
	double frac_v25;
	double frac2_v27;
	double front_v29;
	double back_v30;
	float frac_v31;
	idVec3 cur_end_v32; // v32 v33 v34
	idVec3 curLine_v35; // v35 v36 v37
	idVec3 v38; // v38 v39 v40
	idVec3 cur_mid_v41; // v41 v42 v43
	idVec3 cur_start_v44; // v44 v45 v46
	aasTraceStack_t tracestack_v47[MAX_AAS_TREE_DEPTH]; // 1024
	aasTraceStack_t* retaddr = &tracestack_v47[MAX_AAS_TREE_DEPTH]; // 1024 - 32

	idVec3 cur_start_v17; // v17 v18 v19
	tstack_p_v4 = tracestack_v47;
	trace.numPoints = 0;
	tstack_p_v4->start = start;
	tstack_p_v4->planeNum = 0;
	tstack_p_v4->nodeNum = 1; // LODWORD(v47[7]) = 1; #define LODWORD(x) ((DWORD)(*(uint64_t *)&(x)))
	tstack_p_v4->end = end;
	while (1)
	{
		nodeNum_v6 = tstack_p_v4->nodeNum;
		if (!nodeNum_v6)
			goto LABEL_23;
		list = this->nodes.Ptr();
		nodeNum_v8 = nodeNum_v6;
		noHeightFlag_v9 = (list[nodeNum_v8].flags & AAS_NODE_FLAG_COLUMN_HEIGHT) == 0; // 2
		node_v10 = &list[nodeNum_v8];
		if (!noHeightFlag_v9)
		{
			numPoints = trace.numPoints;
			if (numPoints < trace.maxPoints)
			{
				points = trace.points;
				numPoints_v13 = numPoints;
				points[numPoints_v13] = tstack_p_v4->start;
				tracePoint_v14 = &points[numPoints_v13];
				trace.points[trace.numPoints++].z = (float)((node_v10->flags >> AAS_NODE_FLAG_COLUMN_HEIGHT/* 2 */) -
					AAS_NODE_FLAG_COLUMN_HEIGHT_OFFSET/* 0x2000 */);
			}
			goto LABEL_23;
		}
		planeNum = node_v10->planeNum;
		cur_start_v44 = tstack_p_v4->start;
		plane_v16 = &this->planeList[planeNum];
		cur_end_v32 = tstack_p_v4->end;
		cur_start_v17 = cur_start_v44;
		front_v29 = plane_v16->Distance(cur_start_v44);
		front_v20 = front_v29;
		back_v30 = plane_v16->Distance(cur_end_v32);
		back_v21 = back_v30;
		if (front_v20 >= -ON_EPSILON && back_v21 >= -ON_EPSILON)
		{
			nodeNum_v22 = node_v10->children[0];
			goto LABEL_22;
		}
		if (front_v20 < ON_EPSILON && back_v21 < ON_EPSILON)
		{
			nodeNum_v22 = node_v10->children[1];
			goto LABEL_22;
		}
		tmpPlaneNum_v23 = tstack_p_v4->planeNum;
		v24 = front_v20 >= 0.0f ? front_v20 - TRACEPLANE_EPSILON : (front_v20 + TRACEPLANE_EPSILON);
		frac_v25 = v24 / (front_v20 - back_v21);
		if (frac_v25 >= 0.0f)
		{
			if (frac_v25 <= 1.0f)
			{
				frac2_v27 = frac_v25;
			}
			else
			{
				frac2_v27 = 0.999f;
			}
		}
		else
		{
			frac2_v27 = 0.001f;
		}
		curLine_v35 = cur_end_v32 - cur_start_v17;
		frac_v31 = frac2_v27;
		v38 = curLine_v35 * frac_v31;
		cur_mid_v41 = v38 + cur_start_v17;
		tstack_p_v4->planeNum = planeNum;
		tstack_p_v4->start = cur_mid_v41;
		tstack_p_v4->nodeNum = node_v10->children[0.0f <= front_v20];
		tstack_p_v4++;
		if (tstack_p_v4 >= retaddr)
			break;
		nodeNum_v22 = node_v10->children[0.0f > front_v20];
		tstack_p_v4->start = cur_start_v44;
		tstack_p_v4->end = cur_mid_v41;
		tstack_p_v4->planeNum = tmpPlaneNum_v23;
LABEL_22:
		tstack_p_v4->nodeNum = nodeNum_v22;
		tstack_p_v4++;
		if (tstack_p_v4 >= retaddr)
			break;
LABEL_23:
		tstack_p_v4--;
		if (tstack_p_v4 < tracestack_v47)
			return true;
	}
	common->Warning("idAASFileLocal::Trace: stack overflow");
	return false;
}

/*
============
idAASFileLocal::TraceFloor
 *
 * @param trace save traced floor info
 * @param start start position
 * @param startAreaNum start areaNum
 * @param end end position
 * @param endAreaNum end areaNum
 * @param travelFlags reachable umask
 * @return
============
*/
bool idAASFileLocal::TraceFloor(aasTraceFloor_t& trace, const idVec3& start, int startAreaNum, const idVec3& end, int endAreaNum, int travelFlags) const
{
	struct aasTraceFloor_t* v7; // ebp
	float a; // st6
	float v10; // st5
	float c; // st3
	float b; // st2
	float v13; // st7
	float v14; // st5
	float v15; // st7
	float v16; // st5
	float v17; // st4
	float v18; // st6
	float v19; // rt0
	float v20; // st4
	float v21; // st6
	float v22; // st5
	float v23; // st4
	float v24; // st5
	float v25; // st7
	float v26; // st4
	float v27; // st7
	float v28; // st4
	float v29; // st4
	float v30; // st6
	float v31; // rtt
	float v32; // st4
	float v33; // st3
	float x; // st7
	int i; // eax
	int v36; // edx
	int granularity; // eax
	bool v38; // cc
	int num; // ebx
	int size; // eax
	int v41; // eax
	int* list; // edi
	int j; // eax
	float v44; // st7
	int v45; // edi
	int v46; // eax
	float v47; // st4
	const aasReachability_t* v48;
	//float v48; // ecx
	float v49; // st5
	float v50; // st7
	int v51; // ebx
	const aasArea_t* v52; // eax
	int v53; // eax
	int v54; // ebp
	int v55; // eax
	int v56; // eax
	int* v57; // edi
	int k; // eax
	float v59; // st7
	float v60; // st5
	int v61; // ebp
	int m; // eax
	int v63; // edx
	float v64; // st7
	float v65; // st7
	int* v66; // edi
	int n; // eax
	//float v68; // eax
	const aasReachability_t* v68; // eax
	int ii; // eax
	int v70; // edx
	float v72; // [esp+8h] [ebp-84h]
	float v73; // [esp+8h] [ebp-84h]
	float v74; // [esp+8h] [ebp-84h]
	float v75; // [esp+8h] [ebp-84h]
	float v76; // [esp+8h] [ebp-84h]
	float v77; // [esp+8h] [ebp-84h]
	float v78; // [esp+8h] [ebp-84h]
	float v79; // [esp+8h] [ebp-84h]
	float v80; // [esp+8h] [ebp-84h]
	float v81; // [esp+8h] [ebp-84h]
	float v82; // [esp+8h] [ebp-84h]
	float v83; // [esp+8h] [ebp-84h]
	float v84; // [esp+8h] [ebp-84h]
	float v85; // [esp+8h] [ebp-84h]
	float v86; // [esp+8h] [ebp-84h]
	float v87; // [esp+Ch] [ebp-80h]
	float v88; // [esp+Ch] [ebp-80h]
	float v89; // [esp+Ch] [ebp-80h]
	float v90; // [esp+Ch] [ebp-80h]
	float v91; // [esp+Ch] [ebp-80h]
	float v92; // [esp+Ch] [ebp-80h]
	float v93; // [esp+Ch] [ebp-80h]
	float v94; // [esp+10h] [ebp-7Ch]
	idVec3 v95; // v95 v100 v105
	//float v95; // [esp+10h] [ebp-7Ch]
	idVec3 v96; // v96 v101 v106
	//float v96; // [esp+10h] [ebp-7Ch]
	idVec3 v97; // v97 v102 v107
	//float v97; // [esp+10h] [ebp-7Ch]
	float v98; // [esp+10h] [ebp-7Ch]
	float v99; // [esp+14h] [ebp-78h]
	float v103; // [esp+14h] [ebp-78h]
	float v104; // [esp+18h] [ebp-74h]
	float v108; // [esp+18h] [ebp-74h]
	idPlane toForwardPlane_v109; // [esp+1Ch] [ebp-70h] BYREF // point to forward
	int v110; // [esp+2Ch] [ebp-60h]
	idPlane toLeftPlane_v111; // [esp+30h] [ebp-5Ch] BYREF // point to left
	float v112; // [esp+40h] [ebp-4Ch]
	idVec3 v113; // v113 v114 v115
	int v116; // [esp+50h] [ebp-3Ch]
	floorEdgeSplitPoint_t minSplitPoint_v122; // v122 v123 v124 v125 v126
	floorEdgeSplitPoint_t maxSplitPoint_v117; // v117 y z v120 v121
	float v127; // [esp+88h] [ebp-4h]

	idList<aasArea_t>& areas = (idList<aasArea_t>&)this->areas;
	idList<int>& searchAreaList = (idList<int>&)this->searchAreaList;

	v7 = &trace;
	trace.fraction = 0.0f;
	trace.endpos = start;
	trace.lastAreaNum = startAreaNum;
	trace.lastEdgeNum = 0;
	v113 = end - start;
	idVec3 normal = v113.Cross(this->settings.gravityDir); // left
	toLeftPlane_v111.SetNormal(normal);
	toLeftPlane_v111.Normalize(true);
	a = toLeftPlane_v111[0];
	v10 = -1.0f;
	c = toLeftPlane_v111[2];
	b = toLeftPlane_v111[1];
#if 0
	if (toLeftPlane_v111[0] != 0.0f)
	{
		if (toLeftPlane_v111[1] == 0.0f && 0.0f == c)
		{
			v15 = toLeftPlane_v111[1];
			if (a <= 0.0f)
			{
				v17 = toLeftPlane_v111[2];
				if (-1.0f == a)
				{
				LABEL_43:
					v16 = v17;
					goto LABEL_23;
				}
				v18 = toLeftPlane_v111[2];
			}
			else
			{
				v16 = toLeftPlane_v111[2];
				if (1.0f == a)
					goto LABEL_23;
				v10 = 1.0f;
				v18 = toLeftPlane_v111[2];
			}
			toLeftPlane_v111[0] = v10;
			goto LABEL_22;
		}
	LABEL_28:
		v14 = toLeftPlane_v111[1];
		v79 = fabs(a);
		if (v79 == 1.0f)
		{
			v20 = toLeftPlane_v111[2];
			if (0.0f != v14 || 0.0f != v20)
			{
				toLeftPlane_v111[2] = 0.0f;
				toLeftPlane_v111[1] = 0.0f;
				v13 = 0.0f;
				v14 = v13;
				goto LABEL_24;
			}
			goto LABEL_44;
		}
		v80 = fabs(v14);
		if (v80 != 1.0f)
		{
			v81 = fabs(c);
			v20 = toLeftPlane_v111[2];
			if (v81 == 1.0f && (0.0f != a || 0.0f != v14))
			{
				v13 = toLeftPlane_v111[2];
				toLeftPlane_v111[1] = 0.0f;
				toLeftPlane_v111[0] = 0.0f;
				v14 = 0.0f;
				a = v14;
				goto LABEL_24;
			}
			goto LABEL_44;
		}
		v20 = toLeftPlane_v111[2];
		if (0.0f == a && 0.0f == v20)
		{
		LABEL_44:
			v13 = v20;
			goto LABEL_24;
		}
		v15 = toLeftPlane_v111[1];
		toLeftPlane_v111[2] = 0.0f;
		toLeftPlane_v111[0] = 0.0f;
		v18 = 0.0f;
	LABEL_22:
		v16 = v18;
		a = toLeftPlane_v111[0];
		goto LABEL_23;
	}
	if (toLeftPlane_v111[1] != 0.0f)
	{
		if (0.0f == c)
		{
			v15 = toLeftPlane_v111[1];
			if (b > 0.0f)
			{
				v16 = toLeftPlane_v111[2];
				if (1.0f != b)
				{
					v13 = toLeftPlane_v111[2];
					toLeftPlane_v111[1] = 1.0f;
					v14 = 1.0f;
					goto LABEL_24;
				}
				goto LABEL_23;
			}
			v17 = toLeftPlane_v111[2];
			if (-1.0f != b)
			{
				v13 = toLeftPlane_v111[2];
				toLeftPlane_v111[1] = -1.0f;
				v14 = -1.0f;
				goto LABEL_24;
			}
			goto LABEL_43;
		}
		goto LABEL_28;
	}
	v13 = toLeftPlane_v111[2];
	if (toLeftPlane_v111[2] <= 0.0f)
	{
		if (-1.0f != v13)
		{
			v15 = toLeftPlane_v111[1];
			toLeftPlane_v111[2] = -1.0f;
			v16 = -1.0f;
			goto LABEL_23;
		}
		v14 = toLeftPlane_v111[1];
	}
	else
	{
		v14 = toLeftPlane_v111[1];
		if (1.0f != v13)
		{
			v15 = toLeftPlane_v111[1];
			toLeftPlane_v111[2] = 1.0f;
			v16 = 1.0f;
		LABEL_23:
			v19 = v16;
			v14 = v15;
			v13 = v19;
		}
	}
LABEL_24:
	v75 = v14 * start.y + start.x * a + v13 * start.z;
	toLeftPlane_v111[3] = -v75;
	v95.x = v13 * this->settings.gravityDir.y - this->settings.gravityDir.z * v14;
	v95.y = this->settings.gravityDir.z * a - v13 * this->settings.gravityDir.x;
	v95.z = v14 * this->settings.gravityDir.x - a * this->settings.gravityDir.y;
#else
	v14 = toLeftPlane_v111[1];
	v13 = toLeftPlane_v111[2];
	a = toLeftPlane_v111[0];
	toLeftPlane_v111.FitThroughPoint(start);
	v95 = this->settings.gravityDir.Cross(toLeftPlane_v111.Normal()); // point to start
#endif
	toForwardPlane_v109.SetNormal(v95);
	toForwardPlane_v109.Normalize(true);
#if 0
	v21 = toForwardPlane_v109[0];
	v22 = toForwardPlane_v109[2];
	v23 = toForwardPlane_v109[1];
	if (toForwardPlane_v109[0] != 0.0f)
	{
		if (toForwardPlane_v109[1] == 0.0f && 0.0f == v22)
		{
			v27 = toForwardPlane_v109[1];
			if (v21 <= 0.0f)
				v29 = -1.0f;
			else
				v29 = 1.0f;
			if (v29 == v21)
				goto LABEL_66;
			v30 = toForwardPlane_v109[2];
			toForwardPlane_v109[0] = v29;
			goto LABEL_65;
		}
	LABEL_77:
		v84 = fabs(v21);
		if (1.0f == v84)
		{
			if (0.0f != v23 || 0.0f != v22)
			{
				toForwardPlane_v109[2] = 0.0f;
				toForwardPlane_v109[1] = 0.0f;
				v25 = 0.0f;
				v24 = v25;
				goto LABEL_67;
			}
			goto LABEL_91;
		}
		v85 = fabs(v23);
		if (v85 == 1.0f)
		{
			if (0.0f != v21 || 0.0f != v22)
			{
				v27 = toForwardPlane_v109[1];
				toForwardPlane_v109[2] = 0.0f;
				toForwardPlane_v109[0] = 0.0f;
				v30 = 0.0f;
			LABEL_65:
				v22 = v30;
				v21 = toForwardPlane_v109[0];
			LABEL_66:
				v31 = v22;
				v24 = v27;
				v25 = v31;
				goto LABEL_67;
			}
		}
		else
		{
			v86 = fabs(v22);
			if (v86 == 1.0f && (0.0f != v21 || 0.0f != v23))
			{
				v25 = toForwardPlane_v109[2];
				toForwardPlane_v109[1] = 0.0f;
				toForwardPlane_v109[0] = 0.0f;
				v24 = 0.0f;
				v21 = v24;
				goto LABEL_67;
			}
		}
	LABEL_91:
		v27 = toForwardPlane_v109[1];
		goto LABEL_66;
	}
	if (toForwardPlane_v109[1] == 0.0f)
	{
		v24 = toForwardPlane_v109[1];
		v25 = toForwardPlane_v109[2];
		if (toForwardPlane_v109[2] <= 0.0f)
			v26 = -1.0f;
		else
			v26 = 1.0f;
		if (v26 == v25)
			goto LABEL_67;
		v27 = toForwardPlane_v109[1];
		toForwardPlane_v109[2] = v26;
		v22 = toForwardPlane_v109[2];
		goto LABEL_66;
	}
	if (0.0f != v22)
		goto LABEL_77;
	v27 = toForwardPlane_v109[1];
	if (v23 <= 0.0f)
		v28 = -1.0f;
	else
		v28 = 1.0f;
	if (v28 == v27)
		goto LABEL_66;
	v25 = toForwardPlane_v109[2];
	toForwardPlane_v109[1] = v28;
	v24 = toForwardPlane_v109[1];
LABEL_67:
	v32 = v24 * start.y;
	v33 = start.x * v21;
	v116 = startAreaNum;
	v82 = v32 + v33 + v25 * start.z;
	toForwardPlane_v109[3] = -v82;
#else
	v24 = toForwardPlane_v109[1];
	v21 = toForwardPlane_v109[0];
	v25 = toForwardPlane_v109[2];
	v116 = startAreaNum;
	toForwardPlane_v109.FitThroughPoint(start);
#endif
#if 0
	ID_IF_DBG()
	{
		//printf("Left: %s|Forward: %s\n", toLeftPlane_v111.Normal().ToString(), toForwardPlane_v109.Normal().ToString());
		session->rw->DebugArrow(colorOrange, start, start + toLeftPlane_v111.Normal() * 1000, 10);
		session->rw->DebugArrow(colorBlue, start, start + toForwardPlane_v109.Normal() * 1000, 10);
		//session->rw->DebugAxis(toForwardPlane_v109.Normal() * -toForwardPlane_v109[3], toForwardPlane_v109.Normal().ToAngles().ToMat3());
		AAS_DrawPoint(maxSplitPoint_v117.point, colorBlue);
		AAS_DrawPoint(minSplitPoint_v122.point, colorRed);
	}
#endif
	v83 = v25 * end.z + v21 * end.x + v24 * end.y;
	v127 = -v83;
	//ID_IF_DBG() aaa=1;
	if (!idAASFileLocal::GetFloorEdgeSplitPoints(
		(struct idAASFileLocal::floorEdgeSplitPoint_t*)&minSplitPoint_v122,
		(struct idAASFileLocal::floorEdgeSplitPoint_t*)&maxSplitPoint_v117,
		startAreaNum,
		&toLeftPlane_v111,
		&toForwardPlane_v109))
	{
		x = start.x;
		maxSplitPoint_v117.edgeIndex = 0;
		maxSplitPoint_v117.point = start;
		minSplitPoint_v122.edgeIndex = 0;
		minSplitPoint_v122.point = maxSplitPoint_v117.point;
		maxSplitPoint_v117.distance = 0.0f;
		minSplitPoint_v122.distance = 0.0f;
	}
#if 0
  else
	{
		ID_IF_DBG()
		{
			//printf("Left: %s|Forward: %s\n", toLeftPlane_v111.Normal().ToString(), toForwardPlane_v109.Normal().ToString());
			session->rw->DrawText(va("A: %d", startAreaNum), minSplitPoint_v122.point, 1.0f, colorRed, mat3_identity);
			session->rw->DebugCircle(colorRed, minSplitPoint_v122.point, idVec3(0, 0, 1), 30.0f, 8);
			session->rw->DrawText(va("B: %d", startAreaNum), maxSplitPoint_v117.point, 1.0f, colorRed, mat3_identity);
			session->rw->DebugCircle(colorRed, maxSplitPoint_v117.point, idVec3(0, 0, 1), 30.0f, 8);
			session->rw->DebugArrow(colorRed, minSplitPoint_v122.point, maxSplitPoint_v117.point, 5);
		}
	}
#endif
	// clear last search area bit
	for (i = 0; i < this->searchAreaList.Num(); ++i)
	{
		v36 = this->searchAreaList[i];
		areas[v36].flags &= ~AAS_AREA_FLOOD_VISITED /* 0x8000u */;
	}
	searchAreaList.Clear();
	while (1)
	{
		v44 = maxSplitPoint_v117.point[0];
		v45 = v116;
		searchAreaList.Append(v116);
		areas[v45].flags |= AAS_AREA_FLOOD_VISITED /* 0x8000u */;
		v7->endpos = maxSplitPoint_v117.point;
		v46 = maxSplitPoint_v117.edgeIndex;
		v7->lastEdgeNum = v46;
		if (v45 == endAreaNum)
			break;
		v112 = toForwardPlane_v109.Normal() * v7->endpos + v127;
		if (v112 > 0.1f)
			break;
		v47 = v7->endpos.y;
		v48 = this->areas[v45].reach;
		v49 = toForwardPlane_v109[0] * v7->endpos.x;
		v110 = this->searchAreaList.Num();
		v112 = toForwardPlane_v109.Normal() * v7->endpos;
		v50 = v112;
		const aasReachability_t* _v112 = v48;
		toForwardPlane_v109[3] = -v50;
		if (v48 == NULL)
		{
		LABEL_150:
			v113 = v7->endpos - start;
			idVec3 _v98 = end - start;
			float _v110 = v113.LengthSqr();
			v64 = _v110;
			_v110 = _v98.LengthSqr();
			_v110 = v64 / _v110;
			_v110 = sqrt(_v110);
			v65 = _v110;
			goto LABEL_159;
		}
		while (1)
		{
			if ((v48->travelFlags & ~travelFlags) == 0)
			{
				v51 = v48->toAreaNum; // + 6
				v52 = &this->areas[v51];
				if ((v52->travelFlags & ~travelFlags) == 0 && (v52->flags & AAS_AREA_FLOOD_VISITED /* 0x8000u */) == 0)
				{
					searchAreaList.Append(v51);
					areas[v51].flags |= AAS_AREA_FLOOD_VISITED/* 0x8000u */;
					idAASFileLocal::GetFloorEdgeSplitPoints(
						(struct idAASFileLocal::floorEdgeSplitPoint_t*)&minSplitPoint_v122,
						(struct idAASFileLocal::floorEdgeSplitPoint_t*)&maxSplitPoint_v117,
						_v112->toAreaNum,
						&toLeftPlane_v111,
						&toForwardPlane_v109);
					if (minSplitPoint_v122.distance < idMath::INFINITY && maxSplitPoint_v117.distance >= 0.1f)
					{
						v96 = trace.endpos - minSplitPoint_v122.point;
#if 0
						ID_IF_DBG()
						{
							AAS_DrawPoint(maxSplitPoint_v117.point, colorBlue);
							AAS_DrawPoint(minSplitPoint_v122.point, colorRed);
							AAS_DrawArea((idAASFile*)this, v51);
						}
#endif
						v87 = fabs(v96.x);
						if (v87 < idMath::FLT_EPSILON)
							v96.x = 0.0f;
						v88 = fabs(v96.y);
						if (v88 < idMath::FLT_EPSILON)
							v96.y = 0.0f;
						v89 = fabs(v96.z);
						v59 = v96.z;
						if (v89 < idMath::FLT_EPSILON)
						{
							v59 = 0.0f;
							v96.z = v59;
						}
#if 0
						ID_IF_DBG()
						{
							session->rw->DrawText(va("X: %d", _v112->toAreaNum), minSplitPoint_v122.point, 1.0f,
							                      colorBlue, mat3_identity);
							session->rw->DebugCircle(colorBlue, minSplitPoint_v122.point, idVec3(0, 0, 1), 35.0f, 8);
							session->rw->DrawText(va("Y: %d", _v112->toAreaNum), maxSplitPoint_v117.point, 1.0f,
							                      colorBlue, mat3_identity);
							session->rw->DebugCircle(colorBlue, maxSplitPoint_v117.point, idVec3(0, 0, 1), 35.0f, 8);
							session->rw->DebugArrow(colorBlue, minSplitPoint_v122.point, minSplitPoint_v122.point + v96,
							                        5);
						}
#endif
						v90 = this->settings.gravityDir * v96;
						v113 = v90 * this->settings.gravityDir;
						v91 = v113.LengthSqr();
						v60 = v91;
						v92 = this->settings.maxStepHeight * this->settings.maxStepHeight;
						if (v92 >= v60)
						{
							v97 = v96 - v113;
							v93 = v97.LengthSqr();
							if (v93 <= 0.04f)
								break;
						}
					}
				}
			}
			_v112 = _v112->next;
			v48 = _v112;
			if (_v112 == NULL)
			{
				v45 = v116;
				v7 = &trace;
				goto LABEL_150;
			}
		}
		v61 = v110;
		for (m = v110; m < this->searchAreaList.Num(); ++m)
		{
			v63 = this->searchAreaList[m];
			areas[v63].flags &= ~AAS_AREA_FLOOD_VISITED/* 0x8000u */;
		}
		v68 = _v112;
		searchAreaList.SetNum(v61);
		v7 = &trace;
		v116 = v68->toAreaNum;
	}
	v7->endpos = end;
	v65 = 1.0f;
LABEL_159:
	v7->lastAreaNum = v45;
	v7->fraction = v65;
	for (ii = 0; ii < this->searchAreaList.Num(); ++ii)
	{
		v70 = this->searchAreaList[ii];
		areas[v70].flags &= ~AAS_AREA_FLOOD_VISITED/* 0x8000u */;
	}
	searchAreaList.Clear();
	return true;
}

/*
============
idAASFileLocal::SplitFloorWinding
 * Check edge vertexes of area in plane side and distance
 * @param areaNum_a2 areaNum, positive
 * @param plane_a3 split plane
 * @param retDists_a4 save all distances of edge vertex to plane, size == area's numEdges + 1, first == last, like a closed winding
 * @param retSides_a5 save all sides of edge vertex to plane, size == area's numEdges + 1, first == last, like a closed winding
 * @return true if plane cross edge vertexes, false if all edge vertexes in plane front side
============
*/
bool idAASFileLocal::SplitFloorWinding(int areaNum_a2, const idPlane* plane_a3, float retDists_a4[], int retSides_a5[]) const
{
	const aasArea_t* area_list;
	int i_v6;
	bool noEdges_v7;
	const aasArea_t* area_v8;
	float* distPtr_v9;
	int edgeIndex_v10;
	const idVec3* vertex_v11;
	int sign_v12;
	int front_v14;
	int back_v15;
	const aasArea_t* area_v16;
	int* signPtr = retSides_a5;

	area_list = this->areas.Ptr();
	i_v6 = 0;
	noEdges_v7 = area_list[areaNum_a2].numEdges <= 0;
	area_v8 = &area_list[areaNum_a2];
	back_v15 = 1;
	front_v14 = 0;
	area_v16 = area_v8;
	if (!noEdges_v7)
	{
		distPtr_v9 = retDists_a4;
		do
		{
			edgeIndex_v10 = this->edgeIndex[i_v6 + area_v8->firstEdge];
			vertex_v11 = &this->vertices[this->edges[abs(edgeIndex_v10)].vertexNum[(unsigned int)edgeIndex_v10 >> 31]];
			/*
			if(ID_IS_DBG() && aaa)
			{
			session->rw->DrawText( va( "%d / %d", i_v6 ,area_v16->numEdges), *vertex_v11+idVec3(0,0,50+i_v6*5), 1.0f, colorBlue, mat3_identity );
			session->rw->DebugCircle(colorBlue, *vertex_v11, idVec3( 0, 0, 1 ), 25.0f, 3);
			}
			*/
			++i_v6;
			float d = plane_a3->Distance(*vertex_v11);
			*distPtr_v9++ = d; // retDists_a4 += 1
			sign_v12 = FLOATSIGNBITSET(d);
			// check distance less than 0; sign_v12 = 1 if distance < 0 // *((_DWORD *)distPtr_v9 - 1) >> 31;
			back_v15 &= sign_v12;
			front_v14 |= sign_v12;
			*signPtr++ = sign_v12;
			// retSides_a5 += 1 // *(_DWORD *)((char *)distPtr_v9 + ((char *)retSides_a5 - (char *)retDists_a4) - 4) = sign_v12; // retSides_a5 - retDists_a4 == align16( (1+numEdges) * sizeof(int/float) )
			area_v8 = area_v16;
			//Sys_Printf("EEE %d/%d: %f %d | %d %d\n", i_v6-1,area_v8->numEdges, d,sign_v12,back_v15, front_v14);
		}
		while (i_v6 < area_v16->numEdges);
	}
	retDists_a4[area_v8->numEdges] = *retDists_a4;
	retSides_a5[area_v8->numEdges] = *retSides_a5;
	return back_v15 != front_v14; // all distance >= 0; v14 == v15 if has distance < 0
}

/*
============
idAASFileLocal::GetFloorEdgeSplitPoints
 * Calc split edge of area points of min/max distance to ref plane,
 * @param minSplitPoint_a2 save split point of min distance of ref plane
 * @param maxSplitPoint_a3 save split point of max distance of ref plane
 * @param areaNum_a4 areaNum, positive
 * @param toLeftSplitPlane_a5 split plane, direction is to left
 * @param toForwardRefPlane_a6 ref plane, direction is to forward
 * @return is split
============
*/
bool idAASFileLocal::GetFloorEdgeSplitPoints(idAASFileLocal::floorEdgeSplitPoint_t* minSplitPoint_a2, idAASFileLocal::floorEdgeSplitPoint_t* maxSplitPoint_a3, int areaNum_a4, const idPlane* toLeftSplitPlane_a5, const idPlane* toForwardRefPlane_a6) const
{
	float* dists_v8;
	int* sides_v9;
	bool result;
	int i_v11;
	float* distPtr_v12;
	//int v13;
	int edgeIndex_v14;
	const aasEdge_t* edge_v15;
	const idVec3* vertex0_v16;
	const idVec3* vertex1_v17;
	//int v21;
	//byte v22[8];
	idVec3 projVertex_v23;
	idVec3 projLine_v26;
	idVec3 edgeLine_v29;
	//int i;
	int* sides_v33;
	const aasArea_t* area_v34;
	int size_v35;
	int i_v36;
	float* distPtr_v37;
	float frac_v38;
	float dist_v39;

	idVec3 projVertex_v19;
	minSplitPoint_a2->point = vec3_origin;
	minSplitPoint_a2->edgeIndex = 0;
	minSplitPoint_a2->distance = idMath::INFINITY;
	maxSplitPoint_a3->point = vec3_origin;
	maxSplitPoint_a3->edgeIndex = 0;
	maxSplitPoint_a3->distance = -idMath::INFINITY;
	area_v34 = &this->areas[areaNum_a4];
	size_v35 = 4 * (area_v34->numEdges + 1) + 15;
	dists_v8 = (float*)alloca(size_v35);
	sides_v9 = (int*)alloca(size_v35);
	//sides_v33 = &v21;
	result = idAASFileLocal::SplitFloorWinding(areaNum_a4, toLeftSplitPlane_a5, dists_v8, sides_v9);
	// (this, areaNum_a4, toLeftSplitPlane_a5, (float *)&v21, &v21);
	if (result) // all distance >= 0
	{
		i_v11 = 0;
		i_v36 = 0;
		if (area_v34->numEdges > 0)
		{
			// v13 = (char *)sides_v33 - (char *)&v21; // == sizeof(dists_v8/sides_v9)
			distPtr_v12 = dists_v8 + 1; // float* distance
			sides_v33 = sides_v9; // int* flags
			distPtr_v37 = dists_v8 + 1; // float* distance
			int* sidePtr_v12 = sides_v9 + 1; // int* flags
			idVec3 last = vec3_origin;
			for (; ; /*i = (char *)sides_v33 - (char *)&v21; ; v13 = i*/)
			{
				//if ( sides_v33[i_v11] != *sidePtr_v12 /* *(_DWORD *)((char *)&distPtr_v12->ToFloatPtr()[0] + v13)*/ )
				if (sides_v33[i_v11] != *sidePtr_v12) // cross this edge
				{
					edgeIndex_v14 = this->edgeIndex[i_v11 + area_v34->firstEdge];
					edge_v15 = &this->edges[abs(edgeIndex_v14)];
					vertex0_v16 = &this->vertices[edge_v15->vertexNum[(unsigned int)edgeIndex_v14 >> 31]];
					vertex1_v17 = &this->vertices[edge_v15->vertexNum[edgeIndex_v14 >= 0]];
					distPtr_v12 = distPtr_v37;
					edgeLine_v29 = *vertex1_v17 - *vertex0_v16;
					float lastDist_v37 = *(distPtr_v37 - 1);
					frac_v38 = lastDist_v37 / (lastDist_v37 - *distPtr_v37);
					projLine_v26 = edgeLine_v29 * frac_v38;
					projVertex_v23 = *vertex0_v16 + projLine_v26;
					/*
					  if(ID_IS_DBG() && aaa)
					  {
					  session->rw->DrawText( va( "%d", i_v11 ), projVertex_v23, 1.0f, colorGreen, mat3_identity );
					  session->rw->DebugCircle(colorGreen, projVertex_v23, idVec3( 0, 0, 1 ), 25.0f, 4);
					  if(!last.IsZero())
					  session->rw->DebugArrow( colorGreen, last, projVertex_v23, 5 );
					  session->rw->DebugLine( colorGreen, *vertex0_v16, *vertex1_v17 );
					  last = projVertex_v23;
					  aaa=0;
					  }
					ID_IF_DBG()
					AAS_DrawPoint(projVertex_v23, colorBlack);
					  */
					projVertex_v19 = projVertex_v23;
					dist_v39 = toForwardRefPlane_a6->Distance(projVertex_v23);
					if (minSplitPoint_a2->distance > dist_v39)
					{
						minSplitPoint_a2->distance = dist_v39;
						minSplitPoint_a2->edgeIndex = edgeIndex_v14;
						minSplitPoint_a2->point = projVertex_v19;
					}
					i_v11 = i_v36;
					if (maxSplitPoint_a3->distance < dist_v39)
					{
						maxSplitPoint_a3->distance = dist_v39;
						maxSplitPoint_a3->edgeIndex = edgeIndex_v14;
						maxSplitPoint_a3->point = projVertex_v19;
					}
				}
				++i_v11;
				distPtr_v12++;
				sidePtr_v12++;
				i_v36 = i_v11;
				distPtr_v37 = distPtr_v12;
				if (i_v11 >= area_v34->numEdges)
					break;
			}
		}
		return true;
	}
	return result;
}

/*
============
idAASFileLocal::GetFloorDistance
 *
 * @param areaNum_a2 areaNum, positive
 * @param plane_a3 plane
 * @param origin_a4 origin
 * @param minDist_a5 min distance
 * @param maxDist_a6 max distance
 * @return
============
*/
float idAASFileLocal::GetFloorDistance(int areaNum_a2, const idPlane* plane_a3, const idVec3* origin_a4, float minDist_a5, float maxDist_a6) const
{
	const aasArea_t* area_v6;
	float distAbs_v8;
	float result;
	const aasEdge_t* edge_list;
	const idVec3* vertexes_v11;
	const int* edgeIndex_v12;
	unsigned int edgeIndexAbs_v13;
	int vertexIndex_v14;
	const idVec3* vertex1_p_x;
	const idVec3* vertex0_v16;
	float v17;
	float v18;
	float v19;
	float v20;
	float v21;
	idVec3 v22; // v22 v23 v24
	idVec3 edgeLine_v25; // v25 v26 v27
	idVec3 v28; // v28 v29 v30
	idVec3 v31; // v31 v32 v33
	idVec3 v34(0.0f, 0.0f, 0.0f); // v34 v35 v36
	float dist_v37;
	float distAbs_v38;
	float dot_v39;
	float edgeLineLengthSqr_v40;
	float v41;
	float v42;
	float masDistSqr_v43;
	float v44;
	float v45;
	float dist_v46;
	float maxDist_v47;
	int numEdges;

	idVec3 _v18; // v18 v17 v19

	area_v6 = &this->areas[areaNum_a2];
	dist_v37 = plane_a3->Distance(*origin_a4);
	distAbs_v38 = fabs(dist_v37);
	distAbs_v8 = distAbs_v38;
	dot_v39 = this->settings.invGravityDir * plane_a3->Normal();
	dist_v46 = distAbs_v8 / dot_v39;
	result = dist_v46;
	if (minDist_a5 <= dist_v46)
	{
		maxDist_v47 = idMath::INFINITY;
		if (area_v6->numEdges > 0)
		{
			edge_list = this->edges.Ptr();
			vertexes_v11 = this->vertices.Ptr();
			edgeIndex_v12 = &this->edgeIndex[area_v6->firstEdge];
			numEdges = area_v6->numEdges;
			do
			{
				edgeIndexAbs_v13 = abs(*edgeIndex_v12);
				vertexIndex_v14 = edge_list[edgeIndexAbs_v13].vertexNum[0];
				vertex1_p_x = &vertexes_v11[edge_list[edgeIndexAbs_v13].vertexNum[1]];
				vertex0_v16 = &vertexes_v11[vertexIndex_v14];
				edgeLine_v25 = *vertex1_p_x - *vertex0_v16;
				_v18 = edgeLine_v25;
				edgeLineLengthSqr_v40 = edgeLine_v25.LengthSqr();
				if (edgeLineLengthSqr_v40 >= 0.1f)
				{
					v22 = *origin_a4 - *vertex0_v16;
					v21 = v22 * _v18;
					v41 = v21 / edgeLineLengthSqr_v40;
					if (v41 >= 0.0f)
					{
						v20 = v41;
						if (v41 > 1.0f)
							v20 = 1.0f;
					}
					else
					{
						v20 = 0.0f;
					}
					v28 = _v18 * v20;
					v31 = v22 - v28;
					v42 = v31.LengthSqr();
					if (maxDist_v47 > v42)
					{
						maxDist_v47 = v31.LengthSqr();
						v34 = v22 - v28;
					}
				}
				++edgeIndex_v12;
				--numEdges;
			}
			while (numEdges);
			result = dist_v46;
		}
		masDistSqr_v43 = maxDist_a6 * maxDist_a6;
		if (masDistSqr_v43 <= maxDist_v47)
			return dist_v46;
		v44 = this->settings.invGravityDir * v34;
		v45 = fabs(v44);
		if (v45 >= result)
			return dist_v46;
		else
			return v45;
	}
	return result;
}

/*
============
idAASFileLocal::BoundsBestReachableAreaNum
 * Find best reachable area num of bounds
 * @param bounds_a2 bounds
 * @param origin_a3 origin
 * @param nodeNum_a4 nodeNum, is areaNum if negative
 * @param plane_a5 node's plane, allow NULL
 * @param bestReachableArea_a6 save best reachable area
============
*/
void idAASFileLocal::BoundsBestReachableAreaNum(const idBounds* bounds_a2, const idVec3* origin_a3, int nodeNum_a4, const idPlane* plane_a5, idAASFileLocal::bestReachableArea_t* bestReachableArea_a6) const
{
	int nodeNum_v6;
	bool isAreaNum_v7;
	const aasNode_t* node_v10;
	int side_v11;
	const aasArea_t* area_v12;
	int areaNum_v13;
	float FloorDistance;

	nodeNum_v6 = nodeNum_a4;
	isAreaNum_v7 = nodeNum_a4 < 0;
	if (!nodeNum_a4)
		return;
	while (!isAreaNum_v7)
	{
		node_v10 = &this->nodes[nodeNum_v6];
		side_v11 = bounds_a2->PlaneSide(this->planeList[node_v10->planeNum], 0.1f);
		if (!side_v11)
			goto LABEL_7;
		if (side_v11 != SIDE_BACK/* 1 */)
		{
			idAASFileLocal::BoundsBestReachableAreaNum(bounds_a2, origin_a3, node_v10->children[1], plane_a5, bestReachableArea_a6);
LABEL_7:
			if ((node_v10->flags & AAS_NODE_FLAG_FLOOR_PLANE/* 1 */) != 0)
				plane_a5 = &this->planeList[node_v10->planeNum];
			nodeNum_v6 = node_v10->children[0];
			goto LABEL_10;
		}
		nodeNum_v6 = node_v10->children[1];
LABEL_10:
		isAreaNum_v7 = nodeNum_v6 < 0;
		if (!nodeNum_v6)
			return;
	}
	area_v12 = &this->areas[-nodeNum_v6];
	if ((area_v12->flags & bestReachableArea_a6->areaFlags) != 0 && (area_v12->travelFlags & bestReachableArea_a6->excludeTravelFlags) == 0)
	{
		areaNum_v13 = -nodeNum_v6;
		FloorDistance = idAASFileLocal::GetFloorDistance(areaNum_v13, plane_a5, origin_a3, bestReachableArea_a6->v0, bestReachableArea_a6->v1);
		if (bestReachableArea_a6->distance1 - bestReachableArea_a6->v0 > FloorDistance && bestReachableArea_a6->distance2 > FloorDistance)
		{
			bestReachableArea_a6->distance2 = FloorDistance;
			bestReachableArea_a6->areaNum2 = areaNum_v13;
		}
	}
}

/*
============
idAASFileLocal::PointBestReachableAreaNum
 * Find best reachable area num of point
 * @param origin_a2 origin
 * @param bestReachableArea_a3 save best reachable area
============
*/
void idAASFileLocal::PointBestReachableAreaNum(const idVec3* origin_a2, idAASFileLocal::bestReachableArea_t* bestReachableArea_a3) const
{
	const idPlane* plane_list;
	int nodeNum_v5;
	const aasNode_t* node_v6;
	const idPlane* plane_p_a;
	const aasArea_t* area_v8;
	int areaNum_v9;
	const idPlane* plane_v10;
	float dist_v11;

	plane_list = this->planeList.Ptr();
	plane_v10 = NULL;
	nodeNum_v5 = 1;
	while (1)
	{
		node_v6 = &this->nodes[nodeNum_v5];
		plane_p_a = &plane_list[node_v6->planeNum];
		dist_v11 = plane_p_a->Distance(*origin_a2);
		if (dist_v11 <= 0.0f)
		{
			nodeNum_v5 = node_v6->children[1];
		}
		else
		{
			nodeNum_v5 = node_v6->children[0];
			if ((node_v6->flags & AAS_NODE_FLAG_FLOOR_PLANE/* 1 */) != 0)
				plane_v10 = &plane_list[node_v6->planeNum];
		}
		if (nodeNum_v5 < 0)
			break;
		if (!nodeNum_v5)
			return;
	}
	area_v8 = &this->areas[-nodeNum_v5]; // negative is area num
	if ((area_v8->flags & bestReachableArea_a3->areaFlags) != 0 && (area_v8->travelFlags & bestReachableArea_a3->excludeTravelFlags) == 0)
	{
		areaNum_v9 = -nodeNum_v5;
		bestReachableArea_a3->distance1 = idAASFileLocal::GetFloorDistance(
			-nodeNum_v5, plane_v10, origin_a2, bestReachableArea_a3->v0, bestReachableArea_a3->v1);
		bestReachableArea_a3->areaNum1 = areaNum_v9;
	}
}

/*
============
idAASFileLocal::PointBestReachableAreaNum
 *
 * @param a2
 * @param a3
 * @param a4
 * @return
============
*/
bool idAASFileLocal::Trace(aasTrace_t* trace_a2, const idVec3* start_a3, const idVec3* end_a4) const
{
	const idVec3* v4; // edx
	const idVec3* v5; // ecx
	aasTraceStack_t* v6;
	//float *v6; // esi
	double z; // st7
	int v8; // ebx
	const aasArea_t* v9; // eax
	int numAreas; // eax
	int v11; // ebx
	bool v12; // cc
	int* areas; // ecx
	idVec3* points; // ecx
	idVec3 p_x; // eax
	int v16; // ebx
	const aasNode_t* v17; // ebx
	const idPlane* v18; // eax
	double v19; // st6
	double v20; // st5
	double v21; // st7
	double v22; // st4
	double v23; // st3
	double v24; // st2
	bool v25; // cf
	double v26; // st1
	double v27; // st2
	double v28; // st2
	double v29; // st1
	aasTraceStack_t* v30; // esi
	int v31; // eax
	bool result; // al
	double v33; // st7
	double v34; // st6
	double v35; // st5
	double v36; // st7
	int v37; // esi
	float v38; // [esp+Ch] [ebp-108Ch]
	float v39; // [esp+Ch] [ebp-108Ch]
	float v40; // [esp+Ch] [ebp-108Ch]
	float v41; // [esp+Ch] [ebp-108Ch]
	float v42; // [esp+Ch] [ebp-108Ch]
	float v43; // [esp+Ch] [ebp-108Ch]
	float v44; // [esp+Ch] [ebp-108Ch]
	float v45; // [esp+Ch] [ebp-108Ch]
	float v46; // [esp+Ch] [ebp-108Ch]
	float v47; // [esp+Ch] [ebp-108Ch]
	float v48; // [esp+10h] [ebp-1088h]
	float v49; // [esp+10h] [ebp-1088h]
	float v50; // [esp+10h] [ebp-1088h]
	float v51; // [esp+10h] [ebp-1088h]
	int v52; // [esp+10h] [ebp-1088h]
	float v53; // [esp+10h] [ebp-1088h]
	idVec3 v54; // v54 v57 v60
	//float v54; // [esp+14h] [ebp-1084h]
	idVec3 v55; // v55 v58 v61
	//float v55; // [esp+14h] [ebp-1084h]
	idVec3 v56; // v56 v59 v62
	//float v56; // [esp+14h] [ebp-1084h]
	//float v57; // [esp+18h] [ebp-1080h]
	//float v58; // [esp+18h] [ebp-1080h]
	//float v59; // [esp+18h] [ebp-1080h]
	//float v60; // [esp+1Ch] [ebp-107Ch]
	//float v61; // [esp+1Ch] [ebp-107Ch]
	//float v62; // [esp+1Ch] [ebp-107Ch]
	idVec3 x; // [esp+20h] [ebp-1078h] // x y 67
	//float x; // [esp+20h] [ebp-1078h]
	idVec3 v64; // [esp+20h] [ebp-1078h] // 64 66 68
	//float v64; // [esp+20h] [ebp-1078h]
	//float y; // [esp+24h] [ebp-1074h]
	//float v66; // [esp+24h] [ebp-1074h]
	//float v67; // [esp+28h] [ebp-1070h]
	//float v68; // [esp+28h] [ebp-1070h]
	float v70; // [esp+34h] [ebp-1064h]
	float v71; // [esp+38h] [ebp-1060h]
	idVec3 v72; // v72 v73 v74
	//float v72; // [esp+54h] [ebp-1044h]
	//float v73; // [esp+58h] [ebp-1040h]
	//float v74; // [esp+5Ch] [ebp-103Ch]
	idVec3 v75; // v75 v76 v77
	//float v75; // [esp+60h] [ebp-1038h]
	//float v76; // [esp+64h] [ebp-1034h]
	//float v77; // [esp+68h] [ebp-1030h]
	idVec3 v78; // v78 v79 v80
	//float v78; // [esp+6Ch] [ebp-102Ch]
	//float v79; // [esp+70h] [ebp-1028h]
	//float v80; // [esp+74h] [ebp-1024h]
	double v81; // [esp+78h] [ebp-1020h]
	double v82; // [esp+80h] [ebp-1018h]
	idVec3 v83; // v83 v84 v85
	//float v83; // [esp+8Ch] [ebp-100Ch]
	//float v84; // [esp+90h] [ebp-1008h]
	//float v85; // [esp+94h] [ebp-1004h]
	aasTraceStack_t v86[MAX_AAS_TREE_DEPTH]; // [esp+98h] [ebp-1000h] BYREF
	//float v86[1024]; // [esp+98h] [ebp-1000h] BYREF
	aasTraceStack_t* vars0 = &v86[MAX_AAS_TREE_DEPTH]; // [esp+1098h] [ebp+0h] BYREF

	v6 = &v86[0];
	idVec3 _v23; // v23 v70 v71
	idVec3 _v19; // v19 v20 v21

	v4 = start_a3;
	trace_a2->numAreas = 0;
	trace_a2->lastAreaNum = 0;
	trace_a2->blockingAreaNum = 0;
	v6->start = *start_a3;
	v6->end = *end_a4;
	v6->planeNum = 0;
	v6->nodeNum = 1;
	v5 = end_a4;
	v6 = v86;
	while (1)
	{
		v8 = v6->nodeNum;
		if (v8 >= 0)
		{
			if (v8)
			{
				v17 = &this->nodes[v8];
				v18 = &this->planeList[v17->planeNum];
				_v19 = v6->start;
				v40 = v18->Distance(_v19);
				v22 = v40;
				_v23 = v6->end;
				v41 = v18->Distance(v6->end);
				v24 = v41;
				if (v22 < -ON_EPSILON || v24 < -ON_EPSILON)
				{
					if (v22 >= ON_EPSILON || v24 >= ON_EPSILON)
					{
						v52 = v6->planeNum;
						if (v22 >= 0.0)
							v26 = v22 - TRACEPLANE_EPSILON;
						else
							v26 = v22 + TRACEPLANE_EPSILON;
						v27 = v26 / (v22 - v24);
						if (v27 >= 0.0)
						{
							if (v27 <= 1.0)
							{
								v29 = v27;
								v28 = 0.0;
							}
							else
							{
								v28 = 0.0;
								v29 = 0.999f;
							}
						}
						else
						{
							v28 = 0.0;
							v29 = 0.001f;
						}
						v72 = _v23 - _v19;
						v42 = v29;
						v78 = v72 * v42;
						v54 = v78 + _v19;
						v6->planeNum = v17->planeNum;
						v6->start = v54;
						v30 = v6 + 1;
						v6->nodeNum = v17->children[v28 <= v22];
						if (v30 >= vars0)
						{
						LABEL_50:
							common->Warning("idAASFileLocal::Trace: stack overflow");
							return false;
						}
						v31 = v17->children[v28 > v22];
						v6->start = _v19;
						v6->end = v54;
						v6->planeNum = v52;
						v6->nodeNum = v31;
						v6 = v6 + 1;
						v25 = v6 < vars0;
					}
					else
					{
						v6->nodeNum = v17->children[1];
						v6 = v6 + 1;
						v25 = v6 < vars0;
					}
					if (!v25)
						goto LABEL_50;
				}
				else
				{
					v6->nodeNum = v17->children[0];
					v6 = v6 + 1;
					if (v6 >= vars0)
						goto LABEL_50;
				}
			}
			else
			{
				if (trace_a2->lastAreaNum)
				{
					v75 = *v5 - *v4;
					x = v75;
					v83 = v6->start - *v4;
					v49 = v83.Length();
					v38 = v49;
					v51 = v75.Length();
					v4 = start_a3;
					trace_a2->fraction = v38 / v51;
				}
				else
				{
					trace_a2->fraction = 0.0;
					x = vec3_origin;
				}
				v16 = v6->planeNum;
				trace_a2->endpos = v6->start;
				trace_a2->blockingAreaNum = 0;
				trace_a2->planeNum = v16;
				v39 = x * this->planeList[v16].Normal();
				if (v39 > 0.0)
					trace_a2->planeNum = v16 ^ 1;
				if (trace_a2->lastAreaNum || !trace_a2->getOutOfSolid)
					return true;
			}
		}
		else
		{
			v9 = &this->areas[-v8];
			if ((v9->flags & trace_a2->flags) != 0 || (v9->travelFlags & trace_a2->travelFlags) != 0)
			{
				if (trace_a2->lastAreaNum)
				{
					v55 = *end_a4 - *v4;
					v64 = v55;
					v56 = v6->start - *v4;
					v44 = v6->start.Length();
					v53 = v44;
					v46 = v55.Length();
					trace_a2->fraction = v53 / v46;
				}
				else
				{
					trace_a2->fraction = 0.0;
					v64 = vec3_origin;
				}
				trace_a2->endpos = v6->start;
				v37 = v6->planeNum;
				trace_a2->blockingAreaNum = -v8;
				trace_a2->planeNum = v37;
				v47 = v64 * this->planeList[v37].Normal();
				if (v47 > 0.0)
					trace_a2->planeNum = v37 ^ 1;
				return true;
			}
			numAreas = trace_a2->numAreas;
			v11 = -v8;
			v12 = numAreas < trace_a2->maxAreas;
			trace_a2->lastAreaNum = v11;
			if (v12)
			{
				areas = trace_a2->areas;
				if (areas)
					areas[numAreas] = v11;
				points = trace_a2->points;
				if (points)
				{
					p_x = v6->start;
					points[numAreas] = p_x;
				}
				++trace_a2->numAreas;
			}
		}
		v6 = v6 - 1;
		if (v6 < v86)
			break;
		v5 = end_a4;
	}
	if (trace_a2->lastAreaNum)
	{
		trace_a2->fraction = 1.0;
		trace_a2->endpos = *end_a4;
		result = false;
	}
	else
	{
		result = false;
		trace_a2->fraction = 0.0;
		trace_a2->endpos = *v4;
	}
	trace_a2->planeNum = 0;
	return result;
}

#endif
