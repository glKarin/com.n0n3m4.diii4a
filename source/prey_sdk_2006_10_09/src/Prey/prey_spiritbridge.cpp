//**************************************************************************
//**
//** PREY_SPIRITBRIDGE.CPP
//**
//** Game code for the spirit bridge
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// CLASS DECLARATIONS ------------------------------------------------------

CLASS_DECLARATION( idEntity, hhSpiritBridge )
	EVENT( EV_Activate,		hhSpiritBridge::Event_Activate )
END_CLASS

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// hhSpiritBridge::Spawn
//
//==========================================================================

void hhSpiritBridge::Spawn(void) {
	fl.takedamage = false;

	if ( !spawnArgs.GetInt( "start_off" ) ) {
		GetPhysics()->SetContents( CONTENTS_SPIRITBRIDGE );
	} else { // Start the bridge off
		GetPhysics()->SetContents( 0 );
		Hide();
	}

	SetShaderParm( SHADERPARM_MODE, gameLocal.random.CRandomFloat() );
}

//==========================================================================
//
// hhSpiritBridge::Event_Activate
//
//==========================================================================

void hhSpiritBridge::Event_Activate( idEntity *activator ) {
	if ( IsHidden() ) {
		Show();
		GetPhysics()->SetContents( CONTENTS_SPIRITBRIDGE );
	} else {
		Hide();
		GetPhysics()->SetContents( 0 );
	}
}
