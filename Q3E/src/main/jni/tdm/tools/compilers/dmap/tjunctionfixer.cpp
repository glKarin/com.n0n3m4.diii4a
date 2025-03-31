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

#include "dmap.h"
#include "tjunctionfixer.h"
#include "containers/DisjointSets.h"


static const int MAX_RESOLUTION = 32768;

TJunctionFixer::~TJunctionFixer() {}
TJunctionFixer::TJunctionFixer() {
	Reset();
}

void TJunctionFixer::Reset() {
	linkedLists.Clear();
	allTris.Clear();
	starts.Clear();
	clusterPos.Clear();
	numPointsInCell.Clear();
	clusterOfCorner.Clear();
	hashIndex.ClearFree();
	splitAllTris.Clear();
	splitStarts.Clear();

	starts.Append(0);
}

void TJunctionFixer::AddTriList(struct mapTri_s **pTriList) {
	// drop empty lists (no change required)
	if (*pTriList == nullptr)
		return;

	linkedLists.AddGrow(pTriList);
	for (mapTri_t *tri = *pTriList; tri; tri = tri->next)
		allTris.AddGrow(tri);
	starts.AddGrow(allTris.Num());
}

void TJunctionFixer::Run(float sameVertexTolerance, float vertexOnEdgeTolerance, int snapDiv) {
	vertexTol = sameVertexTolerance;
	edgeTol = vertexOnEdgeTolerance;
	snapDivisor = snapDiv;

	// compute bounding box, estimate triangle size
	SetupWorkArea();

	assert(vertexTol > 0.0f);
	MergeCloseVertices();

	assert(vertexOnEdgeTolerance > 0.0f);
	SplitEdgesByVertices();
	UpdateLists();
}

void TJunctionFixer::SetupWorkArea() {
	// compute bounding box of everything, and average triangle size
	bounds.Clear();
	float sumSizesCubed = 0.0f;
	for (int i = 0; i < allTris.Num(); i++) {
		const mapTri_t &tri = *allTris[i];
		idBounds triBox(tri.v[0].xyz);
		triBox.AddPoint(tri.v[1].xyz);
		triBox.AddPoint(tri.v[2].xyz);
		bounds.AddBounds(triBox);
		float triSize = (triBox[1] - triBox[0]).Max();
		sumSizesCubed += triSize * triSize * triSize;
	}
	avgTriSize = idMath::Pow(sumSizesCubed / idMath::Imax(allTris.Num(), 1), 1.0f / 3.0f);

	// set cubic workarea which contains bounding box
	// (slightly expanded), and has same center
	idVec3 ctr = bounds.GetCenter();
	workArea = idBounds(ctr);
	idVec3 radVec = (bounds[1] - bounds[0]) * 0.5f;
	float radMax = idMath::Fmax(radVec.Max(), 0.0f) + 0.5f;
	workArea.ExpandSelf(radMax);

	// precompute inverse for cell index computation
	workSizeInv = idVec3(1.0f);
	workSizeInv.DivCW(workArea[1] - workArea[0]);

	// deduce hash table size and shift
	hashSize = allTris.Num() / 2;
	hashSize = idMath::Imax(idMath::CeilPowerOfTwo(hashSize), 16);
	hashShift = 32 - idMath::ILog2(hashSize);
}

const idVec3 &TJunctionFixer::GetCorner(int cornerIdx) const {
	int triIdx = unsigned(cornerIdx) / 3;
	int k = unsigned(cornerIdx) % 3;
	assert(triIdx < allTris.Num());
	return allTris[triIdx]->v[k].xyz;
}

int TJunctionFixer::CellIndex(const int location[3]) const {
	uint32 dotRandom = 2654435769U * uint32(location[0]) + 1367130551U * uint32(location[1]) + 2370045253U * uint32(location[2]);
	uint32 cell = dotRandom >> hashShift;
	assert(cell < uint32_t(hashSize));
	return cell;
}

void TJunctionFixer::GetCellRange(const idBounds &bounds, int locations[2][3]) const {
	for (int d = 0; d < 2; d++) {
		idVec3 ratio = (bounds[d] - workArea[0]);
		ratio.MulCW(workSizeInv);
		// that's by construction, see SetupWorkArea
		assert(ratio.Min() > 0.0f && ratio.Max() < 1.0f);
		idVec3 floatIdx = ratio * resolution;
		for (int c = 0; c < 3; c++) {
			int x = (int)floatIdx[c];
			assert(x >= 0 && x < resolution);
			locations[d][c] = x;
		}
	}
}

static const hashVert_t *HashVertAtVec3(const idVec3 *pos) {
	// TODO: rework this hacky practice after old tritjunction is deleted!
	// Here we align pointer so that accessing hashVert_t::v through it works properly.
	// That's the only thing FixTriangleAgainstHashVert uses from hashVert_t.
	// Also, we use this pointer as unique identifier of merged vertex (which is OK).
	return (hashVert_t *)( (char*)pos - offsetof(hashVert_t, v) );
}

