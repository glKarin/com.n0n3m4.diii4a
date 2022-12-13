//*****************************************************************************
//**
//** GAME_TALON.CPP
//**
//** Game code for Tommy's sidekick, Talon the Hawk
//**
//*****************************************************************************

// HEADER FILES ---------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// MACROS ---------------------------------------------------------------------

#define PERCH_ROTATION				3.5
#define ROTATION_SPEED				3.5
#define ROTATION_SPEED_FAST			5
#define TALON_BLEND					100

// TYPES ----------------------------------------------------------------------

// CLASS DECLARATIONS ---------------------------------------------------------

const idEventDef EV_PerchTicker("<perchTicker>", NULL);
const idEventDef EV_PerchChatter("perchChatter", NULL);
const idEventDef EV_PerchSquawk("<perchSquawk>", NULL);

const idEventDef EV_CheckForTarget( "<checkForTarget>", NULL );
const idEventDef EV_CheckForEnemy( "<checkForEnemy>", NULL );

const idEventDef EV_TalonAction("talonaction", "ed");

// Anim Events
const idEventDef EV_LandAnim("<landAnim>", NULL);
const idEventDef EV_PreLandAnim("<prelandAnim>", NULL);
const idEventDef EV_IdleAnim("<idleAnim>", NULL);
const idEventDef EV_TommyIdleAnim("<tommyIdleAnim>", NULL);
const idEventDef EV_FlyAnim("<flyAnim>", NULL);
const idEventDef EV_GlideAnim("<glideAnim>", NULL);
const idEventDef EV_TakeOffAnim("<takeOffAnim>", NULL);
const idEventDef EV_TakeOffAnimB("<takeOffAnimB>", NULL);

CLASS_DECLARATION( hhMonsterAI, hhTalon )
	EVENT(EV_PerchChatter,		hhTalon::Event_PerchChatter)
	EVENT(EV_PerchSquawk,		hhTalon::Event_PerchSquawk)
	EVENT(EV_CheckForTarget,	hhTalon::Event_CheckForTarget)
	EVENT(EV_CheckForEnemy,		hhTalon::Event_CheckForEnemy)

	// Anims
	EVENT(EV_LandAnim,			hhTalon::Event_LandAnim)
	EVENT(EV_PreLandAnim,		hhTalon::Event_PreLandAnim)
	EVENT(EV_IdleAnim,			hhTalon::Event_IdleAnim)
	EVENT(EV_TommyIdleAnim,		hhTalon::Event_TommyIdleAnim)
	EVENT(EV_FlyAnim,			hhTalon::Event_FlyAnim)
	EVENT(EV_GlideAnim,			hhTalon::Event_GlideAnim)
	EVENT(EV_TakeOffAnim,		hhTalon::Event_TakeOffAnim)
	EVENT(EV_TakeOffAnimB,		hhTalon::Event_TakeOffAnimB)
END_CLASS

const idEventDef EV_CallTalon("callTalon", "fff" );
const idEventDef EV_ReleaseTalon("releaseTalon", NULL );
const idEventDef EV_SetPerchState("setPerchState", "f" );

CLASS_DECLARATION( idEntity, hhTalonTarget )
	EVENT( EV_CallTalon,			hhTalonTarget::Event_CallTalon )
	EVENT( EV_ReleaseTalon,			hhTalonTarget::Event_ReleaseTalon )
	EVENT( EV_SetPerchState,		hhTalonTarget::Event_SetPerchState )
END_CLASS

// STATE DECLARATIONS ---------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES -----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ------------------------------------------------

// EXTERNAL DATA DECLARATIONS -------------------------------------------------

// PUBLIC DATA DEFINITIONS ----------------------------------------------------

// PRIVATE DATA DEFINITIONS ---------------------------------------------------

// CODE -----------------------------------------------------------------------

//=============================================================================
//
// hhTalon::Spawn
//
//=============================================================================

void hhTalon::Spawn(void) {
	flyAnim			= GetAnimator()->GetAnim("fly");
	glideAnim		= GetAnimator()->GetAnim("glide");
	prelandAnim		= GetAnimator()->GetAnim("preland");
	landAnim		= GetAnimator()->GetAnim("land");
	idleAnim		= GetAnimator()->GetAnim("idle");
	tommyIdleAnim	= GetAnimator()->GetAnim("tommy_idle");
	squawkAnim		= GetAnimator()->GetAnim("squawk");
	stepAnim		= GetAnimator()->GetAnim("step");
	takeOffAnim		= GetAnimator()->GetAnim("takeoffA");
	takeOffAnimB	= GetAnimator()->GetAnim("takeoffB");
	attackAnim		= GetAnimator()->GetAnim("attack1");
	preAttackAnim	= GetAnimator()->GetAnim("preattack");

	fl.neverDormant = true;
	fl.takedamage = false;
	fl.notarget = true;

	allowHiddenMovement = true; // Allow Talon to move while hidden

	health = 1; // Talon need some health for enemies to consider it alive

	//AOB
	GetAnimator()->RemoveOriginOffset( true );

	// Register the wings skins
	openWingsSkin = declManager->FindSkin( spawnArgs.GetString("skin_openWings") );
	closedWingsSkin = declManager->FindSkin( spawnArgs.GetString("skin_closeWings") );

	Event_SetMoveType(MOVETYPE_FLY);

	GetPhysics()->SetContents( 0 ); // No collisions
	GetPhysics()->SetClipMask( 0 ); // No collisions
	Hide();
	BecomeInactive(TH_THINK);
}

void hhTalon::Save(idSaveGame *savefile) const {
	owner.Save(savefile);
	savefile->WriteVec3(velocity);
	savefile->WriteVec3(acceleration);

	savefile->WriteObject(talonTarget);
	savefile->WriteVec3(talonTargetLoc);
	savefile->WriteMat3(talonTargetAxis);
	savefile->WriteVec3(lastCheckOrigin);

	savefile->WriteFloat(checkTraceTime);
	savefile->WriteFloat(checkFlyTime);
	savefile->WriteFloat(flyStraightTime);

	savefile->WriteBool(bLanding);
	savefile->WriteBool(bReturnToTommy);
	savefile->WriteBool( bForcedTarget );
	savefile->WriteBool( bClawingAtEnemy );

	savefile->WriteFloat( velocityFactor );
	savefile->WriteFloat( rotationFactor );
	savefile->WriteFloat( perchRotationFactor );

	savefile->WriteInt(flyAnim);
	savefile->WriteInt(glideAnim);
	savefile->WriteInt(prelandAnim);
	savefile->WriteInt(landAnim);
	savefile->WriteInt(idleAnim);
	savefile->WriteInt(tommyIdleAnim);
	savefile->WriteInt(squawkAnim);
	savefile->WriteInt(stepAnim);
	savefile->WriteInt(takeOffAnim);
	savefile->WriteInt(takeOffAnimB);
	savefile->WriteInt(attackAnim);
	savefile->WriteInt(preAttackAnim);

	savefile->WriteSkin(openWingsSkin);
	savefile->WriteSkin(closedWingsSkin);

	enemy.Save( savefile );
	trailFx.Save( savefile );

	savefile->WriteInt( state );
}

