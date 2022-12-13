
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef CX_LaserOn("sunBeamOn", NULL, 'd');
const idEventDef CX_LaserOff("sunBeamOff");
const idEventDef MA_UpdateLasers("<updateLasers>");
const idEventDef MA_AssignLeftMuzzleFx( "<assignLeftMuzzleFx>", "e" );
const idEventDef MA_AssignRightMuzzleFx( "<assignRightMuzzleFx>", "e" );
const idEventDef MA_AssignLeftImpactFx( "<assignLeftImpactFx>", "e" );
const idEventDef MA_AssignRightImpactFx( "<assignRightImpactFx>", "e" );
const idEventDef MA_AssignLeftRechargeFx( "<assignLeftRechargeFx>", "e" );
const idEventDef MA_AssignRightRechargeFx( "<assignRightRechargeFx>", "e" );
const idEventDef MA_SetGunOffset( "setGunOffset", "v" );
const idEventDef MA_EndLeftBeams( "<endLeftBeams>" );
const idEventDef MA_EndRightBeams( "<endRightBeams>" );
const idEventDef MA_HudEvent( "hudEvent", "s" );
const idEventDef MA_GunRecharge( "gunRecharge", "d" );
const idEventDef MA_EndRecharge( "endRecharge" );
const idEventDef MA_ResetRechargeBeam( "<resetRechargeBeam>" );
const idEventDef MA_SparkLeft( "<sparkLeft>" );
const idEventDef MA_SparkRight( "<sparkRight>" );
const idEventDef MA_LeftGunDeath( "<leftGunDeath>" );
const idEventDef MA_RightGunDeath( "<rightGunDeath>" );
const idEventDef MA_StartRechargeBeams( "<startRechargeBeams>" );

