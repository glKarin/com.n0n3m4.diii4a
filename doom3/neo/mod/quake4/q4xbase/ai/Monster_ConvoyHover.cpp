//----------------------------------------------------------------
// rvMonsterConvoyHover.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

#ifndef __GAME_VEHICLEAI_H__
#include "VehicleAI.h"
#endif

#ifndef __GAME_PROJECTILE_H__
#include "../Projectile.h"
#endif


class rvMonsterConvoyHover : public rvVehicleMonster {
public:
	CLASS_PROTOTYPE ( rvMonsterConvoyHover );

							rvMonsterConvoyHover		( void );
							~rvMonsterConvoyHover		( void );
	void					Save						( idSaveGame *savefile ) const;
	void					Restore						( idRestoreGame *savefile );
	void					InitSpawnArgsVariables		( void );
	void					Spawn						( void );
	void					Think						( void );

private:

	void					Think_Random				( void );
	void					Think_Pathing				( void );

	void					AttackBlaster				( void );
	void					AttackBeam					( void );
	//void					AttackBomb					( void );

	float					angleYaw;			// actually acos( angleYaw ) from player
	float					minYaw;
	float					maxYaw;
	float					desiredHeight;		// actually acos( desiredHeight ) from player
	float					minHeight;
	float					maxHeight;
	float					distance;			// distance to enemy
	float					minDistance;
	float					maxDistance;
	jointHandle_t			jointGunRight;
	jointHandle_t			jointGunLeft;
	
	int						lastAttackTime;
	int						attackStartTime;

	int						blasterAttackDuration;
	int						blasterAttackRate;
	int						bombAttackDuration;
	int						bombAttackRate;
	
	int						shotCount;

	idPhysics_RigidBody		physicsObj;

	static const int DAMPEN_ANGLE_SAMPLES = 8;
	idAngles				hoverDampening[ DAMPEN_ANGLE_SAMPLES ];

	void					CalcDampening				( const idAngles & cur, idAngles & out );

	stateResult_t			State_Idle					( const stateParms_t& parms );
	stateResult_t			State_BlasterAttack			( const stateParms_t& parms );
	stateResult_t			State_BeamAttack			( const stateParms_t& parms );
	//stateResult_t			State_BombAttack			( const stateParms_t& parms );

	struct {
		bool				pathing:1;
		bool				faceEnemy:1;
		bool				dead:1;
	} myfl;

	virtual void			OnDeath						( void );

	CLASS_STATES_PROTOTYPE ( rvMonsterConvoyHover );
};

CLASS_DECLARATION ( rvVehicleMonster, rvMonsterConvoyHover )
END_CLASS

/*
================
rvMonsterConvoyHover::rvMonsterConvoyHover
================
*/
rvMonsterConvoyHover::rvMonsterConvoyHover ( void ) {
}

/*
================
rvMonsterConvoyHover::~rvMonsterConvoyHover
================
*/
rvMonsterConvoyHover::~rvMonsterConvoyHover ( void ) {
	SetPhysics( NULL );
}

/*
================
rvMonsterConvoyHover::Save
================
*/
void rvMonsterConvoyHover::Save ( idSaveGame *savefile ) const {
	savefile->WriteFloat ( angleYaw );
	savefile->WriteFloat ( desiredHeight );
	savefile->WriteFloat ( distance );

	savefile->WriteInt ( lastAttackTime );
	savefile->WriteInt ( attackStartTime );

	savefile->WriteInt ( shotCount );
	
	savefile->WriteStaticObject ( physicsObj );

	savefile->Write ( hoverDampening, sizeof ( idAngles ) * DAMPEN_ANGLE_SAMPLES );

	savefile->Write( &myfl, sizeof(myfl) );	// cnicholson: Added unsaved var
}

/*
================
rvMonsterConvoyHover::Restore
================
*/
void rvMonsterConvoyHover::Restore ( idRestoreGame *savefile ) {
	savefile->ReadFloat ( angleYaw );
	savefile->ReadFloat ( desiredHeight );
	savefile->ReadFloat ( distance );

	savefile->ReadInt ( lastAttackTime );
	savefile->ReadInt ( attackStartTime );

	savefile->ReadInt ( shotCount );

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	savefile->ReadStaticObject ( physicsObj );
	RestorePhysics ( &physicsObj );
	physicsObj.EnableClip();

	savefile->Read ( hoverDampening, sizeof ( idAngles ) * DAMPEN_ANGLE_SAMPLES );

	savefile->Read( &myfl, sizeof(myfl) );	// cnicholson: Added unsaved var

	InitSpawnArgsVariables();
}