void hhTalon::Restore( idRestoreGame *savefile ) {
	owner.Restore(savefile);
	savefile->ReadVec3(velocity);
	savefile->ReadVec3(acceleration);

	savefile->ReadObject( reinterpret_cast<idClass *&>(talonTarget) );
	savefile->ReadVec3(talonTargetLoc);
	savefile->ReadMat3(talonTargetAxis);
	savefile->ReadVec3(lastCheckOrigin);

	savefile->ReadFloat(checkTraceTime);
	savefile->ReadFloat(checkFlyTime);
	savefile->ReadFloat(flyStraightTime);

	savefile->ReadBool(bLanding);
	savefile->ReadBool(bReturnToTommy);
	savefile->ReadBool( bForcedTarget );
	savefile->ReadBool( bClawingAtEnemy );

	savefile->ReadFloat( velocityFactor );
	savefile->ReadFloat( rotationFactor );
	savefile->ReadFloat( perchRotationFactor );

	savefile->ReadInt(flyAnim);
	savefile->ReadInt(glideAnim);
	savefile->ReadInt(prelandAnim);
	savefile->ReadInt(landAnim);
	savefile->ReadInt(idleAnim);
	savefile->ReadInt(tommyIdleAnim);
	savefile->ReadInt(squawkAnim);
	savefile->ReadInt(stepAnim);
	savefile->ReadInt(takeOffAnim);
	savefile->ReadInt(takeOffAnimB);
	savefile->ReadInt(attackAnim);
	savefile->ReadInt(preAttackAnim);

	savefile->ReadSkin(openWingsSkin);
	savefile->ReadSkin(closedWingsSkin);

	enemy.Restore( savefile );
	trailFx.Restore( savefile );

	savefile->ReadInt( reinterpret_cast<int &> (state) );
}

//=============================================================================
//
// hhTalon::hhTalon
//
//=============================================================================

hhTalon::hhTalon() {
	owner = NULL;
	talonTarget = NULL;
	enemy.Clear();
	trailFx.Clear();
}

//=============================================================================
//
// hhTalon::~hhTalon
//
//=============================================================================

hhTalon::~hhTalon() {
	if( owner.IsValid() ) {
		owner->talon = NULL;
	}

	if ( trailFx.IsValid() ) {
		trailFx->Nozzle( false );
		SAFE_REMOVE( trailFx );
	}
}

//=============================================================================
//
// hhTalon::SummonTalon
//
//=============================================================================

void hhTalon::SummonTalon(void) {	
	hhFxInfo	fxInfo;

	state = StateNone;

	BecomeActive(TH_THINK);	
	Show();

	checkTraceTime = spawnArgs.GetFloat( "traceCheckPulse", "0.1" );
	lastCheckOrigin = GetOrigin();

	FindTalonTarget(NULL, NULL, true);

	CalculateTalonTargetLocation(); // Must recalculate the target location, as the target may have moved
	SetOrigin(talonTargetLoc);
	viewAxis = talonTargetAxis;

	bLanding = false;

	SetSkin( closedWingsSkin ); // The only time this is set in code, otherwise it's set by the land anim

	fl.neverDormant = true;

	SetForcedTarget( false );

	velocityFactor = 1.0f;
	rotationFactor = 1.0f;
	perchRotationFactor = 1.0f;

	SetShaderParm( SHADERPARM_DIVERSITY, 0 );

	EnterTommyState();
	TommyTicker(); // Force Talon to tick once to adjust his orientation
}

//=============================================================================
//
// hhTalon::SetOwner
//
//=============================================================================

void hhTalon::SetOwner( hhPlayer *newOwner ) {
	owner = newOwner;
	Event_SetOwner( newOwner );
}	

void hhTalon::OwnerEnteredVehicle( void ) {
	if ( talonTarget ) {
		talonTarget->Left( this );
	}

	SummonTalon();
	EnterVehicleState();
}

void hhTalon::OwnerExitedVehicle( void ) {
	ExitVehicleState();
	SummonTalon();
}

//=============================================================================
//
// hhTalon::FindTalonTarget
//
//=============================================================================

void hhTalon::FindTalonTarget( idEntity *skipEntity, hhTalonTarget *forceTarget, bool bForcePlayer ) {
	int				i;
	int				count;
	float			dist;
	float			bestDist;
	hhTalonTarget	*ent;
	idVec3			dir;

	talonTarget = NULL;
 	bestDist = spawnArgs.GetFloat( "perchDistance", "250" );

	if ( owner->InVehicle() ) {
		bestDist *= 10;
	}

	if( bReturnToTommy) {
		bForcePlayer = true;
	}

	if ( forceTarget ) {
		talonTarget = forceTarget;
	}

	// Iterate through valid perch spots and find the best choice
	if( !bForcePlayer && !forceTarget ) {
		count = gameLocal.talonTargets.Num();
		for(i = 0; i < count; i++) {
			ent = (hhTalonTarget *)gameLocal.talonTargets[i];

			if ( !ent->bValidForTalon ) { // Not a valid target
				continue;
			}

			// ignore if there is no way we can see this perch spot
			if( ent == skipEntity || !gameLocal.InPlayerPVS( ent ) || ent->priority == -1) {
				continue;
			}

			if ( owner.IsValid() && owner->IsSpiritOrDeathwalking() && ent->bNotInSpiritWalk ) { // If the owner is spiritwalking, then don't try to fly to this target (useful for lifeforce pickups)
				continue;
			}

			// Calculate the distance from the player to the perch spot		
			dist = ( owner->GetEyePosition() - ent->GetPhysics()->GetOrigin() ).Length();

			// Adjust the distance based upon the priority (ranges from 0 - 2, -1 to completely skip).
			dist *= ent->priority;

			if(dist > bestDist) { // Farther than the nearest perch spot
				continue;
			}

			talonTarget = ent;
			bestDist = dist;
		}
	}

	if( !talonTarget && owner.IsValid() ) { // No perch spot, so use a spot close to the player
		owner->GetJointWorldTransform( "FX_bird", talonTargetLoc, talonTargetAxis );
		talonTargetAxis = owner->viewAxis;
	} else {
		CalculateTalonTargetLocation();
	}
}

//=============================================================================
//
// hhTalon::CalculateTalonTargetLocation
//
//=============================================================================

void hhTalon::CalculateTalonTargetLocation() {
	trace_t tr;

	if ( !talonTarget ) {
		return;
	}

	// Trace down a bit to get the exact perch location
	if( gameLocal.clip.TracePoint( tr, talonTarget->GetPhysics()->GetOrigin() + idVec3(0, 0, 4), talonTarget->GetPhysics()->GetOrigin() - idVec3(0, 0, 16), MASK_SOLID, this )) {
		talonTargetLoc = tr.endpos - idVec3( 0, 0, 1 ); // End position, minus a tiny bit so Talon perches on railings
	} else { // No collision, just use the current floating spot
		talonTargetLoc = talonTarget->GetPhysics()->GetOrigin();
	}

	talonTargetAxis = talonTarget->GetPhysics()->GetAxis();
}