CLASS_DECLARATION(hhMonsterAI, hhCreatureX)
	EVENT(CX_LaserOn,				hhCreatureX::Event_LaserOn )
	EVENT(CX_LaserOff,				hhCreatureX::Event_LaserOff )
	EVENT(MA_UpdateLasers,			hhCreatureX::Event_UpdateLasers )
	EVENT(MA_AssignRightMuzzleFx,	hhCreatureX::Event_AssignRightMuzzleFx )
	EVENT(MA_AssignLeftMuzzleFx,	hhCreatureX::Event_AssignLeftMuzzleFx )
	EVENT(MA_AssignRightImpactFx,	hhCreatureX::Event_AssignRightImpactFx )
	EVENT(MA_AssignLeftImpactFx,	hhCreatureX::Event_AssignLeftImpactFx )
	EVENT(MA_SetGunOffset,			hhCreatureX::Event_SetGunOffset )
	EVENT(MA_EndLeftBeams,			hhCreatureX::Event_EndLeftBeams )
	EVENT(MA_EndRightBeams,			hhCreatureX::Event_EndRightBeams )
	EVENT(MA_HudEvent,				hhCreatureX::Event_HudEvent )
	EVENT(MA_GunRecharge,			hhCreatureX::Event_GunRecharge )
	EVENT(MA_EndRecharge,			hhCreatureX::Event_EndRecharge )
	EVENT(MA_ResetRechargeBeam,		hhCreatureX::Event_ResetRechargeBeam )
	EVENT(MA_SparkLeft,				hhCreatureX::Event_SparkLeft )
	EVENT(MA_SparkRight,			hhCreatureX::Event_SparkRight )
	EVENT(MA_LeftGunDeath,			hhCreatureX::Event_LeftGunDeath )
	EVENT(MA_RightGunDeath,			hhCreatureX::Event_RightGunDeath )
	EVENT(MA_StartRechargeBeams,	hhCreatureX::Event_StartRechargeBeams )
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
void hhCreatureX::Spawn() {
	idVec3 boneOrigin;
	idMat3 boneAxis;
	targetStart_L = vec3_zero;
	targetStart_R = vec3_zero;
	targetEnd_L = vec3_zero;
	targetEnd_R = vec3_zero;
	targetCurrent_L = vec3_zero;
	targetCurrent_R = vec3_zero;
	bLaserLeftActive = false;
	bLaserRightActive = false;
	nextBeamTime = 0;
	nextLeftZapTime = 0;
	nextRightZapTime = 0;
	nextLaserLeft = 0;
	nextLaserRight = 0;
	nextHealthTick = 0;
	rightGunLives = spawnArgs.GetInt( "gun_lives", "3" );
	leftGunLives = spawnArgs.GetInt( "gun_lives", "3" );
	rightGunHealth = spawnArgs.GetInt( "gun_health", "20" );
	leftGunHealth = spawnArgs.GetInt( "gun_health", "20" );
	bScripted = spawnArgs.GetBool( "scripted", "0" );
	numBurstBeams = 0;

	if ( bScripted ) {
		//don't spawn any combat related entities if scripted
		return;
	}

	laserRight = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamLaser" ) );
	if( laserRight.IsValid() ) {
		GetJointWorldTransform( spawnArgs.GetString("laser_bone_right"), boneOrigin, boneAxis );

		idVec3 junkOrigin = boneOrigin + viewAxis[0] * 60; //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.x)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.y)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.z)); //Test for bad origin

		laserRight->SetOrigin( boneOrigin + viewAxis[0] * 60 );
		laserRight->Activate( false );
	}
	laserLeft = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamLaser" ) );
	if( laserLeft.IsValid() ) {
		GetJointWorldTransform( spawnArgs.GetString("laser_bone_left"), boneOrigin, boneAxis );

		idVec3 junkOrigin = boneOrigin + viewAxis[0] * 60; //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.x)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.y)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.z)); //Test for bad origin

		laserLeft->SetOrigin( boneOrigin + viewAxis[0] * 60 );
		laserLeft->Activate( false );
	}
	preLaserRight = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamLaser" ) );
	if( preLaserRight.IsValid() ) {
		GetJointWorldTransform( spawnArgs.GetString("laser_bone_right"), boneOrigin, boneAxis );

		idVec3 junkOrigin = boneOrigin; //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.x)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.y)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.z)); //Test for bad origin

		preLaserRight->SetOrigin( boneOrigin );
		preLaserRight->BindToJoint( this, spawnArgs.GetString("laser_bone_right"), false );
		preLaserRight->Activate( false );
	}
	preLaserLeft = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamLaser" ) );
	if( preLaserLeft.IsValid() ) {
		GetJointWorldTransform( spawnArgs.GetString("laser_bone_left"), boneOrigin, boneAxis );

		idVec3 junkOrigin = boneOrigin; //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.x)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.y)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(junkOrigin.z)); //Test for bad origin

		preLaserLeft->SetOrigin( boneOrigin );
		preLaserLeft->BindToJoint( this, spawnArgs.GetString("laser_bone_left"), false );
		preLaserLeft->Activate( false );
	}
	PostEventSec( &MA_UpdateLasers, spawnArgs.GetFloat( "laser_freq", "0.1" ) );

	leftBeamList.Clear();
	rightBeamList.Clear();
	leftRechargeBeam.Clear();
	rightRechargeBeam.Clear();
	leftRecharger.Clear();
	rightRecharger.Clear();
	if ( spawnArgs.GetBool( "use_recharge" ) ) {
		numBurstBeams = spawnArgs.GetInt( "num_burst_beams", "4" );
		for ( int i = 0; i < numBurstBeams; i ++ ) {
			leftBeamList.Append( hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamBurst" ) ) );
			if( leftBeamList[i].IsValid() ) {
				leftBeamList[i]->Activate( false );
				GetJointWorldTransform( spawnArgs.GetString("damage_bone_left"), boneOrigin, boneAxis );
				leftBeamList[i]->SetOrigin( boneOrigin );
				leftBeamList[i]->BindToJoint( this, spawnArgs.GetString("damage_bone_left"), false );
			}
			rightBeamList.Append( hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamBurst" ) ) );
			if( rightBeamList[i].IsValid() ) {
				rightBeamList[i]->Activate( false );
				GetJointWorldTransform( spawnArgs.GetString("damage_bone_right"), boneOrigin, boneAxis );
				rightBeamList[i]->SetOrigin( boneOrigin );
				rightBeamList[i]->BindToJoint( this, spawnArgs.GetString("damage_bone_right"), false );
			}
		}

		leftRechargeBeam = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamRecharge" ) );
		if( leftRechargeBeam.IsValid() ) {
			leftRechargeBeam->Activate( false );
			GetJointWorldTransform( spawnArgs.GetString("laser_bone_left"), boneOrigin, boneAxis );
			leftRechargeBeam->SetOrigin( boneOrigin );
			leftRechargeBeam->BindToJoint( this, spawnArgs.GetString("laser_bone_left"), false );
			leftRechargeBeam->Hide();
		}
		rightRechargeBeam = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamRecharge" ) );
		if( rightRechargeBeam.IsValid() ) {
			rightRechargeBeam->Activate( false );
			GetJointWorldTransform( spawnArgs.GetString("laser_bone_right"), boneOrigin, boneAxis );
			rightRechargeBeam->SetOrigin( boneOrigin );
			rightRechargeBeam->BindToJoint( this, spawnArgs.GetString("laser_bone_right"), false );
			rightRechargeBeam->Hide();
		}
		leftDamageBeam = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamDamage" ) );
		if( leftDamageBeam.IsValid() ) {
			leftDamageBeam->Activate( false );
			GetJointWorldTransform( spawnArgs.GetString("laser_bone_left"), boneOrigin, boneAxis );
			leftDamageBeam->SetOrigin( boneOrigin );
			leftDamageBeam->BindToJoint( this, spawnArgs.GetString("laser_bone_left"), false );
			leftDamageBeam->Hide();
		}
		rightDamageBeam = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamDamage" ) );
		if( rightDamageBeam.IsValid() ) {
			rightDamageBeam->Activate( false );
			GetJointWorldTransform( spawnArgs.GetString("laser_bone_right"), boneOrigin, boneAxis );
			rightDamageBeam->SetOrigin( boneOrigin );
			rightDamageBeam->BindToJoint( this, spawnArgs.GetString("laser_bone_right"), false );
			rightDamageBeam->Hide();
		}
		leftRetractBeam = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamRetract" ) );
		if( leftRetractBeam.IsValid() ) {
			leftRetractBeam->Activate( false );
			GetJointWorldTransform( spawnArgs.GetString("laser_bone_left"), boneOrigin, boneAxis );
			leftRetractBeam->SetOrigin( boneOrigin );
			leftRetractBeam->BindToJoint( this, spawnArgs.GetString("laser_bone_left"), false );
			leftRetractBeam->Hide();
		}
		rightRetractBeam = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamRetract" ) );
		if( rightRetractBeam.IsValid() ) {
			rightRetractBeam->Activate( false );
			GetJointWorldTransform( spawnArgs.GetString("laser_bone_right"), boneOrigin, boneAxis );
			rightRetractBeam->SetOrigin( boneOrigin );
			rightRetractBeam->BindToJoint( this, spawnArgs.GetString("laser_bone_right"), false );
			rightRetractBeam->Hide();
		}

		idEntity *ent;
		idDict dict;
		const idDict *rechargeDict = gameLocal.FindEntityDefDict(spawnArgs.GetString("def_recharger"));
		if ( !rechargeDict ) {
			gameLocal.Error( "Unknown def_recharger:  %s\n", spawnArgs.GetString("def_recharger") );
		}
		dict.Copy(*rechargeDict);
		gameLocal.SpawnEntityDef( dict, &ent );
		if ( ent ) {
			leftRecharger.Assign( ent );
			leftRecharger->GetPhysics()->DisableClip();
			leftRecharger->GetPhysics()->SetClipMask( 0 );
			leftRecharger->HideNoDormant();
			leftRecharger->SetOrigin( GetOrigin() );
			leftRecharger->SetEnemy( this );
			leftRecharger->spawnArgs.Set( "left_recharger", "1" );		//set key for left or right
		}
		ent = NULL;
		gameLocal.SpawnEntityDef( dict, &ent );
		if ( ent ) {
			rightRecharger.Assign( ent );
			rightRecharger->GetPhysics()->DisableClip();
			rightRecharger->GetPhysics()->SetClipMask( 0 );
			rightRecharger->HideNoDormant();
			rightRecharger->SetOrigin( GetOrigin() );
			rightRecharger->SetEnemy( this );
			rightRecharger->spawnArgs.Set( "right_recharger", "1" );		//set key for left or right
		}
	}
}

hhCreatureX::~hhCreatureX() {
	MuzzleLeftOff();
	MuzzleRightOff();

	SAFE_REMOVE( laserRight );
	SAFE_REMOVE( laserLeft );
	SAFE_REMOVE( preLaserLeft );
	SAFE_REMOVE( preLaserRight );
}

void hhCreatureX::Event_LaserOn() {
	if ( !enemy.IsValid() ) {
		idThread::ReturnInt( false );
		return;
	}
	if ( !AI_RIGHT_DAMAGED ) {
		bLaserRightActive = true;
	}
	if ( !AI_LEFT_DAMAGED ) {
		bLaserLeftActive = true;
	}
	idVec3 toEnemy = GetEnemy()->GetOrigin() - GetOrigin();
	targetStart_L = GetOrigin() + spawnArgs.GetFloat( "test_1", "0.5" ) * toEnemy;
	targetStart_L.z = GetOrigin().z;
	float distToStart = (targetStart_L - GetOrigin()).LengthFast();
	if ( distToStart < 200 ) {
		targetStart_L = GetOrigin() + toEnemy;
		targetStart_L.z = GetOrigin().z;		
	}
	targetStart_R = GetOrigin() + spawnArgs.GetFloat( "test_1", "0.5" ) * toEnemy;
	targetStart_R.z = GetOrigin().z;
	distToStart = (targetStart_R - GetOrigin()).LengthFast();
	if ( distToStart < 200 ) {
		targetStart_R = GetOrigin() + toEnemy;
		targetStart_R.z = GetOrigin().z;		
	}
	targetEnd_L = GetOrigin() + spawnArgs.GetFloat( "test_2", "1.4" ) * toEnemy;
	targetEnd_L.z = GetOrigin().z + spawnArgs.GetFloat( "test_4", "50" );
	targetEnd_R = GetOrigin() + spawnArgs.GetFloat( "test_2", "1.4" ) * toEnemy;
	targetEnd_R.z = GetOrigin().z + spawnArgs.GetFloat( "test_4", "50" );
	if ( ai_debugBrain.GetBool() ) {
		gameRenderWorld->DebugArrow( colorRed, targetStart_L, targetEnd_L, 10, 10000 );
	}
	targetAlpha_L = 0.0f;
	targetAlpha_R = 0.0f;
	if ( !bLaserLeftActive && !bLaserRightActive ) {
		idThread::ReturnInt( false );
	}
	idThread::ReturnInt( true );
}

void hhCreatureX::Event_LaserOff() {
	bLaserLeftActive = false;
	bLaserRightActive = false;
	targetStart_L = vec3_zero;
	targetStart_R = vec3_zero;
	targetEnd_L = vec3_zero;
	targetEnd_R = vec3_zero;
	MuzzleLeftOff();
	MuzzleRightOff();
}

void hhCreatureX::Event_UpdateLasers() {
	if ( !enemy.IsValid() ) {
		PostEventSec( &MA_UpdateLasers, spawnArgs.GetFloat( "laser_freq", "0.1" ) ); 
		return;
	}
	trace_t trace;
	float dist = 0;
	idVec3 boneOrigin, traceEnd;
	idMat3 boneAxis;
	idEntity *hitEntity = NULL;
	int hitCount = 0;
	if ( laserRight.IsValid() && bLaserRightActive ) {
		MuzzleRightOn();
		traceEnd = targetCurrent_R + 2000 * (targetCurrent_R - laserRight->GetOrigin()).ToNormal();
		GetJointWorldTransform( spawnArgs.GetString("laser_bone_right"), boneOrigin, boneAxis );
		gameLocal.clip.TracePoint( trace, laserRight->GetOrigin(), traceEnd, MASK_SHOT_RENDERMODEL, this );
		if ( ai_debugBrain.GetBool() ) {
			gameRenderWorld->DebugArrow( colorRed, laserRight->GetOrigin(), traceEnd, 10, 200 );
		}
		if ( trace.fraction < 1.0f ) {
			dist = (trace.endpos - laserRight->GetOrigin()).Length();
			hitEntity = gameLocal.GetTraceEntity( trace );
			if ( hitEntity ) {
				hitEntity->Damage( this, this, hitEntity->GetOrigin() - GetOrigin().ToNormal(), spawnArgs.GetString( "def_laserDamage" ), 1.0, 0 );
			}
		}
	} else {
		MuzzleRightOff();
	}
	if ( laserLeft.IsValid() && bLaserLeftActive ) {
		MuzzleLeftOn();
		traceEnd = targetCurrent_L + 2000 * (targetCurrent_L - laserLeft->GetOrigin()).ToNormal();
		GetJointWorldTransform( spawnArgs.GetString("laser_bone_left"), boneOrigin, boneAxis );
		gameLocal.clip.TracePoint( trace, laserLeft->GetOrigin(), traceEnd, MASK_SHOT_RENDERMODEL, this );
		if ( ai_debugBrain.GetBool() ) {
			gameRenderWorld->DebugArrow( colorRed, laserLeft->GetOrigin(), traceEnd, 10, 200 );
		}
		if ( trace.fraction < 1.0f ) {
			dist = (trace.endpos - laserLeft->GetOrigin()).Length();
			hitEntity = gameLocal.GetTraceEntity( trace );
			if ( hitEntity ) {
				hitEntity->Damage( this, this, hitEntity->GetOrigin() - GetOrigin().ToNormal(), spawnArgs.GetString( "def_laserDamage" ), 1.0, 0 );
			}
		}
	} else {
		MuzzleLeftOff();
	}
	PostEventSec( &MA_UpdateLasers, spawnArgs.GetFloat( "laser_freq", "0.1" ) ); 
}

void hhCreatureX::Event_ResetRechargeBeam() {
	if ( !AI_RECHARGING ) {
		return;
	}
	if ( leftRecharger.IsValid() ) {
		leftRecharger->SetShaderParm( 6, 0.0f );
	}
	if ( rightRecharger.IsValid() ) {
		rightRecharger->SetShaderParm( 6, 0.0f );
	}
	if ( rechargeLeftFx.IsValid() ) {	
		rechargeLeftFx->SetParticleShaderParm( 6, 0.0f );
	}
	if ( rechargeRightFx.IsValid() ) {	
		rechargeRightFx->SetParticleShaderParm( 6, 0.0f );
	}
	if ( leftRechargeBeam.IsValid() ) {
		leftRechargeBeam->Activate( true );
		leftRechargeBeam->Show();
	}
	if ( rightRechargeBeam.IsValid() ) {
		rightRechargeBeam->Activate( true );
		rightRechargeBeam->Show();
	}
	if ( leftDamageBeam.IsValid() ) {
		leftDamageBeam->Activate( false );
	}
	if ( rightDamageBeam.IsValid() ) {
		rightDamageBeam->Activate( false );
	}
	if ( leftRetractBeam.IsValid() ) {
		leftRetractBeam->Activate( false );
	}
	if ( rightRetractBeam.IsValid() ) {
		rightRetractBeam->Activate( false );
	}
}

void hhCreatureX::Think() {
	PROFILE_SCOPE("AI", PROFMASK_NORMAL|PROFMASK_AI);
	if (ai_skipThink.GetBool()) {
		return;
	}

	idVec3 pastEnemy;
	hhMonsterAI::Think();
	if ( AI_RECHARGING ) {
		bool damaged = false;
		if ( leftRecharger.IsValid() ) {
			//recharger damage. switch out beams
			if ( leftRechargeBeam.IsValid() && leftRecharger->GetHealth() < leftRechargerHealth ) {
				if ( leftRechargerHealth > 0 && !leftRecharger->IsDead() ) {
					damaged = true;
				}
				leftRechargeBeam->Activate( false );
				leftDamageBeam->Show();
				leftDamageBeam->Activate( true );
				leftRecharger->SetShaderParm( 6, 1.0f );
				if ( rechargeLeftFx.IsValid() ) {	
					rechargeLeftFx->SetParticleShaderParm( 6, 1.0f );
				}
			}
			leftRechargerHealth = leftRecharger->GetHealth();
		}
		if ( rightRecharger.IsValid() ) {
			//recharger damage. switch out beams
			if ( rightRechargeBeam.IsValid() && rightRecharger->GetHealth() < rightRechargerHealth ) {
				if ( rightRechargerHealth > 0 && !rightRecharger->IsDead() ) {
					damaged = true;
				}
				rightRechargeBeam->Activate( false );
				rightDamageBeam->Show();
				rightDamageBeam->Activate( true );
				rightRecharger->SetShaderParm( 6, 1.0f );
				if ( rechargeRightFx.IsValid() ) {	
					rechargeRightFx->SetParticleShaderParm( 6, 1.0f );
				}
			}
			rightRechargerHealth = rightRecharger->GetHealth();
		}
		if ( damaged ) {
			PostEventSec( &MA_ResetRechargeBeam, spawnArgs.GetFloat( "damage_beam_duration", "0.8" ) );
		}
		if ( !AI_LEFT_DAMAGED && leftRecharger.IsValid() && leftRecharger->GetHealth() <= 0 ) {
			//recharger has died so start retracting the beams and schedule the gun's death
			StartSound( "snd_gun_predeath", SND_CHANNEL_ANY );
			SAFE_REMOVE( leftRechargeBeam );
			PostEventSec( &MA_LeftGunDeath, spawnArgs.GetFloat("retract_delay", "0.9") );
			const char *defName = spawnArgs.GetString( "fx_retract" );
			if ( defName && defName[0] && leftDamageBeam.IsValid() ) {
				hhFxInfo fxInfo;
				fxInfo.RemoveWhenDone( true );
				retractLeftFx = SpawnFxLocal( defName, leftDamageBeam->GetTargetLocation(), mat3_identity, &fxInfo, gameLocal.isClient );
			}
			if ( leftRetractBeam.IsValid() ) {
				leftRetractBeam->Show();
				leftRetractBeam->Activate( true );
			}
			if ( leftDamageBeam.IsValid() ) {
				leftDamageBeam->Activate( false );
				leftDamageBeam->Hide();
			}
			AI_LEFT_DAMAGED = true;
		}
		if ( !AI_RIGHT_DAMAGED && rightRecharger.IsValid() && rightRecharger->GetHealth() <= 0 ) {
			//recharger has died so start retracting the beams and schedule the gun's death
			StartSound( "snd_gun_predeath", SND_CHANNEL_ANY );
			SAFE_REMOVE( rightRechargeBeam );
			PostEventSec( &MA_RightGunDeath, spawnArgs.GetFloat("retract_delay", "1") );
			const char *defName = spawnArgs.GetString( "fx_retract" );
			if ( defName && defName[0] && leftDamageBeam.IsValid() ) {
				hhFxInfo fxInfo;
				fxInfo.RemoveWhenDone( true );
				retractRightFx = SpawnFxLocal( defName, rightDamageBeam->GetTargetLocation(), mat3_identity, &fxInfo, gameLocal.isClient );
			}
			if ( rightRetractBeam.IsValid() ) {
				rightRetractBeam->Show();
				rightRetractBeam->Activate( true );
			}
			if ( rightDamageBeam.IsValid() ) {
				rightDamageBeam->Activate( false );
				rightDamageBeam->Hide();
			}
			AI_RIGHT_DAMAGED = true;
		}
		if ( AI_HEALTH_TICK && gameLocal.time >= nextHealthTick ) {
			health += spawnArgs.GetInt( "recharge_delta", "1" );
			nextHealthTick += spawnArgs.GetInt( "recharge_period" );
			if ( health > spawnArgs.GetInt( "health" ) ) {
				AI_HEALTH_TICK = false;
				health = spawnArgs.GetInt( "health" );
			}
		}
	} else {
		if ( leftRechargeBeam.IsValid() ) {
			leftRechargeBeam->Activate( false );
		}
		if ( rightRechargeBeam.IsValid() ) {
			rightRechargeBeam->Activate( false );
		}
	}

	if ( AI_LEFT_DAMAGED && gameLocal.time > nextLeftZapTime ) {
		nextLeftZapTime = gameLocal.time + SEC2MS(spawnArgs.GetFloat( "zap_period", "0.3" ));
		for ( int i = 0; i < numBurstBeams; i ++ ) {
			if ( leftBeamList[i].IsValid() && !leftBeamList[i]->IsActivated() ) {
				leftBeamList[i]->Activate( true );
				leftBeamList[i]->SetTargetLocation( leftBeamList[i]->GetOrigin() + spawnArgs.GetFloat( "zap_length", "50" ) * hhUtils::RandomSpreadDir( hhUtils::RandomVector().ToMat3(), 16.0f )  );
				PostEventSec(&MA_EndLeftBeams, spawnArgs.GetFloat( "beam_duration_1", "0.2" ) );
				break;
			}
		}
	}
	if ( AI_RIGHT_DAMAGED && gameLocal.time > nextRightZapTime ) {
		nextRightZapTime = gameLocal.time + SEC2MS(spawnArgs.GetFloat( "zap_period", "0.3" ));
		for ( int i = 0; i < numBurstBeams; i ++ ) {
			if ( rightBeamList[i].IsValid() && !rightBeamList[i]->IsActivated() ) {
				rightBeamList[i]->Activate( true );
				rightBeamList[i]->SetTargetLocation( rightBeamList[i]->GetOrigin() + spawnArgs.GetFloat( "zap_length", "50" ) * hhUtils::RandomSpreadDir( hhUtils::RandomVector().ToMat3(), 16.0f )  );
				PostEventSec(&MA_EndRightBeams, spawnArgs.GetFloat( "beam_duration_1", "0.2" ) );
				break;
			}
		}
	}

	if ( enemy.IsValid() ) {
		//left laser
		//HUMANHEAD jsh PCF 5/12/06 30hz issue with beam movement
		targetAlpha_L += spawnArgs.GetFloat( "laser_move_delta", "0.1" ) * (60.0f * USERCMD_ONE_OVER_HZ);
		if ( targetAlpha_L > 1.0f ) {
			targetAlpha_L = 1.0f;
		}
		targetCurrent_L = targetStart_L + targetAlpha_L * ( targetEnd_L - targetStart_L );
		pastEnemy = GetEnemy()->GetOrigin() + idVec3( 0,0,30 );
		//HUMANHEAD jsh PCF 5/12/06 30hz issue with beam movement
		targetEnd_L += spawnArgs.GetFloat( "laser_track_delta", "0.1" ) * ( pastEnemy - targetEnd_L ) * (60.0f * USERCMD_ONE_OVER_HZ);

		//right laser
		//HUMANHEAD jsh PCF 5/12/06 30hz issue with beam movement
		targetAlpha_R += spawnArgs.GetFloat( "laser_move_delta", "0.1" ) * (60.0f * USERCMD_ONE_OVER_HZ);
		if ( targetAlpha_R > 1.0f ) {
			targetAlpha_R = 1.0f;
		}
		targetCurrent_R = targetStart_R + targetAlpha_R * ( targetEnd_R - targetStart_R );
		pastEnemy = GetEnemy()->GetOrigin() + idVec3( 0,0,30 );
		//HUMANHEAD jsh PCF 5/12/06 30hz issue with beam movement
		targetEnd_R += spawnArgs.GetFloat( "laser_track_delta", "0.1" ) * ( pastEnemy - targetEnd_R ) * (60.0f * USERCMD_ONE_OVER_HZ);
	}

	if ( gameLocal.time > nextBeamTime ) {
		nextBeamTime = gameLocal.time + SEC2MS(spawnArgs.GetFloat( "beam_period", "0.3" ));
		for ( int i = 0; i < leftBeamList.Num(); i ++ ) {
			if ( leftBeamList[i].IsValid() && leftBeamList[i]->IsActivated() ) {
				leftBeamList[i]->SetTargetLocation( leftBeamList[i]->GetOrigin() + spawnArgs.GetFloat( "beam_length", "100" ) * hhUtils::RandomSpreadDir( hhUtils::RandomVector().ToMat3(), 16.0f )  );
			}
		}
		for ( int i = 0; i < rightBeamList.Num(); i ++ ) {
			if ( rightBeamList[i].IsValid() && rightBeamList[i]->IsActivated() ) {
				rightBeamList[i]->SetTargetLocation( rightBeamList[i]->GetOrigin() + spawnArgs.GetFloat( "beam_length", "100" ) * hhUtils::RandomSpreadDir( hhUtils::RandomVector().ToMat3(), 16.0f )  );
			}
		}
	}

	idVec3 boneOrigin, traceEnd;
	idMat3 boneAxis;
	trace_t trace;
	memset(&trace, 0, sizeof(trace));
	if ( !enemy.IsValid() ) {
		return;
	}

	if ( laserRight.IsValid() && bLaserRightActive ) {
		traceEnd = targetCurrent_R + 2000 * (targetCurrent_R - laserRight->GetOrigin()).ToNormal();
		GetJointWorldTransform( spawnArgs.GetString("laser_bone_right"), boneOrigin, boneAxis );
		gameLocal.clip.TracePoint( trace, laserRight->GetOrigin(), traceEnd, MASK_SHOT_RENDERMODEL, this );
		if ( trace.fraction < 1.0f ) {
			if ( preLaserRight.IsValid() ) {
				if ( preLaserRight->IsHidden() ) {
					preLaserRight->Show();
				}
				preLaserRight->Activate( true );
				preLaserRight->SetTargetEntity( laserRight.GetEntity() );
			}
			if ( laserRight->IsHidden() ) {
				laserRight->Show();
			}
			laserRight->SetOrigin( boneOrigin + viewAxis[0] * 60 );
			laserRight->Activate( true );
			laserRight->SetTargetLocation( trace.endpos - viewAxis[1] * 10 );
		}
		float dist = (laserRight->GetTargetLocation() - laserRight->GetOrigin()).Length();
		if ( dist > 100.0f && impactRightFx.IsValid() ) {
			impactRightFx->SetOrigin( laserRight->GetTargetLocation() );
		}
		laserEndRight = trace.endpos;
	} else {
		if ( preLaserRight.IsValid() ) {
			preLaserRight->Activate( false );
		}
		if ( laserRight.IsValid() ) {
			laserRight->Activate( false );
		}
	}

	if ( laserLeft.IsValid() && bLaserLeftActive ) {
		traceEnd = targetCurrent_L + 2000 * (targetCurrent_L - laserLeft->GetOrigin()).ToNormal();
		GetJointWorldTransform( spawnArgs.GetString("laser_bone_left"), boneOrigin, boneAxis );
		gameLocal.clip.TracePoint( trace, laserLeft->GetOrigin(), traceEnd, MASK_SHOT_RENDERMODEL, this );
		if ( trace.fraction < 1.0f ) {
			if ( preLaserLeft.IsValid() ) {
				if ( preLaserLeft->IsHidden() ) {
					preLaserLeft->Show();
				}
				preLaserLeft->Activate( true );
				preLaserLeft->SetTargetEntity( laserLeft.GetEntity() );
			}
			if ( laserLeft->IsHidden() ) {
				laserLeft->Show();
			}
			laserLeft->SetOrigin( boneOrigin + viewAxis[0] * 60 );
			laserLeft->Activate( true );
			laserLeft->SetTargetLocation( trace.endpos + viewAxis[1] * 10 );
		}
		float dist = (laserLeft->GetTargetLocation() - laserLeft->GetOrigin()).Length();
		if ( dist > 100.0f && impactLeftFx.IsValid() ) {
			impactLeftFx->SetOrigin( laserLeft->GetTargetLocation() );
		}
		laserEndLeft = trace.endpos;
	} else {
		if ( preLaserLeft.IsValid() ) {
			preLaserLeft->Activate( false );
		}
		if ( laserLeft.IsValid() ) {
			laserLeft->Activate( false );
		}
	}
}

#define LinkScriptVariable( name )	name.LinkTo( scriptObject, #name )
void hhCreatureX::LinkScriptVariables() {
	hhMonsterAI::LinkScriptVariables();
	LinkScriptVariable( AI_GUN_TRACKING );
	LinkScriptVariable( AI_RECHARGING );
	LinkScriptVariable( AI_LEFT_FIRE );
	LinkScriptVariable( AI_RIGHT_FIRE );
	LinkScriptVariable( AI_LEFT_DAMAGED );
	LinkScriptVariable( AI_RIGHT_DAMAGED );
	LinkScriptVariable( AI_FIRING_LASER );
	LinkScriptVariable( AI_HEALTH_TICK );
	LinkScriptVariable( AI_GUN_EXPLODE );
}

void hhCreatureX::Event_RightGunDeath() {
	bool explodeGun = false;
	if ( rightRetractBeam.IsValid() ) {
		rightRetractBeam->Show();
		rightRetractBeam->Activate( true );

		//retract the damage beam a little bit
		idVec3 bonePos;
		idMat3 boneAxis;
		GetJointWorldTransform( spawnArgs.GetString( "gun_bone_right" ), bonePos, boneAxis );
		idVec3 lastTargetLocation = rightRetractBeam->GetTargetLocation();
		rightRetractBeam->SetTargetEntity( NULL );
		if ( ai_debugBrain.GetBool() ) {
			gameRenderWorld->DebugArrow( colorRed, lastTargetLocation, bonePos, 10, 1 );
		}
		rightRetractBeam->SetTargetLocation( lastTargetLocation + spawnArgs.GetFloat("retract_speed", "50") * (bonePos - lastTargetLocation).ToNormal() );
		if ( retractRightFx.IsValid() ) {
			retractRightFx->SetOrigin( rightRetractBeam->GetTargetLocation() );
		}
		if ( (lastTargetLocation - bonePos).LengthFast() < 30.0f ) {
			explodeGun = true;
			rightRetractBeam->Activate( false );
		} else {
			PostEventSec( &MA_RightGunDeath, 0.01f );
		}
	}

	if ( explodeGun || !rightRetractBeam.IsValid() ) {
		CancelEvents( &MA_ResetRechargeBeam );
		SAFE_REMOVE( rightRechargeBeam );
		SAFE_REMOVE( retractRightFx );
		for ( int i=0;i<numBurstBeams;i++ ) {
			if ( rightBeamList[i].IsValid() ) {
				rightBeamList[i]->Activate( true );
				rightBeamList[i]->SetTargetLocation( rightBeamList[i]->GetOrigin() + spawnArgs.GetFloat( "beam_length", "100" ) * hhUtils::RandomSpreadDir( GetEnemy()->GetOrigin().ToMat3(), 16.0f )  );
			}
		}
		StartSound( "snd_jenny_scream", SND_CHANNEL_ANY );
		PostEventSec(&MA_EndRightBeams, spawnArgs.GetFloat( "beam_duration_3", "1.0" ) );
		BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_smoke"), spawnArgs.GetString("damage_bone_right"), NULL );
		BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_gun_death"), spawnArgs.GetString("damage_bone_right"), NULL );
		AI_RIGHT_DAMAGED = true;
		PostEventSec(&MA_SparkLeft, spawnArgs.GetFloat( "spark_freq", "1" ) + gameLocal.random.RandomFloat() );
		if ( AI_LEFT_DAMAGED ) {
			SetSkinByName( spawnArgs.GetString( "skin_nogun_both" ) );
		} else {
			SetSkinByName( spawnArgs.GetString( "skin_nogun_right" ) );
		}
	}
}

void hhCreatureX::Event_LeftGunDeath() {
	bool explodeGun = false;
	if ( leftRetractBeam.IsValid() ) {
		leftRetractBeam->Show();
		leftRetractBeam->Activate( true );

		//retract the damage beam a little bit
		idVec3 bonePos;
		idMat3 boneAxis;
		GetJointWorldTransform( spawnArgs.GetString( "gun_bone_left" ), bonePos, boneAxis );
		idVec3 lastTargetLocation = leftRetractBeam->GetTargetLocation();
		leftRetractBeam->SetTargetEntity( NULL );
		if ( ai_debugBrain.GetBool() ) {
			gameRenderWorld->DebugArrow( colorRed, lastTargetLocation, bonePos, 10, 1 );
		}
		leftRetractBeam->SetTargetLocation( lastTargetLocation + spawnArgs.GetFloat("retract_speed", "50") * (bonePos - lastTargetLocation).ToNormal() );
		if ( retractLeftFx.IsValid() ) {
			retractLeftFx->SetOrigin( leftRetractBeam->GetTargetLocation() );
		}
		if ( (lastTargetLocation - bonePos).LengthFast() < 30.0f ) {
			explodeGun = true;
			leftRetractBeam->Activate( false );
		} else {
			PostEventSec( &MA_LeftGunDeath, 0.01f );
		}
	}

	if ( explodeGun || !leftRetractBeam.IsValid() ) {
		CancelEvents( &MA_ResetRechargeBeam );
		SAFE_REMOVE( leftRechargeBeam );
		SAFE_REMOVE( retractLeftFx );
		for ( int i=0;i<numBurstBeams;i++ ) {
			if ( leftBeamList[i].IsValid() ) {
				leftBeamList[i]->Activate( true );
				leftBeamList[i]->SetTargetLocation( leftBeamList[i]->GetOrigin() + spawnArgs.GetFloat( "beam_length", "100" ) * hhUtils::RandomSpreadDir( GetEnemy()->GetOrigin().ToMat3(), 16.0f )  );
			}
		}
		StartSound( "snd_jenny_scream", SND_CHANNEL_ANY );
		PostEventSec(&MA_EndLeftBeams, spawnArgs.GetFloat( "beam_duration_3", "1.0" ) );
		BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_smoke"), spawnArgs.GetString("damage_bone_left"), NULL );
		BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_gun_death"), spawnArgs.GetString("damage_bone_left"), NULL );
		AI_LEFT_DAMAGED = true;
		PostEventSec(&MA_SparkLeft, spawnArgs.GetFloat( "spark_freq", "1" ) + gameLocal.random.RandomFloat() );
		if ( AI_RIGHT_DAMAGED ) {
			SetSkinByName( spawnArgs.GetString( "skin_nogun_both" ) );
		} else {
			SetSkinByName( spawnArgs.GetString( "skin_nogun_left" ) );
		}
	}
}


bool hhCreatureX::UpdateAnimationControllers( void ) {
	idVec3		local;
	idVec3		focusPos;
	idVec3		left;
	idVec3 		dir;
	idVec3 		orientationJointPos;
	idVec3 		localDir;
	idAngles 	newLookAng;
	idAngles	diff;
	idMat3		mat;
	idMat3		axis;
	idMat3		orientationJointAxis;
	idAFAttachment	*headEnt = head.GetEntity();
	idVec3		eyepos;
	idVec3		pos;
	int			i;
	idAngles	jointAng;
	float		orientationJointYaw;

	if ( AI_DEAD ) {

		for (int i = 0; i < jawFlapList.Num(); i++) {
			jawFlapInfo_t &flapInfo = jawFlapList[i];
			animator.ClearJoint( flapInfo.bone );
		}
		animator.ClearJoint( leftEyeJoint );
		animator.ClearJoint( rightEyeJoint );

		return idActor::UpdateAnimationControllers();
	}

	if ( orientationJoint == INVALID_JOINT ) {
		orientationJointAxis = viewAxis;
		orientationJointPos = physicsObj.GetOrigin();
		orientationJointYaw = current_yaw;
	} else {
		GetJointWorldTransform( orientationJoint, gameLocal.time, orientationJointPos, orientationJointAxis );
		orientationJointYaw = orientationJointAxis[ 2 ].ToYaw();
		orientationJointAxis = idAngles( 0.0f, orientationJointYaw, 0.0f ).ToMat3();
	}

	if ( ai_debugBrain.GetBool() ) {
		gameRenderWorld->DebugArrow( colorCyan, orientationJointPos, orientationJointPos + orientationJointAxis[0] * 64.0, 10, 1 );
	}

	if ( focusJoint != INVALID_JOINT ) {
		if ( headEnt ) {
			headEnt->GetJointWorldTransform( focusJoint, gameLocal.time, eyepos, axis );
		} else {
			// JRMMERGE_GRAVAXIS - What about GetGravAxis() are we still using/needing that?
			GetJointWorldTransform( focusJoint, gameLocal.time, eyepos, axis );
		}
		eyeOffset.z = eyepos.z - physicsObj.GetOrigin().z;
	} else {
		eyepos = GetEyePosition();
	}

	if ( headEnt ) {
		CopyJointsFromBodyToHead();
	}

	// Update the IK after we've gotten all the joint positions we need, but before we set any joint positions.
	// Getting the joint positions causes the joints to be updated.  The IK gets joint positions itself (which
	// are already up to date because of getting the joints in this function) and then sets their positions, which
	// forces the heirarchy to be updated again next time we get a joint or present the model.  If IK is enabled,
	// or if we have a seperate head, we end up transforming the joints twice per frame.  Characters with no
	// head entity and no ik will only transform their joints once.  Set g_debuganim to the current entity number
	// in order to see how many times an entity transforms the joints per frame.
	idActor::UpdateAnimationControllers();

	idEntity *focusEnt = focusEntity.GetEntity();
	//HUMANHEAD jsh allow eyefocus independent from allowJointMod
	if ( ( !allowJointMod && !allowEyeFocus ) || ( gameLocal.time >= focusTime && focusTime != -1 ) || GetPhysics()->GetGravityNormal() != idVec3( 0,0,-1) ) {	
	    focusPos = GetEyePosition() + orientationJointAxis[ 0 ] * 512.0f;
	} else if ( focusEnt == NULL ) {
		// keep looking at last position until focusTime is up
		focusPos = currentFocusPos;
	} else if ( focusEnt == enemy.GetEntity() ) {
		focusPos = lastVisibleEnemyPos + lastVisibleEnemyEyeOffset - eyeVerticalOffset * enemy.GetEntity()->GetPhysics()->GetGravityNormal();
	} else if ( focusEnt->IsType( idActor::Type ) ) {
		focusPos = static_cast<idActor *>( focusEnt )->GetEyePosition() - eyeVerticalOffset * focusEnt->GetPhysics()->GetGravityNormal();
	} else {
		focusPos = focusEnt->GetPhysics()->GetOrigin();
	}

	currentFocusPos = currentFocusPos + ( focusPos - currentFocusPos ) * eyeFocusRate;

	// determine yaw from origin instead of from focus joint since joint may be offset, which can cause us to bounce between two angles
	dir = focusPos - orientationJointPos;
	newLookAng.yaw = idMath::AngleNormalize180( dir.ToYaw() - orientationJointYaw );
	newLookAng.roll = 0.0f;
	newLookAng.pitch = 0.0f;

	newLookAng += lookOffset;

#if 0
	gameRenderWorld->DebugLine( colorRed, orientationJointPos, focusPos, gameLocal.msec );
	gameRenderWorld->DebugLine( colorYellow, orientationJointPos, orientationJointPos + orientationJointAxis[ 0 ] * 32.0f, gameLocal.msec );
	gameRenderWorld->DebugLine( colorGreen, orientationJointPos, orientationJointPos + newLookAng.ToForward() * 48.0f, gameLocal.msec );
#endif

//JRMMERGE_GRAVAXIS: This changed to much to merge, see if you can get your monsters on planets changes back in here.  I'll leave both versions
#if OLD_CODE
	GetGravViewAxis().ProjectVector( dir, localDir ); // HUMANHEAD JRM: VIEWAXIS_TO_GETGRAVVIEWAXIS
	lookAng.yaw		= idMath::AngleNormalize180( localDir.ToYaw() );
	lookAng.pitch	= -idMath::AngleNormalize180( localDir.ToPitch() );
	lookAng.roll	= 0.0f;
#else
	// determine pitch from joint position
	dir = focusPos - eyepos;
	if ( ai_debugBrain.GetBool() ) {
		gameRenderWorld->DebugArrow( colorYellow, eyepos, eyepos + dir, 10, 1 );
	}
	dir.NormalizeFast();
	orientationJointAxis.ProjectVector( dir, localDir );
	newLookAng.pitch = -idMath::AngleNormalize180( localDir.ToPitch() ) + lookOffset.pitch;
	newLookAng.roll	= 0.0f;
#endif

	diff = newLookAng - lookAng;
	
	if ( eyeAng != diff ) {
		eyeAng = diff;
		eyeAng.Clamp( eyeMin, eyeMax );
		idAngles angDelta = diff - eyeAng;
		if ( !angDelta.Compare( ang_zero, 0.1f ) ) {
			alignHeadTime = gameLocal.time;
		} else {
			alignHeadTime = gameLocal.time + ( 0.5f + 0.5f * gameLocal.random.RandomFloat() ) * focusAlignTime;
		}
	}

	if ( idMath::Fabs( newLookAng.yaw ) < 0.1f ) {
		alignHeadTime = gameLocal.time;
	}

	if ( ( gameLocal.time >= alignHeadTime ) || ( gameLocal.time < forceAlignHeadTime ) ) {
		alignHeadTime = gameLocal.time + ( 0.5f + 0.5f * gameLocal.random.RandomFloat() ) * focusAlignTime;
		destLookAng = newLookAng;
		destLookAng.Clamp( lookMin, lookMax );
	}

	diff = destLookAng - lookAng;
	if ( ( lookMin.pitch == -180.0f ) && ( lookMax.pitch == 180.0f ) ) {
		if ( ( diff.pitch > 180.0f ) || ( diff.pitch <= -180.0f ) ) {
			diff.pitch = 360.0f - diff.pitch;
		}
	}
	if ( ( lookMin.yaw == -180.0f ) && ( lookMax.yaw == 180.0f ) ) {
		if ( diff.yaw > 180.0f ) {
			diff.yaw -= 360.0f;
		} else if ( diff.yaw <= -180.0f ) {
			diff.yaw += 360.0f;
		}
	}
	lookAng = lookAng + diff * headFocusRate;
	lookAng.Normalize180();

	jointAng.roll = 0.0f;
	if ( allowJointMod ) {
		for( i = 0; i < lookJoints.Num(); i++ ) {
			jointAng.pitch	= lookAng.pitch * lookJointAngles[ i ].pitch;
			jointAng.yaw	= lookAng.yaw * lookJointAngles[ i ].yaw;
			animator.SetJointAxis( lookJoints[ i ], JOINTMOD_WORLD, jointAng.ToMat3() );
		}
	}

	if ( move.moveType == MOVETYPE_FLY ) {
		// lean into turns
		AdjustFlyingAngles();
	}
	
	if ( headEnt ) {
		idAnimator *headAnimator = headEnt->GetAnimator();

		// HUMANHEAD pdm: Added support for look joints in head entities
		if ( allowJointMod ) {
			for( i = 0; i < headLookJoints.Num(); i++ ) {
				jointAng.pitch	= lookAng.pitch * headLookJointAngles[ i ].pitch;
				jointAng.yaw	= lookAng.yaw * headLookJointAngles[ i ].yaw;
				headAnimator->SetJointAxis( headLookJoints[ i ], JOINTMOD_WORLD, jointAng.ToMat3() );
			}
		}
		// HUMANHEAD END

		if ( allowEyeFocus ) {
			idMat3 eyeAxis = ( lookAng + eyeAng ).ToMat3(); idMat3 headTranspose = headEnt->GetPhysics()->GetAxis().Transpose();
			axis =  eyeAxis * orientationJointAxis;
			left = axis[ 1 ] * eyeHorizontalOffset;
			eyepos -= headEnt->GetPhysics()->GetOrigin();
			headAnimator->SetJointPos( leftEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos + ( axis[ 0 ] * 64.0f + left ) * headTranspose );
			headAnimator->SetJointPos( rightEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos + ( axis[ 0 ] * 64.0f - left ) * headTranspose );

			//if ( ai_debugMove.GetBool() ) {
			//	gameRenderWorld->DebugLine( colorRed, orientationJointPos, eyepos + ( axis[ 0 ] * 64.0f + left ) * headTranspose, gameLocal.msec );
			//}
		} else {
			headAnimator->ClearJoint( leftEyeJoint );
			headAnimator->ClearJoint( rightEyeJoint );
		}
	} else {
		if ( allowEyeFocus ) {
			idMat3 eyeAxis = ( lookAng + eyeAng ).ToMat3();
			axis =  eyeAxis * orientationJointAxis;
			left = axis[ 1 ] * eyeHorizontalOffset;
			eyepos += axis[ 0 ] * 64.0f - physicsObj.GetOrigin();
			animator.SetJointPos( leftEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos + left );
			animator.SetJointPos( rightEyeJoint, JOINTMOD_WORLD_OVERRIDE, eyepos - left );
		} else {
			animator.ClearJoint( leftEyeJoint );
			animator.ClearJoint( rightEyeJoint );
		}
	}

	//HUMANHEAD pdm jawflap
	hhAnimator *theAnimator;
	if (head.IsValid()) {
		theAnimator = head->GetAnimator();
	}
	else {
		theAnimator = GetAnimator();
	}
	JawFlap(theAnimator);
	//END HUMANHEAD

	//update guns
	//HUMANHEAD jsh PCF 4/27/06 no gun tracking
	//if ( enemy.IsValid() ) {
	//	if ( AI_GUN_TRACKING && !AI_DEAD && !AI_RECHARGING ) {
	//		idVec3 focusPos, bonePos;
	//		idMat3 boneAxis;

	//		idAngles ang = ang_zero;
	//		ang.pitch = (bonePos - enemy->GetOrigin()).ToPitch();
	//		ang.pitch += gunShake.pitch;
	//		ang.yaw = (targetCurrent_L-bonePos).ToYaw() - viewAxis.ToAngles().yaw;
	//		ang.yaw += gunShake.yaw;
	//		GetJointWorldTransform( spawnArgs.GetString( "gun_bone_left" ), bonePos, boneAxis );
	//		animator.SetJointAxis( animator.GetJointHandle( spawnArgs.GetString( "gun_bone_left" ) ), JOINTMOD_WORLD, idAngles( (bonePos - targetCurrent_L).ToPitch(), (targetCurrent_L-bonePos).ToYaw() - viewAxis.ToAngles().yaw, 0.0f ).ToMat3() );

	//		ang.yaw = (targetCurrent_R-bonePos).ToYaw() - viewAxis.ToAngles().yaw;
	//		ang.yaw += gunShake.yaw;
	//		GetJointWorldTransform( spawnArgs.GetString( "gun_bone_right" ), bonePos, boneAxis );				
	//		animator.SetJointAxis( animator.GetJointHandle( spawnArgs.GetString( "gun_bone_right" ) ), JOINTMOD_WORLD, idAngles( (bonePos - targetCurrent_R).ToPitch(), (targetCurrent_R-bonePos).ToYaw() - viewAxis.ToAngles().yaw, 0.0f ).ToMat3() );
	//	} else {
	//		animator.SetJointAxis( animator.GetJointHandle( spawnArgs.GetString( "gun_bone_left" ) ), JOINTMOD_WORLD, mat3_identity );
	//		animator.SetJointAxis( animator.GetJointHandle( spawnArgs.GetString( "gun_bone_right" ) ), JOINTMOD_WORLD, mat3_identity );
	//	}
	//}

	return true;
}

void hhCreatureX::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	HandleNoGore();

	//stop sparking upon death
	CancelEvents( &MA_SparkLeft );
	CancelEvents( &MA_SparkRight );
	SAFE_REMOVE( preLaserLeft );
	SAFE_REMOVE( preLaserRight );
	AI_LEFT_DAMAGED = false;
	AI_RIGHT_DAMAGED = false;

	if ( laserLeft.IsValid() ) {
		laserLeft->Hide();
	}
	if ( laserRight.IsValid() ) {
		laserRight->Hide();
	}

	if ( spawnArgs.GetBool( "use_death_point", "0" ) ) {
		if ( !AI_DEAD ) {
			fl.takedamage = false;
			AI_DEAD = true;
			state = GetScriptFunction( "state_Predeath" );
			SetState( state );
			SetWaitState( "" );
			return;
		}
	}

	if ( AI_DEAD ) {
		AI_DAMAGE = true;
		return;
	}

	fl.takedamage = false;
	idAngles ang;
	const char *modelDeath;

	// make sure the monster is activated
	EndAttack();

	if ( g_debugDamage.GetBool() ) {
		gameLocal.Printf( "Damage: joint: '%s', zone '%s'\n", animator.GetJointName( ( jointHandle_t )location ), 
			GetDamageGroup( location ) );
	}

	if ( inflictor ) {
		AI_SPECIAL_DAMAGE = inflictor->spawnArgs.GetInt( "special_damage" );
	} else {
		AI_SPECIAL_DAMAGE = 0;
	}

	if ( AI_DEAD ) {
		AI_PAIN = true;
		AI_DAMAGE = true;
		return;
	}

	// stop all voice sounds
	StopSound( SND_CHANNEL_VOICE, false );
	if ( head.GetEntity() ) {
		head.GetEntity()->StopSound( SND_CHANNEL_VOICE, false );
		head.GetEntity()->GetAnimator()->ClearAllAnims( gameLocal.time, 100 );
	}

	disableGravity = false;
	move.moveType = MOVETYPE_DEAD;
	af_push_moveables = false;

	physicsObj.UseFlyMove( false );
	physicsObj.ForceDeltaMove( false );

	// end our looping ambient sound
	StopSound( SND_CHANNEL_AMBIENT, false );

	if ( attacker && attacker->IsType( idActor::Type ) ) {
		gameLocal.AlertAI( ( idActor * )attacker );
	}

	// activate targets
	ActivateTargets( attacker );

	RemoveAttachments();
	RemoveProjectile();
	StopMove( MOVE_STATUS_DONE );

	ClearEnemy();
	AI_DEAD	= true;

	// HUMANHEAD jsh commented out for girlfriendx
	// make monster nonsolid
	if ( spawnArgs.GetBool( "boss" ) ) {
		physicsObj.SetContents( 0 );
		physicsObj.GetClipModel()->Unlink();
	}

	Unbind();

	// spawn death clip model
	if ( spawnArgs.GetBool( "boss" ) ) {
		idDict dict;
		const idDict *torsoDict = gameLocal.FindEntityDefDict(spawnArgs.GetString("def_deathclip"));
		dict.Copy(*torsoDict);
		dict.SetVector("origin", GetOrigin() + modelOffset * GetAxis() );
		dict.SetMatrix("rotation", GetAxis());
		idEntity *e;
		gameLocal.SpawnEntityDef(dict, &e);
		if ( e ) {
			e->GetPhysics()->SetContents( CONTENTS_PLAYERCLIP );
		}
	}

	if ( StartRagdoll() ) {
		StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
	// HUMANHEAD JRM - some monsters are removed, but always need to play sound
	} else if(spawnArgs.GetBool("death_sound_always","0")) { 
		StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
	}
	// HUMANHEAD JRM - end
	
	if ( spawnArgs.GetString( "model_death", "", &modelDeath ) ) {
		// lost soul is only case that does not use a ragdoll and has a model_death so get the death sound in here
		StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
		renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
		SetModel( modelDeath );
		physicsObj.SetLinearVelocity( vec3_zero );
		physicsObj.PutToRest();
		physicsObj.DisableImpact();
	}

	restartParticles = false;

	state = GetScriptFunction( "state_Killed" );
	SetState( state );
	SetWaitState( "" );

	if ( attacker && attacker->IsType( idPlayer::Type ) ) {
		static_cast< idPlayer* >( attacker )->AddAIKill();
	}

	// General non-item dropping (for monsters, souls, etc.)
	const idKeyValue *kv = NULL;
	kv = spawnArgs.MatchPrefix( "def_drops", NULL );
	while ( kv ) {

		idStr drops = kv->GetValue();			
		idDict args;
						
		idStr last5 = kv->GetKey().Right(5);
		if ( drops.Length() && idStr::Icmp( last5, "Joint" ) != 0) {
			args.Set( "classname", drops );

			// HUMANHEAD pdm: specify monster so souls can call back to remove body when picked up
			args.Set("monsterSpawnedBy", name.c_str());

			idVec3 origin;
			idMat3 axis;			
			idStr jointKey = kv->GetKey() + idStr("Joint");
			idStr jointName = spawnArgs.GetString( jointKey );
			idStr joint2JointKey = kv->GetKey() + idStr("Joint2Joint");
			idStr j2jName = spawnArgs.GetString( joint2JointKey );			
			
			idEntity *newEnt = NULL;
			gameLocal.SpawnEntityDef( args, &newEnt );
			HH_ASSERT(newEnt != NULL);

			if(jointName.Length()) {
				jointHandle_t joint = GetAnimator()->GetJointHandle( jointName );
				if (!GetAnimator()->GetJointTransform( joint, gameLocal.time, origin, axis ) ) {
					gameLocal.Printf( "%s refers to invalid joint '%s' on entity '%s'\n", (const char*)jointKey.c_str(), (const char*)jointName, (const char*)name );
					origin = renderEntity.origin;
					axis = renderEntity.axis;
				}
				axis *= renderEntity.axis;
				origin = renderEntity.origin + origin * renderEntity.axis;
				newEnt->SetAxis(axis);
				newEnt->SetOrigin(origin);
			}
			else {
				
				newEnt->SetAxis(viewAxis);
				newEnt->SetOrigin(GetOrigin());
			}

		}
		
		kv = spawnArgs.MatchPrefix( "def_drops", kv );
	}
}

/*
=====================
hhCreatureX::Save
=====================
*/
void hhCreatureX::Save( idSaveGame *savefile ) const {
	laserRight.Save( savefile );
	laserLeft.Save( savefile );
	savefile->WriteBool( bLaserLeftActive );
	savefile->WriteBool( bLaserRightActive );

	int i, num = leftBeamList.Num();
	savefile->WriteInt( num );
	for ( i = 0; i < num; i++ ) {
		leftBeamList[i].Save( savefile );
	}

	num = rightBeamList.Num();
	savefile->WriteInt( num );
	for ( i = 0; i < num; i++ ) {
		rightBeamList[i].Save( savefile );
	}

	savefile->WriteInt( numBurstBeams );
	savefile->WriteInt( leftGunHealth );
	savefile->WriteInt( rightGunHealth );
	savefile->WriteAngles( gunShake );
	savefile->WriteInt( nextBeamTime );
	savefile->WriteInt( nextLeftZapTime );
	savefile->WriteInt( nextRightZapTime );
	savefile->WriteInt( leftGunLives );
	savefile->WriteInt( rightGunLives );

	savefile->WriteVec3( targetStart_L );
	savefile->WriteVec3( targetEnd_L );
	savefile->WriteVec3( targetCurrent_L );
	savefile->WriteVec3( targetStart_R );
	savefile->WriteVec3( targetEnd_R );
	savefile->WriteVec3( targetCurrent_R );

	savefile->WriteFloat( targetAlpha_L );
	savefile->WriteFloat( targetAlpha_R );

	savefile->WriteVec3( laserEndLeft );
	savefile->WriteVec3( laserEndRight );

	savefile->WriteInt( nextLaserLeft );
	savefile->WriteInt( nextLaserRight );
	savefile->WriteInt( nextHealthTick );

	leftRecharger.Save( savefile );
	rightRecharger.Save( savefile );

	leftRechargeBeam.Save( savefile );
	rightRechargeBeam.Save( savefile );

	leftDamageBeam.Save( savefile );
	rightDamageBeam.Save( savefile );

	leftRetractBeam.Save( savefile );
	rightRetractBeam.Save( savefile );

	preLaserLeft.Save( savefile );
	preLaserRight.Save( savefile );

	muzzleLeftFx.Save( savefile );
	muzzleRightFx.Save( savefile );
	impactLeftFx.Save( savefile );
	impactRightFx.Save( savefile );
	rechargeLeftFx.Save( savefile );
	rechargeRightFx.Save( savefile );
	retractLeftFx.Save( savefile );
	retractRightFx.Save( savefile );

	savefile->WriteInt( leftRechargerHealth );
	savefile->WriteInt( rightRechargerHealth );
	savefile->WriteBool( bScripted );
}

/*
=====================
hhCreatureX::Restore
=====================
*/
void hhCreatureX::Restore( idRestoreGame *savefile ) {
	laserRight.Restore( savefile );
	laserLeft.Restore( savefile );
	savefile->ReadBool( bLaserLeftActive );
	savefile->ReadBool( bLaserRightActive );

	int i, num;
	savefile->ReadInt( num );
	leftBeamList.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		leftBeamList[i].Restore( savefile );
	}

	savefile->ReadInt( num );
	rightBeamList.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		rightBeamList[i].Restore( savefile );
	}

	savefile->ReadInt( numBurstBeams );
	savefile->ReadInt( leftGunHealth );
	savefile->ReadInt( rightGunHealth );
	savefile->ReadAngles( gunShake );
	savefile->ReadInt( nextBeamTime );
	savefile->ReadInt( nextLeftZapTime );
	savefile->ReadInt( nextRightZapTime );
	savefile->ReadInt( leftGunLives );
	savefile->ReadInt( rightGunLives );

	savefile->ReadVec3( targetStart_L );
	savefile->ReadVec3( targetEnd_L );
	savefile->ReadVec3( targetCurrent_L );
	savefile->ReadVec3( targetStart_R );
	savefile->ReadVec3( targetEnd_R );
	savefile->ReadVec3( targetCurrent_R );

	savefile->ReadFloat( targetAlpha_L );
	savefile->ReadFloat( targetAlpha_R );

	savefile->ReadVec3( laserEndLeft );
	savefile->ReadVec3( laserEndRight );

	savefile->ReadInt( nextLaserLeft );
	savefile->ReadInt( nextLaserRight );
	savefile->ReadInt( nextHealthTick );

	leftRecharger.Restore( savefile );
	rightRecharger.Restore( savefile );

	leftRechargeBeam.Restore( savefile );
	rightRechargeBeam.Restore( savefile );

	leftDamageBeam.Restore( savefile );
	rightDamageBeam.Restore( savefile );

	leftRetractBeam.Restore( savefile );
	rightRetractBeam.Restore( savefile );

	preLaserLeft.Restore( savefile );
	preLaserRight.Restore( savefile );

	muzzleLeftFx.Restore( savefile );
	muzzleRightFx.Restore( savefile );
	impactLeftFx.Restore( savefile );
	impactRightFx.Restore( savefile );
	rechargeLeftFx.Restore( savefile );
	rechargeRightFx.Restore( savefile );
	retractLeftFx.Restore( savefile );
	retractRightFx.Restore( savefile );

	savefile->ReadInt( leftRechargerHealth );
	savefile->ReadInt( rightRechargerHealth );
	savefile->ReadBool( bScripted );
}


