// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "AntiLag.h"
#include "Player.h"

idCVar g_drawAntiLag(			"g_drawAntiLag",		"0",	CVAR_BOOL | CVAR_GAME, "Visualizes the anti-lag point generation" );
idCVar g_drawAntiLagHits(		"g_drawAntiLagHits",	"0",	CVAR_BOOL | CVAR_GAME, "Draws information when anti-lag generates a hit" );

#if defined( SD_PUBLIC_BUILD )
	#define ANTILAG_CVAR_FLAGS		( CVAR_GAME | CVAR_SERVERINFO | CVAR_ROM )
#else
	#define ANTILAG_CVAR_FLAGS		( CVAR_GAME | CVAR_SERVERINFO )
#endif

idCVar si_antiLag(				"si_antiLag",			"1",	CVAR_BOOL | ANTILAG_CVAR_FLAGS, "Server does antilag on players" );
idCVar si_antiLagOnly(			"si_antiLagOnly",		"0",	CVAR_BOOL | ANTILAG_CVAR_FLAGS, "ONLY use antilag" );
idCVar si_antiLagForgiving(		"si_antiLagForgiving",	"0",	CVAR_INTEGER | ANTILAG_CVAR_FLAGS, "How forgiving the antilag is - the higher, the more forgiving" );

#ifdef ANTILAG_LOGGING
idCVar si_antiLagLog(			"si_antiLagLog",		"0",	CVAR_BOOL | CVAR_GAME, "Server logs antilag debugging information" );
#endif

/*
===============================================================================

	sdAntiLagSet
		base class for antilag sets

===============================================================================
*/

/*
================
sdAntiLagSet::sdAntiLagSet
================
*/
sdAntiLagSet::sdAntiLagSet() {
	Reset();
}

/*
================
sdAntiLagSet::Reset
================
*/
void sdAntiLagSet::Reset() {
	branchTime = -1;
	bounds.Clear();
	entityBounds.Clear();
	clientsValidFor.Clear();
	antiLagEntity = NULL;
	userCommandOnly = false;
	baseBranchTime = -1;
}

/*
================
sdAntiLagSet::Setup
================
*/
void sdAntiLagSet::Setup( sdAntiLagEntity& _antiLagEntity ) {
	antiLagEntity = &_antiLagEntity;
	branchTime = gameLocal.time;

	idEntity* entity = antiLagEntity->GetSelf();
	points[ 0 ] = entity->GetPhysics()->GetOrigin();
	velocity = entity->GetPhysics()->GetLinearVelocity();
	entityBounds = entity->GetPhysics()->GetBounds();
	bounds = entityBounds.Translate( points[ 0 ] );

	clientsValidFor.Clear();

	onGround = entity->GetPhysics()->HasGroundContacts();

	timeUpto = branchTime;
}

/*
================
sdAntiLagSet::Setup
================
*/
void sdAntiLagSet::Setup( sdAntiLagEntity& _antiLagEntity, sdAntiLagSet& baseSet ) {
	antiLagEntity = &_antiLagEntity;
	branchTime = gameLocal.time;

	points[ 0 ] = *baseSet.GetPointForTime( gameLocal.time );
	velocity = baseSet.GetVelocity();
	entityBounds = baseSet.GetEntityBounds();
	bounds = entityBounds.Translate( points[ 0 ] );

	clientsValidFor.Clear();

	onGround = baseSet.OnGround();

	timeUpto = branchTime;

	baseBranchTime = baseSet.GetBranchTime();
	userCommandOnly = true;
}

/*
================
sdAntiLagSet::DebugDraw
================
*/
void sdAntiLagSet::DebugDraw() {
	if ( !IsValid() ) {
		return;
	}
	if ( gameLocal.msec == 0 ) {
		return;
	}

	int maxIndex = ( timeUpto - branchTime ) / gameLocal.msec;
	for ( int i = 1; i < maxIndex; i++ ) {
		gameRenderWorld->DebugBounds( colorRed, entityBounds, points[ i ] );
	}
}

/*
================
sdAntiLagSet::GetPointForTime
================
*/
const idVec3* sdAntiLagSet::GetPointForTime( int time ) {
	if ( gameLocal.msec == 0 ) {
		return NULL;
	}

	int index = ( time - branchTime ) / gameLocal.msec;
	if ( index < 0 || index >= MAX_ANTILAG_POINTS ) {
		return NULL;
	}

	// make sure its updated all the way
	int lastTimeUpto = timeUpto;
	while ( time > timeUpto ) {
		Update();

		// HACK - if Update didn't change the time then something is badly wrong
		//        (eg using timescale on a server) so abort, otherwise will get
		//        stuck in an infinite loop!
		if ( timeUpto == lastTimeUpto ) {
			return NULL;
		}
	}

	return &points[ index ];
}

/*
===============================================================================

	sdAntiLagSetGeneric
		generic antilag set

===============================================================================
*/

/*
================
sdAntiLagSetGeneric::Setup
================
*/
void sdAntiLagSetGeneric::Setup( sdAntiLagEntity& _antiLagEntity ) {
	sdAntiLagSet::Setup( _antiLagEntity );

	idEntity* entity = antiLagEntity->GetSelf();

	if ( !entity->GetPhysics()->HasGroundContacts() ) {
		acceleration = entity->GetPhysics()->GetGravity();
	} else {
		acceleration.Zero();
	}
}

/*
================
sdAntiLagSetGeneric::Setup
================
*/
void sdAntiLagSetGeneric::Setup( sdAntiLagEntity& _antiLagEntity, sdAntiLagSet& baseSet ) {
	sdAntiLagSet::Setup( _antiLagEntity, baseSet );

	// HACK - totally shouldn't assume this.
	sdAntiLagSetGeneric& genericSet = static_cast< sdAntiLagSetGeneric& >( baseSet );

	acceleration = genericSet.GetAcceleration();
}

/*
================
sdAntiLagSetGeneric::Update
================
*/
void sdAntiLagSetGeneric::Update() {
	if ( !IsValid() ) {
		return;
	}
	if ( gameLocal.msec == 0 ) {
		return;
	}

	int newTime = timeUpto + gameLocal.msec;
	int index = ( newTime - branchTime ) / gameLocal.msec;
	if ( index <= 0 || index >= MAX_ANTILAG_POINTS ) {
		return;
	}

	timeUpto = newTime;

	// carry on the "prediction"
	float timeDelta = MS2SEC( gameLocal.msec );

	velocity += acceleration * timeDelta;
	points[ index ] = points[ index - 1 ] + velocity * timeDelta;
	bounds.AddBounds( entityBounds.Translate( points[ index ] ) );
}


/*
===============================================================================

	sdAntiLagSetPlayer
		player antilag set

===============================================================================
*/