//=============================================================================
//
// CheckReachedTarget
//
//=============================================================================

bool hhTalon::CheckReachedTarget( float distance ) {
	idVec3			vec;

	if(distance < spawnArgs.GetFloat( "distanceSlow", "80" ) ) {
		if(talonTarget) {
			velocity = (talonTargetLoc - GetOrigin()) * 0.05f; // Move more slowly when about to perch

			if( !GetAnimator()->IsAnimPlaying( GetAnimator()->GetAnim( prelandAnim ) ) ) {
				Event_PreLandAnim();
				StartSound( "snd_preland", SND_CHANNEL_BODY );
			}
			bLanding = true;
		}

		if(talonTarget) {
			if(distance < spawnArgs.GetFloat( "distancePerchEpsilon", "3" ) ) { // Perch on this spot
				Event_LandAnim();
				StartSound( "snd_land", SND_CHANNEL_BODY );

				talonTarget->Reached(this);
				EnterPerchState();

				return true; // No need to continue the fly ticker
			}
		} else if(distance < spawnArgs.GetFloat( "distanceTommyEpsilon", "15.0" ) ) { // No perch spot, so perch on Tommy around this location
			Event_LandAnim();
			EnterTommyState();
			return true; // No need to continue the fly ticker
		}
	} else if(distance >= spawnArgs.GetFloat( "distanceTurbo", "2500" ) ) { // Distance is so far, we should kick Talon into turbo mode
		velocity *= 10;
	}

	return false;
}

//=============================================================================
//
// hhTalon::CheckCollisions
//
//=============================================================================

void hhTalon::CheckCollisions(float deltaTime) {
	idVec3			oldOrigin;
	trace_t			tr;

	checkTraceTime -= deltaTime;

	if(checkTraceTime > 0) {
		return;
	}

	checkTraceTime = spawnArgs.GetFloat( "traceCheckPulse", "0.1" );

	idVec3 nextLocation = GetOrigin() + this->velocity * 0.5f;

	// Trace to determine if the hawk is just about to fly into a wall
	if ( gameLocal.clip.TracePoint( tr, GetOrigin(), nextLocation, MASK_SOLID, owner.GetEntity() ) ) { // Struck something solid
		// Glow
		SetShaderParm( SHADERPARM_TIMEOFFSET, MS2SEC( gameLocal.time ) );
	} else if ( gameLocal.clip.TracePoint( tr, GetOrigin(), lastCheckOrigin, MASK_SOLID, owner.GetEntity() ) ) { // Exited something solid
		// Glow
		SetShaderParm( SHADERPARM_TIMEOFFSET, MS2SEC( gameLocal.time ) );
	}

	lastCheckOrigin = GetOrigin();
}

//=============================================================================
//
// hhTalon::Think
//
//=============================================================================

void hhTalon::Think(void) {

	if ( owner->InGravityZone() ) { // don't perch in gravity zones, unless gravity is "normal"
		idVec3 gravity = owner->GetGravity();
		gravity.Normalize();

		float dot = gravity * idVec3( 0.0f, 0.0f, -1.0f );
		if ( dot < 1.0f ) {
			if ( talonTarget ) {
				talonTarget->Left( this );
			}

			ReturnToTommy();
		}
	}

	// Shadow suppression based upon the player
	if ( g_showPlayerShadow.GetBool() ) {
		renderEntity.suppressShadowInViewID	= 0;
	}
	else {
		renderEntity.suppressShadowInViewID	= owner->entityNumber + 1;
	}

	hhMonsterAI::Think();
}

//=============================================================================
//
// hhTalon::FlyMove
//
//=============================================================================

void hhTalon::FlyMove( void ) {
	// Run the specific task Ticker
	switch( state ) {
		case StateTommy:
			TommyTicker();
			break;
		case StateFly:
			FlyTicker();
			break;
		case StateVehicle:
			VehicleTicker();
			break;
		case StatePerch:
			PerchTicker();
			break;
		case StateAttack:		
			AttackTicker();
			break;
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
// hhTalon::AdjustFlyingAngles
//
// No need to adjust flying angles on Talon
//=============================================================================

void hhTalon::AdjustFlyingAngles() {
	return;
}

//=============================================================================
//
// hhTalon::GetPhysicsToVisualTransform
//
//=============================================================================

bool hhTalon::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	origin = modelOffset;
	if ( GetBindMaster() && bindJoint != INVALID_JOINT ) {
		idMat3 masterAxis;
		idVec3 masterOrigin;
		GetMasterPosition( masterOrigin, masterAxis );

		origin = masterOrigin + physicsObj.GetLocalOrigin() * masterAxis;
		axis = GetBindMaster()->GetAxis();
	} else {
		axis = viewAxis;
	}
	return true;
}

//=============================================================================
//
// hhTalon::CrashLand
//
// No need to check crash landing on Talon
//=============================================================================

void hhTalon::CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity ) {
	return;
}

//=============================================================================
//
// hhTalon::Damage
//
// Talon cannot be damaged
//=============================================================================

void hhTalon::Damage( idEntity *inflictor, idEntity *attack, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	return;
}

//=============================================================================
//
// hhTalon::Killed
//
// Talon cannot be killed
//=============================================================================

void hhTalon::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	return;
}

//=============================================================================
//
// hhTalon::Portalled
//
//=============================================================================

void hhTalon::Portalled( idEntity *portal ) {
	if ( state == StateTommy ) { // No need to portal if Talon is already on Tommy's shoulder
		return;
	}

	CancelEvents( &EV_GlideAnim ); // Could possibly have a glide anim queue
	CancelEvents( &EV_TakeOffAnimB );

	// If Talon is currently on a gui, inform the gui he is leaving
	if ( talonTarget && ( state == StatePerch || state == StateVehicle ) ) { // CJR PCF 050306:  Only call Left() if Talon is perching on a console
		talonTarget->Left( this );
	}

	StopAttackFX();
	
	// Force Talon to Tommy's shoulder after he portals
	talonTarget = NULL;
	bLanding = false;
	SetSkin( openWingsSkin ); // The only time this is set in code, otherwise it's set by the land anim
	Event_LandAnim();
	SetForcedTarget( false );
	EnterTommyState();
}

//=============================================================================
//
// hhTalon::EnterFlyState
//
//=============================================================================

void hhTalon::EnterFlyState(void) {
	EndState();
	state = StateFly;
	checkFlyTime = spawnArgs.GetFloat( "checkFlyTime", "4.0" );
	flyStraightTime = spawnArgs.GetFloat( "flyStraightTime", "0.5" );
	bLanding = false;

	SetShaderParm( SHADERPARM_DIVERSITY, 0 ); // Talon's model should phase out if the player is too close


}

