// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BV_BOUNDSSHORT_H__
#define __BV_BOUNDSSHORT_H__

/*
===============================================================================

	Axis Aligned Bounding Box

===============================================================================
*/

class idBoundsShort {
public:
					idBoundsShort( void );
					explicit idBoundsShort( const idBounds &bounds );

	void			SetBounds( const idBounds &bounds );

	void            SetBounds( const short *list );
	const short *   GetBounds( void );

	void			AddPoint( const idVec3& a );
	void			AddBounds( const idBoundsShort& a );

	void			Combine( const idBoundsShort& x, const idBoundsShort& y );

	void			Clear( void );									// inside out bounds
	void			Zero( void );									// single point at origin
	void			Zero( const idVec3 &center );					// single point at center

	bool			IsCleared( void ) const;						// returns true if bounds are inside out

	float			PlaneDistance( const idPlane &plane ) const;
	int				PlaneSide( const idPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const idVec3 &p ) const;
	bool			ContainsPoint2D( const idVec3 &p ) const;
	bool			IntersectsBounds( const idBounds &a ) const;
	bool			IntersectsBounds( const idBoundsShort &a ) const;
	bool			IntersectsBounds2D( const idBoundsShort &a ) const;

	int				GetLargestAxis( void ) const;

	const short*	operator[]( const int index ) const { assert( index >= 0 && index < 2 ); return b[ index ]; }
	short*			operator[]( const int index ) { assert( index >= 0 && index < 2 ); return b[ index ]; }

	idBoundsShort	operator+( const idBoundsShort& a ) const;

	idBounds		ToBounds( void ) const;

	void			TranslateSelf( const idVec3& offset );

private:
	short			b[2][3];
};

extern idBoundsShort bounds_short_zero;

ID_INLINE idBoundsShort::idBoundsShort( void ) {
}

ID_INLINE idBoundsShort::idBoundsShort( const idBounds &bounds ) {
	SetBounds( bounds );
}

ID_INLINE void idBoundsShort::SetBounds( const idBounds &bounds ) {
#if 0
	// idMath::Ftoi doesn't always truncate, for example in this case:
	/*
		input[1]	1515.9961	const float
		input[1] + 0xffff	67050.996093750000	double
		b01	67051	int
	*/
	int b00 = idMath::Ftoi( bounds[0][0] + 0xffff );
	int b01 = idMath::Ftoi( bounds[0][1] + 0xffff );
	int b02 = idMath::Ftoi( bounds[0][2] + 0xffff );
	int b10 = idMath::Ftoi( bounds[1][0] - 0xffff );
	int b11 = idMath::Ftoi( bounds[1][1] - 0xffff );
	int b12 = idMath::Ftoi( bounds[1][2] - 0xffff );

	b[0][0] = b00 - 0xffff;
	b[0][1] = b01 - 0xffff;
	b[0][2] = b02 - 0xffff;
	b[1][0] = b10 + 0xffff;
	b[1][1] = b11 + 0xffff;
	b[1][2] = b12 + 0xffff;
#else
	b[0][0] = idMath::Ftoi( idMath::Floor( bounds[0][0] ) );
	b[0][1] = idMath::Ftoi( idMath::Floor( bounds[0][1] ) );
	b[0][2] = idMath::Ftoi( idMath::Floor( bounds[0][2] ) );
	b[1][0] = idMath::Ftoi( idMath::Ceil( bounds[1][0] ) );
	b[1][1] = idMath::Ftoi( idMath::Ceil( bounds[1][1] ) );
	b[1][2] = idMath::Ftoi( idMath::Ceil( bounds[1][2] ) );
#endif
}

ID_INLINE void idBoundsShort::SetBounds( const short *list ) {
	b[0][0] = *list++;
	b[0][1] = *list++;
	b[0][2] = *list++;
	b[1][0] = *list++;
	b[1][1] = *list++;
	b[1][2] = *list++;
}

ID_INLINE const short * idBoundsShort::GetBounds( void ) {
	return b[0];
}
  	 

ID_INLINE void idBoundsShort::Clear( void ) {
	b[0][0] = b[0][1] = b[0][2] = 32767;
	b[1][0] = b[1][1] = b[1][2] = -32768;
}

ID_INLINE void idBoundsShort::Zero( void ) {
	b[0][0] = b[0][1] = b[0][2] =
	b[1][0] = b[1][1] = b[1][2] = 0;
}

ID_INLINE void idBoundsShort::Zero( const idVec3 &center ) {
	b[0][0] = b[1][0] = idMath::Ftoi( center[0] + 0.5f );
	b[0][1] = b[1][1] = idMath::Ftoi( center[1] + 0.5f );
	b[0][2] = b[1][2] = idMath::Ftoi( center[2] + 0.5f );
}

ID_INLINE bool idBoundsShort::IsCleared( void ) const {
	return b[0][0] > b[1][0];
}

ID_INLINE bool idBoundsShort::ContainsPoint( const idVec3 &p ) const {
	if ( p[0] < b[0][0] || p[1] < b[0][1] || p[2] < b[0][2] ||
			p[0] > b[1][0] || p[1] > b[1][1] || p[2] > b[1][2] ) {
		return false;
	}
	return true;
}

ID_INLINE bool idBoundsShort::ContainsPoint2D( const idVec3 &p ) const {
	if ( p[0] < b[0][0] || p[1] < b[0][1] || p[0] > b[1][0] || p[1] > b[1][1] ) {
		return false;
	}
	return true;
}

