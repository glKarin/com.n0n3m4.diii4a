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



idBounds bounds_zero( vec3_zero, vec3_zero );
idBounds bounds_zeroOneCube(idVec3(0.0f), idVec3(1.0f)); //anon
idBounds bounds_unitCube(idVec3(-1.0f), idVec3(1.0f)); //anon

/*
============
idBounds::GetRadius
============
*/
float idBounds::GetRadius( void ) const {
	int		i;
	float	total, b0, b1;

	total = 0.0f;
	for ( i = 0; i < 3; i++ ) {
		b0 = (float)idMath::Fabs( b[0][i] );
		b1 = (float)idMath::Fabs( b[1][i] );
		if ( b0 > b1 ) {
			total += b0 * b0;
		} else {
			total += b1 * b1;
		}
	}
	return idMath::Sqrt( total );
}

/*
============
idBounds::GetRadius
============
*/
float idBounds::GetRadius( const idVec3 &center ) const {
	int		i;
	float	total, b0, b1;

	total = 0.0f;
	for ( i = 0; i < 3; i++ ) {
		b0 = (float)idMath::Fabs( center[i] - b[0][i] );
		b1 = (float)idMath::Fabs( b[1][i] - center[i] );
		if ( b0 > b1 ) {
			total += b0 * b0;
		} else {
			total += b1 * b1;
		}
	}
	return idMath::Sqrt( total );
}

/*
================
idBounds::PlaneDistance
================
*/
float idBounds::PlaneDistance( const idPlane &plane ) const {
	idVec3 center = ( b[0] + b[1] ) * 0.5f;

	float d1 = plane.Distance( center );
	float d2 = (
		idMath::Fabs( ( b[1].x - center.x ) * plane.Normal().x ) +
		idMath::Fabs( ( b[1].y - center.y ) * plane.Normal().y ) +
		idMath::Fabs( ( b[1].z - center.z ) * plane.Normal().z )
	);

	if ( d1 - d2 > 0.0f ) {
		return d1 - d2;
	}
	if ( d1 + d2 < 0.0f ) {
		return d1 + d2;
	}
	return 0.0f;
}

/*
================
idBounds::PlaneSide
================
*/
int idBounds::PlaneSide( const idPlane &plane, const float epsilon ) const {
	idVec3 center = ( b[0] + b[1] ) * 0.5f;

	float d1 = plane.Distance( center );
	float d2 = (
		idMath::Fabs( ( b[1].x - center.x ) * plane.Normal().x ) +
		idMath::Fabs( ( b[1].y - center.y ) * plane.Normal().y ) +
		idMath::Fabs( ( b[1].z - center.z ) * plane.Normal().z )
	);

	if ( d1 - d2 > epsilon ) {
		return PLANESIDE_FRONT;
	}
	if ( d1 + d2 < -epsilon ) {
		return PLANESIDE_BACK;
	}
	return PLANESIDE_CROSS;
}

/*
============
idBounds::LineIntersection

  Returns true if the line intersects the bounds between the start and end point.
============
*/
bool idBounds::LineIntersection( const idVec3 &start, const idVec3 &end ) const {
    idVec3 ld;
	idVec3 center = ( b[0] + b[1] ) * 0.5f;
	idVec3 extents = b[1] - center;
    idVec3 lineDir = 0.5f * ( end - start );
    idVec3 lineCenter = start + lineDir;
    idVec3 dir = lineCenter - center;

    ld.x = idMath::Fabs( lineDir.x );
	if ( idMath::Fabs( dir.x ) > extents.x + ld.x ) {
        return false;
	}

    ld.y = idMath::Fabs( lineDir.y );
	if ( idMath::Fabs( dir.y ) > extents.y + ld.y ) {
        return false;
	}

    ld.z = idMath::Fabs( lineDir.z );
	if ( idMath::Fabs( dir.z ) > extents.z + ld.z ) {
        return false;
	}

    idVec3 cross = lineDir.Cross( dir );

	if ( idMath::Fabs( cross.x ) > extents.y * ld.z + extents.z * ld.y ) {
        return false;
	}

	if ( idMath::Fabs( cross.y ) > extents.x * ld.z + extents.z * ld.x ) {
        return false;
	}

	if ( idMath::Fabs( cross.z ) > extents.x * ld.y + extents.y * ld.x ) {
        return false;
	}

    return true;
}

