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

#ifndef __BVH_H__
#define __BVH_H__

#include "CircCone.h"

/*
===============================================================================

	Bounding Volume Hierarchy

===============================================================================
*/


/**
 * stgatilov #5886: single node of compressed BVH
 * The full tree is described by:
 *   1. bounding box of root node (i.e. whole model)
 *   2. array of BVH nodes (of this type)
 *   3. consistenty ordered array of primitives (e.g. triangles)
 */
typedef struct bvhNode_s {
	// bounding box of all elements of this node (in compressed format)
	// suppose parent has [L..R] box interval along d-th coordinate
	// divide d-th byte into low and high 4-bit halves
	// then this node has the following box interval along d-th coordinate:
	//   [ L + (R-L) * low/15, L + (R-L) * high/15 ]
	byte subintervals[3];
	// axis of circular cone containing all element directions
	// each byte X is in range [-127..127] means value X/127
	char coneCenter[3];
	// angle of circular cone containing all element directions
	// byte X means angle PI * X/255
	// X = 255 means "full cone", i.e. it contains all directions
	byte coneAngle;

	union {
		// [internal node] with index K has two sons
		// with indices K - sonOffset and K - sonOffset + 1
		// note: sonOffset < 0
		int sonOffset;
		// [leaf] index of the first element in this leaf
		int firstElement;
	};

	// number of elements in this subtree
	// for a leaf, elements have indices firstElement <= index < firstElement + numElements
	int numElements;


	ID_INLINE bool HasSons() const { return sonOffset < 0; }
	ID_INLINE int GetSon(int thisIdx, int sonIdx) const { return thisIdx - sonOffset + sonIdx; }

	idBounds GetBounds(const idBounds &parentBounds) const;

	idCircCone GetCone() const;
	// check scalar products of vectors in bounding code vs vectors from "origin" to points of "box"
	// if all these products have same sign, then this sign is returned (otherwise zero is returned)
	int HaveSameDirection( const idVec3 &origin, const idBounds &box ) const;


	// lookup table for bvhNode_s::HaveSameDirection
	static float quantizedSinLut[128];
	// fills quantizedSinLut, called automatically from idBvhCreator constructor
	static void Init();

} bvhNode_t;


// atomic element to be pruned with BVH (e.g. a triangle)
// passed as input data to idBvhCreator
typedef struct bvhElement_s {
	// center of element, used in clustering
	idVec3 center;
	// unit direction (e.g. normal of triangle)
	idVec3 direction;
	// bounding box
	idBounds bounds;
	// integer identifier (e.g. triangle index)
	int id;
} bvhElement_t;


// builds BVH tree for a set of elements
class idBvhCreator {
public:
	idBvhCreator();
	~idBvhCreator();

	enum Algorithm {
		aUnknown,
		// fast build, perfectly balanced tree
		aMedianSplit,
		// slower build (~ 2x), better quality
		aKmeansClustering,
	};

	void SetAlgorithm(Algorithm algo = aKmeansClustering);
	void SetLeafSize(int leafSize = 32);
	void Build(int elemsNum, bvhElement_t *elements);

	ID_INLINE idBounds GetRootBounds() const { return rootBounds; }
	ID_INLINE int GetNodesNumber() const { return compressed.Num(); }
	ID_INLINE void CopyNodes(bvhNode_t *nodes) const { memcpy(nodes, compressed.Ptr(), compressed.MemoryUsed()); }
	ID_INLINE int GetIdAtPos(int newPos) const { return elements[newPos].id; }

	// internal methods: inverse for bvhNode_t's GetBox and GetCone
	static void CompressSubintervals(const idBounds &parentBounds, const idBounds &sonBounds, byte subintervals[3]);
	static void CompressBoundingCone(const idCircCone &cone, char coneCenter[3], byte &coneAngle);

private:
	bool KMeansClustering(int beg, int end, int *coloring);
	void AgglomerativeClustering(int num, idBounds *bounds, int *leftSons, int *rightSons);
	static void GetLeavesOrder(const int *leftSons, const int *rightSons, int v, int *order, int &ordNum);
	void BuildBvhByClustering();
	void BuildBvhByAxisMedian();
	bool SplitNodeByAxisMedian(int idx);
	void ComputeBoundingCones();
	void CompressBvh();

	// input data
	int desiredLeafSize = 0;
	Algorithm algorithm = aUnknown;
	int elemsNum = 0;
	bvhElement_t *elements = nullptr;

	// full-scale BVH
	struct Node;
	idList<Node> nodes;
	// compressed BVH
	idBounds rootBounds;
	idList<bvhNode_t> compressed;

	// temporary data
	idRandom rnd;
	idList<int> coloring;
	idList<bvhElement_t> tempElems;
};

#endif