//=============================================================================
//
// hhTalon::FlyTicker
//
//=============================================================================

void hhTalon::FlyTicker(void) {
	idAngles	ang;
	float		distance;
	float		deltaTime = MS2SEC(gameLocal.msec);
	idVec3		oldVel;
	float		distanceXY;
	float		deltaZ;
	float		rotSpeed;

	// If the bird is currently in fly mode, but hasn't finished the takeoff anim, then don't let the bird fly around
	if( GetAnimator()->IsAnimPlaying( GetAnimator()->GetAnim( takeOffAnim ) ) ) {
		return;
	}

	acceleration = talonTargetLoc - GetOrigin();

	idVec2 vecXY = acceleration.ToVec2();
	distance = acceleration.Normalize();

	distanceXY = vecXY.Length();
	deltaZ = talonTargetLoc.z - GetOrigin().z;

	float dp = acceleration * GetAxis()[0];
	float side = acceleration * GetAxis()[1];
	ang = GetAxis().ToAngles();

	// Talon can either rotate towards the target, or fly straight.
	if( flyStraightTime > 0 ) { // Continue to fly straight
		flyStraightTime -= deltaTime;

		if ( flyStraightTime <= 0 && !bLanding ) { // No longer flying straight
			Event_FlyAnim();
		}

		rotSpeed = 0;
	} else if( bLanding ) { // About to land, so turn slightly faster than normal
		rotSpeed = ROTATION_SPEED_FAST * rotationFactor;
	} else {
		rotSpeed = ROTATION_SPEED * rotationFactor;
	}


	rotSpeed *= (60.0f * USERCMD_ONE_OVER_HZ);

	// Determine whether Talon should rotate left or right to reach the target
	if(dp > 0) {
		if(dp >= 0.98f || ( side <= 0.1f && side >= 0.1f )) { // CJR PCF 04/28/06:  don't turn if directly facing the target
			rotSpeed = 0;
		} else if (side < -0.1f) {
			rotSpeed *= -1.0f;
		}
	} else {
		if(side < 0) {
			rotSpeed *= -1.0f;
		}
	}

	// Apply rotation
	deltaViewAngles.yaw = rotSpeed;
	oldVel = velocity;

	float defaultVelocityXY = spawnArgs.GetFloat( "velocityXY", "7.0" );
	float defaultVelocityZ = spawnArgs.GetFloat( "velocityZ", "4" );

	velocity = viewAxis[0] * defaultVelocityXY * velocityFactor; // xy-velocity

	// Z-change based upon distance
	if(abs(deltaZ) > 0 && distanceXY > 0) {
		velocity.z = (deltaZ * defaultVelocityXY) / distanceXY;

		// Clamp z velocity
		if(velocity.z > defaultVelocityZ) {
			velocity.z = defaultVelocityZ;
		}
	}

	if ( CheckReachedTarget( distance ) ) {
		return;
	}

	// Apply velocity
	SetOrigin( GetOrigin() + velocity * (60.0f * USERCMD_ONE_OVER_HZ) );

	// Check for colliding with world surfaces (only if not currently trying to land)
	if( !bLanding ) {
		CheckCollisions(deltaTime);
	}

	// Timeout for flying to perch spot... if too much time has passed, then fly straight for a bit
	checkFlyTime -= deltaTime;
	if( checkFlyTime <= 0 && !bLanding ) {
		flyStraightTime = spawnArgs.GetFloat( "flyStraightTime", "0.5" );		
		checkFlyTime = spawnArgs.GetFloat( "checkFlyTime", "4.0" );

		PostEventMS( &EV_GlideAnim, 0 );
	}

	// If no talonTarget (probably flying towards Tommy), then constantly look for a new target
	if( !bForcedTarget && !GetAnimator()->IsAnimPlaying( GetAnimator()->GetAnim( prelandAnim ) ) ) {
		FindTalonTarget( NULL, NULL, false );
	}
}

//=============================================================================
//
// hhTalon::EnterTommyState
//
//=============================================================================

void hhTalon::EnterTommyState(void) {
	idVec3		bindOrigin;
	idMat3		bindAxis;

	bReturnToTommy = false;

	EndState();
	state = StateTommy;

	Hide();

	UpdateVisuals();

	// Bind the bird to Tommy's shoulder
	owner->GetJointWorldTransform( "fx_bird", bindOrigin, bindAxis );
	SetOrigin( bindOrigin );
	SetAxis( owner->viewAxis );
	BindToJoint( owner.GetEntity(), "fx_bird", true );

	// Start checking for enemies
	PostEventSec( &EV_CheckForEnemy, spawnArgs.GetFloat("checkEnemyTime", "4.0" ) );

	// Start checking for nearby targets
	PostEventSec( &EV_CheckForTarget, spawnArgs.GetFloat( "checkTargetTime", "1.0" ) );
}

//=============================================================================
//
// hhTalon::EndState
//
//=============================================================================

void hhTalon::EndState(void) {
	Unbind();

	int oldState = state;
	state = StateNone;

	if ( fl.hidden ) { // If the bird is hidden, show it when ending the state
		Show();
	}

	// Zero out Talon's velocity when initially taking off, so it doesn't inherit any from the bind master
	velocity = vec3_zero;
	GetPhysics()->SetLinearVelocity( vec3_zero );

	// Cancel any checks for enemies
	CancelEvents( &EV_CheckForEnemy );
	CancelEvents( &EV_CheckForTarget );

	StopAttackFX();

	if( oldState == StateTommy ) {
		GetPhysics()->SetAxis( mat3_identity );

		// Show the hawk in the player's view after it takes off
		Show();
	} else if( oldState == StatePerch ) {
		CancelEvents( &EV_PerchSquawk );
	} else if ( oldState == StateAttack ) {
		// Clear the enemy
		if ( enemy.IsValid() ) {
			(static_cast<hhMonsterAI*>(enemy.GetEntity()))->Distracted( GetOwner() ); // Set the enemy's enemy back to Tommy
			enemy.Clear();
		}
	}
}

//=============================================================================
//
// hhTalon::TommyTicker
//
//=============================================================================

void hhTalon::TommyTicker(void) {
	idVec3		viewOrigin;
	idMat3		viewAxis;
	idMat3		ownerAxis;
	float		deltaTime = MS2SEC(gameLocal.msec);

	bLanding = false;

	// Turn towards direction of Tommy
	idVec3 toFace = owner->GetAxis()[0];
	toFace.z = 0;
	toFace.Normalize();
	float dp = toFace * GetAxis()[0];
	float side = toFace * GetAxis()[1];

	if(side > 0.05 || dp < 0 ) {
		deltaViewAngles.yaw = PERCH_ROTATION * perchRotationFactor;
		UpdateVisuals();
	} else if (side < -0.05) {
		deltaViewAngles.yaw = -PERCH_ROTATION * perchRotationFactor;
		UpdateVisuals();
	}
}