/*
================
sdAntiLagSetPlayer::Setup
================
*/
void sdAntiLagSetPlayer::Setup( sdAntiLagEntity& _antiLagEntity ) {
	sdAntiLagSet::Setup( _antiLagEntity );

	idEntity* entity = antiLagEntity->GetSelf();
	assert( entity->IsType( idPlayer::Type ) );

	idPlayer* playerSelf = entity->Cast< idPlayer >();
	gravity = playerSelf->GetPlayerPhysics().GetGravity();
	idVec3 gravityNormal = gravity;
	gravityNormal.Normalize();
	waterLevel = playerSelf->GetPlayerPhysics().GetWaterLevel();

	walkSpeedFwd = playerSelf->GetPlayerPhysics().GetWalkSpeedFwd();
	walkSpeedBack = playerSelf->GetPlayerPhysics().GetWalkSpeedBack();
	walkSpeedSide = playerSelf->GetPlayerPhysics().GetWalkSpeedSide();

	playerSpeed = playerSelf->GetPlayerPhysics().GetCurrentAimSpeed();

	idAngles viewAngles = playerSelf->GetPlayerPhysics().GetViewAngles();
	viewAngles.ToVectors( &viewForward, NULL, NULL );
	viewRight = gravityNormal.Cross( viewForward );
	viewRight.Normalize();

	cmdForwardMove = playerSelf->usercmd.forwardmove;
	cmdRightMove = playerSelf->usercmd.rightmove;
	

	if ( OnGround() ) {
		groundNormal.Set( 0.0f, 0.0f, 1.0f );
	} else {
		groundNormal = playerSelf->GetPlayerPhysics().GetGroundNormal();
	}
}

/*
================
sdAntiLagSetPlayer::Setup
================
*/
void sdAntiLagSetPlayer::Setup( sdAntiLagEntity& _antiLagEntity, sdAntiLagSet& baseSet ) {
	sdAntiLagSet::Setup( _antiLagEntity, baseSet );

	// HACK: Totally shouldn't assume this.
	sdAntiLagSetPlayer& playerSet = static_cast< sdAntiLagSetPlayer& >( baseSet );

	idEntity* entity = antiLagEntity->GetSelf();
	assert( entity->IsType( idPlayer::Type ) );

	idPlayer* playerSelf = entity->Cast< idPlayer >();

	gravity = playerSet.GetGravity();
	waterLevel = playerSet.GetWaterLevel();

	walkSpeedFwd = playerSelf->GetPlayerPhysics().GetWalkSpeedFwd();
	walkSpeedBack = playerSelf->GetPlayerPhysics().GetWalkSpeedBack();
	walkSpeedSide = playerSelf->GetPlayerPhysics().GetWalkSpeedSide();

	playerSpeed = playerSelf->GetPlayerPhysics().GetCurrentAimSpeed();

	idVec3 gravityNormal = gravity;
	gravityNormal.Normalize();
	idAngles viewAngles = playerSelf->GetPlayerPhysics().GetViewAngles();
	viewAngles.ToVectors( &viewForward, NULL, NULL );
	viewRight = gravityNormal.Cross( viewForward );
	viewRight.Normalize();

	cmdForwardMove = playerSelf->usercmd.forwardmove;
	cmdRightMove = playerSelf->usercmd.rightmove;
	
	groundNormal = playerSet.GetGroundNormal();
}

/*
================
sdAntiLagSetPlayer::CmdScale
================
*/
float sdAntiLagSetPlayer::CmdScale( int forwardmove, int rightmove, int upmove, bool noVertical ) const {
	int		max;
	float	total;
	float	scale;

	// since the crouch key doubles as downward movement, ignore downward movement when we're on the ground
	// otherwise crouch speed will be lower than specified
	if ( noVertical ) {
		upmove = 0;
	}

	max = abs( forwardmove );
	if ( abs( rightmove ) > max ) {
		max = abs( rightmove );
	}
	if ( abs( upmove ) > max ) {
		max = abs( upmove );
	}

	if ( !max ) {
		return 0.0f;
	}

	total = idMath::Sqrt( (float) forwardmove * forwardmove + rightmove * rightmove + upmove * upmove );
	scale = (float) playerSpeed * max / ( 127.0f * total );

	return scale;
}

/*
================
sdAntiLagSetPlayer::CalcDesiredWalkMove
================
*/
idVec2 sdAntiLagSetPlayer::CalcDesiredWalkMove( int forwardmove, int rightmove ) const {
	idVec2 adjustedMove( 0.0f, 0.0f );
	float forwardMoveAdjusted = 0.0f;
	float rightMoveAdjusted = 0.0f;

	// divide the desired movement into "max" and "min" axes
	// to smoothly (ish) calculate how we're going to be moving
	float maxMoveSpeedLimit;
	idVec2 maxMoveAxis;
	float maxMoveStrength = idMath::Fabs( ( float )forwardmove );
	if ( forwardmove >= 0 ) {
		maxMoveSpeedLimit = walkSpeedFwd;
		maxMoveAxis.Set( 1.0f, 0.0f );
	} else {
		maxMoveSpeedLimit = walkSpeedBack;
		maxMoveAxis.Set( -1.0f, 0.0f );
	}

	float minMoveSpeedLimit = walkSpeedSide;
	idVec2 minMoveAxis;
	float minMoveStrength = idMath::Fabs( ( float )rightmove );
	if ( rightmove >= 0 ) {
		minMoveAxis.Set( 0.0f, 1.0f );
	} else {
		minMoveAxis.Set( 0.0f, -1.0f );
	}

	if ( minMoveSpeedLimit > maxMoveSpeedLimit ) {
		Swap( minMoveSpeedLimit, maxMoveSpeedLimit );
		Swap( minMoveAxis, maxMoveAxis );
		Swap( minMoveStrength, maxMoveStrength );
	}

	float maxMoveSpeed = maxMoveSpeedLimit * maxMoveStrength / 127.0f;
	float minMoveSpeed = minMoveSpeedLimit * minMoveStrength / 127.0f;

	if ( maxMoveSpeedLimit < idMath::FLT_EPSILON || minMoveSpeedLimit  < idMath::FLT_EPSILON ) {
		// do nothing! - no move
	} else {
		float minMoveAdjusted = 0.0f;
		float maxMoveAdjusted = 0.0f;

		if ( maxMoveSpeed < idMath::FLT_EPSILON && minMoveSpeed < idMath::FLT_EPSILON ) {
			// do nothing! - no move
		} else if ( maxMoveStrength >= minMoveStrength ) {
			// calculate the length of the full-strength move in this direction
			float fullLengthScale = maxMoveSpeedLimit / maxMoveSpeed;
			float fullLength = fullLengthScale * idMath::Sqrt( maxMoveSpeed * maxMoveSpeed + minMoveSpeed * minMoveSpeed );
			
			// scale-back to fit the maximum speed we're allowed
			maxMoveAdjusted = maxMoveSpeed * maxMoveSpeedLimit / fullLength;
			minMoveAdjusted = minMoveSpeed * maxMoveSpeedLimit / fullLength;
		} else {
			// calculate the length of the full-strength move in this direction
			float fullLengthScale = minMoveSpeedLimit / minMoveSpeed;
			float fullLength = fullLengthScale * idMath::Sqrt( maxMoveSpeed * maxMoveSpeed + minMoveSpeed * minMoveSpeed );

			// linearly blend the max total strength based on how close it is to the junction point between
			// this case and the previous case (maxMoveStrength >= minMoveStrength)
			float maxLength = Lerp( minMoveSpeedLimit, maxMoveSpeedLimit, ( maxMoveSpeed * fullLengthScale ) / maxMoveSpeedLimit );

			// scale-back to fit the maximum speed we're allowed
			maxMoveAdjusted = maxMoveSpeed * maxLength / fullLength;
			minMoveAdjusted = minMoveSpeed * maxLength / fullLength;
		}

		adjustedMove = maxMoveAdjusted * maxMoveAxis + minMoveAdjusted * minMoveAxis;

		// scale everything so that the max possible speed equals the playerspeed
		float absoluteMaxSpeed = walkSpeedFwd;
		if ( walkSpeedBack > absoluteMaxSpeed ) {
			absoluteMaxSpeed = walkSpeedBack;
		}
		if ( walkSpeedSide > absoluteMaxSpeed ) {
			absoluteMaxSpeed = walkSpeedSide;
		}
		if ( absoluteMaxSpeed > idMath::FLT_EPSILON ) {
			adjustedMove *= playerSpeed / absoluteMaxSpeed;
		}
	}

	return adjustedMove;
}

