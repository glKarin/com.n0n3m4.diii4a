#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../spawner.h"
#include "AI_Manager.h"
#include "AI_Util.h"
#include "AAS_Find.h"

/*
===============================================================================

rvAIHelper

===============================================================================
*/

CLASS_DECLARATION( idEntity, rvAIHelper  )
	EVENT( EV_Activate,	rvAIHelper::Event_Activate )
END_CLASS

/*
================
rvAIHelper::rvAIHelper
================
*/
rvAIHelper::rvAIHelper ( void ) {
	helperNode.SetOwner( this );	
}

/*
================
rvAIHelper::Spawn
================
*/
void rvAIHelper::Spawn ( void ) {
	// Auto activate?
	if ( spawnArgs.GetBool ( "start_on" ) ) {
		PostEventMS ( &EV_Activate, 0, this );
	}
}

/*
================
rvAIHelper::IsCombat
================
*/
bool rvAIHelper::IsCombat ( void ) const {
	return false;
}


/*
================
rvAIHelper::OnActivate
================
*/
void rvAIHelper::OnActivate ( bool active ) {
	if ( active ) {
		ActivateTargets ( this );
	}
}

/*
================
rvAIHelper::Event_Activate
================
*/
void rvAIHelper::Event_Activate( idEntity *activator ) {
	if ( !helperNode.InList ( ) ) {	
		aiManager.RegisterHelper ( this );
		OnActivate ( true );
	} else { 
		aiManager.UnregisterHelper ( this );
		OnActivate ( false );
	}
}

/*
================
rvAIHelper::GetDirection
================
*/
idVec3 rvAIHelper::GetDirection	( const idAI* ai ) const {
	if ( ai->team == 0 ) {
		return GetPhysics()->GetAxis()[0];
	} 
	return -GetPhysics()->GetAxis()[0];
}

/*
================
rvAIHelper::ValidateDestination
================
*/
bool rvAIHelper::ValidateDestination ( const idAI* ent, const idVec3& dest ) const {
	return true;
}

/*
===============================================================================

rvAICombatHelper

===============================================================================
*/

class rvAICombatHelper : public rvAIHelper {
public:
	CLASS_PROTOTYPE( rvAICombatHelper );

	rvAICombatHelper ( void );

	void			Spawn				( void );

	virtual bool	IsCombat			( void ) const;
	virtual bool	ValidateDestination ( const idAI* ent, const idVec3& dest ) const;
	
protected:

	virtual void	OnActivate			( bool active );

	idEntityPtr<idLocationEntity>	location;
};

CLASS_DECLARATION( rvAIHelper, rvAICombatHelper )
END_CLASS

/*
================
rvAICombatHelper::rvAICombatHelper
================
*/
rvAICombatHelper::rvAICombatHelper ( void ) {
}

/*
================
rvAICombatHelper::Spawn
================
*/
void rvAICombatHelper::Spawn ( void ) {
}

/*
================
rvAICombatHelper::OnActivate
================
*/
void rvAICombatHelper::OnActivate ( bool active ) {
	rvAIHelper::OnActivate ( active );
	
	if ( active ) {
		if ( spawnArgs.GetBool ( "tetherLocation", "1" ) ) {
			location = gameLocal.LocationForPoint ( GetPhysics()->GetOrigin() );
		} else {
			location = NULL;
		}
	}
}

/*
================
rvAICombatHelper::IsCombat
================
*/
bool rvAICombatHelper::IsCombat ( void ) const {
	return true;
}

