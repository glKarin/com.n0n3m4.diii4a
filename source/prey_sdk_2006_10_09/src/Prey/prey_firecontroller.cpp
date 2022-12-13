
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

ABSTRACT_DECLARATION( idClass, hhFireController )
END_CLASS

/*
================
hhFireController::hhFireController
================
*/
hhFireController::hhFireController() : muzzleFlashHandle(-1) {
	// Register us so we can be saved -mdl   
	gameLocal.RegisterUniqueObject( this );
}

/*
================
hhFireController::~hhFireController
================
*/
hhFireController::~hhFireController() {
	gameLocal.UnregisterUniqueObject( this );
	SAFE_FREELIGHT( muzzleFlashHandle );
	Clear();
}

/*
================
hhFireController::Init
================
*/
void hhFireController::Init( const idDict* viewDict ) {
	const char *shader = NULL;

	Clear();

	dict = viewDict;

	if( !dict ) {
		return;
	}

	ammoRequired		= dict->GetInt( "ammoRequired" );

	// set up muzzleflash render light
	const idMaterial*flashShader;
	idVec3			flashColor;
	idVec3			flashTarget;
	idVec3			flashUp;
	idVec3			flashRight;
	//HUMANHEAD: aob - changed from float to idVec3
	idVec3			flashRadius;
	//HUMANHEAD END
	bool			flashPointLight;

	// get the projectile
	SetProjectileDict( dict->GetString("def_projectile") );

	dict->GetString( "mtr_flashShader", "muzzleflash", &shader );
	flashShader = declManager->FindMaterial( shader, false );
	flashPointLight = dict->GetBool( "flashPointLight", "1" );
	dict->GetVector( "flashColor", "0 0 0", flashColor );
	flashTime		= SEC2MS( dict->GetFloat( "flashTime", "0.08" ) );
	flashTarget		= dict->GetVector( "flashTarget" );
	flashUp			= dict->GetVector( "flashUp" );
	flashRight		= dict->GetVector( "flashRight" );

	memset( &muzzleFlash, 0, sizeof( muzzleFlash ) );

	muzzleFlash.pointLight								= flashPointLight;
	muzzleFlash.shader									= flashShader;
	muzzleFlash.shaderParms[ SHADERPARM_RED ]			= flashColor[0];
	muzzleFlash.shaderParms[ SHADERPARM_GREEN ]			= flashColor[1];
	muzzleFlash.shaderParms[ SHADERPARM_BLUE ]			= flashColor[2];
	muzzleFlash.shaderParms[ SHADERPARM_TIMESCALE ]		= 1.0f;

	//HUMANHEAD: aob
	if( dict->GetFloat("flashRadius", "0", flashRadius[0]) ) {	// if 0, no light will spawn
		flashRadius[2] = flashRadius[1] = flashRadius[0];
	} else {
		flashRadius = dict->GetVector( "flashSize" );
		//muzzleFlash.lightCenter.Set( flashRadius[0] * 0.5f, 0.0f, 0.0f );
	}
	muzzleFlash.lightRadius								= flashRadius;
	//HUMANHEAD END

	muzzleFlash.noShadows = true;	//HUMANHEAD bjk

	if ( !flashPointLight ) {
		muzzleFlash.target								= flashTarget;
		muzzleFlash.up									= flashUp;
		muzzleFlash.right								= flashRight;
		muzzleFlash.end									= flashTarget;
	}

	//HUMANHEAD: aob
	fireDelay = dict->GetFloat( "fireRate" );
	spread = DEG2RAD( dict->GetFloat("spread") );

	yawSpread = dict->GetFloat("yawSpread");

	bCrosshair = dict->GetBool("crosshair");

	numProjectiles		= dict->GetInt( "numProjectiles" );
	//HUMANHEAD END

	deferProjNum = 0; //HUMANHEAD rww
	deferProjTime = 0; //HUMANHEAD rww
}

