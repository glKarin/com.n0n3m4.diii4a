/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "script/Script_Thread.h"
#include "Player.h"

#include "WorldSpawn.h"

#include "Trigger.h"

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
		if ( ent->GetPhysics()->GetContents() & ( CONTENTS_TRIGGER | CONTENTS_FLASHLIGHT_TRIGGER | CONTENTS_PLAYERLOOK_TRIGGER ) ) {
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

	if (spawnArgs.GetBool("playerlook_trigger"))
	{
		GetPhysics()->SetContents(CONTENTS_PLAYERLOOK_TRIGGER);
	}
	else
	{
		GetPhysics()->SetContents(CONTENTS_TRIGGER);
	}

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

void idTrigger::CallScriptArgs( idEntity* activator, idEntity* self, const function_t* functionToCall ) const
{
	if ( functionToCall == nullptr )
	{
		functionToCall = scriptFunction;
	}

	if ( functionToCall != NULL )
	{
		assert( functionToCall->parmSize.Num() <= 2 );
		idThread *thread = new idThread();
		
		// 1 or 0 args always pushes activator (this preserves the old behavior)
		thread->PushArg( activator );
		// 2 args pushes self as well
		if ( functionToCall->parmSize.Num() == 2 )
		{
			thread->PushArg( self );
		}
		
		thread->CallFunction( functionToCall, false );
		thread->DelayedStart( 0 );
	}
}

/*
================
idTrigger::GetScriptFunction
================
*/
const function_t *idTrigger::GetScriptFunction( void ) const {
	return scriptFunction;
}

/*
================
idTrigger::Save
================
*/
void idTrigger::Save( idSaveGame *savefile ) const {
	savefile->WriteFunction( scriptFunction );
}

/*
================
idTrigger::Restore
================
*/
void idTrigger::Restore( idRestoreGame *savefile ) {
	savefile->ReadFunction( scriptFunction );
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
	scriptFunction = NULL;
}

/*
================
idTrigger::Spawn
================
*/
void idTrigger::Spawn( void ) {
	GetPhysics()->SetContents( CONTENTS_TRIGGER );

	idStr funcname = spawnArgs.GetString( "call", "" );
	if ( funcname.Length() ) {
		scriptFunction = gameLocal.program.FindFunction( funcname );
		if ( scriptFunction == NULL ) {
			gameLocal.Warning( "trigger '%s' at (%s) calls unknown function '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), funcname.c_str() );
		}
	} else {
		scriptFunction = NULL;
	}
}


/*
===============================================================================

  idTrigger_Multi

===============================================================================
*/

const idEventDef EV_TriggerAction( "<triggerAction>", "e" );
const idEventDef EV_PlayerLookEnter( "PlayerLookEnter" );
const idEventDef EV_PlayerLookExit( "PlayerLookExit" );

CLASS_DECLARATION( idTrigger, idTrigger_Multi )
	EVENT( EV_Touch,			idTrigger_Multi::Event_Touch )
	EVENT( EV_Activate,			idTrigger_Multi::Event_Trigger )
	EVENT( EV_TriggerAction,	idTrigger_Multi::Event_TriggerAction )
	EVENT( EV_PlayerLookEnter,	idTrigger_Multi::Event_PlayerLookEnter )
	EVENT( EV_PlayerLookExit,	idTrigger_Multi::Event_PlayerLookExit )
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
	triggerFirst = false;
	triggerWithSelf = false;
}

/*
================
idTrigger_Multi::Save
================
*/
void idTrigger_Multi::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( wait ); // float wait
	savefile->WriteFloat( random ); // float random
	savefile->WriteFloat( delay ); // float delay
	savefile->WriteFloat( random_delay ); // float random_delay
	savefile->WriteInt( nextTriggerTime ); // int nextTriggerTime
	savefile->WriteString( _requires ); // idString _requires
	savefile->WriteInt( removeItem ); // int removeItem
	savefile->WriteBool( touchClient ); // bool touchClient
	savefile->WriteBool( touchOther ); // bool touchOther
	savefile->WriteBool( triggerFirst ); // bool triggerFirst
	savefile->WriteBool( triggerWithSelf ); // bool triggerWithSelf

	savefile->WriteFunction( playerLookEnterFunc ); // const function_t* playerLookEnterFunc
	savefile->WriteFunction( playerLookExitFunc ); // const function_t* playerLookExitFunc

	savefile->WriteFloat( playerLookRange ); // float playerLookRange
}

