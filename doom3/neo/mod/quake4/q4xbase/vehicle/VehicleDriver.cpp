
// VehicleDriver.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

#include <cfloat>

const idEventDef VD_ChoosePathTarget( "choosePathTarget", "e", 'e' );
const idEventDef EV_FollowOffset( "followOffset",	"v"		);
const idEventDef EV_FireWeapon	( "fireWeapon",		"ff"	);

CLASS_DECLARATION( idActor, rvVehicleDriver )
	EVENT( EV_PostSpawn			, rvVehicleDriver::Event_PostSpawn )
	EVENT( AI_EnterVehicle		, rvVehicleDriver::Event_EnterVehicle )
	EVENT( AI_ExitVehicle		, rvVehicleDriver::Event_ExitVehicle )
	EVENT( AI_ScriptedMove		, rvVehicleDriver::Event_ScriptedMove )
	EVENT( AI_ScriptedDone		, rvVehicleDriver::Event_ScriptedDone )
	EVENT( AI_ScriptedStop		, rvVehicleDriver::Event_ScriptedStop )
	EVENT( EV_Activate			, rvVehicleDriver::Event_Trigger )
	EVENT( EV_Speed				, rvVehicleDriver::Event_SetSpeed )
	EVENT( EV_FireWeapon		, rvVehicleDriver::Event_FireWeapon )
	EVENT( AI_FaceEntity		, rvVehicleDriver::Event_FaceEntity )
	EVENT( AI_LookAt			, rvVehicleDriver::Event_LookAt )
	EVENT( AI_SetLeader			, rvVehicleDriver::Event_SetLeader )

//twhitaker: remove - begin
	EVENT( EV_FollowOffset		, rvVehicleDriver::Event_SetFollowOffset )
//twhitaker: remove - end
END_CLASS								 

/*
================
rvVehicleDriver::rvVehicleDriver 
================
*/
rvVehicleDriver::rvVehicleDriver ( void ) {
}

/*
================
rvVehicleDriver::Spawn
================
*/
void rvVehicleDriver::Spawn ( void ) {
	currentThrottle	= 1.0f;
	faceTarget		= NULL;
	lookTarget		= NULL;
	leader			= NULL;
	leaderFlags		= 0;
	decelDistance	= 0.0f;
	minDistance		= 0.0f;
	fireEndTime		= 0.0f;
	isMoving		= false;
	avoidingLeader	= false;
	pathingMode		= VDPM_Random;
	pathingOrigin	= vec3_origin;
	pathingEntity	= NULL;

	SIMDProcessor->Memset( &pathTargetInfo,		0, sizeof( PathTargetInfo ) );
	SIMDProcessor->Memset( &lastPathTargetInfo,	0, sizeof( PathTargetInfo ) );

	PostEventMS( &EV_PostSpawn, 0 );
}

