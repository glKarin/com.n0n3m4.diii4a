// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __BV_BOUNDS2D_H__
#define __BV_BOUNDS2D_H__

class sdBounds2D {
public:

	enum eSide		{ SIDE_LEFT		= BITT< 0 >::VALUE,
					  SIDE_RIGHT	= BITT< 1 >::VALUE, 
					  SIDE_TOP		= BITT< 2 >::VALUE,
					  SIDE_BOTTOM	= BITT< 3 >::VALUE,
					  SIDE_INTERIOR = BITT< 4 >::VALUE 
	};

	enum eScreenSpace { SPACE_INCREASE_FROM_TOP,
						SPACE_DECREASE_FROM_TOP
	};

					sdBounds2D();
					sdBounds2D( const idBounds& rhs, size_t ignoreAxis = 2 );
					sdBounds2D( const float x, const float y, const float w, const float h );
					sdBounds2D( const idVec2& mins, const idVec2& maxs );
	explicit		sdBounds2D( const idVec4& vec );
	void			Zero();

	const idVec2&	operator[]( int index ) const;
	idVec2&			operator[]( int index );
	sdBounds2D		operator+( const sdBounds2D &rhs ) const;
	sdBounds2D &	operator+=( const sdBounds2D &rhs );
	sdBounds2D		operator*=( const idVec2& s ) const;
	sdBounds2D&		operator*=( const idVec2& s );
	bool			operator==( const sdBounds2D& rhs ) const;
	bool			operator!=( const sdBounds2D& rhs ) const;

	idVec4			ToVec4();
	void			FromRectangle( const idVec4& rect );
	void			FromRectangle( const float x, const float y, const float w, const float h );
	void			Clear();

	bool			AddPoint( const idVec2& p );
	bool			AddBounds( const sdBounds2D &rhs );
	
	int				GetLargestAxis( void ) const;

	idVec2&			GetMins();
	const idVec2&	GetMins() const;

	idVec2&			GetMaxs();
	const idVec2&	GetMaxs() const;

	bool			Compare( const sdBounds2D& rhs ) const;	

	idVec2			GetCenter() const;

	bool			IsCleared() const;
	bool			IsCollapsed( float epsilon = VECTOR_EPSILON ) const;

	bool			ContainsPoint( const idVec2& point ) const;
	bool			ContainsBounds( const sdBounds2D& bounds ) const;
	int				SideForPoint( const idVec2& point, eScreenSpace space = SPACE_INCREASE_FROM_TOP ) const;
	bool			IntersectsBounds( const sdBounds2D& other ) const;
	void			IntersectBounds( const sdBounds2D& other, sdBounds2D& result ) const;

	float			GetWidth() const;
	float			GetHeight() const;

	float&			GetLeft();
	float&			GetRight();

	float&			GetTop( eScreenSpace space = SPACE_INCREASE_FROM_TOP );
	float&			GetBottom( eScreenSpace space = SPACE_INCREASE_FROM_TOP );

	float			GetLeft() const;
	float			GetRight() const;

	float			GetTop( eScreenSpace space = SPACE_INCREASE_FROM_TOP ) const;
	float			GetBottom( eScreenSpace space = SPACE_INCREASE_FROM_TOP ) const;

	void			TranslateSelf( const float xOffset, const float yOffset );
	void			TranslateSelf( const idVec2& offset );

	idVec2			GetSize( void ) const;

	void			ExpandSelf( const float xOffset, const float yOffset );
	void			ExpandSelf( const idVec2& offset );

    void			MakeValid();

private:
	idVec2 bounds[ 2 ];
};

extern sdBounds2D bounds2d_zero;


/*
============
sdBounds2D::sdBounds2D
============
*/
ID_INLINE sdBounds2D::sdBounds2D( const float x, const float y, const float w, const float h ) {
	FromRectangle( x, y, w, h );
}


/*
============
sdBounds2D::sdBounds2D
============
*/
ID_INLINE sdBounds2D::sdBounds2D( const idVec4& vec ) {
	FromRectangle( vec.x, vec.y, vec.z, vec.w );
}


