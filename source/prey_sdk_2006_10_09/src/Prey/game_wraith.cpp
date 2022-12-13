//*****************************************************************************
//**
//** GAME_WRAITH.CPP
//**
//** Game code for the Wraith creature
//**
//*****************************************************************************

// HEADER FILES ---------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// MACROS ---------------------------------------------------------------------

#define	TWEEN_BANK			1000

// TYPES ----------------------------------------------------------------------

// CLASS DECLARATIONS ---------------------------------------------------------

const idEventDef EV_Flee( "<flee>", NULL );
const idEventDef EV_PlayAnimMoveEnd( "<playanimmoveend>", NULL );
extern const idEventDef EV_SpawnDeathWraith;
extern const idEventDef AI_FindEnemy;

CLASS_DECLARATION( hhMonsterAI, hhWraith )
	EVENT( EV_Flee,				hhWraith::Event_Flee )
	EVENT( EV_PlayAnimMoveEnd,	hhWraith::Event_PlayAnimMoveEnd )
	EVENT( AI_FindEnemy,		hhWraith::Event_FindEnemy )
	EVENT( EV_Activate,			hhWraith::Event_Activate )
END_CLASS

//=============================================================================
//
// hhWraith::Spawn
//
//=============================================================================
void hhWraith::Spawn(void) {
	// List animations
	flyAnim			= GetAnimator()->GetAnim( "fly" );
	possessAnim		= GetAnimator()->GetAnim( "alert" );
	leftAnim		= GetAnimator()->GetAnim( "bankLeft" );
	rightAnim		= GetAnimator()->GetAnim( "bankRight" );
	fleeAnim		= GetAnimator()->GetAnim( "flee" );
	fleeInAnim		= GetAnimator()->GetAnim( "fleeIn" );

	lastAnim		= NULL;

	canPossess	= spawnArgs.GetBool( "possess", "1" );

	// Physics and collision information	
	fl.takedamage = true;

	minDamageDist = spawnArgs.GetFloat( "minDamageDist", "50" );

	// Initialize flight logic
	straightTicks = 0;
	damageTicks = 0;
	turnTicks = 0;

	isScaling = false;

	// Initially launch the wraith forward
	velocity = GetPhysics()->GetAxis()[0] * 100;

	GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, flyAnim, gameLocal.time, 0 );

	BecomeActive( TH_THINK );
	fl.neverDormant = true;

	nextCheckTime = gameLocal.time + spawnArgs.GetInt("trace_check_time", "200");
	lastCheckOrigin = GetPhysics()->GetOrigin();
	lastDamageTime = spawnArgs.GetFloat( "damageDelay", "0.5" );

	if ( spawnArgs.GetBool( "spawnFlyUp", "0" ) ) {
		state = WS_SPAWN;

		// Orient the wraith pointing upwards
		desiredAxis[0] = idVec3( 0, 0, 1 );
		desiredAxis[1] = idVec3( 1, 0, 0 );
		desiredAxis[2] = idVec3( 0, -1, 0 );

		// Set the speed of this movement
		desiredVelocity = spawnArgs.GetFloat( "initialVelocity", "10" ); // TODO:  Make this random

		// Calculate how far the wraith should fly and how long that should take
		// NOTE:  If we make the velocity a random range, then we can make this spawn time constant
		countDownTimer = spawnArgs.GetFloat( "spawnTime", "0.5" ) * 60; // Half a second to raise up
	} else {
		state = WS_FLY;	
	}

	// Initialize .def values
	velocity_xy = spawnArgs.GetFloat( "velocity_xy", "7" ) * (60.0f * USERCMD_ONE_OVER_HZ);
	velocity_z = spawnArgs.GetFloat( "velocity_z", "1" ) * (60.0f * USERCMD_ONE_OVER_HZ);
	velocity_z_fast = spawnArgs.GetFloat( "velocity_z_fast", "5" ) * (60.0f * USERCMD_ONE_OVER_HZ);
	dist_z_close = spawnArgs.GetFloat( "dist_z_close", "5" );
	dist_z_far = spawnArgs.GetFloat( "dist_z_far", "100" );

	turn_threshold = DEG2RAD( spawnArgs.GetFloat( "turn_threshold", "5" ) ); // CJR PCF 5/17/06:  Removed unnecessary 30Hz compensation
	turn_radius_max = DEG2RAD( spawnArgs.GetFloat( "turn_radius_max", "130" ) ); // CJR PCF 5/17/06:  Remove unnecessary 30Hz compensation

	straight_ticks = spawnArgs.GetInt( "straight_ticks", "30" ) * (USERCMD_HZ / 60.0f);
	damage_ticks = spawnArgs.GetFloat( "damage_ticks", "8" ) * (USERCMD_HZ / 60.0f);
	turn_ticks = spawnArgs.GetFloat( "turn_ticks", "200" ) * (USERCMD_HZ / 60.0f);

	flee_speed_z = spawnArgs.GetFloat( "flee_speed_z", "10" ) * (60.0f * USERCMD_ONE_OVER_HZ);

	target_z_threshold = spawnArgs.GetFloat( "target_z_threshold", "20" );

	// JRM: are we waiting for a trigger? Then no sound.
	if(!spawnArgs.GetBool("trigger","0")) {
		StartSound( "snd_flyloop", SND_CHANNEL_BODY );
		StartSound( "snd_sight", SND_CHANNEL_VOICE );

		// If this wraith should flee when spawned, then kill it immediately and get it fleeing
		// This is used by the possessed kids to spawn a wraith that flees immediately
		if ( spawnArgs.GetBool("flee_at_spawn", "0" ) ) {	
			// Fleeing
			Killed( this, this, 0, vec3_origin, 0 );

			isScaling = true;
			scaleStart = spawnArgs.GetFloat( "scaleStart", "1" );
			scaleEnd = spawnArgs.GetFloat( "scaleEnd", "1" );
			scaleTime = spawnArgs.GetFloat( "scaleTime", "1" );

			// Set data for dynamically scaling the wraith
			SetShaderParm( SHADERPARM_ANY_DEFORM, DEFORMTYPE_SCALE ); // Scale deform
			SetShaderParm( SHADERPARM_ANY_DEFORM_PARM1, scaleStart ); // Scale deform
			lastScaleTime = MS2SEC( gameLocal.time ) + scaleTime;
		}
	}

	Event_SetMoveType(MOVETYPE_FLY); // JRM to get the wraiths to move again

	GetPhysics()->SetContents( CONTENTS_SHOOTABLE|CONTENTS_SHOOTABLEBYARROW );
	GetPhysics()->SetClipMask( 0 );

	bFaceEnemy = false;
	nextDrop = 0;
	nextChatter = 0;
	nextPossessTime = 0;

	minChatter = SEC2MS(spawnArgs.GetFloat( "min_chatter_time", "3" ));
	maxChatter = SEC2MS(spawnArgs.GetFloat( "max_chatter_time", "6" ));

	if ( !IsHidden() ) {
		Hide();
		PostEventMS( &EV_Activate, 0, this );
	}
}

