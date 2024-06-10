// SimpleCulling.cpp

#include "RenderSystem_local.h"

/*
=================
R_RadiusCullLocalBox

A fast, conservative center-to-corner culling test
Returns true if the box is outside the given global frustum, (positive sides are out)
=================
*/
bool R_RadiusCullLocalBox(const idBounds& bounds, const float modelMatrix[16], int numPlanes, const idPlane* planes) {
	int			i;
	float		d;
	idVec3		worldOrigin;
	float		worldRadius;
	const idPlane* frust;

	if (r_useCulling.GetInteger() == 0) {
		return false;
	}

	// transform the surface bounds into world space
	idVec3	localOrigin = (bounds[0] + bounds[1]) * 0.5;

	R_LocalPointToGlobal(modelMatrix, localOrigin, worldOrigin);

	worldRadius = (bounds[0] - localOrigin).Length();	// FIXME: won't be correct for scaled objects

	for (i = 0; i < numPlanes; i++) {
		frust = planes + i;
		d = frust->Distance(worldOrigin);
		if (d > worldRadius) {
			return true;	// culled
		}
	}

	return false;		// no culled
}

/*
=================
R_CornerCullLocalBox

Tests all corners against the frustum.
Can still generate a few false positives when the box is outside a corner.
Returns true if the box is outside the given global frustum, (positive sides are out)
=================
*/
bool R_CornerCullLocalBox(const idBounds& bounds, const float modelMatrix[16], int numPlanes, const idPlane* planes) {
	int			i, j;
	idVec3		transformed[8];
	float		dists[8];
	idVec3		v;
	const idPlane* frust;

	// we can disable box culling for experimental timing purposes
	if (r_useCulling.GetInteger() < 2) {
		return false;
	}

	// transform into world space
	for (i = 0; i < 8; i++) {
		v[0] = bounds[i & 1][0];
		v[1] = bounds[(i >> 1) & 1][1];
		v[2] = bounds[(i >> 2) & 1][2];

		R_LocalPointToGlobal(modelMatrix, v, transformed[i]);
	}

	// check against frustum planes
	for (i = 0; i < numPlanes; i++) {
		frust = planes + i;
		for (j = 0; j < 8; j++) {
			dists[j] = frust->Distance(transformed[j]);
			if (dists[j] < 0) {
				break;
			}
		}
		if (j == 8) {
			// all points were behind one of the planes
			tr.pc.c_box_cull_out++;
			return true;
		}
	}

	tr.pc.c_box_cull_in++;

	return false;		// not culled
}

/*
=================
R_CullLocalBox

Performs quick test before expensive test
Returns true if the box is outside the given global frustum, (positive sides are out)
=================
*/
bool R_CullLocalBox(const idBounds& bounds, const float modelMatrix[16], int numPlanes, const idPlane* planes) {
	if (R_RadiusCullLocalBox(bounds, modelMatrix, numPlanes, planes)) {
		return true;
	}
	return R_CornerCullLocalBox(bounds, modelMatrix, numPlanes, planes);
}