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



// #include "math.h"
#include "DarkModGlobals.h"
#include "Intersection.h"

// grayman #3584 - use this version for illumination purposes
EIntersection IntersectLinesegmentLightEllipsoid(const idVec3 Segment[LSG_COUNT], 
								 const idVec3 Ellipsoid[ELL_COUNT],
								 idVec3 Contained[2], bool bInside[LSG_COUNT])
{
	EIntersection rc = INTERSECT_COUNT;
	float fRoot;
	float fInvA;
    float afT[2] = { 0.0, 0.0 }; // OrbWeaver: "may be used uninitialised" warning
	float riQuantity;

	//stgatilov: don't intersect with singular ellipsoid (creates Inf/Nan)
	if (Ellipsoid[ELA_AXIS].x == 0 || Ellipsoid[ELA_AXIS].y == 0 || Ellipsoid[ELA_AXIS].z == 0)
		return INTERSECT_OUTSIDE;

    // set up quadratic Q(t) = a*t^2 + 2*b*t + c
	idMat3 A(1/(Ellipsoid[ELA_AXIS].x*Ellipsoid[ELA_AXIS].x), 0, 0,
		0, 1/(Ellipsoid[ELA_AXIS].y*Ellipsoid[ELA_AXIS].y), 0,
		0, 0, 1/(Ellipsoid[ELA_AXIS].z*Ellipsoid[ELA_AXIS].z));

	idVec3 EllOrigin = Ellipsoid[ELL_ORIGIN] + Ellipsoid[ELA_CENTER]; // true origin of ellipse
    idVec3 kDiff = Segment[LSG_ORIGIN] - EllOrigin;
    idVec3 kMatDir = A * Segment[LSG_DIRECTION];
    idVec3 kMatDiff = A * kDiff;
    float fA = Segment[LSG_DIRECTION]* kMatDir;
    float fB = Segment[LSG_DIRECTION] * kMatDiff;
    float fC = kDiff * kMatDiff - 1.0;
	bInside[0] = false;
	bInside[1] = false;

	// grayman #3584 - test to see if either or both points are inside the ellipsoid
	// first point

	float rX = (Segment[LSG_ORIGIN].x - EllOrigin.x)/Ellipsoid[ELA_AXIS].x;
	float rY = (Segment[LSG_ORIGIN].y - EllOrigin.y)/Ellipsoid[ELA_AXIS].y;
	float rZ = (Segment[LSG_ORIGIN].z - EllOrigin.z)/Ellipsoid[ELA_AXIS].z;
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rX = %f/%f = %f, rX^2 = %f\r",Segment[LSG_ORIGIN].x - EllOrigin.x,Ellipsoid[ELA_AXIS].x,rX,rX*rX);
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rY = %f/%f = %f, rY^2 = %f\r",Segment[LSG_ORIGIN].y - EllOrigin.y,Ellipsoid[ELA_AXIS].y,rY,rY*rY);
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rZ = %f/%f = %f, rZ^2 = %f\r",Segment[LSG_ORIGIN].z - EllOrigin.z,Ellipsoid[ELA_AXIS].z,rZ,rZ*rZ);
	if ( (rX*rX + rY*rY + rZ*rZ) <= 1 )
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("P1 [%s] is on/inside ellipsoid\r",Segment[LSG_ORIGIN].ToString());
	}
	else
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("P1 [%s] is outside ellipsoid\r",Segment[LSG_ORIGIN].ToString());
	}
	// second point
	rX = (Segment[LSG_ORIGIN].x + Segment[LSG_DIRECTION].x - EllOrigin.x)/Ellipsoid[ELA_AXIS].x;
	rY = (Segment[LSG_ORIGIN].y + Segment[LSG_DIRECTION].y - EllOrigin.y)/Ellipsoid[ELA_AXIS].y;
	rZ = (Segment[LSG_ORIGIN].z + Segment[LSG_DIRECTION].z - EllOrigin.z)/Ellipsoid[ELA_AXIS].z;
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rX = %f/%f = %f, rX^2 = %f\r",Segment[LSG_ORIGIN].x + Segment[LSG_DIRECTION].x - EllOrigin.x,Ellipsoid[ELA_AXIS].x,rX,rX*rX);
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rY = %f/%f = %f, rY^2 = %f\r",Segment[LSG_ORIGIN].y + Segment[LSG_DIRECTION].y - EllOrigin.y,Ellipsoid[ELA_AXIS].y,rY,rY*rY);
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rZ = %f/%f = %f, rZ^2 = %f\r",Segment[LSG_ORIGIN].z + Segment[LSG_DIRECTION].z - EllOrigin.z,Ellipsoid[ELA_AXIS].z,rZ,rZ*rZ);
	if ( (rX*rX + rY*rY + rZ*rZ) <= 1 )
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("P2 [%s] is on/inside ellipsoid\r",(Segment[LSG_ORIGIN]+ Segment[LSG_DIRECTION]).ToString());
	}
	else
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("P2 [%s] is outside ellipsoid\r",(Segment[LSG_ORIGIN]+ Segment[LSG_DIRECTION]).ToString());
	}

	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineSegStart", Segment[LSG_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineSegDirection", Segment[LSG_DIRECTION]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllOrigin", Ellipsoid[ELL_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Actual Origin", EllOrigin);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllCenter", Ellipsoid[ELA_CENTER]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllAxis", Ellipsoid[ELA_AXIS]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Diff", kDiff);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDir", kMatDir);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDiff", kMatDiff);

    // no intersection if Q(t) has no real roots
    float fDiscr = fB*fB - fA*fC; // discriminant
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("fA: %f   fB: %f   fC: %f   fDiscr: %f\r", fA, fB, fC, fDiscr);
    if ( fDiscr < 0.0 )
    {
		// The line segment is outside the ellipsoid
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("1 - Found no intersections\r");
		rc = INTERSECT_OUTSIDE;
        riQuantity = 0;
		bInside[0] = false;
		bInside[1] = false;
    }
    else if ( fDiscr > 0.0 )
    {
		// There are two points of intersection. Three conditions:
		//
		// 1 - both points are inside
		// 2 - one point is inside
		// 3 - both points are outside

		fRoot = idMath::Sqrt64(fDiscr);
        fInvA = 1.0/fA;

		// afT[N] is the distance you travel from the line segment
		// origin Segment[LSG_ORIGIN], along the direction of the
		// line segment Segment[LSG_DIRECTION], to get to intersecting point N.

        afT[0] = (-fB - fRoot)*fInvA;
        afT[1] = (-fB + fRoot)*fInvA;

		// From here on, "intersection" means the ellipsoid intersects the line segment itself,
		// not that extensions of the line segment in either direction intersect the ellipsoid.

		if ( afT[0] < 0.0 )
		{
			if ( afT[1] < 0.0 )
			{
				// Point 0 is behind the line segment origin.
				// Point 1 is behind the line segment origin.
				// Therefore, the line segment is outside the ellipsoid.
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("2 - Found no intersections\r");
				rc = INTERSECT_OUTSIDE;
				riQuantity = 0;
				bInside[0] = false; // the line origin is outside
				bInside[1] = false; // the line end is outside
			}
			else if ( ( afT[1] >= 0.0 ) && ( afT[1] <= 1.0 ) )
			{
				// Point 0 is behind the line segment origin.
				// Point 1 is in front of the line segment origin, and lies on the segment.
				// Therefore, the line intersects the ellipsoid at Point 1 only.
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("3 - Found one intersection\r");
				rc = INTERSECT_PARTIAL;
				riQuantity = 1;

                Contained[0] = Segment[LSG_ORIGIN]+afT[1]*Segment[LSG_DIRECTION]; // Point 1
				bInside[0] = true;  // the line origin is inside
				bInside[1] = false; // the line end is outside
			}
			else // afT[1] > 1.0
			{
				// Point 0 is behind the line segment origin.
				// Point 1 is in front of the line segment origin, beyond its end.
				// Therefore, the line segment is entirely inside.
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("4 - no intersections, line segment inside\r");
				rc = INTERSECT_NONE;
                riQuantity = 0;
				bInside[0] = true; // the line origin is inside
				bInside[1] = true; // the line end is inside
			}
		}
		else if ( ( afT[0] >= 0.0 ) && ( afT[0] <= 1.0 ) )
		{
			if ( afT[1] < 0.0 )
			{
				// Point 0 is in front of the line segment origin, and lies on the segment.
				// Point 1 is behind the line segment origin.
				// Therefore, the line intersects the ellipsoid at Point 0 only.
				// (We should never encounter this case.)
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("5 - Found one intersection\r");
				rc = INTERSECT_PARTIAL;
				riQuantity = 1;

                Contained[0] = Segment[LSG_ORIGIN]+afT[0]*Segment[LSG_DIRECTION];
				bInside[0] = true;  // the line origin is inside
				bInside[1] = false; // the line end is outside
			}
			else if ( ( afT[1] >= 0.0 ) && ( afT[1] <= 1.0 ) )
			{
				// Point 0 is in front of the line segment origin, and lies on the segment.
				// Point 1 is in front of the line segment origin, and lies on the segment.
				// Therefore, the line segment passes entirely through the ellipsoid.
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("6 - found 2 intersections\r");
				rc = INTERSECT_FULL;
                riQuantity = 2;
                Contained[0] = Segment[LSG_ORIGIN]+afT[0]*Segment[LSG_DIRECTION];
                Contained[1] = Segment[LSG_ORIGIN]+afT[1]*Segment[LSG_DIRECTION];
				bInside[0] = false; // the line origin is outside
				bInside[1] = false; // the line end is outside
			}
			else // afT[1] > 1.0
			{
				// Point 0 is in front of the line segment origin, and lies on the segment.
				// Point 1 is in front of the line segment origin, beyond its end.
				// Therefore, the line intersects the ellipsoid at Point 0 only.
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("7 - Found one intersection\r");
				rc = INTERSECT_PARTIAL;
				riQuantity = 1;

                Contained[0] = Segment[LSG_ORIGIN]+afT[0]*Segment[LSG_DIRECTION];
				bInside[0] = false; // the line origin is outside
				bInside[1] = true;  // the line end is inside
			}
		}
		else // afT[0] > 1.0
		{
			// Point 0 is in front of the line segment origin, beyond its end.
			// The line segment is outside the ellipsoid
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("8 - Found no intersections\r");
			rc = INTERSECT_OUTSIDE;
			riQuantity = 0;
			bInside[0] = false; // the line origin is outside
			bInside[1] = false; // the line end is outside
		}
	}
    else // fDiscr == 0.0, the line segment touches the ellipsoid at a single point
    {
        afT[0] = -fB/fA;
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("9 - Found one intersection\r");
		rc = INTERSECT_PARTIAL; // grayman #3584
        riQuantity = 1;
        Contained[0] = Segment[LSG_ORIGIN]+afT[0]*Segment[LSG_DIRECTION];
		bInside[0] = false; // the line origin is outside
		bInside[1] = false; // the line end is outside
    }

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("afT[0]: %f   afT[1]: %f\r", afT[0], afT[1]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[0]", Contained[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[1]", Contained[1]);

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rc: %u   riQuantity %f\r", rc, riQuantity);

	return rc;
}