//=============================================================================
//
// hhWraith::~hhWraith
//
//=============================================================================
hhWraith::~hhWraith() {
	StopSound( SND_CHANNEL_BODY );
}

//=============================================================================
//
// hhWraith::Save
//
//=============================================================================
void hhWraith::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( lastAnim );
	savefile->WriteBool( canPossess );
	savefile->WriteVec3( velocity );
	savefile->WriteInt( damageTicks );
	savefile->WriteInt( straightTicks );
	savefile->WriteInt( turnTicks );
	savefile->WriteVec3( lastCheckOrigin );
	savefile->WriteFloat( lastDamageTime );
	savefile->WriteInt( state );
	savefile->WriteFloat( velocity_xy );
	savefile->WriteFloat( velocity_z );
	savefile->WriteFloat( velocity_z_fast );
	savefile->WriteFloat( dist_z_close );
	savefile->WriteFloat( dist_z_far );
	savefile->WriteFloat( turn_threshold );
	savefile->WriteFloat( turn_radius_max );
	savefile->WriteInt( straight_ticks );
	savefile->WriteInt( damage_ticks );
	savefile->WriteInt( turn_ticks );
	savefile->WriteFloat( flee_speed_z );
	savefile->WriteFloat( target_z_threshold );
	savefile->WriteFloat( minDamageDist );
	savefile->WriteBool( isScaling );
	savefile->WriteFloat( scaleStart );
	savefile->WriteFloat( scaleEnd );
	savefile->WriteFloat( lastScaleTime );
	savefile->WriteInt( countDownTimer );
	savefile->WriteMat3( desiredAxis );
	savefile->WriteFloat( desiredVelocity );
	savefile->WriteBool( bFaceEnemy );
	fxFly.Save( savefile );
}

