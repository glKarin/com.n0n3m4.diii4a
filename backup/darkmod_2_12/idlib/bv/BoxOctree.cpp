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

#include "BoxOctree.h"


// uncomment this temporarily to force validity check in Debug build
// note that it is VERY SLOW, so never commit it enabled!
//#define ASSERT_VALIDITY AssertValidity();
#define ASSERT_VALIDITY (void(0));

struct idBoxOctree::QueryContext {
	QueryResult *res;
	//total bounding box
	idBounds box;
	//sweeped box = moving bounds
	bool moving;
	idVec3 start;
	idVec3 invDir;
	idVec3 extent;
};

struct idBoxOctree::AddContext {
	Pointer ptr;
	idBounds box;
	int level;
};

struct idBoxOctree::CellRanges {
	int bmin[3];
	int bmax[3];
};

void idBoxOctree::Clear() {
	for (int nodeIdx = 0; nodeIdx < nodes.Num(); nodeIdx++) {
		for (Chunk *chunk = nodes[nodeIdx].links, *nextChunk; chunk; chunk = nextChunk) {
			for (int i = 0; i < chunk->num; i++) {
				Pointer ptr = chunk->arr[i].object;
				getHandle(ptr).ids.Clear();
			}
			nextChunk = chunk->next;
			allocator.Free(chunk);
		}
	}
	nodes.Clear();
}

idBoxOctree::idBoxOctree() {}

idBoxOctree::~idBoxOctree() {
	Clear();
}

void idBoxOctree::Init(const idBounds &worldBounds, HandleGetter getHandle) {
	Clear();

	this->worldBounds = worldBounds;
	this->getHandle = getHandle;
	this->nodes.AddGrow(OctreeNode());

	invWorldSize = idVec3(1.0f);
	invWorldSize.DivCW(worldBounds.GetSize());
	ASSERT_VALIDITY
}

void idBoxOctree::Condense() {
	idList<Chunk*> chunks;
	for (int nodeIdx = 0; nodeIdx < nodes.Num(); nodeIdx++) {

		chunks.Clear();
		for (Chunk *chunk = nodes[nodeIdx].links; chunk; chunk = chunk->next)
			chunks.Append(chunk);
		// note: new chunks are inserted at front, so reverse to get chronological order
		chunks.Reverse();

		// push all items left as much as possible
		int ni = 0;
		int nk = 0;
		for (int i = 0; i < chunks.Num(); i++) {
			for (int k = 0; k < chunks[i]->num; k++) {
				if (nk == CHUNK_SIZE) {
					assert(chunks[ni]->num == 0);
					chunks[ni]->num = nk;
					ni++;
					nk = 0;
				}
				chunks[ni]->arr[nk++] = chunks[i]->arr[k];
			}
			chunks[i]->num = 0;
		}
		if (nk) {
			chunks[ni]->num = nk;
			ni++;
			nk = 0;
		}

		// given that chunks are ordered in reversed order
		// link the last chunk we filled as first
		nodes[nodeIdx].links = ni == 0 ? nullptr : chunks[ni-1];

		// free all chunks with no items
		for (int i = ni; i < chunks.Num(); i++)
			allocator.Free(chunks[i]);
	}
	ASSERT_VALIDITY
}

void idBoxOctree::Add(Pointer ptr, const idBounds &box) {
	assert(!getHandle(ptr).IsLinked());
	AddContext ctx = {ptr, box, GetLevel(box)};
	idBounds cellBox = worldBounds;
	Add_r(ctx, 0, cellBox);
	getHandle(ptr).bounds = box;
	ASSERT_VALIDITY
}

void idBoxOctree::Remove(Pointer ptr) {
	idBoxOctreeHandle &handle = getHandle(ptr);

	for (int i = 0; i < handle.ids.Num(); i++) {
		int nodeIdx = handle.ids[i];
		OctreeNode &node = nodes[nodeIdx];

		int level = -1;
		for (Chunk *chunk = node.links; chunk; chunk = chunk->next) {
			for (int u = chunk->num - 1; u >= 0; u--) {
				if (chunk->arr[u].object == ptr) {
					level = GetLevel(chunk->arr[u].bounds);
					idSwap(chunk->arr[u], chunk->arr[chunk->num - 1]);
					chunk->num--;
					break;
				}
			}
			if (level >= 0)
				break;
		}

		if (level >= 0) {
			bool isSmall = (level > node.depth);
			node.numSmall -= int(isSmall);
			assert(node.numSmall >= 0);
		}
	}

	handle.ids.Clear();
	handle.bounds.Clear();

	ASSERT_VALIDITY
}