void rvMonsterConvoyHover::InitSpawnArgsVariables ( void )
{
	jointGunRight	= animator.GetJointHandle ( spawnArgs.GetString ( "joint_gun_right" ) );
	jointGunLeft	= animator.GetJointHandle ( spawnArgs.GetString ( "joint_gun_left" ) );

	minYaw		= spawnArgs.GetFloat( "minYaw", "0" );
	maxYaw		= spawnArgs.GetFloat( "maxYaw", "360" );
	minHeight	= spawnArgs.GetFloat( "minHeight", "10" );
	maxHeight	= spawnArgs.GetFloat( "maxHeight", "70" );
	minDistance	= spawnArgs.GetFloat( "minDistance", "100" );
	maxDistance	= spawnArgs.GetFloat( "maxDistance", "500" );

	minDistance	*= minDistance;
	maxDistance	*= maxDistance;

	if ( minYaw == 0.0f && maxYaw == 0.0f ) {
		maxYaw = 360.0f;
	}

	blasterAttackDuration	= SEC2MS ( spawnArgs.GetFloat ( "blasterAttackDuration", "1" ) );
	blasterAttackRate		= SEC2MS ( spawnArgs.GetFloat ( "blasterAttackRate", ".25" ) );
	bombAttackDuration		= SEC2MS ( spawnArgs.GetFloat ( "bombAttackDuration", "1" ) );
	bombAttackRate			= SEC2MS ( spawnArgs.GetFloat ( "bombAttackRate", ".25" ) );
}
/*
================
rvMonsterConvoyHover::Spawn
================
*/
void rvMonsterConvoyHover::Spawn ( void ) {
	physicsObj.SetSelf( this );	

	SetClipModel ( physicsObj );

	physicsObj.SetOrigin( GetPhysics()->GetOrigin ( ) );
	physicsObj.SetAxis ( GetPhysics()->GetAxis ( ) );
	physicsObj.SetContents( CONTENTS_BODY );
	physicsObj.SetClipMask( MASK_PLAYERSOLID|CONTENTS_VEHICLECLIP|CONTENTS_FLYCLIP );
	physicsObj.SetFriction ( spawnArgs.GetFloat ( "friction_linear", "1" ), spawnArgs.GetFloat ( "friction_angular", "1" ), spawnArgs.GetFloat ( "friction_contact", "1" ) );
	physicsObj.SetBouncyness ( spawnArgs.GetFloat ( "bouncyness", "0.6" ) );
	physicsObj.SetGravity( vec3_origin );
	SetPhysics( &physicsObj );

	animator.CycleAnim ( ANIMCHANNEL_ALL, animator.GetAnim( spawnArgs.GetString( "anim", "idle" ) ), gameLocal.time, 0 );	

	BecomeActive( TH_THINK );

	InitSpawnArgsVariables();

	shotCount	= 0;

	angleYaw		= rvRandom::flrand( minYaw, maxYaw );
	desiredHeight	= 0.5f * (  maxHeight - minHeight ) + minHeight;
	distance		= 0.5f * (  maxDistance - minDistance ) + minDistance;

	lastAttackTime			= 0;

	SetState( "Idle" );

	myfl.pathing = false;
	myfl.faceEnemy = true;
	myfl.dead = false;

	for ( int i = 0; i < DAMPEN_ANGLE_SAMPLES; i++ ) {
		hoverDampening[ i ].Zero();
	}

	jointHandle_t joint;
	joint = GetAnimator()->GetJointHandle( spawnArgs.GetString ( "joint_thruster", "tail_thrusters" ) );
	if ( joint != INVALID_JOINT ) {
		PlayEffect ( "fx_exhaust", joint, true );
	}
	StartSound ( "snd_flyloop", SND_CHANNEL_ANY, 0, false, NULL );
}

