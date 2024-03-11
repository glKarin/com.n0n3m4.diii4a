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
#ifndef __SYS_ASSERT_H__
#define __SYS_ASSERT_H__

/*
================================================================================================

Getting assert() to work as we want on all platforms and code analysis tools can be tricky.

================================================================================================
*/

bool AssertFailed( const char *file, int line, const char *expression );

// tell PC-Lint that assert failed won't return, which means it can assume the conditions
// are true for subsequent analysis.
//lint -function( exit, AssertFailed )

// stgatilov: leave asserts enabled in "Debug Fast"
// despite NDEBUG being defined by builtin VC rules (because of Release CRT)

//====================== assert in debug mode =======================
#if defined( _DEBUG ) || defined( _lint ) || defined( EXPLICIT_OPTIMIZATION )


#undef assert

// idassert is useful for cases where some external library (think MFC, etc.)
// decides it's a good idea to redefine assert on us
#define idassert( x )	(void)( ( !!( x ) ) || ( AssertFailed( __FILE__, __LINE__, #x ) ) )

// We have the code analysis tools on the 360 compiler,
// so let it know what our asserts are.
// The VS ultimate editions also get it on win32, but not x86
//#define assert( x )		{ __analysis_assume( x ) ; idassert( x ) ; } //duzenko: there must be a better fix but __analysis_assume is doing nothing on Linux anyway
#define assert( x )		idassert( x )

#define verify( x )		( ( x ) ? true : ( AssertFailed( __FILE__, __LINE__, #x ), false ) )


#else // _DEBUG

//====================== assert in release mode =======================

#define idassert( x )	{ (( void )0); } 

#undef assert

#define assert( x )		idassert( x )

#define verify( x )		( ( x ) ? true : false )

#endif // _DEBUG

//=====================================================================

#define idreleaseassert( x )	(void)( ( !!( x ) ) || ( AssertFailed( __FILE__, __LINE__, #x ) ) );

#define release_assert( x )	idreleaseassert( x )

#define assert_2_byte_aligned( ptr )		assert( ( ((uintptr_t)(ptr)) &  1 ) == 0 )
#define assert_4_byte_aligned( ptr )		assert( ( ((uintptr_t)(ptr)) &  3 ) == 0 )
#define assert_8_byte_aligned( ptr )		assert( ( ((uintptr_t)(ptr)) &  7 ) == 0 )
#define assert_16_byte_aligned( ptr )		assert( ( ((uintptr_t)(ptr)) & 15 ) == 0 )
#define assert_32_byte_aligned( ptr )		assert( ( ((uintptr_t)(ptr)) & 31 ) == 0 )
#define assert_64_byte_aligned( ptr )		assert( ( ((uintptr_t)(ptr)) & 63 ) == 0 )
#define assert_128_byte_aligned( ptr )		assert( ( ((uintptr_t)(ptr)) & 127 ) == 0 )
#define assert_aligned_to_type_size( ptr )	assert( ( ((uintptr_t)(ptr)) & ( sizeof( (ptr)[0] ) - 1 ) ) == 0 )

#if !defined( __TYPEINFOGEN__ ) && !defined( _lint )	// pcLint has problems with assert_offsetof()

template<bool> struct compile_time_assert_failed;
template<> struct compile_time_assert_failed<true> {};
template<int x> struct compile_time_assert_test {};
#define compile_time_assert_join2( a, b )	a##b
#define compile_time_assert_join( a, b )	compile_time_assert_join2(a,b)
#define compile_time_assert( x )			typedef compile_time_assert_test<sizeof(compile_time_assert_failed<(bool)(x)>)> compile_time_assert_join(compile_time_assert_typedef_, __LINE__)

#define assert_sizeof( type, size )						compile_time_assert( sizeof( type ) == size )
#define assert_sizeof_8_byte_multiple( type )			compile_time_assert( ( sizeof( type ) &  7 ) == 0 )
#define assert_sizeof_16_byte_multiple( type )			compile_time_assert( ( sizeof( type ) & 15 ) == 0 )
#define assert_offsetof( type, field, offset )			compile_time_assert( offsetof( type, field ) == offset )
#define assert_offsetof_8_byte_multiple( type, field )	compile_time_assert( ( offsetof( type, field ) & 7 ) == 0 )
#define assert_offsetof_16_byte_multiple( type, field )	compile_time_assert( ( offsetof( type, field ) & 15 ) == 0 )

#else

#define compile_time_assert( x )
#define assert_sizeof( type, size )
#define assert_sizeof_8_byte_multiple( type )
#define assert_sizeof_16_byte_multiple( type )
#define assert_offsetof( type, field, offset )
#define assert_offsetof_8_byte_multiple( type, field )
#define assert_offsetof_16_byte_multiple( type, field )

#endif

// useful for verifying that an array of items has the same number of elements in it as an enum type
#define verify_array_size( _array_name_, _max_enum_ ) \
	compile_time_assert( sizeof( _array_name_ ) == ( _max_enum_ ) * sizeof( _array_name_[ 0 ] ) )


// ai debugging macros (designed to limit ai interruptions to non-ai programmers)
#ifdef _DEBUG
//#define DEBUGAI		// NOTE: uncomment for full ai debugging
#endif

#ifdef DEBUGAI
#define ASSERTAI( x )	assert( x )
#define VERIFYAI( x )	verify( x )
#else // DEBUGAI
#define ASSERTAI( x )
#define VERIFYAI( x )	( ( x ) ? true : false )
#endif // DEBUGAI

#endif	// !__SYS_ASSERT_H__
