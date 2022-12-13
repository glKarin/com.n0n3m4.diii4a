//**************************************************************************
//**
//** PREY_SPIRITSECRET.CPP
//**
//** Game code for the spirit secret entities
//** Spirit secret entities are entities that are only visible
//** and blockable in normal mode.
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// CLASS DECLARATIONS ------------------------------------------------------

CLASS_DECLARATION( idEntity, hhSpiritSecret )
	EVENT( EV_Activate,		hhSpiritSecret::Event_Activate )
END_CLASS

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// hhSpiritSecret::Spawn
//
//==========================================================================

void hhSpiritSecret::Spawn(void) {
	fl.takedamage = false;

	if ( !spawnArgs.GetInt( "start_off" ) ) {
		GetPhysics()->SetContents( CONTENTS_FORCEFIELD | CONTENTS_SHOOTABLE );
	} else { // Start the secret off
		GetPhysics()->SetContents( 0 );
		Hide();
	}

	SetShaderParm( SHADERPARM_MODE, gameLocal.random.CRandomFloat() );
}

//==========================================================================
//
// hhSpiritSecret::Event_Activate
//
//==========================================================================

void hhSpiritSecret::Event_Activate( idEntity *activator ) {
	if ( IsHidden() ) {
		Show();
		GetPhysics()->SetContents( CONTENTS_FORCEFIELD | CONTENTS_SHOOTABLE );
	} else {
		Hide();
		GetPhysics()->SetContents( 0 );
	}
}
