

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_StartBurst("startBurst");
const idEventDef EV_EndBurst("endBurst");
const idEventDef EV_StartStaticBeam("startStaticBeam");
const idEventDef EV_EndStaticBeam("endStaticBeam");
const idEventDef EV_StartChargeShot("startChargeShot");
const idEventDef EV_EndChargeShot("endChargeShot");
const idEventDef EV_StartPathing("startPathing");
const idEventDef EV_EndPathing("endPathing");
const idEventDef EV_EndZipBeams("<endZipBeams>");
const idEventDef EV_HealerReset("<healerReset>");

CLASS_DECLARATION(hhMonsterAI, hhDroid)
	EVENT(EV_StartStaticBeam,		hhDroid::Event_StartStaticBeam)
	EVENT(EV_EndStaticBeam,			hhDroid::Event_EndStaticBeam)
	EVENT(EV_StartBurst,			hhDroid::Event_StartBurst)
	EVENT(EV_EndBurst,				hhDroid::Event_EndBurst)
	EVENT(EV_StartChargeShot,		hhDroid::Event_StartChargeShot)
	EVENT(EV_EndChargeShot,			hhDroid::Event_EndChargeShot)
	EVENT(EV_StartPathing,			hhDroid::Event_StartPathing)
	EVENT(EV_EndPathing,			hhDroid::Event_EndPathing)
	EVENT(EV_EndZipBeams,			hhDroid::Event_EndZipBeams)
	EVENT(EV_HealerReset,			hhDroid::Event_HealerReset)
END_CLASS

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
hhDroid::hhDroid() {
	burstLength = 0.0;
	burstSpread = 0.0;
	burstDuration = 0.0;
	staticDuration = 0.0;
	staticRange = 0.0;
	numBurstBeams = 0;
	beamOffset = vec3_zero;
	chargeShotSize = 0.0;
	old_fly_bob_strength = 0.0;
	spinAngle = 0.0f;
}

hhDroid::~hhDroid() {
	for ( int i = 0; i < numBurstBeams; i ++ ) {
		SAFE_REMOVE( beamBurstList[i] );
	}
}

void hhDroid::Show( void ) {
	hhMonsterAI::Show();

	PostEventSec(&EV_StartStaticBeam, gameLocal.random.RandomFloat() * spawnArgs.GetFloat( "staticFreq", "2.5" ) );
}

void hhDroid::Spawn(void)
{
	Event_SetMoveType(MOVETYPE_FLY);
	lives = spawnArgs.GetInt( "lives", "3" );
	numBurstBeams = spawnArgs.GetInt( "numBurstBeams", "5" );
	burstLength = spawnArgs.GetFloat( "burstLength", "250" );
	staticDuration = spawnArgs.GetFloat( "staticDuration", "2.0" );
	burstDuration = spawnArgs.GetFloat( "burstDuration", "2.0" );
	beamOffset = spawnArgs.GetVector( "beamOffset", "0 0 0" );
	burstSpread = spawnArgs.GetFloat( "burstSpread", "1.0" );
	enemyRange = spawnArgs.GetFloat( "enemy_range", "3000" );
	flyDampening = spawnArgs.GetFloat( "fly_dampening", "0.01" );
	bHealer = spawnArgs.GetBool( "healer" );

	beamZip = hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamZip" ) );
	if( beamZip.IsValid() ) {
		beamZip->Activate( false );
		beamZip->SetOrigin( GetPhysics()->GetOrigin() + beamOffset );
		beamZip->Bind( this, false );
	}

	for ( int i = 0; i < numBurstBeams; i ++ ) {
		if ( gameLocal.random.RandomFloat() > 0.5f ) {
			beamBurstList.Append( hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamBurst1" ) ) );
		} else {
			beamBurstList.Append( hhBeamSystem::SpawnBeam( vec3_origin, spawnArgs.GetString( "def_beamBurst2" ) ) );
		}
		if( beamBurstList[i].IsValid() ) {
			beamBurstList[i]->Activate( false );
			beamBurstList[i]->SetOrigin( GetPhysics()->GetOrigin() + beamOffset );
			beamBurstList[i]->Bind( this, false );
		}
	}

	PostEventSec(&EV_StartStaticBeam, gameLocal.random.RandomFloat() * spawnArgs.GetFloat( "staticFreq", "2.5" ) );
}

bool hhDroid::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	bool bPain = hhMonsterAI::Pain( inflictor, attacker, damage, dir, location );
	if ( !bPain ) {
		return bPain;
	}

	//activate some beams for damage fx
	for ( int i = 0; i < beamBurstList.Num(); i ++ ) {
		if ( beamBurstList[i].IsValid() ) {
			beamBurstList[i]->Activate( true );
			beamBurstList[i]->SetTargetLocation( beamBurstList[i]->GetOrigin() + burstLength * hhUtils::RandomSpreadDir( dir.ToMat3(), burstSpread )  );
			PostEventSec(&EV_EndBurst, staticDuration);
		}
	}
	return bPain;
}