EIntersection IntersectLinesegmentEllipsoid(const idVec3 Segment[LSG_COUNT], 
								 const idVec3 Ellipsoid[ELL_COUNT],
								 idVec3 Contained[2], bool bInside[LSG_COUNT])
{
	EIntersection rc = INTERSECT_COUNT;
	float fRoot;
	float fInvA;
    float afT[2] = { 0.0, 0.0 }; // OrbWeaver: "may be used uninitialised" warning
	float riQuantity;

    // set up quadratic Q(t) = a*t^2 + 2*b*t + c
	idMat3 A(1/(Ellipsoid[ELA_AXIS].x*Ellipsoid[ELA_AXIS].x), 0, 0,
		0, 1/(Ellipsoid[ELA_AXIS].y*Ellipsoid[ELA_AXIS].y), 0,
		0, 0, 1/(Ellipsoid[ELA_AXIS].z*Ellipsoid[ELA_AXIS].z));

    idVec3 kDiff = Segment[LSG_ORIGIN] - Ellipsoid[ELL_ORIGIN];
    idVec3 kMatDir = A * Segment[LSG_DIRECTION];
    idVec3 kMatDiff = A * kDiff;
    float fA = Segment[LSG_DIRECTION]* kMatDir;
    float fB = Segment[LSG_DIRECTION] * kMatDiff;
    float fC = kDiff * kMatDiff - 1.0;
	bInside[0] = false;
	bInside[1] = false;

	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineSegStart", Segment[LSG_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineSegDirection", Segment[LSG_DIRECTION]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllOrigin", Ellipsoid[ELL_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllCenter", Ellipsoid[ELA_CENTER]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllAxis", Ellipsoid[ELA_AXIS]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Diff", kDiff);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDir", kMatDir);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDiff", kMatDiff);

    // no intersection if Q(t) has no real roots
    float fDiscr = fB*fB - fA*fC;
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("fA: %f   fB: %f   fC: %f   fDiscr: %f\r", fA, fB, fC, fDiscr);
    if(fDiscr < 0.0)
    {
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("1 - Found no intersections\r");
		rc = INTERSECT_OUTSIDE;
        riQuantity = 0;
    }
    else if(fDiscr > 0.0)
    {
		fRoot = idMath::Sqrt64(fDiscr);
        fInvA = 1.0/fA;
        afT[0] = (-fB - fRoot)*fInvA;
        afT[1] = (-fB + fRoot)*fInvA;

		if(afT[0] > 1.0 || afT[1] < 0.0)
		{
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("2 - Found no intersections\r");
			rc = INTERSECT_NONE;
            riQuantity = 0;
		}
        else if(afT[0] >= 0.0)
        {
            if(afT[1] > 1.0)
            {
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("3 - Found one intersection\r");
				rc = INTERSECT_PARTIAL;
				riQuantity = 1;

                Contained[0] = Segment[LSG_ORIGIN]+afT[0]*Segment[LSG_DIRECTION];
            }
            else
            {
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("4 - Found two intersections\r");
				rc = INTERSECT_FULL;
                riQuantity = 2;
                Contained[0] = Segment[LSG_ORIGIN]+afT[0]*Segment[LSG_DIRECTION];
                Contained[1] = Segment[LSG_ORIGIN]+afT[1]*Segment[LSG_DIRECTION];
            }
        }
        else  // afT[1] >= 0
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("5 - Found one intersection\r");
			rc = INTERSECT_PARTIAL;
			riQuantity = 1;
			Contained[0] = Segment[LSG_ORIGIN]+afT[1]*Segment[LSG_DIRECTION];
        }
    }
    else
    {
        afT[0] = -fB/fA;
        if(0.0 <= afT[0] && afT[0] <= 1.0)
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("6 - Found one intersection\r");
			rc = INTERSECT_PARTIAL; // grayman #3584
            riQuantity = 1;
            Contained[0] = Segment[LSG_ORIGIN]+afT[0]*Segment[LSG_DIRECTION];
        }
        else
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("7 - Found no intersections\r");
			rc = INTERSECT_OUTSIDE;
			riQuantity = 0;
        }
    }

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("afT[0]: %f   afT[1]: %f\r", afT[0], afT[1]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[0]", Contained[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[1]", Contained[1]);

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rc: %u   riQuantity %f\r", rc, riQuantity);

	return rc;
}

