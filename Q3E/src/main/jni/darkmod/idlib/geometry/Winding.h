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

#ifndef __WINDING_H__
#define __WINDING_H__

//for idWinding::CreateTrimmedPlane
enum IncidentPlaneMode {
	INCIDENT_PLANE_DELETE = 0x0,
	INCIDENT_PLANE_RETAIN_CODIRECT = 0x1,
	INCIDENT_PLANE_RETAIN_OPPOSITE = 0x2,
	INCIDENT_PLANE_RETAIN = 0x3,
};


/*
===============================================================================

	A winding is an arbitrary convex polygon defined by an array of points.

===============================================================================
*/

class idWinding {

public:
					idWinding( void );
					explicit idWinding( const int n );								// allocate for n points
					explicit idWinding( const idVec3 *verts, const int n );			// winding from points
					explicit idWinding( const idVec3 &normal, const float dist );	// base winding for plane
					explicit idWinding( const idPlane &plane );						// base winding for plane
					explicit idWinding( const idWinding &winding );
	virtual			~idWinding( void );

	idWinding &		operator=( const idWinding &winding );
	const idVec5 &	operator[]( const int index ) const;
	idVec5 &		operator[]( const int index );

					// add a point to the end of the winding point array
	idWinding &		operator+=( const idVec3 &v );
	idWinding &		operator+=( const idVec5 &v );
	void			AddPoint( const idVec3 &v );
	void			AddPoint( const idVec5 &v );

					// number of points on winding
	int				GetNumPoints( void ) const;
	void			SetNumPoints( int n );
	void			Clear( void );
	virtual void	ClearFree( void );

					// huge winding for plane, the points go counter clockwise when facing the front of the plane
	void			BaseForPlane( const idVec3 &normal, const float dist );
	void			BaseForPlane( const idPlane &plane );

					// stgatilov: creates new winding on mainPlane, trimmed with a set of oriented planes curPlanes[0..numCuts)
	static idWinding *CreateTrimmedPlane( const idPlane &mainPlane, int numCuts, const idPlane *cutPlanes, float epsilon = ON_EPSILON, IncidentPlaneMode incidentPlaneMode = INCIDENT_PLANE_DELETE );

