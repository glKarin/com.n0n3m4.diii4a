// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __LIB_H__
#define __LIB_H__

#if 0
	#ifdef _DEBUG
	#define DEBUG_NEW new(__FILE__, __LINE__)

	#ifdef _WIN32
	#include <crtdbg.h>
	#endif // _WIN32

	inline void* __cdecl operator new(size_t nSize, const char* lpszFileName, int nLine) {
		return ::operator new(nSize, _NORMAL_BLOCK, lpszFileName, nLine);
	}

	inline void __cdecl operator delete(void* pData, const char* /* lpszFileName */, int /* nLine */) {
		::operator delete(pData);
	}

	inline void* __cdecl operator new[](size_t nSize, const char* lpszFileName, int nLine) {
		return ::operator new[](nSize, _NORMAL_BLOCK, lpszFileName, nLine);
	}

	inline void __cdecl operator delete[](void* pData, const char* /* lpszFileName */, int /* nLine */) {
		::operator delete(pData);
	}
	#endif
#else
	#ifdef _DEBUG
	#undef DEBUG_NEW
	#define DEBUG_NEW new
	#endif
#endif

#include "../common/common.h"
#include <string.h>
#include <limits.h>

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
	static void		   			Printf( const char *fmt, ... );
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

template <bool CompileTimeCheckValue> struct sdCompileTimeAssert {};
template<> struct sdCompileTimeAssert<true> { static void assertX() {}; };
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

class idException {
public:
	char error[1024];

	idException( const char *text = "" ) { strcpy( error, text ); }
};

#if defined( __GNUC__ )
	#define id_attribute(x) __attribute__(x)
#else
	#define id_attribute(x)
#endif /* __GNUC__ */

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

#define INT64_MIN _I64_MIN
#define INT64_MAX _I64_MAX

#define UINT64_MIN 0
#define UINT64_MAX _UI64_MAX

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
#ifdef __STDC_ISO_10646__
// starting with glibc 2.2, Linux uses a 32bit wchar_t conformant to ISO 10646
// let's assume other platforms do as well
assert_sizeof( wchar_t, 4 );
#else
// _WIN32
assert_sizeof( wchar_t, 2 );
#endif

typedef int						qhandle_t;

class idFile;
class idVec4;

struct idNullPtr {
	// one pointer member initialized to zero so you can pass NULL as a vararg
	void *value; idNullPtr() : value( 0 ) { }
	// implicit conversion to all pointer types
	template<typename T1> operator T1 * () const { return 0; }
	// implicit conversion to all pointer to member types
	template<typename T1, typename T2> operator T1 T2::* () const { return 0; }
};

#undef NULL
#if defined( _WIN32 ) && !( defined( _AFXEXT ) || defined( _USRDLL ) )
#define NULL					idNullPtr()
#else
#define NULL					0
#endif

#define	MAX_STRING_CHARS		1024		// max length of a static string

// maximum world size
#define MAX_WORLD_COORD			( 128 * 1024 )
#define MIN_WORLD_COORD			( -128 * 1024 )
#define MAX_WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

// basic colors
extern	const idVec4 colorBlack;
extern	const idVec4 colorWhite;
extern	const idVec4 colorRed;
extern	const idVec4 colorGreen;
extern	const idVec4 colorBlue;
extern	const idVec4 colorYellow;
extern	const idVec4 colorMagenta;
extern	const idVec4 colorCyan;
extern	const idVec4 colorOrange;
extern	const idVec4 colorPurple;
extern	const idVec4 colorPink;
extern	const idVec4 colorBrown;
extern	const idVec4 colorLtGrey;
extern	const idVec4 colorMdGrey;
extern	const idVec4 colorDkGrey;

extern	const idVec4 colorLtBlue;
extern	const idVec4 colorDkRed;

// little/big endian conversion
ID_INLINE bool		Swap_IsBigEndian( void ) { short s = 256; return ( *((byte *)&s) != 0 ); }
void				Swap_Init( void );

short				BigShort( short l );
short				LittleShort( short l );
int					BigLong( int l );
int					LittleLong( int l );
float				BigFloat( float l );
float				LittleFloat( float l );
double				LittleDouble( double l );
void				BigRevBytes( void *bp, int elsize, int elcount );
void				LittleRevBytes( void *bp, int elsize, int elcount );
void				LittleBitField( void *bp, int elsize );

ID_INLINE void		SwapLittleShort( short &c ) { c = LittleShort( c ); }
ID_INLINE void		SwapLittleUnsignedShort( unsigned short &c ) { c = LittleShort( c ); }
ID_INLINE void		SwapLittleInt( int &c ) { c = LittleLong( c ); }
ID_INLINE void		SwapLittleUnsignedInt( unsigned int &c ) { c = LittleLong( c ); }
ID_INLINE void		SwapLittleFloat( float &c ) { c = LittleFloat( c ); }
template<class type>
ID_INLINE void		SwapLittleFloatClass( type &c ) { for ( int i = 0; i < c.GetDimension(); i++ ) { c.ToFloatPtr()[i] = LittleFloat( c.ToFloatPtr()[i] ); } }

// for base64
void				SixtetsForInt( byte *out, int src);
int					IntForSixtets( byte *in );

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

/*
===============================================================================

	idLib headers.

===============================================================================
*/

// turn float to int conversions into compile errors
#include "math/FloatErrors.h"

#include "threading/ThreadingDefs.h"
#include "threading/Lock.h"

// memory management and arrays
#ifdef _XENON
#include "../Xenon/Sys/Xenon_Common.h"
#include "../Xenon/Sys/Xenon_Heap.h"
#else
#include "Heap.h"
#endif

#include "Sort.h"
#include "containers/List.h"

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

// math
#include "math/Simd.h"
#include "math/BasicTypes.h"
#include "math/Math.h"
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
#include "math/Perlin.h"
#include "math/Radians.h"

// bounding volumes
#include "bv/Sphere.h"
#include "bv/Bounds.h"
#include "bv/Bounds2D.h"
#include "bv/BoundsShort.h"
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
#include "geometry/Surface_Traceable.h"
#include "geometry/TraceModel.h"
#include "geometry/TraceSurface.h"

// containers
#include "containers/Pair.h"
#include "containers/BTree.h"
#include "containers/BinSearch.h"
#include "containers/HashIndex.h"
#include "containers/HashMap.h"
#include "containers/HashMapGeneric.h"
#include "containers/StaticList.h"
#include "containers/LinkList.h"
#include "containers/Hierarchy.h"
#include "containers/LinkedList.h"
#include "containers/Queue.h"
#include "containers/Stack.h"
#include "containers/StrList.h"
#include "containers/StrPool.h"
#include "containers/VectorSet.h"
#include "containers/PlaneSet.h"
#include "containers/VectorWeld.h"
#include "containers/Grid.h"
#include "containers/QuadTree.h"
#include "containers/BitField.h"
#include "containers/Handles.h"
#include "containers/Deque.h"

// text manipulation
#include "text/LexerBinary.h"
#include "text/Lexer.h"
#include "text/Parser.h"
#include "text/WLexer.h"

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
#include "Singleton.h"
#include "PtrPolicies.h"
#include "AutoPtr.h"
#include "Factory.h"
#include "Callable.h"
#include "Color.h"
#include "Properties.h"
#include "TextUtilities.h"

// type info
/*
#include "typeinfo/TypeInfoFile.h"
#include "typeinfo/TypeInfoTools.h"
#include "typeinfo/TypeInfoObject.h"
#include "typeinfo/TypeInfoTree.h"
*/
#endif	/* !__LIB_H__ */