void hhDroid::Think() {
	PROFILE_SCOPE("AI", PROFMASK_NORMAL|PROFMASK_AI);
	if (ai_skipThink.GetBool()) {
		return;
	}

	hhMonsterAI::Think();

	//update burst beams
	idMat3 dir;
	if ( enemy.IsValid() ) {
		dir = (enemy->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).ToMat3();
	} else {
		dir = GetPhysics()->GetAxis();
	}
	for ( int i = 0; i < beamBurstList.Num(); i ++ ) {
		if ( beamBurstList[i].IsValid() && beamBurstList[i]->IsActivated() ) {
			beamBurstList[i]->SetTargetLocation( beamBurstList[i]->GetOrigin() + burstLength * hhUtils::RandomSpreadDir( hhUtils::RandomVector().ToMat3(), burstSpread )  );
		}
	}

	if ( staticPoint.IsValid() && beamBurstList[0].IsValid() ) {
		beamBurstList[0]->SetTargetLocation( staticPoint->GetPhysics()->GetOrigin() );
	}

	if ( chargeShot.IsValid() ) {
		chargeShot->SetShaderParm( 9, 1 );
		chargeShot->SetShaderParm( 10, chargeShotSize );
		chargeShotSize += spawnArgs.GetFloat( "chargeSizeDelta" );
		chargeShot->GetPhysics()->DisableClip();
	}
}

void hhDroid::Event_StartChargeShot(void) {
	//spawn chargeshot projectile
	const idDict *projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString("def_chargeshot") );
	if ( projectileDef ) {
		chargeShot = hhProjectile::SpawnProjectile( projectileDef );
		if ( chargeShot.IsValid() ) {
			chargeShotSize = spawnArgs.GetFloat( "chargeSizeStart", "1.0" );
			idVec3 launchStart = GetPhysics()->GetOrigin() + spawnArgs.GetVector( "chargeOffset", "15 0 0" ) * GetRenderEntity()->axis;
			chargeShot->Create(this, launchStart, GetRenderEntity()->axis);
			chargeShot->Launch(chargeShot->GetPhysics()->GetOrigin(), chargeShot->GetPhysics()->GetAxis(), vec3_zero );
			chargeShot->Bind(this, true);
		}
	}
}

void hhDroid::Event_EndChargeShot(void) {
	//launch projectile
	if ( chargeShot.IsValid() ) {
		chargeShot->Unbind();
		chargeShot->StartTracking();
		chargeShot.Clear();
	}
}

void hhDroid::Event_StartBurst(void) {
	gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), this, this, this, this, spawnArgs.GetString("def_burstdamage") );

	//burst effect
	idMat3 dir;
	if ( enemy.IsValid() ) {
		dir = (enemy->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).ToMat3();
	} else {
		dir = GetPhysics()->GetAxis();
	}
	for ( int i = 0; i < beamBurstList.Num(); i++ ) {
		if ( beamBurstList[i].IsValid() ) {
			beamBurstList[i]->Activate( true );
			beamBurstList[i]->SetTargetLocation( beamBurstList[i]->GetOrigin() + burstLength * hhUtils::RandomSpreadDir( hhUtils::RandomVector().ToMat3(), burstSpread )  );
		}
	}
	PostEventSec(&EV_EndBurst, burstDuration);
}

void hhDroid::Event_EndBurst(void) {
	for ( int i = 0; i < beamBurstList.Num(); i ++ ) {
		if ( beamBurstList[i].IsValid() ) {
			beamBurstList[i]->Activate( false );
		}
	}
}