EIntersection IntersectRayEllipsoid(const idVec3 Ray[LSG_COUNT], 
								 const idVec3 Ellipsoid[ELL_COUNT],
								 idVec3 Contained[2], bool bInside[LSG_COUNT])
{
	EIntersection rc = INTERSECT_COUNT;
	float fRoot;
	float fInvA;
    float afT[2] = { 0.0, 0.0 };
	float riQuantity;

	idMat3 A(1/(Ellipsoid[ELA_AXIS].x*Ellipsoid[ELA_AXIS].x), 0, 0,
		0, 1/(Ellipsoid[ELA_AXIS].y*Ellipsoid[ELA_AXIS].y), 0,
		0, 0, 1/(Ellipsoid[ELA_AXIS].z*Ellipsoid[ELA_AXIS].z));

    // set up quadratic Q(t) = a*t^2 + 2*b*t + c
    idVec3 kDiff = Ray[LSG_ORIGIN] - Ellipsoid[ELL_ORIGIN];
    idVec3 kMatDir = A * Ray[LSG_DIRECTION];
    idVec3 kMatDiff = A * kDiff;
    float fA = Ray[LSG_DIRECTION]* kMatDir;
    float fB = Ray[LSG_DIRECTION] * kMatDiff;
    float fC = kDiff * kMatDiff - 1.0;
	bInside[0] = false;
	bInside[1] = false;

	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineSegStart", Ray[LSG_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineSegDirection", Ray[LSG_DIRECTION]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllOrigin", Ellipsoid[ELL_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllCenter", Ellipsoid[ELA_CENTER]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllAxis", Ellipsoid[ELA_AXIS]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Diff", kDiff);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDir", kMatDir);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDiff", kMatDiff);

    float fDiscr = fB*fB - fA*fC;
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("fA: %f   fB: %f   fC: %f   fDiscr: %f\r", fA, fB, fC, fDiscr);
    if(fDiscr < 0.0)
    {
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
		rc = INTERSECT_OUTSIDE;
        riQuantity = 0;
    }
    else if(fDiscr > 0.0)
    {
		fRoot = idMath::Sqrt64(fDiscr);
        fInvA = 1.0/fA;
        afT[0] = (-fB - fRoot)*fInvA;
        afT[1] = (-fB + fRoot)*fInvA;

        if(afT[0] >= 0.0)
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found two intersections\r");
			rc = INTERSECT_FULL;
            riQuantity = 2;
            Contained[0] = Ray[LSG_ORIGIN] + afT[0]*Ray[LSG_DIRECTION];
            Contained[1] = Ray[LSG_ORIGIN] + afT[1]*Ray[LSG_DIRECTION];
        }
        else if(afT[1] >= 0.0)
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found one intersection\r");
			rc = INTERSECT_PARTIAL;
            riQuantity = 1;
            Contained[0] = Ray[LSG_ORIGIN] + afT[1]*Ray[LSG_DIRECTION];
        }
        else
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
			rc = INTERSECT_OUTSIDE;
            riQuantity = 0;
        }
    }
    else
    {
        afT[0] = -fB/fA;
        if(afT[0] >= 0.0)
        {
 			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found one intersection\r");
			riQuantity = 1;
            Contained[0] = Ray[LSG_ORIGIN] + afT[0]*Ray[LSG_DIRECTION];
        }
        else
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
            riQuantity = 0;
        }
    }

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("afT[0]: %f   afT[1]: %f\r", afT[0], afT[1]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[0]", Contained[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[1]", Contained[1]);

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rc: %u   riQuantity %f\r", rc, riQuantity);

	return rc;
}


