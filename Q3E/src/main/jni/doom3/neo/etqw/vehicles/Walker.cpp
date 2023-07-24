// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Walker.h"
#include "../ContentMask.h"
#include "../script/Script_Helper.h"
#include "VehicleControl.h"
#include "../../decllib/DeclSurfaceType.h"

/*
===============================================================================

  sdWalker

===============================================================================
*/

const idEventDef EV_GroundPound( "groundPound", '\0', DOC_TEXT( "Performs a radius damage and push event for objects on the ground around the vehicle." ), 3, NULL, "f", "force", "Force to push entities with.", "f", "damageScale", "Scale factor on the applied damage.", "f", "range", "Range around the vehicle to push entities." );

CLASS_DECLARATION( sdTransport_AF, sdWalker )
	EVENT( EV_GroundPound,			sdWalker::Event_GroundPound )
END_CLASS

/*
================
sdWalker::sdWalker
================
*/
sdWalker::sdWalker( void ) {
	viewAxis.Identity();
	lastIKTime = 0;
	lastIKPos.Zero();
	lastForcedUpdateTime = 0;
}

/*
================
sdWalker::~sdWalker
================
*/
sdWalker::~sdWalker( void ) {
	positionManager.EjectAllPlayers();
}

/*
================
sdWalker::UpdateModelTransform
================
*/
void sdWalker::UpdateModelTransform( void ) {
	sdTeleporter* teleportEnt = teleportEntity;
	if ( teleportEnt != NULL ) {
		idPlayer* player = gameLocal.GetLocalPlayer();
		if ( player != NULL && player->GetProxyEntity() == this ) {
			idEntity* viewer = teleportEnt->GetViewEntity();
			if ( viewer ) {
				renderEntity.axis	= viewer->GetPhysics()->GetAxis();
				renderEntity.origin	= viewer->GetPhysics()->GetOrigin();
				return;
			}
		}
	}

	renderEntity.axis	= viewAxis;
	renderEntity.origin	= GetPhysics()->GetOrigin();

	idPlayer* player = gameLocal.GetLocalViewPlayer();
	if ( !player || player->GetProxyEntity() != this ) {
		DoPredictionErrorDecay();
	}
}

/*
================
sdWalker::Think
================
*/
void sdWalker::Think( void ) {
	physicsObj.SetStability( ik.GetStability() );
	sdTransport_AF::Think();

	if ( gameLocal.isClient && lastForcedUpdateTime != lastMovedTime ) {
		if ( ( gameLocal.time - lastMovedTime ) > SEC2MS( 1 ) ) {
			lastForcedUpdateTime = lastMovedTime;

			clientNetworkInfo_t& netInfo = gameLocal.GetNetworkInfo( gameLocal.localClientNum );
			sdEntityState* state = netInfo.states[ entityNumber ][ NSM_VISIBLE ];
			if ( state != NULL ) {
				sdTransportNetworkData* data = static_cast< sdTransportNetworkData* >( state->data );
				sdMonsterPhysicsNetworkData* physicsData = static_cast< sdMonsterPhysicsNetworkData* >( data->physicsData );
				physicsObj.SetOrigin( physicsData->origin );
			}
		}
	}
}

/*
================
sdWalker::LoadAF
================
*/
void sdWalker::LoadAF( void ) {
	idStr fileName;

	if ( !spawnArgs.GetString( "ragdoll", "*unknown*", fileName ) || !fileName.Length() ) {
		return;
	}

	af.SetAnimator( GetAnimator() );
	if( !af.Load( this, fileName ) ) {
		return;
	}
}

/*
================
sdWalker::UpdateAnimationControllers
================
*/
bool sdWalker::UpdateAnimationControllers( void ) {
	if ( !gameLocal.isNewFrame ) {
		return false;
	}

	if ( ik.IsInitialized() ) {
		bool animating = GetAnimator()->IsAnimating( gameLocal.time );
		if ( ( lastIKPos - physicsObj.GetOrigin() ).LengthSqr() > Square( 1.f ) ) {
			lastIKTime = gameLocal.time;
		}

		if ( animating || ( gameLocal.time - lastIKTime ) < SEC2MS( 0.5f ) || ik_debug.GetBool() ) {
			if ( animating ) {
				lastIKTime = gameLocal.time;
			}
			lastIKPos = physicsObj.GetOrigin();
			if ( !ik.IsInhibited() ) {
				return ik.Evaluate();
			}
			return false;
		}
	} else {
		ik.ClearJointMods();
	}

	return false;
}

/*
================
sdWalker::LoadIK
================
*/
bool sdWalker::LoadIK( void ) {
	return ik.Init( this, IK_ANIM, vec3_origin );
}

/*
================
sdWalker::DoLoadVehicleScript
================
*/
void sdWalker::DoLoadVehicleScript( void ) {
	LoadParts( VPT_PART | VPT_SIMPLE_PART );

	lights.Clear();
	lights.Init( this );
	for( int i = 0; i < vehicleScript->lights.Num(); i++ ) {
		lights.AddLight( vehicleScript->lights[ i ]->lightInfo );
	}
}

