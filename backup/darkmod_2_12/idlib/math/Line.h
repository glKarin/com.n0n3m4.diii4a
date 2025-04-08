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

#ifndef __LINE_H__
#define __LINE_H__

#include "Vector.h"

//returns (1/dx, 1/dy, 1/dz) vector for movement
idVec3 GetInverseMovementVelocity(const idVec3 &start, const idVec3 &end);

bool MovingBoundsIntersectBounds(
	//moving bounds: center for t = 0, velocity for t = [0..1], extent
	const idVec3 &startPosition, const idVec3 &invVelocity, const idVec3 &extent,
	//other bounds (standing still)
	const idBounds &objBounds,
	//in  [L,R]: time may vary as L <= t <= R    (usually L=0, R=1)
	//out [L,R]: common points exist during L <= t <= R
  float paramsRange[2]
);

#endif
