//**************************************************************************
//**
//** GAME_DAMAGETESTER.CPP
//**
//**
//** This object is used to calculate the damage done to it over time
//** When initially damaged, it calculates the amount of damage done for a 
//** given amount of time
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"
#include "game_damagetester.h"

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// CLASS DECLARATIONS ------------------------------------------------------

const idEventDef EV_ResetTarget( "<resetTarget>", NULL );
const idEventDef EV_CheckRemove( "<checkRemove>", NULL );

CLASS_DECLARATION( hhAnimatedEntity, hhDamageTester )
	EVENT( EV_ResetTarget,	hhDamageTester::Event_ResetTarget )
	EVENT( EV_CheckRemove,	hhDamageTester::Event_CheckRemove )
END_CLASS

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// hhDamageTester::Spawn
//
//==========================================================================

void hhDamageTester::Spawn(void) {	
	fl.takedamage = true;
	gameLocal.Printf("\"Name\", \"Damage (Melee)\", \"Damage (Close)\", \"Damage (Medium)\", \"Damage (Far)\", \
		\"Hits (Melee)\", \"Hits (Close)\", \"Hits (Medium)\", \"Hits (Far)\"\n");
	
	testTime = spawnArgs.GetFloat( "testTime", "5" );
	if( testTime < 1 ) {
		testTime = 1;
	}

	// Get the distances and reset values
	for( int i = 0; i < DD_MAX; i++ ) {
		distance[i] = spawnArgs.GetVector( va("distance%d", i), "0 0 0" );
		totalDamage[i] = 0;
		hitCount[i] = 0;
	}

	// Get the name of the weapon (useful for outputting to an Excel-friendly format)
	weaponName = spawnArgs.GetString( "weaponName", "None" );

	// Store original location and axis
	originalLocation = GetPhysics()->GetOrigin();
	originalAxis = GetPhysics()->GetAxis();

	// Initialize the target
	targetIndex = -1;
	PostEventMS( &EV_ResetTarget, 0 );

	GetPhysics()->SetContents( CONTENTS_SOLID );
}

//==========================================================================
//
// hhDamageTester::~hhDamageTester
//
//==========================================================================

hhDamageTester::~hhDamageTester() {
	int i;	

	gameLocal.Printf( "\nDAMAGE TEST RESULTS:\n" );

	// Weapon Name
	gameLocal.Printf( "\"%s\"", weaponName);

	// Damage/Sec for each distance
	for( i = 0; i < DD_MAX; i++ ) {
		gameLocal.Printf(", %.1f", totalDamage[i] / testTime );
	}

	// Hits/Sec for each distance
	for( i = 0; i < DD_MAX; i++ ) {
		gameLocal.Printf(", %.1f", hitCount[i] / testTime );
	}

	gameLocal.Printf("\n\n");
}

//==========================================================================
//
// hhDamageTester::Damage
//
//==========================================================================

void hhDamageTester::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if( targetIndex < 0 || targetIndex >= DD_MAX ) {
		return;
	}

	if( bUndamaged ) { // First time this object has been hit
		bUndamaged = false;
		PostEventSec( &EV_ResetTarget, testTime );
	}

	// Obtain damage
	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef( damageDefName, false );
	if ( !damageDef ) {
		gameLocal.Warning( "hhDamageTester::Damage: Unknown damageDef '%s'", damageDefName );
		return;
	}

	int damage = damageDef->dict.GetInt( "damage", "0" );	

	totalDamage[targetIndex] += damage * damageScale;
	hitCount[targetIndex]++;

	return;
}

//==========================================================================
//
// hhDamageTester::Event_ResetTarget
//
//==========================================================================

void hhDamageTester::Event_ResetTarget( void ) {
	// Reset the target to undamaged
	bUndamaged = true;
	health = 999999; // Ensure that it will never die

	targetIndex++;
	if( targetIndex >= DD_MAX ) {
		PostEventMS( &EV_Remove, 0 );
		return;
	}

	// Set the location of the target
	GetPhysics()->SetOrigin( originalLocation + distance[targetIndex] );
	UpdateVisuals();

	PostEventSec( &EV_CheckRemove, testTime ); // Checks to autoremove the entity after a certain time
}

//==========================================================================
//
// hhDamageTester::Event_CheckRemove
//
// Resets the target if a certain period of inactivity has occured (only if
// the weapon was unable to hit the target, for instance the wrench)
// Only happens if the target has not been hit
//==========================================================================

void hhDamageTester::Event_CheckRemove( void ) {
	if( bUndamaged && targetIndex > 0 ) {
		PostEventMS( &EV_ResetTarget, 0 );
	}
}

#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build