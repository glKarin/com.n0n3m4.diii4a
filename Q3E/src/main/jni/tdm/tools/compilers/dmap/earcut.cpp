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
#include "earcut.h"

#include "dmap.h"


int EarCutter::FailsCount = 0;

int EarCutter::AddVertex(const idVec2 &pos) {
	if (!loopBeingAdded) {
		Loop l;
		l.beg = l.end = verts.Num();
		loops.AddGrow(l);
		loopBeingAdded = true;
	}
	int idx = verts.AddGrow({});
	verts[idx].pos = pos;
	verts[idx].id = idx;
	return idx;
}

void EarCutter::FinishLoop(bool *isHole) {
	if (loopBeingAdded) {
		Loop &l = loops[loops.Num() - 1];
		l.end = verts.Num();
		l.isHole = -1;
		if (isHole)
			l.isHole = *isHole;
		loopBeingAdded = false;
	}
}

double EarCutter::CrossVectors(int sA, int eA, int sB, int eB) const {
	//note: the result should be exact thanks to double precision
	idVec2d a = idVec2d(verts[eA].pos) - idVec2d(verts[sA].pos);
	idVec2d b = idVec2d(verts[eB].pos) - idVec2d(verts[sB].pos);
	return a.Cross(b);
}

int EarCutter::OrientVectors(int sA, int eA, int sB, int eB) const {
	double area = CrossVectors(sA, eA, sB, eB);
	if (reversed)
		area = -area;
	return (area == 0.0 ? 0 : (area < 0.0 ? -1 : 1));
}

bool EarCutter::OutsideTriangle(int a, int b, int c, int p) const {
	if (OrientVectors(c, a, c, p) < 0)
		return true;
	if (OrientVectors(a, b, a, p) < 0)
		return true;
	if (OrientVectors(b, c, b, p) < 0)
		return true;
	return false;
}

int EarCutter::DotVectorsSign(int sA, int eA, int sB, int eB) const {
	//note: the result should be exact thanks to double precision
	idVec2d a = idVec2d(verts[eA].pos) - idVec2d(verts[sA].pos);
	idVec2d b = idVec2d(verts[eB].pos) - idVec2d(verts[sB].pos);
	double pdot = a.Dot(b);
	return (pdot == 0.0 ? 0 : (pdot < 0.0 ? -1 : 1));
}

void EarCutter::RemoveReflex(int i) {
	//swap with last reflex vertex, then pop it
	int q = verts[i].inReflex, l = reflexIds.Num() - 1;
	if (q < 0)
		return;
	idSwap(reflexIds[q], reflexIds[l]);
	idSwap(verts[reflexIds[q]].inReflex, verts[reflexIds[l]].inReflex);
	verts[reflexIds[l]].inReflex = -1;
	reflexIds.Pop();
}

void EarCutter::RemoveEar(int i) {
	earIds.Remove(i);
	verts[i].isEar = false;
}

bool EarCutter::UpdateReflex(int i) {
	int vp = verts[i].prev;
	int vn = verts[i].next;
	int ori = OrientVectors(i, vp, i, vn);
	//note: 180 degrees is reflex too (can happen after some ears cut)
	bool newReflex = (ori >= 0);

	if (ori == 0 && DotVectorsSign(i, vp, i, vn) > 0) {
		//idea from FIST: distinguish between angle = 0 (convex) and angle = 360 (reflex)
		for (int vx = vn; vx != vp; vx = verts[vx].next) {
			int xori = OrientVectors(i, vp, i, vx);
			if (xori != 0) {
				newReflex = (xori > 0);
				break;
			}
		}
	}

	if ((verts[i].inReflex >= 0) == newReflex)
		return false;	//same

	if (verts[i].inReflex >= 0) {
		//delete from reflex list
		RemoveReflex(i);
	}
	else {
		//add to reflex list
		int q = reflexIds.AddGrow(i);
		verts[i].inReflex = q;
	}
	return true;	//changed
}