/*
================
rvAICombatHelper::ValidateDestination
================
*/
bool rvAICombatHelper::ValidateDestination ( const idAI* ai, const idVec3& dest ) const {
	// If tethering to a location then see if the location of the given points matches our tethered location
	if ( location ) {
		if ( gameLocal.LocationForPoint ( dest - ai->GetPhysics()->GetGravityNormal() * 32.0f ) != location ) {
			return false;
		}
	}

	// Is the destination on the wrong side of the helper?
	idVec3 origin;
	idVec3 dir;
	
	dir = GetDirection(ai);
	if ( ai->enemy.ent ) {
		origin  = ai->enemy.lastKnownPosition;
		origin -= (dir * ai->combat.attackRange[0]);
	} else {
		origin  = GetPhysics()->GetOrigin();
		origin -= (dir * 32.0f);
	}
	
	if ( dir * (origin-dest) < 0.0f ) {
		return false;
	}
	
	// Would this destination link us to the wrong helper?
	if ( static_cast< const rvAIHelper * >( aiManager.FindClosestHelper( dest ) ) != this ) {
		return false;
	}
	
	return true;
}

/*
===============================================================================

									rvAIAvoid

===============================================================================
*/

class rvAIAvoid : public idEntity {
public:
	CLASS_PROTOTYPE( rvAIAvoid );

	rvAIAvoid ( void );

	void	Spawn	( void );
};

CLASS_DECLARATION( idEntity, rvAIAvoid )
END_CLASS

/*
================
rvAIAvoid::rvAIAvoid
================
*/
rvAIAvoid::rvAIAvoid ( void ) {
} 

/*
================
rvAIAvoid::Spawn
================
*/
void rvAIAvoid::Spawn ( void ) {
	int team = -1;
	if ( !spawnArgs.GetInt ( "teamFilter", "-1", team ) ) {
		//hmm, no "teamFilter" set, check "team" since many were set up like this
		team = spawnArgs.GetInt ( "team", "-1" );
	}
	aiManager.AddAvoid ( GetPhysics()->GetOrigin(), spawnArgs.GetFloat ( "radius", "64" ), team );	
	PostEventMS ( &EV_Remove, 0 );
}

/*
===============================================================================

								rvAITrigger

===============================================================================
*/

const idEventDef AI_AppendFromSpawner ( "<appendFromSpawner>", "ee" );

CLASS_DECLARATION( idEntity, rvAITrigger )
	EVENT( EV_Activate,				rvAITrigger::Event_Activate )
	EVENT( AI_AppendFromSpawner,	rvAITrigger::Event_AppendFromSpawner )
END_CLASS

/*
================
rvAITrigger::rvAITrigger
================
*/
rvAITrigger::rvAITrigger( void ) {
}

/*
================
rvAITrigger::Spawn
================
*/
void rvAITrigger::Spawn ( void ) {
	nextTriggerTime = 0;
	wait = SEC2MS ( spawnArgs.GetFloat ( "wait", "-1" ) );
	
	conditionDead	= spawnArgs.GetBool ( "condition_dead", "0" );
	conditionTether = spawnArgs.GetBool ( "condition_tether", "0" );
	conditionStop	= spawnArgs.GetBool ( "condition_stop", "0" );
	
	percent			= spawnArgs.GetFloat ( "percent", "1" );
	
	// Start on by default?
	nextTriggerTime = spawnArgs.GetBool ( "start_on", "0" ) ? 0 : -1;
	if ( nextTriggerTime == 0 ) {
		BecomeActive ( TH_THINK );
	} else {
		BecomeInactive ( TH_THINK );
	}

	// If there are no conditions we are done
	if ( !conditionDead && !conditionTether && !conditionStop ) {
		gameLocal.Warning ( "No conditions specified on ai trigger entity '%s'", GetName ( ) );
		PostEventMS ( &EV_Remove, 0 );
	}
}

/*
================
rvAITrigger::Save
================
*/
void rvAITrigger::Save ( idSaveGame *savefile ) const {
	int i;
	savefile->WriteInt ( testAI.Num ( ) );
	for ( i = 0; i < testAI.Num(); i ++ ) {
		testAI[i].Save ( savefile );
	}

	savefile->WriteInt ( testSpawner.Num ( ) );
	for ( i = 0; i < testSpawner.Num(); i ++ ) {
		testSpawner[i].Save ( savefile );
	}
	
	savefile->WriteBool ( conditionDead );
	savefile->WriteBool ( conditionTether );
	savefile->WriteBool ( conditionStop );

	savefile->WriteInt ( wait );
	savefile->WriteInt ( nextTriggerTime );

	savefile->WriteFloat ( percent );
}