/*
================
sdAntiLagSetPlayer::Accelerate
================
*/
void sdAntiLagSetPlayer::Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel ) {
	// q2 style
	float addspeed, accelspeed, currentspeed;

	currentspeed = velocity * wishdir;
	addspeed = wishspeed - currentspeed;
	if ( addspeed <= 0.0f ) {
		return;
	}
	accelspeed = accel * MS2SEC( gameLocal.msec ) * wishspeed;
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}
	
	velocity += accelspeed * wishdir;
}

/*
================
sdAntiLagSetPlayer::WalkMove
================
*/
void sdAntiLagSetPlayer::WalkMove() {
	idVec3		wishvel;
	idVec3		wishdir;
	float		wishspeed;
	float		accelerate;

	idPlayer* playerSelf = antiLagEntity->GetSelf()->Cast< idPlayer >();
	assert( playerSelf != NULL );

	Friction();

	idVec3 gravityNormal = gravity;
	gravityNormal.Normalize();

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;

//	assert( groundNormal.z >= MIN_WALK_NORMAL );
	viewForward = idPhysics_Player::AdjustVertically( groundNormal, viewForward );
	viewRight = idPhysics_Player::AdjustVertically( groundNormal, viewRight );

	viewForward.Normalize();
	viewRight.Normalize();

	// find how we want to move
	idVec2 adjustedMove = CalcDesiredWalkMove( cmdForwardMove, cmdRightMove );
	wishdir = viewForward * adjustedMove.x + viewRight * adjustedMove.y;
	wishspeed = wishdir.Normalize();

	// clamp the speed lower if wading or walking on the bottom
	if ( waterLevel ) {
		float	waterScale;

		waterScale = waterLevel / 3.0f;
		waterScale = 1.0f - ( 1.0f - playerSelf->GetPlayerPhysics().GetSwimScale() ) * waterScale;
		if ( wishspeed > playerSpeed * waterScale ) {
			wishspeed = playerSpeed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose full control, which allows them to be moved a bit
//	bool fLowControl = ( groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK ) || current.movementFlags & PMF_TIME_KNOCKBACK;
//	accelerate = fLowControl ? pm_airAccelerate : pm_accelerate;
	accelerate = playerSelf->GetPlayerPhysics().GetGroundAccel();
	Accelerate( wishdir, wishspeed, accelerate );
//	if ( fLowControl ) {
//		current.velocity += gravityVector * frametime;
//	}

	velocity = idPhysics_Player::AdjustVertically( groundNormal, velocity );
}

/*
================
sdAntiLagSetPlayer::Friction
================
*/
void sdAntiLagSetPlayer::Friction() {
	idVec3	vel;
	float	speed, newspeed, control;
	float	drop;
	
	idVec3 gravityNormal = gravity;
	gravityNormal.Normalize();

	vel = velocity;
	if ( onGround ) {
		// ignore slope movement, remove all velocity in gravity direction
		vel += ( vel * gravityNormal ) * gravityNormal;
	}

	speed = vel.Length();
	if ( speed < 1.0f ) {
		// remove all movement orthogonal to gravity, allows for sinking underwater
		if ( fabs( velocity * gravityNormal ) < 1e-5f ) {
			velocity.Zero();
		} else {
			velocity = ( velocity * gravityNormal ) * gravityNormal;
		}
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	idPlayer* playerSelf = antiLagEntity->GetSelf()->Cast< idPlayer >();
	assert( playerSelf != NULL );

	float pm_stopSpeed = playerSelf->GetPlayerPhysics().GetStopSpeed();
	float pm_friction = playerSelf->GetPlayerPhysics().GetGroundFriction();
	float pm_waterFriction = playerSelf->GetPlayerPhysics().GetWaterFriction();
	float pm_airFriction = playerSelf->GetPlayerPhysics().GetAirFriction();
	float frametime = MS2SEC( gameLocal.msec );

	// spectator friction
//	if ( current.movementType == PM_SPECTATOR ) {
//		drop += speed * pm_flyFriction * frametime;
//	} else if ( walking && waterLevel <= WATERLEVEL_FEET ) {
		// apply ground friction	
		// no friction on slick surfaces
//		if ( !(groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK) ) {

			// if getting knocked back, no friction
//			if ( !(current.movementFlags & PMF_TIME_KNOCKBACK) ) {
	if ( onGround ) {
				control = speed < pm_stopSpeed ? pm_stopSpeed : speed;
				float friction;
				if ( pm_friction < 0 ) {
					friction = ::pm_friction.GetFloat();
				} else {
					friction = pm_friction;
				}
				drop += control * friction * frametime;
//			}
//		}
	} else if ( waterLevel > WATERLEVEL_NONE ) {
		// apply water friction even if just wading	
		drop += speed * pm_waterFriction * waterLevel * frametime;
	} else {
		// apply air friction	
		drop += speed * pm_airFriction * frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	velocity *= ( newspeed / speed );

	// snap to avoid denormals
	velocity.FixDenormals( 1.0e-5f );
}

/*
================
sdAntiLagSetPlayer::Update
================
*/
void sdAntiLagSetPlayer::Update() {
	if ( !IsValid() ) {
		return;
	}
	if ( gameLocal.msec == 0 ) {
		return;
	}

	int newTime = timeUpto + gameLocal.msec;
	int index = ( newTime - branchTime ) / gameLocal.msec;
	if ( index <= 0 || index >= MAX_ANTILAG_POINTS ) {
		return;
	}

	timeUpto = newTime;

	// carry on the "prediction"
	float timeDelta = MS2SEC( gameLocal.msec );

	// friction on-ground for players
	idPlayer* playerSelf = static_cast< idPlayer* >( antiLagEntity->GetSelf() );
	if ( OnGround() ) {
		WalkMove();
	} else {
		velocity += gravity * timeDelta;
	}

	points[ index ] = points[ index - 1 ] + velocity * timeDelta;
	// handle landing
	if ( !OnGround() ) {
		if ( playerSelf->GetPlayerPhysics().HasGroundContacts() ) {
			if ( points[ index ].z <= playerSelf->GetPlayerPhysics().GetOrigin().z ) {
				points[ index ].z = playerSelf->GetPlayerPhysics().GetOrigin().z;
				onGround = true;
				groundNormal = playerSelf->GetPlayerPhysics().GetGroundNormal();

				velocity -= ( velocity * groundNormal ) * groundNormal;
			}
		}
	}

	bounds.AddBounds( entityBounds.Translate( points[ index ] ) );
}

/*
===============================================================================

	sdAntiLagEntity
		One entity's antilag state

===============================================================================
*/

/*
================
sdAntiLagEntity::Create
================
*/
void sdAntiLagEntity::Create() {
	for ( int i = 0; i < MAX_ANTILAG_SETS; i++ ) {
		antiLagSets[ i ] = new sdAntiLagSetGeneric;
	}
}

/*
================
sdAntiLagEntity::Destroy
================
*/
void sdAntiLagEntity::Destroy() {
	for ( int i = 0; i < MAX_ANTILAG_SETS; i++ ) {
		delete antiLagSets[ i ];
		antiLagSets[ i ] = NULL;
	}
}


/*
================
sdAntiLagEntity::Reset
================
*/
void sdAntiLagEntity::Reset() {
	self = NULL;

	for ( int i = 0; i < MAX_ANTILAG_SETS; i++ ) {
		antiLagSets[ i ]->Reset();
	}

	antiLagUpto = 0;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		clientEstimates[ i ].Reset( vec3_origin );
	}
}

/*
================
sdAntiLagEntity::Init
================
*/
void sdAntiLagEntity::Init( const idEntity* _self ) {
	Reset();

	self = _self;

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		clientEstimates[ i ].Init( self );
		clientEstimates[ i ].Reset( self->GetPhysics()->GetOrigin() );
	}
}


/*
================
sdAntiLagEntity::Update
================
*/
void sdAntiLagEntity::Update() {
	for ( int i = 0; i < MAX_ANTILAG_SETS; i++ ) {
		if ( !antiLagSets[ i ]->IsValid() ) {
			continue;
		}

		antiLagSets[ i ]->Update();
		if ( g_drawAntiLag.GetBool() ) {
			antiLagSets[ i ]->DebugDraw();
		}
	}

	//
	// Update where the other clients think this entity is
	//
	idPlayer* playerSelf = self->Cast< idPlayer >();
	idVec3 trueOrigin = self->GetPhysics()->GetOrigin();
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( i == self->entityNumber ) {
			// self is always correct
			clientEstimates[ i ].Reset( trueOrigin );
			continue;
		}

		idPlayer* other = gameLocal.GetClient( i );
		if ( other == NULL ) {
			// other does not exist
			clientEstimates[ i ].Reset( trueOrigin );
			continue;
		}

		if ( self->GetHealth() <= 0 ) {
			// dead or invulnerable - no need for antilag
			clientEstimates[ i ].Reset( trueOrigin );
			continue;
		}

		if ( playerSelf != NULL && ( playerSelf->IsSpectator() || playerSelf->GetProxyEntity() != NULL ) ) {
			// spectating or in a vehicle - no need for antilag
			clientEstimates[ i ].Reset( trueOrigin );
			continue;
		}
		
		// estimate based on the sets & prediction time etc where 
		// the other client is predicting this entity to be
		int prediction = networkSystem->ServerGetClientPrediction( i );
		if ( prediction <= 0 ) {
			// not predicting ahead, it can use the exact position
			clientEstimates[ i ].Reset( trueOrigin );
			continue;
		}

		sdAntiLagSet* set = GetAntiLagSet( i );
		if ( set == NULL ) {
			// this means the client is lagging so bad that it doesn't have any information anymore.
			// stuff them - if they're that badly lagged then they won't be able to hit anyway.
			clientEstimates[ i ].Reset( trueOrigin );
			continue;
		}
		
		const idVec3* predictedPosition = set->GetPointForTime( gameLocal.time );
		if ( predictedPosition == NULL ) {
			// out of bounds of the set - again, means they must be hitting an extreme 
			// network performance case - they won't be able to hit anyway
			clientEstimates[ i ].Reset( trueOrigin );
			continue;
		}

		// prevent it dipping below the true ground level
		idVec3 newPos = *predictedPosition;
		if ( newPos.z < trueOrigin.z && self->GetPhysics()->HasGroundContacts() ) {
			newPos.z = trueOrigin.z;
		}

		// set up the information needed for prediction error decay to simulate what the client is doing
		idVec3 viewOrigin;
		idMat3 viewAxis;
		other->GetAORView( viewOrigin, viewAxis );

		sdPredictionErrorDecay_Origin::CustomDecayInfo decayInfo;
		if ( self->aorLayout != NULL ) {
			decayInfo.physicsCutoffSqr = self->aorLayout->GetPhysicsCutoffSqr();
			decayInfo.ownerAorDistSqr = ( self->GetPhysics()->GetOrigin() - viewOrigin ).LengthSqr();
			decayInfo.packetSpread = ( int )( self->aorLayout->GetSpreadForDistanceSqr( decayInfo.ownerAorDistSqr ) * 1000.0f );
		} else {
			decayInfo.physicsCutoffSqr = 1.0f;
			decayInfo.ownerAorDistSqr = 0.0f;
			decayInfo.packetSpread = 0;
		}

#ifdef ANTILAG_LOGGING
		antiLagDistances[ i ] = decayInfo.ownerAorDistSqr;
#endif

		decayInfo.boxDecayClip = decayInfo.heightMapDecayClip = decayInfo.pointDecayClip = false;
		decayInfo.hasLocalPhysics = decayInfo.ownerAorDistSqr < decayInfo.physicsCutoffSqr;
		decayInfo.isPlayer = self->IsType( idPlayer::Type );
		decayInfo.currentPrediction = prediction;
		decayInfo.limitVelocity = set->GetVelocity();
		decayInfo.origin = newPos;
		decayInfo.hasGroundContacts = set->OnGround();

		clientEstimates[ i ].SetOwner( self );
		int branchTime = set->GetBranchTime();
		if ( clientEstimates[ i ].GetNetworkTime() < branchTime ) {
			clientEstimates[ i ].OnNewInfo( set->GetBranchPoint(), branchTime, &decayInfo );
		}

		clientEstimates[ i ].Update( &decayInfo );
		clientEstimates[ i ].Decay( newPos, &decayInfo );

//		if ( other != gameLocal.GetLocalPlayer() ) {
//			gameRenderWorld->DebugBounds( colorBlue, set->GetEntityBounds(), *predictedPosition );
//		}
	}

//	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
//		idPlayer* other = gameLocal.GetClient( i );
//		if ( other != NULL && other != gameLocal.GetLocalPlayer() ) {
//			gameRenderWorld->DebugBounds( colorYellow, self->GetPhysics()->GetBounds(), clientEstimates[ i ].GetLastReturned() );
//		}
//	}
}

/*
================
sdAntiLagEntity::CreateBranch
================
*/
void sdAntiLagEntity::CreateBranch( int clientNum ) {
	assert( antiLagUpto >= 0 && antiLagUpto < MAX_ANTILAG_SETS );

	// check if any set already branches from now
	int upto = antiLagUpto;
	do {
		upto = ( upto - 1 + MAX_ANTILAG_SETS ) % MAX_ANTILAG_SETS;

		if ( antiLagSets[ upto ]->GetBranchTime() == gameLocal.time ) {
			// one already exists, make sure its enabled for this client
			antiLagSets[ upto ]->SetValidFor( clientNum );
			return;
		}
	} while ( upto != antiLagUpto );

	antiLagSets[ antiLagUpto ]->Reset();
	antiLagSets[ antiLagUpto ]->Setup( *this );
	antiLagSets[ antiLagUpto ]->SetValidFor( clientNum );

	antiLagUpto = ( antiLagUpto + 1 ) % MAX_ANTILAG_SETS;
}

/*
================
sdAntiLagEntity::CreateUserCommandBranch
================
*/
void sdAntiLagEntity::CreateUserCommandBranch( int clientNum ) {
	assert( antiLagUpto >= 0 && antiLagUpto < MAX_ANTILAG_SETS );

	// find the previous full branch heading for this client
	int upto = antiLagUpto;
	sdAntiLagSet* previousFullBranch = NULL;
	do {
		upto = ( upto - 1 + MAX_ANTILAG_SETS ) % MAX_ANTILAG_SETS;
		if ( upto == antiLagUpto ) {
			break;
		}

		if ( !antiLagSets[ upto ]->IsUserCommandOnly() ) {
			if ( antiLagSets[ upto ]->IsValidFor( clientNum ) ) {
				previousFullBranch = antiLagSets[ upto ];
				break;
			}
		}
	} while ( upto != antiLagUpto );

	if ( previousFullBranch == NULL ) {
		// there is no previous full branch valid for this client
		// can't really do anything about that!
		return;
	}

	if ( previousFullBranch->GetBranchTime() == gameLocal.time ) {
		// its a full branch from *now*
		// no point branching from that
		return;
	}

	if ( previousFullBranch->GetPointForTime( gameLocal.time ) == NULL ) {
		// its too old to have a point from now!
		// can't really do anything with that!
		return;
	}

	// check if any set already branches from our previous full branch
	// other clients are likely to be branching similarly
	upto = antiLagUpto;
	do {
		upto = ( upto - 1 + MAX_ANTILAG_SETS ) % MAX_ANTILAG_SETS;

		if ( !antiLagSets[ upto ]->IsUserCommandOnly() ) {
			continue;
		}

		if ( antiLagSets[ upto ]->GetBranchTime() == gameLocal.time 
			&& antiLagSets[ upto ]->GetBaseBranchTime() == previousFullBranch->GetBranchTime() ) {
			// branches from the appropriate time
			// set it valid for us and move on :)
			antiLagSets[ upto ]->SetValidFor( clientNum );
			return;
		}
	} while ( upto != antiLagUpto );

	assert( antiLagSets[ antiLagUpto ] != previousFullBranch );

	antiLagSets[ antiLagUpto ]->Reset();
	antiLagSets[ antiLagUpto ]->Setup( *this, *previousFullBranch );
	antiLagSets[ antiLagUpto ]->SetValidFor( clientNum );

	antiLagUpto = ( antiLagUpto + 1 ) % MAX_ANTILAG_SETS;
}

/*
================
sdAntiLagEntity::GetBounds
================
*/
idBounds sdAntiLagEntity::GetBounds() {
	idBounds totalBounds;
	totalBounds.Clear();
	for ( int i = 0; i < MAX_ANTILAG_SETS; i++ ) {
		if ( antiLagSets[ i ]->IsValid() ) {
			totalBounds.AddBounds( antiLagSets[ i ]->GetBounds() );
		}
	}
	return totalBounds;
}

/*
================
sdAntiLagEntity::GetAntiLagSet
================
*/
sdAntiLagSet* sdAntiLagEntity::GetAntiLagSet( int clientNum ) {
	int realPrediction = networkSystem->ServerGetClientPrediction( clientNum );
	const clientNetworkInfo_t& info = gameLocal.GetNetworkInfo( clientNum );

	// use this info to try to estimate when the last received packet will have been
	// see how far in the past we were seeing just from ordinary old prediction
	int fullReceivedTime = gameLocal.time - realPrediction;
	int	commandReceivedTime = fullReceivedTime;
	if ( self->entityNumber < MAX_CLIENTS ) {
		int timeSinceLastConfirm = gameLocal.time - info.lastUserCommand[ self->entityNumber ];
		int timeSinceNextConfirm = timeSinceLastConfirm - info.lastUserCommandDelay[ self->entityNumber ];
		if ( timeSinceNextConfirm > 0 ) {
			commandReceivedTime = gameLocal.time - timeSinceNextConfirm;
		}
	}

	// find the first one branching near here
	int upto = antiLagUpto;
	do {
		upto = ( upto - 1 + MAX_ANTILAG_SETS ) % MAX_ANTILAG_SETS;
		if ( !antiLagSets[ upto ]->IsValid() ) {
			continue;
		}
		if ( !antiLagSets[ upto ]->IsValidFor( clientNum ) ) {
			continue;
		}

		if ( antiLagSets[ upto ]->GetBranchTime() <= commandReceivedTime && antiLagSets[ upto ]->IsUserCommandOnly() ) {
			return antiLagSets[ upto ];
		}

		if ( antiLagSets[ upto ]->GetBranchTime() <= fullReceivedTime ) {
			return antiLagSets[ upto ];
		}
	} while ( upto != antiLagUpto );

	return NULL;
}


/*
===============================================================================

	sdAntiLagPlayer
		One player's antilag state

===============================================================================
*/

/*
================
sdAntiLagPlayer::Create
================
*/
void sdAntiLagPlayer::Create() {
	for ( int i = 0; i < MAX_ANTILAG_SETS; i++ ) {
		antiLagSets[ i ] = &playerSets[ i ];
	}
}

/*
================
sdAntiLagPlayer::Destroy
================
*/
void sdAntiLagPlayer::Destroy() {
	for ( int i = 0; i < MAX_ANTILAG_SETS; i++ ) {
		antiLagSets[ i ] = NULL;
	}
}

/*
================
sdAntiLagPlayer::Trace
================
*/
void sdAntiLagPlayer::Trace( trace_t& result, const idVec3& start, const idVec3& end, int shooterIndex ) {
	result.fraction = 1.0f;

	sdAntiLagSet* set = GetAntiLagSet( shooterIndex );
	idPlayer* player = self->Cast< idPlayer >();

	if ( player != NULL ) {
		// default the last hit origin to the real origin
		sdAntiLagManager::GetInstance().SetLastTraceHitOrigin( player->entityNumber,  player->GetPhysics()->GetOrigin() );
	}

	bool fallback = false;
	if ( set == NULL ) {
		fallback = true;
	}

	if ( player != NULL && player->GetProxyEntity() != NULL ) {
		// in a proxy entity - use the real position
		fallback = true;
	}

	if ( fallback ) {
		if ( player != NULL ) {
			idClipModel* shotClip = player->GetPhysics()->GetClipModel( 1 );
			gameLocal.clip.TranslationModel( result, start, end, NULL, mat3_identity, -1, shotClip, shotClip->GetOrigin(), mat3_identity );
		}
		return;
	}

	// this one
	idClipModel* model = sdAntiLagManager::GetInstance().GetModelForBounds( set->GetEntityBounds() );
	assert( model != NULL );

	// try a few times before & after the real time
	int startTime = gameLocal.time - gameLocal.msec * si_antiLagForgiving.GetInteger();
	int endTime = gameLocal.time + gameLocal.msec * si_antiLagForgiving.GetInteger();

	for ( int testTime = startTime; testTime <= endTime; testTime += gameLocal.msec ) {
		const idVec3* predictedClientPosition = set->GetPointForTime( testTime );
		if ( predictedClientPosition != NULL ) {
			gameLocal.clip.TranslationModel( result, start, end, NULL, mat3_identity, -1, model, *predictedClientPosition, mat3_identity );

			if ( g_drawAntiLagHits.GetBool() ) {
				if ( result.fraction < 1.0f ) {
					gameRenderWorld->DebugBounds( colorGreen, set->GetEntityBounds(), *predictedClientPosition, mat3_identity, 20000 );
					gameRenderWorld->DebugArrow( colorGreen, start, end, 8, 20000 );
//					gameLocal.Printf( "Hit predicted\n" );
				} else {
//					gameRenderWorld->DebugBounds( colorBlue, set->GetEntityBounds(), *predictedClientPosition, mat3_identity, 20000 );
//					gameRenderWorld->DebugArrow( colorBlue, start, end, 8, 20000 );
				}
			}

			if ( result.fraction < 1.0f ) {
				sdAntiLagManager::GetInstance().SetLastTraceHitOrigin( player->entityNumber,  *predictedClientPosition );
				break;
			}
		}
	}

	if ( result.fraction == 1.0f ) {
		// didn't hit - try tracing against the "maybe" position
		const idVec3& maybePosition = clientEstimates[ shooterIndex ].GetLastReturned();
		gameLocal.clip.TranslationModel( result, start, end, NULL, mat3_identity, -1, model, maybePosition, mat3_identity );

		if ( g_drawAntiLagHits.GetBool() ) {
			if ( result.fraction < 1.0f ) {
				gameRenderWorld->DebugBounds( colorCyan, set->GetEntityBounds(), maybePosition, mat3_identity, 20000 );
				gameRenderWorld->DebugArrow( colorCyan, start, end, 8, 20000 );
//				gameLocal.Printf( "Hit decayed\n" );
			} else {
//				gameRenderWorld->DebugBounds( colorPurple, set->GetEntityBounds(), maybePosition, mat3_identity, 20000 );
//				gameRenderWorld->DebugArrow( colorPurple, start, end, 8, 20000 );
			}
		}

		if ( result.fraction < 1.0f ) {
			sdAntiLagManager::GetInstance().SetLastTraceHitOrigin( player->entityNumber,  maybePosition );
		}
	}
}



/*
===============================================================================

	sdAntiLagManagerLocal
		Manages the antilag

===============================================================================
*/

#ifdef ANTILAG_LOGGING

enum antilagRecordType_t {
	ANTILAG_BOUNDS			= 0,
	ANTILAG_TRACE,
	ANTILAG_RESET,
	ANTILAG_CREATEBRANCH,
	ANTILAG_SNAPSHOT,
};

typedef struct {
	int		time;
	int		clientNum;
} antilagResetRecord_t;

typedef struct {
	int					clientNum;
	idVec3				origin;
	idVec3				alOrigin;
	idVec3				velocity;
	float				aorDistSqr;

	int					lastMarker;
	int					lastUserCommand;
	int					lastUserCommandDelay;


	// TWTODO: lots more here!!
	// enough to emulate the antilag fully!
	bool				isSpectating;
	bool				hasProxy;
	int					currentPrediction;

	int					stance;
	bool				onGround;

	idVec3				gravity;
	idVec3				groundNormal;

	waterLevel_t		waterLevel;

	float				walkSpeedFwd;
	float				walkSpeedBack;
	float				walkSpeedSide;

	idAngles			viewAngles;
	float				playerSpeed;

	int					cmdForwardMove;
	int					cmdRightMove;	
} antilagSnapshotRecord_t;

typedef struct {
	int		clientNum;
	int		lastSnapshot;
	float	aorDistSqr;
	idVec3	origin;
	idVec3	physicsOrigin;
} antilagClientSnapshotRecord_t;

typedef struct {
	int		time;
	int		clientNum;
	int		targetNum;
	bool	userCommandOnly;
} antilagCreateRecord_t;

typedef struct {
	idVec3	mins;
	idVec3	maxs;
} antilagBoundsRecord_t;

typedef struct {
	int		time;
	int		shooter;
	idVec3	start;
	idVec3	end;
} antilagTraceRecord_t;

#endif

/*
================
sdAntiLagManagerLocal::sdAntiLagManagerLocal
================
*/
sdAntiLagManagerLocal::sdAntiLagManagerLocal() {
#ifdef ANTILAG_LOGGING
	logFile = NULL;
	clientLogFile = NULL;
#endif
}

/*
================
sdAntiLagManagerLocal::ResetForPlayer
================
*/
void sdAntiLagManagerLocal::ResetForPlayer( const idPlayer* self ) {
	if ( gameLocal.isClient ) {
		return;
	}

	assert( self != NULL );
	assert( self->entityNumber >= 0 && self->entityNumber < MAX_CLIENTS );
	players[ self->entityNumber ].Init( self );

#ifdef ANTILAG_LOGGING
	if ( logFile != NULL ) {
		antilagResetRecord_t reset;
		reset.time			= gameLocal.time;
		reset.clientNum		= self->entityNumber;
		logFile->WriteInt( ANTILAG_RESET );
		logFile->Write( &reset, sizeof( reset ) );
	}
#endif
}

/*
================
sdAntiLagManagerLocal::Think
================
*/
void sdAntiLagManagerLocal::Think() {
	if ( gameLocal.msec == 0 ) {
		return;
	}

	if ( gameLocal.isClient ) {
#ifdef ANTILAG_LOGGING
		int numToWrite = 0;
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player != NULL ) {
				numToWrite++;
			}
		}

		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_ANTILAGDEBUG );
		msg.WriteLong( ANTILAG_SNAPSHOT );
		msg.WriteLong( gameLocal.time );
		msg.WriteLong( numToWrite );
		
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player == NULL ) {
				continue;
			}

			antilagClientSnapshotRecord_t record;
			record.clientNum = i;
			record.lastSnapshot = player->GetPlayerPhysics().GetLastSnapshotTime();
			record.origin = player->GetLastPredictionErrorDecayOrigin();
			record.aorDistSqr = player->aorDistanceSqr;
			record.physicsOrigin = player->GetPlayerPhysics().GetOrigin();

			msg.WriteData( &record, sizeof( record ) );
		}

		msg.Send();