/*
================
sdWalker::DisableClip
================
*/
void sdWalker::DisableClip( bool activateContacting ) {
	sdTransport_AF::DisableClip( activateContacting );

	physicsObj.DisableClip();
	af.GetPhysics()->DisableClip( activateContacting );
}

/*
================
sdWalker::EnableClip
================
*/
void sdWalker::EnableClip( void ) {
	if ( vehicleFlags.modelDisabled ) {
		return;
	}

	sdTransport_AF::EnableClip();

	physicsObj.EnableClip();
	af.GetPhysics()->EnableClip();
}

/*
=====================
sdWalker::SetAxis
=====================
*/
void sdWalker::SetAxis( const idMat3& axis ) {
	viewAxis = axis;

	if ( vehicleControl ) {
		idAngles angles = axis.ToAngles();
		vehicleControl->OnYawChanged( angles.yaw );
	}

	UpdateVisuals();
}


/*
=====================
sdWalker::GetMoveDelta
=====================
*/
void sdWalker::GetMoveDelta( idVec3& delta ) {
	GetAnimator()->GetDelta( gameLocal.time - gameLocal.msec, gameLocal.time, delta, 1 );

//	if ( delta.Length() > 0.f ) {
//		gameLocal.Printf( "Delta: %s Time: %i %s\n", delta.ToString(), gameLocal.time, gameLocal.isNewFrame ? "" : "re-prediction" );
//	}

	delta = viewAxis * delta;
}

/*
================
sdWalker::OnUpdateVisuals
================
*/
void sdWalker::OnUpdateVisuals( void ) {
	sdTransport::OnUpdateVisuals();
	af.SetActive( true );
	af.SetupPose( this, gameLocal.time );
}

/*
================
sdWalker::Spawn
================
*/
void sdWalker::Spawn( void ) {
	animator.RemoveOriginOffset( true );
	animator.ClearAllAnims( gameLocal.time, 0 );
	animator.ClearAllJoints();
	int anim = animator.GetAnim( "base" );
	animator.PlayAnim( ANIMCHANNEL_TORSO, anim, gameLocal.time, 0 );
	animator.CreateFrame( gameLocal.time, true );

	LoadAF();

	LoadVehicleScript();

	if ( !spawnArgs.GetBool( "ik_disabled", "0" ) ) {
		LoadIK();
	}

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "10000" ) );
	physicsObj.SetClipMask( MASK_VEHICLESOLID | CONTENTS_WALKERCLIP | CONTENTS_MONSTER );

	// move up to make sure the monster is at least an epsilon above the floor
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	physicsObj.SetAxis( mat3_identity );
	
	idVec3 gravity = spawnArgs.GetVector( "gravityDir", "0 0 -1" );
	float gravityScale;
	if ( spawnArgs.GetFloat( "gravity", DEFAULT_GRAVITY_STRING, gravityScale ) ) {
		gravity *= gravityScale;
	} else {
		gravity *= g_gravity.GetFloat();		
	}

	physicsObj.SetGravity( gravity );
	SetPhysics( &physicsObj );

	BecomeActive( TH_THINK );

	physicsObj.SetMaxStepHeight( spawnArgs.GetFloat( "ik_maxStepSpeed", "1" ) );
	physicsObj.SetContents( CONTENTS_MONSTER );

	stompDamage			= gameLocal.declDamageType[ spawnArgs.GetString( "dmg_stomp" ) ];
	minStompScale		= spawnArgs.GetFloat( "stomp_min_scale" );
	maxStompScale		= spawnArgs.GetFloat( "stomp_max_scale" );
	stompSpeedScale		= spawnArgs.GetFloat( "stomp_speed_scale" );

	groundPoundMinSpeed	= spawnArgs.GetFloat( "ground_pound_min_speed", "200" );
	groundPoundForce	= spawnArgs.GetFloat( "ground_pound_force", "25000000" );
	groundPoundRange	= spawnArgs.GetFloat( "ground_pound_range", "512" );

	kickForce			= spawnArgs.GetFloat( "kick_force", "25000000" );

	UpdateVisuals();

	Present();
}

/*
================
sdWalker::OnPlayerExited
================
*/
void sdWalker::OnPlayerExited( idPlayer* player, int position ) {
	sdTransport_AF::OnPlayerExited( player, position );

	if ( player == gameLocal.GetLocalViewPlayer() ) {
		ResetPredictionErrorDecay();
	}
}

/*
================
sdWalker::SetDelta
================
*/
void sdWalker::SetDelta( const idVec3& delta ) {
	physicsObj.SetDelta( delta );
}

/*
================
sdWalker::SetCompressionScale
================
*/
void sdWalker::SetCompressionScale( float scale, float length ) {
	ik.SetCompressionScale( scale, length );
}

