#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_ScanForClosestTarget( "<scanForClosestTarget>" );
const idEventDef EV_StartAttack( "<startAttack>" );
const idEventDef EV_NextCycleAnim( "<nextCycleAnim>", "ds" );

CLASS_DECLARATION( hhAnimatedEntity, hhMountedGun )
	EVENT( EV_Activate,								hhMountedGun::Event_Activate )
	EVENT( EV_Deactivate,							hhMountedGun::Event_Deactivate )
	EVENT( EV_PostSpawn,							hhMountedGun::Event_PostSpawn )
	EVENT( EV_ScanForClosestTarget,					hhMountedGun::Event_ScanForClosestTarget )
	EVENT( EV_StartAttack,							hhMountedGun::Event_StartAttack )

	EVENT( EV_NextCycleAnim,						hhMountedGun::Event_NextCycleAnim )
END_CLASS

/*
================
hhMountedGun::hhMountedGun
================
*/
hhMountedGun::hhMountedGun() {
	yawVelocity = 0.0f;
}

/*
================
hhMountedGun::Spawn
================
*/
void hhMountedGun::Spawn( void ) {
	targetingLaser = NULL;
	boneHub.Clear();
	boneGun.Clear();
	boneOrigin.Clear();

	fl.takedamage = false; // Gun doesn't take damage unless it is active

	deathwalkDelay = SEC2MS( spawnArgs.GetInt( "deathwalk_delay", "1" ) );
	burstRateOfFire = SEC2MS( spawnArgs.GetFloat("burstRateOfFire") );
	shotsPerBurst = spawnArgs.GetInt("shotsPerBurst");
	shotsPerBurstCounter = 0;

	firingRange = spawnArgs.GetFloat( "firingRange" );
	nextFireTime = 0;
	projectileDict = gameLocal.FindEntityDefDict( spawnArgs.GetString( "def_projectile" ) );
	if ( !projectileDict ) {
		gameLocal.Error( "Unknown def_projectile:  %s\n", spawnArgs.GetString( "def_projectile" ) );
	}

	// Set up muzzle flash light
	memset( &muzzleFlash, 0, sizeof(renderLight_t) );
	muzzleFlash.lightId = 500 + entityNumber;
	muzzleFlash.shader								= declManager->FindMaterial( spawnArgs.GetString("mtr_flashShader"), false );
	muzzleFlashHandle								= -1;
	muzzleFlash.pointLight							= true;
	muzzleFlashEnd									= gameLocal.GetTime();
	muzzleFlash.lightRadius							= spawnArgs.GetVector( "flashSize" );
	muzzleFlashDuration								= SEC2MS( spawnArgs.GetFloat("flashTime") );
	idVec3 flashColor								= spawnArgs.GetVector( "flashColor" );
	muzzleFlash.shaderParms[ SHADERPARM_RED ]		= flashColor[0];
	muzzleFlash.shaderParms[ SHADERPARM_GREEN ]		= flashColor[1];
	muzzleFlash.shaderParms[ SHADERPARM_BLUE ]		= flashColor[2];
	muzzleFlash.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;

	SpawnParts();

	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetContents( CONTENTS_SOLID );
	physicsObj.SetClipMask( GetPhysics()->GetContents() );
	physicsObj.SetOrigin( GetOrigin() );
	physicsObj.SetAxis( GetAxis() );
	SetPhysics( &physicsObj );

	prevIdealLookAngle.Zero();

	pitchController.Clear();
	pitchController.Setup( this, spawnArgs.GetString("bone_Hub"), idAngles(-90, 0, 0), idAngles(90, 0, 0), idAngles(90, 0, 0), idAngles(1,0,0) );

	channelBody	= ChannelName2Num( "body" );
	channelBarrel = ChannelName2Num( "barrel" );

	PlayCycle( channelBody, "idle", 0 );

	PVSArea = gameLocal.pvs.GetPVSArea( GetOrigin() );

	PostEventMS( &EV_PostSpawn, 0 );

	gunMode = GM_DORMANT;
	team = spawnArgs.GetInt( "team", "1" );

	PlayAnim( ANIMCHANNEL_ALL, "idle_close", 0 );
}