void hhCreatureX::MuzzleLeftOn() {
	if ( !muzzleLeftFx.IsValid() ) {
		BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_muzzle"), spawnArgs.GetString("muzzle_bone_left"), NULL, &MA_AssignLeftMuzzleFx, false );
	}
	if ( !impactLeftFx.IsValid() ) {
		BroadcastFxInfo( spawnArgs.GetString("fx_impact"), GetOrigin(), GetAxis(), NULL, &MA_AssignLeftImpactFx, false );
	}
}

void hhCreatureX::MuzzleLeftOff() {
	SAFE_REMOVE( muzzleLeftFx );
	SAFE_REMOVE( impactLeftFx );
}

void hhCreatureX::Event_AssignLeftMuzzleFx( hhEntityFx* fx ) {
	muzzleLeftFx = fx;
}

void hhCreatureX::Event_AssignRightMuzzleFx( hhEntityFx* fx ) {
	muzzleRightFx = fx;
}

void hhCreatureX::Event_AssignRightImpactFx( hhEntityFx* fx ) {
	impactRightFx = fx;
}

void hhCreatureX::Event_AssignLeftImpactFx( hhEntityFx* fx ) {
	impactLeftFx = fx;
}


void hhCreatureX::Event_AssignLeftRechargeFx( hhEntityFx* fx ) {
	rechargeLeftFx = fx;
}

