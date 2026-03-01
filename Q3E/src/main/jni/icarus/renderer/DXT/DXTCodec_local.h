/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef __DXTCODEC_LOCAL_H__
#define __DXTCODEC_LOCAL_H__

/*
================================================================================================
Contains the DxtEncoder and DxtDecoder declarations.
================================================================================================
*/
#include "../../idlib/precompiled.h"

#define MAX_TYPE( x )			( ( ( ( 1 << ( ( sizeof( x ) - 1 ) * 8 - 1 ) ) - 1 ) << 8 ) | 255 )
#define MIN_TYPE( x )			( - MAX_TYPE( x ) - 1 )
#define MAX_UNSIGNED_TYPE( x )	( ( ( ( 1U << ( ( sizeof( x ) - 1 ) * 8 ) ) - 1 ) << 8 ) | 255U )
#define MIN_UNSIGNED_TYPE( x )	0
#define C_FLOAT_TO_INT( x )		(int)(x)

/*
========================
SwapValues
========================
*/
template< typename _type_ >
ID_INLINE void SwapValues( _type_ & a, _type_ & b )
{
	_type_ c = a;
	a = b;
	b = c;
}

/*
========================
idMath::Ftob
========================
*/
ID_INLINE byte Ftob( float f )
{
	// If a converted result is negative the value (0) is returned and if the
	// converted result is larger than the maximum byte the value (255) is returned.

#if defined(USE_INTRINSICS_SSE)
	__m128 x = _mm_load_ss( &f );
	x = _mm_max_ss( x, SIMD_SP_zero );
	x = _mm_min_ss( x, SIMD_SP_255 );
	return static_cast<byte>( _mm_cvttss_si32( x ) );
#else
	// The converted result is clamped to the range [0,255].
	int i = C_FLOAT_TO_INT( f );
	if( i < 0 )
	{
		return 0;
	}
	else if( i > 255 )
	{
		return 255;
	}
	return static_cast<byte>( i );
}
#endif
#endif // !__DXTCODEC_LOCAL_H__