//=============================================================================
//
// hhWraith::Restore
//
//=============================================================================
void hhWraith::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( lastAnim );
	savefile->ReadBool( canPossess );
	savefile->ReadVec3( velocity );
	savefile->ReadInt( damageTicks );
	savefile->ReadInt( straightTicks );
	savefile->ReadInt( turnTicks );
	savefile->ReadVec3( lastCheckOrigin );
	savefile->ReadFloat( lastDamageTime );
	savefile->ReadInt( (int &)state );
	savefile->ReadFloat( velocity_xy );
	savefile->ReadFloat( velocity_z );
	savefile->ReadFloat( velocity_z_fast );
	savefile->ReadFloat( dist_z_close );
	savefile->ReadFloat( dist_z_far );
	savefile->ReadFloat( turn_threshold );
	savefile->ReadFloat( turn_radius_max );
	savefile->ReadInt( straight_ticks );
	savefile->ReadInt( damage_ticks );
	savefile->ReadInt( turn_ticks );
	savefile->ReadFloat( flee_speed_z );
	savefile->ReadFloat( target_z_threshold );
	savefile->ReadFloat( minDamageDist );
	savefile->ReadBool( isScaling );
	savefile->ReadFloat( scaleStart );
	savefile->ReadFloat( scaleEnd );
	savefile->ReadFloat( lastScaleTime );
	savefile->ReadInt( countDownTimer );
	savefile->ReadMat3( desiredAxis );
	savefile->ReadFloat( desiredVelocity );
	savefile->ReadBool( bFaceEnemy );

	// List animations
	flyAnim			= GetAnimator()->GetAnim( "fly" );
	possessAnim		= GetAnimator()->GetAnim( "alert" );
	leftAnim		= GetAnimator()->GetAnim( "bankLeft" );
	rightAnim		= GetAnimator()->GetAnim( "bankRight" );
	fleeAnim		= GetAnimator()->GetAnim( "flee" );
	fleeInAnim		= GetAnimator()->GetAnim( "fleeIn" );

	canPossess	= spawnArgs.GetBool( "possess", "1" );

	minDamageDist = spawnArgs.GetFloat( "minDamageDist", "50" );

	// Initialize .def values
	velocity_xy = spawnArgs.GetFloat( "velocity_xy", "7" );
	velocity_z = spawnArgs.GetFloat( "velocity_z", "1" );
	velocity_z_fast = spawnArgs.GetFloat( "velocity_z_fast", "5" );
	dist_z_close = spawnArgs.GetFloat( "dist_z_close", "5" );
	dist_z_far = spawnArgs.GetFloat( "dist_z_far", "100" );

	turn_threshold = DEG2RAD( spawnArgs.GetFloat( "turn_threshold", "5" ) );
	turn_radius_max = DEG2RAD( spawnArgs.GetFloat( "turn_radius_max", "130" ) );

	straight_ticks = spawnArgs.GetInt( "straight_ticks", "30" ) * (60.0f * USERCMD_ONE_OVER_HZ);
	damage_ticks = spawnArgs.GetFloat( "damage_ticks", "8" ) * (60.0f * USERCMD_ONE_OVER_HZ);
	turn_ticks = spawnArgs.GetFloat( "turn_ticks", "200" ) * (60.0f * USERCMD_ONE_OVER_HZ);

	flee_speed_z = spawnArgs.GetFloat( "flee_speed_z", "10" );

	target_z_threshold = spawnArgs.GetFloat( "target_z_threshold", "20" );

	nextDrop = 0;
	nextChatter = 0;
	nextPossessTime = gameLocal.time + 500; // Don't possess for half a second after loading
	minChatter = SEC2MS(spawnArgs.GetFloat( "min_chatter_time", "3" ));
	maxChatter = SEC2MS(spawnArgs.GetFloat( "max_chatter_time", "6" ));

	fxFly.Restore( savefile );

	nextCheckTime = gameLocal.time + spawnArgs.GetInt("trace_check_time", "200");
	scaleTime = spawnArgs.GetFloat( "scaleTime", "1" );
}

//=============================================================================
//
// hhWraith::Think
//
//=============================================================================
void hhWraith::Think(void) {
	if(!IsHidden()) { // JRM - NO TRACES WHEN HIDDEN!
		CheckCollisions();		

		if (gameLocal.time > nextDrop) {
			hhMonsterAI::Damage(this, this, vec3_origin, "damage_wraithminion", 0.05f, INVALID_JOINT);
			nextDrop = gameLocal.time + 1000;
		}

		if (gameLocal.time > nextChatter) {
			StartSound( "snd_chatter", SND_CHANNEL_VOICE );
			nextChatter = gameLocal.time + (minChatter + gameLocal.random.RandomFloat() * (maxChatter - minChatter));
		}
	}

	// Scale the wraith
	if ( isScaling ) {
		float delta = lastScaleTime - MS2SEC( gameLocal.time );
		if ( delta > 0 ) {
			SetShaderParm( SHADERPARM_ANY_DEFORM_PARM1, hhMath::Lerp( scaleEnd, scaleStart, delta / scaleTime ) );
		}
	}

	hhMonsterAI::Think(); // JRM: Bypassing hhAI::Think()
}