/*
================
rvMonsterConvoyHover::Think
================
*/
void rvMonsterConvoyHover::Think ( void ) {
	if ( !driver ) {
		rvVehicleMonster::Think();
		return;
	}

	if ( !driver->IsDriving() ) {
		GetPhysics()->SetGravity( gameLocal.GetGravity() );
	}

	trace_t trace;
	idVec3 dir = GetPhysics()->GetLinearVelocity();
	dir.Normalize();
	if ( gameLocal.TracePoint( this, trace, GetOrigin(), GetPhysics()->GetOrigin() + dir * 100.0f, MASK_SOLID|CONTENTS_FLYCLIP, 0 ) ) {
		if ( trace.c.contents == CONTENTS_FLYCLIP ) {
			float maxHeight = this->maxHeight * 2.0f;
			GetPhysics()->ApplyImpulse( 0, GetOrigin(), GetPhysics()->GetLinearVelocity() * 2.0f );
			angleYaw		= rvRandom::flrand( minYaw, maxYaw );
			desiredHeight	= idMath::ClampFloat( minHeight, maxHeight, desiredHeight + 5.0f );
			distance		= idMath::ClampFloat( minDistance, maxDistance, distance + 5.0f );
		}
	}

	if ( !driver->CanSee( driver->enemy.ent, false ) ) {
		desiredHeight	= idMath::ClampFloat( minHeight, maxHeight, desiredHeight + 5.0f );
		distance		= idMath::ClampFloat( minDistance, maxDistance, distance - 5.0f );
	}

	if ( myfl.pathing ) {
		Think_Pathing( );
	} else {
		Think_Random( );
	}

	rvVehicleMonster::Think();

	idVec3 vel = GetPhysics()->GetLinearVelocity();
//	vel += idVec3(	idMath::Sin( gameLocal.time + vel.x ), 
//					idMath::Sin( gameLocal.time + vel.y ), 
//					idMath::Sin( gameLocal.time + vel.z ) );
					

	idAngles hover = idAngles(	idMath::ClampFloat( -45.0f, 45.0f, vel.x / 20.0f ), 
								0.0f,
								idMath::ClampFloat( -25.0f, 25.0f, vel.z / 20.0f ) );

	CalcDampening( hover, hover );

	if ( myfl.faceEnemy ) {
		LookAtEntity( gameLocal.GetLocalPlayer(), 0 );
		idAngles angles = GetPhysics()->GetAxis( ).ToAngles( );
		angles.pitch *= 0.35f;
		GetPhysics()->SetAxis( angles.ToMat3( ) * hover.ToMat3() );
	} else {
		GetPhysics()->SetAxis( GetPhysics()->GetAxis() * hover.ToMat3() );
	}
}

/*
================
rvMonsterConvoyHover::Think_Random
================
*/
void rvMonsterConvoyHover::Think_Random ( void ) {
	const idVec3 xAxis( 1.0f, 0.0f, 0.0f );
	const idVec3 yAxis( 0.0f, 1.0f, 0.0f );
	const idVec3 zAxis( 0.0f, 0.0f, 1.0f );
	const float  xSpeed	= 16000.0f;
	const float  ySpeed	= 9000.0f;
	const float  zSpeed	= 9000.0f;

	idEntity * ent		= driver->enemy.ent;
	if ( !ent ) {
		ent = gameLocal.GetLocalPlayer();
	}

	idVec3 toEnemyYaw	= ent->GetPhysics()->GetOrigin() - GetOrigin();
	toEnemyYaw.z		= 0.0f;
	toEnemyYaw.Normalize();
	float yaw			= toEnemyYaw.ToAngles().yaw;
	float height		= GetOrigin().z - ent->GetPhysics()->GetOrigin().z;
	float dist			= GetOrigin().Dist2XY( ent->GetPhysics()->GetOrigin() );

	// yaw
	if ( idMath::Fabs( yaw - angleYaw ) < 10.0f ) {
		angleYaw = rvRandom::flrand( minYaw, maxYaw );
	} else {
		idVec3 impulse = yAxis * ( ( angleYaw < yaw ) ? ySpeed : -ySpeed );
		impulse *= GetPhysics()->GetAxis();
		GetPhysics()->ApplyImpulse( 0, GetOrigin(), impulse );

		// 5% chance of choosing a new angle
		if ( rvRandom::flrand() < 0.05f ) {
			angleYaw = rvRandom::flrand( minYaw, maxYaw );
		}
	}

	// pitch (changed to height for convenience)
	if ( idMath::Fabs( height - desiredHeight ) < 5.0f ) {
		desiredHeight = rvRandom::flrand( minHeight, maxHeight );
	} else if ( GetOrigin().z < ent->GetPhysics()->GetOrigin().z + desiredHeight ) {
		GetPhysics()->ApplyImpulse( 0, GetOrigin(), zAxis * zSpeed );
	} else {
		GetPhysics()->ApplyImpulse( 0, GetOrigin(), zAxis * -zSpeed );
	}

	// distance
	if ( idMath::Fabs( dist - distance ) > 20.0f ) {
		idVec3 impulse	= xAxis * ( ( dist < distance ) ? -xSpeed : xSpeed );
		idMat3 axis		= GetPhysics()->GetAxis();
		axis[ 1 ]		= idVec3( 0.0f, 0.0f, 1.0f );
		axis[ 2 ]		= axis[ 1 ].Cross( axis[ 0 ] );
		impulse			*= axis;
		GetPhysics()->ApplyImpulse( 0, GetOrigin(), impulse );
	} else {
		// just choose a random distance for now
		distance = rvRandom::flrand( minDistance, maxDistance );
	}
}

