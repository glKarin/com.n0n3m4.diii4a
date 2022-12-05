
#include "precompiled.h"
#pragma hdrstop

#if defined( MACOS_X )
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/*
===============================================================================

	idLib

===============================================================================
*/

idSys *			idLib::sys			= NULL;
idCommon *		idLib::common		= NULL;
idCVarSystem *	idLib::cvarSystem	= NULL;
idFileSystem *	idLib::fileSystem	= NULL;
int				idLib::frameNumber	= 0;

/*
================
idLib::Init
================
*/
void idLib::Init( void ) {

	// initialize little/big endian conversion
	Swap_Init();

	// initialize memory manager
	Mem_Init();

// RAVEN BEGIN
// dluetscher: added the following code to initialize each of the memory heaps immediately
//			   following Mem_Init()
#ifdef _RV_MEM_SYS_SUPPORT
	// initialize each of the memory heaps
	common->InitHeaps();
#endif
// RAVEN END

	// init string memory allocator
	idStr::InitMemory();

	// initialize generic SIMD implementation
	idSIMD::Init();

	// initialize math
	idMath::Init();

// RAVEN BEGIN
// jsinger: There is no reason for us to be doing this on the Xenon
#ifndef _XENON
	// test idMatX
	//idMatX::Test();

	// test idPolynomial
	idPolynomial::Test();
#endif
// RAVEN END

	// initialize the dictionary string pools
	idDict::Init();
}

/*
================
idLib::ShutDown
================
*/
void idLib::ShutDown( void ) {

	// shut down the dictionary string pools
	idDict::Shutdown();

	// shut down the string memory allocator
	idStr::ShutdownMemory();

	// shut down the SIMD engine
	idSIMD::Shutdown();

// RAVEN BEGIN
// dluetscher: added the following code to shutdown each of the memory heaps immediately
//			   before Mem_Shutdown()
#ifdef _RV_MEM_SYS_SUPPORT
	// shutdown each of the memory heaps
	common->ShutdownHeaps();
#endif
// RAVEN END

	// shut down the memory manager
	Mem_Shutdown();
}


/*
===============================================================================

	Colors

===============================================================================
*/

idVec4	colorBlack	= idVec4( 0.00f, 0.00f, 0.00f, 1.00f );
idVec4	colorWhite	= idVec4( 1.00f, 1.00f, 1.00f, 1.00f );
idVec4	colorRed	= idVec4( 1.00f, 0.00f, 0.00f, 1.00f );
idVec4	colorGreen	= idVec4( 0.00f, 1.00f, 0.00f, 1.00f );
idVec4	colorBlue	= idVec4( 0.00f, 0.00f, 1.00f, 1.00f );
idVec4	colorYellow	= idVec4( 1.00f, 1.00f, 0.00f, 1.00f );
idVec4	colorMagenta= idVec4( 1.00f, 0.00f, 1.00f, 1.00f );
idVec4	colorCyan	= idVec4( 0.00f, 1.00f, 1.00f, 1.00f );
idVec4	colorOrange	= idVec4( 1.00f, 0.50f, 0.00f, 1.00f );
idVec4	colorPurple	= idVec4( 0.60f, 0.00f, 0.60f, 1.00f );
idVec4	colorPink	= idVec4( 0.73f, 0.40f, 0.48f, 1.00f );
idVec4	colorBrown	= idVec4( 0.40f, 0.35f, 0.08f, 1.00f );
idVec4	colorLtGrey	= idVec4( 0.75f, 0.75f, 0.75f, 1.00f );
idVec4	colorMdGrey	= idVec4( 0.50f, 0.50f, 0.50f, 1.00f );
idVec4	colorDkGrey	= idVec4( 0.25f, 0.25f, 0.25f, 1.00f );

static dword colorMask[2] = { 255, 0 };

/*
================
ColorFloatToByte
================
*/
ID_INLINE static byte ColorFloatToByte( float c ) {
	return (byte) ( ( (dword) ( c * 255.0f ) ) & colorMask[FLOATSIGNBITSET(c)] );
}

/*
================
PackColor
================
*/
dword PackColor( const idVec4 &color ) {
	dword dw, dx, dy, dz;

	dx = ColorFloatToByte( color.x );
	dy = ColorFloatToByte( color.y );
	dz = ColorFloatToByte( color.z );
	dw = ColorFloatToByte( color.w );

// RAVEN BEGIN
// jnewquist: Big endian support
#ifdef _LITTLE_ENDIAN
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 ) | ( dw << 24 );
#else
	return ( dx << 24 ) | ( dy << 16 ) | ( dz << 8 ) | ( dw << 0 );
