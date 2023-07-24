// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

/*
============
idSimpleStr::ReAllocate
============
*/
void idSimpleStr::ReAllocate( int amount, bool keepold ) {
	char	*newbuffer;
	int		newsize;
	int		mod;

	//assert( data );
	assert( amount > 0 );

	mod = amount % STR_ALLOC_GRAN;
	if ( !mod ) {
		newsize = amount;
	} else {
		newsize = amount + STR_ALLOC_GRAN - mod;
	}
	alloced = newsize;

	newbuffer = new char[ alloced ];
	if ( keepold && data ) {
		if ( len ) {
			strncpy( newbuffer, data, len );
			newbuffer[ len ] = '\0';
		} else {
			newbuffer[0] = '\0';
		}
	}

	if ( data && data != baseBuffer ) {
		delete [] data;
	}

	data = newbuffer;
}

/*
============
idSimpleStr::FreeData
============
*/
void idSimpleStr::FreeData( void ) {
	if ( data && data != baseBuffer ) {
		delete[] data;
		data = baseBuffer;
	}
}
