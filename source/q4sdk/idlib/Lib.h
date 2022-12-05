#ifndef __LIB_H__
#define __LIB_H__


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

class idLib {
public:
	static class idSys *		sys;
	static class idCommon *		common;
	static class idCVarSystem *	cvarSystem;
	static class idFileSystem *	fileSystem;
	static int					frameNumber;

	static void					Init( void );
	static void					ShutDown( void );

	// wrapper to idCommon functions 
	static void					Error( const char *fmt, ... );
	static void					Warning( const char *fmt, ... );
};


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

#ifdef _DEBUG

	#define ID_CONDITIONAL_ASSERT

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

	#undef assert
	#define assert( x )
	#define verify( x )		( ( x ) ? true : false )

#endif

#define assert_8_byte_aligned( pointer )		assert( (((UINT_PTR)(pointer))&7) == 0 );
#define assert_16_byte_aligned( pointer )		assert( (((UINT_PTR)(pointer))&15) == 0 );
#define assert_32_byte_aligned( pointer )		assert( (((UINT_PTR)(pointer))&31) == 0 );

#ifndef __TYPE_INFO_GEN__
#define compile_time_assert( x )				{ typedef int compile_time_assert_failed[(x) ? 1 : -1]; }
#define file_scoped_compile_time_assert( x )	extern int compile_time_assert_failed[(x) ? 1 : -1]
#define assert_sizeof( type, size )				file_scoped_compile_time_assert( sizeof( type ) == size )
#define assert_offsetof( type, field, offset )	file_scoped_compile_time_assert( offsetof( type, field ) == offset )
#define assert_sizeof_8_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) & 8 ) == 0 )
#define assert_sizeof_16_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) & 15 ) == 0 )
#define assert_sizeof_32_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) & 31 ) == 0 )
#else
#define compile_time_assert( x )
#define file_scoped_compile_time_assert( x )
#define assert_sizeof( type, size )
#define assert_offsetof( type, field, offset )
#define assert_sizeof_16_byte_multiple( type )
#endif

class idException {
public:
	char error[2048];

	idException( const char *text = "" ) { strcpy( error, text ); }
};


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

typedef int						qhandle_t;

class idFile;
class idVec3;
#ifdef _XENON
#define ID_VEC4_ALIGN __declspec(align(16))
#else
#define ID_VEC4_ALIGN
#endif
class ID_VEC4_ALIGN idVec4;

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
extern	ID_VEC4_ALIGN idVec4 colorBlack;
extern	ID_VEC4_ALIGN idVec4 colorWhite;
extern	ID_VEC4_ALIGN idVec4 colorRed;
extern	ID_VEC4_ALIGN idVec4 colorGreen;
extern	ID_VEC4_ALIGN idVec4 colorBlue;
extern	ID_VEC4_ALIGN idVec4 colorYellow;
extern	ID_VEC4_ALIGN idVec4 colorMagenta;
extern	ID_VEC4_ALIGN idVec4 colorCyan;
extern	ID_VEC4_ALIGN idVec4 colorOrange;
extern	ID_VEC4_ALIGN idVec4 colorPurple;
extern	ID_VEC4_ALIGN idVec4 colorPink;
extern	ID_VEC4_ALIGN idVec4 colorBrown;
extern	ID_VEC4_ALIGN idVec4 colorLtGrey;
extern	ID_VEC4_ALIGN idVec4 colorMdGrey;
extern	ID_VEC4_ALIGN idVec4 colorDkGrey;

// packs color floats in the range [0,1] into an integer
dword	PackColor( const idVec3 &color );
void	UnpackColor( const dword color, idVec3 &unpackedColor );
dword	PackColor( const idVec4 &color );
void	UnpackColor( const dword color, idVec4 &unpackedColor );

// little/big endian conversion
short	BigShort( short l );
short	LittleShort( short l );
int		BigLong( int l );
int		LittleLong( int l );
float	BigFloat( float l );
float	LittleFloat( float l );
void	BigRevBytes( void *bp, int elsize, int elcount );
void	LittleRevBytes( void *bp, int elsize, int elcount );
void	Swap_Init( void );

bool	Swap_IsBigEndian( void );

// for base64
void	SixtetsForInt( byte *out, int src);
int		IntForSixtets( byte *in );


/*
===============================================================================

	idLib headers.

===============================================================================
*/

#include "math/Math.h"

// memory management and arrays
#include "Heap.h"

// RAVEN BEGIN
// dluetscher: added includes for new heap/memory management system
#ifdef _RV_MEM_SYS_SUPPORT
#include "rvHeapArena.h"
#include "rvHeap.h"
#include "rvMemSys.h"
#endif
// RAVEN END

#include "containers/List.h"

// math
#include "math/Simd.h"
#include "math/Random.h"
#include "math/Complex.h"
#include "math/Vector.h"
#include "math/Matrix.h"
#include "math/Mat3x4.h"
#include "math/Angles.h"
#include "math/Quat.h"
#include "math/Rotation.h"
#include "math/Plane.h"
#include "math/Pluecker.h"
#include "math/Polynomial.h"
#include "math/Extrapolate.h"
#include "math/Interpolate.h"
#include "math/Curve.h"
#include "math/Ode.h"
#include "math/Lcp.h"
// RAVEN BEGIN
#include "math/Radians.h"
#include "math/FFT.h"
// RAVEN END

// bounding volumes
#include "bv/Sphere.h"
#include "bv/Bounds.h"
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
#include "geometry/TraceModel.h"

// RAVEN BEGIN
// dluetscher: added some headers for new vertex formats
#ifdef _MD5R_SUPPORT
#include "geometry/rvVertex.h"
#endif
// RAVEN END

// text manipulation
#include "Str.h"
#include "Token.h"
#include "Lexer.h"
#include "Parser.h"
#include "Base64.h"
#include "CmdArgs.h"

// containers
#include "containers/BTree.h"
#include "containers/BinSearch.h"
#include "containers/HashIndex.h"
#include "containers/HashTable.h"
#include "containers/StaticList.h"
#include "containers/LinkList.h"
#include "containers/Hierarchy.h"
#include "containers/Queue.h"
#include "containers/Stack.h"
#include "containers/StrList.h"
#include "containers/StrPool.h"
#include "containers/VectorSet.h"
#include "containers/PlaneSet.h"
#include "containers/rvBlockPool.h"
// RAVEN BEGIN
// ddynerman: a pair
#include "containers/Pair.h"
// ddynerman: algorithms
#include "algorithms/MultifieldSort.h"
// RAVEN END

// hashing
#include "hashing/CRC8.h"
#include "hashing/CRC16.h"
#include "hashing/CRC32.h"
#include "hashing/Honeyman.h"
#include "hashing/MD4.h"
#include "hashing/MD5.h"

// misc
#include "Dict.h"
#include "LangDict.h"
#include "BitMsg.h"
#include "MapFile.h"
#include "Timer.h"
// RAVEN BEGIN
// jsinger: xenon needs these because the simd class is no longer virtual
#ifdef _XENON
#include "math/Simd_generic.h"
#include "math/Simd_Xenon.h"
#include "Threads/ThreadEventManager.h"
#include "Threads/WorkerThreadManager.h"
#endif
// RAVEN END

#endif	/* !__LIB_H__ */