#endif
		return;
	}

#ifdef ANTILAG_LOGGING
	if ( logFile != NULL ) {
		int numToWrite = 0;
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player != NULL ) {
				numToWrite++;
			}
		}

		logFile->WriteInt( ANTILAG_SNAPSHOT );
		logFile->WriteInt( gameLocal.time );
		logFile->WriteInt( numToWrite );
	}
#endif

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		players[ i ].Update();

		lastTraceHitOrigin[ i ] = player->GetPhysics()->GetOrigin();

#ifdef ANTILAG_LOGGING
		if ( logFile != NULL ) {
			antilagSnapshotRecord_t record;
			record.clientNum = i;
			record.origin = player->GetPhysics()->GetOrigin();
			record.alOrigin = players[ i ].GetClientEstimate( 1 );
			record.aorDistSqr = players[ i ].antiLagDistances[ 1 ];
			record.velocity = player->GetPhysics()->GetLinearVelocity();
			record.isSpectating = player->IsSpectating();
			record.hasProxy = player->GetProxyEntity() != NULL;
			record.currentPrediction = networkSystem->ServerGetClientPrediction( i );
			record.stance = player->IsProne() ? 2 : ( player->IsCrouching() ? 1 : 0 );
			record.onGround = player->GetPhysics()->HasGroundContacts();
			record.gravity = player->GetPlayerPhysics().GetGravity();
			record.groundNormal = player->GetPlayerPhysics().GetGroundNormal();
			record.waterLevel = player->GetPlayerPhysics().GetWaterLevel();

			record.walkSpeedFwd = player->GetPlayerPhysics().GetWalkSpeedFwd();
			record.walkSpeedBack = player->GetPlayerPhysics().GetWalkSpeedBack();
			record.walkSpeedSide = player->GetPlayerPhysics().GetWalkSpeedSide();

			record.viewAngles = player->GetPlayerPhysics().GetViewAngles();
			record.playerSpeed = player->GetPlayerPhysics().GetCurrentAimSpeed();

			record.cmdForwardMove = player->usercmd.forwardmove;
			record.cmdRightMove = player->usercmd.rightmove;

			// client's network statistics
			const clientNetworkInfo_t& info = gameLocal.GetNetworkInfo( 1 );
			record.lastMarker			= info.lastMarker[ 0 ];
			record.lastUserCommand		= info.lastUserCommand[ 0 ];
			record.lastUserCommandDelay	= info.lastUserCommandDelay[ 0 ];

			logFile->Write( &record, sizeof( record ) );
		}
