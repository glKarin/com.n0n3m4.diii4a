// HUMANHEAD nla

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "AAS_local.h"


/*
============================
idAASLocal::FindNearestPoint
============================
*/

idVec3 idAASLocal::FindNearestPoint(const idVec3 &ally, 
									const idVec3 &follower, 
									float distance) {
	hhNearPoint nearPoint(ally, follower, distance);
	idList<int> areasEntered;

	if (!file) {
		return(vec3_origin);
	}


	FindNearestPointInArea(nearPoint, 
						   PointReachableAreaNum(follower),
						   areasEntered);

	
	areasEntered.Clear();

	return(nearPoint.getBestPoint());

}		//. idAASLocal::FindNearestPoint(const idVec3 &, const idVec3 &, float)



/*
=================================
idAASLocal::FindNearestPointInArea

Returns true if a valid point was found
=================================
*/
bool idAASLocal::FindNearestPointInArea(hhNearPoint &nearPoint,
										int currentArea,
										idList<int> &areasEntered) {
	aasArea_t *area;
	findAreaType_t foundPoint;
	idReachability *reach;
	int areaNum;


	if (currentArea == 0) {
		return(false);
	}

	area = &file->areas[currentArea];

	areasEntered.Append(currentArea);

	// Check all points in the area for a better point. true if a valid
	//   point was found
	foundPoint = CheckPointsInArea(nearPoint, currentArea);

	switch (foundPoint) {
		// We are outside of the area completely
		case AREA_ALL_DESIRED:
			return(true);
			break;
		// We are still too close, try out further
		case AREA_MIXED:
			for (reach = area->reach; reach; reach = reach->next) {
				if (reach->travelType  != TFL_WALK) {
					continue;
				}

				areaNum = reach->toAreaNum;
				if (areasEntered.FindIndex(areaNum) == -1) {
					FindNearestPointInArea(nearPoint, areaNum, 
										   areasEntered);
				}
			}	//. reachable areas loop
			return(true);
			break;
		// No valid points were found in this area, return false
		case AREA_NO_VALID_POINTS:
			return(false);
			break;		
	}


	return(false);		// Should never get here. :)

}		//. idAASLocal::FindNearestPointInArea(hhNearPoint, int, int)


/*
==============================
idAASLocal::CheckPointsInArea
==============================
*/
findAreaType_t idAASLocal::CheckPointsInArea(hhNearPoint &nearPoint, 
											   int areaNum) {
	aasArea_t *area;
	aasFace_t *face;
	aasEdge_t *edge; 
	aasVertex_t *vert;
	int numFaces, firstFace, faceNum;
	int numEdges, firstEdge, edgeNum;
	int faceCounter, edgeCounter, vertCounter;
	findPointType_t pointType;
	findAreaType_t areaType;
	int pointTotal;


	areaType = AREA_NO_VALID_POINTS;
	pointTotal = 0;

	area = &file->areas[areaNum];
	numFaces = area->numFaces;
	firstFace = area->firstFace;

	for (faceCounter = 0; faceCounter < numFaces; ++faceCounter) {
		faceNum = abs(file->faceIndex[firstFace + faceCounter]);

		face = &file->faces[faceNum];
		numEdges = face->numEdges;
		firstEdge = face->firstEdge;

		for (edgeCounter = 0; edgeCounter < numEdges; ++edgeCounter) {
			edgeNum = abs(file->edgeIndex[firstEdge + edgeCounter]);
			edge = &file->edges[edgeNum];
			
			for (vertCounter = 0; vertCounter < 2; ++vertCounter) {
				vert = &file->vertices[edge->vertexNum[vertCounter]];
				
				//! Make a function				
				pointType = CheckPoint(*vert, nearPoint);
				
				pointTotal |= pointType;
			}	//. vertCounter
		}		//. edgeCounter
	}	//. faceCounter

	// If there are any just valid points, we are mixed
	if (pointTotal & POINT_VALID) {
		areaType = AREA_MIXED;
	}
	// If just desired points, we are all desired
	else if (pointTotal & POINT_DESIRED) {
		areaType = AREA_ALL_DESIRED;
	}

	return(areaType);
	

}		//. idAASLocal::CheckPointsInArea(hhNearPoint &, int)


/*
======================
idAASLocal::CheckPoint
======================
*/
findPointType_t idAASLocal::CheckPoint(idVec3 &point, 
									   hhNearPoint &nearPoint) {
	idVec3 pointDirection;
	findPointType_t pointType;
	aasTrace_t aasTrace;
	float dot;
	float lengthSq;
	bool blocked;
	bool isBetterDot;


	pointDirection = point - nearPoint.ourOrigin;
	
	pointType = POINT_NOT_VALID;

	if (pointDirection * nearPoint.direction > 0) {
		pointType = POINT_VALID;
		
		Trace(aasTrace, nearPoint.ourOrigin, point);
		blocked = aasTrace.fraction < 1.0;

		lengthSq = pointDirection.LengthSqr();

		// Out far enough
		if (lengthSq > nearPoint.desiredDistSq) {
			// If a point inside the desired dist was chosen, reset key comparisons
			if (nearPoint.bestDistSq < nearPoint.desiredDistSq) {
				nearPoint.bestDot = 0;
				nearPoint.bestBlocked = true;
			}
			
			dot = pointDirection * nearPoint.direction;

			// Is there something between dest and us
			isBetterDot = dot > nearPoint.bestDot;

			if ((!blocked && (nearPoint.bestBlocked || isBetterDot)) ||
				(blocked && nearPoint.bestBlocked && isBetterDot)) {
				nearPoint.bestPoint = point;
				nearPoint.bestDot = dot;
				nearPoint.bestBlocked = blocked;
				nearPoint.bestDistSq = lengthSq;
			}		//. Better fit for desired direction
			pointType = POINT_DESIRED;
		}	//. desired point
		else {	// Inside the circle chose the furthest non blocked point
			if (!blocked && (lengthSq > nearPoint.bestDistSq)) {
				nearPoint.bestPoint = point;
				nearPoint.bestDistSq = lengthSq;				
			}
		}		//. Inside the desired circle
	}		//. valid point


	return(pointType);

}		//. idAASLocal::CheckPoint(const idVec3 &, hhNearPoint)
