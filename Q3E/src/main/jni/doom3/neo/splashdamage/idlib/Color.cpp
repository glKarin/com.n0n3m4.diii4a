// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

const sdColor3	sdColor3::black		= sdColor3( 0.00f, 0.00f, 0.00f );
const sdColor3	sdColor3::white		= sdColor3( 1.00f, 1.00f, 1.00f );
const sdColor3	sdColor3::red		= sdColor3( 1.00f, 0.00f, 0.00f );
const sdColor3	sdColor3::green		= sdColor3( 0.00f, 1.00f, 0.00f );
const sdColor3	sdColor3::blue		= sdColor3( 0.00f, 0.00f, 1.00f );
const sdColor3	sdColor3::yellow	= sdColor3( 1.00f, 1.00f, 0.00f );
const sdColor3	sdColor3::magenta	= sdColor3( 1.00f, 0.00f, 1.00f );
const sdColor3	sdColor3::cyan		= sdColor3( 0.00f, 1.00f, 1.00f );
const sdColor3	sdColor3::orange	= sdColor3( 1.00f, 0.50f, 0.00f );
const sdColor3	sdColor3::purple	= sdColor3( 0.60f, 0.00f, 0.60f );
const sdColor3	sdColor3::pink		= sdColor3( 0.73f, 0.40f, 0.48f );
const sdColor3	sdColor3::brown		= sdColor3( 0.40f, 0.35f, 0.08f );
const sdColor3	sdColor3::ltGrey	= sdColor3( 0.75f, 0.75f, 0.75f );
const sdColor3	sdColor3::mdGrey	= sdColor3( 0.50f, 0.50f, 0.50f );
const sdColor3	sdColor3::dkGrey	= sdColor3( 0.25f, 0.25f, 0.25f );
const sdColor3	sdColor3::ltBlue	= sdColor3( 0.40f, 0.70f, 1.00f );
const sdColor3	sdColor3::dkRed		= sdColor3( 0.70f, 0.00f, 0.00f );

/*
=============
sdColor3::ToString
=============
*/
const char* sdColor3::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void sdColor3::Lerp( const sdColor3& v1, const sdColor3& v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}

const sdColor4	sdColor4::black		= sdColor4( 0.00f, 0.00f, 0.00f, 1.00f );
const sdColor4	sdColor4::white		= sdColor4( 1.00f, 1.00f, 1.00f, 1.00f );
const sdColor4	sdColor4::red		= sdColor4( 1.00f, 0.00f, 0.00f, 1.00f );
const sdColor4	sdColor4::green		= sdColor4( 0.00f, 1.00f, 0.00f, 1.00f );
const sdColor4	sdColor4::blue		= sdColor4( 0.00f, 0.00f, 1.00f, 1.00f );
const sdColor4	sdColor4::yellow	= sdColor4( 1.00f, 1.00f, 0.00f, 1.00f );
const sdColor4	sdColor4::magenta	= sdColor4( 1.00f, 0.00f, 1.00f, 1.00f );
const sdColor4	sdColor4::cyan		= sdColor4( 0.00f, 1.00f, 1.00f, 1.00f );
const sdColor4	sdColor4::orange	= sdColor4( 1.00f, 0.50f, 0.00f, 1.00f );
const sdColor4	sdColor4::purple	= sdColor4( 0.60f, 0.00f, 0.60f, 1.00f );
const sdColor4	sdColor4::pink		= sdColor4( 0.73f, 0.40f, 0.48f, 1.00f );
const sdColor4	sdColor4::brown		= sdColor4( 0.40f, 0.35f, 0.08f, 1.00f );
const sdColor4	sdColor4::ltGrey	= sdColor4( 0.75f, 0.75f, 0.75f, 1.00f );
const sdColor4	sdColor4::mdGrey	= sdColor4( 0.50f, 0.50f, 0.50f, 1.00f );
const sdColor4	sdColor4::dkGrey	= sdColor4( 0.25f, 0.25f, 0.25f, 1.00f );
const sdColor4	sdColor4::ltBlue	= sdColor4( 0.40f, 0.70f, 1.00f, 1.00f );
const sdColor4	sdColor4::dkRed		= sdColor4( 0.70f, 0.00f, 0.00f, 1.00f );