#endif
	}
}

/*
================
sdAntiLagManagerLocal::CreateBranch
================
*/
void sdAntiLagManagerLocal::CreateBranch( const idPlayer* self ) {
	if ( gameLocal.isClient ) {
		return;
	}

	idPlayer* snapShotTarget = gameLocal.GetSnapshotClient();
	if ( snapShotTarget == NULL ) {
		return;
	}

	assert( self != NULL );
	assert( self->entityNumber >= 0 && self->entityNumber < MAX_CLIENTS );
	players[ self->entityNumber ].CreateBranch( snapShotTarget->entityNumber );

#ifdef ANTILAG_LOGGING
	if ( logFile != NULL ) {
		sdAntiLagSet& baseSet = players[ self->entityNumber ].GetMostRecentBranch();
		sdAntiLagSetPlayer& set = static_cast< sdAntiLagSetPlayer& >( baseSet );

		antilagCreateRecord_t	create;
		create.time				= gameLocal.time;
		create.clientNum		= self->entityNumber;
		create.targetNum		= snapShotTarget->entityNumber;
		create.userCommandOnly	= false;

		logFile->WriteInt( ANTILAG_CREATEBRANCH );
		logFile->Write( &create, sizeof( create ) );
	}
#endif
}


/*
================
sdAntiLagManagerLocal::CreateUserCommandBranch
================
*/
void sdAntiLagManagerLocal::CreateUserCommandBranch( const idPlayer* self ) {
	if ( gameLocal.isClient ) {
		return;
	}

	idPlayer* snapShotTarget = gameLocal.GetSnapshotClient();
	if ( snapShotTarget == NULL ) {
		return;
	}

	assert( self != NULL );
	assert( self->entityNumber >= 0 && self->entityNumber < MAX_CLIENTS );
	players[ self->entityNumber ].CreateUserCommandBranch( snapShotTarget->entityNumber );

#ifdef ANTILAG_LOGGING
	if ( logFile != NULL ) {
		sdAntiLagSet& baseSet = players[ self->entityNumber ].GetMostRecentBranch();
		sdAntiLagSetPlayer& set = static_cast< sdAntiLagSetPlayer& >( baseSet );

		antilagCreateRecord_t	create;
		create.time				= gameLocal.time;
		create.clientNum		= self->entityNumber;
		create.targetNum		= snapShotTarget->entityNumber;
		create.userCommandOnly	= true;

		logFile->WriteInt( ANTILAG_CREATEBRANCH );
		logFile->Write( &create, sizeof( create ) );
	}
#endif
}

