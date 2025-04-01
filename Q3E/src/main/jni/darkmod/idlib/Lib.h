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
	static void					Printf( const char* fmt, ... );
	static void					Error( const char *fmt, ... );
	static void					Warning( const char *fmt, ... );
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

typedef int					    int32; //anon
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

// packs color floats in the range [0,1] into an integer
dword	PackColor( const idVec3 &color );
void	UnpackColor( const dword color, idVec3 &unpackedColor );
dword	PackColor( const idVec4 &color );
void	UnpackColor( const dword color, idVec4 &unpackedColor );

// little/big endian conversion
short	BigShort( short l );
short	LittleShort( short l );
int		BigInt( int l );
int		LittleInt( int l );
float	BigFloat( float l );
float	LittleFloat( float l );
void	BigRevBytes( void *bp, int elsize, int elcount );
void	LittleRevBytes( void *bp, int elsize, int elcount );
void	LittleBitField( void *bp, int elsize );
void	Swap_Init( void );

bool	Swap_IsBigEndian( void );

// for base64
void	SixtetsForInt( byte *out, int src);
int		IntForSixtets( byte *in );




template<class Context, class MethodPtr> struct LambdaToFuncPtr_detail {};
template<class Context, class Lambda, class Ret, class... Args> struct LambdaToFuncPtr_detail<Context, Ret (Lambda::*)(Args...) const> {
	// Oh crap! Why are they doing this to my beloved language?! =)))
	static inline Ret thunk(Context context, Args... args) {
		return ((Lambda*)context)->operator()(args...);
	}
};
//stgatilov: converts C++11 lambda into function pointer with void* context as first argument
//note: if you need function pointer without context, just assign a noncapturing lambda to function pointer (C++ allows that)
template<class Lambda> inline auto LambdaToFuncPtr(Lambda &lambda) -> auto {
	return &LambdaToFuncPtr_detail<void*, decltype(&Lambda::operator())>::thunk;
}


class idException {
public:
	char error[MAX_STRING_CHARS];

	// this really, really should be a const function, but it's referenced too many places to change right now
	const char* GetError() {
		return error;
	}	

	idException( const char *text = "" ) { strcpy( error, text ); }
};

//stgatilov: hack which can be used to avoid costly initialization, e.g. for large arrays of idVec3
//use with extreme caution! (do not apply to nontrivial objects)
template<class T> struct idRaw {
	alignas(T) char bytes[sizeof(T)];

	ID_FORCE_INLINE T &Get() { return *(T*)bytes; }
	ID_FORCE_INLINE const T &Get() const { return *(const T*)bytes; }
	ID_FORCE_INLINE T *Ptr() { return (T*)bytes; }
	ID_FORCE_INLINE const T *Ptr() const { return (const T*)bytes; }

	void destructor() {
		((T*)bytes)->~T();
	}
	void constructor() {
		new(bytes) T();
	}
	template<class... Args> void constructor(Args&&... args) {
		new(bytes) T(static_cast<Args&&>(args)...);
	}
};

/*
===============================================================================

	idLib headers.

===============================================================================
*/

// System
#include "sys/sys_assert.h"
#include "sys/sys_threading.h"

// memory management and arrays
#include "Heap.h"
#include "Allocators.h"
#include "containers/List.h"

// math
#include "math/Simd.h"
#include "math/Math.h"
#include "math/Random.h"
#include "math/Complex.h"
#include "math/Vector.h"
#include "math/Matrix.h"
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

// bounding volumes
#include "bv/Sphere.h"
#include "bv/Bounds.h"
#include "bv/Box.h"
#include "bv/Frustum.h"

// geometry
#include "geometry/RenderMatrix.h"
#include "geometry/DrawVert.h"
#include "geometry/JointTransform.h"
#include "geometry/Winding.h"
#include "geometry/Winding2D.h"
#include "geometry/Surface.h"
#include "geometry/Surface_Patch.h"
#include "geometry/Surface_Polytope.h"
#include "geometry/Surface_SweptSpline.h"
#include "geometry/TraceModel.h"

// text manipulation
#include "Str.h"
#include "Token.h"
#include "Lexer.h"
#include "Parser.h"
//#include "Base64.h"
#include "CmdArgs.h"

// containers
#include "containers/BTree.h"
#include "containers/BinSearch.h"
#include "containers/HashIndex.h"
#include "containers/HashTable.h"
#include "containers/HashMap.h"
#include "containers/StaticList.h"
#include "containers/LinkList.h"
#include "containers/Hierarchy.h"
#include "containers/Queue.h"
#include "containers/Stack.h"
#include "containers/StrList.h"
#include "containers/StrPool.h"
#include "containers/VectorSet.h"
#include "containers/PlaneSet.h"

// hashing
//#include "hashing/CRC32.h"
#include "hashing/MD4.h"
#include "hashing/MD5.h"

// misc
#include "Dict.h"
#include "LangDict.h"
#include "BitMsg.h"
#include "MapFile.h"
#include "Timer.h"
#include "Thread.h"
#include "RevisionTracker.h"
#include "ParallelJobList.h"

#endif	/* !__LIB_H__ */
