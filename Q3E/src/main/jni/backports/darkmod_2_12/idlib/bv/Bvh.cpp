/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include "Bvh.h"


struct idBvhCreator::Node {
	int begElem;
	int endElem;
	int sons[2];
	idBounds bounds;
	idCircCone cone;
	int axis;
};

// clustering construction algorithm breaks set of elements into this number of cluster using K-means
static const int K_MEANS_ARITY = 8;

// bounding cones for elements directions are expanded by this angle (in radians) to ensure bounds are conservative
static const float DIRECTION_CONE_EXPAND = 1e-3f;


float bvhNode_t::quantizedSinLut[128];
void bvhNode_t::Init() {
	static bool initialized = false;
	if (initialized)
		return;
	for (int i = 0; i < 128; i++)
		quantizedSinLut[i] = sinf(i / 255.0f * idMath::PI);
}


idBvhCreator::idBvhCreator() {
	bvhNode_t::Init();
	SetLeafSize();
	SetAlgorithm();
}
idBvhCreator::~idBvhCreator() {
}

void idBvhCreator::SetAlgorithm(Algorithm algo) {
	algorithm = algo;
}

void idBvhCreator::SetLeafSize(int leafSize) {
	desiredLeafSize = leafSize;
}

void idBvhCreator::Build(int elemsNum, bvhElement_t *elements) {
	this->elemsNum = elemsNum;
	this->elements = elements;

	desiredLeafSize = idMath::Imax(desiredLeafSize, K_MEANS_ARITY);
	rnd.SetSeed(12345);

	nodes.Clear();
	nodes.Reserve(elemsNum / desiredLeafSize + 1);

	// build intermediate representation
	if (algorithm == aMedianSplit)
		BuildBvhByAxisMedian();
	else if (algorithm == aKmeansClustering)
		BuildBvhByClustering();
	else {
		assert(false);
		BuildBvhByAxisMedian();
	}

	// having full BVH tree structure,
	// compute bounding cones for directions in each node
	ComputeBoundingCones();

	// build final representation
	CompressBvh();
}

void idBvhCreator::BuildBvhByAxisMedian() {
	// create root node
	nodes.SetNum(1, false);
	nodes[0].begElem = 0;
	nodes[0].endElem = elemsNum;
	nodes[0].sons[0] = -1;
	nodes[0].sons[1] = -1;
	nodes[0].bounds.Clear();
	nodes[0].axis = 0;
	for (int i = 0; i < elemsNum; i++)
		nodes[0].bounds.AddBounds(elements[i].bounds);

	// go through all nodes in BFS order
	for (int idx = 0; idx < nodes.Num(); idx++)
		SplitNodeByAxisMedian(idx);
}

bool idBvhCreator::SplitNodeByAxisMedian(int idx) {
	int axis = nodes[idx].axis;
	int beg = nodes[idx].begElem;
	int end = nodes[idx].endElem;
	if (end - beg <= desiredLeafSize) {
		// small enough to be leaf
		return false;
	}
	int med = (beg + end) / 2;

	// find median
	std::nth_element(elements + beg, elements + med, elements + end, [axis](const bvhElement_t &a, const bvhElement_t &b) {
		return a.center[axis] < b.center[axis];
	});
	float split = elements[med].center[axis];
	// partition by it
	std::partition(elements + beg, elements + end, [axis,split](const bvhElement_t &a) {
		return a.center[axis] < split;
	});

	// create two son nodes
	Node lnode, rnode;
	lnode.axis = rnode.axis = (axis + 1) % 3;	// rotate splitting axis
	lnode.begElem = beg;
	lnode.endElem = rnode.begElem = med;
	rnode.endElem = end;
	lnode.bounds.Clear();
	rnode.bounds.Clear();
	for (int i = beg; i < med; i++)
		lnode.bounds.AddBounds(elements[i].bounds);
	for (int i = med; i < end; i++)
		rnode.bounds.AddBounds(elements[i].bounds);
	lnode.sons[0] = lnode.sons[1] = -1;
	rnode.sons[0] = rnode.sons[1] = -1;
	int ls = nodes.AddGrow(lnode);
	int rs = nodes.AddGrow(rnode);
	nodes[idx].sons[0] = ls;
	nodes[idx].sons[1] = rs;
	return true;
}