EIntersection IntersectLineEllipsoid(const idVec3 Line[LSG_COUNT], 
								 const idVec3 Ellipsoid[ELL_COUNT],
								 idVec3 Contained[2])
{
	EIntersection rc = INTERSECT_COUNT;
	float fRoot;
	float fInvA;
    float afT[2] = { 0.0, 0.0 };
	float riQuantity;

	idMat3 A(1/(Ellipsoid[ELA_AXIS].x*Ellipsoid[ELA_AXIS].x), 0, 0,
		0, 1/(Ellipsoid[ELA_AXIS].y*Ellipsoid[ELA_AXIS].y), 0,
		0, 0, 1/(Ellipsoid[ELA_AXIS].z*Ellipsoid[ELA_AXIS].z));

    // set up quadratic Q(t) = a*t^2 + 2*b*t + c
    idVec3 kDiff = Line[LSG_ORIGIN] - Ellipsoid[ELL_ORIGIN];
    idVec3 kMatDir = A * Line[LSG_DIRECTION];
    idVec3 kMatDiff = A * kDiff;
    float fA = Line[LSG_DIRECTION]* kMatDir;
    float fB = Line[LSG_DIRECTION] * kMatDiff;
    float fC = kDiff * kMatDiff - 1.0;

	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineStart", Line[LSG_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineDirection", Line[LSG_DIRECTION]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllOrigin", Ellipsoid[ELL_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllCenter", Ellipsoid[ELA_CENTER]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllAxis", Ellipsoid[ELA_AXIS]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Diff", kDiff);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDir", kMatDir);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDiff", kMatDiff);

    float fDiscr = fB*fB - fA*fC;
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("fA: %f   fB: %f   fC: %f   fDiscr: %f\r", fA, fB, fC, fDiscr);
    if(fDiscr < 0.0)
    {
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
		rc = INTERSECT_OUTSIDE;
        riQuantity = 0;
    }
    else if(fDiscr > 0.0)
    {
		fRoot = idMath::Sqrt64(fDiscr);
        fInvA = 1.0/fA;
        riQuantity = 2;
        afT[0] = (-fB - fRoot)*fInvA;
        afT[1] = (-fB + fRoot)*fInvA;
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found two intersections\r");
		rc = INTERSECT_FULL;
        riQuantity = 2;
        Contained[0] = Line[LSG_ORIGIN] + afT[0]*Line[LSG_DIRECTION];
        Contained[1] = Line[LSG_ORIGIN] + afT[1]*Line[LSG_DIRECTION];
    }
    else
    {
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found one intersection\r");
		rc = INTERSECT_PARTIAL;
        riQuantity = 1;
        afT[0] = -fB/fA;
        Contained[0] = Line[LSG_ORIGIN] + afT[0]*Line[LSG_DIRECTION];
    }

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("afT[0]: %f   afT[1]: %f\r", afT[0], afT[1]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[0]", Contained[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[1]", Contained[1]);

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rc: %u   riQuantity %f\r", rc, riQuantity);

	return rc;
}