void hhDroid::Event_EndStaticBeam(void) {
	if ( beamBurstList[0].IsValid() ) {
		beamBurstList[0]->Activate( false );
	}
	if ( staticPoint.IsValid() ) {
		staticPoint.Clear();
	}

	//wait random time until next static beam
	if ( !IsHidden() && !AI_DEAD ) {
		PostEventSec(&EV_StartStaticBeam, gameLocal.random.RandomFloat() * spawnArgs.GetFloat( "staticFreq", "2.5" ) );
	}
}

void hhDroid::Event_StartStaticBeam(void) {
	idVec3 targetPoint;
	idEntity *entityList[ MAX_GENTITIES ];

	if ( IsHidden() || AI_DEAD ) {
		return;
	}

	//find nearest staticPoint
	float radius = spawnArgs.GetFloat( "staticRange" );
	int listedEntities = gameLocal.EntitiesWithinRadius( GetPhysics()->GetOrigin(), radius, entityList, MAX_GENTITIES );
	for( int i = 0; i < listedEntities; i++ ) {
		if ( entityList[ i ] && entityList[ i ]->spawnArgs.GetInt( "droidBeam", "0" ) ) {
			staticPoint = entityList[ i ];
			break;
		}
	}

	//activate one beam
	if ( staticPoint.IsValid() && beamBurstList[0].IsValid() ) {
		StartSound( "snd_staticBeam", SND_CHANNEL_BODY, 0, true, NULL );
		beamBurstList[0]->Activate( true );
		beamBurstList[0]->SetTargetLocation( beamBurstList[0]->GetOrigin() + burstLength * hhUtils::RandomVector() );
		staticPoint->ActivateTargets( this );
		PostEventSec(&EV_EndStaticBeam, staticDuration);
	}
}

void hhDroid::FlyTurn( void ) {
	if ( AI_PATHING ) {
		hhMonsterAI::FlyTurn();
		return;
	}
	if ( AI_ENEMY_VISIBLE || move.moveCommand == MOVE_FACE_ENEMY ) {
		TurnToward( lastVisibleEnemyPos );
	} else if ( ( move.moveCommand == MOVE_FACE_ENTITY ) && move.goalEntity.GetEntity() ) {
		TurnToward( move.goalEntity.GetEntity()->GetPhysics()->GetOrigin() );
	} else if ( move.speed > 0.0f ) {
		const idVec3 &vel = physicsObj.GetLinearVelocity();
		if ( vel.ToVec2().LengthSqr() > 0.1f ) {
			TurnToward( vel.ToYaw() );
		}
	}
	Turn();
}

void hhDroid::Event_StartPathing(void) {
	AI_PATHING = true;	
	old_fly_bob_strength = fly_bob_strength;
	fly_bob_strength = 0.0;
	ignore_obstacles = true;
}

void hhDroid::Event_EndPathing(void) {
	AI_PATHING = false;
	fly_bob_strength = old_fly_bob_strength;
	ignore_obstacles = false;
}