bool EarCutter::UpdateEar(int i) {
	bool newEar = false;
	//only convex vertex can be ear
	if (verts[i].inReflex < 0) {
		//check if some reflex verts is inside potential ear
		newEar = true;
		for (int q = 0; q < reflexIds.Num(); q++) {
			int a = verts[i].prev, b = i, c = verts[i].next;
			int v = reflexIds[q];
			if (verts[v].pos == verts[a].pos)
				continue;
			if (verts[v].pos == verts[b].pos)
				continue;
			if (verts[v].pos == verts[c].pos)
				continue;
			if (!OutsideTriangle(a, b, c, v)) {
				newEar = false;
				break;
			}
		}
	}

	if (verts[i].isEar == newEar) {
		if (newEar) {
			//update length
			float lenSq = (verts[verts[i].next].pos - verts[verts[i].prev].pos).LengthSqr();
			earIds.Update(i, lenSq);
		}
		return false;	//same
	}

	if (verts[i].isEar) {
		//delete from ear heap
		RemoveEar(i);
	}
	else {
		//add to ear heap
		float lenSq = (verts[verts[i].next].pos - verts[verts[i].prev].pos).LengthSqr();
		earIds.Add(lenSq, &i);
		verts[i].isEar = true;
	}

	return true;	//changed
}

void EarCutter::CutEars() {
	int n = verts.Num();

	first = 0;
	for (int i = 0; i < n; i++) {
		verts[i].next = (i+1 + n) % n;
		verts[i].prev = (i-1 + n) % n;
		verts[i].inReflex = -1;
		verts[i].isEar = false;
	}

	for (int i = 0; i < n; i++)
		UpdateReflex(i);
	for (int i = 0; i < n; i++)
		UpdateEar(i);

	bool failReported = false;
	for (int iter = 0; iter < n - 2; iter++) {
		int ear = -1;

		if (earIds.Num() > 0) {
			//normal case: choose best ear to cut
			ear = earIds.GetMin();
		}
		else {
			//in some rare cases algorithm inevitably breaks =(
			if (!failReported) {
				FailsCount++;
				idVec2 somePos;
				idVec2 zone = EstimateErrorZone(&somePos);
				//usually algorithm breaks on (almost)singular contours
				//the unfilled part of triangulation cannot be thicker than the bounding box of remaining part
				//so let's better suppress the warning in thin cases, when width is less than 1 unit
				if (idMath::Fmin(zone.x, zone.y) > 1.0f)
					common->Printf(
						"EarCutter: no more ears after %d/%d iterations (zone %0.3lf x %0.3lf) near (%s)\n",
						iter, n-2, zone.x, zone.y,
						ReportWorldPositionInOptimizeGroup(somePos, optGroup).c_str()
					);
				failReported = true;
			}
			//"desperate mode" from FIST: pick any remaining vertex
			int bestIdx = -1;
			double bestScore = DBL_MAX;
			for (int i = first; ; i = verts[i].next) {
				int vp = verts[i].prev;
				int vn = verts[i].next;
				double lp = (verts[i].pos - verts[vn].pos).LengthSqr();
				double ln = (verts[i].pos - verts[vp].pos).LengthSqr();
				double ld = (verts[vp].pos - verts[vn].pos).LengthSqr();
				//first try to get rid of triangles with zero-length side
				bool reflex = (verts[i].inReflex >= 0);
				double score = idMath::Fmin(lp, ln, ld) * (reflex ? 10 : 1);
				if (bestScore > score) {
					bestScore = score;
					bestIdx = i;
				}
				if (vn == first)
					break;
			}
			//cut it as ear
			ear = bestIdx;
		}

		//add ear triangle
		Triangle t = {verts[ear].prev, ear, verts[ear].next};
		tris.AddGrow(t);

		//remove ear vertex from polygon
		int vprev = verts[ear].prev;
		int vnext = verts[ear].next;
		if (first == ear)
			first = vnext;
		verts[vnext].prev = vprev;
		verts[vprev].next = vnext;
		//unlink vertex from lists
		RemoveReflex(ear);
		if (verts[ear].isEar)
			RemoveEar(ear);
		//fill vertex with obvious garbage
		verts[ear].next = verts[ear].prev = -1;
		verts[ear].inReflex = 0xCCCCCCCC;

		//update two neighboring vertices
		UpdateReflex(vprev);
		UpdateReflex(vnext);
		UpdateEar(vprev);
		UpdateEar(vnext);
	}
	//line segment connecting two vertices should remain

	//remap vertex indices back to their original ids
	for (int i = 0; i < tris.Num(); i++)
		for (int q = 0; q < 3; q++) {
			int &v = tris[i].ids[q];
			v = verts[v].id;
		}
}