/*
================
rvVehicleDriver::Think
================
*/
void rvVehicleDriver::Think ( void ) {
	if ( !IsDriving() ) {
		if ( leader ) {
			leader->SetLeaderHint( VDLH_Continue );
			leader = NULL;
		}
		return;
	}

	if ( pathTargetInfo.node ) {
		float		targetDistance		= 0.0f;
		float		dotForward			= 0.0f;
		float		dotRight			= 0.0f;
		float		desiredThrottle		= 1.0f;
		idEntity*	currentPathTarget	= pathTargetInfo.node;
		rvVehicle*	vehicle				= vehicleController.GetVehicle();
		idMat3		vehicleAxis			= vehicle->GetAxis();
		idVec3		targetOrigin;
		idVec3		dirToTarget;
		usercmd_t	cmd;
		idAngles	ang;

		// We may want to hack the auto correction variable based on what the state of the driver and the driver's leader.
		UpdateAutoCorrection();

		if ( vehicle->IsAutoCorrecting( ) ) {
			if ( lastPathTargetInfo.node ) {
				currentPathTarget = lastPathTargetInfo.node;
			}

			if ( g_debugVehicleDriver.GetInteger() != 0 ) {
				gameRenderWorld->DebugBounds( colorCyan, vehicle->GetPhysics()->GetAbsBounds().Expand( 30 ) );
			}
		}

		// Touch all the triggers that the vehicle touches
		vehicle->TouchTriggers();

		GetTargetInfo( currentPathTarget, &targetOrigin, &dirToTarget, &dotForward, &dotRight, &targetDistance );

		// The primary purpose of the following portions of code is to set the desiredThrottle variable.
		if ( IsMoving() ) {

			// if we're slowing down lerp throttle.
			if ( pathTargetInfo.throttle < lastPathTargetInfo.throttle ) {
				desiredThrottle			= idMath::Lerp( lastPathTargetInfo.throttle, pathTargetInfo.throttle, targetDistance / pathTargetInfo.initialDistance );
			} else {
				// otherwise accelerate rapidly or maintain throttle.
				desiredThrottle			= pathTargetInfo.throttle;
			}

			// we could potentially be a leader, so check the leader flags
			if ( leaderFlags & VDLH_SlowDown ) {
				desiredThrottle = 0.1f;
			} else if ( leaderFlags & VDLH_Wait ) {
				desiredThrottle = 0.0f;
			}

			// if we're following a someone ...
			if ( leader ) {
				bool	canSeeLeader		= vehicle->CanSee( leader->vehicleController.GetVehicle(), false );
				float	distanceToLeader	= ( leader->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() ).Length();
				avoidingLeader				= false;

				// update the leaders flags based on distance and visibility
				if ( !canSeeLeader ) {
					// if we can't see the leader, tell him to stop
					leader->SetLeaderHint( VDLH_Wait );

				} else if ( distanceToLeader > decelDistance ) {
					// if we're too far from the leader, tell him to slow down
 					leader->SetLeaderHint( VDLH_SlowDown );

				} else {
					// we're within range of the leader so let him move along at a normal rate
					leader->SetLeaderHint( VDLH_Continue );

					// if we're too close to the leader, we need to slow ourself down
					if ( distanceToLeader < minDistance ) {
						desiredThrottle	= -0.2f;
						avoidingLeader	= true;
					}
				}
			}

			// the desiredThrottle variable should be set at this point, the only other thing that could change it
			// would be in SimulateKeys, if the dotForward < 0.
		}

		vehicleController.GetInput( cmd, ang );

		// Simulate Input
		SimulateKeys( cmd, dotForward, dotRight, desiredThrottle * currentThrottle, targetDistance );
		SimulateMouseMove( cmd );
		SimulateButtons( cmd );

		vehicleController.SetInput( cmd, ang );
		
		// Node Transition
		if( (!vehicle->IsAutoCorrecting() || !IsValidPathNode( currentPathTarget )) && isMoving ) {
			idVec3 point = targetOrigin - dirToTarget * pathTargetInfo.minDistance;

			if( vehicle->GetPhysics()->GetAbsBounds().ContainsPoint( point ) ) {
				int numTargets			= NumValidTargets( currentPathTarget );
				lastPathTargetInfo		= pathTargetInfo;
				//TODO: ponder - should I be setting lastPathTargetInfo.node to pathTargetInfo.node or currentPathTarget???

				if( numTargets ) {
					Event_ScriptedMove( ChooseNextNode( currentPathTarget ), 0, 0 );
				}

				if( lastPathTargetInfo.exitVehicle ) {
					Event_ExitVehicle( true );
				} else if( lastPathTargetInfo.throttle == 0 || !numTargets ) { 
					Event_ScriptedStop();

					if( !numTargets ) {
						pathTargetInfo.node = NULL;
					}
				}

				// Debug Output
				if ( g_debugVehicleDriver.GetInteger( ) != 0 ) {
					gameRenderWorld->DebugBounds( colorRed, idBounds(idVec3(-5, -5, -5), idVec3(5, 5, 5)), point );
				}
			}
		}
	} else {	// no path
		float		dotForward;
		float		dotRight;
		usercmd_t	cmd;
		idAngles	ang;

		vehicleController.GetInput( cmd, ang );

		if ( GetTargetInfo( faceTarget, NULL, NULL, &dotForward, &dotRight, NULL) ) {
			if( ( 1.0f - dotForward ) < VECTOR_EPSILON ) {
				Event_ScriptedStop();
			} else {
				SimulateKeys( cmd, dotForward, dotRight, 0.0f );
			}
		}

		SimulateMouseMove( cmd );
		SimulateButtons( cmd );

		vehicleController.SetInput( cmd, ang );
	}
}