void hhMountedGun::Save(idSaveGame *savefile) const {
	savefile->WriteInt( team );
	savefile->WriteInt( gunMode );
	pitchController.Save( savefile );
	trackMover.Save( savefile );
	targetingLaser.Save( savefile );
	boneHub.Save( savefile );
	boneGun.Save( savefile );
	boneExhaust.Save( savefile );
	boneOrigin.Save( savefile );
	enemy.Save( savefile );
	savefile->WriteFloat( enemyRange );
	savefile->WriteFloat( firingRange );
	savefile->WriteInt( nextFireTime );
	savefile->WriteFloat( burstRateOfFire );
	savefile->WriteInt( shotsPerBurst );
	savefile->WriteInt( shotsPerBurstCounter );
	savefile->WriteRenderLight( muzzleFlash );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
	//savefile->WriteInt( muzzleFlashHandle );
	savefile->WriteInt( muzzleFlashEnd );
	savefile->WriteInt( muzzleFlashDuration );
	savefile->WriteAngles( idealLookAngle );
	savefile->WriteAngles( prevIdealLookAngle );
	savefile->WriteStaticObject( physicsObj );
	savefile->WriteInt( animDoneTime );
	savefile->WriteInt( PVSArea );
	savefile->WriteInt( channelBody );
	savefile->WriteInt( channelBarrel );
	savefile->WriteFloat( yawVelocity );
	savefile->WriteInt( deathwalkDelay );
}

void hhMountedGun::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( team );
	savefile->ReadInt( reinterpret_cast<int &> ( gunMode ) );
	pitchController.Restore( savefile );
	trackMover.Restore( savefile );
	targetingLaser.Restore( savefile );
	boneHub.Restore( savefile );
	boneGun.Restore( savefile );
	boneExhaust.Restore( savefile );
	boneOrigin.Restore( savefile );
	enemy.Restore( savefile );
	savefile->ReadFloat( enemyRange );
	savefile->ReadFloat( firingRange );
	savefile->ReadInt( nextFireTime );
	savefile->ReadFloat( burstRateOfFire );
	savefile->ReadInt( shotsPerBurst );
	savefile->ReadInt( shotsPerBurstCounter );
	savefile->ReadRenderLight( muzzleFlash );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
	//savefile->ReadInt( muzzleFlashHandle );
	savefile->ReadInt( muzzleFlashEnd );
	savefile->ReadInt( muzzleFlashDuration );
	savefile->ReadAngles( idealLookAngle );
	savefile->ReadAngles( prevIdealLookAngle );
	savefile->ReadStaticObject( physicsObj );
	savefile->ReadInt( animDoneTime );
	savefile->ReadInt( PVSArea );
	savefile->ReadInt( channelBody );
	savefile->ReadInt( channelBarrel );
	savefile->ReadFloat( yawVelocity );
	savefile->ReadInt( deathwalkDelay );

	projectileDict = gameLocal.FindEntityDefDict( spawnArgs.GetString( "def_projectile" ) );
	if ( !projectileDict ) {
		gameLocal.Error( "Unknown def_projectile:  %s\n", spawnArgs.GetString( "def_projectile" ) );
	}
	RestorePhysics( &physicsObj );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
	muzzleFlashHandle = -1;
}

/*
================
hhMountedGun::~hhMountedGun
================
*/
hhMountedGun::~hhMountedGun( void ) {
	SAFE_REMOVE( targetingLaser );

	SAFE_FREELIGHT( muzzleFlashHandle );

	trackMover = NULL;
}

/*
================
hhMountedGun::SpawnParts
================
*/
void hhMountedGun::SpawnParts() {
	boneHub.name = spawnArgs.GetString("bone_Hub");
	boneHub.handle = GetAnimator()->GetJointHandle( boneHub.name.c_str() );

	boneGun.name = spawnArgs.GetString("bone_Gun");
	boneGun.handle = GetAnimator()->GetJointHandle( boneGun.name.c_str() );
	
	boneOrigin.name = spawnArgs.GetString("bone_Origin");
	boneOrigin.handle = GetAnimator()->GetJointHandle( boneOrigin.name.c_str() );

	UpdateBoneInfo();

	targetingLaser = hhBeamSystem::SpawnBeam( boneGun.origin, spawnArgs.GetString("beam") );
	if( targetingLaser.IsValid() ) {
		targetingLaser->MoveToJoint( this, boneGun.name.c_str() );
		targetingLaser->BindToJoint( this, boneGun.name.c_str(), false );
		targetingLaser->Activate( false );
	}
}