void idBvhCreator::BuildBvhByClustering() {
	static const int ARITY = K_MEANS_ARITY;
	coloring.SetNum(elemsNum);

	// create root node
	nodes.SetNum(1, false);
	nodes[0].begElem = 0;
	nodes[0].endElem = elemsNum;
	nodes[0].sons[0] = -1;
	nodes[0].sons[1] = -1;
	nodes[0].bounds.Clear();
	for (int i = 0; i < elemsNum; i++)
		nodes[0].bounds.AddBounds(elements[i].bounds);

	tempElems.SetNum(elemsNum, false);

	// go through all nodes in BFS order
	for (int idx = 0; idx < nodes.Num(); idx++) {
		if (nodes[idx].sons[0] >= 0) {
			// already intermediate node
			continue;
		}

		int beg = nodes[idx].begElem;
		int end = nodes[idx].endElem;
		if (end - beg <= desiredLeafSize) {
			// small enough to be leaf
			continue;
		}

		// apply K-means clustering to elements of this node
		bool clusteringSuccess = KMeansClustering(beg, end, coloring.Ptr());
		if (!clusteringSuccess) {
			// clustering failed to work due to many indistinguishable elements
			// fallback to median split for this node
			nodes[idx].axis = 0;
			int startSubtree = nodes.Num();
			SplitNodeByAxisMedian(idx);
			// and this subtree fully recursively right now
			for (int j = startSubtree; j < nodes.Num(); j++)
				SplitNodeByAxisMedian(j);
			continue;
		}
		
		// compute bounding box for each subcluster
		idBounds subclBounds[ARITY * 2];
		for (int i = 0; i < ARITY; i++)
			subclBounds[i].Clear();
		for (int i = beg; i < end; i++)
			subclBounds[coloring[i]].AddBounds(elements[i].bounds);

		// apply agglomerative clustering on subclusters to build binary tree
		int leftSons[ARITY * 2], rightSons[ARITY * 2];
		AgglomerativeClustering(ARITY, subclBounds, leftSons, rightSons);

		// determine order of subclusters as they go in binary tree leaves
		int order[ARITY], ok = 0;
		int root = ARITY * 2 - 2;
		GetLeavesOrder(leftSons, rightSons, root, order, ok);
		assert(ok == ARITY);
		int perm[ARITY];
		for (int i = 0; i < ARITY; i++)
			perm[order[i]] = i;

		// renumber subclusters accordingly
		for (int i = beg; i < end; i++)
			coloring[i] = perm[coloring[i]];
		for (int i = ARITY; i <= root; i++) {
			if (leftSons[i] < ARITY)
				leftSons[i] = perm[leftSons[i]];
			if (rightSons[i] < ARITY)
				rightSons[i] = perm[rightSons[i]];
		}

		// reorder elements to turn any node of binary tree into subsegment of elements
		int pos[ARITY + 2] = {0};
		for (int i = beg; i < end; i++)
			pos[coloring[i] + 2]++;
		pos[0] = pos[1] = beg;
		for (int i = 2; i < ARITY + 2; i++)
			pos[i] += pos[i - 1];
		for (int i = beg; i < end; i++) {
			int dst = pos[coloring[i] + 1]++;
			tempElems[dst] = elements[i];
		}
		memcpy(elements + beg, tempElems.Ptr() + beg, (end - beg) * sizeof(elements[0]));
		assert(pos[ARITY + 1] == end);

		// compute bounds and normal masks for all nodes
		idBounds bounds[ARITY * 2];
		for (int i = 0; i < ARITY; i++)
			bounds[i] = subclBounds[order[i]];
		for (int i = ARITY; i <= root; i++) {
			bounds[i] = bounds[leftSons[i]];
			bounds[i].AddBounds(bounds[rightSons[i]]);
		}

		// compute interval of elements of all binary tree nodes
		int interval[ARITY * 2][2];
		for (int i = 0; i < ARITY; i++) {
			interval[i][0] = pos[i];
			interval[i][1] = pos[i+1];
			assert(interval[i][0] < interval[i][1]);
		}
		for (int i = ARITY; i < ARITY * 2 - 1; i++) {
			interval[i][0] = interval[leftSons[i]][0];
			interval[i][1] = interval[rightSons[i]][1];
			assert(interval[i][0] < interval[i][1]);
		}

		// create nodes in the BVH tree
		int nodeMap[ARITY * 2];
		memset(nodeMap, -1, sizeof(nodeMap));
		nodeMap[root] = idx;
		for (int i = root; i >= 0; i--) {
			if (nodeMap[i] < 0)
				continue;	// removed node

			// update bounding info
			nodes[nodeMap[i]].bounds = bounds[i];

			// don't split node which is too small
			// instead, merge subtree of binary tree, leaving single node in BVH
			if (interval[i][1] - interval[i][0] <= desiredLeafSize)
				continue;
			// no sons here: skip the rest
			if (leftSons[i] < 0)
				continue;

			// add two sons in BVH tree
			Node lnode, rnode;
			lnode.begElem = interval[leftSons[i]][0];
			lnode.endElem = interval[leftSons[i]][1];
			lnode.sons[0] = lnode.sons[1] = -1;
			rnode.begElem = interval[rightSons[i]][0];
			rnode.endElem = interval[rightSons[i]][1];
			rnode.sons[0] = rnode.sons[1] = -1;
			int ls = nodes.AddGrow(lnode);
			int rs = nodes.AddGrow(rnode);
			assert(lnode.endElem > lnode.begElem);
			assert(rnode.endElem > rnode.begElem);
			nodes[nodeMap[i]].sons[0] = ls;
			nodes[nodeMap[i]].sons[1] = rs;
			nodeMap[leftSons[i]] = ls;
			nodeMap[rightSons[i]] = rs;
		}
	}
}

