//----------------------------------------------------------------
// CTF.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "CTF.h"

/*
===============================================================================

rvCTF_AssaultPoint

===============================================================================
*/
CLASS_DECLARATION( idEntity, rvCTF_AssaultPoint )
	EVENT( EV_PostSpawn,	rvCTF_AssaultPoint::Event_InitializeLinks )
	EVENT( EV_Touch,		rvCTF_AssaultPoint::Event_Touch )
END_CLASS

rvCTF_AssaultPoint::rvCTF_AssaultPoint() {
	trigger = NULL;
	linked = false;
	owner = AS_NEUTRAL;
}

rvCTF_AssaultPoint::~rvCTF_AssaultPoint() {
	delete trigger;
	trigger = NULL;
}

/*
================
rvCTF_AssaultPoint::Spawn
================
*/
void rvCTF_AssaultPoint::Spawn( void ) {
	PostEventMS( &EV_PostSpawn, 0 );
}

/*
================
rvCTF_AssaultPoint::InitializeLinks
================
*/
void rvCTF_AssaultPoint::Event_InitializeLinks( void ) {
	if( linked ) {
		return;
	}
	
	// pull in our targets
	toStrogg = gameLocal.FindEntity( spawnArgs.GetString( "targetStroggAP" ) );	
	toMarine = gameLocal.FindEntity( spawnArgs.GetString( "targetMarineAP" ) );

	ResetIndices();

	trigger = new idClipModel( idTraceModel( idBounds( vec3_origin ).Expand( 16.0f ) ) );
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	trigger->Link( this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
// RAVEN END
	trigger->SetContents ( CONTENTS_TRIGGER );

	GetPhysics()->SetClipModel( NULL, 1.0f );

	linked = true;
}

/*
================
rvCTF_AssaultPoint::ResetIndices
================
*/
void rvCTF_AssaultPoint::ResetIndices( void ) {
	// find the closest AP to the marine flag, which is index 0, then work towards strogg flag
	if ( !toMarine || !toMarine->IsType ( rvItemCTFFlag::GetClassType() ) ) {
		return;
	}

	idEntityPtr<rvCTF_AssaultPoint> ptr;
	ptr = this;
	gameLocal.mpGame.assaultPoints.Append( ptr );
	int assignIndices = 0;
	index = assignIndices++;
	rvCTF_AssaultPoint* ap = this;
	while( !ap->toStrogg->IsType ( rvItemCTFFlag::GetClassType() ) ) {
		if( !ap->toStrogg->IsType ( rvCTF_AssaultPoint::GetClassType() ) ) {
			gameLocal.Error( "rvCTF_AssaultPoint::ResetIndices() - non assault point targeted in assault point chain" );
		}
		ap = static_cast<rvCTF_AssaultPoint*>(ap->toStrogg.GetEntity());
		ap->index = assignIndices++;
		
		ptr = ap;
		gameLocal.mpGame.assaultPoints.Append( ptr );

		if ( !ap->linked ) {
			ap->Event_InitializeLinks();
		}

		if( !ap->toStrogg ) {
			gameLocal.Error( "rvCTF_AssaultPoint::ResetIndices() - break in assault point chain" );
		}
	}
}

/*
================
rvCTF_AssaultPoint::Event_Activate
================
*/
void rvCTF_AssaultPoint::Event_Touch( idEntity *activator, trace_t *trace ) {
	if( !activator->IsType( idPlayer::GetClassType() ) || ((gameLocal.mpGame.GetGameState())->GetMPGameState() != GAMEON && !cvarSystem->GetCVarBool( "g_testCTF" )) ) {
		return;
	}

	idPlayer* player = static_cast<idPlayer*>(activator);
	int oldOwner = owner;

	if ( owner == player->team )
		return;

	int enemyPowerup = -1;
	int friendlyPowerup = -1;
	
	if ( owner == TEAM_MARINE ) {
		friendlyPowerup = POWERUP_CTF_STROGGFLAG;
		enemyPowerup = POWERUP_CTF_MARINEFLAG;
	} else if ( owner == TEAM_STROGG ) {
		friendlyPowerup = POWERUP_CTF_MARINEFLAG;
		enemyPowerup = POWERUP_CTF_STROGGFLAG;
	} else {
		// neutral assault point
		if ( player->team == TEAM_MARINE ) {
			friendlyPowerup = POWERUP_CTF_MARINEFLAG;
			enemyPowerup = POWERUP_CTF_STROGGFLAG;
		} else {
			friendlyPowerup = POWERUP_CTF_STROGGFLAG;
			enemyPowerup = POWERUP_CTF_MARINEFLAG;
		}
	}

	if ( !player->PowerUpActive ( enemyPowerup ) ) {
		return;
	}


	switch( player->team ) {
		case TEAM_MARINE: {
			if( !toMarine || owner == TEAM_MARINE ) {
				break;
			}

			if( toMarine->IsType( rvItemCTFFlag::GetClassType() ) ) {
				owner = TEAM_MARINE;
				gameLocal.Printf("Assault point %s captured by marines!\n", name.c_str());
			} else if( toMarine->IsType( rvCTF_AssaultPoint::GetClassType() ) ) {
				if( static_cast<rvCTF_AssaultPoint*>(toMarine.GetEntity())->owner == TEAM_MARINE ) {
					owner = TEAM_MARINE;
					gameLocal.Printf("Assault point %s captured by marines!\n", name.c_str());
				}
			}
			break;
		}
		case TEAM_STROGG: {
			if( !toStrogg || owner == TEAM_STROGG ) {
				break;
			}

			if( toStrogg->IsType( rvItemCTFFlag::GetClassType() ) ) {
				owner = TEAM_STROGG;
				gameLocal.Printf("Assault point %s captured by strogg!\n", name.c_str());
			} else if( toStrogg->IsType( rvCTF_AssaultPoint::GetClassType() ) ) {
				if( static_cast<rvCTF_AssaultPoint*>(toStrogg.GetEntity())->owner == TEAM_STROGG ) {
					owner = TEAM_STROGG;
					gameLocal.Printf("Assault point %s captured by strogg!\n", name.c_str());
				}
			}			
			break;
		}
	}
	

	if( oldOwner != owner ) {
		// we switched hands, reset forward spawns
		gameLocal.ClearForwardSpawns();
		
		if( oldOwner == TEAM_MARINE ) {
			if( static_cast<rvCTF_AssaultPoint*>(toMarine.GetEntity())->IsType( rvCTF_AssaultPoint::Type ) ) {
				static_cast<rvCTF_AssaultPoint*>(toMarine.GetEntity())->ResetSpawns( oldOwner );
			}
		} else if( oldOwner == TEAM_STROGG ) {
			if( static_cast<rvCTF_AssaultPoint*>(toStrogg.GetEntity())->IsType( rvCTF_AssaultPoint::Type ) ) {
				static_cast<rvCTF_AssaultPoint*>(toStrogg.GetEntity())->ResetSpawns( oldOwner );
			}
		}
		
		rvItemCTFFlag::ResetFlag ( enemyPowerup );

		gameLocal.mpGame.AddPlayerTeamScore( player, 2 );

		((rvCTFGameState*)gameLocal.mpGame.GetGameState())->SetAPOwner( index, owner );

		ActivateTargets( this );

		SetOwnerColor ();
	}
}

/*
================
rvCTF_AssaultPoint::ResetSpawns
================
*/
void rvCTF_AssaultPoint::ResetSpawns( int team ) {
	// check to see if team can spawn at me, if not pass down
	if( owner == team ) {
		ActivateTargets( this );
	} else {
		if( team == TEAM_MARINE ) {
			if( static_cast<rvCTF_AssaultPoint*>(toMarine.GetEntity())->IsType( rvCTF_AssaultPoint::Type ) ) {
				static_cast<rvCTF_AssaultPoint*>(toMarine.GetEntity())->ResetSpawns( team );
			}
		} else if( team == TEAM_STROGG ) {
			if( static_cast<rvCTF_AssaultPoint*>(toStrogg.GetEntity())->IsType( rvCTF_AssaultPoint::Type ) ) {
				static_cast<rvCTF_AssaultPoint*>(toStrogg.GetEntity())->ResetSpawns( team );
			}
		}	
	}
}

void rvCTF_AssaultPoint::SetOwner ( int newOwner ) {
	if ( !gameLocal.isClient ) {
		// server should set owner via EVENT_ACTIVATE, not here
		return;
	}
	
	owner = newOwner;
	SetOwnerColor ();
}

void rvCTF_AssaultPoint::Reset ( void ) {
	owner = AS_NEUTRAL;
	SetOwnerColor();
}

/*
================
rvCTF_AssaultPoint::ChangeColor
================
*/
void rvCTF_AssaultPoint::SetOwnerColor ( void ) {
	const idDeclSkin* skin = NULL;
	if( owner == TEAM_MARINE ) {
		skin = declManager->FindSkin( "skins/ddynerman/green_glow", false );
	} else if ( owner == TEAM_STROGG ) { 
		skin = declManager->FindSkin( "skins/ddynerman/orange_glow", false );
	} else {
		skin = declManager->FindSkin( "skins/ddynerman/white", false );
	}

	if ( skin ) {
		SetSkin( skin );	
	}
}

/*
===============================================================================

rvCTFAssaultPlayerStart

===============================================================================
*/

CLASS_DECLARATION( idPlayerStart, rvCTFAssaultPlayerStart )
	EVENT( EV_Activate,	rvCTFAssaultPlayerStart::Event_Activate )
END_CLASS

/*
================
rvCTFAssaultPlayerStart::Spawn
================
*/
void rvCTFAssaultPlayerStart::Spawn(void) {
	if( !idStr::Icmp( spawnArgs.GetString( "team" ), "marine" ) ) {
		team = TEAM_MARINE;
	} else if( !idStr::Icmp( spawnArgs.GetString( "team" ), "strogg" ) ) {
		team = TEAM_STROGG;
	} else {
		gameLocal.Error("rvCTFAssaultPlayerStart::Spawn() - unknown team\n");
		team = -1;
	}
}

void rvCTFAssaultPlayerStart::Event_Activate( idEntity *activator ) {
	if ( !activator->IsType( rvCTF_AssaultPoint::GetClassType() ) ) {
		gameLocal.Warning( "rvCTFAssaultPlayerStart::Event_Activate() - was activated by something other than an assault point\n" );
		return;
	}
	
	rvCTF_AssaultPoint* ap = static_cast<rvCTF_AssaultPoint*>(activator);

	if( ap->GetOwner() == team ) {
		gameLocal.UpdateForwardSpawns( this, team );
	}
}