//=============================================================================
//
// hhTalon::FindEnemy
//
//=============================================================================

bool hhTalon::FindEnemy( void ) {
	idEntity	*ent;
	hhMonsterAI	*actor;
	hhMonsterAI	*bestEnemy;
	float		bestDist;
	float		maxEnemyDist;
	float		dist;
	idVec3		delta;
	int			enemyCount;

	// Check if Talon already has a valid enemy
	if ( enemy.IsValid() && enemy->health > 0 ) {
		return true;
	}

	bestDist = 0;
	maxEnemyDist = 1024.0f * 1024.0f; // sqr
	bestEnemy = NULL;
	enemyCount = 0;
	for ( ent = gameLocal.activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
		if ( ent->fl.hidden || ent->fl.isDormant || !ent->IsType( hhMonsterAI::Type ) ) {
			continue;
		}

		actor = static_cast<hhMonsterAI *>( ent );

		// Only attack living enemies, and only attack enemies that are targetting Tommy or Talon
		if ( (actor->health <= 0) || ( actor->GetEnemy() != GetOwner() && actor->GetEnemy() != this ) ) {
			continue;
		}

		if ( !gameLocal.InPlayerPVS( actor ) ) {
			continue;
		}

		// Check if Talon should avoid this enemy -- these enemies are also not included in the enemy count
		if ( !ent->spawnArgs.GetBool( "bTalonAttack", "0" ) ) {
			continue;
		}

		delta = GetOwner()->GetOrigin() - actor->GetPhysics()->GetOrigin(); // Find the farthest enemy (in the PVS) to Tommy
		dist = delta.LengthSqr();
		if ( dist > bestDist && dist < maxEnemyDist && GetOwner()->CanSee( actor, false ) ) {
			bestDist = dist;
			bestEnemy = actor;
		}

		enemyCount++;
	}
	if ( bestEnemy && enemyCount >= 2 ) { // Only attack if more than one enemy is attacking the player
		enemy = bestEnemy;
		return true;
	}

	enemy.Clear();
	return false;
}

//=============================================================================
//
// hhTalon::EnterPerchState
//
//=============================================================================

void hhTalon::EnterPerchState(void) {
	EndState();
	state = StatePerch;

	velocity = vec3_zero;

	if ( talonTarget ) {
		CalculateTalonTargetLocation(); // Must recalculate the target location, as the target may have moved
		SetOrigin( talonTargetLoc );

		if ( talonTarget->IsBound() ) { // Only bind talon to this target, if it's bound to something
			Bind( talonTarget, true );
		}

		GetPhysics()->SetLinearVelocity( vec3_zero );
	}

	if ( talonTarget && talonTarget->bShouldSquawk ) {
		float frequency = talonTarget->spawnArgs.GetFloat( "squawkFrequency", "5.0" );
		PostEventSec( &EV_PerchSquawk, frequency );
	}

	SetShaderParm( SHADERPARM_DIVERSITY, 1 ); // Talon's model shouldn't phase out if the player is too close

	// Start checking for nearby enemies
	PostEventSec( &EV_CheckForEnemy, spawnArgs.GetFloat("checkEnemyTime", "4.0" ) );

	// Start checking for nearby targets
	PostEventSec( &EV_CheckForTarget, spawnArgs.GetFloat( "checkTargetTime", "1.0" ) );
}

//=============================================================================
//
// hhTalon::PerchTicker
//
//=============================================================================

void hhTalon::PerchTicker(void) {
	float			deltaTime = MS2SEC(gameLocal.msec);

	bLanding = false;

	// Turn towards direction of perch spot
	idVec3 toFace = talonTarget->GetPhysics()->GetAxis()[0];
	toFace.z = 0;
	toFace.Normalize();
	float dp = toFace * GetAxis()[0];
	float side = toFace * GetAxis()[1];

	if ( dp < 0 ) {
		if ( side >= 0 ) {
			deltaViewAngles.yaw = PERCH_ROTATION * perchRotationFactor;		
		} else {
			deltaViewAngles.yaw = -PERCH_ROTATION * perchRotationFactor;		
		}
	} else if (side > 0.05 ) {
		deltaViewAngles.yaw = PERCH_ROTATION * perchRotationFactor;
	} else if (side < -0.05) {
		deltaViewAngles.yaw = -PERCH_ROTATION * perchRotationFactor;
	}

	UpdateVisuals();
}

//=============================================================================
//
// hhTalon::Event_PerchChatter
//
// Small chirping noise which is part of Talon's idle animation
//=============================================================================

void hhTalon::Event_PerchChatter(void) {
	if ( state != StateTommy ) { // Don't chatter when on Tommy's shoulder
		StartSound( "snd_chatter", SND_CHANNEL_VOICE );
	}
}

//=============================================================================
//
// hhTalon::Event_PerchSquawk
//
// Louder, squawking noise, alerting the player to this location
//=============================================================================

void hhTalon::Event_PerchSquawk(void) {
	if ( !talonTarget ) {
		return;
	}

	if ( talonTarget->bShouldSquawk ) { // Always check, because it could have been turned off after Talon perched

		// Check if the player is outside of the squawk distance
		float squawkDistSqr = talonTarget->spawnArgs.GetFloat( "squawkDistance", "0" );
		squawkDistSqr *= squawkDistSqr;

		if ( (GetOrigin() - owner->GetOrigin() ).LengthSqr() > squawkDistSqr ) { // Far enough away, so squawk
			GetAnimator()->ClearAllAnims( gameLocal.time, TALON_BLEND );
			GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, squawkAnim, gameLocal.time, TALON_BLEND);
			PostEventMS( &EV_IdleAnim, GetAnimator()->GetAnim( squawkAnim )->Length() + TALON_BLEND );
			StartSound( "snd_squawk", SND_CHANNEL_VOICE );

			// Glow
			SetShaderParm( SHADERPARM_TIMEOFFSET, MS2SEC( gameLocal.time ) );
		}

		// Post the next squawk attempt
		float frequency = talonTarget->spawnArgs.GetFloat( "squawkFrequency", "5.0" );
		PostEventSec( &EV_PerchSquawk, frequency + frequency * gameLocal.random.RandomFloat() );
	}
}

//=============================================================================
//
// hhTalon::Event_CheckForTarget
//
//=============================================================================

void hhTalon::Event_CheckForTarget() {
	hhTalonTarget *oldTarget = talonTarget;

	// Determine if Talon should take off from his perch
	if ( !bForcedTarget ) { // do not check if Talon is forced this this point
		FindTalonTarget(NULL, NULL, false);

		if( oldTarget != talonTarget ) {
			Event_TakeOffAnim();

			if( oldTarget ) { // Inform the perch spot that talon is leaving
				oldTarget->Left(this);
			}

			EnterFlyState();
			return;
		}
	}

	PostEventSec( &EV_CheckForTarget, spawnArgs.GetFloat( "checkTargetTime", "1.0" ) );
}

