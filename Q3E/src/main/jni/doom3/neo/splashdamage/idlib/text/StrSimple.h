// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __STRSIMPLE_H__
#define __STRSIMPLE_H__

/*
===============================================================================

	Character string class that doesn't use the string data allocator but instead the thread safe OS memory allocation calls

===============================================================================
*/

class idSimpleStr : public idStr {
public:
						idSimpleStr( void );
						idSimpleStr( const idStr &text );
						idSimpleStr( const idStr &text, int start, int end );
						idSimpleStr( const char *text );
						idSimpleStr( const char *text, int start, int end );
						explicit idSimpleStr( const bool b );
						explicit idSimpleStr( const char c );
						explicit idSimpleStr( const int i );
						explicit idSimpleStr( const unsigned u );
						explicit idSimpleStr( const float f );

	void				ReAllocate( int amount, bool keepold );				// reallocate string data buffer
	void				FreeData( void );									// free allocated string memory
};


ID_INLINE idSimpleStr::idSimpleStr( void ) :
	idStr() {
}

ID_INLINE idSimpleStr::idSimpleStr( const idStr &text ) :
	idStr( text ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const idStr &text, int start, int end ) :
	idStr( text, start, end ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const char *text ) :
	idStr( text ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const char *text, int start, int end ) :
	idStr( text, start, end ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const bool b ) :
	idStr( b ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const char c ) :
	idStr( c ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const int i ) :
	idStr( i ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const unsigned u ) :
	idStr( u ) {
}

ID_INLINE idSimpleStr::idSimpleStr( const float f ) :
	idStr( f ) {
}

#endif /* !__STRSIMPLE_H__ */