#endif
// RAVEN END
}

/*
================
UnpackColor
================
*/
void UnpackColor( const dword color, idVec4 &unpackedColor ) {
// RAVEN BEGIN
// jnewquist: Xenon is big endian
#ifdef _LITTLE_ENDIAN
	unpackedColor.Set( ( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ), 
						( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ) );
#else
	unpackedColor.Set( ( ( color >> 24 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ), 
						( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ) );
#endif
// RAVEN END
}

/*
================
PackColor
================
*/
dword PackColor( const idVec3 &color ) {
	dword dx, dy, dz;

	dx = ColorFloatToByte( color.x );
	dy = ColorFloatToByte( color.y );
	dz = ColorFloatToByte( color.z );

// RAVEN BEGIN
// jnewquist: Xenon is big endian
#ifdef _LITTLE_ENDIAN
	return ( dx << 0 ) | ( dy << 8 ) | ( dz << 16 );
#else
	return ( dy << 16 ) | ( dz << 8 ) | ( dx << 0 );
#endif
// RAVEN END
}

/*
================
UnpackColor
================
*/
void UnpackColor( const dword color, idVec3 &unpackedColor ) {
// RAVEN BEGIN
// jnewquist: Xenon is big endian
#ifdef _LITTLE_ENDIAN
	unpackedColor.Set( ( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ), 
						( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ) );
#else
	unpackedColor.Set( ( ( color >> 16 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 8 ) & 255 ) * ( 1.0f / 255.0f ),
						( ( color >> 0 ) & 255 ) * ( 1.0f / 255.0f ) );
#endif
// RAVEN END
}

/*
===============
idLib::Error
===============
*/
void idLib::Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Error( "%s", text );
}

/*
===============
idLib::Warning
===============
*/
void idLib::Warning( const char *fmt, ... ) {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Warning( "%s", text );
}

/*
===============================================================================

	Byte order functions

===============================================================================
*/

// can't just use function pointers, or dll linkage can mess up
static short	(*_BigShort)( short l );
static short	(*_LittleShort)( short l );
static int		(*_BigLong)( int l );
static int		(*_LittleLong)( int l );
static float	(*_BigFloat)( float l );
static float	(*_LittleFloat)( float l );
static void		(*_BigRevBytes)( void *bp, int elsize, int elcount );
static void		(*_LittleRevBytes)( void *bp, int elsize, int elcount );
static void		(*_SixtetsForInt)( byte *out, int src );
static int		(*_IntForSixtets)( byte *in );

#ifdef _LITTLE_ENDIAN

short	LittleShort( short l ) { return l; }
int		LittleLong( int l ) { return l; }
float	LittleFloat( float l ) { return l; }
void	LittleRevBytes( void *bp, int elsize, int elcount ) {}

short	BigShort( short l ) { return _BigShort( l ); }
int		BigLong( int l ) { return _BigLong( l ); }
float	BigFloat( float l ) { return _BigFloat( l ); }
void	BigRevBytes( void *bp, int elsize, int elcount ) { _BigRevBytes( bp, elsize, elcount ); }

#else

short	LittleShort( short l ) { return _LittleShort( l ); }
int		LittleLong( int l ) { return _LittleLong( l ); }
float	LittleFloat( float l ) { return _LittleFloat( l ); }
void	LittleRevBytes( void *bp, int elsize, int elcount ) { _LittleRevBytes( bp, elsize, elcount ); }

short	BigShort( short l ) { return l; }
int		BigLong( int l ) { return l; }
float	BigFloat( float l ) { return l; }
void	BigRevBytes( void *bp, int elsize, int elcount ) {}

#endif


void	SixtetsForInt( byte *out, int src) { _SixtetsForInt( out, src ); }
int		IntForSixtets( byte *in ) { return _IntForSixtets( in ); }

