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
#include "precompiled.h"
#pragma hdrstop



#include "roq.h"

void R_LoadImage( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp );

NSBitmapImageRep::NSBitmapImageRep( void ) {
	bmap = NULL;
	width = 0;
	height = 0;
	timestamp = 0;
}

NSBitmapImageRep::NSBitmapImageRep( const char *filename ) {

	R_LoadImage( filename, &bmap, &width, &height, &timestamp );
	if (!width || !height) {
		common->FatalError( "roqvq: unable to load image %s\n", filename );
	}
}

NSBitmapImageRep::NSBitmapImageRep( int wide, int high ) {
	bmap = (byte *)Mem_ClearedAlloc( wide * high * 4 );
	width = wide;
	height = high;
}

void R_StaticFree( void *data );

NSBitmapImageRep::~NSBitmapImageRep() {
	R_StaticFree( bmap );
	bmap = NULL;
}

int NSBitmapImageRep::samplesPerPixel( void ) {
	return 4;
}

int NSBitmapImageRep::pixelsWide( void ) {
	return width;
}

int NSBitmapImageRep::pixelsHigh( void ) {
	return height;
}

byte * NSBitmapImageRep::bitmapData( void ) {
	return bmap;
}

bool NSBitmapImageRep::hasAlpha( void ) {
	return false;
}

bool NSBitmapImageRep::isPlanar( void ) {
	return false;
}

NSBitmapImageRep &NSBitmapImageRep::operator=( const NSBitmapImageRep &a ) {

	// check for assignment to self
	if ( this == &a ) {
		return *this;
	}

	if (bmap) {
		Mem_Free(bmap);
	}
	bmap	= (byte *)Mem_Alloc( a.width * a.height * 4 );
	memcpy( bmap, a.bmap, a.width * a.height * 4 );
	width = a.width;
	height = a.height;
	timestamp = a.timestamp;

	return *this;
}