/*
================
rvAITrigger::Restore
================
*/
void rvAITrigger::Restore ( idRestoreGame *savefile ) {
	int i;
	int num;

	savefile->ReadInt ( num );
	testAI.Clear ( );
	testAI.SetNum ( num );
	for ( i = 0; i < num; i ++ ) {
		testAI[i].Restore ( savefile );
	}

	savefile->ReadInt ( num );
	testSpawner.Clear ( );
	testSpawner.SetNum ( num );
	for ( i = 0; i < num; i ++ ) {
		testSpawner[i].Restore ( savefile );
	}

	savefile->ReadBool ( conditionDead );
	savefile->ReadBool ( conditionTether );
	savefile->ReadBool ( conditionStop );

	savefile->ReadInt ( wait );
	savefile->ReadInt ( nextTriggerTime );

	savefile->ReadFloat ( percent );
}

/*
================
rvAITrigger::Think
================
*/
void rvAITrigger::Think ( void ) {
	int		v;

	// Only trigger so often		
	if ( nextTriggerTime == -1 || gameLocal.time < nextTriggerTime ) {
		return;
	}

	// If we have any attached spawners then our condition cannot be met
	if ( testSpawner.Num() ) {
		for ( v = 0; v < testSpawner.Num(); v ++ ) {
			rvSpawner* spawner = testSpawner[v];
			if ( !spawner ) {
				testSpawner.RemoveIndex ( v );
				v--;
				continue;
			}
			return;
		}
	}

	// If we have no AI we are tracking then wait until we do
	if ( !testAI.Num ( ) ) {
		return;
	}
	
	int count = 0;
	for ( v = 0; v < testAI.Num(); v ++ ) {
		idAI* ai = testAI[v];
		if ( !ai ) {
			testAI.RemoveIndex ( v );
			v--;
			continue;
		}
		if ( !ai->aifl.dead ) {
			if ( conditionDead ) {
				continue;
			}
			if ( conditionTether && !ai->IsWithinTether ( ) ) {
				continue;
			}
			if ( conditionStop && ai->move.fl.moving ) {
				continue;
			}
		}
		count++;
	}
	
	// If result is true then we should fire our trigger now
	if ( count >= (int) ((float)testAI.Num()*percent) ) {
		ActivateTargets ( this );
		
		// If only triggering once just remove ourselves now
		if ( wait < 0 ) {
			nextTriggerTime = -1;
		} else {
			nextTriggerTime = gameLocal.time + wait;
		}
	}
}

/*
================
rvAITrigger::FindTargets
================
*/
void rvAITrigger::FindTargets ( void ) {
	int t;
	
	idEntity::FindTargets ( );
	
	for ( t = 0; t < targets.Num(); t ++ ) {
		idEntity* ent = targets[t];
		if ( !ent ) {
			continue;
		}
		if ( ent->IsType ( idAI::GetClassType ( ) ) ) {
			testAI.Append ( idEntityPtr<idAI>(static_cast<idAI*>(ent)) );
			targets.RemoveIndex ( t );
			t--;
			continue;
		}
		if ( ent->IsType ( rvSpawner::GetClassType ( ) ) ) {
			static_cast<rvSpawner*>(ent)->AddCallback ( this, &AI_AppendFromSpawner );
			testSpawner.Append ( idEntityPtr<rvSpawner>(static_cast<rvSpawner*>(ent)) );
			targets.RemoveIndex ( t );
			t--;
			continue;
		}
	}		
}