//=============================================================================
//
// hhWraith::Event_Activate
//
//=============================================================================
void hhWraith::Event_Activate(idEntity *activator) {
	if ( IsHidden() ) {
		TeleportIn(activator);
		return;
	}
	if ( spawnArgs.GetBool("flee_at_spawn", "0" ) ) {	
		// Fleeing
		Killed( this, this, 0, vec3_origin, 0 );

		isScaling = true;
		scaleStart = spawnArgs.GetFloat( "scaleStart", "1" );
		scaleEnd = spawnArgs.GetFloat( "scaleEnd", "1" );
		scaleTime = spawnArgs.GetFloat( "scaleTime", "1" );

		// Set data for dynamically scaling the wraith
		SetShaderParm( SHADERPARM_ANY_DEFORM, DEFORMTYPE_SCALE ); // Scale deform
		SetShaderParm( SHADERPARM_ANY_DEFORM_PARM1, scaleStart ); // Scale deform
		lastScaleTime = MS2SEC( gameLocal.time ) + scaleTime;
	}

	hhMonsterAI::Event_Activate(activator);

	// we've been waiting for a trigger
	if (spawnArgs.GetBool("trigger")) {
		StartSound( "snd_flyloop", SND_CHANNEL_BODY );
		StartSound( "snd_sight", SND_CHANNEL_VOICE );
	}


	const char *defName = spawnArgs.GetString("fx_fly");
	if (defName && defName[0]) {
		hhFxInfo fxInfo;

		fxInfo.SetNormal( -GetAxis()[0] );
		fxInfo.SetEntity( this );
		fxInfo.RemoveWhenDone( false );
		fxFly = SpawnFxLocal( defName, GetOrigin(), GetAxis(), &fxInfo, gameLocal.isClient );
		if (fxFly.IsValid()) {
			fxFly->fl.neverDormant = true;

			fxFly->fl.networkSync = false;
			fxFly->fl.clientEvents = true;
		}
	}
}

//=============================================================================
//
// hhWraith::FindEnemy
//
//=============================================================================
void hhWraith::Event_FindEnemy( int useFOV ) {
	int			i;
	idEntity	*ent;
	idActor		*closest;
	idActor		*actor;
	float		closestDist;
	float		distSquared;

	closest = NULL;
	closestDist = 99999999.0f;

	if (team != 0) {	// Search players
		for ( i = 0; i < gameLocal.numClients ; i++ ) {
			ent = gameLocal.entities[ i ];

			if ( !ent || !ent->IsType( idActor::Type ) ) {
				continue;
			}

			if ( ent->IsType( hhPlayer::Type ) ) { // Check if the player is really dead or deathwalking
				hhPlayer *player = static_cast< hhPlayer * >( ent );
				if ( player->IsDead() || player->IsDeathWalking() ) {
					continue;
				}
			}

			// Find the closest enemy
			distSquared = ( GetOrigin() - ent->GetOrigin() ).LengthSqr();
			if ( distSquared < closestDist ) {
				closestDist = distSquared;
				closest = static_cast<idActor *>( ent );
			}
		}
	}
	else {		// Search monsters
		int numMonsters = hhMonsterAI::allSimpleMonsters.Num();
		for ( i = 0; i < numMonsters ; i++ ) {
			actor = hhMonsterAI::allSimpleMonsters[ i ];

			if ( !actor || actor==this ) {
				continue;
			}

			// Ignore dormant entities!
			if (actor->IsHidden() || actor->fl.isDormant) {
				continue;
			}

			if ( (actor->health <= 0) || !(ReactionTo(actor) & ATTACK_ON_SIGHT) ) {
				continue;
			}

			distSquared = (actor->GetOrigin() - GetOrigin()).LengthSqr();
			if ( distSquared < closestDist ) {
				closestDist = distSquared;
				closest = actor;
			}
		}
	}

	if (closest) {
		SetEnemy( closest );
	}
	else {
		// FIXME: No enemies found, should we just go away or circle for a while?
		Damage(this, this, vec3_origin, "damage_suicide", 1.0f, INVALID_JOINT);
	}

	idThread::ReturnEntity( closest );
}