void hhCreatureX::Event_AssignRightRechargeFx( hhEntityFx* fx ) {
	rechargeRightFx= fx;
}

void hhCreatureX::MuzzleRightOn() {
	if ( !muzzleRightFx.IsValid() ) {
		BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_muzzle"), spawnArgs.GetString("muzzle_bone_right"), NULL, &MA_AssignRightMuzzleFx, false );
	}
	if ( !impactRightFx.IsValid() ) {
		BroadcastFxInfo( spawnArgs.GetString("fx_impact"), GetOrigin(), GetAxis(), NULL, &MA_AssignRightImpactFx, false );
	}
}

void hhCreatureX::MuzzleRightOff() {
	SAFE_REMOVE( muzzleRightFx );
	SAFE_REMOVE( impactRightFx );
}

void hhCreatureX::Event_SetGunOffset( const idAngles &ang ) {
	gunShake = ang;
}

void hhCreatureX::Event_EndLeftBeams() {
	for ( int i = 0; i < numBurstBeams; i ++ ) {
		if( leftBeamList[i].IsValid() ) {
			leftBeamList[i]->Activate( false );
		}
	}
}

void hhCreatureX::Event_EndRightBeams() {
	for ( int i = 0; i < numBurstBeams; i ++ ) {
		if( rightBeamList[i].IsValid() ) {
			rightBeamList[i]->Activate( false );
		}
	}
}

