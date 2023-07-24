// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

template< typename T > ID_INLINE bool Is1ByteUTF8SequenceStart( const T value ) { return ( ( value & 0x80 ) == 0x00 ); }
template< typename T > ID_INLINE bool Is2ByteUTF8SequenceStart( const T value ) { return ( ( value & 0xE0 ) == 0xC0 ); }
template< typename T > ID_INLINE bool Is3ByteUTF8SequenceStart( const T value ) { return ( ( value & 0xF0 ) == 0xE0 ); }
template< typename T > ID_INLINE bool IsValidUTF8Sequence( const T value ) { return ( ( value & 0xC0 ) == 0x80 ); }

template< typename T > ID_INLINE bool Is3ByteUTF8Sequence( const T value ) { return value > 0x7FF; }
template< typename T > ID_INLINE bool Is2ByteUTF8Sequence( const T value ) { return value > 0x7F; }

/*
============
sdUTF8::sdUTF8
============
*/
sdUTF8::sdUTF8( idFile* file ) {
	Init();
	EnsureAlloced( file->Length() );
	len = alloced;
	file->Read( data, len );
}

/*
============
sdUTF8::sdUTF8
============
*/
sdUTF8::sdUTF8( const byte* data, const int size ) {
	Init();
	EnsureAlloced( size );
	len = alloced;
	::memcpy( this->data, data, len );
}

/*
============
sdUTF8::DecodeLength
============
*/
int sdUTF8::DecodeLength() const {
	// count the number of characters in the UTF-8 data
	int length = 0;

	const byte* ptr = data;
	
	while ( ( ptr - data ) < len ) {
		if ( Is1ByteUTF8SequenceStart( *ptr ) ) {
			length++;
			ptr += 1;
			continue;
		} else if ( Is2ByteUTF8SequenceStart( *ptr ) && IsValidUTF8Sequence( *(ptr + 1) ) ) {
			length++;
			ptr += 2;
			continue;
		} else if ( Is3ByteUTF8SequenceStart( *ptr ) && IsValidUTF8Sequence( *(ptr + 1) ) && IsValidUTF8Sequence( *(ptr + 2) ) ) {
			length++;
			ptr += 3;
			continue;
		} else {
			// malformed UTF-8 data
			//assert( false );
			length++;
			ptr += 1; 
		}
	}

	return length;
}

/*
============
sdUTF8::Decode
============
*/
int sdUTF8::Decode( wchar_t* to ) const {
	int i = 0;
	int decodeLength;
	wchar_t* ptr = to;

	while ( i < len ) {
		decodeLength = UTF8toUCS2( data + i, len - i, ptr );

		if ( decodeLength < 0 ) {
			break;
		}

		i += decodeLength;
		ptr += 1;
	}

	*ptr = L'\0';

	return ( ptr - to );
}


/*
============
sdUTF8::Encode
============
*/
void sdUTF8::Encode( idFile* file, const wchar_t* data, int len ) {
	int index = 0;

	while( index < len ) {
		if( Is3ByteUTF8Sequence( data[ index ] ) ) {
			file->WriteUnsignedChar( 0xE0 | ( data[ index ] >> 12 ) );
			file->WriteUnsignedChar( 0x80 | ( ( data[ index ] >> 6 ) & 0x3F ) );
			file->WriteUnsignedChar( 0x80 | ( data[ index ] & 0x3F ) );
		} else if( Is2ByteUTF8Sequence( data[ index ] ) ) {
			file->WriteUnsignedChar( 0xC0 | ( ( data[ index ] >> 6 ) & 0x1F ) );
			file->WriteUnsignedChar( 0x80 | ( data[ index ] & 0x3F ) );
		} else {			
			file->WriteUnsignedChar( data[ index ] );
		}
		index++;
	}
}

/*
============
sdUTF8::UTF8toUCS2
============
*/
int sdUTF8::UTF8toUCS2( const byte* data, const int len, wchar_t* ucs2 ) const {
	wchar_t b0, b1, b2;

	if ( len < 1 ) {
		return -1;
	}

	b0 = static_cast< wchar_t >( *data );

	if ( Is1ByteUTF8SequenceStart( b0 ) ) {
		*ucs2 = ( b0 & 0x7F );
		return 1;
	} else if ( Is2ByteUTF8SequenceStart( b0 ) ) {
		if ( len < 2 ) {
			return -1;
		}
		b1 = static_cast< wchar_t >( *(data + 1) );
		if ( !IsValidUTF8Sequence( b1 ) ) {
			return -2;
		}
		*ucs2 = ( ( b0 & 0x1F ) << 6 ) | ( b1 & 0x3F );
		return 2;
	} else if ( Is3ByteUTF8SequenceStart( b0 ) ) {
		if ( len < 3 ) {
			return -1;
		}
		b1 = static_cast< wchar_t >( *(data + 1) );
		b2 = static_cast< wchar_t >( *(data + 2) );
		if ( !IsValidUTF8Sequence( b1 ) || !IsValidUTF8Sequence( b2 ) ) {
			return -2;
		}
		*ucs2 = ( ( b0 & 0x0F ) << 12 ) | ( ( b1 & 0x3F ) << 6 ) | ( b2 & 0x3F );
		return 3;
	} else {
		return -2;
	}
}
