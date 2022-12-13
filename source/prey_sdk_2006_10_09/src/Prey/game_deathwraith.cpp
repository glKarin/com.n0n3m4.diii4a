#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


#define DEBUGGING_WRAITHS		0

//=============================================================================
// hhDeathWraith
//=============================================================================

CLASS_DECLARATION( hhWraith, hhDeathWraith )
	EVENT( AI_FindEnemy,		hhWraith::Event_FindEnemy )
END_CLASS


//=============================================================================
//
// hhDeatWraith::Spawn
//
//=============================================================================

void hhDeathWraith::Spawn() {
	// Each wraith is assigned a random distance, rotation speed, and height around the enemy
	idVec3 minCircleInfo = spawnArgs.GetVector( "min_circleInfo", "100 10 100" ); // dist, speed, height
	idVec3 maxCircleInfo = spawnArgs.GetVector( "max_circleInfo", "100 10 100" ); // dist, speed, height
	
	circleDist = minCircleInfo[0] + gameLocal.random.RandomFloat() * ( maxCircleInfo[0] - minCircleInfo[0] );
	circleSpeed = minCircleInfo[1] + gameLocal.random.RandomFloat() * ( maxCircleInfo[1] - minCircleInfo[1] );
	circleHeight = minCircleInfo[2] + gameLocal.random.RandomFloat() * ( maxCircleInfo[2] - minCircleInfo[2] );

	circleClockwise = (gameLocal.random.RandomFloat() > 0.5f ? true : false);

	// Compute the random attack time (in frames)
	attackTimeMin	= spawnArgs.GetFloat( "min_attackTime", "4" ) * 60;
	attackTimeDelta	= ( spawnArgs.GetFloat( "max_attackTime", "8" ) * 60 ) - attackTimeMin;

	if ( state == WS_FLY ) {
		// The wraith is now circling the enemy - set the attack time
		countDownTimer = attackTimeMin + gameLocal.random.RandomInt( attackTimeDelta );
	}

	// Determine the wraith type
	healthWraith = (gameLocal.random.RandomFloat() < spawnArgs.GetFloat( "healthTypeChance", "0.35" ) ? true : false);
}

//=============================================================================
//
// hhDeathWraith::FlyUp
//
//=============================================================================
void hhDeathWraith::FlyUp() {
	hhWraith::FlyUp();

	if ( state == WS_FLY ) {
		// The wraith is now circling the enemy - set the attack time
		countDownTimer = attackTimeMin + gameLocal.random.RandomInt( attackTimeDelta );
	}
}