/*
================
hhFireController::Clear
================
*/
void hhFireController::Clear() {
	dict	= NULL;

	muzzleOrigin.Zero();
	muzzleAxis.Identity();

	// weapon definition
	numProjectiles = 0;
	projectile = NULL;
	projDictName = "";

	memset( &muzzleFlash, 0, sizeof( muzzleFlash ) );
	SAFE_FREELIGHT( muzzleFlashHandle );

	muzzleFlashEnd = 0;;
	flashTime = 0;

	// ammo management
	ammoRequired = 0;		// amount of ammo to use each shot.  0 means weapon doesn't need ammo.

	fireDelay = 0.0f;
	spread = 0.0f;

	yawSpread = 0.0f;

	bCrosshair = false;

	projectileMaxHalfDimension = 0.0f;
}


/*
================
hhFireController::SetProjectileDict
================
*/
void hhFireController::SetProjectileDict( const char* name ) {
	idVec3 mins;
	idVec3 maxs;
	projectileMaxHalfDimension = 1.0f;

	projDictName = name;

	if ( name[0] ) {
		projectile = gameLocal.FindEntityDefDict( name, false );
		if ( !projectile ) {
			gameLocal.Warning( "Unknown projectile '%s'", name );
		} else {
			const char *spawnclass = projectile->GetString( "spawnclass" );
			idTypeInfo *cls = idClass::GetClass( spawnclass );
			if ( !cls || !cls->IsType( idProjectile::Type ) ) {
				gameLocal.Error( "Invalid spawnclass '%s' on projectile '%s'", spawnclass, name );
			}
	
			mins = projectile->GetVector( "mins" );
			maxs = projectile->GetVector( "maxs" );
			projectileMaxHalfDimension = hhMath::hhMax( (maxs.y - mins.y) * 0.5f, (maxs.z - mins.z) * 0.5f );
		}
	} else {
		projectile = NULL;
	}
}

/*
================
hhFireController::DetermineProjectileAxis
================
*/
idMat3 hhFireController::DetermineProjectileAxis( const idMat3& axis ) {
	idVec3 dir = hhUtils::RandomSpreadDir( axis, spread );
	idAngles projectileAngles( dir.ToAngles() );

	projectileAngles[YAW] += yawSpread*hhMath::Sin(gameLocal.random.RandomFloat()*hhMath::TWO_PI); //rww - seperate optional yaw spread

	projectileAngles[2] = axis.ToAngles()[2];

	return projectileAngles.ToMat3();
}

/*
================
hhFireController::LaunchProjectiles
================
*/
bool hhFireController::LaunchProjectiles( const idVec3& pushVelocity ) {
	if( !GetProjectileOwner() ) {
		return false;
	}

	// check if we're out of ammo or the clip is empty
	if( !HasAmmo() ) {
		return false;
	}

	UseAmmo();

	// calculate the muzzle position
	CalculateMuzzlePosition( muzzleOrigin, muzzleAxis );

	if ( !gameLocal.isClient || dict->GetBool("net_clientProjectiles", "1") ) { //HUMANHEAD rww - clientside projectiles, because our weapons make the god of bandwidth weep.
		//HUMANHEAD: aob
		idVec3 adjustedOrigin = AssureInsideCollisionBBox( muzzleOrigin, GetSelf()->GetAxis(), GetCollisionBBox(), projectileMaxHalfDimension );
		idMat3 aimAxis = DetermineAimAxis( adjustedOrigin, GetSelf()->GetAxis() );
		//HUMANHEAD END

		LaunchProjectiles( adjustedOrigin, aimAxis, pushVelocity, GetProjectileOwner() );

		//rww - remove this from here and only do it on the client, we need to create the effect locally and with knowledge
		//of if we should get the viewmodel bone or the worldmodel one
		//CreateMuzzleFx( muzzleOrigin, muzzleAxis );
	}
	
	if (gameLocal.GetLocalPlayer())
	{ //rww - create muzzle fx on client
		idVec3 localMuzzleOrigin = muzzleOrigin;
		idMat3 localMuzzleAxis = muzzleAxis;
		if (gameLocal.isMultiplayer) { //rww - check if we should display the actual muzzle flash from a point different than the projectile launch location
			idVec3 newOrigin;
			idMat3 newAxis;
			if (CheckThirdPersonMuzzle(newOrigin, newAxis)) {
				localMuzzleOrigin = newOrigin;
				localMuzzleAxis = newAxis;
			}
		}
		CreateMuzzleFx( localMuzzleOrigin, localMuzzleAxis );
	}

	return true;
}