/*
================
rvAITrigger::Event_Activate
================
*/
void rvAITrigger::Event_Activate( idEntity *activator ) {
	
	// Add spawners and ai to the list when they come in
	if ( activator && activator->IsType ( idAI::GetClassType ( ) ) ) {
		testAI.Append ( idEntityPtr<idAI>(static_cast<idAI*>(activator)) );	
		return;
	}

	if ( nextTriggerTime == -1 ) {
		nextTriggerTime = 0;
		BecomeActive ( TH_THINK );
	} else {
		nextTriggerTime = -1;
		BecomeInactive ( TH_THINK );
	}
}

/*
================
rvAITrigger::Event_AppendFromSpawner
================
*/
void rvAITrigger::Event_AppendFromSpawner ( rvSpawner* spawner, idEntity* spawned ) {
	// If its an ai entity being spawned then add it to our test list
	if ( spawned && spawned->IsType ( idAI::GetClassType ( ) ) ) {	
		testAI.Append ( idEntityPtr<idAI>(static_cast<idAI*>(spawned)) );	
	}
}

/*
===============================================================================

								rvAITether

===============================================================================
*/
const idEventDef EV_TetherSetupLocation ( "tetherSetupLocation" );
const idEventDef EV_TetherGetLocation ( "tetherGetLocation" );
CLASS_DECLARATION( idEntity, rvAITether )
	EVENT( EV_Activate,					rvAITether::Event_Activate )
	EVENT( EV_TetherGetLocation,		rvAITether::Event_TetherGetLocation )
	EVENT( EV_TetherSetupLocation,		rvAITether::Event_TetherSetupLocation )
END_CLASS

/*
================
rvAITether::rvAITether
================
*/
rvAITether::rvAITether ( void ) {
}

/*
================
rvAITether::InitNonPersistentSpawnArgs
================
*/
void rvAITether::InitNonPersistentSpawnArgs ( void ) {
	tfl.canBreak   		 = spawnArgs.GetBool ( "allowBreak", "1" );
	tfl.autoBreak		 = spawnArgs.GetBool ( "autoBreak", "0" );
	tfl.forceRun   		 = spawnArgs.GetBool ( "forceRun", "0" );
	tfl.forceWalk  		 = spawnArgs.GetBool ( "forceWalk", "0" );
	tfl.becomeAggressive = spawnArgs.GetBool ( "becomeAggressive", "0" );
	tfl.becomePassive    = spawnArgs.GetBool ( "becomePassive",    "0" );

	// Check for both being set
	if ( tfl.forceRun && tfl.forceWalk ) {
		gameLocal.Warning ( "both forceRun and forceWalk were specified for tether '%s', forceRun will take precedence" );
		tfl.forceWalk = false;
	}
	if ( tfl.becomeAggressive && tfl.becomePassive ) {
		gameLocal.Warning ( "both becomePassive and becomeAggressive were specified for tether '%s', becomeAggressive will take precedence" );
		tfl.becomePassive = false;
	}
}

/*
================
rvAITether::Spawn
================
*/
void rvAITether::Spawn ( void ) {
	InitNonPersistentSpawnArgs ( );

	PostEventMS( &EV_TetherSetupLocation, 100 );
}

/*
================
rvAITether::Event_TetherSetupLocation
================
*/
void rvAITether::Event_TetherSetupLocation( void ) {
	//NOTE: we now do this right after spawn so we don't stomp other tether's locations
	//		if we activate after them and are in the same room as them.
	//		Dynamically-spawned locations are very, very bad!
	
	// All pre-existing locations should be placed and spread by now
	// Get the location entity we are attached to
	if ( spawnArgs.GetBool ( "location", "1" ) ) {
		location = gameLocal.LocationForPoint ( GetPhysics()->GetOrigin() - GetPhysics()->GetGravityNormal() * 32.0f );
		if ( !location ) {
			location = gameLocal.AddLocation ( GetPhysics()->GetOrigin() - GetPhysics()->GetGravityNormal() * 32.0f, "tether_location" );
		}
	} else {
		location = NULL;
	}
	PostEventMS( &EV_TetherGetLocation, 100 );
}