bool LineSegTriangleIntersect(const idVec3 Seg[LSG_COUNT], idVec3 Tri[3], idVec3 &Intersect, float &t)
{
	bool rc = false;

	idVec3 e1, e2, p, s, q;
	float u, v, tmp;

	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Tri[0]", Tri[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Tri[1]", Tri[1]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Tri[2]", Tri[2]);

	e1 = Tri[1] - Tri[0];
	e2 = Tri[2] - Tri[0];
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "e1", e1);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "e2", e2);

	p = Seg[LSG_DIRECTION].Cross(e2);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "p", p);

	tmp = p * e1;			// Dotproduct

	if(tmp > -idMath::FLT_EPS && tmp < idMath::FLT_EPS)
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("No triangle intersection: tmp = %f\r", tmp);
		goto Quit;
	}

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("tmp = %f\r", tmp);
	tmp = 1.0/tmp;
	s = Seg[LSG_ORIGIN] - Tri[0];
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "s", s);

	u = tmp * (s * p);
	if(u < 0.0 || u > 1.0)
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("No triangle intersection: u = %f\r", u);
		goto Quit;
	}

	q = s.Cross(e1);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "q", q);
	v = tmp * (Seg[LSG_DIRECTION] * q);
	if(v < 0.0 || v > 1.0)
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("No triangle intersection: v = %f\r", v);
		goto Quit;
	}

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("u = %f   v = %f\r", u, v);
	if((u + v) > 1.0)
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("No triangle intersection  (u + v) = %f\r", u + v);
		goto Quit;
	}

	// positive value for t > 1 means that the linesegment is below the triangle
	// while negative values mean the lineseg is above the triangle.
	t = tmp * (e2 * q);
	Intersect = Seg[LSG_ORIGIN] + (t * Seg[LSG_DIRECTION]);