/*
================
sdAntiLagManagerLocal::Trace
================
*/
void sdAntiLagManagerLocal::Trace( trace_t& result, const idVec3& start, const idVec3& end, int mask, idPlayer* clientShooting, idEntity* ignore ) {
	result.fraction = 1.0f;
	
#ifdef ANTILAG_LOGGING
	if ( gameLocal.isClient ) {
		if ( clientShooting == gameLocal.GetLocalPlayer() ) {
			SendTrace( start, end );
		}
	} else {
		RecordServerTrace( result, start, end, clientShooting );
	}
#endif

	if ( gameLocal.isClient || !si_antiLag.GetBool() || clientShooting == NULL ) {
		// drop through to default
		gameLocal.clip.TracePoint( result, start, end, mask, ignore );
		return;
	}

	// use client prediction to estimate which branch/es the client may have taken
	int clientPrediction = networkSystem->ServerGetClientPrediction( clientShooting->entityNumber );
	if ( clientPrediction <= gameLocal.msec ) {
		// local client
		gameLocal.clip.TracePoint( result, start, end, mask, ignore );
		return;
	}


	// k, really doing antilag.
	if ( si_antiLagOnly.GetBool() ) {
		// ONLY use antilag for shooting at players - disable all the player clipmodels temporarily
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player == NULL ) {
				continue;
			}

			idClipModel* hitBox = player->GetPhysics()->GetClipModel( 1 );
			if ( hitBox == NULL ) {
				continue;
			}

			hitBox->Disable();
		}
	}

	// do the trace
	gameLocal.clip.TracePoint( result, start, end, mask, ignore );

	if ( si_antiLagOnly.GetBool() ) {
		// re-enable all the player clipmodels
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player == NULL ) {
				continue;
			}

			idClipModel* hitBox = player->GetPhysics()->GetClipModel( 1 );
			if ( hitBox == NULL ) {
				continue;
			}

			hitBox->Enable();
		}
	}


	// check against all the players
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( ignore != NULL && ignore->entityNumber == i ) {
			// ignoring this player
			continue;
		}
		if ( clientShooting->entityNumber == i ) {
			// ignoring self
			continue;
		}

		idPlayer* other = gameLocal.GetClient( i );
		if ( other == NULL ) {
			// other isn't in the game
			continue;
		}
		if ( !other->GetPhysics()->GetClipModel( 1 )->GetContents() & mask ) {
			// doesn't touch the mask
			continue;
		}

		if ( !other->CanCollide( ignore, TM_DEFAULT ) || !ignore->CanCollide( other, TM_DEFAULT ) ) {
			// these two can't collide
			continue;
		}

		// rough check against the current total bounds
		idBounds moveBounds = players[ i ].GetBounds();
		// make sure to include the current decayed position
		float halfwidth = pm_bboxwidth.GetFloat() * 0.5f;
		float height = pm_normalheight.GetFloat();
		idBounds playerBBox( idVec3( -halfwidth, -halfwidth, 0.0f ), idVec3( halfwidth, halfwidth, height ) );
		playerBBox.TranslateSelf( players[ i ].GetClientEstimate( clientShooting->entityNumber ) );
		moveBounds.AddBounds( playerBBox );
		if ( !moveBounds.LineIntersection( start, end ) ) {
			// doesn't hit the total bounds
			continue;
		}

		trace_t trace;
		players[ i ].Trace( trace, start, end, clientShooting->entityNumber );
		if ( trace.fraction < result.fraction ) {
			trace.c.entityNum = i;
			trace.c.surfaceColor.Set( 1.0f, 1.0f, 1.0f );
			trace.c.surfaceType = NULL;
			trace.c.material = NULL;
			result = trace;

			if ( g_drawAntiLagHits.GetBool() ) {
				gameRenderWorld->DebugBounds( colorRed, other->GetPhysics()->GetBounds(), other->GetPhysics()->GetOrigin(), mat3_identity, 20000 );
			}
		}
	}
}