/*
================
rvMonsterConvoyHover::Think_Pathing
================
*/
void rvMonsterConvoyHover::Think_Pathing ( void ) {
}

/*
================
rvMonsterConvoyHover::AttackBlaster
================
*/
void rvMonsterConvoyHover::AttackBlaster ( void ) {
	idVec3 offset;
	idMat3 axis;
	jointHandle_t joint;	
	
	joint = ((shotCount++)&1) ? jointGunRight : jointGunLeft;
	
	if ( joint == INVALID_JOINT ) {
		return;
	}
	
	if ( !GetJointWorldTransform( joint, gameLocal.time, offset, axis ) ) {
		return;
	}

	PlayEffect ( "fx_muzzleflash", joint );
	idProjectile* proj = gameLocal.SpawnSafeEntityDef<idProjectile>( spawnArgs.GetString("def_attack_blaster") );
	if( proj ) {
		idVec3 dir = GetVectorToEnemy();
		if ( dir.Normalize() == 0.0f ) {
			dir = axis[ 0 ];
		}
		proj->Create( this, offset, dir, NULL );
		proj->Launch( offset, dir, GetPhysics()->GetPushedLinearVelocity() );
	}
}

/*
================
rvMonsterConvoyHover::AttackBeam
================
*/
void rvMonsterConvoyHover::AttackBeam ( void ) {
	idVec3 offset;
	idMat3 axis;
	jointHandle_t joint;	
	
	joint = ((shotCount++)&1) ? jointGunRight : jointGunLeft;
	
	if ( joint == INVALID_JOINT ) {
		return;
	}
	
	if ( !GetJointWorldTransform( joint, gameLocal.time, offset, axis ) ) {
		return;
	}

	PlayEffect ( "fx_muzzleflash", joint );
	idProjectile* proj = gameLocal.SpawnSafeEntityDef<idProjectile>( spawnArgs.GetString("def_attack_blaster") );
	if( proj ) {
		idVec3 dir = GetVectorToEnemy();
		if ( dir.Normalize() == 0.0f ) {
			dir = axis[ 0 ];
		}
		proj->Create( this, offset, dir, NULL );
		proj->Launch( offset, dir, GetPhysics()->GetPushedLinearVelocity() );
	}
}

/*
================
rvMonsterConvoyHover::AttackBomb
================
*
void rvMonsterConvoyHover::AttackBomb ( void ) {
	jointHandle_t joint;	
	joint = ((shotCount++)&1) ? jointGunRight : jointGunLeft;

	if ( joint == INVALID_JOINT ) {
		return;
	}
	
	if ( !GetJointWorldTransform( joint, gameLocal.time, offset, axis ) ) {
		return;
	}

	StartSound ( "snd_bombrun", SND_CHANNEL_ANY, 0, false, NULL );

	PlayEffect ( "fx_bombflash", joint );
	idProjectile* proj = gameLocal.SpawnSafeEntityDef<idProjectile>( spawnArgs.GetString("def_attack_bomb") );
	if( proj ) {
		proj->Create( this, offset, axis[0], NULL );
		proj->Launch( offset, axis[0], GetPhysics()->GetPushedLinearVelocity() );
	}
}
*/



/*
================
rvMonsterConvoyHover::OnDeath
================
*/
void rvMonsterConvoyHover::OnDeath ( void ) {
	myfl.dead = true;
	idVec3 angular = idVec3( rvRandom::flrand( 180.0f, 250.0f ), rvRandom::flrand( 180.0f, 250.0f ), rvRandom::flrand( 180.0f, 250.0f ) );

	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetFriction( 0.0f, 0.0f, 0.0f );
	GetPhysics()->SetAngularVelocity ( angular );

	gameLocal.PlayEffect ( spawnArgs, "fx_death", GetPhysics()->GetOrigin(), (-GetPhysics()->GetGravityNormal()).ToMat3() );
}