/*
=====================
hhMountedGun::Hide
=====================
*/
void hhMountedGun::Hide() {
	hhAnimatedEntity::Hide();

	if( targetingLaser.IsValid() ) {
		targetingLaser->Hide();
	}
}

/*
=====================
hhMountedGun::Show
=====================
*/
void hhMountedGun::Show() {
	hhAnimatedEntity::Show();

	if( targetingLaser.IsValid() ) {
		targetingLaser->Show();
	}
}

/*
=====================
hhMountedGun::DetermineTargetingLaserEndPoint
=====================
*/
idVec3 hhMountedGun::DetermineTargetingLaserEndPoint() {
	trace_t traceInfo;
	idVec3	start = boneGun.origin;
	idVec3	dist = boneGun.axis[0] * CM_MAX_TRACE_DIST;

	gameLocal.clip.TracePoint( traceInfo, start, start + dist, MASK_SHOT_BOUNDINGBOX, this );
	
	return (enemy.IsValid()) ? traceInfo.endpos + enemy->GetAxis()[2] * -10.0f : traceInfo.endpos;
}

/*
=====================
hhMountedGun::Ticker
=====================
*/
void hhMountedGun::Ticker( void ) {
	if( !pitchController.IsFinishedMoving(PITCH) ) {
		pitchController.Update( gameLocal.GetTime() );
	}

	UpdateBoneInfo();

	if( targetingLaser.IsValid() && !targetingLaser->IsHidden() ) {
		targetingLaser->SetTargetLocation( DetermineTargetingLaserEndPoint() );
	}

	UpdateMuzzleFlash();
}

/*
================
hhMountedGun::UpdateOrientation
================
*/
void hhMountedGun::UpdateOrientation() {
	SetAngles( idAngles( 0, idealLookAngle.yaw, 0 ) );
	pitchController.TurnTo( idealLookAngle );
}

/*
================
hhMountedGun::UpdateBoneInfo
================
*/
void hhMountedGun::UpdateBoneInfo() {
	idVec3 TempVec;

	GetJointWorldTransform( boneHub.handle, boneHub.origin, boneHub.axis );
	GetJointWorldTransform( boneGun.handle, boneGun.origin, boneGun.axis );

	GetJointWorldTransform( boneOrigin.handle, boneOrigin.origin, boneOrigin.axis );
}

/*
================
hhMountedGun::SetIdealLookAngle
================
*/
void hhMountedGun::SetIdealLookAngle( const idAngles& LookAngle ) {
	idealLookAngle.pitch = -hhMath::AngleNormalize180( LookAngle.pitch );
	idealLookAngle.yaw = hhMath::AngleNormalize360( LookAngle.yaw );
	idealLookAngle.roll = 0.0f;

	if( prevIdealLookAngle == idealLookAngle ) {
		return;
	}

	prevIdealLookAngle = idealLookAngle;

	UpdateOrientation();
}

/*
================
hhMountedGun::AttemptToRemoveMuzzleFlash
================
*/
void hhMountedGun::AttemptToRemoveMuzzleFlash() {
	if( gameLocal.GetTime() >= muzzleFlashEnd ) {
		SAFE_FREELIGHT( muzzleFlashHandle );
	}
}

/*
================
hhMountedGun::UpdateMuzzleFlash
================
*/
void hhMountedGun::UpdateMuzzleFlash() {
	AttemptToRemoveMuzzleFlash();

	if( muzzleFlashHandle != -1 ) {
		UpdateMuzzleFlashPosition();
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
	}
}

/*
================
hhMountedGun::UpdateMuzzleFlashPosition
================
*/
void hhMountedGun::UpdateMuzzleFlashPosition() {	
	muzzleFlash.axis = boneGun.axis;
	muzzleFlash.origin = boneGun.origin;

	//TEST
	trace_t trace;
	idVec3 flashSize = spawnArgs.GetVector( "flashSize" );
	if( gameLocal.clip.TracePoint(trace, muzzleFlash.origin, muzzleFlash.origin + muzzleFlash.axis[0] * flashSize[0], MASK_SHOT_BOUNDINGBOX, this) ) {
		flashSize[0] *= trace.fraction;
	}
	muzzleFlash.lightRadius = flashSize;
	muzzleFlash.origin += muzzleFlash.axis[0] * flashSize[0] * 0.5f;
	muzzleFlash.lightCenter.Set( flashSize[0] * -0.45f, 0.0f, 0.0f );

	hhUtils::Swap<float>( muzzleFlash.lightRadius[0], muzzleFlash.lightRadius[2] );
	hhUtils::Swap<float>( muzzleFlash.lightCenter[0], muzzleFlash.lightCenter[2] );
	muzzleFlash.axis = hhUtils::SwapXZ( muzzleFlash.axis );
	//TEST
}