/*
	if(t < 0.0 || t > 1.0)
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("No triangle intersection  t = %f\r", t);
		goto Quit;
	}
*/
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Triangle intersection t = %f\r", t);

	rc = true;

Quit:
	return rc;
}


void R_SetLightProject(idPlane lightProject[4], 
					   const idVec3 &origin, 
					   const idVec3 &target,
					   idVec3 &right, 
					   idVec3 &up,
					   const idVec3 &start, 
					   const idVec3 &stop)
{
	float		dist;
	float		scale;
	float		rLen, uLen;
	idVec3		normal;
	float		ofs;
	idVec3		startGlobal;
	idVec4		targetGlobal;

	rLen = right.Normalize();
	uLen = up.Normalize();
	normal = up.Cross(right);
	normal.Normalize();

	dist = target * normal; //  - (origin * normal);
	if(dist < 0)
	{
		dist = -dist;
		normal = -normal;
	}

	scale = (0.5f * dist) / rLen;
	right *= scale;
	scale = -(0.5f * dist) / uLen;
	up *= scale;

	lightProject[2] = normal;
	lightProject[2][3] = -(origin * lightProject[2].Normal());

	lightProject[0] = right;
	lightProject[0][3] = -(origin * lightProject[0].Normal());

	lightProject[1] = up;
	lightProject[1][3] = -(origin * lightProject[1].Normal());

	// now offset to center
	targetGlobal.ToVec3() = target + origin;
	targetGlobal[3] = 1;
	ofs = 0.5f - (targetGlobal * lightProject[0].ToVec4()) / (targetGlobal * lightProject[2].ToVec4());
	lightProject[0].ToVec4() += ofs * lightProject[2].ToVec4();
	ofs = 0.5f - (targetGlobal * lightProject[1].ToVec4()) / (targetGlobal * lightProject[2].ToVec4());
	lightProject[1].ToVec4() += ofs * lightProject[2].ToVec4();

	// set the falloff vector
	normal = stop - start;
	dist = normal.Normalize();
	if (dist <= 0)
	{
		dist = 1;
	}
	lightProject[3] = normal * (1.0f / dist);
	startGlobal = start + origin;
	lightProject[3][3] = -(startGlobal * lightProject[3].Normal());
}

/*void R_SetLightFrustum(const idPlane lightProject[4], idPlane frustum[6])
{
	int		i;

	// we want the planes of s=0, s=q, t=0, and t=q
	frustum[0] = lightProject[0];
	frustum[1] = lightProject[1];
	frustum[2] = lightProject[2] - lightProject[0];
	frustum[3] = lightProject[2] - lightProject[1];

	// we want the planes of s=0 and s=1 for front and rear clipping planes
	frustum[4] = lightProject[3];

	frustum[5] = lightProject[3];
	frustum[5][3] -= 1.0f;
	frustum[5] = -frustum[5];

	for (i = 0 ; i < 6 ; i++)
	{
		float	l;

		frustum[i] = -frustum[i];
		l = frustum[i].Normalize();
		frustum[i][3] /= l;
	}
}*/