//=============================================================================
//
// hhTalon::Event_CheckForEnemy
//
//=============================================================================

void hhTalon::Event_CheckForEnemy() {
	if ( !ai_talonAttack.GetBool() ) {
		return;
	}
	if ( !bForcedTarget ) {
		if ( FindEnemy() ) {
			Event_TakeOffAnim();

			if( talonTarget ) { // Inform the perch spot that talon is leaving
				talonTarget->Left( this );
			}

			EnterAttackState();
			return;
		}
	}

	PostEventSec( &EV_CheckForEnemy, spawnArgs.GetFloat("checkEnemyTime", "4.0" ) );
}

//=============================================================================
//
// hhTalon::EnterAttackState
//
//=============================================================================

void hhTalon::EnterAttackState(void) {
	EndState();
	state = StateAttack;
	checkFlyTime = spawnArgs.GetFloat( "attackTime", "8.0f" );

	StartSound( "snd_attack", SND_CHANNEL_BODY );

	bClawingAtEnemy = false;

	StartAttackFX();
}

//=============================================================================
//
// hhTalon::AttackTicker
//
//=============================================================================

void hhTalon::AttackTicker(void) {
	float			deltaTime = MS2SEC(gameLocal.msec);

	if ( !FindEnemy() ) {
		ReturnToTommy();
		StopAttackFX();
		return;
	}

	idAngles	ang;
	float		distance;
	idVec3		oldVel;
	bool		landing = false;	
	float		distanceXY;
	float		deltaZ;
	float		rotSpeed;
	float		distToEnemy;

	idMat3 junkAxis;
	enemy->GetViewPos(talonTargetLoc, junkAxis);
	talonTargetLoc -= junkAxis[2] * 10.0f;

	distToEnemy = (talonTargetLoc - GetOrigin()).Length();

	acceleration = talonTargetLoc - GetOrigin();

	idVec2 vecXY = acceleration.ToVec2();
	distance = acceleration.Normalize();

	distanceXY = vecXY.Length();
	deltaZ = talonTargetLoc.z - GetOrigin().z;

	float dp = acceleration * GetAxis()[0];
	float side = acceleration * GetAxis()[1];
	ang = GetAxis().ToAngles();

	// Talon can either rotate towards the target, or fly straight.
	if( flyStraightTime > 0 ) { // Continue to fly straight
		flyStraightTime -= deltaTime;

		if ( flyStraightTime <= 0 ) { // Done gliding, play a flap animation
			Event_FlyAnim();

			if ( bClawingAtEnemy ) { // Done attacking
				bClawingAtEnemy = false;
				
				flyStraightTime = 1.0f;
			}
		}

		rotSpeed = 0;
	} else {
		rotSpeed = ROTATION_SPEED * rotationFactor;
	}

	rotSpeed *= (60.0f * USERCMD_ONE_OVER_HZ); // CJR PCF 04/28/06:  Adjust for 30Hz

	// Determine whether Talon should rotate left or right to reach the target
	if(dp > 0) {
		if(dp >= 0.98f || ( side <= 0.1f && side >= 0.1f )) { // CJR PCF 04/28/06:  don't turn if directly facing the target
			rotSpeed = 0;
		} else if (side < -0.1f) {
			rotSpeed *= -1;
		}
	} else {
		if(side < 0) {
			rotSpeed *= -1;
		}
	}

	// Apply rotation
	deltaViewAngles.yaw = rotSpeed;
	float desiredRoll = -side * 20 * rotSpeed;

	oldVel = velocity;

	float defaultVelocityXY = spawnArgs.GetFloat( "velocityXY_attack", "8.5" );
	float defaultVelocityZ = spawnArgs.GetFloat( "velocityZ", "4" );

	// Adjust velocity based upon the attack
	if ( bClawingAtEnemy ) {
		defaultVelocityXY = 0.0f;

		idVec3 tempVec( vecXY.x, vecXY.y, 0.0f );
		viewAxis = tempVec.ToMat3();

	} else if ( flyStraightTime > 0 ) { // Flying away from the enemy
		float adjust = hhMath::ClampFloat( 0.0f, 1.0f, 2.0f - flyStraightTime * 2.0f );
		defaultVelocityXY *= adjust; // Move more slowly when about to attack
	} else { // Flying around, check if the bird should slow down when nearing the enemy
		if ( distToEnemy < 150.0f && dp > 0 ) { // Close to the enemy and facing it
			if( !GetAnimator()->IsAnimPlaying( GetAnimator()->GetAnim( preAttackAnim ) ) ) {
				GetAnimator()->ClearAllAnims( gameLocal.time, TALON_BLEND );
				GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, preAttackAnim, gameLocal.time, TALON_BLEND);
				CancelEvents( &EV_GlideAnim ); // Could possibly have a glide anim queue
				CancelEvents( &EV_TakeOffAnimB );
			}
			float adjust = 1.0f - hhMath::ClampFloat( 0.0f, 0.9f, (100 - (distToEnemy-50) ) / 100.0f); // Decelerate Talon
			defaultVelocityXY *= adjust;
		}
	}

	velocity = viewAxis[0] * defaultVelocityXY * velocityFactor; // xy-velocity

	// Z-change based upon distance
	if(abs(deltaZ) > 0 && distanceXY > 0) {
		velocity.z = (deltaZ * defaultVelocityXY) / distanceXY;

		// Clamp z velocity
		if(velocity.z > defaultVelocityZ) {
			velocity.z = defaultVelocityZ;
		}
	}

	// Apply velocity
	SetOrigin( GetOrigin() + velocity );

	UpdateVisuals();

	// Timeout for flying to perch spot... if too much time has passed, then fly straight for a bit
	checkFlyTime -= deltaTime;
	if( checkFlyTime <= 0 && !bClawingAtEnemy && flyStraightTime <= 0 ) {
		ReturnToTommy();
		StopAttackFX();
	}

	// Check if Talon reached the target and attack!
	if ( flyStraightTime <= 0 && ( distToEnemy < spawnArgs.GetFloat( "distanceAttackEpsilon", "60" ) ) ) {
		// Damage the enemy
		StartSound( "snd_attack", SND_CHANNEL_BODY );

		// Distract the enemy by setting Talon as its enemy -- random chance that this will succeed
		if ( enemy.IsValid() && gameLocal.random.RandomFloat() < spawnArgs.GetFloat( "attackChance", "0.5" ) ) {
			(static_cast<hhMonsterAI*>(enemy.GetEntity()))->Distracted( this );
			owner->TalonAttackComment();			
		}

		// Play an attack anim here
		GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
		GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, attackAnim, gameLocal.time, 0 );

		flyStraightTime = MS2SEC( GetAnimator()->GetAnim( attackAnim )->Length() );
		PostEventSec( &EV_FlyAnim, flyStraightTime ); // Play a fly anim once this attack animation is done

		bClawingAtEnemy = true;
	}
}

