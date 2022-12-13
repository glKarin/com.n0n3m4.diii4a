//*****************************************************************************
//**
//** GAME_SUNCORONA.CPP
//**
//** Game code for Sun Coronas
//**
//** TODO:
//**	- Implement corona scale (or just leave this for the shader?)
//**	- Check if 4096 is good enough for distance -- issues in very large rooms?
//**	- Verify if the noFragment check is valid for checking for skyboxes
//**	- Make it function corrected with the rifle zoom view
//*****************************************************************************

// HEADER FILES ---------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// MACROS ---------------------------------------------------------------------

// TYPES ----------------------------------------------------------------------

// CLASS DECLARATIONS ---------------------------------------------------------

CLASS_DECLARATION( idEntity, hhSunCorona )
END_CLASS

// STATE DECLARATIONS ---------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES -----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ------------------------------------------------

// EXTERNAL DATA DECLARATIONS -------------------------------------------------

// PUBLIC DATA DEFINITIONS ----------------------------------------------------

// PRIVATE DATA DEFINITIONS ---------------------------------------------------

// CODE -----------------------------------------------------------------------

//=============================================================================
//
// hhSunCorona::Spawn
//
//=============================================================================

void hhSunCorona::Spawn(void) {
	corona = declManager->FindMaterial( spawnArgs.GetString( "mtr_corona" ) );
	scale = spawnArgs.GetFloat( "scale", "1" );
	sunVector = spawnArgs.GetVector( "sunVector", "0 0 -1" );
	sunVector.Normalize();
	sunDistance = spawnArgs.GetFloat( "sunDistance", "4096" );

	GetPhysics()->SetContents(0);
	Hide();
	BecomeInactive(TH_THINK);

	gameLocal.SetSunCorona( this );
}

void hhSunCorona::Save(idSaveGame *savefile) const {
	savefile->WriteMaterial( corona );
	savefile->WriteFloat( scale );
	savefile->WriteVec3( sunVector );
	savefile->WriteFloat( sunDistance );
}

void hhSunCorona::Restore( idRestoreGame *savefile ) {
	savefile->ReadMaterial( corona );
	savefile->ReadFloat( scale );
	savefile->ReadVec3( sunVector );
	savefile->ReadFloat( sunDistance );
}

//=============================================================================
//
// hhSunCorona::~hhSunCorona
//
//=============================================================================

hhSunCorona::~hhSunCorona() {
	corona = NULL;
}

//=============================================================================
//
// hhhSunCorona::Draw
//
//=============================================================================

void hhSunCorona::Draw( hhPlayer *player ) {
/* Commented out for E3 demo.  Need a better method for visibility, instead of a trace.  -cjr
	idVec3 v[3];
	float dot[3];
	player->viewAngles.ToVectors( &v[0], &v[1], &v[2] );

	for( int i = 0; i < 3; i++ ) {
		dot[i] = v[i] * sunVector;
	}

	if ( dot[0] < 0 ) {
		trace_t trace;
		idVec3 origin;
		idMat3 axis;
		player->GetViewPos( origin, axis );
		idVec3 end = origin - sunVector * sunDistance;
		gameLocal.clip.TracePoint( trace, origin, end, MASK_SOLID, player );

		if ( trace.c.material->NoFragment() ) { // Trace succeeded, or it hit a skybox
			// Draw a corona on the screen
			renderSystem->SetColor4( -dot[0], -dot[0], -dot[0], -dot[0] );
			float hFov = idMath::Sin( g_fov.GetFloat() * 0.5 );
			float vFov = idMath::Sin( g_fov.GetFloat() * 0.5 * 0.75 );
			float sx = ( ( dot[1] - hFov ) / ( -2 * hFov ) ) * 640 - 640;
			float sy = ( dot[2] + vFov ) / ( 2 * vFov ) * 480 - 480;
			renderSystem->DrawStretchPic( sx, sy, 1280, 960, 0, 0, 1, 1, corona );
		}
	}
*/
}