/*
============
sdBounds2D::FromRectangle
============
*/
ID_INLINE void sdBounds2D::FromRectangle( const float x, const float y, const float w, const float h ) {
	bounds[ 0 ][ 0 ] = x;
	bounds[ 1 ][ 0 ] = x + w;
	bounds[ 0 ][ 1 ] = y;
	bounds[ 1 ][ 1 ] = y + h;
}

/*
============
sdBounds2D::GetWidth
============
*/
ID_INLINE float	sdBounds2D::GetWidth() const {
	return bounds[ 1 ][ 0 ] - bounds[ 0 ][ 0 ];
}

/*
============
sdBounds2D::GetHeight
============
*/
ID_INLINE float	sdBounds2D::GetHeight() const {
	return bounds[ 1 ][ 1 ] - bounds[ 0 ][ 1 ];
}

/*
============
sdBounds2D::operator[]
============
*/
ID_INLINE const idVec2& sdBounds2D::operator[]( int index ) const {
	return bounds[ index ];
}

/*
============
sdBounds2D::operator[]
============
*/
ID_INLINE idVec2&	sdBounds2D::operator[]( int index ) {
	return bounds[index];
}

/*
============
sdBounds2D::Zero
============
*/
ID_INLINE void sdBounds2D::Zero() {
	bounds[ 0 ] = bounds[ 1 ] = vec2_zero;
}

/*
============
sdBounds2D::GetMins
============
*/
ID_INLINE const idVec2& sdBounds2D::GetMins() const {
	return bounds[ 0 ];
}

/*
============
sdBounds2D::GetMaxs
============
*/
ID_INLINE const idVec2& sdBounds2D::GetMaxs() const {
	return bounds[ 1 ];
}

/*
============
sdBounds2D::GetMins
============
*/
ID_INLINE idVec2& sdBounds2D::GetMins() {
	return bounds[ 0 ];
}

/*
============
sdBounds2D::GetMaxs
============
*/
ID_INLINE idVec2& sdBounds2D::GetMaxs() {
	return bounds[ 1 ];
}

/*
============
sdBounds2D::GetCenter
============
*/
ID_INLINE idVec2 sdBounds2D::GetCenter() const {
	return bounds[ 0 ] + ( ( bounds[ 1 ] - bounds[ 0 ] ) * 0.5f );
}

/*
============
sdBounds2D::operator+
============
*/
ID_INLINE sdBounds2D sdBounds2D::operator+( const sdBounds2D &rhs ) const {
	sdBounds2D newBounds;
	newBounds = *this;
	newBounds.AddBounds( rhs );
	return newBounds;
}

/*
============
sdBounds2D::operator+=
============
*/
ID_INLINE sdBounds2D &sdBounds2D::operator+=( const sdBounds2D &rhs ) {
	sdBounds2D::AddBounds( rhs );
	return *this;
}

/*
============
sdBounds2D::AddBounds
============
*/
ID_INLINE bool sdBounds2D::AddBounds( const sdBounds2D &rhs ) {
	bool expanded = false;
	if ( rhs.bounds[ 0 ][ 0 ] < bounds[ 0 ][ 0 ] ) {
		bounds[ 0 ][ 0 ] = rhs.bounds[ 0 ][ 0 ];
		expanded = true;
	}
	if ( rhs.bounds[ 0 ][ 1 ] < bounds[ 0 ][ 1 ] ) {
		bounds[ 0 ][ 1 ] = rhs.bounds[ 0 ][ 1 ];
		expanded = true;
	}
	if ( rhs.bounds[ 1 ][ 0 ] > bounds[ 1 ][ 0 ] ) {
		bounds[ 1 ][ 0 ] = rhs.bounds[ 1 ][ 0 ];
		expanded = true;
	}
	if ( rhs.bounds[ 1 ][ 1 ] > bounds[ 1 ][ 1 ] ) {
		bounds[ 1 ][ 1 ] = rhs.bounds[ 1 ][ 1 ];
		expanded = true;
	}
	return expanded;
}