// grayman #3584 - rewritten for light cones
EIntersection IntersectLineLightCone(const idVec3 rkLine[LSG_COUNT],
								 idVec3 rkCone[ELC_COUNT],
								 idVec3 Intersect[2],
								 bool inside[2])
{
	EIntersection rc = INTERSECT_COUNT;
	int i, n, intersectionCount, x;
	float t;
	idPlane lightProject[4];
	idPlane frustum[6];
	int sides[6][2] = { {0,0},
						{0,0},
						{0,0},
						{0,0},
						{0,0},
						{0,0} };
	int sidesBack = 0;
	idStr txt;
	idStr format("Frustum[%u]");
	idVec3 EndPoint(rkLine[LSG_ORIGIN]+rkLine[LSG_DIRECTION]);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("  rkLine[LSG_ORIGIN] = [%s]\r", rkLine[LSG_ORIGIN].ToString());
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("  rkLine[LSG_DIRECTION] = [%s]\r", rkLine[LSG_DIRECTION].ToString());
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("  EndPoint = [%s]\r", EndPoint.ToString());

	R_SetLightProject(lightProject,
					   rkCone[ELC_ORIGIN],
					   rkCone[ELA_TARGET],
					   rkCone[ELA_RIGHT],
					   rkCone[ELA_UP],
					   rkCone[ELA_START],
					   rkCone[ELA_END]);
	R_SetLightFrustum(lightProject, frustum);

/*
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[0]", lightProject[0]);
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[1]", lightProject[1]);
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[2]", lightProject[2]);
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[3]", lightProject[3]);
 */
	n = format.Length();

	// grayman #3584 - determine whether the start and end points of the line segment
	// lie on the back (inside) or front (outside) of each of the 6 planes of the light cone

	intersectionCount = 0;
	for ( i = 0 ; i < 6 ; i++ )
	{
		sprintf(txt, format, i);

		DM_LOGPLANE(LC_MATH, LT_DEBUG, txt, frustum[i]);

		sides[i][0] = frustum[i].Side(rkLine[LSG_ORIGIN], idMath::FLT_EPS);
		sides[i][1] = frustum[i].Side(EndPoint, idMath::FLT_EPS);
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%d]: sides[%d][0]: %d sides[%d][1]: %d\r", i, i,sides[i][0], i,sides[i][1]);
		if ( ( sides[i][0] == PLANESIDE_FRONT ) && ( sides[i][1] == PLANESIDE_FRONT ) )
		{
			// both ends of the line are outside the frustum, no need to calculate an intersection
			inside[0] = false;
			inside[1] = false;
			return INTERSECT_OUTSIDE;
		}

		if ( sides[i][0] == PLANESIDE_BACK )
		{
			sidesBack++;
		}

		if ( sides[i][1] == PLANESIDE_BACK )
		{
			sidesBack++;
		}
	}

	if ( sidesBack == 12 )
	{
		// both ends of the line are inside the frustum, no need to calculate an intersection
		rc = INTERSECT_NONE;
		Intersect[0] = rkLine[LSG_ORIGIN];
		Intersect[1] = EndPoint;
		inside[0] = true;
		inside[1] = true;
	}
	else // calculate an intersection
	{
		// We've determined that at least one of the endpoints is possibly inside
		// the frustum (light volume).

		for ( i = 0 ; i < 6 ; i++ )
		{
			if ( sides[i][0] != sides[i][1] )
			{
				// the line crosses the plane, so calculate an intersection

				if ( frustum[i].RayIntersection(rkLine[LSG_ORIGIN], rkLine[LSG_DIRECTION], t) ) // grayman #3584
				//if (frustum[i].LineIntersection(rkLine[LSG_ORIGIN], rkLine[LSG_DIRECTION], &t) == true) // grayman #3584 - this call gives wrong answers
				{
					idVec3 candidate = rkLine[LSG_ORIGIN] + t*rkLine[LSG_DIRECTION];

					// This intersection is valid iff it lies on the backside of all planes.
					// So check that, except for the plane you just intersected, because
					// you know that will return INTERSECT_ON, which is okay.

					bool inside = true;
					for ( int j = 0 ; j < 6 ; j++ )
					{
						if ( j == i )
						{
							continue; // skip the plane you just intersected
						}
						x = frustum[j].Side(candidate, idMath::FLT_EPS);
						if (x == PLANESIDE_FRONT)
						{
							inside = false;
							break; // no need to continue, since the point is outside
						}
					}

					if ( inside )
					{
						// This is a valid intersection point.

						Intersect[intersectionCount] = candidate;

						DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u] intersects\r", i);

						if (++intersectionCount > 1)
						{
							break;
						}
					}
				}
			}
		}

		if ( intersectionCount == 0 )
		{
			// both ends of the line are outside the frustum, no need to calculate an intersection
			rc = INTERSECT_OUTSIDE;
			inside[0] = false;
			inside[1] = false;
		}
		else if ( intersectionCount == 1 )
		{
			// One end of the line segment is inside the cone, and the other outside.

			rc = INTERSECT_PARTIAL;

			// Need to determine which end of the line is inside. It will be
			// the second of the two points returned. The first point will be
			// the intersection, and it has already been filled in (Intersect[0]).

			// Check the results of the side checks. If sides[0..6][0] are all
			// PLANESIDE_BACK, then the line origin is inside.
			// Otherwise, the line end is inside

			Intersect[1] = rkLine[LSG_ORIGIN]; // assume it's the line origin
			inside[0] = true;
			inside[1] = false;

			for ( i = 0 ; i < 6 ; i++ )
			{
				if (sides[i][0] == PLANESIDE_FRONT )
				{
					Intersect[1] = EndPoint; // nope, it must be the line end
					inside[0] = false;
					inside[1] = true;
					break;
				}
			}
		}
		else // intersectionCount == 2
		{
			rc = INTERSECT_FULL;
			inside[0] = false;
			inside[1] = false;

/*			// old code
			bool bStart = true;
			bool bEnd = true;
			for ( i = 0 ; i < 6 ; i++ )
			{
				x = frustum[i].Side(Intersect[0], idMath::FLT_EPS);
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u/0] intersection test returns %u\r", i, x);
				if (x != PLANESIDE_BACK)
				{
					bStart = false;
				}

				x = frustum[i].Side(Intersect[1], idMath::FLT_EPS);
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u/1] intersection test returns %u\r", i, x);
				if (x != PLANESIDE_BACK)
				{
					bEnd = false;
				}
			}

			if (bStart == false && bEnd == false)
			{
				rc = INTERSECT_OUTSIDE;
			}*/
		}
	}

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Intersection count = %u\r", intersectionCount);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Intersect[0]", Intersect[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Intersect[1]", Intersect[1]);

	return rc;
}