/*
================
idTrigger_Multi::Restore
================
*/
void idTrigger_Multi::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( wait ); // float wait
	savefile->ReadFloat( random ); // float random
	savefile->ReadFloat( delay ); // float delay
	savefile->ReadFloat( random_delay ); // float random_delay
	savefile->ReadInt( nextTriggerTime ); // int nextTriggerTime
	savefile->ReadString( _requires ); // idString _requires
	savefile->ReadInt( removeItem ); // int removeItem
	savefile->ReadBool( touchClient ); // bool touchClient
	savefile->ReadBool( touchOther ); // bool touchOther
	savefile->ReadBool( triggerFirst ); // bool triggerFirst
	savefile->ReadBool( triggerWithSelf ); // bool triggerWithSelf

	savefile->ReadFunction( playerLookEnterFunc ); // const function_t* playerLookEnterFunc
	savefile->ReadFunction( playerLookExitFunc ); // const function_t* playerLookExitFunc

	savefile->ReadFloat( playerLookRange ); // float playerLookRange
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

	spawnArgs.GetString( "requires", "", _requires );
	spawnArgs.GetInt( "removeItem", "0", removeItem );
	spawnArgs.GetBool( "triggerFirst", "0", triggerFirst );
	spawnArgs.GetBool( "triggerWithSelf", "0", triggerWithSelf );

	if ( spawnArgs.GetBool( "anyTouch" ) ) {
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
	playerLookEnterFunc = nullptr;
	playerLookExitFunc = nullptr;
	playerLookRange = 0.0f;

	if ( spawnArgs.GetBool( "flashlight_trigger" ) ) {
		GetPhysics()->SetContents( CONTENTS_FLASHLIGHT_TRIGGER );
	} else if ( spawnArgs.GetBool( "playerlook_trigger" ) ) {
		GetPhysics()->SetContents( CONTENTS_PLAYERLOOK_TRIGGER );
		
		idStr funcname = spawnArgs.GetString( "playerlook_enter", "" );
		if ( funcname.Length() > 1 ) {
			playerLookEnterFunc = gameLocal.program.FindFunction( funcname );
			if ( playerLookEnterFunc == NULL ) {
				gameLocal.Warning( "trigger '%s' at (%s) playerlook_enter calls unknown function '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ), funcname.c_str() );
			}
		}

		funcname = spawnArgs.GetString( "playerlook_exit", "" );
		if ( funcname.Length() > 1) {
			playerLookExitFunc = gameLocal.program.FindFunction( funcname );
			if ( playerLookExitFunc == NULL ) {
				gameLocal.Warning( "trigger '%s' at (%s) playerlook_exit calls unknown function '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ), funcname.c_str() );
			}
		}

		playerLookRange = spawnArgs.GetFloat( "playerlook_range", "0" );
	} else {
		GetPhysics()->SetContents( CONTENTS_TRIGGER );
	}
}

/*
================
idTrigger_Multi::CheckFacing
================
*/
bool idTrigger_Multi::CheckFacing( idEntity *activator ) {
	if ( spawnArgs.GetBool( "facing" ) ) {
		if ( !activator->IsType( idPlayer::Type ) ) {
			return true;
		}
		idPlayer *player = static_cast< idPlayer* >( activator );
		float dot = player->viewAngles.ToForward() * GetPhysics()->GetAxis()[0];
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
	ActivateTargets( triggerWithSelf ? this : activator );
	CallScriptArgs(activator, this);

	if ( wait >= 0 ) {
		nextTriggerTime = gameLocal.time + SEC2MS( wait + random * gameLocal.random.CRandomFloat() );
	} else {
		// we can't just remove (this) here, because this is a touch function
		// called while looping through area links...
#ifdef _D3XP
		// If the player spawned inside the trigger, the player Spawn function called Think directly,
		// allowing for multiple triggers on a trigger_once.  Increasing the nextTriggerTime prevents it.
		nextTriggerTime = gameLocal.time + 99999;
#else
		nextTriggerTime = gameLocal.time + 1;
#endif
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
	if ( nextTriggerTime > gameLocal.time ) {
		// can't retrigger until the wait is over
		return;
	}

	// see if this trigger requires an item
	if ( !gameLocal.RequirementMet( activator, _requires, removeItem ) ) {
		return;
	}

	if ( !CheckFacing( activator ) ) {
		return;
	}

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
idTrigger_Multi::Event_Touch
================
*/
void idTrigger_Multi::Event_Touch( idEntity *other, trace_t *trace ) {
	if( triggerFirst ) {
		return;
	}

	bool player = other->IsType( idPlayer::Type );
	if ( player ) {
		if ( !touchClient ) {
			return;
		}
		if ( static_cast< idPlayer * >( other )->spectating ) {
			return;
		}
	} else if ( !touchOther ) {
		return;
	}

	if ( nextTriggerTime > gameLocal.time ) {
		// can't retrigger until the wait is over
		return;
	}

	// see if this trigger requires an item
	if ( !gameLocal.RequirementMet( other, _requires, removeItem ) ) {
		return;
	}

	if ( !CheckFacing( other ) ) {
		return;
	}

	if ( spawnArgs.GetBool( "toggleTriggerFirst" ) ) {
		triggerFirst = true;
	}

	nextTriggerTime = gameLocal.time + 1;
	if ( delay > 0 ) {
		// don't allow it to trigger again until our delay has passed
		nextTriggerTime += SEC2MS( delay + random_delay * gameLocal.random.CRandomFloat() );
		PostEventSec( &EV_TriggerAction, delay, other );
	} else {
		TriggerAction( other );
	}
}


void idTrigger_Multi::Event_PlayerLookEnter()
{
	if ( playerLookEnterFunc )
	{
		CallScriptArgs( gameLocal.GetLocalPlayer(), this, playerLookEnterFunc );
	}
}


void idTrigger_Multi::Event_PlayerLookExit()
{
	if ( playerLookExitFunc )
	{
		CallScriptArgs( gameLocal.GetLocalPlayer(), this, playerLookExitFunc );
	}
}

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
	savefile->WriteFloat( wait ); // float wait
	savefile->WriteFloat( random ); // float random
	savefile->WriteFloat( delay ); // float delay
	savefile->WriteFloat( random_delay ); // float random_delay
	savefile->WriteInt( nextTriggerTime ); // int nextTriggerTime
	savefile->WriteBool( triggerFirst ); // bool triggerFirst
	savefile->WriteString( entityName ); // idString entityName
}

/*
================
idTrigger_EntityName::Restore
================
*/
void idTrigger_EntityName::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( wait ); // float wait
	savefile->ReadFloat( random ); // float random
	savefile->ReadFloat( delay ); // float delay
	savefile->ReadFloat( random_delay ); // float random_delay
	savefile->ReadInt( nextTriggerTime ); // int nextTriggerTime
	savefile->ReadBool( triggerFirst ); // bool triggerFirst
	savefile->ReadString( entityName ); // idString entityName
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
	ActivateTargets( activator );
	CallScriptArgs(activator, this);

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

	if ( !activator || ( activator->name != entityName ) ) {
		return;
	}

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

	if ( !other || ( other->name != entityName ) ) {
		return;
	}

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
	savefile->WriteFloat( random ); // float random
	savefile->WriteFloat( wait ); // float wait
	savefile->WriteBool( on ); // bool on
	savefile->WriteFloat( delay ); // float delay
	savefile->WriteString( onName ); // idString onName
	savefile->WriteString( offName ); // idString offName
}

/*
================
idTrigger_Timer::Restore
================
*/
void idTrigger_Timer::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( random ); // float random
	savefile->ReadFloat( wait ); // float wait
	savefile->ReadBool( on ); // bool on
	savefile->ReadFloat( delay ); // float delay
	savefile->ReadString( onName ); // idString onName
	savefile->ReadString( offName ); // idString offName
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
	savefile->WriteInt( goal ); // int goal
	savefile->WriteInt( count ); // int count
	savefile->WriteFloat( delay ); // float delay
}

/*
================
idTrigger_Count::Restore
================
*/
void idTrigger_Count::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( goal ); // int goal
	savefile->ReadInt( count ); // int count
	savefile->ReadFloat( delay ); // float delay
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
	CallScriptArgs(activator, this);
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
	savefile->WriteBool( on ); // bool on
	savefile->WriteFloat( delay ); // float delay
	savefile->WriteInt( nextTime ); // int nextTime
}

/*
================
idTrigger_Hurt::Restore
================
*/
void idTrigger_Hurt::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( on ); // bool on
	savefile->ReadFloat( delay ); // float delay
	savefile->ReadInt( nextTime ); // int nextTime
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

	if ( on && other && gameLocal.time >= nextTime ) {

		bool playerOnly = spawnArgs.GetBool( "playerOnly" );
		if ( playerOnly ) {
			if ( !other->IsType( idPlayer::Type ) ) {
				return;
			}
		}

		damage = spawnArgs.GetString( "def_damage", "damage_painTrigger" );



		idVec3 dir = vec3_origin;
		if(spawnArgs.GetBool("kick_from_center", "0")) {
			dir = other->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
			dir.Normalize();
		}
		else
		{
			//BC so we know the flame origin point.
			idVec3 damage_origin = spawnArgs.GetVector("damage_origin");

			if (damage_origin != vec3_zero)
			{
				dir = other->GetPhysics()->GetOrigin() - damage_origin;
				dir.Normalize();
			}
		}

		other->Damage( this, NULL, dir, damage, 1.0f, INVALID_JOINT );



		ActivateTargets( other );
		CallScriptArgs(other, this);

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
idTrigger_Touch::Spawn
================
*/
void idTrigger_Touch::Spawn( void ) {
	// get the clip model
	clipModel = new idClipModel( GetPhysics()->GetClipModel() );

	// remove the collision model from the physics object
	GetPhysics()->SetClipModel( NULL, 1.0f );

	if ( spawnArgs.GetBool( "start_on" ) ) {
		BecomeActive( TH_THINK );
	}
}

/*
================
idTrigger_Touch::Save
================
*/
void idTrigger_Touch::Save( idSaveGame *savefile ) {
	savefile->WriteClipModel( clipModel ); // 	idClipModel * clipModel
}

/*
================
idTrigger_Touch::Restore
================
*/
void idTrigger_Touch::Restore( idRestoreGame *savefile ) {
	savefile->ReadClipModel( clipModel ); // 	idClipModel * clipModel
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

	if ( clipModel == NULL || scriptFunction == NULL ) {
		return;
	}

	bounds.FromTransformedBounds( clipModel->GetBounds(), clipModel->GetOrigin(), clipModel->GetAxis() );
	numClipModels = gameLocal.clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModelList[ i ];

		if ( !cm->IsTraceModel() ) {
			continue;
		}

		idEntity *entity = cm->GetEntity();

		if ( !entity ) {
			continue;
		}

		if ( !gameLocal.clip.ContentsModel( cm->GetOrigin(), cm, cm->GetAxis(), -1,
									clipModel->Handle(), clipModel->GetOrigin(), clipModel->GetAxis() ) ) {
			continue;
		}

		ActivateTargets( entity );

		idThread *thread = new idThread();
		thread->CallFunction( entity, scriptFunction, false );
		thread->DelayedStart( 0 );
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



#ifdef CTF
/*
===============================================================================

  idTrigger_Flag

===============================================================================
*/

CLASS_DECLARATION( idTrigger_Multi, idTrigger_Flag )
	EVENT( EV_Touch, idTrigger_Flag::Event_Touch )
END_CLASS

idTrigger_Flag::idTrigger_Flag( void ) {
	team		= -1;
	player		= false;
	eventFlag	= NULL;
}

void idTrigger_Flag::Spawn( void ) {
	team = spawnArgs.GetInt( "team", "0" );
	player = spawnArgs.GetBool( "player", "0" );

	idStr funcname = spawnArgs.GetString( "eventflag", "" );
	if ( funcname.Length() ) {
		eventFlag = idEventDef::FindEvent( funcname );// gameLocal.program.FindFunction( funcname );//, &idItemTeam::Type );
		if ( eventFlag == NULL ) {
			gameLocal.Warning( "trigger '%s' at (%s) event unknown '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), funcname.c_str() );
		}
	} else {
		eventFlag = NULL;
	}

	idTrigger_Multi::Spawn();
}

void idTrigger_Flag::Save(idSaveGame* savefile) {
	savefile->WriteInt( team ); // int team
	savefile->WriteBool( player ); // bool player
	savefile->WriteEventDef( eventFlag ); // const idEventDef * eventFlag
}
void idTrigger_Flag::Restore(idRestoreGame* savefile) {
	savefile->ReadInt( team ); // int team
	savefile->ReadBool( player ); // bool player
	savefile->ReadEventDef( eventFlag ); // const idEventDef * eventFlag
}

void idTrigger_Flag::Event_Touch( idEntity *other, trace_t *trace ) {

	idItemTeam * flag = NULL;

	if ( player ) {
		if ( !other->IsType( idPlayer::Type ) )
			return;

		idPlayer * player = static_cast<idPlayer *>(other);
		if ( player->carryingFlag == false )
			return;

		if ( team != -1 && ( player->team != team || (player->team != 0 && player->team != 1))  )
			return;

		idItemTeam * flags[2];

		flags[0] = gameLocal.mpGame.GetTeamFlag( 0 );
		flags[1] = gameLocal.mpGame.GetTeamFlag( 1 );

		int iFriend = 1 - player->team;			// index to the flag player team wants
		int iOpp	= player->team;				// index to the flag opp team wants

		// flag is captured if :
		// 1)flag is truely bound to the player
		// 2)opponent flag has been return
		if ( flags[iFriend]->carried && !flags[iFriend]->dropped && //flags[iFriend]->IsBoundTo( player ) &&
			!flags[iOpp]->carried && !flags[iOpp]->dropped )
			flag = flags[iFriend];
		else
			return;
	} else {
		if ( !other->IsType( idItemTeam::Type ) )
			return;

		idItemTeam * item = static_cast<idItemTeam *>( other );

		if ( item->team == team || team == -1 ) {
			flag = item;
		}
		else
			return;
	}

	if ( flag ) {
		switch ( eventFlag->GetNumArgs() ) {
			default :
			case 0 :
				flag->PostEventMS( eventFlag, 0 );
			break;
			case 1 :
				flag->PostEventMS( eventFlag, 0, 0 );
			break;
			case 2 :
				flag->PostEventMS( eventFlag, 0, 0, 0 );
			break;
		}

/*
		ServerSendEvent( eventFlag->GetEventNum(), NULL, true, false );

		idThread *thread;
		if ( scriptFlag ) {
			thread = new idThread();
			thread->CallFunction( flag, scriptFlag, false );
			thread->DelayedStart( 0 );
		}
*/
		idTrigger_Multi::Event_Touch( other, trace );
	}
}







//BC

const idEventDef EV_triggerpushactivate("triggerpushactivate", "d");

//bc trigger push.
CLASS_DECLARATION(idTrigger, idTrigger_Push)
EVENT(EV_Touch, idTrigger_Push::Event_Touch)
EVENT(EV_triggerpushactivate, idTrigger_Push::Event_triggerpushactivate)
END_CLASS

idTrigger_Push::idTrigger_Push(void)
{
	on = false;
	wait = 0.2f;
	yaw = 0;
	pitch = -20;
	force = 256;
}

void idTrigger_Push::Save(idSaveGame *savefile) const {
	savefile->WriteBool( on ); // bool on
	savefile->WriteInt( nextTime ); // int nextTime
	savefile->WriteInt( yaw ); // int yaw
	savefile->WriteInt( pitch ); // int pitch
	savefile->WriteInt( force ); // int force
	savefile->WriteFloat( wait ); // float wait
}

void idTrigger_Push::Restore(idRestoreGame *savefile) {
	savefile->ReadBool( on ); // bool on
	savefile->ReadInt( nextTime ); // int nextTime
	savefile->ReadInt( yaw ); // int yaw
	savefile->ReadInt( pitch ); // int pitch
	savefile->ReadInt( force ); // int force
	savefile->ReadFloat( wait ); // float wait
}


void idTrigger_Push::Spawn(void)
{
	spawnArgs.GetBool("start_on", "1", on);
	spawnArgs.GetFloat("wait", "0.2", wait);
	spawnArgs.GetInt("yaw", "0", yaw);
	spawnArgs.GetInt("pitch", "-20", pitch);
	spawnArgs.GetInt("force", "256", force);

	nextTime = gameLocal.time;
	Enable();
}

void idTrigger_Push::Event_Touch(idEntity *other, trace_t *trace)
{
	if (on && other && gameLocal.time >= nextTime)
	{
		//push the thing.
		idAngles ang;
		ang.pitch = pitch;
		ang.yaw = yaw;
		ang.roll = 0;

		idVec3 dir;
		dir = ang.ToForward();

		dir *= force;

		other->GetPhysics()->SetLinearVelocity(other->GetPhysics()->GetLinearVelocity() + dir);

		nextTime = gameLocal.time + SEC2MS(wait);
	}
}

void idTrigger_Push::Event_triggerpushactivate(int value)
{
	if (value > 0)
	{
		on = 1;
	}
	else
	{
		on = 0;
	}
}


//BC
// ---------------- idTrigger_classname ------------------
// Triggers when it touches a specific classname.

CLASS_DECLARATION(idTrigger, idTrigger_Classname)
EVENT(EV_Activate, idTrigger_Classname::Event_Trigger)
END_CLASS

idTrigger_Classname::idTrigger_Classname(void) {
	clipModel = NULL;
}

void idTrigger_Classname::Spawn(void) {
	// get the clip model
	clipModel = new idClipModel(GetPhysics()->GetClipModel());

	// remove the collision model from the physics object
	GetPhysics()->SetClipModel(NULL, 1.0f);

	if (spawnArgs.GetBool("start_on"))
	{
		BecomeActive(TH_THINK);
	}

	const char* requiredClassname = spawnArgs.GetString("requires", "");
	if (requiredClassname[0] == '\0')
	{
		gameLocal.Error("'%s' is missing 'requires' keyvalue.", GetName());
	}

	idStr requiredStr( requiredClassname );
	requiredClassnames = requiredStr.Split( ',', true );
	if ( requiredClassnames.Num() == 0 )
	{
		gameLocal.Error( "'%s' has nothing under 'requires'.", GetName() );
	}

	wait = (int)(spawnArgs.GetFloat("wait", ".1") * 1000.0f);
	timer = 0;

	repeats = spawnArgs.GetBool("repeats", "0");
	isEmpty = true;
}

void idTrigger_Classname::Save(idSaveGame *savefile) {
	savefile->WriteClipModel( clipModel ); // idClipModel * clipModel

	savefile->WriteInt( wait ); // int wait
	savefile->WriteInt( timer ); // int timer
	SaveFileWriteArray( requiredClassnames, requiredClassnames.Num(), WriteString ); // idList<idStr> requiredClassnames

	savefile->WriteBool( repeats ); // bool repeats
	savefile->WriteBool( isEmpty ); // bool isEmpty
}

void idTrigger_Classname::Restore(idRestoreGame *savefile) {
	savefile->ReadClipModel( clipModel ); // idClipModel * clipModel

	savefile->ReadInt( wait ); // int wait
	savefile->ReadInt( timer ); // int timer
	SaveFileReadList( requiredClassnames, ReadString ); // idList<idStr> requiredClassnames

	savefile->ReadBool( repeats ); // bool repeats
	savefile->ReadBool( isEmpty ); // bool isEmpty
}

void idTrigger_Classname::TouchEntities(void) {
	int numClipModels, i;
	idBounds bounds;
	idClipModel *cm, *clipModelList[MAX_GENTITIES];

	if (clipModel == NULL) {
		return;
	}

	bounds.FromTransformedBounds(clipModel->GetBounds(), clipModel->GetOrigin(), clipModel->GetAxis());
	numClipModels = gameLocal.clip.ClipModelsTouchingBounds(bounds, -1, clipModelList, MAX_GENTITIES);

	bool hasMatch = false;

	for (i = 0; i < numClipModels; i++)
	{
		cm = clipModelList[i];

		if (!cm->IsTraceModel()) {
			continue;
		}

		idEntity *entity = cm->GetEntity();

		if (!entity) {
			continue;
		}

		if (entity->IsHidden())
			continue;

		if (!gameLocal.clip.ContentsModel(cm->GetOrigin(), cm, cm->GetAxis(), -1,
			clipModel->Handle(), clipModel->GetOrigin(), clipModel->GetAxis())) {
			continue;
		}

		//check for classname.
		if (Matches(entity->spawnArgs.GetString("classname")))
		{
			//Match. do nothing.
			hasMatch = true;

			if (!repeats)
			{
				if (!isEmpty)
					return;

				isEmpty = false;
			}
		}
		else
		{
			//does NOT match. Exit here.
			continue;
		}	

		ActivateTargets(entity);

		CallScriptArgs(entity, this);
	}

	if (!repeats)
	{
		if (!hasMatch && !isEmpty)
		{
			isEmpty = true;
		}
	}
}


void idTrigger_Classname::Think(void) {

	if (thinkFlags & TH_THINK && gameLocal.time >= timer && !fl.hidden)
	{
		timer = gameLocal.time + wait;
		TouchEntities();
	}

	idEntity::Think();
}


void idTrigger_Classname::Event_Trigger(idEntity *activator) {
	if (thinkFlags & TH_THINK) {
		BecomeInactive(TH_THINK);
	}
	else {
		BecomeActive(TH_THINK);
	}
}


bool idTrigger_Classname::Matches( const idStr& className )
{
	if ( className.Length() <= 0 )
	{
		return false;
	}

	for ( int i = 0; i < requiredClassnames.Num(); i++ )
	{
		const idStr& pattern = requiredClassnames[i];
		if ( pattern == className )
		{
			return true;
		}
		// This has one or more wildcards
		else if ( (pattern.Find('*') != -1  || pattern.Find('?') != -1) && className.Matches(pattern) )
		{
			return true;
		}
	}

	return false;
}

void idTrigger_Classname::Enable(void) {
	BecomeActive(TH_THINK);
}


void idTrigger_Classname::Disable(void) {
	BecomeInactive(TH_THINK);
}



// ---------------- idTrigger_FallTrigger ------------------
//Triggers when player touches it; teleports player to last solid ground they were on. Used for death pits.

CLASS_DECLARATION(idTrigger, idTrigger_FallTrigger)
	EVENT(EV_Activate,	idTrigger_FallTrigger::Event_Trigger)
	EVENT(EV_Touch,		idTrigger_FallTrigger::Event_Touch)
END_CLASS

#define FALLTRIGGER_THINKINTERVAL  300
#define CHECKBOUND_RADIUS 48
#define GROUNDCHECK_RADIUS 40

idTrigger_FallTrigger::idTrigger_FallTrigger(void)
{
}

void idTrigger_FallTrigger::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( wait ); // int wait
	savefile->WriteInt( nextTriggerTime ); // int nextTriggerTime
	savefile->WriteInt( nextThinkTime ); // int nextThinkTime

	savefile->WriteVec3( popcornPos ); // idVec3 popcornPos
	savefile->WriteFloat( popcornAngle ); // float popcornAngle
}

void idTrigger_FallTrigger::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( wait ); // int wait
	savefile->ReadInt( nextTriggerTime ); // int nextTriggerTime
	savefile->ReadInt( nextThinkTime ); // int nextThinkTime

	savefile->ReadVec3( popcornPos ); // idVec3 popcornPos
	savefile->ReadFloat( popcornAngle ); // float popcornAngle
}

void idTrigger_FallTrigger::Spawn(void)
{
	wait = (int)(spawnArgs.GetFloat("wait", "0.1") * 1000.0f);	
	nextTriggerTime = 0;
	nextThinkTime = 0;

	popcornPos = vec3_zero;
	popcornAngle = 0;

	BecomeActive(TH_THINK);
}

void idTrigger_FallTrigger::Event_Touch(idEntity *other, trace_t *trace)
{	
	if (!other->IsType(idPlayer::Type))
		return;

	if (static_cast< idPlayer * >(other)->spectating)	
		return;
		
	if (nextTriggerTime > gameLocal.time) //delay between triggers.
		return;

	nextTriggerTime = gameLocal.time + wait;

	Event_Trigger(this);
}

void idTrigger_FallTrigger::Think(void)
{
	if (gameLocal.time > nextThinkTime)
	{
		nextThinkTime = gameLocal.time + FALLTRIGGER_THINKINTERVAL;

		//check if player is on solid ground.

		if (IsPlayerOnSolidGround())
		{
			popcornPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0, 0, 1);
			popcornAngle = gameLocal.GetLocalPlayer()->viewAngles.yaw;
		}
	}
	
	idEntity::Think();
}

bool idTrigger_FallTrigger::IsPlayerOnSolidGround()
{
	//Determine whether this is a safe place to drop a popcorn marker.

	if (!gameLocal.GetLocalPlayer()->GetPhysics()->HasGroundContacts()
		|| gameLocal.GetLocalPlayer()->health <= 0)
		return false;

	//see if there's breathing room around player.
	trace_t boundTr;
	idBounds checkBound = idBounds(idVec3(-CHECKBOUND_RADIUS, -CHECKBOUND_RADIUS, 16), idVec3(CHECKBOUND_RADIUS, CHECKBOUND_RADIUS, 64));
	gameLocal.clip.TraceBounds(boundTr, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin(), gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin(), checkBound, MASK_SHOT_RENDERMODEL, gameLocal.GetLocalPlayer());
	if (boundTr.fraction < 1)
		return false;

	
	//check if we are surrounded by ground.
	idVec3 forward, right;
	idAngles playerAngle = idAngles(0, gameLocal.GetLocalPlayer()->viewAngles.yaw, 0);
	playerAngle.ToVectors(&forward, &right, NULL);
	for (int i = 0; i < 4; i++)
	{
		trace_t tr;
		idVec3 groundPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();

		if (i == 0)
			groundPos += (forward * GROUNDCHECK_RADIUS);
		else if (i == 1)
			groundPos += (forward * -GROUNDCHECK_RADIUS);
		else if (i == 2)
			groundPos += (right * GROUNDCHECK_RADIUS);
		else
			groundPos += (right * -GROUNDCHECK_RADIUS);

		groundPos.z -= 16;
		
		gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->GetEyePosition(), groundPos, MASK_SOLID, NULL);
		//gameRenderWorld->DebugArrow(tr.fraction >= 1 ? colorRed : colorGreen, gameLocal.GetLocalPlayer()->GetEyePosition(), groundPos, 4, 200);

		if (tr.fraction >= 1 || tr.c.material->GetSurfaceFlags() >= 256/*if touch sky*/)
			return false;
	}

	//check if ground directly below is worldspawn or func_static
	trace_t tr;
	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0, 0, 1), gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0, 0, -4), MASK_SOLID, NULL);
	if (tr.fraction >= 1)
		return false;

	if (!gameLocal.entities[tr.c.entityNum]->IsType(idWorldspawn::Type) && !gameLocal.entities[tr.c.entityNum]->IsType(idStaticEntity::Type))
		return false;	

	return true;
}

void idTrigger_FallTrigger::Event_Trigger(idEntity *activator)
{
	if (popcornPos == vec3_zero)
		return;

	gameLocal.GetLocalPlayer()->Teleport(popcornPos, idAngles(0, popcornAngle, 0), NULL);
	StartSound("snd_teleport", SND_CHANNEL_ANY, SSF_GLOBAL);
}

#endif