/*
=================
sdWalker::Collide
=================
*/
bool sdWalker::Collide( const trace_t& trace, const idVec3& velocity, int bodyId ) {
	idEntity* ent = gameLocal.entities[ trace.c.entityNum ];

	idVec3 relativeVelocity = velocity;
	if ( ent != NULL ) {
		relativeVelocity -= ent->GetPhysics()->GetLinearVelocity();
	}

	float verticalSpeed = relativeVelocity * physicsObj.GetGravityNormal();
	float absVerticalSpeed = idMath::Fabs( verticalSpeed );

	if ( ent != NULL ) {
		float horizontalSpeed = relativeVelocity.Length() - absVerticalSpeed;
		if ( horizontalSpeed > 5.f ) {

			idVec3 otherOrigin = ent->GetPhysics()->GetAbsBounds().GetCenter();
			const idVec3& origin = trace.c.point;

			idVec3 impulse = otherOrigin - origin;
			impulse.z += 4.0f;
			impulse.Normalize();
			impulse *= ent->GetRadiusPushScale();

			idVec3 delta = ( otherOrigin - origin ) * 0.5f;
			ent->GetPhysics()->AddForce( 0, origin + delta, kickForce * impulse );
		}
	}

	if ( verticalSpeed > groundPoundMinSpeed ) {
		float scale = absVerticalSpeed * stompSpeedScale;
		if ( scale > minStompScale ) {
			if ( scale > maxStompScale ) {
				scale = maxStompScale;
			}
		} else {
			scale = 0.f;
		}

		GroundPound( groundPoundForce, scale, 512.f );

		if ( scale > 4.f ) {
			const char* surfaceTypeName = NULL;
			if ( trace.c.surfaceType ) {
				surfaceTypeName = trace.c.surfaceType->GetName();
			}
			idVec3 xaxis(-1.f, 0.f, 0.f);
			PlayEffect( "fx_ground_collide", colorWhite.ToVec3(), surfaceTypeName, physicsObj.GetOrigin(), xaxis.ToMat3() );//physicsObj.GetGroundNormal().ToMat3() );
		}
	}

	return false;
}

/*
=================
sdWalker::GroundPound
=================
*/
void sdWalker::GroundPound( float force, float damageScale, float shockWaveRange ) {
	if ( stompDamage != NULL && damageScale > 0.f ) {
		idBounds bounds = physicsObj.GetBounds();
		bounds.GetMaxs()[ 2 ] = bounds.GetMins()[ 2 ] + 64.f;
		bounds.GetMins()[ 2 ] -= 32.f;
		bounds.TranslateSelf( physicsObj.GetOrigin() );

		idEntity* driver = GetPositionManager().FindDriver();
		if ( driver == NULL ) {
			driver = this;
		}

		const int MAX_GROUND_POUND_ENTITIES = 128;
		idEntity* entities[ MAX_GROUND_POUND_ENTITIES ];
		int count = gameLocal.clip.EntitiesTouchingBounds( bounds, MASK_VEHICLESOLID | CONTENTS_CORPSE | CONTENTS_MONSTER, entities, MAX_GROUND_POUND_ENTITIES, true );
		for ( int i = 0; i < count; i++ ) {
			idEntity* ent = entities[ i ];
			if ( ent == this ) {
				continue;
			}

			float scale = damageScale;
			if ( GetEntityAllegiance( ent ) == TA_FRIEND ) {
				scale *= 0.25f;
			}

			ent->Damage( this, driver, physicsObj.GetGravityNormal(), stompDamage, scale, NULL );
		}
	}

	gameLocal.RadiusPush( physicsObj.GetOrigin(), shockWaveRange, stompDamage, force, this, this, RP_GROUNDONLY, false );
}

/*
=================
sdWalker::Event_GroundPound
=================
*/
void sdWalker::Event_GroundPound( float force, float damageScale, float shockWaveRange ) {
	GroundPound( force, damageScale, shockWaveRange );

	bool leftleg = false;
	jointHandle_t jh = GetAnimator()->GetJointHandle( spawnArgs.GetString( leftleg ? "joint_foot_left" : "joint_foot_right" ) );

	idVec3 traceOrig;
	GetWorldOrigin( jh, traceOrig );

	idVec3 traceEnd = traceOrig;
	traceOrig.z += 100.0f;
	traceEnd.z -= 10.0f;

	trace_t		traceObject;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS traceObject, traceOrig, traceEnd, MASK_SOLID | CONTENTS_WATER | MASK_OPAQUE, this );
	traceEnd = traceObject.endpos;

	if ( traceObject.fraction < 1.f ) {
		int cont = gameLocal.clip.Contents( CLIP_DEBUG_PARMS traceEnd, NULL, mat3_identity, CONTENTS_WATER, this );
		if ( !cont ) {

			const char* surfaceTypeName = NULL;
			if ( traceObject.c.surfaceType ) {
				surfaceTypeName = traceObject.c.surfaceType->GetName();
			}

			idVec3 colorWhite(1.f,1.f,1.f);
			idVec3 xaxis(-1.f, 0.f, 0.f);
			PlayEffect( "fx_ground_pound", colorWhite, surfaceTypeName, traceEnd, xaxis.ToMat3() );
		}
	}
}

/*
=================
sdWalker::ApplyNetworkState
=================
*/
void sdWalker::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	sdTransport_AF::ApplyNetworkState( mode, newState );
}
