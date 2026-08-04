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

// Copyright (C) 2010 Tels (Donated to The Dark Mod Team)

/*
	LightController

	Lights and local ambient lights are registered with the light controller.
	This object then controls the brightness of the local ambient lights depending
	on the light energy of the other lights.

	TODO: Basically everything.

*/

#include "precompiled.h"
#pragma hdrstop



#include "LightController.h"

/*
===============
CLightController::CLightController
===============
*/
CLightController::CLightController( void ) {

	// the ambient lights we control
	m_Ambients.Clear();
	// the lights we watch and use to change the ambients
	m_Lights.Clear();
	m_bActive = true;
}

CLightController::~CLightController()
{
	Shutdown();
}

/*
===============
CLightController::Init - will be called by game_local
===============
*/
void CLightController::Init ( void )
{
}

/*
===============
CLightController::Shutdown
===============
*/
void CLightController::Shutdown ( void )
{
}

/*
===============
CLightController::Clear - will be called by game_local
===============
*/
void CLightController::Clear ( void )
{
	m_Ambients.Clear();
	m_Lights.Clear();
}

/*
===============
CLightController::Save
===============
*/
void CLightController::Save( idSaveGame *savefile ) const {

	savefile->WriteBool( m_bActive );

//	savefile->WriteInt( m_iNextUpdate );
//	savefile->WriteInt( m_iUpdateTime );

	int n = m_Ambients.Num();
	savefile->WriteInt( n );
	for (int i = 0; i < n; i++ )
	{
		savefile->WriteVec3( m_Ambients[i].origin );
		savefile->WriteVec3( m_Ambients[i].color );
		savefile->WriteVec3( m_Ambients[i].target_color );
	}

	n = m_Lights.Num();
	savefile->WriteInt( n );
	for (int i = 0; i < n; i++ )
	{
		savefile->WriteVec3( m_Lights[i].origin );
		savefile->WriteVec3( m_Lights[i].color );
		savefile->WriteFloat( m_Lights[i].radius );
	}
}

/*
===============
CLightController::Restore
===============
*/
void CLightController::Restore( idRestoreGame *savefile ) {

	savefile->ReadBool( m_bActive );
//	savefile->ReadInt( m_iNextUpdate );
//	savefile->ReadInt( m_iUpdateTime );

	m_Ambients.Clear();
	int n;

	savefile->ReadInt( n );
	m_Ambients.SetNum(n);
	for (int i = 0; i < n; i ++)
	{
		savefile->ReadVec3( m_Ambients[i].origin );
		savefile->ReadVec3( m_Ambients[i].color );
		savefile->ReadVec3( m_Ambients[i].target_color );
	}

	m_Lights.Clear();
	savefile->ReadInt( n );
	m_Lights.SetNum(n);
	for (int i = 0; i < n; i ++)
	{
		savefile->ReadVec3( m_Ambients[i].origin );
		savefile->ReadVec3( m_Lights[i].color );
		savefile->ReadFloat( m_Lights[i].radius );
	}
}

/*
===============
CLightController::LightChanged
===============
*/
void CLightController::LightChanged( const int entityNum )
{
	if (m_bActive == false ||
		m_Ambients.Num() == 0 ||
		m_Lights.Num() == 0)
	{
		return;
	}

	// TODO: update the ambients
	return;
}

/*
===============
CLightController::RegisterLight
===============
*/
void CLightController::RegisterLight()
{
//	m_Changes.Clear();
}

/*
===============
CLightController::UnregisterLight
===============
*/
void CLightController::UnregisterLight()
{
//	m_Changes.Clear();
}

/*
===============
CLightController::RegisterAmbient
===============
*/
void CLightController::RegisterAmbient()
{
//	m_Changes.Clear();
}

/*
===============
CLightController::UnregisterAmbient
===============
*/
void CLightController::UnregisterAmbient()
{
//	m_Changes.Clear();
}