/*
================
rvVehicleDriver::GetTargetInfo
================
*/
bool rvVehicleDriver::GetTargetInfo( const idEntity* target, idVec3* targetOrigin, idVec3* dirToTarget, float* dotForward, float* dotRight, float* distance ) const {
	if( !target || !IsDriving() ) {
		return false;
	}

	idMat3 vehicleAxis		= vehicleController.GetVehicle()->GetAxis();
	idVec3 vehicleOrigin	= vehicleController.GetVehicle()->GetOrigin();
	idVec3 forward			= vehicleAxis[ 0 ];
	idVec3 localTargetOrigin = target->GetPhysics()->GetOrigin();
	localTargetOrigin.z		= vehicleOrigin.z;// Find way to include vehicleAxis here
	idVec3 vectorToTarget	= localTargetOrigin - vehicleOrigin;
	idVec3 localDirToTarget	= vectorToTarget;
	float  distToTarget		= localDirToTarget.Normalize();

	if( targetOrigin ) {
		*targetOrigin = localTargetOrigin;
	}

	if( dirToTarget ) {
		*dirToTarget = localDirToTarget;
	}
		
	if( distance ) {
		*distance	= distToTarget;
	}

	if( dotForward ) {
		*dotForward	= localDirToTarget * forward;
	}

	if( dotRight ) {
		*dotRight	= localDirToTarget * vehicleAxis[ 1 ];
	}

	// Debug help
	if ( g_debugVehicleDriver.GetInteger( ) != 0 ) {
		idStr temp;
		const idVec3 & origin = GetPhysics()->GetOrigin();
		gameRenderWorld->DebugLine( colorBlue, origin, target->GetPhysics()->GetOrigin() );
		gameRenderWorld->DebugBounds( colorBlue, GetPhysics()->GetAbsBounds() );
		gameRenderWorld->DebugBounds( colorBlue, target->GetPhysics()->GetBounds(), target->GetPhysics()->GetOrigin() );
		gameRenderWorld->DrawText( target->GetName(), vehicleOrigin + vectorToTarget + vehicleAxis[2] * 5.0f, 1.0f, colorBlue, gameLocal.GetLocalPlayer()->viewAxis );
		gameRenderWorld->DrawText( idStr( "State: " ) + LeaderHintsString( leaderFlags, temp ), origin, 1.0f, colorBlue, gameLocal.GetLocalPlayer()->viewAxis );

		if ( vehicleController.GetVehicle()->IsAutoCorrecting() ) {
			gameRenderWorld->DebugBounds( colorPurple, vehicleController.GetVehicle()->GetPhysics()->GetAbsBounds() );
			gameRenderWorld->DrawText( "Auto-Correcting", vehicleOrigin, 1.0f, colorPurple, gameLocal.GetLocalPlayer()->viewAxis );
		}

		if ( pathTargetInfo.node && g_debugVehicleDriver.GetInteger( ) == 2 ) {
			gameRenderWorld->DebugBounds( colorOrange, pathTargetInfo.node->GetPhysics()->GetBounds() );

			for ( int ix = 0; ix < pathTargetInfo.node->targets.Num(); ix++ ) {
				gameRenderWorld->DebugLine( colorOrange, pathTargetInfo.node->GetPhysics()->GetOrigin(), pathTargetInfo.node->targets[ ix ]->GetPhysics()->GetOrigin(), 0, true );
			}
		}
		if ( leader ) {
			idVec4 & color = colorGreen;

			if ( leader->GetLeaderHint() & VDLH_SlowDown ) {
				color = colorYellow;
			} else if ( leader->GetLeaderHint() & VDLH_Wait ) {
				color = colorRed;
			}

			idStr str = idStr( "decel_distance: " ) + idStr( decelDistance ) + idStr( "\nmin_distance: " ) + idStr( minDistance );

			gameRenderWorld->DrawText( str, leader->GetPhysics()->GetOrigin(), 1.0f, color, gameLocal.GetLocalPlayer()->viewAxis, 1, 0, true );
			gameRenderWorld->DebugLine( color, GetPhysics()->GetOrigin(), leader->GetPhysics()->GetOrigin(), 0, true );
			gameRenderWorld->DebugBounds( color, leader->vehicleController.GetVehicle()->GetPhysics()->GetAbsBounds(), vec3_origin, 0, true );
			gameRenderWorld->DebugCircle( color, leader->GetPhysics()->GetOrigin(), idVec3( 0, 0, 1 ), decelDistance, 10, 0, true );
			gameRenderWorld->DebugCircle( color, leader->GetPhysics()->GetOrigin(), idVec3( 0, 0, 1 ), minDistance, 10, 0, true );
		}
	}

	return true;
}