void idBoxOctree::Update(Pointer ptr, const idBounds &box) {
	idBoxOctreeHandle &handle = getHandle(ptr);

	bool fastUpdate;
	if (handle.IsLinked()) {
		// check if location has changed at all
		static_assert(sizeof(idBounds) == 24, "idBounds: expected tight packing");
		if (memcmp(&handle.bounds, &box, sizeof(idBounds)) == 0)
			return;	// nothing to update

		// find maximum depth of the object
		int maxDepth = 0;
		for (int i = 0; i < handle.ids.Num(); i++)
			maxDepth = idMath::Imax(maxDepth, nodes[handle.ids[i]].depth);

		// compare set of cells which the object spans over
		CellRanges oldRange = GetCellRanges(handle.bounds, maxDepth);
		CellRanges newRange = GetCellRanges(box, maxDepth);

		static_assert(sizeof(CellRanges) == 24, "CellRanges: expected tight packing");
		fastUpdate = (memcmp(&oldRange, &newRange, sizeof(CellRanges)) == 0);

		if (fastUpdate) {
			int oldLevel = GetLevel(handle.bounds);
			int newLevel = GetLevel(box);
			if (oldLevel != newLevel) {
				// level has changed => numSmall might change too
				// the only exception is when object was and is small in all its nodes
				if (!(oldLevel > maxDepth && newLevel > maxDepth))
					fastUpdate = false;
			}
		}
	}
	else {
		fastUpdate = false;
	}

	if (fastUpdate) {
		// the object should remain exactly in the same nodes
		// just update Link::box everywhere
		for (int i = 0; i < handle.ids.Num(); i++) {
			bool found = false;

			for (Chunk *chunk = nodes[handle.ids[i]].links; chunk; chunk = chunk->next) {
				for (int j = chunk->num - 1; j >= 0; j--) {
					if (chunk->arr[j].object == ptr) {
						assert(chunk->arr[j].bounds == handle.bounds);
						chunk->arr[j].bounds = box;
						// bubble updated object up like Remove + Add does
						idSwap(chunk->arr[j], chunk->arr[chunk->num - 1]);
						found = true;
						break;
					}
				}
				if (found)
					break;
			}
			assert(found);
		}

		handle.bounds = box;
	}
	else {
		// something might have changed: remove and add afresh
		Remove(ptr);
		Add(ptr, box);
	}

	ASSERT_VALIDITY
}

static const float WHOLE_SPACE_SIZE = 1e+10f;

void idBoxOctree::QueryInBox(const idBounds &box, QueryResult &res) const {
	res.Clear();
	QueryContext ctx = {&res, box, false, idVec3(0), idVec3(0), idVec3(0)};
	idBounds cellBox = worldBounds;
	idBounds spaceBox(idVec3(-WHOLE_SPACE_SIZE), idVec3(WHOLE_SPACE_SIZE));
	Query_r(ctx, 0, cellBox, spaceBox);
	ASSERT_VALIDITY
}

void idBoxOctree::QueryInMovingBox(const idBounds &box, const idVec3 &start, const idVec3 &invDir, const idVec3 &radius, QueryResult &res) const {
	res.Clear();
	QueryContext ctx = {&res, box, true, start, invDir, radius};
	idBounds cellBox = worldBounds;
	idBounds spaceBox(idVec3(-WHOLE_SPACE_SIZE), idVec3(WHOLE_SPACE_SIZE));
	Query_r(ctx, 0, cellBox, spaceBox);
	ASSERT_VALIDITY
}

int idBoxOctree::GetLevel(const idBounds &box) const {
	idVec3 ratio = box.GetSize().MulCW(invWorldSize);
	float maxRatio = ratio.Max();
	int level = idMath::ILog2(maxRatio);
	// obj_size / cell_size in [0.25 .. 0.5)
	return idMath::Imax(-level - 2, 0);
}