//=============================================================================
//
// hhWraith::UpdateEnemyPosition
//
// This is here to override the default UpdateEnemyPosition, which
// runs code that is unnecessary for the wraiths
//=============================================================================
void hhWraith::UpdateEnemyPosition( void ) {
	// Do nothing.  Wraith's don't need to check if the enemy is no longer visible, since they can go through walls
}

//=============================================================================
//
// hhWraith::EnemyDead
//
//=============================================================================
void hhWraith::EnemyDead() {
	if ( !enemy.IsValid() || !enemy->IsType( hhPlayer::Type ) ) { // Only consider the enemy dead if the enemy is not a player
		hhMonsterAI::EnemyDead();
	} else if ( enemy.IsValid() && enemy->IsType( hhPlayer::Type ) ) { // If the player is deathwalking, then the enemy is dead
		hhPlayer *player = static_cast< hhPlayer * >( enemy.GetEntity() );
		if ( player->IsDead() || player->IsDeathWalking() ) {
			hhMonsterAI::EnemyDead();
		}
	}
}

//=============================================================================
//
// hhWraith::FlyMove
//
//=============================================================================
void hhWraith::FlyMove( void ) {
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
//	case WS_POSSESS_CHARGE:
//		WraithDamageEnemy();
//		break;
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
// hhWraith::FlyToEnemy
//
//=============================================================================
void hhWraith::FlyToEnemy( void ) {
	float			delta = 0;
	bool			dir = false;
	int				anim;

	if ( !enemy.IsValid() ) {
		PostEventMS( &AI_FindEnemy, 1, 0 );
		return;
	}

	if (canPossess) {
		// Determine which direction the wraith needs to turn to the enemy (if the wraith should turn)
		if ( --straightTicks <= 0 ) {
			dir = GetFacePosAngle( enemy->GetOrigin(), delta );

			turnTicks++; // Add in the number of ticks that the wraith has been turning
			// If the wraith has been turning for a very long period of time (more than 4 seconds), then force the wraith to go straight
			
			// Vary turn_ticks by +/- 25 depending on the DDA value
			if ( turnTicks > this->turn_ticks - (25 * ((gameLocal.GetDDAValue() - 0.5f) * 2))) {
				dir = 0;
				delta = 0;
				turnTicks = 0;
				deltaViewAngles.yaw = 0;
			}
		} else {	
			dir = 0;
			delta = 0;
			deltaViewAngles.yaw = 0;
			turnTicks = 0;
		}
	}

	// Turn the wraith, and set the correct bank animation
	anim = flyAnim;
	if ( delta > this->turn_threshold ) { // Wraith should turn
		if ( dir ) { // Turn to the left
			deltaViewAngles.yaw = this->turn_radius_max;
			anim = leftAnim;
		} else {
			deltaViewAngles.yaw = -this->turn_radius_max;
			anim = rightAnim;
		}
	} else if ( straightTicks <= 0 ) { // Straight at the player
		deltaViewAngles.yaw = 0;
		// Vary straight_ticks by +/- 25 depending on the DDA value
		straightTicks = this->straight_ticks - (25 * ((gameLocal.GetDDAValue() - 0.5f) * 2)); // Stay on this path for a short period of time
	}

	// Actually set the animation
	if ( anim != lastAnim ) {
		GetAnimator()->ClearAllAnims( gameLocal.time, TWEEN_BANK );
		GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, TWEEN_BANK );
		lastAnim = anim;
	}

	velocity = viewAxis[0] * this->velocity_xy;
	if ( damageTicks > 0 ) { // Fly slower if damaged
		velocity *= 0.85f;
		velocity += 10 * idVec3( gameLocal.random.CRandomFloat(), gameLocal.random.CRandomFloat(), gameLocal.random.CRandomFloat() );

		damageTicks--;
	}

	// Equalize the z-height of the wraith relative to its target	
	idVec3 enemyOrigin = enemy->GetEyePosition();
	idVec3 origin = GetPhysics()->GetOrigin();

	// Factor in fly_offset
	enemyOrigin.z += fly_offset;

	// Adjust z-velocity based upon the delta Z to target
	velocity.z = 0;
	if( ( origin.z > enemyOrigin.z + this->target_z_threshold ) || ( origin.z < enemyOrigin.z - this->target_z_threshold ) ) {
		float newZ = enemyOrigin.z + target_z_threshold * 0.5f;
		float deltaZ = newZ - origin.z;
		if ( abs( deltaZ ) > this->dist_z_far ) {
			velocity.z = this->velocity_z_fast * ( ( deltaZ > 0 ) ? 1 : -1 );
		} else if ( abs( deltaZ ) > this->dist_z_close ) {
			velocity.z = this->velocity_z * ( ( deltaZ > 0 ) ? 1 : -1 );
		}
	}

	// 1.5x the velocity if DDA is 1.0, half it for DDA = 0.0, leave it the same for normal (DDA = 0.5)
	GetPhysics()->SetOrigin( GetPhysics()->GetOrigin() + (velocity * (gameLocal.GetDDAValue() + 0.5f)));

	UpdateVisuals();
}

