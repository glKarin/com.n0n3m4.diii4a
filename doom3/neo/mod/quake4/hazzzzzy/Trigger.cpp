
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "ai/AI_Manager.h"

/*
===============================================================================

  idTrigger
	
===============================================================================
*/

const idEventDef EV_Enable( "enable", NULL );
const idEventDef EV_Disable( "disable", NULL );

CLASS_DECLARATION( idEntity, idTrigger )
	EVENT( EV_Enable,	idTrigger::Event_Enable )
	EVENT( EV_Disable,	idTrigger::Event_Disable )
END_CLASS

/*
================
idTrigger::DrawDebugInfo
================
*/
void idTrigger::DrawDebugInfo( void ) {
	idMat3		axis = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();
	idVec3		up = axis[ 2 ] * 5.0f;
	idBounds	viewTextBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idBounds	viewBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idBounds	box( idVec3( -4.0f, -4.0f, -4.0f ), idVec3( 4.0f, 4.0f, 4.0f ) );
	idEntity	*ent;
	idEntity	*target;
	int			i;
	bool		show;
	const function_t *func;

	viewTextBounds.ExpandSelf( 128.0f );
	viewBounds.ExpandSelf( 512.0f );
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->GetPhysics()->GetContents() & ( CONTENTS_TRIGGER | CONTENTS_FLASHLIGHT_TRIGGER ) ) {
			show = viewBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() );
			if ( !show ) {
				for( i = 0; i < ent->targets.Num(); i++ ) {
					target = ent->targets[ i ].GetEntity();
					if ( target && viewBounds.IntersectsBounds( target->GetPhysics()->GetAbsBounds() ) ) {
						show = true;
						break;
					}
				}
			}

			if ( !show ) {
				continue;
			}

			gameRenderWorld->DebugBounds( colorOrange, ent->GetPhysics()->GetAbsBounds() );
			if ( viewTextBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) ) {
				gameRenderWorld->DrawText( ent->name.c_str(), ent->GetPhysics()->GetAbsBounds().GetCenter(), 0.1f, colorWhite, axis, 1 );
				gameRenderWorld->DrawText( ent->GetEntityDefName(), ent->GetPhysics()->GetAbsBounds().GetCenter() + up, 0.1f, colorWhite, axis, 1 );
				if ( ent->IsType( idTrigger::Type ) ) {
					func = static_cast<idTrigger *>( ent )->GetScriptFunction();
				} else {
					func = NULL;
				}

				if ( func ) {
					gameRenderWorld->DrawText( va( "call script '%s'", func->Name() ), ent->GetPhysics()->GetAbsBounds().GetCenter() - up, 0.1f, colorWhite, axis, 1 );
				}
			}

			for( i = 0; i < ent->targets.Num(); i++ ) {
				target = ent->targets[ i ].GetEntity();
				if ( target ) {
					gameRenderWorld->DebugArrow( colorYellow, ent->GetPhysics()->GetAbsBounds().GetCenter(), target->GetPhysics()->GetOrigin(), 10, 0 );
					gameRenderWorld->DebugBounds( colorGreen, box, target->GetPhysics()->GetOrigin() );
					if ( viewTextBounds.IntersectsBounds( target->GetPhysics()->GetAbsBounds() ) ) {
						gameRenderWorld->DrawText( target->name.c_str(), target->GetPhysics()->GetAbsBounds().GetCenter(), 0.1f, colorWhite, axis, 1 );
					}
				}
			}
		}
	}
}

/*
================
idTrigger::Enable
================
*/
void idTrigger::Enable( void ) {
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	GetPhysics()->EnableClip();
}

/*
================
idTrigger::Disable
================
*/
void idTrigger::Disable( void ) {
	// we may be relinked if we're bound to another object, so clear the contents as well
	GetPhysics()->SetContents( 0 );
	GetPhysics()->DisableClip();
}

/*
================
idTrigger::CallScript
================
*/
void idTrigger::CallScript( idEntity* scriptEntity ) {
// RAVEN BEGIN
// abahr
	for( int ix = scriptFunctions.Num() - 1; ix >= 0; --ix ) {
		scriptFunctions[ix].InsertEntity( scriptEntity, 0 );//We could pass both the activator and self if wanted
		scriptFunctions[ix].CallFunc( &spawnArgs );
		scriptFunctions[ix].RemoveIndex( 0 );
	}
// RAVEN END

}

/*
================
idTrigger::GetScriptFunction
================
*/
const function_t *idTrigger::GetScriptFunction( void ) const {
// RAVEN BEGIN
// abahr: 
	return (scriptFunctions.Num()) ? scriptFunctions[0].GetFunc() : NULL;
// RAVEN END
}

/*
================
idTrigger::Save
================
*/
void idTrigger::Save( idSaveGame *savefile ) const {
// RAVEN BEGIN
// abahr
	savefile->WriteInt( scriptFunctions.Num() );
	for( int ix = scriptFunctions.Num() - 1; ix >= 0; --ix ) {
		scriptFunctions[ix].Save( savefile );
	}
// RAVEN END
}

/*
================
idTrigger::Restore
================
*/
void idTrigger::Restore( idRestoreGame *savefile ) {
// RAVEN BEGIN
// abahr
	int numScripts = 0;
	savefile->ReadInt( numScripts );
	scriptFunctions.SetNum( numScripts );
	for( int ix = scriptFunctions.Num() - 1; ix >= 0; --ix ) {
		scriptFunctions[ix].Restore( savefile );
	}
// RAVEN END
}

