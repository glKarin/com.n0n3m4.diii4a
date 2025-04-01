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

#ifndef __BV_CIRCCONE_H__
#define __BV_CIRCCONE_H__

#include "../math/Vector.h"

/*
===============================================================================

	Bounding Circular Cone

===============================================================================
*/

/**
 * stgatilov #5886: Circular bounding cone bounds set of 3D directions (similar to how AABox bounds 3D positions).
 * It are used in BVH tree to cull away whole nodes with backfacing/frontfacing triangles.
 * Such BVH is also called Spatial Normal Cone Hierarchy, see e.g. paper:
 *   https://www.cs.utah.edu/gdc/publications/papers/johnsonI3D2001.pdf
*/
class idCircCone {
public:
					idCircCone() = default;
	explicit		idCircCone( const idVec3 &axis, float cosAng, float sinAng );

	void			Set( const idVec3 &axis, float cosAng, float sinAng );	// set by unit axis and cos,sin
	void			SetAngle( const idVec3 &axis, float angle );	// set by axis and angle
	void			SetBox( const idVec3 &origin, const idBounds &box );	// set cone with all vectors from origin to point in box

	static idCircCone Empty();
	static idCircCone Full();
	void			Clear();										// make "empty" cone (neutral element for AddXXX)
	void			MakeFull();										// make "full" cone (contains all direction, gives no information)

	idVec3			GetAxis() const;								// returns central axis
	float			GetAngle() const;								// returns axis-boundary angle in radians
	float			GetCos() const;									// returns sine of the angle
	float			GetSin() const;									// returns cosine of the angle

	bool			IsEmpty() const;								// returns true for "empty" cone
	bool			IsFull() const;									// returns true for "full" cone
	bool			IsConvex() const;								// returns true when cone angle is less than 90 degrees (or empty)

	bool			ContainsDir( const idVec3 &v ) const;			// is unit vector inside or on boundary?
	bool			ContainsVec( const idVec3 &v ) const;			// is vector inside or on boundary?
	bool			ContainsDirFast( const idVec3 &v ) const;		// same as ContainsDir, but weak precision on low angles
	bool			ContainsVecFast( const idVec3 &v ) const;		// same as ContainsVec, but weak precision on low angles

	bool			HaveEqual( const idCircCone &c ) const;			// is there a pair of equal directions in two cones?
	bool			HaveCollinear( const idCircCone &c ) const;		// is there a pair of collinear directions in two cones?
	bool			HaveOrthogonal( const idCircCone &c ) const;	// is there a pair of orthogonal directions in two cones?
	int				HaveSameDirection( const idCircCone &c ) const;	// returns +/-1 if all pairs of vectors give positive/negative dot product

	idCircCone &	Transform( const idMat3 &rotation );			// matrix must be orthogonal
	idCircCone &	Negate();										// reverse axis
	idCircCone		Negated() const;								// get cone with reversed axis

	idCircCone &	ExpandSelf( float dcos, float dsin );			// increase cone angle by delta angle (given as cos,sin)
	idCircCone		ExpandAngle( float d ) const;					// increase cone angle by delta angle
	idCircCone &	ExpandAngleSelf( float d );						// increase cone angle by delta angle

	void			AddDir( const idVec3 &v );						// add unit vector (tight bound)
	void			AddVec( const idVec3 &v );						// add vector (tight bound)
	void			AddCone( const idCircCone &c );					// add cone of directions (tight bound)
	void			AddDirSaveAxis( const idVec3 &v );				// add unit vector without changing axis
	void			AddVecSaveAxis( const idVec3 &v );				// add vector without changing axis
	void			AddConeSaveAxis( const idCircCone &c );			// add cone of directions without changing axis
	void			FromVectors( const idVec3 *vectors, int num );	// bounding cone for a list of directions

	void			Normalize();									// normalize cos,sin pair (do this after long sequence of operations)

private:
	// unit vector = central direction of the cone unit vector
	// zero vector = cone is either "empty" of "full"
	idVec3			axis;
	// cos and sin of the angle between axis and cone boundary
	// "full" cone has angle = 180 degrees
	// "empty" cone has angle = zero
	float			cosAngle;
	float			sinAngle;
};


ID_FORCE_INLINE idCircCone::idCircCone( const idVec3 &ax, float cosAng, float sinAng ) {
	// note: this setter is "unsafe", it assumes that caller has prepared data properly
	axis = ax;
	cosAngle = cosAng;
	sinAngle = sinAng;
}
ID_FORCE_INLINE void idCircCone::Set( const idVec3 &ax, float cosAng, float sinAng ) {
	// note: this setter is "unsafe", it assumes that caller has prepared data properly
	axis = ax;
	cosAngle = cosAng;
	sinAngle = sinAng;
}

ID_INLINE idCircCone idCircCone::Empty() {
	return idCircCone( idVec3( 0.0f, 0.0f, 0.0f ), 1.0f, 0.0f );
}
ID_INLINE idCircCone idCircCone::Full() {
	return idCircCone( idVec3( 0.0f, 0.0f, 0.0f ), -1.0f, 0.0f );
}
ID_INLINE void idCircCone::Clear() {
	axis.Zero();
	cosAngle = 1.0f;
	sinAngle = 0.0f;
}
ID_INLINE void idCircCone::MakeFull() {
	axis.Zero();
	cosAngle = -1.0f;
	sinAngle = 0.0f;
}

ID_FORCE_INLINE idVec3 idCircCone::GetAxis() const {
	return axis;
}
ID_FORCE_INLINE float idCircCone::GetCos() const {
	return cosAngle;
}
ID_FORCE_INLINE float idCircCone::GetSin() const {
	return sinAngle;
}

ID_INLINE bool idCircCone::IsEmpty() const {
	return cosAngle == 1.0f && axis.x == 0.0f && axis.y == 0.0f && axis.z == 0.0f;
}
ID_INLINE bool idCircCone::IsFull() const {
	return cosAngle == -1.0f;
}
ID_INLINE bool idCircCone::IsConvex() const {
	return cosAngle > 1e-4f;
}

ID_INLINE bool idCircCone::ContainsDirFast( const idVec3 &v ) const {
	float cosInter = axis * v;
	// note: cosInter = 0 and cosAngle = +-1 for empty/full
	// this automatically gives correct outcome
	return cosInter >= cosAngle;
}
ID_INLINE bool idCircCone::ContainsVecFast( const idVec3 &v ) const {
	float cosInter = axis * v;
	// note: cosInter = 0 and cosAngle = +-1 for empty/full
	// we can get correct result by forcing vector length to be positive
	return cosInter >= cosAngle * ( v.Length() + idMath::FLT_SMALLEST_NON_DENORMAL );
}

ID_INLINE idCircCone &idCircCone::Transform( const idMat3 &rotation ) {
	axis = rotation * axis;
	return *this;
}
ID_INLINE idCircCone &idCircCone::Negate() {
	axis = -axis;
	return *this;
}
ID_INLINE idCircCone idCircCone::Negated() const {
	return idCircCone( -axis, cosAngle, sinAngle );
}

ID_INLINE void idCircCone::Normalize() {
	axis.Normalize();
	idVec2 v( cosAngle, sinAngle );
	v.Normalize();
	cosAngle = v.x;
	sinAngle = v.y;
}

#endif
