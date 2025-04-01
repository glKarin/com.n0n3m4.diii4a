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
#include "planargraph.h"

#include "containers/DisjointSets.h"
#include "dmap.h"


int PlanarGraph::AddVertex(idVec2 pos) {
	Vertex nv;
	nv.pos = pos;
	return verts.AddGrow(nv);
}
int PlanarGraph::AddEdge(int v0, int v1) {
	Coedge ce;
	ce.v[0] = v0;
	ce.v[1] = v1;
	coedges.AddGrow(ce);
	ce.v[0] = v1;
	ce.v[1] = v0;
	coedges.AddGrow(ce);
	Edge ne;
	return edges.AddGrow(ne);
}

void PlanarGraph::Finish() {
	int n = verts.Num();
	int m = edges.Num();
	assert(coedges.Num() == 2*m);

	//-----------------------------
	//step 1: find connected components

	idList<int> &dsu = _work;
	idDisjointSets::Init(dsu, n);
	for (int i = 0; i < m; i++) {
		const Coedge &ce = coedges[2*i];
		idDisjointSets::Merge(dsu, ce.v[0], ce.v[1]);
	}
	int cc = idDisjointSets::ConvertToColors(dsu);

	components.SetNum(cc, false);
	for (int i = 0; i < n; i++) {
		verts[i].inComp = dsu[i];
		components[dsu[i]].v = i;
	}

	//-----------------------------
	//step 2: fill "outgoing" array

	outgoing.SetNum(2*m, false);
	for (int i = 0; i < 2*m; i++)
		outgoing[i] = i;
	//sort coedge indices by start vertex
	std::sort(outgoing.begin(), outgoing.end(), [this](int a, int b) -> bool {
		return coedges[a].v[0] < coedges[b].v[0];
	});
	//fill outBeg/outEnd
	for (int v = 0, q = 0; v < n; v++) {
		verts[v].outBeg = q;
		while (q < 2*m && coedges[outgoing[q]].v[0] == v)
			q++;
		verts[v].outEnd = q;
	}

	//------------------------------------------
	//step 3: sort outgoing edges by polar angle

	//prepare direction vectors
	for (int j = 0; j < 2*m; j++) {
		Coedge &ce = coedges[j];
		ce.vec = idVec2d(verts[ce.v[1]].pos) - idVec2d(verts[ce.v[0]].pos);
		assert(ce.vec.x != 0.0 || ce.vec.y != 0.0);
	}

	for (int v = 0; v < n; v++) {
		int beg = verts[v].outBeg, end = verts[v].outEnd;
		//sort in CCW order
		std::sort(outgoing.Ptr() + beg, outgoing.Ptr() + end, [this](int a, int b) -> bool {
			const Coedge &ceA = coedges[a];
			const Coedge &ceB = coedges[b];
			//note: no numeric errors here
			int cmp = idVec2d::PolarAngleCompare(ceA.vec, ceB.vec);
			if (cmp != 0)
				return cmp < 0;
			if (a == b)
				return false;
			//being here means that two incident edges have same direction
			//cannot handle this case in general, but can hack the case of one singular triangle
			//for such case, put longer edge first if it is going up, and second if it is going down
			bool goesUp = (ceA.vec.Quarter() < 2);
			double lenA = ceA.vec.LengthSqr();
			double lenB = ceB.vec.LengthSqr();
			if (lenA != lenB)
				return (lenA < lenB) ^ goesUp;
			return a < b;
		});
		for (int j = beg; j < end; j++)
			coedges[outgoing[j]].outIdx = j;
	}
}

