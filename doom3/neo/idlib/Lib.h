/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __LIB_H__
#define __LIB_H__

#ifdef _SPLASHDAMAGE
#ifdef _DEBUG
#undef DEBUG_NEW
#define DEBUG_NEW new
#endif

#include "../common/common.h"
#include <string.h>
#include <limits.h>
#endif

/*
===============================================================================

	idLib contains stateless support classes and concrete types. Some classes
	do have static variables, but such variables are initialized once and
	read-only after initialization (they do not maintain a modifiable state).

	The interface pointers idSys, idCommon, idCVarSystem and idFileSystem
	should be set before using idLib. The pointers stored here should not
	be used by any part of the engine except for idLib.

	The frameNumber should be continuously set to the number of the current
	frame if frame base memory logging is required.

===============================================================================
*/

class idLib
{
	public:
		static class idSys 		*sys;
		static class idCommon 		*common;
		static class idCVarSystem 	*cvarSystem;
		static class idFileSystem 	*fileSystem;
		static int					frameNumber;

		static void					Init(void);
		static void					ShutDown(void);

		// wrapper to idCommon functions
#ifdef _SPLASHDAMAGE
    	static void		   			Printf( const char *fmt, ... );
#endif
		static void					Error(const char *fmt, ...);
		static void					Warning(const char *fmt, ...);
};

#ifdef _SPLASHDAMAGE
/*
===============================================================================

	Asserts and Exceptions

===============================================================================
*/

/*
The verify(x) macro just returns true or false in release mode, but breaks
in debug mode.  That way the code can take a non-fatal path in release mode
if something that's not supposed to happen happens.

if ( !verify(game) ) {
	// This should never happen!
	return;
}
*/

template <bool CompileTimeCheckValue> struct sdCompileTimeAssert {};
template<> struct sdCompileTimeAssert<true> {
    static void assertX() {};
};
#define CompileTimeAssert(__a) {const bool __b = (__a) ? true : false; sdCompileTimeAssert<__b>::assertX();}

#ifdef _DEBUG
void AssertFailed( const char *file, int line, const char *expression );
#undef assert