/*
================
hhFireController::CheckDeferredProjectiles
================
*/
void hhFireController::CheckDeferredProjectiles(void) { //rww
	assert(!gameLocal.isClient);
	if (deferProjTime > gameLocal.time) {
		return;
	}

	//FIXME compensate for intermediate time?
    
	deferProjTime = gameLocal.time + 50; //time is rather arbitrary.

	int i = 0;
	while (deferProjNum > 0 && i < MAX_NET_PROJECTILES) {
		if (!deferProjOwner.IsValid()) {
			return;
		}

		hhProjectile *projectile = SpawnProjectile();

		projectile->Create(deferProjOwner.GetEntity(), deferProjLaunchOrigin, deferProjLaunchAxis);
		projectile->spawnArgs.Set( "weapontype", GetSelf()->spawnArgs.GetString("ddaname", "") );
		projectile->Launch(deferProjLaunchOrigin, DetermineProjectileAxis(deferProjLaunchAxis), deferProjPushVelocity, 0.0f, 1.0f );

		deferProjNum--;
		i++;
	}
}

/*
================
hhFireController::LaunchProjectiles
================
*/
void hhFireController::LaunchProjectiles( const idVec3& launchOrigin, const idMat3& aimAxis, const idVec3& pushVelocity, idEntity* projOwner ) {
	if (gameLocal.isMultiplayer && numProjectiles > MAX_NET_PROJECTILES && !dict->GetBool("net_noProjectileDefer", "0") &&
		!dict->GetBool("net_clientProjectiles", "1") && !gameLocal.isClient) {
		//HUMANHEAD rww - in mp our gigantic single-snapshot projectile spawns tend to be very destructive toward bandwidth.
		//and so, projectile deferring.
		deferProjNum = numProjectiles;
		deferProjTime = 0;
		deferProjLaunchOrigin = launchOrigin;
		deferProjLaunchAxis = aimAxis;
		deferProjPushVelocity = pushVelocity;
		deferProjOwner = projOwner;
	}
	else {
		if (gameLocal.isClient && !gameLocal.isNewFrame) {
			return;
		}

		hhProjectile* projectile = NULL;
		bool clientProjectiles = dict->GetBool("net_clientProjectiles", "1");
		for( int ix = 0; ix < numProjectiles; ++ix ) {
			if (clientProjectiles) { //HUMANHEAD rww - clientside projectiles!
				projectile = hhProjectile::SpawnClientProjectile( GetProjectileDict() );
			}
			else {
				projectile = SpawnProjectile();
			}

			projectile->Create( projOwner, launchOrigin, aimAxis );

			projectile->spawnArgs.Set( "weapontype", GetSelf()->spawnArgs.GetString("ddaname", "") );

			projectile->Launch( launchOrigin, DetermineProjectileAxis(aimAxis), pushVelocity, 0.0f, 1.0f );
		}
	}
}

/*
================
hhFireController::AssureInsideCollisionBBox
================
*/
idVec3 hhFireController::AssureInsideCollisionBBox( const idVec3& origin, const idMat3& axis, const idBounds& ownerAbsBounds, float projMaxHalfDim ) const {
	float distance = 0.0f;
	if( !ownerAbsBounds.RayIntersection(origin, -axis[0], distance) ) {
		distance = 0.0f; 
	}

	// HUMANHEAD CJR:  If the player is touching a portal, then set the projectile inside the player so it has a chance to collide with the portal
	idEntity *owner = GetProjectileOwner();
	if ( owner && owner->IsType( hhPlayer::Type ) ) {
		hhPlayer *player = static_cast<hhPlayer *>(owner);
		if ( player->IsPortalColliding() ) { // Player is touching a portal, so force it inside the player bounds
			distance = 2.0f * idMath::Fabs( owner->GetPhysics()->GetBounds()[1].y ); // Push back by the player's size
		}
	} // HUMANHEAD END
	
	//Need to come back half the size of the projectiles bbox to
	//guarentee that the whole projectile is in the owners bbox
	return origin + (distance + projMaxHalfDim) * -axis[0];
}