//=============================================================================
//
// hhWraith::FlyAway
//
// Wraiths will automatically fly away from their enemy and up
// The Wraith will eventually be removed from the world in the CheckFleeRemove function
//=============================================================================
void hhWraith::FlyAway( void ) {
	
	velocity.x = 0;
	velocity.y = 0;
	velocity.z = this->flee_speed_z;

	GetPhysics()->SetOrigin( GetPhysics()->GetOrigin() + velocity );

	UpdateVisuals();

	// Check to see if the wraith has flown out of the world
	CheckFleeRemove();
}

//=============================================================================
//
// hhWraith::FlyUp
//
// mdl:  Wraiths will automatically fly up after being spawned if spawnFlyUp is set to 1
// NOT related to flying away during death, that's FlyAway()
//=============================================================================
void hhWraith::FlyUp( void ) {
	idVec3	velocity;

	// Turn towards desired angle
	// TODO:  Make this smooth
	SetAxis( desiredAxis );

	// Apply velocity 
	velocity = desiredAxis[0] * desiredVelocity;
	SetOrigin( GetOrigin() + velocity );

	UpdateVisuals();

	// If the wraith is near the end of the spawn, then smoothly rotate the wraith to the horizontal	
	if ( --countDownTimer <= 0 ) {
		state = WS_FLY;
	}
}

//=============================================================================
//
// hhWraith::TurnTowardEnemy
//
//=============================================================================
void hhWraith::TurnTowardEnemy() {
	float			delta;
	bool			dir;

	if ( !enemy.IsValid() ) {
		return;
	}

	// Face torward the enemy
	dir = GetFacePosAngle( enemy->GetOrigin(), delta );

	if ( delta > turn_threshold ) {
		// Turn based upon delta angles
		if ( dir ) { // Turn to the right
			deltaViewAngles.yaw = turn_radius_max;
		} else {
			deltaViewAngles.yaw = -turn_radius_max;
		}
	}

	UpdateVisuals();
}

//=============================================================================
//
// hhWraith::CheckFleeRemove
//
//=============================================================================
void hhWraith::CheckFleeRemove( void ) {
	// Check if the Wraith has flown out of the world
	if( gameLocal.clip.Contents( GetPhysics()->GetOrigin() + idVec3(0, 0, 48), GetPhysics()->GetClipModel(), viewAxis, CONTENTS_SOLID, this ) ) {
		Hide();
		PostEventSec( &EV_Remove, 2.0f ); // Keep the wraith around for a few seconds so the sounds can finish
		state = WS_STILL; // No reason to keep moving the wraith
	}
}

//=============================================================================
//
// hhWraith::Damage
//
//=============================================================================
void hhWraith::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	hhMonsterAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	damageTicks = this->damage_ticks;
}

//=============================================================================
//
// hhWraith::Killed
//
//=============================================================================
void hhWraith::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	hhFxInfo fx;
	//fx.SetEntity( this );
	fx.RemoveWhenDone( true );
	const char *deathFX;

	// Stop flyloop and chatter sounds, if any
	StopSound( SND_CHANNEL_BODY );
	StopSound( SND_CHANNEL_VOICE );

	if ( inflictor == this ) {
		// If we timed out, don't drop a soul
		spawnArgs.Delete( "def_dropSoul" );
		deathFX = spawnArgs.GetString( "fx_death" );
	} else {
		// Use alternate death fx if we were killed by the player
		deathFX = spawnArgs.GetString( "fx_death2" );
	}

	SpawnFxLocal( deathFX, GetOrigin(), GetAxis(), &fx );

	SAFE_REMOVE(fxFly);

	hhMonsterAI::Killed( inflictor, attacker, damage, dir, location );

	StartSound( "snd_death", SND_CHANNEL_VOICE );

	Hide();

	// activate targets
	//ActivateTargets( this );

	PostEventSec( &EV_Remove, 4 );
}