/*
================
hhMountedGun::MuzzleFlash
================
*/
void hhMountedGun::MuzzleFlash() {
	if( !muzzleFlash.lightRadius[0] ) {
		return;
	}

	UpdateMuzzleFlashPosition();

	// these will be different each fire
	muzzleFlash.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.GetTime() );
	muzzleFlash.shaderParms[ SHADERPARM_DIVERSITY ]	= gameLocal.random.RandomFloat();

	// the light will be removed at this time
	muzzleFlashEnd = gameLocal.GetTime() + muzzleFlashDuration;

	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
	} else {
		muzzleFlashHandle = gameRenderWorld->AddLightDef( &muzzleFlash );
	}
}

/*
================
hhMountedGun::TraceTargetVerified
================
*/
bool hhMountedGun::TraceTargetVerified( const idActor* target, int traceEntNum ) const {
	int entNum = target->InVehicle() ? target->GetVehicleInterface()->GetVehicle()->entityNumber : target->entityNumber;
	return entNum == traceEntNum;
}

/*
================
hhMountedGun::ScanForClosestTarget
================
*/
idActor *hhMountedGun::ScanForClosestTarget() {
	trace_t			TraceInfo;
	idEntity		*entity;
	idActor			*actor;
	hhPlayer		*player;
	float			firingRangeSquared = firingRange * firingRange;
	float			closestDistSqr = firingRangeSquared;
	idActor			*closestActor = NULL;
	pvsHandle_t		pvs;

	pvs = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );

	for ( entity = gameLocal.activeEntities.Next(); entity != NULL; entity = entity->activeNode.Next() ) {
		if ( entity->fl.hidden || entity->fl.isDormant || !entity->IsType( idActor::Type ) ) {
			continue;
		}

		actor = static_cast<idActor *>( entity );

		// Check if this actor is in the PVS (needed?)
		if ( !gameLocal.pvs.InCurrentPVS( pvs, actor->GetPVSAreas(), actor->GetNumPVSAreas() ) ) {
			continue;
		}

		// Check if the actor can be damaged and should be targeted by the gun
		// Also, check the actor's team, only player team (humans, player) should be targetted
		if ( !actor->fl.takedamage || actor->GetHealth() <= 0 || actor->team != 0 ) {
			if ( !actor->InVehicle() ) { //HUMANHEAD jsh target vehicles
				continue;
			}
		}

		// Player-specific checks -- ignore the spirit player
		if ( actor->IsType( hhPlayer::Type ) ) {
			player = static_cast<hhPlayer *>( actor );
			if ( !player || player->IsSpiritOrDeathwalking() ) {
				continue;
			}
		}

		// Calculate the distance to the actor
		float distSqr = (actor->GetEyePosition() - GetOrigin()).LengthSqr();

		// Ignore actors beyond the max distance
		if( distSqr > firingRangeSquared ) {
			continue;
		}

		// Only check if this distance is closer than the others
		if( distSqr > closestDistSqr ) {
			continue;
		}

		// Check if this actor is visible from the gun
		if( !gameLocal.clip.TracePoint(TraceInfo, GetOrigin(), actor->GetEyePosition(), MASK_SHOT_BOUNDINGBOX, this) ) {
			continue;
		}

		if( !TraceTargetVerified(actor, TraceInfo.c.entityNum) ) {
			continue;
		}

		if ( actor->IsType( hhPlayer::Type ) ) {
			hhPlayer *player = static_cast<hhPlayer*>(actor);
			if ( player && player->GetLastResurrectTime() ) {
				if ( gameLocal.time < player->GetLastResurrectTime() + deathwalkDelay ) {
					continue;
				}
			}
		}

		closestActor = actor;
		closestDistSqr = distSqr;
	}

	gameLocal.pvs.FreeCurrentPVS( pvs );

	return closestActor;
}

/*
================
hhMountedGun::PlayAnim
================
*/
void hhMountedGun::PlayAnim( int iChannelNum, const char* pAnim, int iBlendTime ) {
	PlayAnim( iChannelNum, GetAnimator()->GetAnim( pAnim ), iBlendTime );
}