void hhCreatureX::Event_StartRechargeBeams() {
	StartSound( "snd_recharge_beam_start", SND_CHANNEL_ANY );
	if ( leftRecharger.IsValid() ) {
		const char *defName = spawnArgs.GetString( "fx_recharge_beam_start" );
		if (defName && defName[0]) {
			hhFxInfo fxInfo;
			fxInfo.RemoveWhenDone( true );
			idEntityFx *rechargeFx = SpawnFxLocal( defName, leftRecharger->GetOrigin(), leftRecharger->GetAxis(), &fxInfo, gameLocal.isClient );
			if ( rechargeFx ) {
				rechargeFx->Bind( leftRecharger.GetEntity(), true );
			}
		}
		if ( leftRechargeBeam.IsValid() ) {
			leftRechargeBeam->Activate( true );
			leftRechargeBeam->SetTargetEntity( leftRecharger.GetEntity(), 0, leftRecharger->spawnArgs.GetVector( "offsetModel" ) );
		}
	}
	if ( rightRecharger.IsValid() ) {
		const char *defName = spawnArgs.GetString( "fx_recharge_beam_start" );
		if (defName && defName[0]) {
			hhFxInfo fxInfo;
			fxInfo.RemoveWhenDone( true );
			idEntityFx *rechargeFx = SpawnFxLocal( defName, rightRecharger->GetOrigin(), rightRecharger->GetAxis(), &fxInfo, gameLocal.isClient );
			if ( rechargeFx ) {
				rechargeFx->Bind( rightRecharger.GetEntity(), true );
			}
		}

		if ( rightRechargeBeam.IsValid() ) {
			rightRechargeBeam->Activate( true );
			rightRechargeBeam->SetTargetEntity( rightRecharger.GetEntity(), 0, rightRecharger->spawnArgs.GetVector( "offsetModel" ) );
		}
	}		
}