void hhDroid::FlyMove( void ) {
	idVec3	goalPos;
	idVec3	oldorigin;
	idVec3	newDest;

	AI_BLOCKED = false;
	if ( ( move.moveCommand != MOVE_NONE ) && ReachedPos( move.moveDest, move.moveCommand ) ) {
		if ( AI_PATHING ) {
			physicsObj.SetLinearVelocity( idVec3(0,0,0) );
		}
		StopMove( MOVE_STATUS_DONE );
	}

	if ( move.moveCommand != MOVE_TO_POSITION_DIRECT ) {
		idVec3 vel = physicsObj.GetLinearVelocity();

		if ( GetMovePos( goalPos ) ) {
			move.obstacle = NULL;
		}

		if ( move.speed	) {
			FlySeekGoal( vel, goalPos );
		}

		// add in bobbing
		AddFlyBob( vel );

		if ( enemy.GetEntity() && ( move.moveCommand != MOVE_TO_POSITION ) ) {
			float dist = ( GetPhysics()->GetOrigin() - enemy->GetPhysics()->GetOrigin() ).LengthFast();
			if ( dist < enemyRange && enemyRange > 0 ) {
				AdjustFlyHeight( vel, goalPos );
			}
		}

		AdjustFlySpeed( vel );

		physicsObj.SetLinearVelocity( vel );
	}

	// turn	
	FlyTurn();

	// run the physics for this frame
	oldorigin = physicsObj.GetOrigin();
	physicsObj.UseFlyMove( true );
	physicsObj.UseVelocityMove( false );
	physicsObj.SetDelta( vec3_zero );
	physicsObj.ForceDeltaMove( disableGravity );
	RunPhysics();

	monsterMoveResult_t	moveResult = physicsObj.GetMoveResult();
	if ( !af_push_moveables && attack.Length() && TestMelee() ) {
		DirectDamage( attack, enemy.GetEntity() );
	} else {
		idEntity *blockEnt = physicsObj.GetSlideMoveEntity();
		if ( blockEnt && blockEnt->IsType( idMoveable::Type ) && blockEnt->GetPhysics()->IsPushable() ) {
			KickObstacles( viewAxis[ 0 ], kickForce, blockEnt );
		} else if ( moveResult == MM_BLOCKED ) {
			move.blockTime = gameLocal.time + 500;
			AI_BLOCKED = true;
		}
	}

	idVec3 org = physicsObj.GetOrigin();
	if ( oldorigin != org ) {
		TouchTriggers();
	}

	if ( ai_debugMove.GetBool() ) {
		gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObj.GetOrigin(), 4000 );
		gameRenderWorld->DebugBounds( colorOrange, physicsObj.GetBounds(), org, gameLocal.msec );
		gameRenderWorld->DebugBounds( colorMagenta, physicsObj.GetBounds(), move.moveDest, gameLocal.msec );
		gameRenderWorld->DebugLine( colorRed, org, org + physicsObj.GetLinearVelocity(), gameLocal.msec, true );
		gameRenderWorld->DebugLine( colorBlue, org, goalPos, gameLocal.msec, true );
		gameRenderWorld->DebugLine( colorYellow, org + EyeOffset(), org + EyeOffset() + viewAxis[ 0 ] * physicsObj.GetGravityAxis() * 16.0f, gameLocal.msec, true );
		DrawRoute();
	}
}