void PlanarGraph::BuildFaces() {
	int n = verts.Num();
	int m = edges.Num();
	assert(coedges.Num() == 2*m);
	int cc = components.Num();

	//------------------------------------------
	//step 1: extract graph facets

	for (int j = 0; j < 2*m; j++)
		coedges[j].inFacet = -1;

	for (int s = 0; s < 2*m; s++) {
		if (coedges[s].inFacet >= 0)
			continue;	//coedge already in facet

		//create new facet here
		Facet facet;
		facet.fcBeg = facetCoedges.Num();

		int curr = s;
		do {
			//mark this coedge and add it
			assert(coedges[curr].inFacet < 0);
			coedges[curr].inFacet = facets.Num();
			facetCoedges.AddGrow(curr);

			//look at end vertex
			int v = coedges[curr].v[1];

			//take opposite coedge
			int rev = curr ^ 1;
			assert(coedges[rev].v[0] == v);
			//find it in outgoing array of v
			int idx = coedges[rev].outIdx;
			assert(idx >= verts[v].outBeg && idx < verts[v].outEnd);

			//take previous v-outgoing coedge in CCW order
			if (idx == verts[v].outBeg)
				idx = verts[v].outEnd;
			idx--;

			//switch to this coedge (it is next in facet)
			curr = outgoing[idx];
		} while (curr != s);

		//finalize new facet
		facet.fcEnd = facetCoedges.Num();
		facets.AddGrow(facet);
	}

	//------------------------------------------
	//step 2: classify facets into CW/CCW

	int k = facets.Num();
	for (int i = 0; i < k; i++) {
		Facet &f = facets[i];
		int beg = f.fcBeg, end = f.fcEnd;

		//compute bounding box and area
		f.bounds[0].Set( FLT_MAX,  FLT_MAX);
		f.bounds[1].Set(-FLT_MAX, -FLT_MAX);
		f.area = 0.0;
		for (int j = beg; j < end; j++) {
			const Coedge &ce = coedges[facetCoedges[j]];
			const Vertex &u = verts[ce.v[0]];
			const Vertex &v = verts[ce.v[1]];
			for (int d = 0; d < 2; d++) {
				f.bounds[0][d] = idMath::Fmin(f.bounds[0][d], u.pos[d]);
				f.bounds[1][d] = idMath::Fmax(f.bounds[1][d], u.pos[d]);
			}
			//integral(-y dx) gives double oriented area
			//note: summing may introduces some numeric error
			//but it should be small enough to sort CW facets properly
			f.area -= ce.vec.x * (double(u.pos.y) + double(v.pos.y));
		}

		//find rightmost vertex in facet (note: it can occur multiple times)
		idVec2 bestPos(-FLT_MAX, -FLT_MAX);
		f.rightmost = -1;
		for (int j = beg; j < end; j++) {
			const Coedge &ce = coedges[facetCoedges[j]];
			const Vertex &v = verts[ce.v[0]];
			if (bestPos.x < v.pos.x || bestPos.x == v.pos.x && bestPos.y < v.pos.y) {
				bestPos = v.pos;
				f.rightmost = ce.v[0];
			}
		}
		assert(f.rightmost >= 0);

		//note: polar sort counts angles from axisX counterclockwise
		//so the last outgoing coedge must belong to inner/CW loop
		int lastCe = outgoing[verts[f.rightmost].outEnd - 1];
		f.clockwise = false;
		for (int j = beg; j < end; j++)
			if (facetCoedges[j] == lastCe) {
				f.clockwise = true;
				break;
			}
	}

	//------------------------------------------
	//step 3: assign CW facets to components

	for (int i = 0; i < cc; i++)
		components[i].cwFacet = -1;

	//add special single-vertex facets for isolated vertices
	for (int i = 0; i < n; i++) {
		if (verts[i].outEnd != verts[i].outBeg)
			continue;
		Facet f;
		f.rightmost = i;
		f.bounds[0] = f.bounds[1] = verts[i].pos;
		f.area = 0.0;
		f.clockwise = true;
		f.fcBeg = f.fcEnd = facetCoedges.Num();
		facets.AddGrow(f);
	}
	k = facets.Num();

	//find clockwise facet in every component
	idList<int> &badComponents = _work;
	badComponents.SetNum(0, false);
	for (int i = 0; i < k; i++) {
		//take any vertex, get its component index
		int v = facets[i].rightmost;
		int q = verts[v].inComp;

		facets[i].inComp = q;

		if (facets[i].clockwise) {
			//register clockwise facet in component
			if (components[q].cwFacet >= 0 || facets[i].area > 1.0)
				badComponents.AddUnique(q);
			components[q].cwFacet = i;
		}
	}
	for (int i = 0; i < cc; i++)
		if (components[i].cwFacet < 0)
			badComponents.AddUnique(i);

	if (badComponents.Num()) {
		//normally, this should never happen
		//however, if some loops are singular (aka singular triangle)
		//then topological criterion for CW/CCW loops can fail

		//switch to plan B: assign CW facet as the one with negative (or minimum) area
		for (int i = 0; i < badComponents.Num(); i++) {
			int c = badComponents[i];
			double minArea = DBL_MAX;
			int minIdx = -1;
			for (int j = 0; j < k; j++) {
				if (facets[j].inComp != c)
					continue;
				facets[j].clockwise = false;
				if (minArea > facets[j].area) {
					minArea = facets[j].area;
					minIdx = j;
				}
			}
			facets[minIdx].clockwise = true;
			components[c].cwFacet = minIdx;
			common->Printf(
				"PlanarGraph: facets reclassified by area (CW area %0.3lf) near (%s)\n",
				-minArea, ReportWorldPositionInOptimizeGroup(verts[facets[minIdx].rightmost].pos, optGroup).c_str()
			);
		}
	}

	//------------------------------------------
	//step 4: check how components are nested

	//sort connected components by their outer area decreasing
	//i.e. by area of clockwise facet (which is negative)
	std::sort(components.begin(), components.end(), [this](const Component &a, const Component &b) -> bool {
		return facets[a.cwFacet].area < facets[b.cwFacet].area;
	});
	//update component backlinks
	idList<int> &remap = _work;
	remap.SetNum(cc, false);
	for (int i = 0; i < cc; i++) {
		int f = components[i].cwFacet;
		int oldCC = facets[f].inComp;
		remap[oldCC] = i;
	}
	for (int i = 0; i < n; i++)
		verts[i].inComp = remap[verts[i].inComp];
	for (int i = 0; i < k; i++)
		facets[i].inComp = remap[facets[i].inComp];

	auto IsVertexInsideFacet = [this](int v, int f) -> bool {
		idVec2 chk = verts[v].pos;
		const Facet &fac = facets[f];
		if (chk.x < fac.bounds[0].x || chk.x > fac.bounds[1].x)
			return false;
		if (chk.y < fac.bounds[0].y || chk.y > fac.bounds[1].y)
			return false;

		int intersCnt = 0;
		int beg = fac.fcBeg, end = fac.fcEnd;
		for (int j = beg; j < end; j++) {
			const Coedge &ce = coedges[facetCoedges[j]];
			const idVec2 &a = verts[ce.v[0]].pos;
			const idVec2 &b = verts[ce.v[1]].pos;

			//note: points with y = 0 are considered "slightly above"
			bool ayneg = (a.y < chk.y);
			bool byneg = (b.y < chk.y);
			if (ayneg == byneg)
				continue;

			bool axneg = (a.x < chk.x);
			bool bxneg = (b.x < chk.x);
			if (axneg == bxneg) {
				if (!axneg) {
					assert(a.x != chk.x || b.x != chk.x);	//(point on line)
					intersCnt++;
				}
				continue;
			}

			//note: no numeric errors here
			double cross = (idVec2d(a) - idVec2d(chk)).Cross(idVec2d(b) - idVec2d(chk));
			assert(cross != 0.0);	//(point on line)
			if (byneg == (cross < 0.0))
				intersCnt++;
		}

		return intersCnt % 2 != 0;
	};

	//determine how components lie inside each other
	for (int i = components.Num() - 1; i >= 0; i--) {
		components[i].parent = -1;
		//find minimum-area component, enclosing this one
		for (int j = i-1; j >= 0; j--) {
			if (IsVertexInsideFacet(components[i].v, components[j].cwFacet)) {
				components[i].parent = j;
				break;
			}
		}
	}

	//------------------------------------------
	//step 5: determine parent of each facet

	for (int i = 0; i < k; i++)
		facets[i].parent = -1;

	for (int i = 0; i < k; i++) {
		int c = facets[i].inComp;
		int parC = components[c].parent;

		if (facets[i].clockwise) {
			if (parC < 0)
				continue;
			//among all CCW facets of parent component, find one which contains this CW facet
			for (int j = 0; j < k; j++) {
				if (facets[j].inComp != parC || facets[j].clockwise)
					continue;
				if (IsVertexInsideFacet(facets[i].rightmost, j)) {
					facets[i].parent = j;
					break;
				}
			}
		}
		else {
			//the only clockwise facet in same component is parent of this CCW facet
			facets[i].parent = components[c].cwFacet;
		}

		assert(facets[i].parent >= 0);
	}

	//------------------------------------------
	//step 6: build lists of facet holes

	idList<int> &holeCnt = _work;
	holeCnt.SetNum(k, false);
	for (int i = 0; i < k; i++)
		holeCnt[i] = 0;

	//count how many holes are there in every facet
	for (int i = 0; i < k; i++)
		if (facets[i].clockwise) {
			int par = facets[i].parent;
			if (par >= 0)
				holeCnt[par]++;
		}

	//determine ranges in facetHoles array
	int sum = 0;
	for (int i = 0; i < k; i++) {
		facets[i].holeBeg = sum;
		sum += holeCnt[i];
		facets[i].holeEnd = sum;

		holeCnt[i] = facets[i].holeBeg;
	}

	//distribute hole facets into facetHoles array
	facetHoles.SetNum(sum, false);
	for (int i = 0; i < k; i++)
		if (facets[i].clockwise) {
			int par = facets[i].parent;
			if (par >= 0)
				facetHoles[holeCnt[par]++] = i;
		}
}