void hhCreatureX::Event_GunRecharge( int onOff ) {
	if ( onOff ) {
		idVec3 boneOrigin;
		idMat3 boneAxis;
		GetJointWorldTransform( spawnArgs.GetString("laser_bone_right"), boneOrigin, boneAxis );
		BroadcastFxInfo( spawnArgs.GetString("fx_recharger_enter"), boneOrigin, boneAxis, NULL, NULL, false );

		GetJointWorldTransform( spawnArgs.GetString("laser_bone_left"), boneOrigin, boneAxis );
		BroadcastFxInfo( spawnArgs.GetString("fx_recharger_enter"), boneOrigin, boneAxis, NULL, NULL, false );

		PostEventSec( &MA_StartRechargeBeams, spawnArgs.GetFloat( "recharge_delay", "0.9" ) );
		if ( leftRecharger.IsValid() ) {
			if ( leftRetractBeam.IsValid() ) {
				leftRetractBeam->SetTargetEntity( leftRecharger.GetEntity(), 0, leftRecharger->spawnArgs.GetVector( "offsetModel" ) );
			}
			leftRechargerHealth = leftRecharger->GetHealth();
			if ( leftDamageBeam.IsValid() ) {
				leftDamageBeam->SetTargetEntity( leftRecharger.GetEntity(), 0, leftRecharger->spawnArgs.GetVector( "offsetModel" ) );
			}
			leftRecharger->Show();
			leftRecharger->SetOrigin( GetOrigin() );
			idVec3 offset = spawnArgs.GetVector( "healer_offset", "0 80 60" );
			leftRecharger->MoveToPosition( GetOrigin() + viewAxis * offset );
			leftRecharger->SetState( leftRecharger->GetScriptFunction( "state_Healer" ) );
			leftRecharger->SetWaitState( "" );
		}
		if ( rightRecharger.IsValid() ) {
			if ( rightRetractBeam.IsValid() ) {
				rightRetractBeam->SetTargetEntity( rightRecharger.GetEntity(), 0, rightRecharger->spawnArgs.GetVector( "offsetModel" ) );
			}
			rightRechargerHealth = rightRecharger->GetHealth();
			if ( rightDamageBeam.IsValid() ) {
				rightDamageBeam->SetTargetEntity( rightRecharger.GetEntity(), 0, rightRecharger->spawnArgs.GetVector( "offsetModel" ) );
			}
			rightRecharger->Show();
			idVec3 offset = spawnArgs.GetVector( "healer_offset", "0 80 60" );
			offset.y *= -1;
			rightRecharger->SetOrigin( GetOrigin() );
			rightRecharger->MoveToPosition( GetOrigin() + viewAxis * offset );
			rightRecharger->SetState( rightRecharger->GetScriptFunction( "state_Healer" ) );
			rightRecharger->SetWaitState( "" );
		}
		if ( !AI_LEFT_DAMAGED ) {
			BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_weakpoint"), spawnArgs.GetString("recharge_bone_left"), NULL, &MA_AssignLeftRechargeFx, false );
		}
		if ( !AI_RIGHT_DAMAGED ) {
			BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_weakpoint"), spawnArgs.GetString("recharge_bone_right"), NULL, &MA_AssignRightRechargeFx, false );	
		}
	} else {
		SAFE_REMOVE( rechargeLeftFx );
		SAFE_REMOVE( rechargeRightFx );
	}
}