EIntersection IntersectLineCone(const idVec3 rkLine[LSG_COUNT],
								 idVec3 rkCone[ELC_COUNT],
								 idVec3 Intersect[2], bool Stump)
{
	EIntersection rc = INTERSECT_COUNT;
	int i, n, l, x;
	float t, angle;
	idPlane lightProject[4];
	idPlane frustum[6];
	int Start[6];
	int End[6];
	bool bStart, bEnd;
	bool bCalcIntersection;
	idStr txt;
	idStr format("Frustum[%u]");
	idVec3 EndPoint(rkLine[LSG_ORIGIN]+rkLine[LSG_DIRECTION]);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("  rkLine[LSG_ORIGIN] = [%s]\r", rkLine[LSG_ORIGIN].ToString());
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("  rkLine[LSG_DIRECTION] = [%s]\r", rkLine[LSG_DIRECTION].ToString());
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("  EndPoint = [%s]\r", EndPoint.ToString());

	R_SetLightProject(lightProject,
					   rkCone[ELC_ORIGIN],
					   rkCone[ELA_TARGET],
					   rkCone[ELA_RIGHT],
					   rkCone[ELA_UP],
					   rkCone[ELA_START],
					   rkCone[ELA_END]);
	R_SetLightFrustum(lightProject, frustum);

/*
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[0]", lightProject[0]);
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[1]", lightProject[1]);
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[2]", lightProject[2]);
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[3]", lightProject[3]);
*/
	n = format.Length();

	// Calculate the angle between the target and the lightvector.
	angle = rkCone[ELA_TARGET].Length() * rkLine[LSG_DIRECTION].Length();
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Denominator: %f\r", angle);
	if ( angle >= idMath::FLT_EPS )
	{
		angle = idMath::ACos((rkCone[ELA_TARGET] * rkLine[LSG_DIRECTION])/angle);
//		if(t > (idMath::PI/2))
//			angle = idMath::PI  - angle;

		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Angle: %f\r", angle);
	}
	else
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Impossible line!\r");
	}

	bCalcIntersection = false;
	l = 0;
	for ( i = 0 ; i < 6 ; i++ )
	{
		sprintf(txt, format, i);

		DM_LOGPLANE(LC_MATH, LT_DEBUG, txt, frustum[i]);

		Start[i] = frustum[i].Side(rkLine[LSG_ORIGIN], idMath::FLT_EPS);
		End[i] = frustum[i].Side(EndPoint, idMath::FLT_EPS);

		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u]: Start %u   End: %u\r", i, Start[i], End[i]);

		// If the points are all on the outside there will be no intersections
		if ( ( Start[i] == PLANESIDE_BACK ) || ( End[i] == PLANESIDE_BACK ) )
		{
			bCalcIntersection = true;
		}
	}

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("CalcIntersection: %u\r", bCalcIntersection);
	if (bCalcIntersection == true)
	{
		DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "TargetOrigin", rkLine[LSG_ORIGIN]);
		DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "TargetDirection", rkLine[LSG_DIRECTION]);
		DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Endpoint", EndPoint);

		for(i = 0; i < 6; i++)
		{
			if (frustum[i].LineIntersection(rkLine[LSG_ORIGIN], rkLine[LSG_DIRECTION], &t) == true)
			{
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u] intersects\r", i);
				Intersect[l] = rkLine[LSG_ORIGIN] + t*rkLine[LSG_DIRECTION];
				l++;

				if (l > 1)
				{
					break;
				}
			}
		}
	}

	if (l < 2)
	{
		rc = INTERSECT_OUTSIDE;
	}
	else
	{
		rc = INTERSECT_FULL;
		bStart = bEnd = true;
		for (i = 0; i < 6; i++)
		{
			x = frustum[i].Side(Intersect[0], idMath::FLT_EPS);
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u/0] intersection test returns %u\r", i, x);
			if (x != PLANESIDE_BACK)
			{
				bStart = false;
			}

			x = frustum[i].Side(Intersect[1], idMath::FLT_EPS);
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u/1] intersection test returns %u\r", i, x);
			if (x != PLANESIDE_BACK)
			{
				bEnd = false;
			}
		}

		if (bStart == false && bEnd == false)
		{
			rc = INTERSECT_OUTSIDE;
		}
	}

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Intersection count = %u\r", l);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "akPoint[0]", Intersect[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "akPoint[1]", Intersect[1]);

	return rc;
}