//=============================================================================
//
// hhTalon::StartAttackFX
//
//=============================================================================

void hhTalon::StartAttackFX() {
	SetShaderParm( SHADERPARM_MODE, 1 ); // Fire glow attack

	if ( trailFx.IsValid() ) { // An effect already exists, turn it on
		trailFx->Nozzle( true );
	} else { // Spawn a new effect
		const char *defName = spawnArgs.GetString( "fx_attackTrail" );
		if (defName && defName[0]) {
			hhFxInfo fxInfo;

			fxInfo.SetNormal( -GetAxis()[0] );
			fxInfo.SetEntity( this );
			fxInfo.RemoveWhenDone( false );
			trailFx = SpawnFxLocal( defName, GetOrigin(), GetAxis(), &fxInfo, gameLocal.isClient );
			if (trailFx.IsValid()) {
				trailFx->fl.neverDormant = true;
				trailFx->fl.networkSync = false;
				trailFx->fl.clientEvents = true;
			}
		}
	}
}

//=============================================================================
//
// hhTalon::StopAttackFX
//
//=============================================================================

void hhTalon::StopAttackFX() {
	SetShaderParm( SHADERPARM_MODE, 0 ); // Fire glow attack done

	if ( trailFx.IsValid() ) {
		trailFx->Nozzle( false );
	}
}

//=============================================================================
//
// hhTalon::EnterVehicleState
//
//=============================================================================

void hhTalon::EnterVehicleState(void) {
	EndState();
	state = StateVehicle;
	Hide();

	CancelEvents( &EV_GlideAnim ); // Could possibly have a glide anim queue
	CancelEvents( &EV_TakeOffAnimB );
}

//=============================================================================
//
// hhTalon::ExitVehicleState
//
//=============================================================================

void hhTalon::ExitVehicleState(void) {
	if ( talonTarget ) {
		talonTarget->Left( this );
	}
}

//=============================================================================
//
// hhTalon::VehicleTicker
//
// How Talon acts when the player is in a vehicle:
//		- In the interest of speed, Talon warps from target to Target
//		- This allows for the giant translation GUIs to instantly translate as the player gets near
//=============================================================================

void hhTalon::VehicleTicker(void) {
	float		deltaTime = MS2SEC(gameLocal.msec);
	hhTalonTarget *oldEnt = talonTarget;

	if( GetAnimator()->IsAnimPlaying( GetAnimator()->GetAnim( prelandAnim ) ) ) {
		return;
	}

	FindTalonTarget( NULL, NULL, bReturnToTommy );
	
	if ( talonTarget == oldEnt ) { // No change to the target, so do nothing
		return;
	}

	if ( !talonTarget ) { // No Talon Target, so hide Talon
		Hide();
	} else { // have a target, to show the bird
		Show();
	}

	// Have a target, so spawn the bird at the target, if it isn't already there
	CalculateTalonTargetLocation(); // Must recalculate the target location, as the target may have moved
	SetOrigin( talonTargetLoc );	
	SetAxis( talonTargetAxis );
	UpdateVisuals();	

	SetSkin( openWingsSkin );
	Event_LandAnim();

	// Trigger the target
	if ( talonTarget ) {
		talonTarget->Reached( this ); // Reached the spot, but fly through it
	}

	// Inform the last talonTarget that Talon has left
	if ( oldEnt ) {
		oldEnt->Left( this );
	}
}

//=============================================================================
//
// hhTalon::UpdateAnimationControllers
// Talon doesn't use any anim controllers
//=============================================================================

bool hhTalon::UpdateAnimationControllers() {
	idVec3	vel;
	float 	speed;
	float 	roll;

	speed = velocity.Length();
	if ( speed < 3.0f || flyStraightTime > 0 ) {
		roll = 0.0f;
	} else {
		roll = acceleration * viewAxis[1] * -fly_roll_scale / fly_speed;
		if ( roll > fly_roll_max ) {
			roll = fly_roll_max;
		} else if ( roll < -fly_roll_max ) {
			roll = -fly_roll_max;
		}
	}

	fly_roll = fly_roll * 0.95f + roll * 0.05f;

	viewAxis = idAngles( 0, current_yaw, fly_roll ).ToMat3();

	return false;
}

//=============================================================================
//
// hhTalon::ReturnToTommy
//
//=============================================================================

void hhTalon::ReturnToTommy( void ) {
	if ( bReturnToTommy ) { // already returning
		return;
	}

	bReturnToTommy = true;
	bLanding = false;

	FindTalonTarget(NULL, NULL, true);
	
	SetSkin( openWingsSkin );
	GetAnimator()->ClearAllAnims( gameLocal.time, TALON_BLEND );
	GetAnimator()->CycleAnim(ANIMCHANNEL_ALL, flyAnim, gameLocal.time, TALON_BLEND);
	EnterFlyState();
}

// ANIM EVENTS ================================================================

//=============================================================================
//
// hhTalon::Event_LandAnim
//
//=============================================================================

void hhTalon::Event_LandAnim(void) {
	GetAnimator()->ClearAllAnims( gameLocal.time, TALON_BLEND );
	GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, landAnim, gameLocal.time, TALON_BLEND);

	if ( talonTarget ) { // Different animation if landing on Tommy or not
		PostEventMS( &EV_IdleAnim, GetAnimator()->GetAnim( landAnim )->Length() + TALON_BLEND );
	} else { // No target, so the bird must be landing on Tommy
		PostEventMS( &EV_TommyIdleAnim, GetAnimator()->GetAnim( landAnim )->Length() + TALON_BLEND );
	}

	CancelEvents( &EV_GlideAnim ); // Could possibly have a glide anim queue
	CancelEvents( &EV_TakeOffAnimB );
}

//=============================================================================
//
// hhTalon::Event_PreLandAnim
//
//=============================================================================

void hhTalon::Event_PreLandAnim(void) {
	GetAnimator()->ClearAllAnims( gameLocal.time, TALON_BLEND );
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, prelandAnim, gameLocal.time, TALON_BLEND );

	CancelEvents( &EV_GlideAnim ); // Could possibly have a glide anim queue
	CancelEvents( &EV_TakeOffAnimB );
}

//=============================================================================
//
// hhTalon::Event_IdleAnim
//
//=============================================================================

void hhTalon::Event_IdleAnim(void) {
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, idleAnim, gameLocal.time, TALON_BLEND);

	StartSound("snd_idle", SND_CHANNEL_BODY);
}

//=============================================================================
//
// hhTalon::Event_TommyIdleAnim
//
//=============================================================================

void hhTalon::Event_TommyIdleAnim(void) {
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, tommyIdleAnim, gameLocal.time, TALON_BLEND);
}

//=============================================================================
//
// hhTalon::Event_FlyAnim
//
//=============================================================================

void hhTalon::Event_FlyAnim(void) {
	GetAnimator()->ClearAllAnims( gameLocal.time, TALON_BLEND );
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, flyAnim, gameLocal.time, TALON_BLEND);

	StartSound("snd_fly", SND_CHANNEL_BODY);
}