void PlanarGraph::TriangulateFaces(idList<Triangle> &tris, idList<AddedEdge> &addedEdges) {
	for (int i = 0; i < facets.Num(); i++) {
		if (facets[i].clockwise)
			continue;
		const Facet &fOuter = facets[i];

		idList<int> &remap = _work;
		remap.SetNum(0, false);

		earcut.Reset();
		earcut.SetOptimizeGroup(optGroup);

		//pass all holes
		for (int u = fOuter.holeBeg; u < fOuter.holeEnd; u++) {
			const Facet &fHole = facets[facetHoles[u]];
			for (int j = fHole.fcBeg; j < fHole.fcEnd; j++) {
				const Coedge &ce = coedges[facetCoedges[j]];
				int v = ce.v[0];
				earcut.AddVertex(verts[v].pos);
				remap.AddGrow(v);
			}
			if (fHole.fcBeg == fHole.fcEnd) {	//single-vertex facet
				int v = fHole.rightmost;
				earcut.AddVertex(verts[v].pos);
				remap.AddGrow(v);
			}
			bool isHole = true;
			earcut.FinishLoop(&isHole);
		}
		//pass outer loop
		for (int j = fOuter.fcBeg; j < fOuter.fcEnd; j++) {
			const Coedge &ce = coedges[facetCoedges[j]];
			int v = ce.v[0];
			earcut.AddVertex(verts[v].pos);
			remap.AddGrow(v);
		}
		bool isHole = false;
		earcut.FinishLoop(&isHole);

		earcut.Triangulate();

		const idList<Triangle> &currTris = earcut.GetTriangles();
		const idList<AddedEdge> &currSeams = earcut.GetSeamEdges();

		//append seam edges to output arrays
		for (int i = 0; i < currSeams.Num(); i++) {
			AddedEdge e = currSeams[i];
			for (int q = 0; q < 2; q++)
				e.ids[q] = remap[e.ids[q]];

			addedEdges.AddGrow(e);
		}

		//append triangles and diagonal edges to output arrays
		for (int i = 0; i < currTris.Num(); i++) {
			Triangle t = currTris[i];
			for (int q = 0; q < 3; q++)
				t.ids[q] = remap[t.ids[q]];

			tris.AddGrow(t);
			if (i < currTris.Num() - 1) {
				//diagonal edge
				AddedEdge diag = {t.ids[2], t.ids[0]};
				addedEdges.AddGrow(diag);
			}
		}
	}
}