/*
================
ShortSwap
================
*/
short ShortSwap( short l ) {
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

/*
================
ShortNoSwap
================
*/
short ShortNoSwap( short l ) {
	return l;
}

/*
================
LongSwap
================
*/
int LongSwap ( int l ) {
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

/*
================
LongNoSwap
================
*/
int	LongNoSwap( int l ) {
	return l;
}

/*
================
FloatSwap
================
*/
float FloatSwap( float f ) {
	union {
		float	f;
		byte	b[4];
	} dat1, dat2;
	
	
	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

/*
================
FloatNoSwap
================
*/
float FloatNoSwap( float f ) {
	return f;
}

/*
=====================================================================
RevBytesSwap

Reverses byte order in place.

INPUTS
   bp       bytes to reverse
   elsize   size of the underlying data type
   elcount  number of elements to swap

RESULTS
   Reverses the byte order in each of elcount elements.
===================================================================== */
void RevBytesSwap( void *bp, int elsize, int elcount ) {
	register unsigned char *p, *q;

	p = ( unsigned char * ) bp;

	if ( elsize == 2 ) {
		q = p + 1;
		while ( elcount-- ) {
			*p ^= *q;
			*q ^= *p;
			*p ^= *q;
			p += 2;
			q += 2;
		}
		return;
	}

	while ( elcount-- ) {
		q = p + elsize - 1;
		while ( p < q ) {
			*p ^= *q;
			*q ^= *p;
			*p ^= *q;
			++p;
			--q;
		}
		p += elsize >> 1;
	}
}

/*
================
RevBytesNoSwap
================
*/
void RevBytesNoSwap( void *bp, int elsize, int elcount ) {
	return;
}

/*
================
SixtetsForIntLittle
================
*/
void SixtetsForIntLittle( byte *out, int src) {
	byte *b = (byte *)&src;
	out[0] = ( b[0] & 0xfc ) >> 2;
	out[1] = ( ( b[0] & 0x3 ) << 4 ) + ( ( b[1] & 0xf0 ) >> 4 );
	out[2] = ( ( b[1] & 0xf ) << 2 ) + ( ( b[2] & 0xc0 ) >> 6 );
	out[3] = b[2] & 0x3f;
}

/*
================
SixtetsForIntBig
TTimo: untested - that's the version from initial base64 encode
================
*/
void SixtetsForIntBig( byte *out, int src) {
	for( int i = 0 ; i < 4 ; i++ ) {
		out[i] = src & 0x3f;
		src >>= 6;
	}
}

/*
================
IntForSixtetsLittle
================
*/
int IntForSixtetsLittle( byte *in ) {
	int ret = 0;
	byte *b = (byte *)&ret;
	b[0] |= in[0] << 2;
	b[0] |= ( in[1] & 0x30 ) >> 4;
	b[1] |= ( in[1] & 0xf ) << 4;
	b[1] |= ( in[2] & 0x3c ) >> 2;
	b[2] |= ( in[2] & 0x3 ) << 6;
	b[2] |= in[3];
	return ret;
}

/*
================
IntForSixtetsBig
TTimo: untested - that's the version from initial base64 decode
================
*/
int IntForSixtetsBig( byte *in ) {
	int ret = 0;
	ret |= in[0];
	ret |= in[1] << 6;
	ret |= in[2] << 2*6;
	ret |= in[3] << 3*6;
	return ret;
}

/*
================
Swap_Init
================
*/
void Swap_Init( void ) {
	byte	swaptest[2] = {1,0};

	// set the byte swapping variables in a portable manner	
	if ( *(short *)swaptest == 1) {
		// little endian ex: x86
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
		_BigRevBytes = RevBytesSwap;
		_LittleRevBytes = RevBytesNoSwap;
		_SixtetsForInt = SixtetsForIntLittle;
		_IntForSixtets = IntForSixtetsLittle;
	} else {
		// big endian ex: ppc
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
		_BigRevBytes = RevBytesNoSwap;
		_LittleRevBytes = RevBytesSwap;
		_SixtetsForInt = SixtetsForIntBig;
		_IntForSixtets = IntForSixtetsBig;
	}
}

/*
==========
Swap_IsBigEndian
==========
*/
bool Swap_IsBigEndian( void ) {
	byte	swaptest[2] = {1,0};
	return *(short *)swaptest != 1;
}

/*
===============================================================================

	Assertion

===============================================================================
*/

void AssertFailed( const char *file, int line, const char *expression ) {
	if ( idLib::sys ) {
		idLib::sys->DebugPrintf( "\n\nASSERTION FAILED!\n%s(%d): '%s'\n", file, line, expression );
	}
#ifdef _WIN32
// RAVEN BEGIN
// jnewquist: Visual Studio platform independent breakpoint
	__debugbreak();
// RAVEN END
#elif defined( __linux__ )
	__asm__ __volatile__ ("int $0x03");
#elif defined( MACOS_X )
	kill( getpid(), SIGINT );
#endif
}