/*
================
rvVehicleDriver::ChooseNextNode
================
*/
idEntity* rvVehicleDriver::ChooseNextNode( idEntity* target ) {
	if( !target ) {
		return NULL;
	}

 	if( func.Init( target->spawnArgs.GetString( "call_nextNode" )) <= 0 ) {
		idEntity * best	= NULL;
		float bestDist;

		switch ( pathingMode ) {
		case VDPM_MoveTo:
			bestDist	= FLT_MAX;

			for ( int i = target->targets.Num() - 1; i; i -- ) {
				float distance = ( target->targets[ i ]->GetPhysics()->GetOrigin() - pathingOrigin ).LengthSqr();

				if ( bestDist > distance ) {
					bestDist = distance;
					best = target->targets[ i ];
				}
			}

			break;

		case VDPM_MoveAway:
			bestDist = FLT_MIN;
		
			for ( int i = target->targets.Num() - 1; i; i -- ) {
				float distance = ( target->targets[ i ]->GetPhysics()->GetOrigin() - pathingOrigin ).LengthSqr();

				if ( bestDist < distance ) {
					bestDist = distance;
					best = target->targets[ i ];
				}
			}

		case VDPM_Custom:
			if ( pathingEntity ) {
//				best = pathingCallback( target );
				pathingEntity->ProcessEvent( &VD_ChoosePathTarget, target );
				best = gameLocal.program.GetReturnedEntity();
			}
		}

		return ( best ) ? best : RandomValidTarget( target );
	}

	func.InsertEntity( this, 0 );
	func.CallFunc( &spawnArgs );
	func.RemoveIndex( 0 );

	return ( func.ReturnsAVal()) ? gameLocal.FindEntity( spawnArgs.GetString( func.GetReturnKey() )) : const_cast<idEntity*>( target );
}

/*
================
rvVehicleDriver::SimulateButtons
================
*/
void rvVehicleDriver::SimulateButtons( usercmd_t& cmd ) {
	cmd.buttons	= (fireEndTime >= gameLocal.time) ? BUTTON_ATTACK : 0;
}

/*
================
rvVehicleDriver::SimulateMouseMove
================
*/
void rvVehicleDriver::SimulateMouseMove( usercmd_t& cmd ) {
	idVec3 origin;
	idMat3 axis;

	idVec3 vectorToTarget;
	idAngles anglesToTarget;
	idAngles turretAngles;
	idAngles deltaAngles; 

	if( !lookTarget ) {
		for( int ix = 0; ix < 3; ++ix ) {
			cmd.angles[ix] = 0;
		}
		return;
	}

	vehicleController.GetEyePosition( origin, axis );
	vectorToTarget	= (lookTarget->GetPhysics()->GetOrigin() - origin).ToNormal();
	anglesToTarget	= vectorToTarget.ToAngles().Normalize360();
	turretAngles	= (axis[0]).ToAngles().Normalize360();
	deltaAngles		= (anglesToTarget - turretAngles).Normalize180();

	for( int ix = 0; ix < deltaAngles.GetDimension(); ++ix ) {
		cmd.angles[ix] += ANGLE2SHORT( deltaAngles[ix] );
	}

	// Debug Output
	if( g_debugVehicleDriver.GetInteger( ) != 0 ) {
		gameRenderWorld->DebugLine( colorGreen, origin, origin + anglesToTarget.ToForward() * 100.0f, 17, true );
		gameRenderWorld->DebugLine( colorYellow, origin, origin + turretAngles.ToForward() * 100.0f, 17, true );
	}
}