bool idBvhCreator::KMeansClustering(int beg, int end, int *coloring) {
	static const int ARITY = K_MEANS_ARITY;
	static const int ITERS = 8;
	static const int INIT_TRIES = 16;

	// which elements are "heads" of clusters
	int headStart[ARITY];
	idVec3 headPos[ARITY];

	// choose random element for first head
	headStart[0] = beg + rnd.RandomInt(end - beg);
	headPos[0] = elements[headStart[0]].center;
	for (int h = 1; h < ARITY; h++) {
		float bestDist2 = 0.0f;
		int bestIdx = -1;
		// pick random element a few times, select one most distant from previous heads
		for (int t = 0; t < INIT_TRIES; t++) {
			int idx = beg + rnd.RandomInt(end - beg);
			float minDist2 = 1e+20f;
			for (int j = 0; j < h; j++) {
				float currDist2 = (elements[idx].center - headPos[j]).LengthSqr();
				minDist2 = idMath::Fmin(minDist2, currDist2);
			}
			if (bestDist2 < minDist2) {
				bestDist2 = minDist2;
				bestIdx = idx;
			}
		}
		if (bestIdx < 0) {
			// failed to pick samples: too many equal elements
			return false;
		}
		headStart[h] = bestIdx;
		headPos[h] = elements[bestIdx].center;
	}

	// sum/average of centers in each cluster
	idVec3 clusterPos[ARITY];
	// number of elements in each cluster
	int clusterCnt[ARITY];

	for (int i = 0; i < ITERS; i++) {
		memset(clusterPos, 0, sizeof(clusterPos));
		memset(clusterCnt, 0, sizeof(clusterCnt));

		for (int u = beg; u < end; u++) {
			idVec3 pos = elements[u].center;

			// find closest cluster head
			float bestDist2 = 1e+20f;
			int bestIdx = -1;
			for (int h = 0; h < ARITY; h++) {
				float currDist2 = (pos - headPos[h]).LengthSqr();
				if (bestDist2 > currDist2) {
					bestDist2 = currDist2;
					bestIdx = h;
				}
			}

			// add element to cluster stats
			clusterPos[bestIdx] += pos;
			clusterCnt[bestIdx]++;

			if (i == ITERS-1) {
				// last iteration: save which cluster this element is in
				assert(bestIdx >= 0 && bestIdx < ARITY);
				coloring[u] = bestIdx;
			}
		}

		// update positions of cluster centers
		for (int h = 0; h < ARITY; h++) {
			if (clusterCnt[h] == 0) {
				// cluster degenerated into emptyness
				return false;
			}
			headPos[h] = clusterPos[h] / clusterCnt[h];
			assert( headPos[h].Length() <= 1e+10f );
		}
	}

	return true;
}