/*
================
hhFireController::CalculateMuzzlePosition
================
*/
void hhFireController::CalculateMuzzlePosition( idVec3& origin, idMat3& axis ) {
	origin = GetMuzzlePosition();
	axis = GetSelf()->GetAxis();

	if( g_showProjectileLaunchPoint.GetBool() ) {
		idVec3 v = (origin - GetSelf()->GetOrigin()) * GetSelf()->GetAxis().Transpose();
		gameLocal.Printf( "Launching from: (relative to weapon): %s\n", v.ToString() );
		hhUtils::DebugCross( colorGreen, origin, 5, 5000 );
		gameRenderWorld->DebugLine( colorBlue, GetSelf()->GetOrigin(), GetSelf()->GetOrigin() + (v * GetSelf()->GetAxis()), 5000 );
	}
}

/*
================
hhFireController::UpdateMuzzleFlashPosition
================
*/
void hhFireController::UpdateMuzzleFlashPosition() {	
	muzzleFlash.axis = GetSelf()->GetAxis();
	muzzleFlash.origin = AssureInsideCollisionBBox( muzzleOrigin, muzzleFlash.axis, GetProjectileOwner()->GetPhysics()->GetAbsBounds(), projectileMaxHalfDimension );

	//TEST
	/*trace_t trace;
	idVec3 flashSize = dict->GetVector( "flashSize" );
	if( gameLocal.clip.TracePoint(trace, muzzleFlash.origin, muzzleFlash.origin + muzzleFlash.axis[0] * flashSize[0], MASK_VISIBILITY, GetSelf()) ) {
		flashSize[0] *= trace.fraction;
	}
	muzzleFlash.lightRadius = flashSize;
	muzzleFlash.origin += muzzleFlash.axis[0] * flashSize[0] * 0.5f;
	muzzleFlash.lightCenter.Set( flashSize[0] * -0.5f, 0.0f, 0.0f );

	hhUtils::Swap<float>( muzzleFlash.lightRadius[0], muzzleFlash.lightRadius[2] );
	hhUtils::Swap<float>( muzzleFlash.lightCenter[0], muzzleFlash.lightCenter[2] );
	muzzleFlash.axis = hhUtils::SwapXZ( muzzleFlash.axis );*/
	//TEST

	// put the world muzzle flash on the end of the joint, no matter what
	//GetGlobalJointTransform( false, flashJointWorld, muzzleOrigin, muzzleFlash.axis );
}

/*
================
hhFireController::MuzzleFlash
================
*/
void hhFireController::MuzzleFlash() {
	if (!g_muzzleFlash.GetBool()) {
		return;
	}

	UpdateMuzzleFlashPosition();

	if( muzzleFlash.lightRadius[0] < VECTOR_EPSILON || muzzleFlash.lightRadius[1] < VECTOR_EPSILON || muzzleFlash.lightRadius[2] < VECTOR_EPSILON ) {
		return;
	}

	// these will be different each fire
	muzzleFlash.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.GetTime() );
	muzzleFlash.shaderParms[ SHADERPARM_DIVERSITY ]		= gameLocal.random.RandomFloat();

	//info.worldMuzzleFlash.shaderParms[ SHADERPARM_TIMEOFFSET ]	= -MS2SEC( gameLocal.GetTime() );
	//info.worldMuzzleFlash.shaderParms[ SHADERPARM_DIVERSITY ]	= renderEntity.shaderParms[ SHADERPARM_DIVERSITY ];

	// the light will be removed at this time
	muzzleFlashEnd = gameLocal.GetTime() + flashTime;

	if ( muzzleFlashHandle != -1 ) {
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
		//gameRenderWorld->UpdateLightDef( info.worldMuzzleFlashHandle, &info.worldMuzzleFlash );
	} else {
		muzzleFlashHandle = gameRenderWorld->AddLightDef( &muzzleFlash );
		//info.worldMuzzleFlashHandle = gameRenderWorld->AddLightDef( &info.worldMuzzleFlash );
	}
}