idProjectile *hhDroid::LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, const idDict* desiredProjectileDef ) {
	//jsh overridden to allow per-projectile accuracy
	idVec3				muzzle;
	idVec3				dir;
	idVec3				start;
	trace_t				tr;
	idBounds			projBounds;
	float				distance;
	const idClipModel	*projClip;
	float				attack_accuracy;
	float				attack_cone;
	float				projectile_spread;
	float				diff;
	float				angle;
	float				spin;
	idAngles			ang;
	int					num_projectiles;
	int					i;
	idMat3				axis;
	idVec3				tmp;
	idProjectile		*lastProjectile;

	//HUMANHEAD mdc - added to support multiple projectiles
	if( desiredProjectileDef ) {	//try to set our projectile to the desiredProjectile
		int projIndex = FindProjectileInfo( desiredProjectileDef );
		if( projIndex >= 0 ) {
			SetCurrentProjectile( projIndex );
		}
	}
	//HUMANHEAD END

	if ( !projectileDef ) {
		gameLocal.Warning( "%s (%s) doesn't have a projectile specified", name.c_str(), GetEntityDefName() );
		return NULL;
	}

	if ( projectileDef->GetFloat( "attack_accuracy" ) ) {
		attack_accuracy = projectileDef->GetFloat( "attack_accuracy", "7" );
	} else {
		attack_accuracy = spawnArgs.GetFloat( "attack_accuracy", "7" );
	}
	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	if ( projectileDef->GetFloat( "projectile_spread" ) ) {
		projectile_spread = projectileDef->GetFloat( "projectile_spread", "0" );
	} else {
		projectile_spread = spawnArgs.GetFloat( "projectile_spread", "0" );
	}
	if ( projectileDef->GetFloat( "num_projectiles" ) ) {
		num_projectiles = projectileDef->GetFloat( "num_projectiles", "1" );
	} else {
		num_projectiles = spawnArgs.GetFloat( "num_projectiles", "1" );
	}


	GetMuzzle( jointname, muzzle, axis );

	if ( !projectile.GetEntity() ) {
		CreateProjectile( muzzle, axis[ 0 ] );
	}

	lastProjectile = projectile.GetEntity();

	if ( target != NULL ) {
		tmp = target->GetPhysics()->GetAbsBounds().GetCenter() - muzzle;
		tmp.Normalize();
		axis = tmp.ToMat3();
	} else {
		axis = viewAxis;
	}

	// rotate it because the cone points up by default
	tmp = axis[2];
	axis[2] = axis[0];
	axis[0] = -tmp;

	// make sure the projectile starts inside the monster bounding box
	const idBounds &ownerBounds = physicsObj.GetAbsBounds();
	projClip = lastProjectile->GetPhysics()->GetClipModel();
	projBounds = projClip->GetBounds().Rotate( axis );

	// check if the owner bounds is bigger than the projectile bounds
	if ( ( ( ownerBounds[1][0] - ownerBounds[0][0] ) > ( projBounds[1][0] - projBounds[0][0] ) ) &&
		( ( ownerBounds[1][1] - ownerBounds[0][1] ) > ( projBounds[1][1] - projBounds[0][1] ) ) &&
		( ( ownerBounds[1][2] - ownerBounds[0][2] ) > ( projBounds[1][2] - projBounds[0][2] ) ) ) {
		if ( (ownerBounds - projBounds).RayIntersection( muzzle, viewAxis[ 0 ], distance ) ) {
			start = muzzle + distance * viewAxis[ 0 ];
		} else {
			start = ownerBounds.GetCenter();
		}
	} else {
		// projectile bounds bigger than the owner bounds, so just start it from the center
		start = ownerBounds.GetCenter();
	}

	gameLocal.clip.Translation( tr, start, muzzle, projClip, axis, MASK_SHOT_RENDERMODEL, this );
	muzzle = tr.endpos;

	// set aiming direction	
	GetAimDir( muzzle, target, this, dir );
	ang = dir.ToAngles();

	// adjust his aim so it's not perfect.  uses sine based movement so the tracers appear less random in their spread.
	float t = MS2SEC( gameLocal.time + entityNumber * 497 );
	ang.pitch += idMath::Sin16( t * 5.1 ) * attack_accuracy;
	ang.yaw	+= idMath::Sin16( t * 6.7 ) * attack_accuracy;

	if ( !AI_WALLWALK && clampToAttackCone ) {
		// clamp the attack direction to be within monster's attack cone so he doesn't do
		// things like throw the missile backwards if you're behind him
		diff = idMath::AngleDelta( ang.yaw, current_yaw );
		if ( diff > attack_cone ) {
			ang.yaw = current_yaw + attack_cone;
		} else if ( diff < -attack_cone ) {
			ang.yaw = current_yaw - attack_cone;
		}
	}

	axis = ang.ToMat3();

	float spreadRad = DEG2RAD( projectile_spread );
	for( i = 0; i < num_projectiles; i++ ) {
		// spread the projectiles out
		angle = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
		spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
		dir = axis[ 0 ] + axis[ 2 ] * ( angle * idMath::Sin( spin ) ) - axis[ 1 ] * ( angle * idMath::Cos( spin ) );
		dir.Normalize();

		// launch the projectile
		if ( !projectile.GetEntity() ) {
			CreateProjectile( muzzle, dir );
		}
		lastProjectile = projectile.GetEntity();
		lastProjectile->Launch( muzzle, dir, vec3_origin );
		projectile = NULL;
	}

	TriggerWeaponEffects( muzzle, axis );

	lastAttackTime = gameLocal.time;

//HUMANHEAD mdc - added to support multiple projectiles
	projectile = NULL;
	SetCurrentProjectile( projectileDefaultDefIndex );	//set back to our default projectile to be on the safe side
//HUMANHEAD END

	return lastProjectile;
}

void hhDroid::Event_FlyZip() {
	idVec3 old_origin = GetOrigin();
	hhMonsterAI::Event_FlyZip();
	if ( old_origin != GetOrigin() ) {
		if ( beamZip.IsValid() ) {
			beamZip->SetTargetLocation( old_origin );
		}
		if ( beamZip.IsValid() ) {
			beamZip->Activate( true );
		}
		PostEventSec( &EV_EndZipBeams, spawnArgs.GetFloat( "zip_beam_duration", "1.0" ) );
	}
}

void hhDroid::Event_EndZipBeams() {
	if ( beamZip.IsValid() ) {
		beamZip->Activate( false );
	}
}

void hhDroid::Event_HealerReset() {
	fl.takedamage = true;
}