void PlanarGraph::Reset() {
	verts.SetNum(0, false);
	edges.SetNum(0, false);
	coedges.SetNum(0, false);
	outgoing.SetNum(0, false);
	components.SetNum(0, false);
	facets.SetNum(0, false);
	facetCoedges.SetNum(0, false);
	facetHoles.SetNum(0, false);
	_work.SetNum(0, false);
	earcut.Reset();
	optGroup = nullptr;
}

void PlanarGraph::SetOptimizeGroup(optimizeGroup_t *group) {
	optGroup = group;
}




#include "../tests/testing.h"

static const int dirs[8][2] = {{1,0}, {1,1}, {0,1}, {-1,1}, {-1,0}, {-1,-1}, {0,-1}, {1,-1}};
static void GenerateAndTest(int sx, int sy, float pFill, bool perturb, bool outputCheck, int seed) {
	idRandom rnd(seed);
	EarCutter::FailsCount = 0;

	int sz[2];
	sz[0] = sx;
	sz[1] = sy;
	int n = sz[0] * sz[1];

	//generate random field
	idList<idList<bool>> matr;
	matr.SetNum(sz[0]);
	for (int i = 0; i < sz[0]; i++) {
		matr[i].SetNum(sz[1]);
		for (int j = 0; j < sz[1]; j++)
			matr[i][j] = (rnd.RandomFloat() < pFill);
	}
#if 0
	//find connected components
	idList<int> dsu;
	idDisjointSets::Init(dsu, sz[0] * sz[1]);
	for (int i = 0; i < sz[0]; i++) {
		for (int j = 0; j < sz[1]; j++) if (matr[i][j]) {
			for (int d = 0; d < 8; d+=2) {
				int ni = i + dirs[d][0];
				int nj = j + dirs[d][1];
				if (ni < 0 || nj < 0 || ni >= sz[0] || nj >= sz[1])
					continue;
				if (!matr[ni][nj])
					continue;
				idDisjointSets::Merge(dsu, i*sz[1]+j, ni*sz[1]+nj);
			}
		}
	}
	int k = idDisjointSets::ConvertToColors(dsu);
	//ensure connectivity
	for (int i = 1; i < k; i++) {
		int from = 0, to = 0;
		while (dsu[from] != 0) from++;
		while (dsu[to] != i) to++;

		int cx = from % sz[1], cy = from / sz[1];
		int tx = to % sz[1], ty = to / sz[1];
		while (cx != tx || cy != ty) {
			matr[cx][cy] = true;
			if (cx != tx)
				cx += (tx > cx ? 1 : -1);
			else
				cy += (ty > cy ? 1 : -1);
		}
	}
#endif

	//assign vertex positions
	idList<idList<idVec2>> allPos;
	allPos.SetNum(sz[0] + 1);
	for (int i = 0; i <= sz[0]; i++) {
		allPos[i].SetNum(sz[1] + 1);
		for (int j = 0; j <= sz[1]; j++) {
			allPos[i][j] = idVec2(i, j);
			if (perturb) {
				allPos[i][j].x += rnd.CRandomFloat() * 0.3f;
				allPos[i][j].y += rnd.CRandomFloat() * 0.3f;
			}
		}
	}

	PlanarGraph graph;

	//add boundary vertices
	idList<idList<int>> active;
	idList<idVec2> actVerts;
	active.SetNum(sz[0] + 1);
	for (int i = 0; i <= sz[0]; i++) {
		active[i].SetNum(sz[1] + 1);
		for (int j = 0; j <= sz[1]; j++) {
			active[i][j] = -1;
			int cnt = 0;
			for (int d = 1; d < 8; d += 2) {
				int ni = i + (dirs[d][0]-1)/2;
				int nj = j + (dirs[d][1]-1)/2;
				bool filled = false;
				if (ni >= 0 && nj >= 0 && ni < sz[0] && nj < sz[1])
					filled = matr[ni][nj];
				cnt += int(filled);
			}
			if (cnt > 0 && cnt < 4) {
				active[i][j] = graph.AddVertex(allPos[i][j]);
				int q = actVerts.AddGrow(allPos[i][j]);
				assert(q == active[i][j]);
			}
		}
	}
	//add boundary edges
	for (int i = 0; i < sz[0]; i++)
		for (int j = 0; j <= sz[1]; j++) {
			bool prev = (j == 0 ? false : matr[i][j-1]);
			bool next = (j == sz[1] ? false : matr[i][j]);
			if (prev != next)
				graph.AddEdge(active[i][j], active[i+1][j]);
		}
	for (int i = 0; i <= sz[0]; i++)
		for (int j = 0; j < sz[1]; j++) {
			bool prev = (i == 0 ? false : matr[i-1][j]);
			bool next = (i == sz[0] ? false : matr[i][j]);
			if (prev != next)
				graph.AddEdge(active[i][j], active[i][j+1]);
		}

	graph.Finish();
	graph.BuildFaces();

	idList<PlanarGraph::Triangle> tris;
	idList<PlanarGraph::AddedEdge> edges;
	graph.TriangulateFaces(tris, edges);
	CHECK(EarCutter::FailsCount == 0);

	if (outputCheck) {
		//check that triangles don't intersect
		int errorCnt = 0;
		for (int i = 0; i < tris.Num(); i++)
			for (int j = 0; j < i; j++) {
				PlanarGraph::Triangle t[2] = {tris[i], tris[j]};
				idVec2d tv[2][3];
				for (int q = 0; q < 2; q++)
					for (int s = 0; s < 3; s++)
						tv[q][s] = idVec2d(actVerts[t[q].ids[s]]);
				//separate axis theorem: check 6 side normals
				bool separate = false;
				for (int q = 0; q < 2 && !separate; q++)
					for (int s = 0; s < 3 && !separate; s++) {
						idVec2d dir = tv[q][(s+1)%3] - tv[q][s];
						dir = idVec2d(-dir.y, dir.x);
						double interv[2][2] = {{DBL_MAX, -DBL_MAX}, {DBL_MAX, -DBL_MAX}};
						for (int p = 0; p < 2; p++)
							for (int t = 0; t < 3; t++) {
								double x = tv[p][t].Dot(dir);
								interv[p][0] = std::min(interv[p][0], x);
								interv[p][1] = std::max(interv[p][1], x);
							}
						if (interv[0][0] >= interv[1][1] - 1e-13 || interv[0][1] <= interv[1][0] + 1e-13)
							separate = true;
					}
				if (!separate)
					errorCnt++;
			}
		CHECK(errorCnt == 0);

		//check that cell centers have correct in/out status
		errorCnt = 0;
		for (int i = 0; i < sz[0]; i++)
			for (int j = 0; j < sz[1]; j++) {
				idVec2d ctr(i+0.5, j+0.5);
				bool filled = matr[i][j];
				int inTriNum = 0;
				for (int t = 0; t < tris.Num(); t++) {
					idVec2d tv[3];
					for (int u = 0; u < 3; u++)
						tv[u] = idVec2d(actVerts[tris[t].ids[u]]);
					int sgnCnt[3] = {0};
					for (int u = 0; u < 3; u++) {
						idVec2d curr = tv[u], next = tv[(u+1)%3];
						double cross = (next - curr).Cross(ctr - curr);
						int sign = (cross == 0.0 ? 0 : (cross < 0.0 ? -1 : 1));
						sgnCnt[sign+1]++;
					}
					if (sgnCnt[0] && sgnCnt[2])	//have both <0 and >0
						continue;
					inTriNum++;
				}
				if (filled && inTriNum == 0)
					errorCnt++;
				//note: empty cells are triangulated too
			}
		CHECK(errorCnt == 0);
	}
}
TEST_CASE("PlanarGraph: stress raster (1000)") {
	for (int seed = 0; seed < 1000; seed++)
		GenerateAndTest(7, 7, 0.6f, seed % 2 == 0, seed % 31 == 0, seed);
}
TEST_CASE("PlanarGraph: stress raster (infinite)"
	* doctest::skip()
) {
	//int seed = 4654373;
	for (int seed = 0; seed < INT_MAX; seed++)
		GenerateAndTest(7, 7, 0.6f, seed % 2 == 0, seed % 31 == 0, seed);
}