void TJunctionFixer::MergeCloseVertices() {
	// cell size = 10 x tol: perfect for matching equal points
	float cellSize = 10.0f * vertexTol;
	float floatResolution = (workArea[1] - workArea[0]).x / cellSize;
	resolution = idMath::ClampInt(1, MAX_RESOLUTION, (int)idMath::Ceil(floatResolution));

	// create empty hash grid
	idDisjointSets::Init(clusterOfCorner, 3 * allTris.Num());
	hashIndex.ClearFree(hashSize, 3 * allTris.Num());

	for (int cornerIdx = 0; cornerIdx < 3 * allTris.Num(); cornerIdx++) {
		idVec3 pos = GetCorner(cornerIdx);

		int loc[2][3];
		GetCellRange(idBounds(pos).Expand(vertexTol), loc);

		int idx[3];
		for (idx[0] = loc[0][0]; idx[0] <= loc[1][0]; idx[0]++)
			for (idx[1] = loc[0][1]; idx[1] <= loc[1][1]; idx[1]++)
				for (idx[2] = loc[0][2]; idx[2] <= loc[1][2]; idx[2]++) {
					int cell = CellIndex(idx);

					for (int c = hashIndex.First(cell); c >= 0; c = hashIndex.Next(c)) {
						idVec3 otherPos = GetCorner(c);
						float distSqr = (otherPos - pos).LengthSqr();
						if (distSqr > vertexTol * vertexTol)
							continue;

						// close vertices detected, merge their clusters in DSU
						idDisjointSets::Merge(clusterOfCorner, cornerIdx, c);
					}
				}

		GetCellRange(idBounds(pos), loc);
		int cell = CellIndex(loc[0]);
		hashIndex.Add(cell, cornerIdx);
	}

	// for each corner, save index of merged vertex it belongs to
	int num = idDisjointSets::ConvertToColors(clusterOfCorner);

	// compute average position
	clusterPos.SetNum(num, false);
	idFlexList<int, 128> counts;
	counts.SetNum(num);
	clusterPos.FillZero();
	memset(counts.Ptr(), 0, counts.Num() * sizeof(counts[0]));
	for (int i = 0; i < clusterOfCorner.Num(); i++) {
		int clusterIdx = clusterOfCorner[i];
		clusterPos[clusterIdx] += GetCorner(i);
		counts[clusterIdx]++;
	}
	for (int i = 0; i < num; i++) {
		clusterPos[i] /= counts[i];

		if (snapDivisor > 0) {
			// snap coordinates to D-rational numbers
			for (int d = 0; d < 3; d++) {
				float &x = clusterPos[i][d];
				int q = (int)floor(x * snapDivisor + 0.5);
				x = double(q) / snapDivisor;
			}
		}
	}

	// replace original vertices with merged ones
	for (int i = 0; i < clusterOfCorner.Num(); i++) {
		int clusterIdx = clusterOfCorner[i];
		allTris[i/3]->v[i%3].xyz = clusterPos[clusterIdx];
		allTris[i/3]->hashVert[i%3] = HashVertAtVec3(&clusterPos[clusterIdx]);
	}
}

int64_t TJunctionFixer::EstimateWork(int resol) {
	int64_t hashSearches = 0;
	int64_t triMatches = 0;

	// initialize hash grid with point counter in each cell
	resolution = resol;
	numPointsInCell.SetNum(hashSize, false);
	memset(numPointsInCell.Ptr(), 0, numPointsInCell.Allocated());

	for (int i = 0; i < clusterPos.Num(); i++) {
		// increment counter in hash cell
		int loc[2][3];
		GetCellRange(idBounds(clusterPos[i]), loc);
		int cell = CellIndex(loc[0]);
		numPointsInCell[cell]++;
	}

	for (int i = 0; i < allTris.Num(); i++) {
		const mapTri_t &tri = *allTris[i];

		// consider bbox of the tri
		idBounds triBox(tri.v[0].xyz);
		triBox.AddPoint(tri.v[1].xyz);
		triBox.AddPoint(tri.v[2].xyz);
		triBox.ExpandSelf(edgeTol);

		// pay cost for all points in triangle's bounding box
		int loc[2][3];
		GetCellRange(triBox, loc);
		int idx[3];
		for (idx[0] = loc[0][0]; idx[0] <= loc[1][0]; idx[0]++)
			for (idx[1] = loc[0][1]; idx[1] <= loc[1][1]; idx[1]++)
				for (idx[2] = loc[0][2]; idx[2] <= loc[1][2]; idx[2]++) {
					int cell = CellIndex(idx);
					hashSearches++;
					triMatches += numPointsInCell[cell];
				}
	}

	// weights were assigned rather arbitrarily =)
	return hashSearches * 10 + triMatches;
}