/*
================
rvVehicleDriver::SimulateKeys
================
*/
void rvVehicleDriver::SimulateKeys( usercmd_t& cmd, float dotForward, float dotRight, float speed, float distance ) {
	rvVehicle * vehicle = vehicleController.GetVehicle();

	if( !vehicle ) {
		cmd.forwardmove	= 0;
		cmd.rightmove	= 0;
		return;
	}
	
	cmd.forwardmove	= static_cast< signed char >( (!vehicle->IsAutoCorrecting() ? 127.0f : forwardThrustScale) * dotForward * speed );
	cmd.rightmove	= static_cast< signed char >( ( ( dotForward < 0.0f ) ? rightThrustScale : -rightThrustScale ) * dotRight );
}


/*
================
rvVehicleDriver::SortValid
================
*/
int rvVehicleDriver::SortValid( const void* a, const void* b ) {
	idEntityPtr<idEntity>	A = *(idEntityPtr<idEntity>*)a;
	idEntityPtr<idEntity>	B = *(idEntityPtr<idEntity>*)b;

	return rvVehicleDriver::IsValidTarget( B ) - rvVehicleDriver::IsValidTarget( A );
}

/*
================
rvVehicleDriver::SortTargetList
================
*/
int	rvVehicleDriver::SortTargetList( idEntity* ent ) const {
	if( !ent ) {
		return 0;
	}

	int			numValidEntities = 0;
	idEntity *	target;

	ent->RemoveNullTargets();
	qsort( ent->targets.Ptr(), NumTargets( ent ), ent->targets.TypeSize(), rvVehicleDriver::SortValid );

	for( int ix = NumTargets( ent ) - 1; ix >= 0; --ix ) {
		target = GetTarget( ent, ix );

		if( IsValidTarget( target ) ) {
			++numValidEntities;
		}
	}

	return numValidEntities;
}

/*
================
rvVehicleDriver::RandomValidTarget
================
*/
idEntity* rvVehicleDriver::RandomValidTarget( idEntity* ent ) const {
	int numValid = NumValidTargets( ent );
	return (!numValid) ? NULL : ent->targets[ rvRandom::irand(0, numValid - 1) ];
}

/*
================
rvVehicleDriver::NumValidTargets
================
*/
int	rvVehicleDriver::NumValidTargets( idEntity* ent ) const {
	return SortTargetList(ent);
}

/*
================
rvVehicleDriver::NumTargets
================
*/
int	rvVehicleDriver::NumTargets( const idEntity* ent ) const {
	return (ent) ? ent->targets.Num() : 0;
}

/*
================
rvVehicleDriver::GetTarget
================
*/
idEntity* rvVehicleDriver::GetTarget( const idEntity* ent, int index ) const {
	return (ent) ? ent->targets[index] : NULL;
}

/*
================
rvVehicleDriver::IsValidPathNode
================
*/
bool rvVehicleDriver::IsValidPathNode( const idEntity* ent ) {
	if( !ent ) {
		return false;
	}

	return ent->IsType( idTarget::GetClassType() );
}

/*
================
rvVehicleDriver::IsValidTarget
================
*/
bool rvVehicleDriver::IsValidTarget( const idEntity* ent ) {
	return IsValidPathNode( ent ) || ent->IsType( rvVehicle::GetClassType() ) || ent->IsType( idPlayer::GetClassType() );
}

/*
================
rvVehicleDriver::SetLeader
================
*/
bool rvVehicleDriver::SetLeader( idEntity* ent ) {
	if ( !ent || !ent->IsType( rvVehicleDriver::GetClassType() ) ) {
		return false;
	}

	leader			= static_cast<rvVehicleDriver*>( ent );
	minDistance		= leader->spawnArgs.GetFloat( "min_distance", "500" );
	idStr decel_distance( minDistance * 3.0f );
	decelDistance	= leader->spawnArgs.GetFloat( "decel_distance", decel_distance );

	return true;
}

/*
================
rvVehicleDriver::Event_PostSpawn 
================
*/
void rvVehicleDriver::Event_PostSpawn ( void ) {
	for ( int i = targets.Num() - 1; i >= 0; i-- ) {
		idEntity * ent = targets[ i ].GetEntity();
		if ( ent->IsType( rvVehicle::GetClassType() ) ) {
			Event_EnterVehicle( ent );
		}
	}

	for ( int i = targets.Num() - 1; i >= 0; i-- ) {
		idEntity * ent = targets[ i ].GetEntity();
		if ( ent->IsType( idTarget::GetClassType() ) ) {
			Event_ScriptedMove( ent, 0, 0 );
		}
		if ( ent->IsType( rvVehicleDriver::GetClassType() ) ) {
			SetLeader( ent );
		}
	}
}