/*
================
idTrigger::Event_Enable
================
*/
void idTrigger::Event_Enable( void ) {
	Enable();
}

/*
================
idTrigger::Event_Disable
================
*/
void idTrigger::Event_Disable( void ) {
	Disable();
}

/*
================
idTrigger::idTrigger
================
*/
idTrigger::idTrigger() {
// RAVEN BEGIN
// abahr: scriptFunction init's itself
	//scriptFunction = NULL;
// RAVEN END
}

/*
================
idTrigger::Spawn
================
*/
void idTrigger::Spawn( void ) {
	GetPhysics()->SetContents( CONTENTS_TRIGGER );

// RAVEN BEGIN
// abahr:
	scriptFunctions.SetGranularity( 1 );
	for( const idKeyValue* kv = spawnArgs.MatchPrefix("call"); kv; kv = spawnArgs.MatchPrefix("call", kv) ) {
		if( !kv->GetValue() ) {
			continue;
		}

		rvScriptFuncUtility& utility = scriptFunctions.Alloc();
		if( !utility.Init(kv->GetValue()) ) {
			gameLocal.Warning( "Trigger '%s' at (%s) trying to call an unknown function.", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
		}
	}
// RAVEN END
}


/*
===============================================================================

  idTrigger_Multi
	
===============================================================================
*/

// RAVEN BEGIN
// abahr: changed to 'E' to allow NULL entities
const idEventDef EV_TriggerAction( "<triggerAction>", "E" );
// RAVEN END

CLASS_DECLARATION( idTrigger, idTrigger_Multi )
	EVENT( EV_Touch,			idTrigger_Multi::Event_Touch )
	EVENT( EV_Activate,			idTrigger_Multi::Event_Trigger )
	EVENT( EV_TriggerAction,	idTrigger_Multi::Event_TriggerAction )

// RAVEN BEGIN
// kfuller: respond to earthquakes
	EVENT( EV_Earthquake,		idTrigger_Multi::Event_EarthQuake )
// RAVEN END
END_CLASS


/*
================
idTrigger_Multi::idTrigger_Multi
================
*/
idTrigger_Multi::idTrigger_Multi( void ) {
	wait = 0.0f;
	random = 0.0f;
	delay = 0.0f;
	random_delay = 0.0f;
	nextTriggerTime = 0;
	removeItem = 0;
	touchClient = false;
	touchOther = false;
	touchVehicle = false;
	triggerFirst = false;
	triggerWithSelf = false;
	buyZoneTrigger = 0;
	controlZoneTrigger = 0;
	prevZoneController = TEAM_NONE;
}

/*
================
idTrigger_Multi::Save
================
*/
void idTrigger_Multi::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( wait );
	savefile->WriteFloat( random );
	savefile->WriteFloat( delay );
	savefile->WriteFloat( random_delay );
	savefile->WriteInt( nextTriggerTime );
	savefile->WriteString( requires );
	savefile->WriteInt( removeItem );
	savefile->WriteBool( touchClient );
	savefile->WriteBool( touchOther );
	savefile->WriteBool( touchVehicle );
	savefile->WriteBool( triggerFirst );
	savefile->WriteBool( triggerWithSelf );
}

/*
================
idTrigger_Multi::Restore
================
*/
void idTrigger_Multi::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( wait );
	savefile->ReadFloat( random );
	savefile->ReadFloat( delay );
	savefile->ReadFloat( random_delay );
	savefile->ReadInt( nextTriggerTime );
	savefile->ReadString( requires );
	savefile->ReadInt( removeItem );
	savefile->ReadBool( touchClient );
	savefile->ReadBool( touchOther );
	savefile->ReadBool( touchVehicle );
	savefile->ReadBool( triggerFirst );
	savefile->ReadBool( triggerWithSelf );
}

