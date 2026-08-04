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



#include "renderer/resources/CinematicID.h"
#include "renderer/resources/CinematicFFMpeg.h"
#include "renderer/tr_local.h"


/*
==============
idCinematic::InitCinematic
==============
*/
void idCinematic::InitCinematic( void ) {
	idCinematicLocal::InitCinematic();
	idCinematicFFMpeg::InitCinematic();
}

/*
==============
idCinematic::ShutdownCinematic
==============
*/
void idCinematic::ShutdownCinematic( void ) {
	idCinematicLocal::ShutdownCinematic();
	idCinematicFFMpeg::ShutdownCinematic();
}

/*
==============
idCinematic::Alloc
==============
*/
idCinematic *idCinematic::Alloc( const char *qpath ) {
	int mode = r_cinematic_legacyRoq.GetInteger();
	bool useID = !qpath || mode == 2 || (mode == 1 && idStr::CheckExtension(qpath, ".roq"));
	if (useID)
		return new idCinematicLocal();
	else
		return new idCinematicFFMpeg();
}

/*
==============
idCinematic::~idCinematic
==============
*/
idCinematic::~idCinematic( ) {
	Close();
}

/*
==============
idCinematic::InitFromFile
==============
*/
bool idCinematic::InitFromFile( const char *qpath, bool looping, bool withAudio ) {
	return false;
}

/*
==============
idCinematic::InitFromFile
==============
*/
const char *idCinematic::GetFilePath() const {
	return nullptr;
}

/*
==============
idCinematic::AnimationLength
==============
*/
int idCinematic::AnimationLength() {
	return 0;
}

/*
==============
idCinematic::ResetTime
==============
*/
void idCinematic::ResetTime(int milliseconds) {
}

/*
==============
idCinematic::ImageForTime
==============
*/
cinData_t idCinematic::ImageForTime( int milliseconds ) {
	cinData_t c;
	memset( &c, 0, sizeof( c ) );
	return c;
}

bool idCinematic::SoundForTimeInterval(int sampleOffset44k, int *sampleSize, float *output) {
	return false;
}

int idCinematic::GetRealSoundOffset(int sampleOffset) const {
	return sampleOffset;
}

cinStatus_t idCinematic::GetStatus() const {
	return FMV_PLAY;
}

/*
==============
idCinematic::Close
==============
*/
void idCinematic::Close() {
}


//===========================================

/*
==============
idSndWindow::InitFromFile
==============
*/
bool idSndWindow::InitFromFile( const char *qpath, bool looping, bool withAudio ) {
	idStr fname = qpath;

	fname.ToLower();
	if ( !fname.Icmp( "waveform" ) ) {
		showWaveform = true;
	} else {
		showWaveform = false;
	}
	return true;
}

/*
==============
idSndWindow::ImageForTime
==============
*/
cinData_t idSndWindow::ImageForTime( int milliseconds ) {
	return soundSystem->ImageForTime( milliseconds, showWaveform );
}

/*
==============
idSndWindow::AnimationLength
==============
*/
int idSndWindow::AnimationLength() {
	return -1;
}