/*
============
idBounds::RayIntersection

  Returns true if the ray intersects the bounds.
  The ray can intersect the bounds in both directions from the start point.
  If start is inside the bounds it is considered an intersection with scale = 0
============
*/
bool idBounds::RayIntersection( const idVec3 &start, const idVec3 &dir, float &scale ) const {
	int i, ax0, ax1, ax2, side, inside;
	float f;
	idVec3 hit;

	ax0 = -1;
	inside = 0;
	for ( i = 0; i < 3; i++ ) {
		if ( start[i] < b[0][i] ) {
			side = 0;
		}
		else if ( start[i] > b[1][i] ) {
			side = 1;
		}
		else {
			inside++;
			continue;
		}
		if ( dir[i] == 0.0f ) {
			continue;
		}
		f = ( start[i] - b[side][i] );
		if ( ax0 < 0 || idMath::Fabs( f ) > idMath::Fabs( scale * dir[i] ) ) {
			scale = - ( f / dir[i] );
			ax0 = i;
		}
	}

	if ( ax0 < 0 ) {
		scale = 0.0f;
		// return true if the start point is inside the bounds
		return ( inside == 3 );
	}

	ax1 = (ax0+1)%3;
	ax2 = (ax0+2)%3;
	hit[ax1] = start[ax1] + scale * dir[ax1];
	hit[ax2] = start[ax2] + scale * dir[ax2];

	return (
		hit[ax1] >= b[0][ax1] && hit[ax1] <= b[1][ax1] &&
		hit[ax2] >= b[0][ax2] && hit[ax2] <= b[1][ax2]
	);
}

/*
============
idBounds::FromTransformedBounds
============
*/
void idBounds::FromTransformedBounds( const idBounds &bounds, const idVec3 &origin, const idMat3 &axis ) {
	int i;
	idVec3 center, extents, rotatedExtents;

	center = (bounds[0] + bounds[1]) * 0.5f;
	extents = bounds[1] - center;

	for ( i = 0; i < 3; i++ ) {
		rotatedExtents[i] = idMath::Fabs( extents.x * axis[0][i] ) +
							idMath::Fabs( extents.y * axis[1][i] ) +
							idMath::Fabs( extents.z * axis[2][i] );
	}

	center = origin + center * axis;
	b[0] = center - rotatedExtents;
	b[1] = center + rotatedExtents;
}

/*
============
idBounds::FromPoints

  Most tight bounds for a point set.
============
*/
void idBounds::FromPoints( const idVec3 *points, const int numPoints ) {
	SIMDProcessor->MinMax( b[0], b[1], points, numPoints );
}

/*
============
idBounds::FromPointTranslation

  Most tight bounds for the translational movement of the given point.
============
*/
void idBounds::FromPointTranslation( const idVec3 &point, const idVec3 &translation ) {
	int i;

	for ( i = 0; i < 3; i++ ) {
		if ( translation[i] < 0.0f ) {
			b[0][i] = point[i] + translation[i];
			b[1][i] = point[i];
		}
		else {
			b[0][i] = point[i];
			b[1][i] = point[i] + translation[i];
		}
	}
}

/*
============
idBounds::FromBoundsTranslation

  Most tight bounds for the translational movement of the given bounds.
============
*/
void idBounds::FromBoundsTranslation( const idBounds &bounds, const idVec3 &origin, const idMat3 &axis, const idVec3 &translation ) {
	int i;

	if ( axis.IsRotated() ) {
		FromTransformedBounds( bounds, origin, axis );
	}
	else {
		b[0] = bounds[0] + origin;
		b[1] = bounds[1] + origin;
	}
	for ( i = 0; i < 3; i++ ) {
		if ( translation[i] < 0.0f ) {
			b[0][i] += translation[i];
		}
		else {
			b[1][i] += translation[i];
		}
	}
}
void idBounds::FromBoundsTranslation( const idBounds &bounds, const idVec3 &origin, const idVec3 &translation ) {
	b[0] = bounds[0] + origin;
	b[1] = bounds[1] + origin;
	for ( int i = 0; i < 3; i++ ) {
		if ( translation[i] < 0.0f ) {
			b[0][i] += translation[i];
		}
		else {
			b[1][i] += translation[i];
		}
	}
}