/*
============
sdBounds2D::Clear
============
*/
ID_INLINE void sdBounds2D::Clear(){
	bounds[ 0 ].Set( idMath::INFINITY, idMath::INFINITY );
	bounds[ 1 ].Set( -idMath::INFINITY, -idMath::INFINITY );
}

/*
============
sdBounds2D::IsCleared
============
*/
ID_INLINE bool sdBounds2D::IsCleared() const {
	return( bounds[ 0 ][ 0 ] > bounds[ 1 ][ 0 ] );
}

/*
============
sdBounds2D::IsCollapsed
============
*/
ID_INLINE bool sdBounds2D::IsCollapsed( float epsilon ) const {
	return(	( idMath::Fabs( bounds[ 1 ][ 0 ] - bounds[ 0 ][ 0 ] ) < epsilon )  || 
			( idMath::Fabs( bounds[ 1 ][ 1 ] - bounds[ 0 ][ 1 ] ) < epsilon ) );
}

/*
============
sdBounds2D::operator*=
============
*/
ID_INLINE sdBounds2D sdBounds2D::operator*=( const idVec2& s ) const {
	return sdBounds2D( idVec2( bounds[ 0 ][ 0 ] * s[ 0 ], bounds[ 0 ][ 1 ] * s[ 1 ] ), idVec2( bounds[ 1 ][ 0 ] * s[ 0 ], bounds[ 1 ][ 1 ] * s[ 1 ] ) );
}

/*
============
sdBounds2D::Compare
============
*/
ID_INLINE bool sdBounds2D::Compare( const sdBounds2D& rhs ) const {
	return ( bounds[ 0 ].Compare( rhs.bounds[ 0 ] ) && bounds[ 1 ].Compare( rhs.bounds[ 1 ] ) );
}

/*
============
sdBounds2D::operator*=
============
*/
ID_INLINE sdBounds2D & sdBounds2D::operator*=( const idVec2& s ) {
	this->bounds[ 0 ][ 0 ] *= s[ 0 ];
	this->bounds[ 0 ][ 1 ] *= s[ 1 ];
	this->bounds[ 1 ][ 0 ] *= s[ 0 ];
	this->bounds[ 1 ][ 1 ] *= s[ 1 ];
	return *this;
}


/*
============
sdBounds2D::operator==
============
*/
ID_INLINE bool sdBounds2D::operator==( const sdBounds2D& rhs ) const {
	return Compare( rhs );
}

/*
============
sdBounds2D::operator!=
============
*/
ID_INLINE bool sdBounds2D::operator!=( const sdBounds2D& rhs ) const {
	return !Compare( rhs );
}

/*
============
sdBounds2D::GetLargestAxis
============
*/
ID_INLINE int sdBounds2D::GetLargestAxis( void ) const
{
	idVec2 work = bounds[ 1 ] - bounds[ 0 ];
	int axis = 0;

	if ( work[ 1 ] > work[ 0 ] ) {
		axis = 1;
	}
	return( axis );
}


/*
============
sdBounds2D::MakeValid
============
*/
ID_INLINE void sdBounds2D::MakeValid() {
	if( bounds[ 0 ].x > bounds[ 1 ].x ) {
		idSwap(bounds[ 0 ].x, bounds[ 1 ].x );
	}
	if( bounds[ 0 ].y > bounds[ 1 ].y ) {
		idSwap(bounds[ 0 ].y, bounds[ 1 ].y );
	}
}


/*
============
sdBounds2D::SideForPoint
============
*/
ID_INLINE int sdBounds2D::SideForPoint( const idVec2& point, eScreenSpace space ) const {
	int sides = 0;
	if( point.x < bounds[ 0 ].x ) {
		sides |= SIDE_LEFT;
	} else if( point.x > bounds[ 1 ].x ) {
		sides |= SIDE_RIGHT;
	}

	if( point.y < bounds[ 0 ].y ) {
		sides |= ( space == SPACE_INCREASE_FROM_TOP ) ? SIDE_TOP : SIDE_BOTTOM;
	} else if( point.y > bounds[ 1 ].y ) {
		sides |= ( space == SPACE_INCREASE_FROM_TOP ) ? SIDE_BOTTOM : SIDE_TOP;
	}

	if( sides == 0 ) {
		sides = SIDE_INTERIOR;
	}

	return sides;
}