/*
================
sdAntiLagManagerLocal::GetAntiLagPlayer
================
*/
sdAntiLagPlayer& sdAntiLagManagerLocal::GetAntiLagPlayer( int index ) {
	assert( index >= 0 && index < MAX_CLIENTS );
	return players[ index ];
}

/*
================
sdAntiLagManagerLocal::OnNewMapLoad
================
*/
void sdAntiLagManagerLocal::OnMapLoad() {
#ifdef ANTILAG_LOGGING
	lastStart.Set( 999999.0f, 999999.0f, 999999.0f );
	lastEnd.Set( 999999.0f, 999999.0f, 999999.0f );
	lastTime = -1;
#endif

	if ( gameLocal.isClient ) {
		return;
	}

	// paranoid:
	OnMapShutdown();

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		players[ i ].Create();
	}


	float halfwidth = pm_bboxwidth.GetFloat() * 0.5f;
	idBounds bounds( idVec3( -halfwidth, -halfwidth, 0.0f ), idVec3( halfwidth, halfwidth, pm_normalheight.GetFloat() ) );

	// create the default player clip models - likely it'll never have to create any others
	// unless pm_bboxwidth etc change

	// normal
	cachedPlayerModels.Append( new idClipModel( idTraceModel( bounds, 8 ), false ) );

	// crouch
	bounds.GetMaxs().z = pm_crouchheight.GetFloat();
	cachedPlayerModels.Append( new idClipModel( idTraceModel( bounds, 8 ), false ) );

	// prone
	bounds.GetMaxs().z = pm_proneheight.GetFloat();
	cachedPlayerModels.Append( new idClipModel( idTraceModel( bounds, 8 ), false ) );

	// dead
	bounds.GetMaxs().z = pm_deadheight.GetFloat();
	cachedPlayerModels.Append( new idClipModel( idTraceModel( bounds, 8 ), false ) );