/*
================
hhMountedGun::PlayAnim
================
*/
void hhMountedGun::PlayAnim( int iChannelNum, int pAnim, int iBlendTime ) {
	ClearAnims( iChannelNum, iBlendTime );

	GetAnimator()->PlayAnim( iChannelNum, pAnim, gameLocal.GetTime(), iBlendTime );
	animDoneTime = GetAnimator()->CurrentAnim( iChannelNum )->Length();
	animDoneTime = (animDoneTime > iBlendTime) ? animDoneTime - iBlendTime : animDoneTime;

	animDoneTime += gameLocal.GetTime();
}

/*
================
hhMountedGun::PlayCycle
================
*/
void hhMountedGun::PlayCycle( int iChannelNum, const char* pAnim, int iBlendTime ) {
	PlayCycle( iChannelNum, GetAnimator()->GetAnim( pAnim ), iBlendTime );
}

/*
================
hhMountedGun::NextCycleAnim

// Cycles the next animation in after the current one completes
================
*/
void hhMountedGun::NextCycleAnim( int iChannelNum, const char* pAnim ) {
	PostEventMS( &EV_NextCycleAnim, GetAnimDoneTime() - gameLocal.GetTime(), iChannelNum, pAnim );
}

void hhMountedGun::Event_NextCycleAnim( int iChannelNum, const char* pAnim ) {
	PlayCycle( iChannelNum, pAnim, 0 );
}

/*
================
hhMountedGun::PlayCycle
================
*/
void hhMountedGun::PlayCycle( int iChannelNum, int pAnim, int iBlendTime ) {
	ClearAnims( iChannelNum, iBlendTime );

	GetAnimator()->CycleAnim( iChannelNum, pAnim, gameLocal.GetTime(), iBlendTime ); 
}

/*
================
hhMountedGun::ClearAnims
================
*/
void hhMountedGun::ClearAnims( int iChannelNum, int iBlendTime ) {
	GetAnimator()->Clear( iChannelNum, gameLocal.GetTime(), iBlendTime );

	animDoneTime = 0;
}

/*
=====================
hhMountedGun::Event_PostSpawn
=====================
*/
void hhMountedGun::Event_PostSpawn() {
	// Find the trackmover if it exists
	trackMover = gameLocal.FindEntity( spawnArgs.GetString("target") );
	if( trackMover.IsValid() && trackMover->IsType( hhTrackMover::Type ) ) { //HUMANHEAD PCF mdl 04/26/06 - Only bind to track movers, in case target is only for triggers
		SetOrigin( trackMover->GetOrigin() );		
		Bind( trackMover.GetEntity(), false );
	}

	SetIdealLookAngle( GetAxis().ToAngles() );
}


/*
=====================
hhMountedGun::Event_Activate
=====================
*/
void hhMountedGun::Event_Activate( idEntity* pActivator ) {
	if ( gunMode == GM_DORMANT ) {
		gunMode = GM_IDLE;

		BecomeActive( TH_THINK | TH_TICKER );
		
		ProcessEvent( &EV_ScanForClosestTarget );
	}
}

/*
=====================
hhMountedGun::Awaken
=====================
*/

void hhMountedGun::Awaken() {
	if ( gunMode != GM_DEAD ) {
		if( targetingLaser.IsValid() && !targetingLaser->IsActivated() ) {
			targetingLaser->Activate( true );
		}

		PreAttack();
		fl.takedamage = true; // Gun can now be shot

		PlayAnim( ANIMCHANNEL_ALL, "open", 0 );
		NextCycleAnim( ANIMCHANNEL_ALL, "idle_open" );
	}
}

/*
=====================
hhMountedGun::Event_Deactivate
=====================
*/
void hhMountedGun::Event_Deactivate() {
	if (gunMode != GM_DEAD) {
		if( targetingLaser.IsValid() && targetingLaser->IsActivated() ) {
			targetingLaser->Activate( false );
		}

		if( trackMover.IsValid() ) {
			trackMover->ProcessEvent( &EV_Deactivate );
		}

		CancelEvents( &EV_ScanForClosestTarget );
		CancelEvents( &EV_StartAttack );

		gunMode = GM_DORMANT;
		fl.takedamage = false;

		StopSound( SND_CHANNEL_ANY, false );

		PlayAnim( ANIMCHANNEL_ALL, "close", 0 );
		NextCycleAnim( ANIMCHANNEL_ALL, "idle_close" );
	}
}