void TJunctionFixer::SplitEdgesByVertices() {
	// start with such cell size that a triangle/point covers O(1) cell on average
	// but this can be expensive due to duplicates, so halve it while it reduces estimate
	float cellSize = avgTriSize + edgeTol;
	float floatResolution = (workArea[1] - workArea[0]).x / cellSize;
	int initialResol = idMath::ClampInt(1, MAX_RESOLUTION, (int)idMath::Ceil(floatResolution));

	// increase resolution while it helps with cost estimate
	int64_t prevEstimate = INT64_MAX;
	for (int resol = initialResol; resol < MAX_RESOLUTION; resol *= 2) {
		int64_t currEstimate = EstimateWork(resol);
		if (currEstimate >= prevEstimate * 0.9f)
			break;
		prevEstimate = currEstimate;
	}

	// fallback by one iteration
	// reset grid for merged vertices (clusters)
	resolution /= 2;
	hashIndex.ClearFree(hashSize, clusterPos.Num());

	// insert all merged verts into grid
	for (int i = 0; i < clusterPos.Num(); i++) {
		idVec3 pos = clusterPos[i];

		int loc[2][3];
		GetCellRange(idBounds(pos), loc);
		int idx[3];
		for (idx[0] = loc[0][0]; idx[0] <= loc[1][0]; idx[0]++)
			for (idx[1] = loc[0][1]; idx[1] <= loc[1][1]; idx[1]++)
				for (idx[2] = loc[0][2]; idx[2] <= loc[1][2]; idx[2]++) {
					int cell = CellIndex(idx);
					hashIndex.Add(cell, i);
				}
	}

	splitStarts.Clear();
	splitStarts.Append(0);
	splitAllTris.Clear();

	// go through triangles, splitting them with all vertices in bbox
	idFlexList<mapTri_t*, 128> splitTris;
	int listIdx = 0;
	for (int i = 0; i < allTris.Num(); i++) {
		mapTri_t &tri = *allTris[i];

		// drop topologically degenerate triangles
		const int *ids = &clusterOfCorner[3 * i];
		bool dropTri = (ids[0] == ids[1] || ids[1] == ids[2] || ids[2] == ids[0]);

		if (dropTri) {
			FreeTri(&tri);
			allTris[i] = nullptr;
		}
		else {
			// consider bbox of the tri
			idBounds triBox(tri.v[0].xyz);
			triBox.AddPoint(tri.v[1].xyz);
			triBox.AddPoint(tri.v[2].xyz);
			triBox.ExpandSelf(edgeTol);

			// the list of triangular pieces after splitting
			splitTris.Clear();
			splitTris.AddGrow(&tri);

			// look through all merged verts in tri's bbox
			int loc[2][3];
			GetCellRange(triBox, loc);
			int idx[3];
			for (idx[0] = loc[0][0]; idx[0] <= loc[1][0]; idx[0]++)
				for (idx[1] = loc[0][1]; idx[1] <= loc[1][1]; idx[1]++)
					for (idx[2] = loc[0][2]; idx[2] <= loc[1][2]; idx[2]++) {
						int cell = CellIndex(idx);

						for (int c = hashIndex.First(cell); c >= 0; c = hashIndex.Next(c)) {
							const idVec3 &splitter = clusterPos[c];

							for (int p = 0; p < splitTris.Num(); p++) {

								extern mapTri_t *FixTriangleAgainstHashVert(const mapTri_t *a, const hashVert_t *hv);
								if (mapTri_t *pieces = FixTriangleAgainstHashVert(splitTris[p], HashVertAtVec3(&splitter))) {
									assert(pieces->next && !pieces->next->next);	// exactly two triangles

									FreeTri(splitTris[p]);
									splitTris[p] = nullptr;

									splitTris.AddGrow(nullptr);
									memmove(&splitTris[p+1], &splitTris[p], (splitTris.Num() - (p+1)) * sizeof(splitTris[0]));

									splitTris[p] = pieces;
									splitTris[p+1] = pieces->next;
								}
							}
						}
					}

			// append list of pieces into newly gathered list
			for (int p = 0; p < splitTris.Num(); p++)
				splitAllTris.AddGrow(splitTris[p]);
		}

		// fill "starts" delimiters in the new list
		// note: here we assume that lists are non-empty!
		if (i + 1 == starts[listIdx + 1]) {
			assert(splitStarts.Num() == listIdx + 1);
			splitStarts.AddGrow(splitAllTris.Num());
			listIdx++;
		}
	}
	assert(splitStarts.Num() == starts.Num());
	assert(splitStarts[0] == 0 && splitStarts.Last() == splitAllTris.Num());
}

void TJunctionFixer::UpdateLists() {
	for (int i = 0; i + 1 < splitStarts.Num(); i++) {
		int beg = splitStarts[i + 0];
		int end = splitStarts[i + 1];

		mapTri_t **ppCurr = linkedLists[i];
		for (int j = beg; j < end; j++) {
			*ppCurr = splitAllTris[j];
			ppCurr = &splitAllTris[j]->next;
		}
		*ppCurr = nullptr;
	}
}