/*
================
rvAITether::Event_TetherGetLocation
================
*/
void rvAITether::Event_TetherGetLocation( void ) {
	//NOW: all locations should be made & spread, get our location (may not be the one
	//		we added, it could be be the same as another tether if it's in the same room as us)
	if ( spawnArgs.GetBool ( "location", "1" ) ) {
		location = gameLocal.LocationForPoint ( GetPhysics()->GetOrigin() - GetPhysics()->GetGravityNormal() * 32.0f );
	} else {
		location = NULL;
	}
}

/*
================
rvAITether::Save
================
*/
void rvAITether::Save ( idSaveGame *savefile ) const {
	location.Save( savefile );
}

/*
================
rvAITether::Restore
================
*/
void rvAITether::Restore ( idRestoreGame *savefile ) {
	location.Restore( savefile );
	
	InitNonPersistentSpawnArgs ( );
}

/*
================
rvAITether::ValidateAAS
================
*/
bool rvAITether::ValidateAAS ( idAI* ai ) {
	if ( !ai->aas ) {
		return false;
	}
	return true;
}

/*
================
rvAITether::ValidateDestination
================
*/
bool rvAITether::ValidateDestination ( idAI* ai, const idVec3& dest ) {
	if ( location ) {
		if ( gameLocal.LocationForPoint ( dest - ai->GetPhysics()->GetGravityNormal() * 32.0f ) != location ) {
			return false;
		}
	}
	return true;
}

/*
================
rvAITether::ValidateBounds
================
*/
bool rvAITether::ValidateBounds	( const idBounds& bounds ) {
	return true;
}

/*
================
rvAITether::FindGoal
================
*/
bool rvAITether::FindGoal ( idAI* ai, aasGoal_t& goal ) {
	rvAASFindGoalForTether findGoal ( ai, this );
	if ( !ai->aas->FindNearestGoal( goal, 
								ai->PointReachableAreaNum( ai->GetPhysics()->GetOrigin() ), 
								ai->GetPhysics()->GetOrigin(), 
								GetPhysics()->GetOrigin(), 
								ai->move.travelFlags, 
								0.0f, 0.0f,
								NULL, 0, findGoal ) ) {
		return false;
	}
	return true;
}


/*
================
rvAITether::Event_Activate
================
*/
void rvAITether::Event_Activate( idEntity *activator ) {
	int i;

	//WELL!  Turns out designers are binding tethers to movers, so we have to get our location *again* on activation...
	if ( spawnArgs.GetBool ( "location", "1" ) ) {
		location = gameLocal.LocationForPoint ( GetPhysics()->GetOrigin() - GetPhysics()->GetGravityNormal() * 32.0f );
	}

	if ( activator && activator->IsType ( idAI::GetClassType() ) ) {
		activator->ProcessEvent ( &EV_Activate, this );
	}
	
	// All targetted AI will be activated with the tether AI entity
	for ( i = 0; i < targets.Num(); i ++ ) {
		if ( !targets[i] ) {
			continue;
		}
		if ( targets[i]->IsType ( idAI::GetClassType() ) ) {
			targets[i]->ProcessEvent ( &EV_Activate, this );			
			
			// Aggressive/Passive stance change?
			if ( tfl.becomeAggressive ) {
				targets[i]->ProcessEvent ( &AI_BecomeAggressive );
		 	} else if ( tfl.becomePassive ) {
				targets[i]->ProcessEvent ( &AI_BecomePassive, false );
			}
		}
	}			
}