/*
================
rvVehicleDriver::Event_EnterVehicle 
================
*/
void rvVehicleDriver::Event_EnterVehicle ( idEntity * vehicle ) {
	if ( vehicle ) {
		forwardThrustScale	= vehicle->spawnArgs.GetFloat( "driver_forward_thrust", ".75" ) * 127.0f;
		rightThrustScale	= vehicle->spawnArgs.GetFloat( "driver_right_thrust", "1" ) * 127.0f;

		EnterVehicle( vehicle );
	}
}

/*
================
rvVehicleDriver::Event_ExitVehicle
================
*/
void rvVehicleDriver::Event_ExitVehicle( bool force ) {
	Event_ScriptedStop();

	if( vehicleController.GetVehicle() && force && !ExitVehicle(force) ) {
		vehicleController.GetVehicle()->RemoveDriver( vehicleController.GetPosition(), force );
	}

	pathTargetInfo.node	= NULL;
	faceTarget			= NULL;
	lookTarget			= NULL;
	leader				= NULL;
}

/*
================
rvVehicleDriver::Event_ScriptedMove
================
*/
void rvVehicleDriver::Event_ScriptedMove( idEntity *target, float minDist, bool exitVehicle ) {
	isMoving	= false;

	if( !target ) {
		return;
	}

	if( IsValidTarget( target ) ) {
		isMoving						= true;
		faceTarget						= NULL;
		pathTargetInfo.node				= target;
		pathTargetInfo.initialDistance	= ( target->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() ).Length();

		if ( !target->IsType( idPlayer::GetClassType() ) ) {
			pathTargetInfo.minDistance		= target->spawnArgs.GetFloat( "min_distance", va("%f", Max(200.0f, minDist)) );
			pathTargetInfo.throttle			= idMath::ClampFloat( 0.0f, 1.0f, target->spawnArgs.GetFloat( "throttle", "1" ) );
			pathTargetInfo.exitVehicle		= target->spawnArgs.GetBool( "exit_vehicle", exitVehicle ? "1" : "0" );

			if( pathTargetInfo.exitVehicle  ) {
				pathTargetInfo.throttle		= 0.0f;
			}
		} else {
			pathTargetInfo.minDistance		= minDist;
			pathTargetInfo.throttle			= 1.0f;
			pathTargetInfo.exitVehicle		= false;
		}

	} else {
		SIMDProcessor->Memset( &pathTargetInfo, 0, sizeof( PathTargetInfo ) );
	}
}

/*
================
rvVehicleDriver::Event_ScriptedDone
================
*/
void rvVehicleDriver::Event_ScriptedDone( void ) {
	idThread::ReturnFloat( !IsMoving() );
}

/*
================
rvVehicleDriver::Event_ScriptedStop
================
*/
void rvVehicleDriver::Event_ScriptedStop( void ) {
	if( IsDriving() ) {
		usercmd_t	cmd				= { 0 };

		vehicleController.SetInput( cmd, ang_zero );

		if( pathTargetInfo.node ) {
 			if( func.Init( pathTargetInfo.node->spawnArgs.GetString( "call_doneMoving" )) > 0 ) {
				func.InsertEntity( this, 0 );
				func.CallFunc( &spawnArgs );
				func.RemoveIndex( 0 );
			}

			pathTargetInfo.node->ActivateTargets(this);
			pathTargetInfo.node	= NULL;
		}

		isMoving			= false;
	}
}

/*
================
rvVehicleDriver::Event_Trigger
================
*/
void rvVehicleDriver::Event_Trigger( idEntity *activator ) {
	if( IsDriving() && pathTargetInfo.node ) {
		isMoving = true;
	}
}

/*
================
rvVehicleDriver::Event_SetSpeed
================
*/
void rvVehicleDriver::Event_SetSpeed( float speed ) {
	currentThrottle = speed;
}