idBoxOctree::CellRanges idBoxOctree::GetCellRanges(const idBounds &box, int maxDepth) const {
	idVec3 cellMin = (box[0] - worldBounds[0]).MulCW(invWorldSize);
	idVec3 cellMax = (box[1] - worldBounds[0]).MulCW(invWorldSize);
	cellMin.Clamp(idVec3(0.0f), idVec3(1.0f));
	cellMax.Clamp(idVec3(0.0f), idVec3(1.0f));
	cellMin *= (1 << maxDepth);
	cellMax *= (1 << maxDepth);
	CellRanges res;
	res.bmin[0] = int(cellMin.x);
	res.bmin[1] = int(cellMin.y);
	res.bmin[2] = int(cellMin.z);
	res.bmax[0] = int(cellMax.x);
	res.bmax[1] = int(cellMax.y);
	res.bmax[2] = int(cellMax.z);
	return res;
}

void idBoxOctree::Query_r(QueryContext &ctx, int nodeIdx, const idBounds &cellBox, const idBounds &spaceBox) const {
	const OctreeNode &node = nodes[nodeIdx];
	assert(spaceBox.IntersectsBounds(ctx.box));

	if (ctx.moving) {
		float range[2] = {0.0f, 1.0f};
		if (!MovingBoundsIntersectBounds(ctx.start, ctx.invDir, ctx.extent, spaceBox, range))
			return;
	}

	// add objects in this node to result
	if (node.links) {
		for (Chunk *chunk = node.links; chunk; chunk = chunk->next)
			ctx.res->AddGrow(chunk);
	}

	int base = node.firstSonIdx;
	if (base < 0)
		return;

	// this is not leaf: split cell
	idVec3 middle = cellBox.GetCenter();

	// detect which sons overlap with box
	int maskX = (ctx.box[0].x <= middle.x ? 1 : 0) + (ctx.box[1].x >= middle.x ? 2 : 0);
	int maskY = (ctx.box[0].y <= middle.y ? 1 : 0) + (ctx.box[1].y >= middle.y ? 2 : 0);
	int maskZ = (ctx.box[0].z <= middle.z ? 1 : 0) + (ctx.box[1].z >= middle.z ? 2 : 0);

	// process all sons recursively
	#define PROCESS_SON(s)                                                                          \
		if ((maskX & (s & 1 ? 2 : 1)) && (maskY & (s & 2 ? 2 : 1)) && (maskZ & (s & 4 ? 2 : 1))) {  \
			idBounds subCellBox = cellBox;                                                          \
			idBounds subSpaceBox = spaceBox;                                                        \
			subCellBox[s & 1 ? 0 : 1].x = subSpaceBox[s & 1 ? 0 : 1].x = middle.x;	                \
			subCellBox[s & 2 ? 0 : 1].y = subSpaceBox[s & 2 ? 0 : 1].y = middle.y;	                \
			subCellBox[s & 4 ? 0 : 1].z = subSpaceBox[s & 4 ? 0 : 1].z = middle.z;	                \
			Query_r(ctx, base + s, subCellBox, subSpaceBox);                                        \
		}

	PROCESS_SON(0)
	PROCESS_SON(1)
	PROCESS_SON(2)
	PROCESS_SON(3)
	PROCESS_SON(4)
	PROCESS_SON(5)
	PROCESS_SON(6)
	PROCESS_SON(7)

	#undef PROCESS_SON
}

void idBoxOctree::Add_r(AddContext &ctx, int nodeIdx, const idBounds &cellBox) {
	OctreeNode &node = nodes[nodeIdx];

	int base = node.firstSonIdx;

	if (ctx.level <= node.depth || base < 0) {
		// either this is a leaf, or the object is too large to go deeper
		return AddToNode(ctx, nodeIdx, cellBox);
	}

	// this is not leaf: split cell
	idVec3 middle = cellBox.GetCenter();

	// detect which sons overlap with box
	int maskX = (ctx.box[0].x <= middle.x ? 1 : 0) + (ctx.box[1].x >= middle.x ? 2 : 0);
	int maskY = (ctx.box[0].y <= middle.y ? 1 : 0) + (ctx.box[1].y >= middle.y ? 2 : 0);
	int maskZ = (ctx.box[0].z <= middle.z ? 1 : 0) + (ctx.box[1].z >= middle.z ? 2 : 0);

	// process all sons recursively
	#define PROCESS_SON(s)                                                                      \
	if ((maskX & (s & 1 ? 2 : 1)) && (maskY & (s & 2 ? 2 : 1)) && (maskZ & (s & 4 ? 2 : 1))) {  \
		idBounds subCellBox = cellBox;                                                          \
		subCellBox[s & 1 ? 0 : 1].x = middle.x;													\
		subCellBox[s & 2 ? 0 : 1].y = middle.y;													\
		subCellBox[s & 4 ? 0 : 1].z = middle.z;													\
		Add_r(ctx, base + s, subCellBox);                                                       \
	}

	PROCESS_SON(0)
	PROCESS_SON(1)
	PROCESS_SON(2)
	PROCESS_SON(3)
	PROCESS_SON(4)
	PROCESS_SON(5)
	PROCESS_SON(6)
	PROCESS_SON(7)

	#undef PROCESS_SON
}