//=============================================================================
//
// hhDeathWraith::FlyToEnemy
//
// Circle an enemy
//=============================================================================
void hhDeathWraith::FlyToEnemy() {
	float	distAdjust;
	idVec3	origin;
	float	delta;
	bool	dir;

#if DEBUGGING_WRAITHS
gameRenderWorld->DebugLine(colorWhite, GetOrigin(), GetOrigin()+idVec3(0,0,5), 5000);
#endif

	// Look for an enemy for the Wraith
	// FIXME:  This needs to be done often, because the player could spiritwalk after the Wraith has him as the enemy
	//		and we want the wraiths to try to target the player's proxy.
	//		We could either have this check via a heartbeat, or have some type of broadcast message when the player spiritwalks (?)
	Event_FindEnemy( false );

	// Ensure that the wraith is on the correct path, and if not, then set the wraith's velocity and angle to return to the path
	origin = GetOrigin();
	idVec3 toEnemy = enemy->GetOrigin() - origin;
	toEnemy.z = 0;
	float distance = toEnemy.Normalize();

	idVec3 tangent( -toEnemy.y, toEnemy.x, 0 ); // Quick tangent, swap x and y

	// If counter-clockwise, then reverse the tangent vector
	if ( !circleClockwise ) {
		tangent *= -1;
	}

	// Determine if the radius is too large and gradually pull the Wraith in closer or if the wraith is too close, push it out
	if ( distance > circleDist + circleSpeed ) { // Move closer
		distAdjust = circleSpeed * 0.1f;
	} else if ( distance < circleDist - circleSpeed ) { // Move farther away
		distAdjust = -circleSpeed * 0.1f;
	} else { // No adjustment
		distAdjust = 0;
	}

	// Decide if the wraith should change z-height
	idVec3 zVelocity( 0, 0, 0 );

	if ( origin.z < enemy->GetOrigin().z + circleHeight ) {
		zVelocity.z = 2.0f;
	} else if ( origin.z > enemy->GetOrigin().z + circleHeight + 30.0f ) {
		zVelocity.z = -2.0f;
	}

	// Move the wraith through the world
	idVec3 velocity = tangent * circleSpeed + toEnemy * distAdjust + zVelocity;

	// Rotate towards velocity direction
	dir = GetFacePosAngle( GetOrigin() + velocity, delta );

	if ( delta > this->turn_threshold ) { // Wraith should turn
		if ( dir ) { // Turn to the left
			deltaViewAngles.yaw = this->turn_radius_max * (60.0f * USERCMD_ONE_OVER_HZ);
		} else {
			deltaViewAngles.yaw = -this->turn_radius_max * (60.0f * USERCMD_ONE_OVER_HZ);
		}
	}

	SetOrigin( GetOrigin() + (velocity * (60.0f * USERCMD_ONE_OVER_HZ)) );

	// Decide if the wraith has waited long enough and then attack
	if ( --countDownTimer <= 0 ) {
		if ( health > 0 && enemy.IsValid() ) { // Only attack if the wraith has an enemy (which it should at all times) and the wraith hasn't been killed
			EnterAttackState();
		} else { // Reset the attack time
			countDownTimer = attackTimeMin + gameLocal.random.RandomInt( attackTimeDelta );
		}
	}
}

//=============================================================================
//
// hhDeathWraith::Damage
//
//=============================================================================
void hhDeathWraith::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	// Skip wraiths brighten when damaged logic
	hhMonsterAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

//=============================================================================
//
// hhDeathWraith::Killed
//
// When a deathwraith is killed, it immediately is removed
// The queue is reset and a new wraith will be spawned shortly after
//=============================================================================
void hhDeathWraith::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	hhFxInfo fxInfo;

	// Stop flyloop and chatter sounds, if any
	StopSound( SND_CHANNEL_BODY );
	StopSound( SND_CHANNEL_VOICE );

	SetShaderParm( SHADERPARM_TIMEOFFSET, MS2SEC( gameLocal.time ) );
	SetShaderParm( SHADERPARM_DIVERSITY, 1.0f );

	const char *deathEffect = spawnArgs.GetString("fx_deatheffect1");
	if (attacker && attacker->IsType(hhPlayer::Type)) {
		hhPlayer *player = static_cast<hhPlayer *>( attacker );
		if ( healthWraith ) {
			deathEffect = spawnArgs.GetString("fx_deatheffect2");
		}
	}

	fxInfo.RemoveWhenDone( true );
	BroadcastFxInfoAlongBone( deathEffect, spawnArgs.GetString( "joint_collision" ), &fxInfo );

	physicsObj.SetContents(0);
	fl.takedamage = false;
	UnlinkCombat();

	StartSound( "snd_death", SND_CHANNEL_VOICE );

	NotifyDeath( inflictor, attacker );

	PostEventSec( &EV_Remove, spawnArgs.GetFloat( "burnOutTime" ) );
}

//=============================================================================
//
// hhDeathWraith::NotifyDeath
//
// Handles notification of player on death
//=============================================================================
void hhDeathWraith::NotifyDeath( idEntity *inflictor, idEntity *attacker ) {

	if ( attacker && attacker->IsType( hhPlayer::Type ) ) {
		hhPlayer *player = static_cast<hhPlayer *>( attacker );

		SpawnEnergy(player);

		if ( player->IsDeathWalking() ) {
			player->KilledDeathWraith();
		}
	}
}