//twhitaker: remove - begin
/*
================
rvVehicleDriver::Event_SetFollowOffset
================
*/
void rvVehicleDriver::Event_SetFollowOffset( const idVec3 &offset ) {
	gameLocal.Warning( "Script Event \"followOffset\" is deprecated, please remove it immediately to avoid errors." );
}
//twhitaker: remove - end

/*
================
rvVehicleDriver::Event_FireWeapon
================
*/
void rvVehicleDriver::Event_FireWeapon( float weapon_index, float time ) {
	if( IsDriving() ) {
		fireEndTime	= gameLocal.GetTime() + SEC2MS( time );
		vehicleController.SelectWeapon( weapon_index );
	}
}

/*
================
rvVehicleDriver::Event_FaceEntity
================
*/
void rvVehicleDriver::Event_FaceEntity( const idEntity* entity ) {
	if( IsMoving() ) {
		return;
	}

	faceTarget	= entity;
	isMoving	= true;
}

/*
================
rvVehicleDriver::Event_LookAt
================
*/
void rvVehicleDriver::Event_LookAt( const idEntity* entity ) {
	lookTarget	= entity;
}

/*
================
rvVehicleDriver::Event_SetLeader
================
*/
void rvVehicleDriver::Event_SetLeader( idEntity* newLeader ) {
	SetLeader( newLeader );
}

/*
================
rvVehicleDriver::UpdateAutoCorrection
================
*/
void rvVehicleDriver::UpdateAutoCorrection ( void ) { 
	if ( IsDriving() ) {
		rvVehicle * vehicle = vehicleController.GetVehicle();

		// Disregard your autocorrection state if we're slowing down to avoid collision with the leader.
		if ( vehicle->IsAutoCorrecting() ) {
			if ( avoidingLeader == true ) {
				vehicle->autoCorrectionBegin = 0;
			}
		}
	}
}

/*
================
rvVehicleDriver::Save
================
*/
void rvVehicleDriver::Save ( idSaveGame *savefile ) const {
	savefile->Write ( &pathTargetInfo, sizeof ( pathTargetInfo ) );
	savefile->Write ( &lastPathTargetInfo, sizeof ( lastPathTargetInfo ) );
	savefile->WriteFloat ( currentThrottle );

	faceTarget.Save ( savefile );
	lookTarget.Save ( savefile );

	leader.Save ( savefile );
	savefile->WriteInt ( leaderFlags );
	savefile->WriteFloat ( decelDistance );
	savefile->WriteFloat ( minDistance );
	savefile->WriteBool ( avoidingLeader );

	savefile->WriteFloat ( fireEndTime );

	func.Save ( savefile );

	savefile->WriteBool ( isMoving );

	savefile->WriteInt ( (int&)pathingMode );
	pathingEntity.Save ( savefile );
	savefile->WriteVec3 ( pathingOrigin );

	savefile->WriteFloat( forwardThrustScale );	// cnicholson: Added unsaved var
	savefile->WriteFloat( rightThrustScale );	// cnicholson: Added unsaved var

}

/*
================
rvVehicleDriver::Restore
================
*/
void rvVehicleDriver::Restore ( idRestoreGame *savefile ) {
	savefile->Read ( &pathTargetInfo, sizeof ( pathTargetInfo ) );
	savefile->Read ( &lastPathTargetInfo, sizeof ( lastPathTargetInfo ) );
	savefile->ReadFloat ( currentThrottle );

	faceTarget.Restore ( savefile );
	lookTarget.Restore ( savefile );

	leader.Restore ( savefile );
	savefile->ReadInt ( leaderFlags );
	savefile->ReadFloat ( decelDistance );
	savefile->ReadFloat ( minDistance );
	savefile->ReadBool ( avoidingLeader );

	savefile->ReadFloat ( fireEndTime );

	func.Restore ( savefile );

	savefile->ReadBool ( isMoving );

	savefile->ReadInt ( (int&)pathingMode );
	pathingEntity.Restore ( savefile );
	savefile->ReadVec3 ( pathingOrigin );

	savefile->ReadFloat( forwardThrustScale );	// cnicholson: Added unrestored var
	savefile->ReadFloat( rightThrustScale );	// cnicholson: Added unrestored var
}