void hhCreatureX::Event_EndRecharge() {
	if ( leftRechargeBeam.IsValid() ) {
		leftRechargeBeam->Activate( false );
	}
	if ( leftRecharger.IsValid() ) {
		leftRecharger->SetState( leftRecharger->GetScriptFunction( "state_Return" ) );
	}
	if ( rightRechargeBeam.IsValid() ) {
		rightRechargeBeam->Activate( false );
	}
	if ( rightRecharger.IsValid() ) {
		rightRecharger->SetState( rightRecharger->GetScriptFunction( "state_Return" ) );
	}
	if ( rightDamageBeam.IsValid() ) {
		rightDamageBeam->Activate( false );
	}
	if ( leftDamageBeam.IsValid() ) {
		leftDamageBeam->Activate( false );
	}
	CancelEvents( &MA_ResetRechargeBeam );
	CancelEvents( &MA_StartRechargeBeams );
}

//HUMANHEAD jsh PCF 4/27/06 initialized proj and made sure ReturnEntity is called
void hhCreatureX::Event_AttackMissile( const char *jointname, const idDict *projDef, int boneDir ) {
	idProjectile *proj = NULL;

	// Bonedir launch?
	if((BOOL)boneDir) {
		proj = hhProjectile::SpawnProjectile(projDef);
		if ( proj ) {
			idMat3 axis;
			idVec3 muzzle;
			GetMuzzle( jointname, muzzle, axis );
			proj->Create(this, muzzle, axis);
			proj->Launch(muzzle, axis, vec3_zero);		
		}
	}
	else {		
		if ( shootTarget.IsValid() ) {
			proj = LaunchProjectile( jointname, shootTarget.GetEntity(), false, projDef );	//HUMANHEAD mdc - pass projDef on for multiple proj support
		} else {
			proj = LaunchProjectile( jointname, enemy.GetEntity(), false, projDef );	//HUMANHEAD mdc - pass projDef on for multiple proj support
		}
	}

	idThread::ReturnEntity( proj );
}