/*
=====================
hhMountedGun::Sleep

// Very similar to dormant, except the gun continues to think and look for enemies while closed
=====================
*/
void hhMountedGun::Sleep() {
	if( targetingLaser.IsValid() && targetingLaser->IsActivated() ) {
		targetingLaser->Activate( false );
		targetingLaser->SetShaderParm( SHADERPARM_MODE, 0 );
	}

	// Note:  A sleeping gun doesn't disable the track mover
	CancelEvents( &EV_StartAttack );

	gunMode = GM_IDLE;
	fl.takedamage = false;

	StopSound( SND_CHANNEL_ANY, false );

	PlayAnim( ANIMCHANNEL_ALL, "close", 0 );
	NextCycleAnim( ANIMCHANNEL_ALL, "idle_close" );
}

//===============================================================================================================
//===============================================================================================================
//===============================================================================================================
//===============================================================================================================
//===============================================================================================================

void hhMountedGun::Think() {
	hhAnimatedEntity::Think();

	if (thinkFlags & TH_THINK) {
		// Run  the current state
		switch ( gunMode ) {
		case GM_DORMANT:
		case GM_IDLE:
		case GM_DEAD:
			break;
		case GM_PREATTACKING:
			TurnTowardsEnemy( DEG2RAD( spawnArgs.GetFloat( "turnMax", "3" ) ), DEG2RAD( spawnArgs.GetFloat( "turnAccel", "0.2" ) ) );
			break;
		case GM_ATTACKING:
			TurnTowardsEnemy( DEG2RAD( spawnArgs.GetFloat( "attackTurnMax", "0.5" ) ), DEG2RAD( spawnArgs.GetFloat( "attackTurnAccel", "0.05" ) ) );
			Attack();
			break;
		case GM_RELOADING:
			// TODO:  Handled by an event
			if ( ReadyToFire() ) { // hack!
				PreAttack();
			}
			break;
		}
	}
}

bool hhMountedGun::GetFacePosAngle( const idVec3 &sourceOrigin, float angle1, const idVec3 &targetOrigin, float &delta ) {
	float	diff;
	float	angle2;

	angle2 = hhUtils::PointToAngle( targetOrigin.x - sourceOrigin.x, targetOrigin.y - sourceOrigin.y );
	if(angle2 > angle1) {
		diff = angle2 - angle1;

		if( diff > DEG2RAD(180.0f) ) {
			delta = DEG2RAD(359.9f) - diff;
			return false;
		}
		else {
			delta = diff;
			return true;
		}
	}
	else {
		diff = angle1 - angle2;
		if( diff > DEG2RAD(180.0f) ) {
			delta = DEG2RAD(359.9f) - diff;
			return true;
		}
		else {
			delta = diff;
			return false;
		}
	}
}

void hhMountedGun::TurnTowardsEnemy( float maxYawVelocity, float yawAccel ) {
	idVec3 dirToEnemy;
	idVec3 localDirToEnemy;

	bool dir;
	float deltaYaw;

	if( !EnemyIsVisible() ) { // Enemy isn't valid or visible, so go to sleep until the enemy is available again
		return;
	}

	if ( !enemy->IsType( hhPlayer::Type ) ) { // Turn twice as fast if the enemy is not a player (to guarantee that the gun will kill other humans/creatures)
		maxYawVelocity *= 2.0f;
		yawAccel *= 2.0f;
	}

	idVec3 currentVec = this->GetAxis()[0];
	float currentYaw = DEG2RAD( currentVec.ToYaw() );

	// Face toward the enemy
	dir = GetFacePosAngle( GetOrigin(), currentYaw, enemy->GetOrigin(), deltaYaw );

	// Acceleration
	if ( dir ) {
		yawVelocity += yawAccel;
	} else {
		yawVelocity -= yawAccel;
	}

	if ( yawVelocity > maxYawVelocity ) {
		yawVelocity = maxYawVelocity;
	} else if ( yawVelocity < -maxYawVelocity ) {
		yawVelocity = -maxYawVelocity;
	}

	deltaYaw = yawVelocity;

	// Compute the pitch to the enemy
	dirToEnemy = ( ( enemy->GetEyePosition() + enemy->GetOrigin() ) * 0.5f ) - boneGun.origin;
	dirToEnemy.Normalize();
	GetAxis().ProjectVector( dirToEnemy, localDirToEnemy );

	SetIdealLookAngle( idAngles( localDirToEnemy.ToPitch(), RAD2DEG( currentYaw + deltaYaw ), 0.0f ) );

	StartSound( "snd_rotate", SND_CHANNEL_BODY3 );
}