					// splits the winding into a front and back winding, the winding itself stays unchanged
					// returns a SIDE_?
	int				Split( const idPlane &plane, const float epsilon, idWinding **front, idWinding **back ) const;
					// returns the winding fragment at the front of the clipping plane,
					// if there is nothing at the front the winding itself is destroyed and NULL is returned
	idWinding *		Clip( const idPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );
					// cuts off the part at the back side of the plane, returns true if some part was at the front
					// if there is nothing at the front the number of points is set to zero
	bool			ClipInPlace( const idPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

					// returns a copy of the winding
	idWinding *		Copy( void ) const;
	idWinding *		Reverse( void ) const;
	void			ReverseSelf( void );
	void			RemoveEqualPoints( const float epsilon = ON_EPSILON );
	void			RemoveColinearPoints( const idVec3 &normal, const float epsilon = ON_EPSILON );
	void			RemovePoint( int point );
	void			InsertPoint( const idVec3 &point, int spot );
	bool			InsertPointIfOnEdge( const idVec3 &point, const idPlane &plane, const float epsilon = ON_EPSILON );
					// add a winding to the convex hull
	void			AddToConvexHull( const idWinding *winding, const idVec3 &normal, const float epsilon = ON_EPSILON );
					// add a point to the convex hull
	void			AddToConvexHull( const idVec3 &point, const idVec3 &normal, const float epsilon = ON_EPSILON );
					// tries to merge 'this' with the given winding, returns NULL if merge fails, both 'this' and 'w' stay intact
					// 'keep' tells if the contacting points should stay even if they create colinear edges
	idWinding *		TryMerge( const idWinding &w, const idVec3 &normal, int keep = false ) const;
					// check whether the winding is valid or not
	bool			Check( bool print = true ) const;

	float			GetArea( void ) const;
	idVec3			GetCenter( void ) const;
	float			GetRadius( const idVec3 &center ) const;
	void			GetPlane( idVec3 &normal, float &dist ) const;
	void			GetPlane( idPlane &plane ) const;
	void			GetBounds( idBounds &bounds ) const;

	bool			IsTiny( void ) const;
	bool			IsHuge( void ) const;	// base winding for a plane is typically huge
	void			Print( void ) const;

	float			PlaneDistance( const idPlane &plane ) const;
	int				PlaneSide( const idPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			PlanesConcave( const idWinding &w2, const idVec3 &normal1, const idVec3 &normal2, float dist1, float dist2 ) const;

	bool			PointInside( const idVec3 &normal, const idVec3 &point, const float epsilon ) const;
	bool			PointInsideDst( const idVec3 &normal, const idVec3 &point, const float epsilon ) const;
	bool			PointLiesOn( const idVec3 &point, const float epsilon ) const;
					// returns true if the line or ray intersects the winding
	bool			LineIntersection( const idPlane &windingPlane, const idVec3 &start, const idVec3 &end, bool backFaceCull = false ) const;
					// intersection point is start + dir * scale
	bool			RayIntersection( const idPlane &windingPlane, const idVec3 &start, const idVec3 &dir, float &scale, bool backFaceCull = false ) const;

	static float	TriangleArea( const idVec3 &a, const idVec3 &b, const idVec3 &c );
	static double	TriangleAreaDbl( const idVec3 &a, const idVec3 &b, const idVec3 &c );

protected:
	int				numPoints;				// number of points
	int				allocedSize;
	idVec5 *		p;						// pointer to point data

	bool			EnsureAlloced( int n, bool keep = false );
	virtual bool	ReAllocate( int n, bool keep = false );
};

ID_INLINE idWinding::idWinding( void ) {
	numPoints = allocedSize = 0;
	p = NULL;
}

ID_INLINE idWinding::idWinding( int n ) {
	numPoints = allocedSize = 0;
	p = NULL;
	EnsureAlloced( n );
}

ID_INLINE idWinding::idWinding( const idVec3 *verts, const int n ) {
	int i;

	numPoints = allocedSize = 0;
	p = NULL;
	if ( !EnsureAlloced( n ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < n; i++ ) {
		p[i].ToVec3() = verts[i];
		p[i].s = p[i].t = 0.0f;
	}
	numPoints = n;
}

ID_INLINE idWinding::idWinding( const idVec3 &normal, const float dist ) {
	numPoints = allocedSize = 0;
	p = NULL;
	BaseForPlane( normal, dist );
}

ID_INLINE idWinding::idWinding( const idPlane &plane ) {
	numPoints = allocedSize = 0;
	p = NULL;
	BaseForPlane( plane );
}

ID_INLINE idWinding::idWinding( const idWinding &winding ) {
	int i;
	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

ID_INLINE idWinding::~idWinding( void ) {
	delete[] p;
	p = NULL;
}

ID_INLINE idWinding &idWinding::operator=( const idWinding &winding ) {
	int i;

	if ( !EnsureAlloced( winding.numPoints ) ) {
		numPoints = 0;
		return *this;
	}
	for ( i = 0; i < winding.numPoints; i++ ) {
		p[i] = winding.p[i];
	}
	numPoints = winding.numPoints;
	return *this;
}

ID_FORCE_INLINE const idVec5 &idWinding::operator[]( const int index ) const {
	//assert( index >= 0 && index < numPoints );
	return p[ index ];
}

ID_FORCE_INLINE idVec5 &idWinding::operator[]( const int index ) {
	//assert( index >= 0 && index < numPoints );
	return p[ index ];
}

ID_INLINE idWinding &idWinding::operator+=( const idVec3 &v ) {
	AddPoint( v );
	return *this;
}

ID_INLINE idWinding &idWinding::operator+=( const idVec5 &v ) {
	AddPoint( v );
	return *this;
}

ID_INLINE void idWinding::AddPoint( const idVec3 &v ) {
	if ( !EnsureAlloced(numPoints+1, true) ) {
		return;
	}
	p[numPoints] = v;
	numPoints++;
}

ID_INLINE void idWinding::AddPoint( const idVec5 &v ) {
	if ( !EnsureAlloced(numPoints+1, true) ) {
		return;
	}
	p[numPoints] = v;
	numPoints++;
}

ID_FORCE_INLINE int idWinding::GetNumPoints( void ) const {
	return numPoints;
}

ID_INLINE void idWinding::SetNumPoints( int n ) {
	if ( !EnsureAlloced( n, true ) ) {
		return;
	}
	numPoints = n;
}

ID_INLINE void idWinding::Clear( void ) {
	numPoints = 0;
}

ID_INLINE void idWinding::ClearFree( void ) {
	numPoints = 0;
	allocedSize = 0;
	delete[] p;
	p = NULL;
}

ID_INLINE void idWinding::BaseForPlane( const idPlane &plane ) {
	BaseForPlane( plane.Normal(), plane.Dist() );
}

ID_INLINE bool idWinding::EnsureAlloced( int n, bool keep ) {
	if ( n > allocedSize ) {
		return ReAllocate( n, keep );
	}
	return true;
}


/*
===============================================================================

	idFixedWinding is a fixed buffer size winding not using
	memory allocations.

	When an operation would overflow the fixed buffer a warning
	is printed and the operation is safely cancelled.

===============================================================================
*/

#define	MAX_POINTS_ON_WINDING	64

class idFixedWinding : public idWinding {

public:
					idFixedWinding( void );
					explicit idFixedWinding( const int n );
					explicit idFixedWinding( const idVec3 *verts, const int n );
					explicit idFixedWinding( const idVec3 &normal, const float dist );
					explicit idFixedWinding( const idPlane &plane );
					explicit idFixedWinding( const idWinding &winding );
					explicit idFixedWinding( const idFixedWinding &winding );
	virtual			~idFixedWinding( void ) override;

	idFixedWinding &operator=( const idWinding &winding );

	virtual void	ClearFree( void ) override;

					// splits the winding in a back and front part, 'this' becomes the front part
					// returns a SIDE_?
	int				Split( idFixedWinding *back, const idPlane &plane, const float epsilon = ON_EPSILON );

protected:
	idVec5			data[MAX_POINTS_ON_WINDING];	// point data

	virtual bool	ReAllocate( int n, bool keep = false ) override;
};

ID_INLINE idFixedWinding::idFixedWinding( void ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
}

ID_INLINE idFixedWinding::idFixedWinding( int n ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
}

ID_INLINE idFixedWinding::idFixedWinding( const idVec3 *verts, const int n ) {
	int i;

	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	if ( !EnsureAlloced( n ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < n; i++ ) {
		p[i].ToVec3() = verts[i];
		p[i].s = p[i].t = 0;
	}
	numPoints = n;
}

ID_INLINE idFixedWinding::idFixedWinding( const idVec3 &normal, const float dist ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	BaseForPlane( normal, dist );
}

ID_INLINE idFixedWinding::idFixedWinding( const idPlane &plane ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	BaseForPlane( plane );
}

ID_INLINE idFixedWinding::idFixedWinding( const idWinding &winding ) {
	int i;

	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

ID_INLINE idFixedWinding::idFixedWinding( const idFixedWinding &winding ) {
	int i;

	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

ID_INLINE idFixedWinding::~idFixedWinding( void ) {
	p = NULL;	// otherwise it tries to free the fixed buffer
}

ID_INLINE idFixedWinding &idFixedWinding::operator=( const idWinding &winding ) {
	int i;

	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return *this;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
	return *this;
}

ID_INLINE void idFixedWinding::ClearFree( void ) {
	numPoints = 0;
}
#endif	/* !__WINDING_H__ */