//=============================================================================
//
// hhWraith::CheckCollisions
//
// Check if the wraith is entering or exiting the world
//=============================================================================
void hhWraith::CheckCollisions( void ) {

	if (gameLocal.time < nextCheckTime) {
		return;
	}

	hhFxInfo	fxInfo;
	trace_t		tr1;
	trace_t		tr2;
	idVec3		jointPos;
	idMat3		jointAxis;

	if (!GetJointWorldTransform( spawnArgs.GetString("joint_collision"), jointPos, jointAxis )) {
		jointPos = GetOrigin();
		jointAxis = GetAxis();
	}

	// Trace to determine if the wraith just flew through a wall, or just emerged from a wall
	// For speed, this only checks against the world
	idVec3 dir, point;
	gameLocal.clip.TracePoint( tr1, jointPos, lastCheckOrigin, MASK_SHOT_BOUNDINGBOX, this );	// Check if we entered something solid
	gameLocal.clip.TracePoint( tr2, lastCheckOrigin, jointPos, MASK_SHOT_BOUNDINGBOX, this );	// Check if we exited something solid

	idVec3 playerOrigin = gameLocal.GetLocalPlayer()->GetOrigin();
	if ( tr1.fraction < 1.0f ) {
		point = tr1.c.point;
		dir = tr1.c.normal;
	}
	else if ( tr2.fraction < 1.0f) {
		point = tr2.c.point;
		dir = tr2.c.normal;
	}
	if ( tr1.fraction < 1.0f && tr2.fraction < 1.0f ) {
		if ( (tr1.c.point - playerOrigin).LengthSqr() <= (tr2.c.point - playerOrigin).LengthSqr() ) {
			point = tr1.c.point;
			dir = tr1.c.normal;
		}
		else {
			point = tr2.c.point;
			dir = tr2.c.normal;
		}
	}

	if ( tr1.fraction < 1.0f || tr2.fraction < 1.0f ) {
		// Spawn an FX at wall collision
		//TODO: IMPORTANT: This currently spawns a system every check while passing through something.  This creates enormous
		// amounts of particles.  Fix so there is a single system spawned on the way in and a single system spawned on the way out.
		//TODO: This currently hits players so you get a big blob of particles right in your face and right behind you.
		const char *hitWallFxName = spawnArgs.GetString("fx_hitwall");
		if (hitWallFxName && *hitWallFxName) {
			fxInfo.RemoveWhenDone( true );
			BroadcastFxInfo(hitWallFxName, point, mat3_identity, &fxInfo);
		}


		// Project a spooge decal on the wall here
		const char *decalName = spawnArgs.GetString("mtr_decal", NULL);
		if (decalName && *decalName) {
			float depth = spawnArgs.GetFloat("decal_trace");
			float size = spawnArgs.GetFloat( "decal_size" );

//			gameRenderWorld->DebugArrow(colorRed, point, point + dir*128, 5, 10000);

			gameLocal.ProjectDecal(tr1.c.point, -tr1.c.normal, depth, true, size, decalName);
			gameLocal.ProjectDecal(tr2.c.point, -tr2.c.normal, depth, true, size, decalName);
		}
	}

	// Possession check
	if ( canPossess && gameLocal.time > nextPossessTime ) {
		gameLocal.clip.TraceBounds( tr1, lastCheckOrigin, jointPos, GetPhysics()->GetBounds(), MASK_SHOT_BOUNDINGBOX, this );
		if ( gameLocal.entities[ tr1.c.entityNum ] && gameLocal.entities[ tr1.c.entityNum ]->IsType( idActor::Type ) ) {
			// If we hit a possessable actor, possess them
			idActor *actor = reinterpret_cast<idActor *> ( gameLocal.entities[ tr1.c.entityNum ] );
			if ( actor != lastActor.GetEntity() ) {
				// Play the attack sound
				StartSound( "snd_attack", SND_CHANNEL_VOICE );
				if ( actor->IsType( hhPlayer::Type ) ) {
					hhPlayer *player = reinterpret_cast<hhPlayer *> ( gameLocal.entities[ tr1.c.entityNum ] );
					int power = player->GetSpiritPower();
					power -= 25;

					if ( power <= 0 ) { // Possess the player
						power = 0;
						// mdl:  Wraiths no longer possess
						//if ( player->CanBePossessed() ) {
							// Possessable
							//WraithPossess( player );
						//} else
						if ( enemy.GetEntity() == player && player->IsSpiritWalking() && !player->IsPossessed() ) {
							// Send the player back to his body
							reinterpret_cast<hhPlayer *> (enemy.GetEntity())->DisableSpiritWalk(5);
							// Don't immediately possess if the players body happens to be right there.
							nextPossessTime = gameLocal.time + 1000;
						}
					} else { // Gain health from the player's spirit power
						health += 5;
						if ( health > spawnHealth ) {
							health = spawnHealth;
						}
					}
					player->SetSpiritPower( power );

				}// else if ( actor->CanBePossessed() ) {
				//	// Possess the actor
				//	WraithPossess( actor );
				//}
			}
			lastActor = actor;
		} else {
			lastActor = NULL;
		}
	}

	nextCheckTime = gameLocal.time + spawnArgs.GetInt("trace_check_time", "200");
	lastCheckOrigin = jointPos;
}