ID_INLINE bool idBoundsShort::IntersectsBounds( const idBounds &a ) const {
	if ( a.GetMaxs()[0] < b[0][0] || a.GetMaxs()[1] < b[0][1] || a.GetMaxs()[2] < b[0][2]
		|| a.GetMins()[0] > b[1][0] || a.GetMins()[1] > b[1][1] || a.GetMins()[2] > b[1][2] ) {
		return false;
	}
	return true;
}

ID_INLINE bool idBoundsShort::IntersectsBounds( const idBoundsShort &a ) const {
	if ( a.b[1][0] < b[0][0] || a.b[1][1] < b[0][1] || a.b[1][2] < b[0][2]
		|| a.b[0][0] > b[1][0] || a.b[0][1] > b[1][1] || a.b[0][2] > b[1][2] ) {
		return false;
	}
	return true;
}

ID_INLINE bool idBoundsShort::IntersectsBounds2D( const idBoundsShort &a ) const {
	if ( a.b[1][0] < b[0][0] || a.b[1][1] < b[0][1] || a.b[0][0] > b[1][0] || a.b[0][1] > b[1][1] ) {
		return false;
	}
	return true;
}

ID_INLINE idBounds idBoundsShort::ToBounds( void ) const {
	return idBounds( idVec3( b[0][0], b[0][1], b[0][2] ), idVec3( b[1][0], b[1][1], b[1][2] ) );
}

ID_INLINE void idBoundsShort::AddBounds( const idBoundsShort& a ) {
	if ( a.b[0][0] < b[0][0] ) {
		b[0][0] = a.b[0][0];
	}
	if ( a.b[0][1] < b[0][1] ) {
		b[0][1] = a.b[0][1];
	}
	if ( a.b[0][2] < b[0][2] ) {
		b[0][2] = a.b[0][2];
	}
	if ( a.b[1][0] > b[1][0] ) {
		b[1][0] = a.b[1][0];
	}
	if ( a.b[1][1] > b[1][1] ) {
		b[1][1] = a.b[1][1];
	}
	if ( a.b[1][2] > b[1][2] ) {
		b[1][2] = a.b[1][2];
	}
}

ID_INLINE  idBoundsShort idBoundsShort::operator+( const idBoundsShort& a ) const {
	idBoundsShort other;
	other = *this;
	other.AddBounds( a );
	return other;
}

ID_INLINE int idBoundsShort::GetLargestAxis( void ) const {
	short work[ 3 ];
	work[ 0 ] = b[ 1 ][ 0 ] - b[ 0 ][ 0 ];
	work[ 1 ] = b[ 1 ][ 1 ] - b[ 0 ][ 1 ];
	work[ 2 ] = b[ 1 ][ 2 ] - b[ 0 ][ 2 ];

	int axis = 0;

	if ( work[ 1 ] > work[ 0 ] ) {
		axis = 1;
	}

	if ( work[ 2 ] > work[ axis ] ) {
		axis = 2;
	}

	return axis;
}

ID_INLINE void idBoundsShort::TranslateSelf( const idVec3& offset ) {
	b[ 0 ][ 0 ] += idMath::Ftoi( idMath::Floor( offset[ 0 ] ) );
	b[ 0 ][ 1 ] += idMath::Ftoi( idMath::Floor( offset[ 1 ] ) );
	b[ 0 ][ 2 ] += idMath::Ftoi( idMath::Floor( offset[ 2 ] ) );
	b[ 1 ][ 0 ] += idMath::Ftoi( idMath::Ceil( offset[ 0 ] ) );
	b[ 1 ][ 1 ] += idMath::Ftoi( idMath::Ceil( offset[ 1 ] ) );
	b[ 1 ][ 2 ] += idMath::Ftoi( idMath::Ceil( offset[ 2 ] ) );
}

ID_INLINE void idBoundsShort::Combine( const idBoundsShort& x, const idBoundsShort& y ) {
	b[0][0] = ( x.b[0][0] < y.b[0][0] ) ? x.b[0][0] : y.b[0][0];
	b[0][1] = ( x.b[0][1] < y.b[0][1] ) ? x.b[0][1] : y.b[0][1];
	b[0][2] = ( x.b[0][2] < y.b[0][2] ) ? x.b[0][2] : y.b[0][2];
	b[1][0] = ( x.b[1][0] > y.b[1][0] ) ? x.b[1][0] : y.b[1][0];
	b[1][1] = ( x.b[1][1] > y.b[1][1] ) ? x.b[1][1] : y.b[1][1];
	b[1][2] = ( x.b[1][2] > y.b[1][2] ) ? x.b[1][2] : y.b[1][2];
}

ID_INLINE void idBoundsShort::AddPoint( const idVec3& x ) {
	idBoundsShort temp;
	temp.b[ 0 ][ 0 ] = idMath::Ftoi( idMath::Floor( x[ 0 ] ) );
	temp.b[ 0 ][ 1 ] = idMath::Ftoi( idMath::Floor( x[ 1 ] ) );
	temp.b[ 0 ][ 2 ] = idMath::Ftoi( idMath::Floor( x[ 2 ] ) );
	temp.b[ 1 ][ 0 ] = idMath::Ftoi( idMath::Ceil( x[ 0 ] ) );
	temp.b[ 1 ][ 1 ] = idMath::Ftoi( idMath::Ceil( x[ 1 ] ) );
	temp.b[ 1 ][ 2 ] = idMath::Ftoi( idMath::Ceil( x[ 2 ] ) );
	AddBounds( temp );
}


#endif /* !__BV_BOUNDSSHORT_H__ */