bool hhMountedGun::EnemyIsInRange() {
	idVec3 dirToEnemy;

	if ( !enemy.IsValid() ) {
		return false;
	}

	dirToEnemy = enemy->GetEyePosition() - boneHub.origin;
	if ( dirToEnemy.LengthSqr() > firingRange * firingRange ) {
		return false;
	}

	return true;
}

bool hhMountedGun::EnemyCanBeAttacked() {
	if ( !enemy.IsValid() ) { // No enemy
		return false; 
	}

	// Check if the enemy is visible and is alive
	if ( ClearLineOfFire() && enemy->health > 0 ) {
		return true;
	}

	return false;
}

/*
=====================
hhMountedGun::Event_ScanForClosestTarget
=====================
*/
void hhMountedGun::Event_ScanForClosestTarget() {
	idActor	*entity = ScanForClosestTarget();

	enemy = entity;
	if ( entity && gunMode == GM_IDLE ) { // Sleeping and found a target
		Awaken();
	}
	else if ( !entity && gunMode != GM_IDLE ) { // No target, so go to sleep if not already sleeping
		Sleep();
	}

	PostEventSec( &EV_ScanForClosestTarget, spawnArgs.GetFloat("scanPeriod") );
}

void hhMountedGun::PreAttack() {
	gunMode = GM_PREATTACKING;

	// Change the beam to something more threatening
	targetingLaser->SetShaderParm( SHADERPARM_MODE, 1 );
	
	// Play a preattack warning sound
	StartSound( "snd_preattack", SND_CHANNEL_ANY );

	// Delay before attacking
	PostEventSec( &EV_StartAttack, spawnArgs.GetFloat( "attackDelay", "0.2" ) ); // Delay a bit before attacking
}

void hhMountedGun::Event_StartAttack() {
	gunMode = GM_ATTACKING;
}

void hhMountedGun::Attack() {
	hhPlayer *player = NULL;

	if ( !enemy.IsValid() ) {
		return;
	}

	if ( enemy->IsType( hhPlayer::Type ) ) {
		player = static_cast<hhPlayer*>( enemy.GetEntity() );
		if ( player->IsSpiritOrDeathwalking() ) {
			return;
		}
	}

	// Actually fire the gun
	if ( ReadyToFire() ) {

		Fire();

		shotsPerBurstCounter++;
		if ( shotsPerBurstCounter >= shotsPerBurst || enemy->GetHealth() <= 0 ) {
			Reload();
		}
	}
}

void hhMountedGun::Reload() {
	gunMode = GM_RELOADING;
	PlayAnim( ANIMCHANNEL_ALL, "reload", 0 );
	
	nextFireTime = gameLocal.GetTime() + SEC2MS( 2.0f ); // HACK:  fake way of currently forcing a wait when reloading
	shotsPerBurstCounter = 0;

	targetingLaser->SetShaderParm( SHADERPARM_MODE, 0 );

	if ( enemy->GetHealth() <= 0 ) { // Enemy was killed, so clear it and wait for the gun to find a new enemy
		enemy.Clear();
	}

	StopSound( SND_CHANNEL_BODY3 ); // Stop the rotation sound
}

/*
=====================
hhMountedGun::DetermineNextFireTime
=====================
*/
int hhMountedGun::DetermineNextFireTime() {
	return gameLocal.GetTime() + burstRateOfFire;
}

/*
=====================
hhMountedGun::ReadyToFire
=====================
*/
bool hhMountedGun::ReadyToFire() {
	return gameLocal.GetTime() > nextFireTime;
}

/*
================
hhMountedGun::Fire
================
*/
void hhMountedGun::Fire() {
	idVec3	LaunchDir;
	idMat3	LaunchAxis;

	PlayAnim( channelBarrel, "fireA", 0 );
	nextFireTime = DetermineNextFireTime();
	
	MuzzleFlash();

	hhProjectile* pProjectile = hhProjectile::SpawnProjectile( projectileDict );
	if( !pProjectile ) {
		return;
	}

	LaunchAxis = hhUtils::RandomSpreadDir( boneGun.axis, DEG2RAD(spawnArgs.GetFloat("spread"))).ToMat3();
	pProjectile->Create( this, boneGun.origin, LaunchAxis );
	pProjectile->Launch( boneGun.origin, LaunchAxis, vec3_zero );
}