//=============================================================================
//
// hhWraith::Event_Flee
//
// The wraith is about to flee and has already played the flee intro anim
//=============================================================================
void hhWraith::Event_Flee( ) {
	GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, fleeAnim, gameLocal.time, 0 );
	state = WS_FLEE;
}

//=============================================================================
//
// hhWraith::PlayAnimMove
//
//=============================================================================
void hhWraith::PlayAnimMove( int anim, int blendTime ) {
	GetAnimator()->ClearAllAnims( gameLocal.time, blendTime );
	GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, blendTime );
	state = WS_STILL;
	PostEventMS( &EV_PlayAnimMoveEnd, GetAnimator()->GetAnim( possessAnim )->Length() - blendTime );
}

//=============================================================================
//
// hhWraith::PlayAnimMoveEnd
//
//=============================================================================
void hhWraith::PlayAnimMoveEnd() {
	idVec3 boneOrigin;
	idMat3 boneAxis;
	GetJointWorldTransform( "Head", boneOrigin, boneAxis );
	
	GetPhysics()->SetOrigin( boneOrigin ); // Move the wraith to the end of the animation

	GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, flyAnim, gameLocal.time, 0 );
	state = WS_FLY;

	damageTicks = 0;
}

//=============================================================================
//
// hhWraith::WraithPossess
//
// Actually possess an actor
//=============================================================================
void hhWraith::WraithPossess( idActor *actor ) {
	hhFxInfo fxInfo;

	actor->Possess( this );

	// Spawn in a flash
	fxInfo.RemoveWhenDone( true );
	BroadcastFxInfoPrefixed( "fx_possessionFlash", GetOrigin(), GetAxis(), &fxInfo );

	// activate targets
	ActivateTargets( actor );

	PostEventMS( &EV_Remove, 0 );
}


//=============================================================================
//
// hhWraith::Event_PlayAnimMoveEnd
//
//=============================================================================
void hhWraith::Event_PlayAnimMoveEnd( ) {
	PlayAnimMoveEnd();
}

void hhWraith::Event_EnemyIsSpirit( hhPlayer *player, hhSpiritProxy *proxy ) {
	// Only switch to the proxy if the player has no spirit power
	//if ( player->GetSpiritPower() == 0 ) {
	//	hhMonsterAI::Event_EnemyIsSpirit( player, proxy );
	//}
}

void hhWraith::Event_EnemyIsPhysical( hhPlayer *player, hhSpiritProxy *proxy ) {
	// We don't care whether we can see the enemy or not
	enemy = player;
}

void hhWraith::TeleportIn( idEntity *activator ) {
	if ( spawnArgs.GetBool( "quickSpawn" ) ) {
		PostEventMS( &EV_Show, 0 );
		PostEventMS( &EV_Activate, 50, activator );
	}
	else {
		hhFxInfo fx;
		fx.RemoveWhenDone( true );
		fx.SetEntity( this );
		SpawnFxLocal( spawnArgs.GetString( "fx_spawn" ), GetOrigin(), GetAxis(), &fx );
		PostEventMS( &EV_Show, 1000 );
		PostEventMS( &EV_Activate, 1050, activator );
	}
}

void hhWraith::Portalled(idEntity *portal) {
	hhMonsterAI::Portalled( portal );
	if ( fxFly.IsValid() ) {
		hhFxInfo fxInfo;
		fxInfo.SetNormal( -GetAxis()[0] );
		fxInfo.SetEntity( this );
		fxInfo.RemoveWhenDone( false );

		fxFly->SetFxInfo( fxInfo );

		// Reset the fx system
		fxFly->Stop();
		fxFly->Start( gameLocal.time );
	}	
}