/*
============
sdBounds2D::sdBounds2D
============
*/
ID_INLINE sdBounds2D::sdBounds2D () {
}

/*
============
sdBounds2D::sdBounds2D
============
*/
ID_INLINE sdBounds2D::sdBounds2D( const idBounds& rhs, size_t ignoreAxis ) {
	const idVec3& mins = rhs.GetMins();
	const idVec3& maxs = rhs.GetMaxs();

	switch( ignoreAxis ) {
		case 0:
			bounds[ 0 ][ 0 ] = mins[ 1 ];
			bounds[ 1 ][ 0 ] = maxs[ 1 ];	

			bounds[ 0 ][ 1 ] = mins[ 2 ];
			bounds[ 1 ][ 1 ] = maxs[ 2 ];
			break;
		case 1:
			bounds[ 0 ][ 0 ] = mins[ 0 ];
			bounds[ 1 ][ 0 ] = maxs[ 0 ];	

			bounds[ 0 ][ 1 ] = mins[ 2 ];
			bounds[ 1 ][ 1 ] = maxs[ 2 ];
			break;
		case 2:
			bounds[ 0 ][ 0 ] = mins[ 0 ];
			bounds[ 1 ][ 0 ] = maxs[ 0 ];	

			bounds[ 0 ][ 1 ] = mins[ 1 ];
			bounds[ 1 ][ 1 ] = maxs[ 1 ];
			break;
	}
}

/*
============
sdBounds2D::sdBounds2D
============
*/
ID_INLINE sdBounds2D::sdBounds2D( const idVec2& mins, const idVec2& maxs ) {
	bounds[ 0 ] = mins;
	bounds[ 1 ] = maxs;
}

/*
============
sdBounds2D::AddPoint
============
*/
ID_INLINE bool sdBounds2D::AddPoint( const idVec2& p ) {
	bool expanded = false;
	for( int j = 0; j < 2; j++ ) {
		if( p[j] < bounds[ 0 ][j] ) {
			bounds[ 0 ][j] = p[j];
			expanded = true;
		}
		if( p[j] > bounds[ 1 ][j] ) {
			bounds[ 1 ][j] = p[j];
			expanded = true;
		}
	}
	return expanded;
}

/*
============
sdBounds2D::ContainsPoint
============
*/
ID_INLINE bool sdBounds2D::ContainsPoint( const idVec2& point ) const {
	bool contained = true;
	for(  int i = 0; i < 2; i++ ) {
		if( point[ i ] < bounds[ 0 ][ i ] ) {
			contained = false;
			break;
		}
		if( point[ i ] > bounds[ 1 ][ i ] ) {
			contained = false;
			break;
		}
	}
	return contained;
}


/*
============
sdBounds2D::ContainsBounds
============
*/
ID_INLINE bool sdBounds2D::ContainsBounds( const sdBounds2D& bounds ) const {
	return (( bounds.GetMins().x >= GetMins().x ) &&
			( bounds.GetMins().y >= GetMins().y ) &&
			( bounds.GetMaxs().x <= GetMaxs().x ) &&
			( bounds.GetMaxs().y <= GetMaxs().y ) );
}

/*
============
sdBounds2D::IntersectsBounds
============
*/
ID_INLINE bool sdBounds2D::IntersectsBounds( const sdBounds2D& other ) const {
	if ( &other == this ) {
		return true;
	}

	if ( other.bounds[ 1 ][ 0 ] < bounds[ 0 ][ 0 ] || other.bounds[ 1 ][ 1 ] < bounds[ 0 ][ 1 ]
	|| other.bounds[ 0 ][ 0 ] > bounds[ 1 ][ 0 ] || other.bounds[ 0 ][ 1 ] > bounds[ 1 ][ 1 ] ) {
		return false;
	}

	return true;
}