void idBvhCreator::AgglomerativeClustering(int num, idBounds *bounds, int *leftSons, int *rightSons) {
	bool *used = (bool*)_alloca(num * 2);
	memset(used, 0, num * 2);

	memset(leftSons, -1, num * sizeof(leftSons[0]));
	memset(rightSons, -1, num * sizeof(rightSons[0]));

	for (int i = 0; i < num-1; i++) {
		// find two yet unmerged clusters with minimum difference of bounding boxes
		float minDiff = 1e+20f;
		int bestU = -1, bestV = -1;
		for (int u = 0; u < num + i; u++) if (!used[u])
			for (int v = u + 1; v < num + i; v++) if (!used[v]) {
				float currDiff = (bounds[u][0] - bounds[v][0]).LengthSqr() + (bounds[u][1] - bounds[v][1]).LengthSqr();
				if (minDiff > currDiff) {
					minDiff = currDiff;
					bestU = u;
					bestV = v;
				}
			}

		// merge these two clusters
		bounds[num + i] = bounds[bestU];
		bounds[num + i].AddBounds(bounds[bestV]);
		leftSons[num + i] = bestU;
		rightSons[num + i] = bestV;
		used[bestU] = used[bestV] = true;
	}
}

void idBvhCreator::GetLeavesOrder(const int *leftSons, const int *rightSons, int v, int *order, int &ordNum) {
	assert((leftSons[v] < 0) == (rightSons[v] < 0));
	if (leftSons[v] < 0)
		order[ordNum++] = v;
	else {
		GetLeavesOrder(leftSons, rightSons, leftSons[v], order, ordNum);
		GetLeavesOrder(leftSons, rightSons, rightSons[v], order, ordNum);
	}
}

void idBvhCreator::CompressSubintervals(const idBounds &parentBounds, const idBounds &sonBounds, byte subintervals[3]) {
	for (int d = 0; d < 3; d++) {
		float pmin = parentBounds[0][d];
		float pmax = parentBounds[1][d];
		float l = (sonBounds[0][d] - pmin) / idMath::Fmax(pmax - pmin, 1e-20f);
		float r = (sonBounds[1][d] - pmin) / idMath::Fmax(pmax - pmin, 1e-20f);
		int lower = idMath::Floor(l * 15.0f);
		int upper = idMath::Ceil(r * 15.0f);
		lower = idMath::ClampInt(0, 15, lower);
		upper = idMath::ClampInt(0, 15, upper);
		if (lower == upper) {
			if (upper == 15) lower--;
			else upper++;
		}
		byte code = lower + (upper << 4);
		subintervals[d] = code;
	}
}

void idBvhCreator::CompressBoundingCone(const idCircCone &cone, char coneCenter[3], byte &coneAngle) {
	// round cone axis to grid nearest
	for (int d = 0; d < 3; d++) {
		int x = idMath::Round(cone.GetAxis()[d] * 127.0f);
		x = idMath::ClampInt(-127, 127, x);
		coneCenter[d] = x;
	}

	idCircCone coneCompr;
	if (cone.IsFull())
		coneCompr.MakeFull();
	else {
		// set cone axis to grid point, bound the old cone
		bvhNode_t tempNode;
		memcpy(tempNode.coneCenter, coneCenter, sizeof(tempNode.coneCenter));
		tempNode.coneAngle = 0;
		coneCompr = tempNode.GetCone();
		coneCompr.AddConeSaveAxis(cone);
		coneCompr.ExpandAngleSelf(DIRECTION_CONE_EXPAND);
	}

	// round cone angle up to nearest grid point
	float angle = coneCompr.GetAngle();
	int x = idMath::Ceil(angle / idMath::PI * 255.0f);
	x = idMath::ClampInt(0, 255, x);
	coneAngle = x;
}

void idBvhCreator::ComputeBoundingCones() {
	for (int i = nodes.Num() - 1; i >= 0; i--) {
		int beg = nodes[i].begElem;
		int end = nodes[i].endElem;

		idCircCone cone;
		cone.Clear();

		if (nodes[i].sons[0] >= 0) {
			int s0 = nodes[i].sons[0];
			int s1 = nodes[i].sons[1];
			assert(s0 > i && s1 > i);
			if (nodes[s0].cone.IsFull() || nodes[s1].cone.IsFull()) {
				// full cone in son -> this node surely has full cone too
				cone.MakeFull();
			}
		}

		if (cone.IsEmpty()) {
			// compute normal cone directly
			for (int j = beg; j < end; j++) {
				cone.AddVec(elements[j].direction);
				if (cone.IsFull())
					break;
				if ((j & 31) == 0)
					cone.Normalize();
			}
		}
		cone.Normalize();

		nodes[i].cone = cone;
	}
}