/*
================
rvMonsterConvoyHover::CalcDampening
================
*/
void rvMonsterConvoyHover::CalcDampening ( const idAngles & cur, idAngles & out ) {
	idAngles current = cur;	// just incase cur == out
	out = cur;

	for ( int i = 1; i < DAMPEN_ANGLE_SAMPLES; i ++ ) {
		hoverDampening[ i - 1 ] = hoverDampening[ i ];
		out += hoverDampening[ i ];
	}
	hoverDampening[ DAMPEN_ANGLE_SAMPLES - 1 ] = current;

	out *= ( 1.0f / DAMPEN_ANGLE_SAMPLES );
}


/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterConvoyHover )
	STATE( "Idle",				rvMonsterConvoyHover::State_Idle )
	STATE( "BlasterAttack",		rvMonsterConvoyHover::State_BlasterAttack )
	STATE( "BeamAttack",		rvMonsterConvoyHover::State_BeamAttack )
//	STATE( "BombAttack",		rvMonsterConvoyHover::State_BombAttack )
END_CLASS_STATES


/*
================
rvMonsterConvoyHover::State_Idle
================
*/
stateResult_t rvMonsterConvoyHover::State_Idle ( const stateParms_t& parms ) {
	if ( driver ) {
		if ( gameLocal.time - lastAttackTime > rvRandom::irand( 1000, 1200 ) && CanSee( driver->enemy.ent, false ) ) {
//			if ( rvRandom::irand( 0, 100 ) < 10 ) {
//				distance = 0;
//				PostState( "BeamAttack" );
//			} else {
				PostState( "BlasterAttack" );
//			}
			PostState( "Idle" );
			return SRESULT_DONE;
		}
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}

/*
================
rvMonsterConvoyHover::State_BlasterAttack
================
*/
stateResult_t rvMonsterConvoyHover::State_BlasterAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_BLASTER,
		STAGE_BLASTERWAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			attackStartTime = gameLocal.time;
			return SRESULT_STAGE ( STAGE_BLASTER );
		
		case STAGE_BLASTER:
			lastAttackTime = gameLocal.time;
			AttackBlaster ( );
			return SRESULT_STAGE ( STAGE_BLASTERWAIT );
		
		case STAGE_BLASTERWAIT:
			if ( gameLocal.time - attackStartTime > blasterAttackDuration ) {
				return SRESULT_DONE;
			}
			if ( gameLocal.time - lastAttackTime > blasterAttackRate ) {
				return SRESULT_STAGE ( STAGE_BLASTER );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterConvoyHover::State_BeamAttack
================
*/
stateResult_t rvMonsterConvoyHover::State_BeamAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_BEAM,
		STAGE_BEAMWAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			attackStartTime = gameLocal.time;
			return SRESULT_STAGE ( STAGE_BEAM );
		
		case STAGE_BEAM:
			lastAttackTime = gameLocal.time;
			AttackBeam( );
			return SRESULT_STAGE ( STAGE_BEAMWAIT );
		
		case STAGE_BEAMWAIT:
			if ( gameLocal.time - attackStartTime > blasterAttackDuration ) {
				return SRESULT_DONE;
			}
			if ( gameLocal.time - lastAttackTime > blasterAttackRate ) {
				return SRESULT_STAGE ( STAGE_BEAM );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}

/*
================
rvMonsterConvoyHover::State_BombAttack
================
*
stateResult_t rvMonsterConvoyHover::State_BombAttack ( const stateParms_t& parms ) {
	enum {
		STAGE_INIT,
		STAGE_BOMB,
		STAGE_BOMBWAIT
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			attackStartTime = gameLocal.time;
			return SRESULT_STAGE ( STAGE_BOMB );
		
		case STAGE_BOMB:
			lastAttackTime = gameLocal.time;
			AttackBomb ( );
			return SRESULT_STAGE ( STAGE_BOMBWAIT );
		
		case STAGE_BOMBWAIT:
			if ( !enemy.fl.inFov || gameLocal.time - attackStartTime > bombAttackDuration ) {
				return SRESULT_DONE;
			}
			if ( gameLocal.time - lastAttackTime > bombAttackRate ) {
				return SRESULT_STAGE ( STAGE_BOMB );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR;
}
*/