/*
================
BoundsForPointRotation

  only for rotations < 180 degrees
================
*/
idBounds BoundsForPointRotation( const idVec3 &start, const idRotation &rotation ) {
	int i;
	float radiusSqr;
	idVec3 v1, v2;
	idVec3 origin, axis, end;
	idBounds bounds;

	end = start * rotation;
	axis = rotation.GetVec();
	origin = rotation.GetOrigin() + axis * ( axis * ( start - rotation.GetOrigin() ) );
	radiusSqr = ( start - origin ).LengthSqr();
	v1 = ( start - origin ).Cross( axis );
	v2 = ( end - origin ).Cross( axis );

	for ( i = 0; i < 3; i++ ) {
		// if the derivative changes sign along this axis during the rotation from start to end
		if ( ( v1[i] > 0.0f && v2[i] < 0.0f ) || ( v1[i] < 0.0f && v2[i] > 0.0f ) ) {
			if ( ( 0.5f * (start[i] + end[i]) - origin[i] ) > 0.0f ) {
				bounds[0][i] = Min( start[i], end[i] );
				bounds[1][i] = origin[i] + idMath::Sqrt( radiusSqr * ( 1.0f - axis[i] * axis[i] ) );
			}
			else {
				bounds[0][i] = origin[i] - idMath::Sqrt( radiusSqr * ( 1.0f - axis[i] * axis[i] ) );
				bounds[1][i] = Max( start[i], end[i] );
			}
		}
		else if ( start[i] > end[i] ) {
			bounds[0][i] = end[i];
			bounds[1][i] = start[i];
		}
		else {
			bounds[0][i] = start[i];
			bounds[1][i] = end[i];
		}
	}

	return bounds;
}

/*
============
idBounds::FromPointRotation

  Most tight bounds for the rotational movement of the given point.
============
*/
void idBounds::FromPointRotation( const idVec3 &point, const idRotation &rotation ) {
	float radius;

	if ( idMath::Fabs( rotation.GetAngle() ) < 180.0f ) {
		(*this) = BoundsForPointRotation( point, rotation );
	}
	else {

		radius = ( point - rotation.GetOrigin() ).Length();

		b[0].Set( -radius, -radius, -radius );
		b[1].Set( radius, radius, radius );

		//stgatilov: according to BoundsForPointRotation code
		//the resulting bounds should be in global coordinate system
		//(without this addition the result would be incorrect)
		TranslateSelf(rotation.GetOrigin());
	}
}

/*
============
idBounds::FromBoundsRotation

  Most tight bounds for the rotational movement of the given bounds.
============
*/
void idBounds::FromBoundsRotation( const idBounds &bounds, const idVec3 &origin, const idMat3 &axis, const idRotation &rotation ) {
	int i;
	float radius;
	idVec3 point;

	if ( idMath::Fabs( rotation.GetAngle() ) < 180.0f ) {

		(*this) = BoundsForPointRotation( bounds[0] * axis + origin, rotation );
		for ( i = 1; i < 8; i++ ) {
			point.x = bounds[(i^(i>>1))&1].x;
			point.y = bounds[(i>>1)&1].y;
			point.z = bounds[(i>>2)&1].z;
			(*this) += BoundsForPointRotation( point * axis + origin, rotation );
		}

	}
	else {

		/*//stgatilov: the original code here is trash!
		point = (bounds[1] - bounds[0]) * 0.5f;
		radius = (bounds[1] - point).Length() + (point - rotation.GetOrigin()).Length();

		// FIXME: these bounds are usually way larger
		b[0].Set( -radius, -radius, -radius );
		b[1].Set( radius, radius, radius );*/

		//radius of bounds in local csys (before applying origin/axis)
		float boundsRadius = bounds.GetSize().Length() * 0.5f;
		//position of box center in global csys, with rotation center subtracted
		point = bounds.GetCenter() * axis + (origin - rotation.GetOrigin());
		//radius of bounding sphere (rotation center is center of sphere)
		radius = point.Length() + boundsRadius;

		b[0].Set( -radius, -radius, -radius );
		b[1].Set( radius, radius, radius );

		//stgatilov: according to BoundsForPointRotation code
		//the resulting bounds should be in global coordinate system
		TranslateSelf(rotation.GetOrigin());

#if 0
		//test code (slow)
		idBounds test;
		test.FromPointRotation( bounds[0] * axis + origin, rotation );
		for ( i = 1; i < 8; i++ ) {
			point.x = bounds[(i^(i>>1))&1].x;
			point.y = bounds[(i>>1)&1].y;
			point.z = bounds[(i>>2)&1].z;
			idBounds qq;
			qq.FromPointRotation( point * axis + origin, rotation );
			test += qq;
		}
		assert((b[0] - test.b[0]).Max() <= 0.1f);
		assert((test.b[1] - b[1]).Max() <= 0.1f);
#endif
	}
}

/*
============
idBounds::ToPoints
============
*/
void idBounds::ToPoints( idVec3 points[8] ) const {
	for ( int i = 0; i < 8; i++ ) {
		points[i].x = b[(i^(i>>1))&1].x;
		points[i].y = b[(i>>1)&1].y;
		points[i].z = b[(i>>2)&1].z;
	}
}

/*
============
idBounds::ToString
============
*/
idStr idBounds::ToString( const int precision ) const
{
	idStr res;
	res += "[";
	res += b[0].ToString( precision );
	res += " : ";
	res += b[1].ToString( precision );
	res += "]";
	return res;
}