/*
=====================
hhMountedGun::EnemyIsVisible

// If the enemy is visible at any angle from the gun
=====================
*/
bool hhMountedGun::EnemyIsVisible() {
	trace_t		traceInfo;

	if( !enemy.IsValid() || !EnemyIsInPVS() ) {
		return false;
	}

	if( !gameLocal.clip.TracePoint( traceInfo, GetOrigin(), enemy->GetEyePosition(), MASK_SHOT_BOUNDINGBOX, this ) ) {
		return true;
	}

	if( TraceTargetVerified( enemy.GetEntity(), traceInfo.c.entityNum ) ) {
		return true;
	}
	
	return false;
}

/*
=====================
hhMountedGun::ClearLineOfFire

// If the enemy can be shot (line of sight from the gun barrel to the enemy)
=====================
*/
bool hhMountedGun::ClearLineOfFire() {
	trace_t		TraceInfo;
	idVec3		BonePos;
	idMat3		BoneAxis;

	if( !enemy.IsValid() || !EnemyIsInPVS() ) {
		return false;
	}

	if( !gameLocal.clip.TracePoint(TraceInfo, boneGun.origin, boneGun.origin + boneGun.axis[0] * firingRange, MASK_SHOT_BOUNDINGBOX, this) ) {
		return false;
	}

	if( !TraceTargetVerified(enemy.GetEntity(), TraceInfo.c.entityNum) ) {
		return false;
	}

	return true;
}

/*
=====================
hhMountedGun::EnemyIsInPVS
=====================
*/
bool hhMountedGun::EnemyIsInPVS() {
	pvsHandle_t PVSHandle = gameLocal.pvs.SetupCurrentPVS( PVSArea );

	bool bResult = gameLocal.pvs.InCurrentPVS( PVSHandle, enemy->GetPVSAreas(), enemy->GetNumPVSAreas() );
	
	gameLocal.pvs.FreeCurrentPVS( PVSHandle );

	return bResult;
}

/*
================
hhMountedGun::Killed
================
*/
void hhMountedGun::Killed( idEntity *pInflictor, idEntity *pAttacker, int iDamage, const idVec3 &Dir, int iLocation ) {
	hhFxInfo fxInfo;

	if ( gunMode != GM_DEAD ) {
		BecomeInactive( TH_THINK|TH_TICKER );
		CancelEvents( &EV_ScanForClosestTarget );
		CancelEvents( &EV_StartAttack );
		CancelEvents( &EV_NextCycleAnim );

		fxInfo.SetNormal( -GetAxis()[2] );
		fxInfo.RemoveWhenDone( true );
		BroadcastFxInfo( spawnArgs.GetString("fx_detonate"), boneHub.origin, GetAxis(), &fxInfo );

		hhUtils::SpawnDebrisMass(spawnArgs.GetString("def_debrisspawner"), this);

		if( targetingLaser.IsValid() ) {
			targetingLaser->Activate( false );
		}
		SAFE_REMOVE( targetingLaser );
		SAFE_FREELIGHT( muzzleFlashHandle );

		if( trackMover.IsValid() ) {
			trackMover->ProcessEvent( &EV_Deactivate );
		}

		const char *killedModel = spawnArgs.GetString("model_killed", NULL);
		if (killedModel) {
			SetModel(killedModel);
			UpdateVisuals();
			GetPhysics()->SetContents(0);
			fl.takedamage = false;
		}

		RemoveBinds();

		StopSound( SND_CHANNEL_ANY, false );

		StartSound( "snd_die", SND_CHANNEL_ANY);

		ActivateTargets( pAttacker );

		gunMode = GM_DEAD;
	}
}

void hhMountedGun::jointInfo_t::Save( idSaveGame *savefile ) const {
	savefile->WriteString( name );
	savefile->WriteJoint( handle );
	savefile->WriteVec3( origin );
	savefile->WriteMat3( axis );
}

void hhMountedGun::jointInfo_t::Restore( idRestoreGame *savefile ) {
	savefile->ReadString( name );
	savefile->ReadJoint( handle );
	savefile->ReadVec3( origin );
	savefile->ReadMat3( axis );
}

