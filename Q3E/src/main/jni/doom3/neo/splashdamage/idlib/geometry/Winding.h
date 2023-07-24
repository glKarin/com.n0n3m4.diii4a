// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __WINDING_H__
#define __WINDING_H__

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
					explicit idWinding( const idVec3 &normal, const float dist, const float radius );	// base winding for plane
					explicit idWinding( const idPlane &plane, const float radius );						// base winding for plane
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
	virtual void	Clear( void );

	void			Rotate( const idVec3& origin, const idMat3& axis );

					// huge winding for plane, the points go counter clockwise when facing the front of the plane
	void			BaseForPlane( const idVec3 &normal, const float dist, const float radius );
	void			BaseForPlane( const idPlane &plane, const float radius );

					// splits the winding into a front and back winding, the winding itself stays unchanged
					// returns a SIDE_?
	int				Split( const idPlane &plane, const float epsilon, idWinding **front, idWinding **back ) const;
					
					// idential to the above version, but avoids heap allocations
	int				Split( const idPlane &plane, const float epsilon, idWinding& front, idWinding& back ) const;

					// chops of the part of the winding at the back of the plane
					// if there is nothing at the front the number of points is set to zero, returns a SIDE_?
	int				SplitInPlace( const idPlane &plane, const float epsilon, idWinding **back );
					// cuts off the part at the back side of the plane, returns true if some part was at the front
					// if there is nothing at the front the number of points is set to zero
	bool			ClipInPlace( const idPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

					// splits the winding into a front and back winding, the winding itself stays unchanged
					// returns a SIDE_?
	int				SplitWithEdgeNums( const int *edgeNums, const idPlane &plane, const int edgeNum, const float epsilon,
												idWinding **front, idWinding **back, int **frontEdgePlanes, int **backEdgePlanes ) const;
					// chops of the part of the winding at the back of the plane
					// if there is nothing at the front the number of points is set to zero, returns a SIDE_?
	int				SplitInPlaceWithEdgeNums( idList<int> &edgeNums, const idPlane &plane, const float epsilon,
												idWinding **back, int **backEdgePlanes );
					// cuts off the part at the back side of the plane, returns true if some part was at the front
					// if there is nothing at the front the number of points is set to zero
	int				ClipInPlaceWithEdgeNums( idList<int> &edgeNums, const idPlane &plane, const int edgeNum,
												const float epsilon = ON_EPSILON, const bool keepOn = false );

					// returns a copy of the winding
	idWinding *		Copy( void ) const;
	idWinding *		Reverse( void ) const;
	void			ReverseSelf( void );
	void			RemoveEqualPoints( const float epsilon = ON_EPSILON );
	void			RemoveColinearPoints( const idVec3 &normal, const float epsilon = ON_EPSILON );
	void			RemovePoint( int point );
	void			InsertPoint( const idVec3 &point, int spot );
	bool			InsertPointIfOnEdge( const idVec3 &point, const idPlane &plane, const float epsilon = ON_EPSILON, int *index = NULL );
					// add a winding to the convex hull
	void			AddToConvexHull( const idWinding *winding, const idVec3 &normal, const float epsilon = ON_EPSILON );
					// add a point to the convex hull
	void			AddToConvexHull( const idVec3 &point, const idVec3 &normal, const float epsilon = ON_EPSILON );
					// tries to merge 'this' with the given winding, returns NULL if merge fails, both 'this' and 'w' stay intact
					// 'keep' tells if the contacting points should stay even if they create colinear edges
	idWinding *		TryMerge( const idWinding &w, const idVec3 &normal, int keep = false ) const;
					// The parameter indices should point to an array with at least GetNumPoints() * 3 integers.
					// If no triangle fan could be created from one of the corners an assumed center of the
					// winding is used to create a fan and the indices referncing this additional vertex will
					// equal GetNumPoints() and indices[0] will always be equal to GetNumPoints().
					// Returns the number of indices written out.
	int				CreateTriangles( int *indices, const float epsilon ) const;
					// check whether the winding is valid or not
	bool			Check( bool print = true ) const;

	float			GetArea( void ) const;
	idVec3			GetCenter( void ) const;
	idVec3			GetNormal( void ) const;
	float			GetRadius( const idVec3 &center ) const;
	void			GetPlane( idVec3 &normal, float &dist ) const;
	void			GetPlane( idPlane &plane ) const;
	void			GetBounds( idBounds &bounds ) const;

	idPlane			GetPlane() const;
	idBounds		GetBounds() const;

	bool			IsTiny( void ) const;
	bool			IsHuge( void ) const;	// base winding for a plane is typically huge

	bool			IsTiny( float epsilon ) const;
	bool			IsHuge( float radius ) const;

	void			Print( void ) const;

	float			PlaneDistance( const idPlane &plane ) const;
	int				PlaneSide( const idPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			PlanesConcave( const idWinding &w2, const idVec3 &normal1, const idVec3 &normal2, float dist1, float dist2 ) const;

	bool			PointInside( const idVec3 &normal, const idVec3 &point, const float epsilon ) const;
					// returns true if the line or ray intersects the winding
	bool			LineIntersection( const idPlane &windingPlane, const idVec3 &start, const idVec3 &end, bool backFaceCull = false ) const;
					// intersection point is start + dir * scale
	bool			RayIntersection( const idPlane &windingPlane, const idVec3 &start, const idVec3 &dir, float &scale, bool backFaceCull = false ) const;

	static float	TriangleArea( const idVec3 &a, const idVec3 &b, const idVec3 &c );

	virtual bool	ReAllocate( int n, bool keep = false );

protected:
	int				numPoints;				// number of points
	idVec5 *		p;						// pointer to point data
	int				allocedSize;

	bool			EnsureAlloced( int n, bool keep = false );
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

ID_INLINE idWinding::idWinding( const idVec3 &normal, const float dist, const float radius ) {
	numPoints = allocedSize = 0;
	p = NULL;
	BaseForPlane( normal, dist, radius );
}

ID_INLINE idWinding::idWinding( const idPlane &plane, const float radius ) {
	numPoints = allocedSize = 0;
	p = NULL;
	BaseForPlane( plane, radius );
}

ID_INLINE idWinding::idWinding( const idWinding &winding ) {
	numPoints = allocedSize = 0;
	p = NULL;

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

ID_INLINE const idVec5 &idWinding::operator[]( const int index ) const {
	//assert( index >= 0 && index < numPoints );
	return p[ index ];
}

ID_INLINE idVec5 &idWinding::operator[]( const int index ) {
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

ID_INLINE int idWinding::GetNumPoints( void ) const {
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
	allocedSize = 0;
	delete[] p;
	p = NULL;
}

ID_INLINE void idWinding::BaseForPlane( const idPlane &plane, const float radius ) {
	BaseForPlane( plane.Normal(), plane.Dist(), radius );
}

ID_INLINE bool idWinding::EnsureAlloced( int n, bool keep ) {
	if ( n > allocedSize ) {
		return ReAllocate( n, keep );
	}
	return true;
}

ID_INLINE void idWinding::Rotate( const idVec3& origin, const idMat3& axis ) {
	int i;
	for( i = 0; i < numPoints; i++ ) {
		p[ i ].ToVec3() -= origin;
		p[ i ].ToVec3() *= axis;
		p[ i ].ToVec3() += origin;
	}
}

ID_INLINE float idWinding::TriangleArea( const idVec3 &a, const idVec3 &b, const idVec3 &c ) {
	idVec3 v1 = b - a;
	idVec3 v2 = c - a;
	idVec3 cross = v1.Cross( v2 );
	return 0.5f * cross.Length();
}


#define	MAX_POINTS_ON_WINDING	64

/*
===============================================================================

	idGrowingWinding 

===============================================================================
*/

class idGrowingWinding : public idWinding {
public:

	idGrowingWinding();
	~idGrowingWinding();

	virtual void	Clear( void );
	virtual bool	ReAllocate( int n, bool keep = false );

private:

	idVec5			data[MAX_POINTS_ON_WINDING];	// point data

};

ID_INLINE idGrowingWinding::idGrowingWinding() {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
}

ID_INLINE idGrowingWinding::~idGrowingWinding() {
	if ( p == data ) {
		p = NULL;
	}
}

ID_INLINE void idGrowingWinding::Clear( void ) {
	numPoints = 0;
	allocedSize = MAX_POINTS_ON_WINDING;
	if ( p != data ) {
		delete[] p;
	}
	p = data;
}


/*
===============================================================================

	idFixedWinding is a fixed buffer size winding not using
	memory allocations.

	When an operation would overflow the fixed buffer a warning
	is printed and the operation is safely cancelled.

===============================================================================
*/

class idFixedWinding : public idWinding {

public:
					idFixedWinding( void );
					explicit idFixedWinding( const int n );
					explicit idFixedWinding( const idVec3 *verts, const int n );
					explicit idFixedWinding( const idVec3 &normal, const float dist, const float radius );
					explicit idFixedWinding( const idPlane &plane, const float radius );
					explicit idFixedWinding( const idWinding &winding );
					explicit idFixedWinding( const idFixedWinding &winding );
	virtual			~idFixedWinding( void );

	idFixedWinding &operator=( const idWinding &winding );

	virtual void	Clear( void );

					// chops off the part of the winding at the back of the plane
					// if there is nothing at the front the number of points is set to zero, returns a SIDE_?
	int				SplitInPlace( const idPlane &plane, const float epsilon, idFixedWinding *back );

	virtual bool	ReAllocate( int n, bool keep = false );

protected:
	idVec5			data[MAX_POINTS_ON_WINDING];	// point data
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

ID_INLINE idFixedWinding::idFixedWinding( const idVec3 &normal, const float dist, const float radius ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	BaseForPlane( normal, dist, radius );
}

ID_INLINE idFixedWinding::idFixedWinding( const idPlane &plane, const float radius ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	BaseForPlane( plane, radius );
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

ID_INLINE void idFixedWinding::Clear( void ) {
	numPoints = 0;
}

#endif	/* !__WINDING_H__ */