void idBvhCreator::CompressBvh() {
	int n = nodes.Num();
	compressed.Clear();
	compressed.Reserve(n);

	int root = 0;
	rootBounds = nodes[root].bounds;
	compressed.AddGrow(bvhNode_t());
	for (int d = 0; d < 3; d++)
		compressed[0].subintervals[d] = 0xF0;

	idList<int> idxQueue;
	idxQueue.Reserve(n);
	idxQueue.AddGrow(root);

	for (int i = 0; i < idxQueue.Num(); i++) {
		int idx = idxQueue[i];
		int base = compressed.Num();

		idCircCone cone = nodes[idx].cone;
		CompressBoundingCone(cone, compressed[i].coneCenter, compressed[i].coneAngle);
		//note: compressed[i].subintervals already set from parent

		compressed[i].numElements = nodes[idx].endElem - nodes[idx].begElem;
		if (compressed[i].numElements == 0)
			assert(elemsNum == 0 && idxQueue.Num() == 1);

		if (nodes[idx].sons[0] < 0) {
			compressed[i].firstElement = nodes[idx].begElem;
			continue;
		}
		compressed[i].sonOffset = i - base;

		for (int s = 0; s < 2; s++) {
			int sonIdx = nodes[idx].sons[s];
			bvhNode_t newn;
			idBounds oldSonBounds = nodes[sonIdx].bounds;
			CompressSubintervals(nodes[idx].bounds, oldSonBounds, newn.subintervals);
			idBounds newSonBounds = newn.GetBounds(nodes[idx].bounds);
			nodes[sonIdx].bounds = newSonBounds;
#ifdef _DEBUG
			//check that bounds approximation is conservative
			newSonBounds.ExpandSelf(0.1f);
			assert(newSonBounds.ContainsPoint(oldSonBounds[0]) && newSonBounds.ContainsPoint(oldSonBounds[1]));
#endif
			compressed.AddGrow(newn);
			idxQueue.AddGrow(sonIdx);
		}
	}
}

//optimize this function in Debug with Inlines configuration
DEBUG_OPTIMIZE_ON
idBounds bvhNode_t::GetBounds(const idBounds &parentBounds) const {
#ifdef __SSE2__
	__m128 par0Xyzx = _mm_loadu_ps( &parentBounds[0].x );
	__m128 par1Zxyz = _mm_loadu_ps( &parentBounds[0].z );
	__m128 pmin = par0Xyzx;
	__m128 pmax = _mm_shuffle_ps( par1Zxyz, par1Zxyz, _MM_SHUFFLE(0, 3, 2, 1) );
	__m128 plen = _mm_sub_ps( pmax, pmin );

	int all = *(int*)subintervals;
	__m128i code = _mm_cvtsi32_si128(all);
	code = _mm_unpacklo_epi8(code, _mm_setzero_si128());
	code = _mm_unpacklo_epi16(code, _mm_setzero_si128());
	__m128 l = _mm_mul_ps( _mm_cvtepi32_ps( _mm_and_si128(code, _mm_set1_epi32(0x0F)) ), _mm_set1_ps(1.0f / 15.0f) );
	__m128 r = _mm_mul_ps( _mm_cvtepi32_ps( _mm_srli_epi32(code, 4) ), _mm_set1_ps(1.0f / 15.0f) );
	__m128 resMin = _mm_add_ps( pmin, _mm_mul_ps(plen, l) );
	__m128 resMax = _mm_add_ps( pmin, _mm_mul_ps(plen, r) );

	// note: this is pretty ugly, but don't see other way, and unnecessary loads are unavoidable here
	float data[8];
	_mm_storeu_ps( data + 0, resMin );
	_mm_storeu_ps( data + 3, resMax );
	return *(idBounds*)data;
#else
	idBounds res;
	for (int d = 0; d < 3; d++) {
		byte code = subintervals[d];
		float l = (code & 0x0F) * (1.0f / 15.0f);
		float r = (code >> 4) * (1.0f / 15.0f);
		float pmin = parentBounds[0][d];
		float pmax = parentBounds[1][d];
		res[0][d] = pmin + (pmax - pmin) * l;
		res[1][d] = pmin + (pmax - pmin) * r;
	}
	return res;
#endif
}
DEBUG_OPTIMIZE_OFF

idCircCone bvhNode_t::GetCone() const {
	if (coneAngle == 255)
		return idCircCone::Full();
	float angle = coneAngle * (idMath::PI / 255.0f);
	idVec3 axis;
	for (int d = 0; d < 3; d++)
		axis[d] = coneCenter[d] * (1.0f / 127.0f);
	idCircCone res;
	res.SetAngle(axis, angle);
	return res;
}

