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
#include "Line.h"

idVec3 GetInverseMovementVelocity(const idVec3 &start, const idVec3 &end) {
	auto inverse = [](float x) -> float{
		//note: we change axis-aligned directions by at most 1e-10
		static const float MinDelta = 1e-10f;
		if (idMath::Fabs(x) < MinDelta)
			x = x < 0.0f ? -MinDelta : MinDelta;
		return 1.0f / x;
	};

	idVec3 vel = end - start;
	assert(vel.LengthSqr() > 1e-10f);
	idVec3 invVel(inverse(vel.x), inverse(vel.y), inverse(vel.z));
	return invVel;
}

bool MovingBoundsIntersectBounds(
	const idVec3 &startPosition, const idVec3 &invVelocity, const idVec3 &extent,
	const idBounds &objBounds,
  float paramsRange[2]
) {
	assert(!objBounds.IsBackwards());
	idVec3 pmin = objBounds[0] - extent;
	idVec3 pmax = objBounds[1] + extent;
	idVec3 tmin = (pmin - startPosition);
	idVec3 tmax = (pmax - startPosition);
	tmin.MulCW(invVelocity);
	tmax.MulCW(invVelocity);
	for (int d = 0; d < 3; d++) {
		float a = tmin[d], b = tmax[d];
		tmin[d] = idMath::Fmin(a, b);
		tmax[d] = idMath::Fmax(a, b);
	}
	float scaleMin = idMath::Fmax(idMath::Fmax(tmin.x, tmin.y), idMath::Fmax(tmin.z, paramsRange[0]));
	float scaleMax = idMath::Fmin(idMath::Fmin(tmax.x, tmax.y), idMath::Fmin(tmax.z, paramsRange[1]));
	paramsRange[0] = scaleMin;
	paramsRange[1] = scaleMax;
	return scaleMin <= scaleMax;
}