TEST_CASE("PlanarGraph: vertices and edges"
) {
	//checks that isolated vertices and hanging edges are handled properly
	PlanarGraph pg;
	pg.AddVertex(idVec2(0, 0));
	pg.AddVertex(idVec2(0, 5));
	pg.AddVertex(idVec2(5, 5));
	pg.AddVertex(idVec2(5, 0));
	pg.AddVertex(idVec2(2, 4));
	pg.AddVertex(idVec2(3, 4));
	pg.AddVertex(idVec2(3, 1));
	pg.AddVertex(idVec2(2, 1));
	pg.AddVertex(idVec2(1, 4));
	pg.AddVertex(idVec2(1, 3));
	pg.AddVertex(idVec2(1, 2));
	pg.AddVertex(idVec2(1, 1));
	pg.AddVertex(idVec2(4, 3));
	pg.AddVertex(idVec2(4, 2));
	pg.AddVertex(idVec2(4, 1));
	pg.AddVertex(idVec2(2.5f, 3));
	pg.AddVertex(idVec2(6, 6));
	pg.AddVertex(idVec2(6, 4));
	pg.AddVertex(idVec2(6, 1));
	pg.AddVertex(idVec2(6, 0));
	pg.AddEdge(0, 1);
	pg.AddEdge(1, 2);
	pg.AddEdge(2, 3);
	pg.AddEdge(3, 0);
	pg.AddEdge(4, 5);
	pg.AddEdge(5, 6);
	pg.AddEdge(6, 7);
	pg.AddEdge(7, 4);
	pg.AddEdge(1, 8);
	pg.AddEdge(11, 7);
	pg.AddEdge(3, 14);
	pg.AddEdge(3, 19);
	pg.AddEdge(2, 16);

	pg.Finish();
	pg.BuildFaces();
	idList<PlanarGraph::Triangle> tris;
	idList<PlanarGraph::AddedEdge> edges;
	pg.TriangulateFaces(tris, edges);

	//note: edges and vertices inside faces are included into triangulations
	//vertices and edges outside faces are left as is
	CHECK(tris.Num() == 26);
	CHECK(edges.Num() == 30);
}