//optimize this function in Debug with Inlines configuration
DEBUG_OPTIMIZE_ON
int bvhNode_t::HaveSameDirection( const idVec3 &origin, const idBounds &box ) const {
#if 0
	// generic but slow implementaton
	idCircCone cone = GetCone();
	idCircCone tobox;
	tobox.SetBox(origin, box);
	return cone.HaveSameDirection(tobox);

#else
	if (coneAngle >= 128)
		return 0;	// full or reflex cone -> can't get anything else

	static const float TOL = 1e-3f;

	// compute "origin-to-box" bounding cone
	float boxRadius = 0.5f * box.GetSize().LengthFast();
	idVec3 boxAxis = box.GetCenter() - origin;
	float boxDistSqr = boxAxis.LengthSqr();
	float boxDistInv = idMath::RSqrt(boxDistSqr);
	float boxAngleSin = boxRadius * boxDistInv + TOL;
	float boxAngleCos = 1.0f - boxAngleSin * boxAngleSin;
	if (boxAngleCos <= 0.0f)
		return 0;	// origin inside box
	boxAngleCos *= idMath::RSqrt(boxAngleCos);

	// decompress this cone angle
	idVec3 nodeAxis = idVec3(coneCenter[0], coneCenter[1], coneCenter[2]) * (1.0f / 127.0f);
	float nodeAngleSin = quantizedSinLut[coneAngle];
	float nodeAngleCos = 1.0f - nodeAngleSin * nodeAngleSin;
	nodeAngleCos *= idMath::RSqrt(nodeAngleCos);

	// compute angle between cone axes
	float dotAxes = (nodeAxis * boxAxis);
	float betweenAngleCos = dotAxes * boxDistInv * idMath::RSqrt(nodeAxis.LengthSqr());
	float betweenAngleSin = 1.0f - betweenAngleCos * betweenAngleCos + TOL;
	betweenAngleSin *= idMath::RSqrt(betweenAngleSin);
	float sumAngleCos = nodeAngleCos * boxAngleCos - nodeAngleSin * boxAngleSin;
	if (betweenAngleSin + TOL >= sumAngleCos)
		return 0;

	return (dotAxes < 0.0f ? -1 : 1);
#endif
}
DEBUG_OPTIMIZE_OFF


#include "../tests/testing.h"

TEST_CASE("BvhChecks:HaveSameDirection") {
	bvhNode_t::Init();

#ifdef _DEBUG
	static const int TRIES = 1000000;
#else
	static const int TRIES = 10000000;
#endif
	idRandom rnd;

	int numConservative = 0;
	int numErrors = 0;
	int numRes[3] = {0};

	for (int t = 0; t < TRIES; t++) {
		idVec3 origin;
		origin.x = rnd.RandomFloat();
		origin.y = rnd.RandomFloat();
		origin.z = rnd.RandomFloat();

		idVec3 boxPntA, boxPntB;
		boxPntA.x = rnd.RandomFloat();
		boxPntA.y = rnd.RandomFloat();
		boxPntA.z = rnd.RandomFloat();
		boxPntB.x = rnd.CRandomFloat() * 0.3f;
		boxPntB.y = rnd.CRandomFloat() * 0.3f;
		boxPntB.z = rnd.CRandomFloat() * 0.3f;
		boxPntB += boxPntA;
		idBounds box(boxPntA);
		box.AddPoint(boxPntB);

		idVec3 axis;
		do {
			axis.x = rnd.CRandomFloat();
			axis.y = rnd.CRandomFloat();
			axis.z = rnd.CRandomFloat();
		} while (axis.LengthSqr() < 0.1f || axis.LengthSqr() > 1.0f);
		axis.Normalize();
		double angle = rnd.RandomFloat() * idMath::PI;
		idCircCone cone;
		cone.SetAngle(axis, angle);

		bvhNode_t node;
		idBvhCreator::CompressBoundingCone(cone, node.coneCenter, node.coneAngle);

		int oriFast = node.HaveSameDirection(origin, box);

		idCircCone decompCone = node.GetCone();
		idCircCone tobox;
		tobox.SetBox(origin, box);
		int oriSlow = decompCone.HaveSameDirection(tobox);

		numRes[oriSlow + 1]++;
		if (oriSlow == oriFast)
			continue;
		if (oriFast == 0)
			numConservative++;
		else
			numErrors++;	// wrong culling
	}

	// I got no errors on 10^9 tries
	CHECK(numErrors == 0);
	CHECK(numConservative <= 0.3e-2 * TRIES);	// I got a bit more than 0.1%
}