void idBoxOctree::AddToNode(AddContext &ctx, int nodeIdx, const idBounds &cellBox) {
	OctreeNode &node = nodes[nodeIdx];

	// check if existing chunk has free space
	Chunk *added = nullptr;
	for (Chunk *chunk = node.links; chunk; chunk = chunk->next)
		if (chunk->num < CHUNK_SIZE) {
			added = chunk;
			break;
		}
	if (!added) {
		// add new chunk at start
		added = allocator.Alloc();
		added->num = 0;
		added->next = node.links;
		node.links = added;
	}

	// add object to chunk
	added->arr[added->num++] = Link{ctx.ptr, ctx.box};
	// update number of small objects
	node.numSmall += ctx.level > node.depth;
	// register this node in handle
	getHandle(ctx.ptr).ids.AddGrow(nodeIdx);

	if (node.numSmall < SPLIT_SMALL_NUM)
		return;
	// too many small objects in this leaf
	// time to grow octree deeper here

	// create empty son nodes
	assert(node.firstSonIdx < 0);
	int base = node.firstSonIdx = nodes.Num();
	int sonDepth = node.depth + 1;
	for (int s = 0; s < 8; s++) {
		nodes.AddGrow(OctreeNode());
		nodes[base + s].depth = sonDepth;
	}
	OctreeNode &updnode = nodes[nodeIdx];

	// detach chunks with links from this node
	// this node becomes perfectly empty
	Chunk *reinsertLinks = updnode.links;
	updnode.links = nullptr;
	updnode.numSmall = 0;

	// go through objects and sift small objects down
	for (Chunk *chunk = reinsertLinks; chunk; chunk = chunk->next) {
		for (int i = 0; i < chunk->num; i++) {
			const Link &link = chunk->arr[i];

			// unregister this node from object's handle
			auto &ids = getHandle(link.object).ids;
			for (int j = 0; j < ids.Num(); j++)
				if (ids[j] == nodeIdx) {
					idSwap(ids[j], ids[ids.Num() - 1]);
					ids.Pop();
					break;
				}

			// reinsert object as usual, starting from this node
			AddContext ctx = {link.object, link.bounds, GetLevel(link.bounds)};
			Add_r(ctx, nodeIdx, cellBox);
		}
	}

	// free detached chunks
	for (Chunk *chunk = reinsertLinks, *nextChunk; chunk; chunk = nextChunk) {
		nextChunk = chunk->next;
		allocator.Free(chunk);
	}
}

void idBoxOctree::AssertValidity() const {
	AssertValidity_r(0, 0);
}

void idBoxOctree::AssertValidity_r(int nodeIdx, int depth) const {
	const OctreeNode &node = nodes[nodeIdx];
	assert(node.depth == depth);

	int realNumSmall = 0;
	for (Chunk *chunk = node.links; chunk; chunk = chunk->next) {
		int k = chunk->num;
		assert(k >= 0 && k <= CHUNK_SIZE);

		for (int i = 0; i < k; i++) {
			const Link &link = chunk->arr[i];

			const idBoxOctreeHandle &handle = getHandle(link.object);
			assert(handle.IsLinked());
			assert(link.bounds == handle.bounds);

			bool found = false;
			for (int u = 0; u < handle.ids.Num(); u++) {
				int linkNodeIdx = handle.ids[u];
				found |= (linkNodeIdx == nodeIdx);
			}
			assert(found);

			int level = GetLevel(handle.bounds);
			realNumSmall += (level > depth);
		}
	}
	assert(realNumSmall == node.numSmall);

	if (node.firstSonIdx != -1) {
		assert(node.firstSonIdx > nodeIdx && node.firstSonIdx + 8 <= nodes.Num());
		for (int s = 0; s < 8; s++)
			AssertValidity_r(node.firstSonIdx + s, depth + 1);
	}
}
