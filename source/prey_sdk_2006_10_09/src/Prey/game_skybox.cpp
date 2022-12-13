//**************************************************************************
//**
//** GAME_SKYBOX.CPP
//**
//** Game code for Prey-specific skyboxes
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// CLASS DECLARATIONS ------------------------------------------------------

CLASS_DECLARATION(idEntity, hhSkybox)
END_CLASS

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// hhSkybox::Spawn
//
//==========================================================================

void hhSkybox::Spawn(void) {
	// Setup the initial state for the skybox
	SetSkin( NULL );

	BecomeActive( TH_UPDATEVISUALS );

	UpdateVisuals();
}

/*
================
hhSkybox::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/
void hhSkybox::Present( void ) {
	PROFILE_SCOPE("Present", PROFMASK_NORMAL);

	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	// camera target for remote render views
	// HUMANHEAD tmj: this is the only change from idEntity::Present. Don't care if the
	// skybox is in the PVS since we only build the remoteRenderView on the first think
	// before the skybox goes inactive.
	if ( cameraTarget ) {
		renderEntity.remoteRenderView = cameraTarget->GetRenderView();
	}

	// if set to invisible, skip
	if ( !renderEntity.hModel || IsHidden() ) {
		return;
	}

	// add to refresh list
	if ( modelDefHandle == -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}
