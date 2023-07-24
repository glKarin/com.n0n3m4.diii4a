// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __WINDING2D_H__
#define __WINDING2D_H__

/*
===============================================================================

	A 2D winding is an arbitrary convex 2D polygon defined by an array of points.

===============================================================================
*/

class idWinding2D {
public:
	static const int MAX_POINTS = 32;
					idWinding2D( void );
					idWinding2D( const int numPoints, const idVec2* points, const idVec2* st );

	idWinding2D &	operator=( const idWinding2D &winding );
	const idVec2 &	operator[]( const int index ) const;
	idVec2 &		operator[]( const int index );

	idVec2&			GetST( int index );
	const idVec2&	GetST( int index ) const;

	void			Clear( void );
	void			AddPoint( const idVec2 &point );
	void			AddPoint( float x, float y );
	void			AddPoint( float x, float y, float s, float t );
	void			AddPoint( const idVec2 &point, const idVec2 &st );
	void			SetPointST( int index, float s, float t );
	int				GetNumPoints( void ) const;

	void			Expand( const float d );
	void			ExpandForAxialBox( const idVec2 bounds[2] );

					// splits the winding into a front and back winding, the winding itself stays unchanged
					// returns a SIDE_?
	int				Split( const idVec3 &plane, const float epsilon, idWinding2D **front, idWinding2D **back ) const;
					// cuts off the part at the back side of the plane, returns true if some part was at the front
					// if there is nothing at the front the number of points is set to zero
	bool			ClipInPlace( const idVec3 &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

	bool			SplitEdgesByLine( const idVec2& start, const idVec2& end, const float epsilon = ON_EPSILON );
	bool			ClipByBounds( const sdBounds2D& bounds, const float epsilon = ON_EPSILON );

	idWinding2D *	Copy( void ) const;
	idWinding2D *	Reverse( void ) const;
	idWinding2D&	ReverseSelf( void );

	float			GetArea( void ) const;
	idVec2			GetCenter( void ) const;
	float			GetRadius( const idVec2 &center ) const;
	void			GetBounds( idVec2 bounds[2] ) const;
	void			GetBounds( sdBounds2D& bounds ) const;
	void			GetBoundsST( sdBounds2D& bounds ) const;

	bool			IsTiny( void ) const;
	bool			IsHuge( void ) const;	// base winding for a plane is typically huge
	void			Print( void ) const;

	float			PlaneDistance( const idVec3 &plane ) const;
	int				PlaneSide( const idVec3 &plane, const float epsilon = ON_EPSILON ) const;

	bool			PointInside( const idVec2 &point, const float epsilon ) const;
	bool			LineIntersection( const idVec2 &start, const idVec2 &end ) const;
	bool			RayIntersection( const idVec2 &start, const idVec2 &dir, float &scale1, float &scale2, int *edgeNums = NULL ) const;

	static idVec3	Plane2DFromPoints( const idVec2 &start, const idVec2 &end, const bool normalize = false );
	static idVec3	Plane2DFromVecs( const idVec2 &start, const idVec2 &dir, const bool normalize = false );
	static bool		Plane2DIntersection( const idVec3 &plane1, const idVec3 &plane2, idVec2 &point );

	void			Rotation( const idVec2& org, float angle );	
	void			RotationST( const idVec2& org, float angle );
	void			Scale( const idVec2& scale );

private:
	int				numPoints;
	idVec2			p[idWinding2D::MAX_POINTS];
	idVec2			st[idWinding2D::MAX_POINTS];
};

ID_INLINE idWinding2D::idWinding2D( void ) {
	numPoints = 0;
}

ID_INLINE idWinding2D::idWinding2D( const int numPoints, const idVec2* points, const idVec2* st ) :
	numPoints( numPoints ) {
	::memcpy( p, points, numPoints * sizeof( idVec2 ) );
	::memcpy( this->st, st, numPoints * sizeof( idVec2 ) );
}

ID_INLINE idWinding2D &idWinding2D::operator=( const idWinding2D &winding ) {
	int i;

	for ( i = 0; i < winding.numPoints; i++ ) {
		p[i] = winding.p[i];
		st[i] = winding.st[i];
	}
	numPoints = winding.numPoints;
	return *this;
}

ID_INLINE const idVec2 &idWinding2D::operator[]( const int index ) const {
	return p[ index ];
}

ID_INLINE idVec2 &idWinding2D::operator[]( const int index ) {
	return p[ index ];
}

ID_INLINE idVec2&	idWinding2D::GetST( int index ) {
	return st[ index ];
}

ID_INLINE const idVec2&	idWinding2D::GetST( int index ) const {
	return st[ index ];
}


ID_INLINE void idWinding2D::Clear( void ) {
	numPoints = 0;
}

ID_INLINE void idWinding2D::AddPoint( const idVec2 &point ) {
	if ( numPoints >= idWinding2D::MAX_POINTS ) {
		assert( false );
		return;
	}
	p[numPoints++] = point;
}

ID_INLINE void idWinding2D::AddPoint( const idVec2 &point, const idVec2 &_st ) {
	if ( numPoints >= idWinding2D::MAX_POINTS ) {
		assert( false );
		return;
	}
	p[numPoints] = point;
	st[numPoints] = _st;
	numPoints++;
}

ID_INLINE void idWinding2D::SetPointST( int index, float s, float t ) {
	st[ index ].x = s;
	st[ index ].y = t;
}

ID_INLINE void idWinding2D::AddPoint( float x, float y ) {
	p[numPoints].x = x;
	p[numPoints].y = y;
	numPoints++;
}

ID_INLINE void idWinding2D::AddPoint( float x, float y, float s, float t ) {
	p[numPoints].x = x;
	p[numPoints].y = y;
	st[numPoints].x = s;
	st[numPoints].y = t;
	numPoints++;
}

ID_INLINE int idWinding2D::GetNumPoints( void ) const {
	return numPoints;
}

ID_INLINE idVec3 idWinding2D::Plane2DFromPoints( const idVec2 &start, const idVec2 &end, const bool normalize ) {
	idVec3 plane;
	plane.x = start.y - end.y;
	plane.y = end.x - start.x;
	if ( normalize ) {
		plane.ToVec2().Normalize();
	}
	plane.z = - ( start.x * plane.x + start.y * plane.y );
	return plane;
}

ID_INLINE idVec3 idWinding2D::Plane2DFromVecs( const idVec2 &start, const idVec2 &dir, const bool normalize ) {
	idVec3 plane;
	plane.x = -dir.y;
	plane.y = dir.x;
	if ( normalize ) {
		plane.ToVec2().Normalize();
	}
	plane.z = - ( start.x * plane.x + start.y * plane.y );
	return plane;
}

ID_INLINE bool idWinding2D::Plane2DIntersection( const idVec3 &plane1, const idVec3 &plane2, idVec2 &point ) {
	float n00, n01, n11, det, invDet, f0, f1;

	n00 = plane1.x * plane1.x + plane1.y * plane1.y;
	n01 = plane1.x * plane2.x + plane1.y * plane2.y;
	n11 = plane2.x * plane2.x + plane2.y * plane2.y;
	det = n00 * n11 - n01 * n01;

	if ( idMath::Fabs(det) < 1e-6f ) {
		return false;
	}

	invDet = 1.0f / det;
	f0 = ( n01 * plane2.z - n11 * plane1.z ) * invDet;
	f1 = ( n01 * plane1.z - n00 * plane2.z ) * invDet;
	point.x = f0 * plane1.x + f1 * plane2.x;
	point.y = f0 * plane1.y + f1 * plane2.y;
	return true;
}

ID_INLINE void idWinding2D::GetBounds( sdBounds2D& bounds ) const {
	bounds.Clear();

	if ( !numPoints ) {		
		return;
	}

	for ( int i = 0; i < numPoints; i++ ) {
		bounds.AddPoint( p[ i ] );
	}
}

ID_INLINE void idWinding2D::GetBoundsST( sdBounds2D& bounds ) const {
	bounds.Clear();
	if ( !numPoints ) {		
		return;
	}

	for ( int i = 0; i < numPoints; i++ ) {
		bounds.AddPoint( st[ i ] );
	}
}

#endif /* !__WINDING2D_H__ */