void hhCreatureX::Event_SparkLeft() {
	BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_spark"), spawnArgs.GetString("damage_bone_left"), NULL );
	PostEventSec(&MA_SparkLeft, spawnArgs.GetFloat( "spark_freq", "1" ) + gameLocal.random.RandomFloat() );
}

void hhCreatureX::Event_SparkRight() {
	BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_spark"), spawnArgs.GetString("damage_bone_right"), NULL );	
	PostEventSec(&MA_SparkRight, spawnArgs.GetFloat( "spark_freq", "1" ) + gameLocal.random.RandomFloat() );
}

void hhCreatureX::Event_HudEvent( const char* eventName ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( player && player->hud ) {
		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent( eventName );
		gameLocal.GetLocalPlayer()->hud->StateChanged(gameLocal.time);
	}
}

void hhCreatureX::Activate( idEntity *activator ) {
	if ( preLaserLeft.IsValid() ) {
		HH_ASSERT(!FLOAT_IS_NAN(preLaserLeft->GetOrigin().x)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(preLaserLeft->GetOrigin().y)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(preLaserLeft->GetOrigin().z)); //Test for bad origin
	}
	if ( preLaserRight.IsValid() ) {
		HH_ASSERT(!FLOAT_IS_NAN(preLaserRight->GetOrigin().x)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(preLaserRight->GetOrigin().y)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(preLaserRight->GetOrigin().z)); //Test for bad origin
	}
	if ( laserLeft.IsValid() ) {
		HH_ASSERT(!FLOAT_IS_NAN(laserLeft->GetOrigin().x)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(laserLeft->GetOrigin().y)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(laserLeft->GetOrigin().z)); //Test for bad origin
	}
	if ( laserRight.IsValid() ) {
		HH_ASSERT(!FLOAT_IS_NAN(laserRight->GetOrigin().x)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(laserRight->GetOrigin().y)); //Test for bad origin
		HH_ASSERT(!FLOAT_IS_NAN(laserRight->GetOrigin().z)); //Test for bad origin
	}

	hhMonsterAI::Activate( activator );
}

void hhCreatureX::Show() {
	hhMonsterAI::Show();
	if ( preLaserLeft.IsValid() ) {
		preLaserLeft->Hide();
		preLaserLeft->Activate( false );
	}
	if ( preLaserRight.IsValid() ) {
		preLaserRight->Hide();
		preLaserRight->Activate( false );
	}
	if ( laserLeft.IsValid() ) {
		laserLeft->Hide();
		laserLeft->Activate( false );
	}
	if ( laserRight.IsValid() ) {
		laserRight->Hide();
		laserRight->Activate( false );
	}
	if ( leftRechargeBeam.IsValid() ) {
		leftRechargeBeam->Hide();
		leftRechargeBeam->Activate( false );
	}
	if ( rightRechargeBeam.IsValid() ) {
		rightRechargeBeam->Hide();
		rightRechargeBeam->Activate( false );
	}
	if ( leftDamageBeam.IsValid() ) {
		leftDamageBeam->Hide();
		leftDamageBeam->Activate( false );
	}
	if ( rightDamageBeam.IsValid() ) {
		rightDamageBeam->Hide();
		rightDamageBeam->Activate( false );
	}
	if ( leftRetractBeam.IsValid() ) {
		leftRetractBeam->Hide();
		leftRetractBeam->Activate( false );
	}
	if ( rightRetractBeam.IsValid() ) {
		rightRetractBeam->Hide();
		rightRetractBeam->Activate( false );
	}
}

void hhCreatureX::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if ( spawnArgs.GetBool( "boss" ) ) {
		if ( attacker && attacker->IsType( idPlayer::Type ) && idStr::Icmp( damageDefName, spawnArgs.GetString( "def_damageTelefrag", "damage_telefrag" ) ) == 0 ) {
			//telefragged so move somewhere nice
			SetState( GetScriptFunction( "state_Telefragged" ) );
			return;
		}
	}

	if ( !fl.takedamage ) {
		return;
	}
	//check for splash damage or jenny-specific damage based on location
	if ( AI_DEAD && spawnArgs.GetBool( "boss" ) ) {
		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
		if ( ( damageDef && damageDef->GetFloat( "radius" ) > 0 ) || 
			 ( location && strcmp( GetDamageGroup( location ), "jenny" ) == 0 ) ) {
			SetState( GetScriptFunction( "state_RealDeath" ) );
			fl.takedamage = false;
		}
	}
	hhMonsterAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build