//=============================================================================
//
// hhDeathWraith::SpawnEnergy
//
// Energy travels from wraith to deathWalkProxy
//=============================================================================
void hhDeathWraith::SpawnEnergy(hhPlayer *player) {
	idEntity *destEntity = NULL;
	const char *deathEnergyName = NULL;
	idVec3 origin;
	idMat3 axis;
	idDict args;

	if ( healthWraith ) {
		// Spawn health energy
		deathEnergyName = spawnArgs.GetString("def_deathEnergyHealth", NULL);
	}
	else {
		// Spawn spirit power energy
		deathEnergyName = spawnArgs.GetString("def_deathEnergySpirit", NULL);
	}
	destEntity = player->GetDeathwalkEnergyDestination();

	// Spawn the energy system that will deliver the spiritpower to the player
	if (deathEnergyName && destEntity) {
		idVec3 center = GetPhysics()->GetAbsBounds().GetCenter();
		args.Clear();
		args.Set("origin", center.ToString());
		idEntity *ent = gameLocal.SpawnObject(deathEnergyName, &args);
		if (ent && ent->IsType(hhDeathWraithEnergy::Type)) {
			hhDeathWraithEnergy *energy = static_cast<hhDeathWraithEnergy*>(ent);
			energy->SetPlayer(player);
			energy->SetDestination( destEntity->GetOrigin() );
		}
	}
}

//=============================================================================
//
// hhDeathWraith::Think
//
//=============================================================================
void hhDeathWraith::Think( void ) {

	if ( enemy.IsValid() ) {
		assert( enemy->IsType( hhPlayer::Type ) );
		hhPlayer *player = reinterpret_cast<hhPlayer *> ( enemy.GetEntity() );

		if( !player->IsDeathWalking() ) {
			BecomeInactive( TH_THINK );
			PostEventMS( &EV_Remove, 0 );
			return;
		}
		else {
			if ( healthWraith ) {
				SetShaderParm(SHADERPARM_MODE, 1.0f);
			}
			else {
				SetShaderParm(SHADERPARM_MODE, 0.0f);
			}
		}
	}

	// Don't call hhWraith::Think(), since we don't want our health to drop over time
	hhMonsterAI::Think();

	// Since idAI is only capable of yawing, not piching, we post apply our pitch
	if (state == WS_DEATH_CHARGE && enemy.IsValid()) {
		idVec3 toEnemy = enemy->GetOrigin() - GetOrigin();
		toEnemy.Normalize();
		viewAxis = toEnemy.ToMat3();
	}

#if DEBUGGING_WRAITHS
	gameRenderWorld->DebugLine(colorRed, GetOrigin(), GetOrigin()+viewAxis[0]*20, 1000);
	gameRenderWorld->DebugLine(colorGreen, GetOrigin(), GetOrigin()+viewAxis[1]*20, 1000);
	gameRenderWorld->DebugLine(colorBlue, GetOrigin(), GetOrigin()+viewAxis[2]*20, 1000);
#endif
}