idVec2 EarCutter::EstimateErrorZone(idVec2 *somePos) const {
	int n = verts.Num();
	//find leftmost vertex
	idVec2 leftPos(FLT_MAX, FLT_MAX);
	int leftIdx = -1;
	for (int i = 0; i < n; i++) if (verts[i].next >= 0) {
		const idVec2 &p = verts[i].pos;
		if (p.x < leftPos.x || p.x == leftPos.x && p.y < leftPos.y) {
			leftPos = p;
			leftIdx = i;
		}
	}
	assert(leftIdx >= 0);
	//find most-distant vertex from it
	float farDist = -FLT_MAX;
	int farIdx = -1;
	for (int i = 0; i < n; i++) if (verts[i].next >= 0) {
		const idVec2 &p = verts[i].pos;
		float dist = (p - leftPos).Length();
		if (farDist < dist) {
			farDist = dist;
			farIdx = i;
		}
	}
	assert(farIdx >= 0);
	//align X axis along this segment
	idVec2 axisX = verts[farIdx].pos - leftPos;
	if (farDist <= 1e-3f)
		axisX.Set(1.0f, 0.0f);
	axisX.Normalize();
	idVec2 axisY(-axisX.y, axisX.x);
	//estimate aligned bounding box of remaining vertices
	idBounds bbox;
	bbox.Clear();
	for (int i = 0; i < n; i++) if (verts[i].next >= 0) {
		const idVec2 &p = verts[i].pos;
		idVec3 local(p * axisX, p * axisY, 0.0f);
		bbox.AddPoint(local);
	}
	//return values
	if (somePos)
		*somePos = leftPos;
	return bbox.GetSize().ToVec2();
}