#ifdef ANTILAG_LOGGING
	if ( si_antiLagLog.GetBool() ) {
		// figure out which log file number to use
		int logFileNum = 1;
		while ( 1 ) {
			if ( !fileSystem->FileExists( va( "antiLagLog_%i.log", logFileNum ) ) ) {
				break;
			}
			logFileNum++;
		}

		logFile = fileSystem->OpenFileWrite( va( "antiLagLog_%i.log", logFileNum ) );
		clientLogFile = fileSystem->OpenFileWrite( va( "antiLagLog_client_%i.log", logFileNum ) );
	} else {
		logFile = NULL;
		clientLogFile = NULL;
	}

	if ( logFile != NULL ) {
		for ( int i = 0; i < 3; i++ ) {
			idBounds bounds = cachedPlayerModels[ i ]->GetBounds();

			antilagBoundsRecord_t boundsR;
			boundsR.mins[ 0 ] = bounds[ 0 ][ 0 ];
			boundsR.mins[ 1 ] = bounds[ 0 ][ 1 ];
			boundsR.mins[ 2 ] = bounds[ 0 ][ 2 ];

			boundsR.maxs[ 0 ] = bounds[ 1 ][ 0 ];
			boundsR.maxs[ 1 ] = bounds[ 1 ][ 1 ];
			boundsR.maxs[ 2 ] = bounds[ 1 ][ 2 ];

			logFile->WriteInt( ANTILAG_BOUNDS );
			logFile->Write( &boundsR, sizeof( boundsR ) );

//			textLogFile->Printf( "BOUNDS %i %.2f %.2f %.2f %.2f %.2f %.2f\n", i,
//								bounds[ 0 ][ 0 ], bounds[ 0 ][ 1 ], bounds[ 0 ][ 2 ],
//								bounds[ 1 ][ 0 ], bounds[ 1 ][ 1 ], bounds[ 1 ][ 2 ] );
		}
	}
#endif

}

/*
================
sdAntiLagManagerLocal::OnMapShutdown
================
*/
void sdAntiLagManagerLocal::OnMapShutdown() {
	if ( gameLocal.isClient ) {
		return;
	}

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		players[ i ].Destroy();
	}

	for ( int i = 0; i < cachedPlayerModels.Num(); i++ ) {
		gameLocal.clip.DeleteClipModel( cachedPlayerModels[ i ] );
	}
	cachedPlayerModels.SetNum( 0, false );

#ifdef ANTILAG_LOGGING
	if ( logFile != NULL ) {
		fileSystem->CloseFile( logFile );
		logFile = NULL;
	}
	if ( clientLogFile != NULL ) {
		fileSystem->CloseFile( clientLogFile );
		clientLogFile = NULL;
	}

#endif
}

/*
================
sdAntiLagManagerLocal::GetModelForBounds
================
*/
idClipModel* sdAntiLagManagerLocal::GetModelForBounds( const idBounds& bounds ) {
	for ( int i = 0; i < cachedPlayerModels.Num(); i++ ) {
		if ( cachedPlayerModels[ i ]->GetBounds().Compare( bounds, 0.5f ) ) {
			return cachedPlayerModels[ i ];
		}
	}
	
	// couldn't find it in the cache - make a new one
	idClipModel* newModel = new idClipModel( idTraceModel( bounds, 8 ), false );
	cachedPlayerModels.Append( newModel );
	return newModel; 
}

/*
================
sdAntiLagManagerLocal::OnNetworkEvent
================
*/
void sdAntiLagManagerLocal::OnNetworkEvent( int clientNum, const idBitMsg& msg ) {

#ifdef ANTILAG_LOGGING
	antilagRecordType_t type = ( antilagRecordType_t )msg.ReadLong();
	if ( type == ANTILAG_TRACE ) {
		// received info from a client about shootin', yall
		// record it!
		int time = msg.ReadLong();
		idVec3 start = msg.ReadVector();
		idVec3 end = msg.ReadVector();

		RecordTrace( time, false, start, end, clientNum );
	} else if ( type == ANTILAG_SNAPSHOT ) {
		int time = msg.ReadLong();
		int num = msg.ReadLong();
		
		if ( clientLogFile != NULL ) {
			clientLogFile->WriteInt( ANTILAG_SNAPSHOT );
			clientLogFile->WriteInt( time );
			clientLogFile->WriteInt( num );
		}

		for ( int i = 0; i < num; i++ ) {
			antilagClientSnapshotRecord_t record;
			msg.ReadData( &record, sizeof( record ) );

			if ( clientLogFile != NULL ) {
				clientLogFile->Write( &record, sizeof( record ) );
			}
		}
	}
#endif
}

#ifdef ANTILAG_LOGGING
/*
================
sdAntiLagManagerLocal::SendTrace
================
*/
void sdAntiLagManagerLocal::SendTrace( const idVec3& start, const idVec3& end ) {
	// if its not different from the last one then don't send it
	if ( lastTime == gameLocal.time && lastStart.Compare( start, idMath::FLT_EPSILON ) && lastEnd.Compare( end, idMath::FLT_EPSILON ) ) {
		return;
	}
	lastTime = gameLocal.time;
	lastStart = start;
	lastEnd = end;

	sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_ANTILAGDEBUG );
	msg.WriteLong( ANTILAG_TRACE );
	msg.WriteLong( gameLocal.time );
	msg.WriteVector( start );
	msg.WriteVector( end );
	msg.Send();
}

/*
================
sdAntiLagManagerLocal::RecordTrace
================
*/
void sdAntiLagManagerLocal::RecordTrace( int time, bool server, const idVec3& start, const idVec3& end, int shooter ) {

	if ( logFile == NULL || clientLogFile == NULL ) {
		return;
	}

	idPlayer* player = gameLocal.GetClient( shooter );
	if ( player == NULL ) {
		return;
	}

	antilagTraceRecord_t preamble;
	preamble.time = time;
	preamble.shooter = shooter;
	preamble.start = start;
	preamble.end = end;

	if ( server ) {
		logFile->WriteInt( ANTILAG_TRACE );
		logFile->Write( &preamble, sizeof( preamble ) );
	} else {
		clientLogFile->WriteInt( ANTILAG_TRACE );
		clientLogFile->Write( &preamble, sizeof( preamble ) );
	}
}

/*
================
sdAntiLagManagerLocal::RecordServerTrace
================
*/
void sdAntiLagManagerLocal::RecordServerTrace( trace_t& result, const idVec3& start, const idVec3& end, idPlayer* clientShooting ) {
	// if its not different from the last one then don't send it
	if ( lastTime == gameLocal.time && lastStart.Compare( start, idMath::FLT_EPSILON ) && lastEnd.Compare( end, idMath::FLT_EPSILON ) ) {
		return;
	}

	lastTime = gameLocal.time;
	lastStart = start;
	lastEnd = end;

	// not passing any hit info from the server in - the tool can figure that out based on all the info given
	RecordTrace( gameLocal.time, true, start, end, clientShooting->entityNumber );
}
#endif