void hhDroid::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {				
	if ( bHealer ) {
		lives--;
		if ( lives <= 0 ) {
			hhMonsterAI::Killed( inflictor, attacker, damage, dir, location );
		} else {
			const char *defName = spawnArgs.GetString( "fx_pain" );
			if (defName && defName[0]) {
				hhFxInfo fxInfo;
				fxInfo.RemoveWhenDone( true );
				idEntityFx *painFx = SpawnFxLocal( defName, GetOrigin(), GetAxis(), &fxInfo, gameLocal.isClient );
				if ( painFx ) {
					painFx->Bind( this, true );
				}
			}
			health = spawnArgs.GetInt( "health" );
			fl.takedamage = false;
			PostEventSec( &EV_HealerReset, spawnArgs.GetFloat( "reset_delay", "0.8" ) );
		}
		return;
	}
	if ( AI_DEAD ) {
		AI_DAMAGE = true;
		return;
	}
	hhMonsterAI::Killed( inflictor, attacker, damage, dir, location );
	for ( int i = 0; i < beamBurstList.Num(); i ++ ) {
		if ( beamBurstList[i].IsValid() ) {
			beamBurstList[i]->Activate( true );
			beamBurstList[i]->SetTargetLocation( beamBurstList[i]->GetOrigin() + burstLength * hhUtils::RandomSpreadDir( dir.ToMat3(), burstSpread )  );
			PostEventSec(&EV_EndBurst, staticDuration);
		}
	}
}

void hhDroid::AdjustFlySpeed( idVec3 &vel ) {
	float speed;

	// apply dampening
	vel -= vel * flyDampening * MS2SEC( gameLocal.msec );

	// gradually speed up/slow down to desired speed
	speed = vel.Normalize();
	speed += ( move.speed - speed ) * MS2SEC( gameLocal.msec );
	if ( speed < 0.0f ) {
		speed = 0.0f;
	} else if ( move.speed && ( speed > move.speed ) ) {
		speed = move.speed;
	}

	vel *= speed;
}

void hhDroid::UpdateModelTransform( void ) {
	idVec3 origin;
	idMat3 axis;

	if ( GetPhysicsToVisualTransform( origin, axis ) ) {
		renderEntity.axis = axis * GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin() + origin * renderEntity.axis;

		if ( bHealer ) {
			renderEntity.axis = idAngles( 0, 0, spinAngle ).ToMat3() * renderEntity.axis;
			spinAngle += 10;
			if ( spinAngle > 360.0f ) {
				spinAngle = 0.0f;
			}
		}
	} else {
		renderEntity.axis = GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin();
	}
}

/*
=====================
hhDroid::Save
=====================
*/
void hhDroid::Save( idSaveGame *savefile ) const {
	int num = beamBurstList.Num();
	savefile->WriteInt( num );
	for ( int i = 0; i < num; i++ ) {
		beamBurstList[i].Save( savefile );
	}

	staticPoint.Save( savefile );
	chargeShot.Save( savefile );
	beamZip.Save( savefile );

	savefile->WriteVec3( savedGravity );
	savefile->WriteFloat( chargeShotSize );
	savefile->WriteFloat( burstLength );
	savefile->WriteFloat( burstSpread );
	savefile->WriteFloat( burstDuration );
	savefile->WriteFloat( staticDuration );
	savefile->WriteFloat( staticRange );
	savefile->WriteInt( numBurstBeams );
	savefile->WriteVec3( beamOffset );
	savefile->WriteFloat( old_fly_bob_strength );
	savefile->WriteFloat( enemyRange );
	savefile->WriteFloat( flyDampening );
	savefile->WriteFloat( spinAngle );
	savefile->WriteBool( bHealer );
	savefile->WriteInt( lives );
}

/*
=====================
hhDroid::Restore
=====================
*/
void hhDroid::Restore( idRestoreGame *savefile ) {
	int num;
	savefile->ReadInt( num );
	beamBurstList.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		beamBurstList[i].Restore( savefile );
	}

	staticPoint.Restore( savefile );
	chargeShot.Restore( savefile );
	beamZip.Restore( savefile );

	savefile->ReadVec3( savedGravity );
	savefile->ReadFloat( chargeShotSize );
	savefile->ReadFloat( burstLength );
	savefile->ReadFloat( burstSpread );
	savefile->ReadFloat( burstDuration );
	savefile->ReadFloat( staticDuration );
	savefile->ReadFloat( staticRange );
	savefile->ReadInt( numBurstBeams );
	savefile->ReadVec3( beamOffset );
	savefile->ReadFloat( old_fly_bob_strength );
	savefile->ReadFloat( enemyRange );
	savefile->ReadFloat( flyDampening );
	savefile->ReadFloat( spinAngle );
	savefile->ReadBool( bHealer );
	savefile->ReadInt( lives );
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build