/*
================
hhFireController::UpdateMuzzleFlash
================
*/
void hhFireController::UpdateMuzzleFlash() {
	AttemptToRemoveMuzzleFlash();

	if( muzzleFlashHandle != -1 ) {
		UpdateMuzzleFlashPosition();
		if( muzzleFlash.lightRadius[0] < VECTOR_EPSILON || muzzleFlash.lightRadius[1] < VECTOR_EPSILON || muzzleFlash.lightRadius[2] < VECTOR_EPSILON ) { //rww - added to mimic the behaviour of hhFireController::MuzzleFlash
			return;
		}
		gameRenderWorld->UpdateLightDef( muzzleFlashHandle, &muzzleFlash );
		//gameRenderWorld->UpdateLightDef( worldMuzzleFlashHandle, &worldMuzzleFlash );
	}
}

/*
================
hhFireController::CreateMuzzleFx
================
*/
void hhFireController::CreateMuzzleFx( const idVec3& pos, const idMat3& axis ) {
	hhFxInfo fxInfo;

	if (!gameLocal.GetLocalPlayer()) { //rww - not at all necessary for ded server
		return;
	}
	if( GetSelf()->IsHidden() || !GetSelf()->GetRenderEntity()->hModel ) {
		return;
	}

	fxInfo.SetNormal( axis[0] );
	fxInfo.RemoveWhenDone( true );
	fxInfo.SetEntity( GetSelf() );
	

	//GetSelf()->BroadcastFxInfo( dict->GetString("fx_muzzleFlash"), pos, axis, &fxInfo );
	//rww - this is now client-only.
	GetSelf()->SpawnFxLocal( dict->GetString("fx_muzzleFlash"), pos, axis, &fxInfo, true );
}

/*
================
hhFireController::Save
================
*/
void hhFireController::Save( idSaveGame *savefile ) const {
	savefile->WriteDict( dict );
	savefile->WriteFloat( yawSpread );
	savefile->WriteString( projDictName );
	savefile->WriteVec3( muzzleOrigin );
	savefile->WriteMat3( muzzleAxis );
	savefile->WriteRenderLight( muzzleFlash );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
	//savefile->WriteInt( muzzleFlashHandle );
	savefile->WriteInt( muzzleFlashEnd );
	savefile->WriteInt( flashTime );
	savefile->WriteInt( ammoRequired );
	savefile->WriteFloat( fireDelay );
	savefile->WriteFloat( spread );
	savefile->WriteBool( bCrosshair );
	savefile->WriteFloat( projectileMaxHalfDimension );
	savefile->WriteInt( numProjectiles );
	//HUMANHEAD PCF mdl 05/04/06 - Save whether the light is active
	savefile->WriteBool( muzzleFlashHandle != -1 );
}

/*
================
hhFireController::Restore
================
*/
void hhFireController::Restore( idRestoreGame *savefile ) {
	savefile->ReadDict( &restoredDict );
	dict = &restoredDict;
	savefile->ReadFloat( yawSpread );
	savefile->ReadString( projDictName );
	savefile->ReadVec3( muzzleOrigin );
	savefile->ReadMat3( muzzleAxis );
	savefile->ReadRenderLight( muzzleFlash );
	//HUMANHEAD PCF mdl 05/04/06 - Don't save light handles
	//savefile->ReadInt( muzzleFlashHandle );
	savefile->ReadInt( muzzleFlashEnd );
	savefile->ReadInt( flashTime );
	savefile->ReadInt( ammoRequired );
	savefile->ReadFloat( fireDelay );
	savefile->ReadFloat( spread );
	savefile->ReadBool( bCrosshair );
	savefile->ReadFloat( projectileMaxHalfDimension );
	savefile->ReadInt( numProjectiles );

	//HUMANHEAD PCF mdl 05/04/06 - Restore the light if necessary
	bool bLight;
	savefile->ReadBool( bLight );
	if ( bLight ) {
		muzzleFlashHandle = gameRenderWorld->AddLightDef( &muzzleFlash );
	}

	SetProjectileDict( projDictName );
}

/*
==============================
hhFireController::GetMuzzlePosition
==============================
*/
idVec3 hhFireController::GetMuzzlePosition() const {
	idVec3 muzzle( dict->GetVector("muzzleOffset", "2 0 0") );
	return GetSelfConst()->GetOrigin() + ( muzzle * GetSelfConst()->GetAxis() );
}

/*
==============================
hhFireController::GetMuzzlePosition
==============================
*/
bool hhFireController::CheckThirdPersonMuzzle(idVec3 &origin, idMat3 &axis) { //rww
	return false;
}