//=============================================================================
//
// hhTalon::Event_GlideAnim
//
//=============================================================================

void hhTalon::Event_GlideAnim(void) {
	GetAnimator()->ClearAllAnims( gameLocal.time, TALON_BLEND );
	GetAnimator()->CycleAnim( ANIMCHANNEL_ALL, glideAnim, gameLocal.time, TALON_BLEND);

	StartSound("snd_glide", SND_CHANNEL_BODY);
}

//=============================================================================
//
// hhTalon::Event_TakeOffAnim
//
//=============================================================================

void hhTalon::Event_TakeOffAnim(void) {
	GetAnimator()->ClearAllAnims( gameLocal.time, 0 ); // No blend when clearing old anims while taking off
	GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, takeOffAnim, gameLocal.time, TALON_BLEND);

	PostEventMS( &EV_TakeOffAnimB, GetAnimator()->GetAnim( takeOffAnim )->Length() + TALON_BLEND );

	StartSound("snd_takeoff", SND_CHANNEL_ANY);
}

//=============================================================================
//
// hhTalon::Event_TakeOffAnimB
//
//=============================================================================

void hhTalon::Event_TakeOffAnimB(void) {
	GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
	GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, takeOffAnimB, gameLocal.time, 0);

	PostEventMS( &EV_GlideAnim, GetAnimator()->GetAnim( takeOffAnimB )->Length() + TALON_BLEND );
}

//=============================================================================
//
// hhTalon::Show
//
// Special show needed to remove any collision information on Talon
//=============================================================================

void hhTalon::Show() {
	if ( state == StateTommy ) {
		return;
	}

	hhMonsterAI::Show();

	// Needed after the show
	GetPhysics()->SetContents( 0 ); // No collisions
	GetPhysics()->SetClipMask( 0 ); // No collisions
}

// TALON PERCH SPOT ===========================================================

//=============================================================================
//
// hhTalonTarget::Spawn
//
//=============================================================================

void hhTalonTarget::Spawn(void) {
	priority = 2.0f - spawnArgs.GetInt("priority");
	if ( priority < 0 ) {
		priority = 0;
	} else if ( priority > 2) {
		priority = 2;
	}

	bNotInSpiritWalk = spawnArgs.GetBool( "notInSpiritWalk" );
	bShouldSquawk = spawnArgs.GetBool( "shouldSquawk" );
	bValidForTalon = spawnArgs.GetBool( "valid", "1" );

	gameLocal.RegisterTalonTarget(this);

	GetPhysics()->SetContents( 0 );

	Hide();	
}

void hhTalonTarget::Save(idSaveGame *savefile) const {
	savefile->WriteInt( priority );
	savefile->WriteBool( bNotInSpiritWalk );
	savefile->WriteBool( bShouldSquawk );
	savefile->WriteBool( bValidForTalon );
}

void hhTalonTarget::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( priority );
	savefile->ReadBool( bNotInSpiritWalk );
	savefile->ReadBool( bShouldSquawk );
	savefile->ReadBool( bValidForTalon );
}

//=============================================================================
//
// hhTalonTarget::ShowTargets
//
//=============================================================================

void hhTalonTarget::ShowTargets( void ) {
	int			i;
	int			count;
	idEntity	*ent;

	count = gameLocal.talonTargets.Num();
	for(i = 0; i < count; i++) {
		ent = (hhTalonTarget *)gameLocal.talonTargets[i];
		ent->Show();
	}
}

//=============================================================================
//
// hhTalonTarget::HideTargets
//
//=============================================================================

void hhTalonTarget::HideTargets( void ) {
	int			i;
	int			count;
	idEntity	*ent;

	count = gameLocal.talonTargets.Num();
	for(i = 0; i < count; i++) {
		ent = (hhTalonTarget *)gameLocal.talonTargets[i];
		ent->Hide();
	}
}

//=============================================================================
//
// hhTalonTarget::~hhTalonTarget
//
//=============================================================================

hhTalonTarget::~hhTalonTarget() {
	gameLocal.talonTargets.Remove( this );
}

//=============================================================================
//
// hhTalonTarget::Reached
//
// Called when Talon reaches the target point
//=============================================================================

void hhTalonTarget::Reached( hhTalon *talon ) {
	if( !talon ) {
		return;
	}

	// Landing on a perch spot sends an action message to the spot's targets
	int num = targets.Num();
	for (int ix=0; ix<num; ix++) {
		if (targets[ix].IsValid()) {
			targets[ix].GetEntity()->PostEventMS(&EV_TalonAction, 0, talon, true);
		}
	}
}

//=============================================================================
//
// hhTalonTarget::Left
//
// Called when Talon leaves the target point
//=============================================================================

void hhTalonTarget::Left( hhTalon *talon ) {
	if( !talon ) {
		return;
	}

	// Leaving on a perch spot sends an action message to the spot's targets
	int num = targets.Num();
	for (int ix=0; ix<num; ix++) {
		if (targets[ix].IsValid()) {
			targets[ix].GetEntity()->PostEventMS(&EV_TalonAction, 0, talon, false);
		}
	}
}

//=============================================================================
//
// hhTalonTarget::Event_CallTalon
//
// Calls Talon to this point and forces him to stay on this point unless released
// or called to another point
//=============================================================================

void hhTalonTarget::Event_CallTalon( float vel, float rot, float perch ) {
	hhPlayer	*player;
	hhTalon		*talon;

	// Find Talon
	player = static_cast<hhPlayer *>( gameLocal.GetLocalPlayer() );
	if ( !player->talon.IsValid() ) {
		return;
	}

	talon = player->talon.GetEntity();

	talon->SetForcedTarget( true );

	talon->SetForcedPhysicsFactors( vel, rot, perch );

	talon->FindTalonTarget( NULL, this, false ); // Force this entity as the target
	talon->Event_TakeOffAnim();

	talon->EnterFlyState();
}

//=============================================================================
//
// hhTalonTarget::Event_ReleaseTalon
//
// Releases Talon from this perch spot and allows him to continue his normal scripting
//=============================================================================

void hhTalonTarget::Event_ReleaseTalon() {
	hhPlayer	*player;
	hhTalon		*talon;

	// Find Talon
	player = static_cast<hhPlayer *>( gameLocal.GetLocalPlayer() );
	if ( !player->talon.IsValid() ) {
		return;
	}

	talon = player->talon.GetEntity();

	talon->SetForcedTarget( false );
	talon->SetForcedPhysicsFactors( 1.0f, 1.0f, 1.0f );
}

//=============================================================================
//
// hhTalonTarget::Event_SetPerchState
//
// Set the state if Talon can or cannot fly towards this target
//=============================================================================

void hhTalonTarget::Event_SetPerchState( float newState ) {
	bValidForTalon = ( newState > 0 ) ? true : false;
}