#ifdef ID_CONDITIONAL_ASSERT
// lets you disable an assertion at runtime when needed
// could extend this to count and produce an assert log - useful for 'release with asserts' builds
#define assert( x ) \
		{ \
			volatile static bool assert_enabled = true; \
			if ( assert_enabled ) { \
				if ( x ) { } else AssertFailed( __FILE__, __LINE__, #x );	\
			} \
		}
#define verify( x ) \
		( \
			( ( x ) ? true : ( \
				( { \
					volatile static bool assert_enabled = true; \
					if ( assert_enabled ) { AssertFailed( __FILE__, __LINE__, #x ); } \
				} ) \
				, false ) ) \
		)
#else
#define assert( x )		if ( x ) { } else AssertFailed( __FILE__, __LINE__, #x )
#define verify( x )		( ( x ) ? true : ( AssertFailed( __FILE__, __LINE__, #x ), false ) )
#endif

#else

#define verify( x )		( ( x ) ? true : false )
#undef assert
#define assert( x )

#endif

#define assert_8_byte_aligned( pointer )		assert( ( ((UINT_PTR)(pointer)) &  7 ) == 0 );
#define assert_16_byte_aligned( pointer )		assert( ( ((UINT_PTR)(pointer)) & 15 ) == 0 );
#define assert_32_byte_aligned( pointer )		assert( ( ((UINT_PTR)(pointer)) & 31 ) == 0 );
#define assert_64_byte_aligned( pointer )		assert( ( ((UINT_PTR)(pointer)) & 63 ) == 0 );

#ifndef __TYPE_INFO_GEN__
#define compile_time_assert( x )				{ typedef int compile_time_assert_failed[(x) ? 1 : -1]; }
#define file_scoped_compile_time_assert( x )	extern int compile_time_assert_failed[(x) ? 1 : -1]
#define assert_sizeof( type, size )				file_scoped_compile_time_assert( sizeof( type ) == size )
#define assert_offsetof( type, field, offset )	file_scoped_compile_time_assert( offsetof( type, field ) == offset )
#define assert_offsetof_16_byte_multiple( type, field )	file_scoped_compile_time_assert( ( offsetof( type, field ) & 15 ) == 0 )
#define assert_offsetof_8_byte_multiple( type, field )	file_scoped_compile_time_assert( ( offsetof( type, field ) & 7 ) == 0 )
#define assert_sizeof_8_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) &  7 ) == 0 )
#define assert_sizeof_16_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) & 15 ) == 0 )
#define assert_sizeof_32_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) & 31 ) == 0 )
#else
#define compile_time_assert( x )
#define file_scoped_compile_time_assert( x )
#define assert_sizeof( type, size )
#define assert_offsetof( type, field, offset )
#define assert_offsetof_16_byte_multiple( type, field )
#define assert_sizeof_8_byte_multiple( type )
#define assert_sizeof_16_byte_multiple( type )
#define assert_sizeof_32_byte_multiple( type )
#endif

#if !defined(id_attribute)
#if defined( __GNUC__ )
#define id_attribute(x) __attribute__(x)
#else
#define id_attribute(x)
#endif /* __GNUC__ */
#endif

#endif

/*
===============================================================================

	Types and defines used throughout the engine.

===============================================================================
*/

typedef unsigned char			byte;		// 8 bits
typedef unsigned short			word;		// 16 bits
typedef unsigned int			dword;		// 32 bits
typedef unsigned int			uint;
typedef unsigned long			ulong;

#ifdef _SPLASHDAMAGE
/*
64 bit notes:
Microsoft compiler is LLP64: int and long are 32 bits on all platforms, long long is 64 bits on all platforms
gcc is LP64: int is 32 bits, long is 64 bits on 64 bit environment

when you explicitely need a 64 bit integer, do not use __int64 which is Microsoft compiler specific
instead, use int64_t which is defined in the C99 standard. sadly, M$ doesn't ship <stdint.h> with MSVC
*/

#ifdef _MSC_VER
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#if !defined(INT64_MIN)
#define INT64_MIN _I64_MIN
#endif
#if !defined(INT64_MAX)
#define INT64_MAX _I64_MAX
#endif

#if !defined(UINT64_MIN)
#define UINT64_MIN 0
#endif
#if !defined(UINT64_MAX)
#define UINT64_MAX _UI64_MAX
#endif

#else
#include <stdint.h>
#endif

// The C/C++ standard guarantees the size of an unsigned type is the same as the signed type.
// The exact size in bytes of several types is guaranteed here.
assert_sizeof( bool,	1 );
assert_sizeof( char,	1 );
assert_sizeof( short,	2 );
assert_sizeof( int,		4 );
//assert_sizeof( long,	8 );
assert_sizeof( float,	4 );
assert_sizeof( byte,	1 );
assert_sizeof( word,	2 );
assert_sizeof( dword,	4 );
#if defined(__STDC_ISO_10646__) || defined(__linux__) //karin: 4 bytes on Linux
// starting with glibc 2.2, Linux uses a 32bit wchar_t conformant to ISO 10646
// let's assume other platforms do as well
assert_sizeof( wchar_t, 4 );
#else
// _WIN32
assert_sizeof( wchar_t, 2 );
#endif

#endif
typedef int						qhandle_t;

class idFile;
class idVec3;
class idVec4;

#ifndef NULL
#define NULL					((void *)0)
#endif

#ifndef BIT
#define BIT( num )				( 1 << ( num ) )
#endif

#define	MAX_STRING_CHARS		1024		// max length of a string

// maximum world size
#define MAX_WORLD_COORD			( 128 * 1024 )
#define MIN_WORLD_COORD			( -128 * 1024 )
#define MAX_WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

// basic colors
extern	idVec4 colorBlack;
extern	idVec4 colorWhite;
extern	idVec4 colorRed;
extern	idVec4 colorGreen;
extern	idVec4 colorBlue;
extern	idVec4 colorYellow;
extern	idVec4 colorMagenta;
extern	idVec4 colorCyan;
extern	idVec4 colorOrange;
extern	idVec4 colorPurple;
extern	idVec4 colorPink;
extern	idVec4 colorBrown;
extern	idVec4 colorLtGrey;
extern	idVec4 colorMdGrey;
extern	idVec4 colorDkGrey;

#ifdef _SPLASHDAMAGE
extern	const idVec4 colorLtBlue;
extern	const idVec4 colorDkRed;
#endif

// packs color floats in the range [0,1] into an integer
dword	PackColor(const idVec3 &color);
void	UnpackColor(const dword color, idVec3 &unpackedColor);
dword	PackColor(const idVec4 &color);
void	UnpackColor(const dword color, idVec4 &unpackedColor);

// little/big endian conversion
short	BigShort(short l);
short	LittleShort(short l);
int		BigLong(int l);
int		LittleLong(int l);
float	BigFloat(float l);
float	LittleFloat(float l);
void	BigRevBytes(void *bp, int elsize, int elcount);
void	LittleRevBytes(void *bp, int elsize, int elcount);
void	LittleBitField(void *bp, int elsize);
void	Swap_Init(void);

int64_t	BigLongLong(int64_t l);
int64_t	LittleLongLong(int64_t l);
double	BigDouble(double l);
double	LittleDouble(double l);
int     LongSwap(int l); // in idLib/Lib.cpp
int64_t LongLongSwap(int64_t l);

bool	Swap_IsBigEndian(void);

// for base64
void	SixtetsForInt(byte *out, int src);
int		IntForSixtets(byte *in);

#ifdef _SPLASHDAMAGE
// using shorts for triangle indexes can save a significant amount of traffic, but
// to support the large models that renderBump loads, they need to be 32 bits

#if defined( SD_USE_INDEX_SIZE_16 )
#define GL_INDEX_TYPE		GL_UNSIGNED_SHORT
#define GL_INDEX_SHORT
typedef unsigned short		glIndex_t;
typedef glIndex_t			vertIndex_t;
#else
#define GL_INDEX_TYPE		GL_UNSIGNED_INT
#define GL_INDEX_INT
typedef int					glIndex_t;
typedef glIndex_t			vertIndex_t;
#endif
#endif

#if !defined(_SPLASHDAMAGE)
#ifdef _DEBUG
void AssertFailed(const char *file, int line, const char *expression);
#undef assert
#define assert( X )		if ( X ) { } else AssertFailed( __FILE__, __LINE__, #X )
#endif
#endif

#ifdef _DEBUG
#if __cplusplus < 201103L
#define ID_STATIC_ASSERT(exp) do { char _static_assert_exp_var_[(exp) ? 1 : -1]; (void)_static_assert_exp_var_; } while(0)
#define ID_STATIC_ASSERT2(exp, msg) do { if(exp) {} else { printf(msg); } char _static_assert_exp_var_[(exp) ? 1 : -1]; (void)_static_assert_exp_var_; } while(0)
#else
#define ID_STATIC_ASSERT(exp) static_assert(exp, "")
#define ID_STATIC_ASSERT2(exp, msg) static_assert(exp, msg)
#endif
#else
#define ID_STATIC_ASSERT(exp)
#define ID_STATIC_ASSERT2(exp, msg)
#endif

class idException
{
	public:
		char error[MAX_STRING_CHARS];

		idException(const char *text = "") {
			strcpy(error, text);
		}
};

// move from Math.h to keep gcc happy
template<class T> ID_INLINE T	Max(T x, T y)
{
	return (x > y) ? x : y;
}
template<class T> ID_INLINE T	Min(T x, T y)
{
	return (x < y) ? x : y;
}

/*
===============================================================================

	idLib headers.

===============================================================================
*/

#ifdef _SPLASHDAMAGE
// turn float to int conversions into compile errors
#include "math/FloatErrors.h"

#include "threading/ThreadingDefs.h"
#include "threading/Lock.h"
#endif

// memory management and arrays
#include "Heap.h"

#ifdef _SPLASHDAMAGE
#include "Sort.h"
#endif
#include "containers/List.h"

#ifdef _SPLASHDAMAGE
// text manipulation
#include "text/Str.h"
#include "text/StrSimple.h"
#include "text/WStr.h"
#include "text/StrBuilder.h"

// threading
#include "threading/Signal.h"
#include "threading/Thread.h"
#include "threading/ThreadProcess.h"

// more complex memory allocators
#include "PoolAllocator.h"

// text manipulation
#include "text/Base64.h"
#include "text/CmdArgs.h"
#include "text/Token.h"
#include "text/WToken.h"
#include "text/UTF8.h"
#endif

// math
#include "math/Simd.h"
#ifdef _SPLASHDAMAGE
#include "math/BasicTypes.h"
#endif
#include "math/Math.h"
#include "math/Random.h"
#include "math/Complex.h"
#include "math/Vector.h"
#include "math/Matrix.h"
#ifdef _SPLASHDAMAGE
#include "math/Mat3x4.h"
#endif
#include "math/Angles.h"
#include "math/Quat.h"
#include "math/Rotation.h"
#include "math/Plane.h"
#include "math/Pluecker.h"
#include "math/Polynomial.h"
#include "math/Extrapolate.h"
#include "math/Interpolate.h"
#ifdef _HUMANHEAD
#include "../humanhead/idlib/math/prey_interpolate.h"		// HUMANHEAD pdm
#include "../humanhead/idlib/math/prey_math.h"				// HUMANHEAD pdm
#endif
#include "math/Curve.h"
#include "math/Ode.h"
#include "math/Lcp.h"
#ifdef _SPLASHDAMAGE
#include "math/Perlin.h"
#include "math/Radians.h"
#endif

// bounding volumes
#include "bv/Sphere.h"
#include "bv/Bounds.h"
#ifdef _SPLASHDAMAGE
#include "bv/Bounds2D.h"
#include "bv/BoundsShort.h"
#endif
#include "bv/Box.h"
#include "bv/Frustum.h"

// geometry
#include "geometry/DrawVert.h"
#include "geometry/JointTransform.h"
#include "geometry/Winding.h"
#include "geometry/Winding2D.h"
#include "geometry/Surface.h"
#include "geometry/Surface_Patch.h"
#include "geometry/Surface_Polytope.h"
#include "geometry/Surface_SweptSpline.h"
#ifdef _SPLASHDAMAGE
#include "geometry/Surface_Traceable.h"
#endif
#include "geometry/TraceModel.h"
#ifdef _SPLASHDAMAGE
#include "geometry/TraceSurface.h"
#endif

// text manipulation
#include "Str.h"
#include "Token.h"
#if !defined(_SPLASHDAMAGE)
#include "Lexer.h"
#include "Parser.h"
#endif
#include "Base64.h"
#include "CmdArgs.h"

// containers
#ifdef _SPLASHDAMAGE
#include "containers/Pair.h"
#endif
#include "containers/BTree.h"
#include "containers/BinSearch.h"
#include "containers/HashIndex.h"
#include "containers/HashTable.h"
#ifdef _SPLASHDAMAGE
#include "containers/HashMap.h"
#include "containers/HashMapGeneric.h"
#endif
#include "containers/StaticList.h"
#include "containers/LinkList.h"
#include "containers/Hierarchy.h"
#ifdef _SPLASHDAMAGE
#include "containers/LinkedList.h"
#endif
#include "containers/Queue.h"
#include "containers/Stack.h"
#include "containers/StrList.h"
#include "containers/StrPool.h"
#include "containers/VectorSet.h"
#include "containers/PlaneSet.h"
#ifdef _SPLASHDAMAGE
#include "containers/VectorWeld.h"
#include "containers/Grid.h"
#include "containers/QuadTree.h"
#include "containers/BitField.h"
#include "containers/Handles.h"
#include "containers/Deque.h"
#endif

#ifdef _HUMANHEAD
// HUMANHEAD pdm: idlib additions
#include "../humanhead/idlib/containers/PreyStack.h"
// HUMANHEAD END
#endif
#ifdef _SPLASHDAMAGE
#include "text/LexerBinary.h"
#ifdef _SPLASHDAMAGE
#include "Lexer.h"
#include "Parser.h"
#endif
#include "text/WLexer.h"
#endif

// hashing
#ifdef _SPLASHDAMAGE
#include "hashing/CRC8.h"
#include "hashing/CRC16.h"
#endif
#include "hashing/CRC32.h"
#ifdef _SPLASHDAMAGE
#include "hashing/Honeyman.h"
#endif
#include "hashing/MD4.h"
#include "hashing/MD5.h"

// misc
#include "Dict.h"
#include "LangDict.h"
#include "BitMsg.h"
#include "MapFile.h"
#include "Timer.h"
#ifdef _SPLASHDAMAGE
#include "Singleton.h"
#include "PtrPolicies.h"
#include "AutoPtr.h"
#include "Factory.h"
#include "Callable.h"
#include "Color.h"
#include "Properties.h"
#include "TextUtilities.h"

//karin: wide-character string and multibytes character string convert
idStr WStrToStr( const wchar_t *wstr );
idWStr StrToWStr( const char *str );
idStr WStrToStr( const idWStr &wstr );
idWStr StrToWStr( const idStr &str );
#endif

#endif	/* !__LIB_H__ */