/*
================
rvAITether::DebugDraw
================
*/
void rvAITether::DebugDraw ( void ) {
	const idBounds& bounds = GetPhysics()->GetAbsBounds();
	gameRenderWorld->DebugBounds ( colorYellow, bounds.Expand ( 8.0f ) );
	gameRenderWorld->DebugArrow ( colorWhite, bounds.GetCenter(), bounds.GetCenter() + GetPhysics()->GetAxis()[0] * 16.0f, 8.0f );
	gameRenderWorld->DrawText( name.c_str(), bounds.GetCenter(), 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	gameRenderWorld->DrawText( va( "#%d", entityNumber ), bounds.GetCenter() - GetPhysics()->GetGravityNormal() * 5.0f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );	
}

/*
===============================================================================

								rvAITetherBehind

===============================================================================
*/

CLASS_DECLARATION( rvAITether, rvAITetherBehind )
END_CLASS

/*
================
rvAITetherBehind::InitNonPersistentSpawnArgs
================
*/
void rvAITetherBehind::InitNonPersistentSpawnArgs ( void ) {
	range = spawnArgs.GetFloat ( "range" );
}

/*
================
rvAITetherBehind::Spawn
================
*/
void rvAITetherBehind::Spawn ( void ) {
	InitNonPersistentSpawnArgs ( );
}

/*
================
rvAITetherBehind::Restore
================
*/
void rvAITetherBehind::Restore ( idRestoreGame* savefile ) {
	InitNonPersistentSpawnArgs ( );
}

/*
================
rvAITetherBehind::ValidateDestination
================
*/
bool rvAITetherBehind::ValidateDestination ( idAI* ai, const idVec3& dest ) {
	// Check base tether first
	if ( !rvAITether::ValidateDestination ( ai, dest ) ) {
		return false;
	}
	
	// Make sure we include the move range in the tether
	idVec3 origin;
	if ( range ) {
		origin = GetPhysics()->GetOrigin ( ) + GetPhysics()->GetAxis()[0] * GetOriginReachedRange() - GetPhysics()->GetAxis()[0] * range;
		if ( GetPhysics()->GetAxis()[0] * (origin-dest) > 0.0f ) {
			return false;
		}
	}			

	origin = GetPhysics()->GetOrigin ( ) - GetPhysics()->GetAxis()[0] * GetOriginReachedRange(); 			
	
	// Are we on wrong side of tether?
	return ( GetPhysics()->GetAxis()[0] * (origin-dest) ) >= 0.0f;
}

/*
================
rvAITetherBehind::ValidateBounds
================
*/
bool rvAITetherBehind::ValidateBounds ( const idBounds& bounds ) {
	if ( !rvAITether::ValidateBounds ( bounds ) ) {
		return false;
	}

	idPlane plane;
	int     side;
	plane.SetNormal ( GetPhysics()->GetAxis ( )[0] );
	plane.FitThroughPoint ( GetPhysics()->GetOrigin ( ) );
	side = bounds.PlaneSide ( plane );	
	return ( side == PLANESIDE_CROSS || side == PLANESIDE_BACK );
}

/*
================
rvAITetherBehind::DebugDraw
================
*/
void rvAITetherBehind::DebugDraw ( void ) {
	idVec3 dir;
	
	rvAITether::DebugDraw ( );	
	
	dir = GetPhysics()->GetGravityNormal ( ).Cross ( GetPhysics()->GetAxis()[0] );
	gameRenderWorld->DebugLine ( colorPink, 
								 GetPhysics()->GetOrigin() - dir * 1024.0f,
								 GetPhysics()->GetOrigin() + dir * 1024.0f );

	if ( range ) {
		idVec3 origin;
		origin = GetPhysics()->GetOrigin ( ) - GetPhysics()->GetAxis ( )[0] * range;
		gameRenderWorld->DebugArrow ( colorPink, GetPhysics()->GetOrigin(), origin, 5.0f );
		gameRenderWorld->DebugLine ( colorPink, 
									 origin - dir * 1024.0f,
									 origin + dir * 1024.0f );
	}	
}

/*
===============================================================================

								rvAITetherRadius

===============================================================================
*/

CLASS_DECLARATION( rvAITether, rvAITetherRadius )
END_CLASS

/*
================
rvAITetherRadius::InitNonPersistentSpawnArgs
================
*/
void rvAITetherRadius::InitNonPersistentSpawnArgs ( void ) {
	float radius;
	if ( !spawnArgs.GetFloat ( "tetherRadius", "0", radius ) ) {	
		radius = spawnArgs.GetFloat ( "radius", "128" );
	}
	radiusSqr = Square ( radius );
}

/*
================
rvAITetherRadius::Spawn
================
*/
void rvAITetherRadius::Spawn ( void ) {
	InitNonPersistentSpawnArgs ( );
}

/*
================
rvAITetherRadius::Restore
================
*/
void rvAITetherRadius::Restore ( idRestoreGame* savefile ) {
	InitNonPersistentSpawnArgs ( );
}

/*
================
rvAITetherRadius::ValidateDestination
================
*/
bool rvAITetherRadius::ValidateDestination ( idAI* ai, const idVec3& dest ) {
	// Check base tether first
	if ( !rvAITether::ValidateDestination ( ai, dest ) ) {
		return false;
	}
	// Are we within tether radius?
	return ((dest - GetPhysics()->GetOrigin()).LengthSqr ( ) < radiusSqr - Square ( ai->move.range ) );
}


/*
================
rvAITetherRadius::ValidateBounds
================
*/
bool rvAITetherRadius::ValidateBounds ( const idBounds& bounds ) {
	if ( !rvAITether::ValidateBounds ( bounds ) ) {
		return false;
	}
	return ( Square ( bounds.ShortestDistance ( GetPhysics()->GetOrigin ( ) ) ) < radiusSqr );
}

/*
================
rvAITetherRadius::DebugDraw
================
*/
void rvAITetherRadius::DebugDraw ( void ) {
	rvAITether::DebugDraw ( );
	gameRenderWorld->DebugCircle( colorPink, GetPhysics()->GetOrigin(), GetPhysics()->GetGravityNormal(), idMath::Sqrt(radiusSqr), 25 );
}


/*
===============================================================================

								rvAITetherClear

===============================================================================
*/

CLASS_DECLARATION( rvAITether, rvAITetherClear )
END_CLASS


/*
===============================================================================

								rvAIBecomePassive

===============================================================================
*/

CLASS_DECLARATION( idEntity, rvAIBecomePassive )
	EVENT( EV_Activate,	rvAIBecomePassive::Event_Activate )
END_CLASS

/*
================
rvAIBecomePassive::Event_Activate
================
*/
void rvAIBecomePassive::Event_Activate( idEntity *activator ) {
	int		i;
	bool	ignoreEnemies;
	
	ignoreEnemies = spawnArgs.GetBool ( "ignoreEnemies", "1" );
	
	// All targeted AI will become passive
	for ( i = 0; i < targets.Num(); i ++ ) {
		if ( !targets[i] ) {
			continue;
		}
		if ( targets[i]->IsType ( idAI::GetClassType() ) ) {
			targets[i]->ProcessEvent ( &AI_BecomePassive, ignoreEnemies );
		}
	}			
}

/*
===============================================================================

								rvAIBecomeAggressive

===============================================================================
*/

CLASS_DECLARATION( idEntity, rvAIBecomeAggressive )
	EVENT( EV_Activate,	rvAIBecomeAggressive::Event_Activate )
END_CLASS

/*
================
rvAIBecomeAggressive::Event_Activate
================
*/
void rvAIBecomeAggressive::Event_Activate( idEntity *activator ) {
	int i;
	
	// All targetted AI will become aggressive
	for ( i = 0; i < targets.Num(); i ++ ) {
		if ( !targets[i] ) {
			continue;
		}
		if ( targets[i]->IsType ( idAI::GetClassType() ) ) {
			targets[i]->ProcessEvent ( &AI_BecomeAggressive );
		}
	}			
}