/*
============
sdBounds2D::IntersectBounds
============
*/
ID_INLINE void sdBounds2D::IntersectBounds( const sdBounds2D& other, sdBounds2D& result ) const {
	if( &other == this ) {
		result = *this;
		return;
	}

	result.Clear();

	for( int i = 0; i < 2; i++ ) {
		if( other.bounds[ 0 ][ i ] < bounds[ 0 ][ i ] ) {
			result[ 0 ][ i ] = bounds[ 0 ][ i ];
		} else {
			result[ 0 ][ i ] = other.bounds[ 0 ][ i ];
		}

		if( other.bounds[ 1 ][ i ] > bounds[ 1 ][ i ] ) {
			result[ 1 ][ i ] = bounds[ 1 ][ i ];
		} else {
			result[ 1 ][ i ] = other.bounds[ 1 ][ i ];
		}
	}
}

/*
============
sdBounds2D::TranslateSelf
============
*/
ID_INLINE void sdBounds2D::TranslateSelf( const idVec2& offset ) {
	TranslateSelf( offset.x, offset.y );
}

/*
============
sdBounds2D::TranslateSelf
============
*/
ID_INLINE void sdBounds2D::TranslateSelf( const float xOffset, const float yOffset ) {
	GetMins().x += xOffset;
	GetMaxs().x += xOffset;

	GetMins().y += yOffset;
	GetMaxs().y += yOffset;
}

/*
============
sdBounds2D::ExpandSelf
============
*/
ID_INLINE idVec2 sdBounds2D::GetSize( void ) const {
	return GetMaxs() - GetMins();
}

/*
============
sdBounds2D::ExpandSelf
============
*/
ID_INLINE void sdBounds2D::ExpandSelf( const float xOffset, const float yOffset ) {
	GetMins().x -= xOffset;
	GetMaxs().x += xOffset;

	GetMins().y -= yOffset;
	GetMaxs().y += yOffset;
}

/*
============
sdBounds2D::ExpandSelf
============
*/
ID_INLINE void sdBounds2D::ExpandSelf( const idVec2& offset ) {
	ExpandSelf( offset.x, offset.y );
}

/*
============
sdBounds2D::GetLeft
============
*/
ID_INLINE float	sdBounds2D::GetLeft() const {
	return GetMins().x;
}

/*
============
sdBounds2D::Right
============
*/
ID_INLINE float	sdBounds2D::GetRight() const {
	return GetMaxs().x;
}

/*
============
sdBounds2D::GetTop
============
*/
ID_INLINE float	sdBounds2D::GetTop( eScreenSpace space ) const {
	return ( space == SPACE_INCREASE_FROM_TOP ) ? GetMins().y : GetMaxs().y;
}

/*
============
sdBounds2D::GetBottom
============
*/
ID_INLINE float	sdBounds2D::GetBottom( eScreenSpace space ) const {
	return ( space == SPACE_INCREASE_FROM_TOP ) ? GetMaxs().y : GetMins().y;
}

/*
============
sdBounds2D::GetLeft
============
*/
ID_INLINE float& sdBounds2D::GetLeft() {
	return GetMins().x;
}

/*
============
sdBounds2D::GetRight
============
*/
ID_INLINE float& sdBounds2D::GetRight() {
	return GetMaxs().x;
}

/*
============
sdBounds2D::GetTop
============
*/
ID_INLINE float& sdBounds2D::GetTop( eScreenSpace space ) {
	return ( space == SPACE_INCREASE_FROM_TOP ) ? GetMins().y : GetMaxs().y;
}

/*
============
sdBounds2D::GetBottom
============
*/
ID_INLINE float& sdBounds2D::GetBottom( eScreenSpace space ) {
	return ( space == SPACE_INCREASE_FROM_TOP ) ? GetMaxs().y : GetMins().y;
}


/*
============
sdBounds2D::ToVec4
============
*/
ID_INLINE idVec4 sdBounds2D::ToVec4() {
	return idVec4( GetMins().x, GetMins().y, GetWidth(), GetHeight() );
}

/*
============
sdBounds2D::FromRectangle
============
*/
ID_INLINE void sdBounds2D::FromRectangle( const idVec4& rect ) {
	FromRectangle( rect.x, rect.y, rect.z, rect.w );
}

#endif /* ! __BV_BOUNDS2D_H__ */