//=============================================================================
//
// hhDeathWraith::FlyMove
//
//=============================================================================
void hhDeathWraith::FlyMove( void ) {
	idMat3 axis;
	idVec3 delta;
	idVec3 toEnemy;

	// The state of the creature determines how it will move
	switch ( state ) {
	case WS_SPAWN:
		FlyUp(); // Flying up right after spawn
		break;
	case WS_FLY:
		FlyToEnemy();
		break;
	case WS_FLEE: // Flying away
		FlyAway();
		break;
	case WS_DEATH_CHARGE:	// Charging an enemy

#if 1
		// Tranform animation movement by actual 'pitched' axis
		if (enemy.IsValid()) {
			toEnemy = enemy->GetOrigin() - GetOrigin();
			toEnemy.Normalize();
			axis = toEnemy.ToMat3();
		} else { // HUMANHEAD mdl:  Enemy might not be valid if player is just leaving deathwalk
			axis = viewAxis;
		}
#else
		axis = viewAxis;
#endif

		if (ChargeEnemy()) {
			// Using animation delta to generate movement
			physicsObj.UseFlyMove( false );
			physicsObj.UseVelocityMove( false );
			GetMoveDelta( axis, axis, delta );
			physicsObj.SetDelta( delta );
			physicsObj.ForceDeltaMove( true );
			RunPhysics();
		}
		return;
	case WS_STILL: // Wraith is not moving at all (playing a specific anim, etc)
		return;
	}

	// run the physics for this frame
	physicsObj.UseFlyMove( true );
	physicsObj.UseVelocityMove( false );
	physicsObj.SetDelta( vec3_zero );
	physicsObj.ForceDeltaMove( disableGravity );
	RunPhysics();
}


//=============================================================================
//
// hhDeathWraith::EnterAttackState
//
//=============================================================================
void hhDeathWraith::EnterAttackState() {
	state = WS_DEATH_CHARGE;

	// Set the wraith's rotation target towards the enemy
	TurnTowardEnemy();

	SetShaderParm( SHADERPARM_MISC, 1 ); // Charging
	StartSound( "snd_charge", SND_CHANNEL_VOICE2 );

	countDownTimer = PlayAnim( "alert", 2 ) / USERCMD_MSEC;
}

//=============================================================================
//
// hhDeathWraith::ChargeEnemy
//
//=============================================================================
bool hhDeathWraith::ChargeEnemy( void ) {

	if ( !enemy.IsValid() ) {
		ExitAttackState();
		return false;
	}

	assert(enemy.IsValid() && enemy->IsType(hhPlayer::Type));

	// Set the wraith's rotation target towards the enemy
	TurnTowardEnemy();

	// Attempt to attack the enemy
	// If the wraith did not damage the enemy, then continue the attack flight
	if ( !CheckEnemy() ) {
		// Once the animation is finished (and the wraith wasn't killed or didn't hit the player), 
		// then transition to circle mode which will handle getting the wraith back to the path
		if ( --countDownTimer <= 0 ) {
			ExitAttackState();
			return false;
		}
	}
	return true;
}

//=============================================================================
//
// hhDeathWraith::ExitAttackState
//
//=============================================================================
void hhDeathWraith::ExitAttackState() {
	state = WS_FLY;
	PlayCycle( "fly", 2 );
	countDownTimer = attackTimeMin + gameLocal.random.RandomInt( attackTimeDelta );
	SetShaderParm( SHADERPARM_MISC, 0 ); // No longer charging

	GetPhysics()->SetLinearVelocity(vec3_origin);
}

//=============================================================================
//
// hhDeathWraith::CheckEnemy
//
//=============================================================================
bool hhDeathWraith::CheckEnemy( void ) {

	// Don't try to damage if the wraith doesn't have a target, or if the wraith is dead or harvested
	if ( health <= 0 || !enemy.IsValid() || fl.takedamage == false ) {
		return false;
	}

	idVec3 center = GetPhysics()->GetAbsBounds().GetCenter();
	idVec3 dir = enemy->GetPhysics()->GetAbsBounds().GetCenter() - center;

	if ( dir.LengthSqr() < spawnArgs.GetFloat( "minDamageDistSqr", "2500" ) ) {
		HitEnemy();
		return true;
	}

	return false;
}

