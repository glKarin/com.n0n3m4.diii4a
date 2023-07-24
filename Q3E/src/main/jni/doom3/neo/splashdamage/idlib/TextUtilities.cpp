// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

/*
============
sdTextUtilities::WriteFloatString
============
*/
int sdTextUtilities::Write( idFile* file, const char* string, bool indent ) {	
	int indentLevel = 0;
	int index = fileList.FindIndex( file );

	if( index >= 0 ) {
		indentLevel = indentList[ index ];
	}

	if( indent ) {
		idStr ind;
		ind.Fill( '\t', indentLevel );
		return file->WriteFloatString( ind + string );
	} else {
		return file->WriteFloatString( string );
	}
}


/*
============
sdTextUtilities::Indent
============
*/
void sdTextUtilities::Indent( idFile* file ) {
	int index = fileList.FindIndex( file );
	if( index < 0 ) {
		fileList.Append( file );
		indentList.Append( 1 );
		return;
	}

	indentList[ index ] = indentList[ index ] + 1;
}


/*
============
sdTextUtilities::Unindent
============
*/
void sdTextUtilities::Unindent( idFile* file ) {
	int index = fileList.FindIndex( file );
	if( index < 0 ) {
		fileList.Append( file );
		indentList.Append( 0 );
		return;
	}

	if( indentList[ index ] != 0 ) {
		indentList[ index ] = indentList[ index ] - 1;
	} else {
		common->Warning( "sdTextUtilities::Unindent: mismatched unindent!" );
	}
}


/*
============
sdTextUtilities::CloseFile
============
*/
void sdTextUtilities::CloseFile( idFile* file ) {
	int index = fileList.FindIndex( file );
	if( index >= 0 ) {
		indentList.RemoveIndex( index );
		fileList.RemoveIndex( index );
	} else {
		common->Warning( "sdTextUtilities::CloseFile: Unknown file" );
	}
}