void EarCutter::ConnectHoles() {
	int k = loops.Num();
	if (k == 1)
		return;

	//detect hole loops
	int outerCnt = 0;
	for (int i = 0; i < k; i++) {
		Loop &l = loops[i];
		if (l.isHole < 0) {
			//not specified by caller: try to determine it
			if (l.end - l.beg <= 2) {
				l.isHole = true;
				assert(k > 1);
			}
			else {
				l.isHole = (l.area < 0.0) != reversed;
			}
		}
		outerCnt += !l.isHole;
	}
	//note: multiple outer loops not supported!
	assert(outerCnt == 1);

	//detect rightmost vertex in each hole
	for (int i = 0; i < k; i++) {
		Loop &l = loops[i];
		if (!l.isHole) {
			l.rightmost = -1;
			continue;
		}
		idVec2 bestPoint(-FLT_MAX, -FLT_MAX);
		l.rightmost = -1;
		for (int j = l.beg; j < l.end; j++) {
			const Vertex &v = verts[j];
			if (v.pos.x > bestPoint.x || v.pos.x == bestPoint.x && v.pos.y > bestPoint.y) {
				bestPoint = v.pos;
				l.rightmost = j;
			}
		}
	}

	//sort holes by rightmost X decreasing
	std::sort(loops.begin(), loops.end(), [this](const Loop &a, const Loop &b) -> bool {
		if (a.isHole != b.isHole)
			return int(a.isHole) < int(b.isHole);	//outer loop first
		const idVec2 &pa = verts[a.rightmost].pos;
		const idVec2 &pb = verts[b.rightmost].pos;
		if (pa.x != pb.x)
			return pa.x > pb.x;
		if (pa.y != pb.y)
			return pa.y > pb.y;
		return false;
	});

	//sequence of vertices in the outer loop
	//it will grow gradually, possible including same vertex several times
	_work1.SetNum(0, false);
	idList<int> &sequence = _work1;
	assert(!loops[0].isHole);
	for (int j = loops[0].beg; j < loops[0].end; j++)
		sequence.AddGrow(j);

	//merge holes into outer loop one by one
	for (int i = 1; i < k; i++) {
		const Loop &l = loops[i];

		//find first intersection of horizontal ray with outer polygon
		int si = l.rightmost;
		idVec2 s = verts[si].pos;
		double minDeltaX = DBL_MAX;
		int majorIdx = -1, minorIdx = -1;
		for (int t = 0; t < sequence.Num(); t++) {
			int ai = sequence[t];
			int bi = sequence[t+1 < sequence.Num() ? t+1 : 0];
			const idVec2 &a = verts[ai].pos;
			const idVec2 &b = verts[bi].pos;

			if (a.y > s.y && b.y > s.y)
				continue;
			if (a.y < s.y && b.y < s.y)
				continue;
			if (a.x < s.x && b.x < s.x)
				continue;

			//note: no rounding errors here (yet)
			double numer = CrossVectors(si, ai, si, bi);
			double denom = double(b.y) - double(a.y);
			int iMaj = -1, iMin = -1;

			if (denom == 0.0) {
				//ray goes along this edge
				assert(a.y == s.y && b.y == s.y);	//equivalent to denom == 0.0
				assert(a.x > s.x && b.x > s.x);		//hole and outer polygon have common point!
				denom = 1.0;
				if (a.x < b.x) {
					numer = a.x - s.x;
					iMaj = iMin = ai;
				}
				else {
					numer = b.x - s.x;
					iMaj = iMin = bi;
				}
			}
			else {
				if (a.x > b.x) {
					iMaj = ai;
					iMin = bi;
				}
				else {
					iMaj = bi;
					iMin = ai;
				}
			}

			//rounding errors may happen here =(
			double deltaX = numer / denom;
			if (deltaX < 0.0)
				continue;

			if (minDeltaX > deltaX) {
				minDeltaX = deltaX;
				majorIdx = iMaj;
				minorIdx = iMin;
			}
		}
		assert(majorIdx >= 0);	//hole not inside outer loop
		assert(minDeltaX >= 0.0);

		int conn = -1;
		idVec2 pMaj = verts[majorIdx].pos;
		idVec2 pMin = verts[minorIdx].pos;
		if (pMaj.y == s.y)
			conn = majorIdx;	//vertex hit
		else if (pMin.y == s.y)
			conn = minorIdx;	//vertex hit
		else {
			//ray hits edge
			assert((pMaj.y < s.y && pMin.y > s.y) || (pMaj.y > s.y && pMin.y < s.y));
			assert(pMaj.x > s.x);
			conn = majorIdx;
			//find vertex inside triangle S - Maj - Inters
			//which is closest to the casted ray (by angle)
			for (int t = 0; t < sequence.Num(); t++) {
				int j = sequence[t];
				const idVec2 &v = verts[j].pos;
				if (v.x <= s.x)
					continue;
				//check side relative to horizontal ray
				if (pMaj.y > s.y && v.y <= s.y)
					continue;
				if (pMaj.y < s.y && v.y >= s.y)
					continue;
				//check side relative to the edge hit
				double sCross = CrossVectors(majorIdx, si, majorIdx, minorIdx);
				double vCross = CrossVectors(majorIdx, j, majorIdx, minorIdx);
				assert(sCross != 0.0);
				if (sCross < 0.0 && vCross >= 0.0)
					continue;
				if (sCross > 0.0 && vCross <= 0.0)
					continue;
				//check if closer to ray by angle
				double relCross = CrossVectors(si, conn, si, j);
				if (pMaj.y > s.y && relCross > 0.0)
					continue;
				if (pMaj.y < s.y && relCross < 0.0)
					continue;
				//save better vertex
				conn = j;
			}
		}

		//found mutually visible vertex pair
		SeamEdge seam = {si, conn};
		seams.AddGrow(seam);

		//the second vertex can occur several times
		//(even due to loop containing same vertex twice)
		//find such occurence seam edge is within angle
		int posS = -1;
		for (int i = 0; i < sequence.Num(); i++) {
			if (verts[sequence[i]].pos != verts[conn].pos)
				continue;
			int curr = sequence[i];
			int prev = sequence[(i ? i : sequence.Num()) - 1];
			int next = sequence[i + 1 < sequence.Num() ? i + 1 : 0];
			struct TaggedVec {
				idVec2d v;
				int tag;
			};
			TaggedVec arr[3] = {
				TaggedVec{idVec2d(verts[next].pos) - idVec2d(verts[curr].pos), 0},	//N
				TaggedVec{idVec2d(verts[si].pos) - idVec2d(verts[curr].pos), 1},	//S
				TaggedVec{idVec2d(verts[prev].pos) - idVec2d(verts[curr].pos), 2}	//P
			};
			//sort by polar angle
			for (int u = 0; u < 3; u++)
				for (int v = 0; v < u; v++)
					if (idVec2d::PolarAngleCompare(arr[u].v, arr[v].v) < 0)
						idSwap(arr[u], arr[v]);
			//switch to CW order if face is reversed
			if (reversed)
				idSwap(arr[0], arr[2]);
			//vectors must go in cyclic order [N, S, P]
			bool inside = true;
			for (int u = 0; u < 2; u++)
				if (arr[u+1].tag != (arr[u].tag + 1) % 3) {
					inside = false;
					break;
				}
			//record position if order is correct
			if (!inside)
				continue;
			assert(posS < 0);
			posS = i;
		}
		if (posS < 0) {
			//normally, this should never happen
			//but at least we should not crash in this case
			common->Printf(
				"EarCutter: failed to connect inner loop of area %0.3lf near (%s)\n",
				l.area, ReportWorldPositionInOptimizeGroup(verts[sequence[0]].pos, optGroup).c_str()
			);
			FailsCount++;
			continue;	//drop this inner loop
		}

		//connect outer polygon with hole along seam
		int posH = si - l.beg;
		_work2.SetNum(0, false);
		idList<int> &newSeq = _work2;
		for (int j = 0; j <= posS; j++)
			newSeq.AddGrow(sequence[j]);
		int tk = l.end - l.beg;
		if (tk > 1) tk++;
		for (int j = posH; j < posH + tk; j++)
			newSeq.AddGrow(l.beg + j % (l.end - l.beg));
		for (int j = posS; j < sequence.Num(); j++)
			newSeq.AddGrow(sequence[j]);
		sequence.Swap(newSeq);
	}


	//there is one loop now, this array makes no sense anymore
	loops.SetNum(0, false);

	//rebuild vertices array
	idList<Vertex> &newVerts = _work3;
	newVerts.SetNum(0, false);
	for (int j = 0; j < sequence.Num(); j++)
		newVerts.AddGrow(verts[sequence[j]]);
	verts.Swap(newVerts);
}

void EarCutter::DetectOrientation() {
	//compute total area
	double totalArea = 0.0;
	for (int i = 0; i < loops.Num(); i++) {
		Loop &l = loops[i];
		l.area = 0.0;
		for (int j = l.beg; j < l.end; j++) {
			int nj = j + 1;
			if (nj == l.end) nj = l.beg;
			l.area += CrossVectors(0, j, 0, nj);
		}
		totalArea += l.area;
	}
	//detect orientation (is outer loop clockwise?)
	reversed = (totalArea < 0.0);
}

void EarCutter::Triangulate() {
	FinishLoop();

	DetectOrientation();
	ConnectHoles();
	CutEars();
}

void EarCutter::Reset() {
	loopBeingAdded = false;
	loops.SetNum(0, false);
	verts.SetNum(0, false);
	first = -1;
	reflexIds.SetNum(0, false);
	earIds.Clear();
	tris.SetNum(0, false);
	seams.SetNum(0, false);
	optGroup = nullptr;
}

void EarCutter::SetOptimizeGroup(optimizeGroup_t *group) {
	optGroup = group;
}