/*
=============
sdColor4::ToString
=============
*/
const char* sdColor4::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void sdColor4::Lerp( const sdColor4& v1, const sdColor4& v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}

/*
================
ColorFloatToByte
================
*/
ID_INLINE static byte ColorFloatToByte( float c ) {
	return idMath::Ftob( c * 255.0f );
}

/*
================
sdColor4::PackColor
================
*/
dword sdColor4::PackColor( const idVec3& color, float alpha ) {
	dword dw, dx, dy, dz;

	dx = ColorFloatToByte( color.x );
	dy = ColorFloatToByte( color.y );
	dz = ColorFloatToByte( color.z );
	dw = ColorFloatToByte( alpha );

#if defined( _XENON )
	return ( dx << 24 ) | ( dy << 16 ) | ( dz << 8 ) | ( dw << 0 );
#elif defined( _WIN32 ) || defined( __linux__ )
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
#elif defined( MACOS_X )
#if defined( __ppc__ )
	return ( dx << 24 ) | ( dy << 16 ) | ( dz << 8 ) | ( dw << 0 );
#else
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
#endif
#else
#error OS define is required!
#endif
}

/*
================
sdColor4::PackColor
================
*/
dword sdColor4::PackColor( const idVec4& color ) {
	dword dw, dx, dy, dz;

	dx = ColorFloatToByte( color.x );
	dy = ColorFloatToByte( color.y );
	dz = ColorFloatToByte( color.z );
	dw = ColorFloatToByte( color.w );

#if defined( _XENON )
	return ( dx << 24 ) | ( dy << 16 ) | ( dz << 8 ) | ( dw << 0 );
#elif defined( _WIN32 ) || defined( __linux__ )
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
#elif defined( MACOS_X )
	#if defined( __ppc__ )
	return ( dx << 24 ) | ( dy << 16 ) | ( dz << 8 ) | ( dw << 0 );
	#else
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
	#endif
#else
#error OS define is required!
#endif
}

/*
================
sdColor4::UnpackColor
================
*/
void sdColor4::UnpackColor( const dword color, idVec4& unpackedColor ) {
#if defined( _XENON )
	unpackedColor.Set( ( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ), 
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ) );
#elif defined( _WIN32 ) || defined( __linux__ )
	unpackedColor.Set( ( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ), 
		( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ) );
#elif defined(MACOS_X)
	#if defined ( __ppc__ )
	unpackedColor.Set( ( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ), 
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ) );
	#else
	unpackedColor.Set( ( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ), 
		( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ) );
	#endif
#else
#error OS define is required!
#endif
}

/*
================
sdColor3::PackColor
================
*/
dword sdColor3::PackColor( const idVec3& color ) {
	dword dx, dy, dz;

	dx = ColorFloatToByte( color.x );
	dy = ColorFloatToByte( color.y );
	dz = ColorFloatToByte( color.z );

#if defined(_WIN32) || defined(__linux__)
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 );
#elif defined(MACOS_X)
	#if defined(__ppc__)
	return ( dy << 16 ) | ( dz << 8 ) | ( dx << 0 );
	#else
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 );
	#endif
#else
#error OS define is required!
#endif
}

/*
================
sdColor3::UnpackColor
================
*/
void sdColor3::UnpackColor( const dword color, idVec3& unpackedColor ) {
#if defined(_WIN32) || defined(__linux__)
	unpackedColor.Set( ( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ), 
		( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ) );
#elif defined(MACOS_X)
	#if defined(__ppc__)
	unpackedColor.Set( ( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ) );
	#else
	unpackedColor.Set( ( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
		( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ), 
		( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ) );
	#endif
#else
#error OS define is required!
#endif
}
