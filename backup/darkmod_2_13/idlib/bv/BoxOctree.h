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

#pragma once

#include "math/Line.h"


// in order to put objects into idBoxOctree,
// user has to associate this "handle" with every object
// and provide HandleGetter function to obtain handle from object pointer
class idBoxOctreeHandle {
private:
	// indices of octree nodes which this object is attached to
	idFlexList<int, 2> ids;
	// bounds passed to idBoxOctree::Add (or idBoxOctree::Update)
	idBounds bounds;

	friend class idBoxOctree;

public:
	// is the object with this handle already included in the octree?
	ID_FORCE_INLINE bool IsLinked() const {
		return ids.Num() > 0;
	}
};


// Octree contains a set of objects, which geometrically are axis-aligned boxes.
// Used mainly in idClip to store clipmodels of all entities in the world.
//
// Every octree node is assigned a list of objects, whose bounds at least partly cover the node space.
// This list is represented as a linked list of fixed-size arrays (see Chunk).
// 
// The octree starts as a single root node and grows on demand.
// When number of objects in a leaf node becomes larger than SPLIT_SMALL_NUM,
// 8 sons are added to the node, and the majority of attached objects are sifted down into sons.
// However, the ratio object_size / cell_size never exceeds 50% for any object.
// So large objects never go too deep into the octree, remaining assigned to non-leaf nodes.
//
class idBoxOctree {
public:
	// pointer to object stored in octree
	// this is usually idClipModel*, but can be something else as well
	typedef void *Pointer;

	// function pointer for getting handle stored inside object
	// usually it returns ((idClipModel*)ptr)->octreeHandle
	typedef idBoxOctreeHandle& (*HandleGetter)(Pointer);

	// maximum number of objects per chunk
	// if exceeded, then more chunks are chained in a linked list
	static const int CHUNK_SIZE = 127;	// 4 KB chunk

	// critical number of objects which should ideally reside in smaller cells
	// when this number of "small" objects is gathered, octree node is split into 8 subnodes
	static const int SPLIT_SMALL_NUM = 90;

	// link to single object
	struct Link {
		Pointer object;
		idBounds bounds;
	};
	// a bunch of links to objects
	struct Chunk {
		int num;
		Chunk *next;
		Link arr[CHUNK_SIZE];
	};

	// number of chunk pointers embedded in QueryResult object (same as CLIPARRAY_AUTOSIZE)
	static const int RESULT_AUTOSIZE = 128;

	// returned by Query methods
	// called should walk through all objects mentioned in the returned chunks
	// note: ignore "next" member, iterate over arr[0..num) for every chunk
	typedef idFlexList<Chunk*, RESULT_AUTOSIZE> QueryResult;

	idBoxOctree();
	~idBoxOctree();

	// must be called before any other operations
	void Init(const idBounds &worldBounds, HandleGetter getHandle);
	// condense arrays in chunks and prune away empty chunks
	void Condense();
	// remove all elements
	void Clear();

	// add object with specified box
	void Add(Pointer ptr, const idBounds &box);
	// remove object
	void Remove(Pointer ptr);
	// update location of the object
	// equivalent to Remove + Add, but often faster
	void Update(Pointer ptr, const idBounds &box);

	// find objects with bounding box intersecting the specified box
	// returns list of all chunks which may contain such objects
	void QueryInBox(const idBounds &box, QueryResult &res) const;
	// find objects with bounding box intersecting the specified moving box
	// returns list of all chunks which may contain such objects
	void QueryInMovingBox(const idBounds &box, const idVec3 &start, const idVec3 &invDir, const idVec3 &radius, QueryResult &res) const;

private:
	struct QueryContext;
	struct AddContext;
	struct CellRanges;

	int GetLevel(const idBounds &box) const;
	CellRanges GetCellRanges(const idBounds &box, int maxDepth) const;
	void Query_r(QueryContext &ctx, int nodeIdx, const idBounds &cellBox, const idBounds &spaceBox) const;
	void Add_r(AddContext &ctx, int nodeIdx, const idBounds &cellBox);
	void AddToNode(AddContext &ctx, int nodeIdx, const idBounds &cellBox);

	void AssertValidity() const;
	void AssertValidity_r(int nodeIdx, int depth) const;

	struct OctreeNode {
		Chunk *links = nullptr;		// linked list of chunks attached to node
		int firstSonIdx = -1;		// sons have indices [firstSonIdx .. firstSonIdx + 8)
		short depth = 0;
		short numSmall = 0;			// number of objects with GetLevel(obj) > depth
	};

	// this function gives access to handle within an object
	HandleGetter getHandle = nullptr;
	// space area at the root node, which is subdivided by octree
	// note: objects may go outside worldBounds, but too many outliers will harm performance
	idBounds worldBounds;
	// 1 / worldSize for each coordinate
	idVec3 invWorldSize;
	// all nodes of octree
	idList<OctreeNode> nodes;
	// allocates ~4 MB blocks from where chunks are quickly allocated
	idBlockAlloc<Chunk, 1024> allocator;
};