/*
================
idTrigger_Multi::Spawn

"wait" : Seconds between triggerings, 0.5 default, -1 = one time only.
"call" : Script function to call when triggered
"random"	wait variance, default is 0
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
================
*/
void idTrigger_Multi::Spawn( void ) {
	spawnArgs.GetFloat( "wait", "0.5", wait );
	spawnArgs.GetFloat( "random", "0", random );
	spawnArgs.GetFloat( "delay", "0", delay );
	spawnArgs.GetFloat( "random_delay", "0", random_delay );
	
	if ( random && ( random >= wait ) && ( wait >= 0 ) ) {
		random = wait - 1;
		gameLocal.Warning( "idTrigger_Multi '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	if ( random_delay && ( random_delay >= delay ) && ( delay >= 0 ) ) {
		random_delay = delay - 1;
		gameLocal.Warning( "idTrigger_Multi '%s' at (%s) has random_delay >= delay", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	spawnArgs.GetString( "requires", "", requires );
	spawnArgs.GetInt( "removeItem", "0", removeItem );
	spawnArgs.GetBool( "triggerFirst", "0", triggerFirst );
	spawnArgs.GetBool( "triggerWithSelf", "0", triggerWithSelf );
	spawnArgs.GetInt( "buyZone", "0", buyZoneTrigger);
	spawnArgs.GetInt( "controlZone", "0", controlZoneTrigger);

	if ( buyZoneTrigger == -1 )
		gameLocal.Warning( "trigger_buyzone '%s' at (%s) has no buyZone key set!", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );

	if ( controlZoneTrigger == -1 )
		gameLocal.Warning( "trigger_controlzone '%s' at (%s) has no controlZone key set!", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );


	if ( spawnArgs.GetBool( "onlyVehicle" ) ) {
		touchVehicle = true;
	} else if ( spawnArgs.GetBool( "anyTouch" ) ) {
		touchClient = true;
		touchOther = true;
	} else if ( spawnArgs.GetBool( "noTouch" ) ) {
		touchClient = false;
		touchOther = false;
	} else if ( spawnArgs.GetBool( "noClient" ) ) {
		touchClient = false;
		touchOther = true;
	} else {
		touchClient = true;
		touchOther = false;
	}

	nextTriggerTime = 0;

	if ( spawnArgs.GetBool( "flashlight_trigger" ) ) {
		GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
	} else if ( spawnArgs.GetBool( "projectile_trigger" ) ) {
		GetPhysics()->SetContents( CONTENTS_TRIGGER | CONTENTS_PROJECTILE );
	} else {
		GetPhysics()->SetContents( CONTENTS_TRIGGER );
	}

	BecomeActive( TH_THINK );
}

/*
================
idTrigger_Multi::CheckFacing
================
*/
bool idTrigger_Multi::CheckFacing( idEntity *activator ) {
	if ( spawnArgs.GetBool( "facing" ) ) {
		if ( !activator->IsType( idPlayer::GetClassType() ) ) {
			return true;
		}
		idPlayer *player = static_cast< idPlayer* >( activator );

		// Unfortunately, the angle key rotates the trigger entity also.  So I've added
		//	an angleFacing key which is used instead when present, otherwise the code defaults
		//	to the behaviour present prior to this change
		idVec3 tFacing = GetPhysics()->GetAxis()[0];
		if ( spawnArgs.FindKey( "angleFacing" )) {
			idAngles angs(0,spawnArgs.GetFloat( "angleFacing", "0" ),0);
			tFacing = angs.ToForward();
		}
		float dot = player->viewAngles.ToForward() * tFacing;

		float angle = RAD2DEG( idMath::ACos( dot ) );
		if ( angle  > spawnArgs.GetFloat( "angleLimit", "30" ) ) {
			return false;
		}
	}
	return true;
}


/*
================
idTrigger_Multi::TriggerAction
================
*/
void idTrigger_Multi::TriggerAction( idEntity *activator ) {
// RAVEN BEGIN
// jdischler: added for Aweldon.  The trigger, when activated, will call the listed func with all attached targets, then return.
	if ( spawnArgs.GetBool( "_callWithTargets", "0" )) 
	{
		idEntity *ent;
		for( int i = 0; i < targets.Num(); i++ ) 
		{
			ent = targets[ i ].GetEntity();
			if ( !ent ) 
			{
				continue;
			}
			CallScript( ent );
		}
		return;
	}
// RAVEN END
	ActivateTargets( triggerWithSelf ? this : activator );
	CallScript( triggerWithSelf ? this : activator );

	if ( wait >= 0 ) {
		nextTriggerTime = gameLocal.time + SEC2MS( wait + random * gameLocal.random.CRandomFloat() );
	} else {
		// we can't just remove (this) here, because this is a touch function
		// called while looping through area links...
		nextTriggerTime = gameLocal.time + 1;
		PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
idTrigger_Multi::Event_TriggerAction
================
*/
void idTrigger_Multi::Event_TriggerAction( idEntity *activator ) {
	TriggerAction( activator );
}

/*
================
idTrigger_Multi::Event_Trigger

the trigger was just activated
activated should be the entity that originated the activation sequence (ie. the original target)
activator should be set to the activator so it can be held through a delay
so wait for the delay time before firing
================
*/
void idTrigger_Multi::Event_Trigger( idEntity *activator ) {
// RAVEN BEGIN
// bdube: moved trigger first 
	if ( triggerFirst ) {
		triggerFirst = false;
		return;
	}

	if ( nextTriggerTime > gameLocal.time ) {
		// can't retrigger until the wait is over
		return;
	}

	// see if this trigger requires an item
	if ( !gameLocal.RequirementMet( activator, requires, removeItem ) ) {
		return;
	}

	if ( !CheckFacing( activator ) ) {
		return;
	}
// RAVEN END

	// don't allow it to trigger twice in a single frame
	nextTriggerTime = gameLocal.time + 1;

	if ( delay > 0 ) {
		// don't allow it to trigger again until our delay has passed
		nextTriggerTime += SEC2MS( delay + random_delay * gameLocal.random.CRandomFloat() );
		PostEventSec( &EV_TriggerAction, delay, activator );
	} else {
		TriggerAction( activator );
	}
}


void idTrigger_Multi::HandleControlZoneTrigger()
{
	// This only does something in multiplayer
	if ( !gameLocal.isMultiplayer )
		return;

	const int TEAM_DEADLOCK = 2;

	int pCount = 0;
	int count = 0, controllingTeam = TEAM_NONE;
	count = playersInTrigger.Num();

	for ( int i = 0; i<count; i++ )
	{
		// No token? Ignore em!
		if ( spawnArgs.GetBool("requiresDeadZonePowerup", "1") && !playersInTrigger[i]->PowerUpActive( POWERUP_DEADZONE ) )
			continue;

		if ( spawnArgs.GetBool("requiresDeadZonePowerup", "1") )
		{
			pCount++;
		}

		int team = playersInTrigger[i]->team;

		if ( i == 0 )
			controllingTeam = playersInTrigger[i]->team;

		// Assign the controlling team based on the first player
		// for zones that accept both.
		if ( team != controllingTeam )
		{
			controllingTeam = TEAM_DEADLOCK;
			pCount = 0;
		}
	}

	if ( controllingTeam != controlZoneTrigger-1 && controlZoneTrigger != 3 )
	{
		controllingTeam = TEAM_NONE;
		pCount = 0;
	}

	int situation = DZ_NONE;
	if ( controllingTeam != prevZoneController )
	{
		if ( controllingTeam == TEAM_MARINE && prevZoneController == TEAM_NONE )
			situation = DZ_MARINES_TAKEN;
		else if ( controllingTeam == TEAM_STROGG && prevZoneController == TEAM_NONE )
			situation = DZ_STROGG_TAKEN;
		else if ( controllingTeam == TEAM_NONE && prevZoneController == TEAM_MARINE )
			situation = DZ_MARINES_LOST;
		else if ( controllingTeam == TEAM_NONE && prevZoneController == TEAM_STROGG )
			situation = DZ_STROGG_LOST;
		else if ( controllingTeam == TEAM_MARINE && prevZoneController == TEAM_STROGG )
			situation = DZ_STROGG_TO_MARINE;
		else if ( controllingTeam == TEAM_STROGG && prevZoneController == TEAM_MARINE )
			situation = DZ_MARINE_TO_STROGG;

		// DEADLOCK
		else if ( controllingTeam == TEAM_DEADLOCK && prevZoneController == TEAM_MARINE )
			situation = DZ_MARINE_DEADLOCK;
		else if ( controllingTeam == TEAM_DEADLOCK && prevZoneController == TEAM_STROGG )
			situation = DZ_STROGG_DEADLOCK;
		else if ( controllingTeam == TEAM_DEADLOCK && prevZoneController == TEAM_NONE )
			situation = DZ_MARINE_DEADLOCK; // Unlikely case, just use this.
		else if ( controllingTeam == TEAM_MARINE && prevZoneController == TEAM_DEADLOCK )
			situation = DZ_MARINE_REGAIN;
		else if ( controllingTeam == TEAM_STROGG && prevZoneController == TEAM_DEADLOCK )
			situation = DZ_STROGG_REGAIN;
		else if ( controllingTeam == TEAM_NONE && prevZoneController == TEAM_DEADLOCK )
			situation = DZ_MARINES_LOST; // Unlikely case, just use this.
	}

	/// Report individual credits
	for( int i = 0; i < count; i++ )
	{
		idPlayer* player = playersInTrigger[i];

		// No token? Ignore em!
		if ( spawnArgs.GetBool("requiresDeadZonePowerup", "1") && !player->PowerUpActive( POWERUP_DEADZONE ) )
			continue;

		int team = player->team;
		if( team == controllingTeam )
		{
			gameLocal.mpGame.ReportZoneControllingPlayer( player );
		}
	}

	/// Report zone control to multiplayer game manager
	gameLocal.mpGame.ReportZoneController(controllingTeam, pCount, situation, this);

	playersInTrigger.Clear();
	prevZoneController = controllingTeam;
}


/*
================
idTrigger_Multi::Think
================
*/
void idTrigger_Multi::Think()
{
	// Control zone handling
	if ( controlZoneTrigger > 0 ) 
		HandleControlZoneTrigger();
}

/*
================
idTrigger_Multi::Event_Touch
================
*/
void idTrigger_Multi::Event_Touch( idEntity *other, trace_t *trace ) {
	if( triggerFirst ) {
		return;
	}

// RAVEN BEGIN
// jdischler: vehicle only trigger
	if ( touchVehicle ) {
		if ( !other->IsType(rvVehicle::GetClassType()) ) {
			return;
		}
	} else {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		bool player = other->IsType( idPlayer::GetClassType() );
// RAVEN END
		if ( player ) {
			if ( !touchClient ) {
				return;
			}
			if ( static_cast< idPlayer * >( other )->spectating ) {
				return;
			}

		    // Buy zone handling
		    if ( buyZoneTrigger /*&& gameLocal.mpGame.mpGameState.gameState.currentState != 1*/ ) {
			    idPlayer *p = static_cast< idPlayer * >( other );
			    if ( buyZoneTrigger-1 == p->team || buyZoneTrigger == 3)
			    {
				    p->inBuyZone = true;
				    p->inBuyZonePrev = true;
			    }
		    }
    
		    // Control zone handling
		    if ( controlZoneTrigger > 0 ) {
			    idPlayer *p = static_cast< idPlayer * >( other );
				if ( p->PowerUpActive(POWERUP_DEADZONE) || !spawnArgs.GetBool("requiresDeadZonePowerup", "1") )
					playersInTrigger.Append(p);
		    }

		} else if ( !touchOther ) {
			return;
		}
	}

	if ( nextTriggerTime > gameLocal.time ) {
		// can't retrigger until the wait is over
		return;
	}

	// see if this trigger requires an item
	if ( !gameLocal.RequirementMet( other, requires, removeItem ) ) {
		return;
	}

	if ( !CheckFacing( other ) ) {
		return;
	}

	if ( spawnArgs.GetBool( "toggleTriggerFirst" ) ) {
		triggerFirst = true;
	}

// RAVEN BEGIN
// rjohnson: added block
	if ( developer.GetBool() && *spawnArgs.GetString ( "message" ) ) {
		gameLocal.DPrintf ( "Trigger: %s\n", spawnArgs.GetString ( "message" ) );
	}
// RAVEN END

	nextTriggerTime = gameLocal.time + 1;
	if ( delay > 0 ) {
		// don't allow it to trigger again until our delay has passed
		nextTriggerTime += SEC2MS( delay + random_delay * gameLocal.random.CRandomFloat() );
		PostEventSec( &EV_TriggerAction, delay, other );
	} else {
		TriggerAction( other );
	}
}

// RAVEN BEGIN
// kfuller:
void idTrigger_Multi::Event_EarthQuake(float requiresLOS)
{
	// does this entity even care about earthquakes?
	float	quakeChance = 0;

	if (!spawnArgs.GetFloat("quakeChance", "0", quakeChance))
	{
		return;
	}
	if (rvRandom::flrand(0, 1.0f) > quakeChance)
	{
		// failed its activation roll
		return;
	}
	if (requiresLOS)
	{
		// if the player doesn't have line of sight to this fx, don't do anything
		trace_t		trace;
		idPlayer	*player = gameLocal.GetLocalPlayer();
		idVec3		viewOrigin;
		idMat3		viewAxis;

		player->GetViewPos(viewOrigin, viewAxis);
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		gameLocal.TracePoint( this, trace, viewOrigin, GetPhysics()->GetOrigin(), MASK_OPAQUE, player );
// RAVEN END
		if (trace.fraction < 1.0f)
		{
			// something blocked LOS
			return;
		}
	}
	// activate this effect now
	TriggerAction(gameLocal.entities[ENTITYNUM_WORLD]);
}

// RAVEN END

/*
===============================================================================

  idTrigger_EntityName
	
===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_EntityName )
	EVENT( EV_Touch,			idTrigger_EntityName::Event_Touch )
	EVENT( EV_Activate,			idTrigger_EntityName::Event_Trigger )
	EVENT( EV_TriggerAction,	idTrigger_EntityName::Event_TriggerAction )
END_CLASS

/*
================
idTrigger_EntityName::idTrigger_EntityName
================
*/
idTrigger_EntityName::idTrigger_EntityName( void ) {
	wait = 0.0f;
	random = 0.0f;
	delay = 0.0f;
	random_delay = 0.0f;
	nextTriggerTime = 0;
	triggerFirst = false;
}

/*
================
idTrigger_EntityName::Save
================
*/
void idTrigger_EntityName::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( wait );
	savefile->WriteFloat( random );
	savefile->WriteFloat( delay );
	savefile->WriteFloat( random_delay );
	savefile->WriteInt( nextTriggerTime );
	savefile->WriteBool( triggerFirst );
	savefile->WriteString( entityName );	
}

/*
================
idTrigger_EntityName::Restore
================
*/
void idTrigger_EntityName::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( wait );
	savefile->ReadFloat( random );
	savefile->ReadFloat( delay );
	savefile->ReadFloat( random_delay );
	savefile->ReadInt( nextTriggerTime );
	savefile->ReadBool( triggerFirst );
	savefile->ReadString( entityName );
}

/*
================
idTrigger_EntityName::Spawn
================
*/
void idTrigger_EntityName::Spawn( void ) {
	spawnArgs.GetFloat( "wait", "0.5", wait );
	spawnArgs.GetFloat( "random", "0", random );
	spawnArgs.GetFloat( "delay", "0", delay );
	spawnArgs.GetFloat( "random_delay", "0", random_delay );
	
	if ( random && ( random >= wait ) && ( wait >= 0 ) ) {
		random = wait - 1;
		gameLocal.Warning( "idTrigger_EntityName '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	if ( random_delay && ( random_delay >= delay ) && ( delay >= 0 ) ) {
		random_delay = delay - 1;
		gameLocal.Warning( "idTrigger_EntityName '%s' at (%s) has random_delay >= delay", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	spawnArgs.GetBool( "triggerFirst", "0", triggerFirst );

	entityName = spawnArgs.GetString( "entityname" );
	if ( !entityName.Length() ) {
		gameLocal.Error( "idTrigger_EntityName '%s' at (%s) doesn't have 'entityname' key specified", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	nextTriggerTime = 0;

	if ( !spawnArgs.GetBool( "noTouch" ) ) {
		GetPhysics()->SetContents( CONTENTS_TRIGGER );
	}
}

/*
================
idTrigger_EntityName::TriggerAction
================
*/
void idTrigger_EntityName::TriggerAction( idEntity *activator ) {
// RAVEN BEGIN
// abahr: want same functionality as trigger_multi.  Need to move this code into these two function calls
	idEntity* scriptEntity = spawnArgs.GetBool("triggerWithSelf") ? this : activator;
	ActivateTargets( scriptEntity );
	CallScript( scriptEntity );
// RAVEN END

	if ( wait >= 0 ) {
		nextTriggerTime = gameLocal.time + SEC2MS( wait + random * gameLocal.random.CRandomFloat() );
	} else {
		// we can't just remove (this) here, because this is a touch function
		// called while looping through area links...
		nextTriggerTime = gameLocal.time + 1;
		PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
idTrigger_EntityName::Event_TriggerAction
================
*/
void idTrigger_EntityName::Event_TriggerAction( idEntity *activator ) {
	TriggerAction( activator );
}

/*
================
idTrigger_EntityName::Event_Trigger

the trigger was just activated
activated should be the entity that originated the activation sequence (ie. the original target)
activator should be set to the activator so it can be held through a delay
so wait for the delay time before firing
================
*/
void idTrigger_EntityName::Event_Trigger( idEntity *activator ) {
	if ( nextTriggerTime > gameLocal.time ) {
		// can't retrigger until the wait is over
		return;
	}

// RAVEN BEGIN
// abahr: so we can exclude an entity by name
	if( !activator ) {
		return;
	}

	if( spawnArgs.GetBool("excludeEntityName") && activator->name == entityName ) {
		return;
	}

	if( !spawnArgs.GetBool("excludeEntityName") && activator->name != entityName ) {
		return;
	}
// RAVEN END

	if ( triggerFirst ) {
		triggerFirst = false;
		return;
	}

	// don't allow it to trigger twice in a single frame
	nextTriggerTime = gameLocal.time + 1;

	if ( delay > 0 ) {
		// don't allow it to trigger again until our delay has passed
		nextTriggerTime += SEC2MS( delay + random_delay * gameLocal.random.CRandomFloat() );
		PostEventSec( &EV_TriggerAction, delay, activator );
	} else {
		TriggerAction( activator );
	}
}

/*
================
idTrigger_EntityName::Event_Touch
================
*/
void idTrigger_EntityName::Event_Touch( idEntity *other, trace_t *trace ) {
	if( triggerFirst ) {
		return;
	}

	if ( nextTriggerTime > gameLocal.time ) {
		// can't retrigger until the wait is over
		return;
	}

// RAVEN BEGIN
// abahr: so we can exclude an entity by name
	if( !other ) {
		return;
	}

	if( spawnArgs.GetBool("excludeEntityName") && other->name == entityName ) {
		return;
	}

	if( !spawnArgs.GetBool("excludeEntityName") && other->name != entityName ) {
		return;
	}
// RAVEN END

	nextTriggerTime = gameLocal.time + 1;
	if ( delay > 0 ) {
		// don't allow it to trigger again until our delay has passed
		nextTriggerTime += SEC2MS( delay + random_delay * gameLocal.random.CRandomFloat() );
		PostEventSec( &EV_TriggerAction, delay, other );
	} else {
		TriggerAction( other );
	}
}

/*
===============================================================================

  idTrigger_Timer
	
===============================================================================
*/

const idEventDef EV_Timer( "<timer>", NULL );

CLASS_DECLARATION( idTrigger, idTrigger_Timer )
	EVENT( EV_Timer,		idTrigger_Timer::Event_Timer )
	EVENT( EV_Activate,		idTrigger_Timer::Event_Use )
END_CLASS

/*
================
idTrigger_Timer::idTrigger_Timer
================
*/
idTrigger_Timer::idTrigger_Timer( void ) {
	random = 0.0f;
	wait = 0.0f;
	on = false;
	delay = 0.0f;
}

/*
================
idTrigger_Timer::Save
================
*/
void idTrigger_Timer::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( random );
	savefile->WriteFloat( wait );
	savefile->WriteBool( on );
	savefile->WriteFloat( delay );
	savefile->WriteString( onName );
	savefile->WriteString( offName );
}

/*
================
idTrigger_Timer::Restore
================
*/
void idTrigger_Timer::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( random );
	savefile->ReadFloat( wait );
	savefile->ReadBool( on );
	savefile->ReadFloat( delay );
	savefile->ReadString( onName );
	savefile->ReadString( offName );
}

/*
================
idTrigger_Timer::Spawn

Repeatedly fires its targets.
Can be turned on or off by using.
================
*/
void idTrigger_Timer::Spawn( void ) {
	spawnArgs.GetFloat( "random", "1", random );
	spawnArgs.GetFloat( "wait", "1", wait );
	spawnArgs.GetBool( "start_on", "0", on );
	spawnArgs.GetFloat( "delay", "0", delay );
	onName = spawnArgs.GetString( "onName" );
	offName = spawnArgs.GetString( "offName" );

	if ( random >= wait && wait >= 0 ) {
		random = wait - 0.001;
		gameLocal.Warning( "idTrigger_Timer '%s' at (%s) has random >= wait", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	if ( on ) {
		PostEventSec( &EV_Timer, delay );
	}
}

/*
================
idTrigger_Timer::Enable
================
*/
void idTrigger_Timer::Enable( void ) {
	// if off, turn it on
	if ( !on ) {
		on = true;
		PostEventSec( &EV_Timer, delay );
	}
}

/*
================
idTrigger_Timer::Disable
================
*/
void idTrigger_Timer::Disable( void ) {
	// if on, turn it off
	if ( on ) {
		on = false;
		CancelEvents( &EV_Timer );
	}
}

/*
================
idTrigger_Timer::Event_Timer
================
*/
void idTrigger_Timer::Event_Timer( void ) {
	ActivateTargets( this );

	// set time before next firing
	if ( wait >= 0.0f ) {
		PostEventSec( &EV_Timer, wait + gameLocal.random.CRandomFloat() * random );
	}
}

/*
================
idTrigger_Timer::Event_Use
================
*/
void idTrigger_Timer::Event_Use( idEntity *activator ) {
	// if on, turn it off
	if ( on ) {
		if ( offName.Length() && offName.Icmp( activator->GetName() ) ) {
			return;
		}
		on = false;
		CancelEvents( &EV_Timer );
	} else {
		// turn it on
		if ( onName.Length() && onName.Icmp( activator->GetName() ) ) {
			return;
		}
		on = true;
		PostEventSec( &EV_Timer, delay );
	}
}

/*
===============================================================================

  idTrigger_Count
	
===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_Count )
	EVENT( EV_Activate,	idTrigger_Count::Event_Trigger )
	EVENT( EV_TriggerAction,	idTrigger_Count::Event_TriggerAction )
END_CLASS

/*
================
idTrigger_Count::idTrigger_Count
================
*/
idTrigger_Count::idTrigger_Count( void ) {
	goal = 0;
	count = 0;
	delay = 0.0f;
}

/*
================
idTrigger_Count::Save
================
*/
void idTrigger_Count::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( goal );
	savefile->WriteInt( count );
	savefile->WriteFloat( delay );
}

/*
================
idTrigger_Count::Restore
================
*/
void idTrigger_Count::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( goal );
	savefile->ReadInt( count );
	savefile->ReadFloat( delay );
}

/*
================
idTrigger_Count::Spawn
================
*/
void idTrigger_Count::Spawn( void ) {
	spawnArgs.GetInt( "count", "1", goal );
	spawnArgs.GetFloat( "delay", "0", delay );
	count = 0;
}

/*
================
idTrigger_Count::Event_Trigger
================
*/
void idTrigger_Count::Event_Trigger( idEntity *activator ) {
	// goal of -1 means trigger has been exhausted
	if (goal >= 0) {
		count++;
		if ( count >= goal ) {
			if (spawnArgs.GetBool("repeat")) {
				count = 0;
			} else {
				goal = -1;
			}
			PostEventSec( &EV_TriggerAction, delay, activator );
		}
	}
}

/*
================
idTrigger_Count::Event_TriggerAction
================
*/
void idTrigger_Count::Event_TriggerAction( idEntity *activator ) {
	ActivateTargets( activator );
	CallScript( activator );
	if ( goal == -1 ) {
		PostEventMS( &EV_Remove, 0 );
	}
}

/*
===============================================================================

  idTrigger_Hurt
	
===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_Hurt )
	EVENT( EV_Touch,		idTrigger_Hurt::Event_Touch )
	EVENT( EV_Activate,		idTrigger_Hurt::Event_Toggle )
END_CLASS

/*
================
idTrigger_Hurt::idTrigger_Hurt
================
*/
idTrigger_Hurt::idTrigger_Hurt( void ) {
	on = false;
	delay = 0.0f;
	nextTime = 0;
}

/*
================
idTrigger_Hurt::Save
================
*/
void idTrigger_Hurt::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( on );
	savefile->WriteFloat( delay );
	savefile->WriteInt( nextTime );
// RAVEN BEGIN
// bdube: playeronly flag
	savefile->WriteBool ( playerOnly );
// RAVEN END	
}

/*
================
idTrigger_Hurt::Restore
================
*/
void idTrigger_Hurt::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( on );
	savefile->ReadFloat( delay );
	savefile->ReadInt( nextTime );
// RAVEN BEGIN
// bdube: playeronly flag
	savefile->ReadBool( playerOnly );
// RAVEN END	
}

/*
================
idTrigger_Hurt::Spawn

	Damages activator
	Can be turned on or off by using.
================
*/
void idTrigger_Hurt::Spawn( void ) {
	spawnArgs.GetBool( "on", "1", on );
	spawnArgs.GetFloat( "delay", "1.0", delay );

// RAVEN BEGIN
// kfuller: playeronly flag
	spawnArgs.GetBool( "playerOnly", "0", playerOnly );
// RAVEN END

	nextTime = gameLocal.time;
	Enable();
}

/*
================
idTrigger_Hurt::Event_Touch
================
*/
void idTrigger_Hurt::Event_Touch( idEntity *other, trace_t *trace ) {
	const char *damage;

// RAVEN BEGIN
// kfuller: playeronly flag
// jnewquist: Use accessor for static class type 
	if ( playerOnly && !other->IsType( idPlayer::GetClassType() ) ) {
		return;
	}
// RAVEN END

	if ( on && other && gameLocal.time >= nextTime ) {
		damage = spawnArgs.GetString( "def_damage", "damage_painTrigger" );
		other->Damage( this, NULL, vec3_origin, damage, 1.0f, INVALID_JOINT );

		ActivateTargets( other );
		CallScript( other );

		nextTime = gameLocal.time + SEC2MS( delay );
	}
}

/*
================
idTrigger_Hurt::Event_Toggle
================
*/
void idTrigger_Hurt::Event_Toggle( idEntity *activator ) {
	on = !on;
}


/*
===============================================================================

  idTrigger_Fade

===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_Fade )
	EVENT( EV_Activate,		idTrigger_Fade::Event_Trigger )
END_CLASS

/*
================
idTrigger_Fade::Event_Trigger
================
*/
void idTrigger_Fade::Event_Trigger( idEntity *activator ) {
	idVec4		fadeColor;
	int			fadeTime;
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( player ) {
		fadeColor = spawnArgs.GetVec4( "fadeColor", "0, 0, 0, 1" );
		fadeTime = SEC2MS( spawnArgs.GetFloat( "fadeTime", "0.5" ) );
		player->playerView.Fade( fadeColor, fadeTime );
		PostEventMS( &EV_ActivateTargets, fadeTime, activator );
	}
}

/*
===============================================================================

  idTrigger_Touch
	
===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_Touch )
	EVENT( EV_Activate,		idTrigger_Touch::Event_Trigger )
END_CLASS


/*
================
idTrigger_Touch::idTrigger_Touch
================
*/
idTrigger_Touch::idTrigger_Touch( void ) {
	clipModel = NULL;
}


/*
================
idTrigger_Touch::idTrigger_Touch
================
*/
idTrigger_Touch::~idTrigger_Touch( ) {
	if ( clipModel ) {
		delete clipModel;
		clipModel = 0;
	}
}

/*
================
idTrigger_Touch::Spawn
================
*/
void idTrigger_Touch::Spawn( void ) {
	// get the clip model
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	clipModel = new idClipModel( GetPhysics()->GetClipModel() );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
	// remove the collision model from the physics object
	GetPhysics()->SetClipModel( NULL, 1.0f );

	if ( spawnArgs.GetBool( "start_on" ) ) {
		BecomeActive( TH_THINK );
	}
	filterTeam = -1;
	idStr filterTeamStr = spawnArgs.GetString( "filterTeam" );
	if ( filterTeamStr.Size() )
	{
		if ( !idStr::Icmp( "marine", filterTeamStr.c_str() ) )
		{
			filterTeam = AITEAM_MARINE;
		}
		else if ( !idStr::Icmp( "strogg", filterTeamStr.c_str() ) )
		{
			filterTeam = AITEAM_STROGG;
		}
	}
}

/*
================
idTrigger_Touch::Save
================
*/
void idTrigger_Touch::Save( idSaveGame *savefile ) {
	savefile->WriteClipModel( clipModel );
	savefile->WriteInt( filterTeam );
}

/*
================
idTrigger_Touch::Restore
================
*/
void idTrigger_Touch::Restore( idRestoreGame *savefile ) {
	savefile->ReadClipModel( clipModel );
	savefile->ReadInt( filterTeam );
}

/*
================
idTrigger_Touch::TouchEntities
================
*/
void idTrigger_Touch::TouchEntities( void ) {
	int numClipModels, i;
	idBounds bounds;
	idClipModel *cm, *clipModelList[ MAX_GENTITIES ];

// RAVEN BEGIN
// abahr: now scriptFunction list
	if ( clipModel == NULL || !scriptFunctions.Num() ) {
// RAVEN END
		return;
	}

	bounds.FromTransformedBounds( clipModel->GetBounds(), GetBindMaster()!=NULL?GetPhysics()->GetOrigin():clipModel->GetOrigin(), GetBindMaster()!=NULL?GetPhysics()->GetAxis():clipModel->GetAxis() );
// RAVEN BEGIN
// MCG: filterTeam
	if ( filterTeam != -1 )
	{
		idActor* actor;
		// Iterate through the filter team
		for( actor = aiManager.GetAllyTeam ( (aiTeam_t)filterTeam ); actor; actor = actor->teamNode.Next() ) {
			// Skip hidden actors and actors that can't be targeted
			if( actor->fl.notarget || actor->fl.isDormant || ( actor->IsHidden ( ) && !actor->IsInVehicle() ) ) {
				continue;
			}
			if ( !bounds.IntersectsBounds ( actor->GetPhysics()->GetAbsBounds ( ) ) ) {
				continue;
			}
			cm = actor->GetPhysics()->GetClipModel();
			if ( !cm || !cm->IsTraceModel() ) {
				continue;
			}
			if ( !gameLocal.ContentsModel( this, cm->GetOrigin(), cm, cm->GetAxis(), -1,
				clipModel->GetCollisionModel(), GetBindMaster()!=NULL?GetPhysics()->GetOrigin():clipModel->GetOrigin(), GetBindMaster()!=NULL?GetPhysics()->GetAxis():clipModel->GetAxis() ) ) {
				continue;
			}
			ActivateTargets( (idEntity*)actor );

			CallScript( (idEntity*)actor );
		}
		return;
	}
// ddynerman: multiple clip worlds
	numClipModels = gameLocal.ClipModelsTouchingBounds( this, bounds, -1, clipModelList, MAX_GENTITIES );
// RAVEN END

	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModelList[ i ];

		if ( !cm->IsTraceModel() ) {
			continue;
		}

		idEntity *entity = cm->GetEntity();

		if ( !entity ) {
			continue;
		}
		
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		if ( !gameLocal.ContentsModel( this, cm->GetOrigin(), cm, cm->GetAxis(), -1,
									clipModel->GetCollisionModel(), clipModel->GetOrigin(), clipModel->GetAxis() ) ) {
// RAVEN END
			continue;
		}

		ActivateTargets( entity );

// RAVEN BEGIN
// abahr: changed to be compatible with new script function utility
		CallScript( entity );
// RAVEN END
	}
}

/*
================
idTrigger_Touch::Think
================
*/
void idTrigger_Touch::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		TouchEntities();
	}
	idEntity::Think();
}

/*
================
idTrigger_Touch::Event_Trigger
================
*/
void idTrigger_Touch::Event_Trigger( idEntity *activator ) {
	if ( thinkFlags & TH_THINK ) {
		BecomeInactive( TH_THINK );
	} else {
		BecomeActive( TH_THINK );
	}
}

/*
================
idTrigger_Touch::Enable
================
*/
void idTrigger_Touch::Enable( void ) {
	BecomeActive( TH_THINK );
}

/*
================
idTrigger_Touch::Disable
================
*/
void idTrigger_Touch::Disable( void ) {
	BecomeInactive( TH_THINK );
}
