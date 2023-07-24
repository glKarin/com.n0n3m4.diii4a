// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Trigger.h"
#include "Player.h"
#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"
#include "vehicles/Transport.h"

/*
===============================================================================

  idTrigger
	
===============================================================================
*/

extern const idEventDef EV_Enable;
extern const idEventDef EV_Disable;

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

	viewTextBounds.ExpandSelf( 128.0f );
	viewBounds.ExpandSelf( 512.0f );
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->GetPhysics()->GetContents() & CONTENTS_TRIGGER ) {
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
	EnableClip();
}

/*
================
idTrigger::Disable
================
*/
void idTrigger::Disable( void ) {
	// we may be relinked if we're bound to another object, so clear the contents as well
	GetPhysics()->SetContents( 0 );
	DisableClip();
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
idTrigger::idTrigger( void ) {
	onTouchFunc = NULL;
}

/*
================
idTrigger::Spawn
================
*/
void idTrigger::Spawn( void ) {
	GetPhysics()->SetContents( CONTENTS_TRIGGER );

	requirements.Load( spawnArgs, "require" );

	if ( scriptObject ) {
		onTouchFunc = scriptObject->GetFunction( "OnTouch" );
	}
}


/*
===============================================================================

  idTrigger_Multi
	
===============================================================================
*/

const idEventDefInternal EV_TriggerAction( "internal_triggerAction", "e" );

CLASS_DECLARATION( idTrigger, idTrigger_Multi )
	EVENT( EV_Activate,			idTrigger_Multi::Event_Trigger )
	EVENT( EV_TriggerAction,	idTrigger_Multi::Event_TriggerAction )
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

	GetPhysics()->SetContents( CONTENTS_TRIGGER );
}

/*
================
idTrigger_Multi::CheckFacing
================
*/
bool idTrigger_Multi::CheckFacing( idEntity *activator ) {
	if ( spawnArgs.GetBool( "facing" ) ) {
		idPlayer* player = activator->Cast< idPlayer >();
		if ( !player ) {
			return true;
		}
		float dot = player->viewAngles.ToForward() * GetPhysics()->GetAxis()[0];
		float angle = RAD2DEG( idMath::ACos( dot ) );
		if ( angle > spawnArgs.GetFloat( "angleLimit", "30" ) ) {
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
	if ( onTouchFunc ) {
		sdScriptHelper h1;
		h1.Push( activator ? activator->GetScriptObject() : NULL );
		scriptObject->CallNonBlockingScriptEvent( onTouchFunc, h1 );
	}

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
	if ( nextTriggerTime > gameLocal.time ) {
		// can't retrigger until the wait is over
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
idTrigger_Multi::OnTouch
================
*/
void idTrigger_Multi::OnTouch( idEntity *other, const trace_t& trace ) {
	if( triggerFirst ) {
		return;
	}

	if( other->IsType( sdTransport::Type ) ) {
		sdTransport* transport = reinterpret_cast< sdTransport* >( other );
		
		idPlayer* driver = transport->GetPositionManager().FindDriver();
		if( !driver ) {
			return;
		}

		other = driver;
	}

	idPlayer* player = other->Cast< idPlayer >();
	if ( player != NULL && player->GetHealth() <= 0 ) {
		return;
	}

	if ( player ) {
		if ( !touchClient ) {
			return;
		}
		if ( player->IsSpectating() ) {
			return;
		}
	} else if ( !touchOther ) {
		return;
	}

	if ( nextTriggerTime > gameLocal.time ) {
		// can't retrigger until the wait is over
		return;
	}

	if ( player ) {
		if( !requirements.Check( player ) ) {
			return;
		}
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

/*
===============================================================================

  idTrigger_Hurt
	
===============================================================================
*/

CLASS_DECLARATION( idTrigger, idTrigger_Hurt )
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
	nextPassSoundTime = 0;
	nextFailSoundTime = 0;
	triggerTimeList.Clear();
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
	delay = SEC2MS( spawnArgs.GetFloat( "delay", "1" ) );

	damageDecl = DAMAGE_FOR_NAME( spawnArgs.GetString( "dmg_damage", "damage_painTrigger" ) );

	Enable();
}

/*
================
idTrigger_Hurt::PlayPassSound
================
*/
void idTrigger_Hurt::PlayPassSound( void ) {
	if ( gameLocal.time < nextPassSoundTime ) {
		return;
	}
	int length;
	if ( StartSound( "snd_pass", SND_ANY, 0, &length ) ) {
		nextPassSoundTime = gameLocal.time + length;
	} else {
		nextPassSoundTime = gameLocal.time + MINS2MS( 1 );
	}
}

/*
================
idTrigger_Hurt::PlayFailSound
================
*/
void idTrigger_Hurt::PlayFailSound( void ) {
	if ( gameLocal.time < nextFailSoundTime ) {
		return;
	}
	int length;
	if ( StartSound( "snd_fail", SND_ANY, 0, &length ) ) {
		nextFailSoundTime = gameLocal.time + length;
	} else {
		nextFailSoundTime = gameLocal.time + MINS2MS( 1 );
	}
}

/*
================
idTrigger_Hurt::UpdateTriggerEntities
================
*/
void idTrigger_Hurt::UpdateTriggerEntities( idEntity *ent ) {
	int newTime = gameLocal.time + delay;

	for ( int i = 0; i < triggerTimeList.Num() ; i++ ) {
		if ( triggerTimeList[ i ].first == ent ) {
			triggerTimeList[ i ].second = newTime;
			return;
		}
	}

	for ( int i = 0; i < triggerTimeList.Num(); i++ ) {
		if ( !triggerTimeList[ i ].first.IsValid() ) {
			triggerTimeList[ i ].first = ent;
			triggerTimeList[ i ].second = newTime;
			return;
		}
	}

	triggerTimeList.Append( entityTriggerTime_t( ent, newTime ) );
}

/*
================
idTrigger_Hurt::CanTrigger
================
*/
bool idTrigger_Hurt::HasTriggered( idEntity *ent ) {
	for ( int i = 0; i < triggerTimeList.Num(); i++ ) {
		if ( triggerTimeList[ i ].first == ent ) {
			return triggerTimeList[ i ].second >= gameLocal.time;
		}
	}

	return false;
}
/*
================
idTrigger_Hurt::OnTouch
================
*/
void idTrigger_Hurt::OnTouch( idEntity *other, const trace_t& trace ) {
	if ( !on || other == NULL || HasTriggered( other ) ) {
		return;
	}

	UpdateTriggerEntities( other );

	idPlayer* player = other->Cast< idPlayer >();
	if ( player != NULL && player->IsInLimbo() ) {
		return;
	}

	if ( player != NULL ) {
		if ( player->IsSpectator() ) {
			return;
		}

		if ( !requirements.Check( player ) ) {
			PlayPassSound();
			return;
		} else {
			PlayFailSound();
		}
	}

	other->Damage( this, this, other->GetPhysics()->GetOrigin() - GetPhysics()->GetAbsBounds().GetCenter(), damageDecl, 1.0f, NULL );
}

/*
================
idTrigger_Hurt::Event_Toggle
================
*/
void idTrigger_Hurt::Event_Toggle( idEntity *activator ) {
	on = !on;
}