//=============================================================================
//
// hhDeathWraith::HitEnemy
//
//=============================================================================
void hhDeathWraith::HitEnemy( void ) {
	hhFxInfo fxInfo;

	if (IsHidden()) {
		return;
	}

	// Activate the death trigger
	idEntityPtr<idEntity> deathTrigger = gameLocal.FindEntity( spawnArgs.GetString( "triggerOnHit" ) );
	if ( deathTrigger.IsValid() ) {
		if ( deathTrigger->RespondsTo( EV_Activate ) || deathTrigger->HasSignal( SIG_TRIGGER ) ) {
			deathTrigger->Signal( SIG_TRIGGER );
			deathTrigger->ProcessEvent( &EV_Activate, this );
		} 		
	}

	Hide();
	fl.takedamage = false;

	// Hit enemy effects -- for now, using the death effects
	idVec3 center = GetPhysics()->GetAbsBounds().GetCenter();
	fxInfo.RemoveWhenDone( true );
	BroadcastFxInfoPrefixed( "fx_hitenemy", center, mat3_identity, &fxInfo );
	StartSound( "snd_attack", SND_CHANNEL_VOICE );

	ApplyImpulseToEnemy();

	if (enemy->IsType(hhPlayer::Type)) {
		hhPlayer *player = static_cast<hhPlayer*>(enemy.GetEntity());
	
		if ( healthWraith ) {
			int playerHealth = player->GetHealth();
			int minHealth = player->spawnArgs.GetInt( "minResurrectHealth", "50" );

			playerHealth -= spawnArgs.GetInt( "playerHealthDamage", "10" );
			if ( playerHealth < minHealth ) {
				playerHealth = minHealth;
			}
			player->SetHealth( playerHealth );				
		} else {
			player->DeathWalkDamagedByWraith( this, spawnArgs.GetString( "def_damagetype" ) );
		}
	}

	PostEventMS( &EV_Remove, 2000 );
}

//=============================================================================
//
// hhDeathWraith::ApplyImpulseToEnemy
//
//=============================================================================
void hhDeathWraith::ApplyImpulseToEnemy() {
	if( enemy.IsValid() ) {
		idMat3 axis = GetAxis();
		idVec3 impulseDir = axis[0] - axis[2];
		impulseDir.Normalize();
		enemy->ApplyImpulse( this, 0, enemy->GetOrigin(), impulseDir * spawnArgs.GetFloat("impulseMagnitude") * enemy->GetPhysics()->GetMass() );
	}
}

//=============================================================================
//
// hhDeathWraith::EnemyDead
//
//=============================================================================
void hhDeathWraith::EnemyDead() {
	return; // Death wraiths expect their enemies to be dead. 
}

//=============================================================================
//
// hhDeathWraith::PlayCycle
//
//=============================================================================
void hhDeathWraith::PlayCycle( const char *name, int blendFrame ) {
	GetAnimator()->ClearAllAnims( gameLocal.GetTime(), FRAME2MS(blendFrame) );
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, GetAnimator()->GetAnim(name), gameLocal.GetTime(), FRAME2MS(blendFrame) );
}

//=============================================================================
//
// hhDeathWraith::PlayAnim
//
//=============================================================================
int hhDeathWraith::PlayAnim( const char *name, int blendFrame ) {
	int fadeTime = FRAME2MS( blendFrame );
	int animLength = 0;
 
	GetAnimator()->ClearAllAnims( gameLocal.GetTime(), FRAME2MS(blendFrame) );

	int anim = GetAnimator()->GetAnim( name );
	if ( anim ) {
		GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.GetTime(), fadeTime );
		animLength = GetAnimator()->CurrentAnim( ANIMCHANNEL_ALL )->Length();
		animLength = (animLength > fadeTime) ? animLength - fadeTime : animLength;
	}

	return animLength;
}

//=============================================================================
//
// hhDeathWraith::Event_FindEnemy
//
//=============================================================================
void hhDeathWraith::Event_FindEnemy( int useFOV ) {
	// These are meant for single player deathwalk only
	idPlayer *player = gameLocal.GetLocalPlayer();
	SetEnemy( player );
	idThread::ReturnEntity( player );
}

//=============================================================================
//
// hhDeathWraith::TeleportIn
//
//=============================================================================

void hhDeathWraith::TeleportIn( idEntity *activator ) {
	Show();
	PostEventMS( &EV_Activate, 0, activator );
}
