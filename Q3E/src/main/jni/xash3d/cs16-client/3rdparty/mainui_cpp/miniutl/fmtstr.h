//========= Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A simple class for performing safe and in-expression sprintf-style
//			string formatting
//
// $NoKeywords: $
//=============================================================================//

#ifndef FMTSTR_H
#define FMTSTR_H

#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "strtools.h"

#if defined( _WIN32 )
#pragma once
#endif

#if defined(POSIX)
// clang will error if the symbol visibility for an object changes between static libraries (and/or your main dylib)
// so force the FmtStr templates and importantly the global scAsserted below to hidden (i.e don't escape the dll) forcefully
#pragma GCC visibility push(hidden)
#endif

//=============================================================================

// using macro to be compatible with GCC
#define FmtStrVSNPrintf( szBuf, nBufSize, refnFormattedLength, bQuietTruncation, pszFormat, lastArg ) \
	do \
	{ \
		va_list arg_ptr; \
		bool bTruncated = false; \
		static int scAsserted = 0; \
	\
		va_start(arg_ptr, lastArg); \
		(refnFormattedLength) = V_vsnprintfRet( (szBuf), (nBufSize), pszFormat, arg_ptr, &bTruncated ); \
		va_end(arg_ptr); \
	\
		(szBuf)[(nBufSize)-1] = 0; \
		if ( bTruncated && !(bQuietTruncation) && scAsserted < 5 ) \
		{ \
			Assert( !bTruncated ); \
			scAsserted++; \
		} \
	} \
	while (0)


//-----------------------------------------------------------------------------
//
// Purpose: String formatter with specified size
//

class CNumStr
{
public:

	CNumStr() { m_szBuf[0] = 0; m_nLength = 0; }
	explicit CNumStr( bool b )        { SetBool( b ); }
	explicit CNumStr( int8_t n8 )     { SetInt8( n8 ); }
	explicit CNumStr( uint8_t un8 )   { SetUint8( un8 ); }
	explicit CNumStr( int16_t n16 )   { SetInt16( n16 ); }
	explicit CNumStr( uint16_t un16 ) { SetUint16( un16 ); }
	explicit CNumStr( int32_t n32 )   { SetInt32( n32 ); }
	explicit CNumStr( uint32_t un32 ) { SetUint32( un32 ); }
	explicit CNumStr( int64_t n64 )   { SetInt64( n64 ); }
	explicit CNumStr( uint64_t un64 ) { SetUint64( un64 ); }
	explicit CNumStr( double f )      { SetDouble( f ); }
	explicit CNumStr( float f )       { SetFloat( f ); }

#define NUMSTR_FAST_DIGIT( digit )					{ m_nLength = 1; m_szBuf[0] = '0' + ( digit ); m_szBuf[1] = 0; return m_szBuf; }
#define NUMSTR_CHECK_FAST( val, utype )				if ( (utype)val < 10 ) NUMSTR_FAST_DIGIT( (utype)val )

	inline const char* SetBool( bool b )          { NUMSTR_FAST_DIGIT( b & 1 ); }
	inline const char* SetInt8( int8_t n8 )       { NUMSTR_CHECK_FAST( n8,   uint8_t )  m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%" PRIi32, n8 ); return m_szBuf; }
	inline const char* SetUint8( uint8_t un8 )    { NUMSTR_CHECK_FAST( un8,  uint8_t )  m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%" PRIi32, un8 ); return m_szBuf; }
	inline const char* SetInt16( int16_t n16 )    { NUMSTR_CHECK_FAST( n16,  uint16_t ) m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%" PRIi32, n16 ); return m_szBuf; }
	inline const char* SetUint16( uint16_t un16 ) { NUMSTR_CHECK_FAST( un16, uint16_t ) m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%" PRIi32, un16 ); return m_szBuf; }
	inline const char* SetInt32( int32_t n32 )    { NUMSTR_CHECK_FAST( n32,  uint32_t ) m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%" PRIi32, n32 ); return m_szBuf; }
	inline const char* SetUint32( uint32_t un32 ) { NUMSTR_CHECK_FAST( un32, uint32_t ) m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%" PRIu32, un32 ); return m_szBuf; }
	inline const char* SetInt64( int64_t n64 )    { NUMSTR_CHECK_FAST( n64,  uint64_t ) m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%" PRIi64, n64 ); return m_szBuf; }
	inline const char* SetUint64( uint64_t un64 ) { NUMSTR_CHECK_FAST( un64, uint64_t ) m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%" PRIu64, un64 ); return m_szBuf; }
	inline const char* SetHexUint64( uint64_t un64 ) { m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%" PRIx64, un64 ); return m_szBuf; }

	inline const char* SetDouble( double f )
	{
#ifndef _MSC_VER
		if ( f == 0.0  && !signbit( f ) )
			NUMSTR_FAST_DIGIT( 0 );
#endif
		if ( f == 1.0 )
			NUMSTR_FAST_DIGIT( 1 );
		m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%.18g", f );
		return m_szBuf;
	}
	inline const char* SetFloat( float f )
	{
#ifndef _MSC_VER
		if ( f == 0.0f && !signbit( f ) )
			NUMSTR_FAST_DIGIT( 0 );
#endif
		if ( f == 1.0f )
			NUMSTR_FAST_DIGIT( 1 );
		m_nLength = V_snprintf( m_szBuf, sizeof( m_szBuf ), "%.18g", f );
		return m_szBuf;
	}
	//SDR_PUBLIC

#undef NUMSTR_FAST_DIGIT
#undef NUMSTR_CHECK_FAST

	operator const char *() const { return m_szBuf; }
	char *Access() { return m_szBuf; }
	const char* String() const { return m_szBuf; }
	int Length() const { return m_nLength; }

	void AddQuotes()
	{
		Assert( m_nLength + 2 <= V_ARRAYSIZE(m_szBuf) );
		memmove( m_szBuf+1, m_szBuf, m_nLength );
		m_szBuf[0] = '"';
		m_szBuf[m_nLength+1] = '"';
		m_nLength+=2;
		m_szBuf[m_nLength]=0;
	}

protected:
	char m_szBuf[28]; // long enough to hold 18 digits of precision, a decimal, a - sign, e+### suffix, and quotes
	int m_nLength;
};

#endif // FMTSTR_H
