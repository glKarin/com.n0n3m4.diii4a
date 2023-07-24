// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "PredictionErrorDecay.h"

#ifdef PREDICTION_ERROR_DECAY_DEBUG

idCVar net_debugPredictionError( "net_debugPredictionError", "0", CVAR_BOOL | CVAR_GAME | CVAR_CHEAT, "dump debugging information about the prediction error decay" );
int sdPredictionErrorDecay_Origin::pedDebugCount = 0;
int sdPredictionErrorDecay_Angles::pedDebugCount = 0;

#endif

idCVar net_predictionErrorDecay( "net_predictionErrorDecay", "1", CVAR_BOOL | CVAR_GAME | CVAR_NOCHEAT, "Enable/disable prediction error decay" );
idCVar net_limitApparentVelocity( "net_limitApparentVelocity", "1", CVAR_BOOL | CVAR_GAME | CVAR_NOCHEAT, "limit the apparent velocity of objects in prediction to realistic levels" );
idCVar net_limitApparentMaxLagAllowance( "net_limitApparentMaxLagAllowance", "0.25", CVAR_FLOAT | CVAR_GAME | CVAR_NOCHEAT, "fraction of the current physics speed added to the maximum apparent speed due to client lag" );
idCVar net_limitApparentMaxErrorAllowance( "net_limitApparentMaxErrorAllowance", "0.25", CVAR_FLOAT | CVAR_GAME | CVAR_NOCHEAT, "fraction of the current physics speed added to the maximum apparent speed due to client prediction error" );
idCVar net_limitApparentMinSpeed( "net_limitApparentMinSpeed", "200", CVAR_FLOAT | CVAR_GAME | CVAR_NOCHEAT, "minimum value for maximum apparent speed to reach" );

#define ENABLE_JP_FLOAT_CHECKS
#if defined( ENABLE_JP_FLOAT_CHECKS )
	#undef FLOAT_CHECK_BAD
	#undef VEC_CHECK_BAD

#define FLOAT_CHECK_BAD( x ) \
	( FLOAT_IS_NAN( x ) || FLOAT_IS_INF( x ) || FLOAT_IS_IND( x ) || FLOAT_IS_DENORMAL( x ) )

//	if ( FLOAT_IS_NAN( x ) || FLOAT_IS_INF( x ) || FLOAT_IS_IND( x ) || FLOAT_IS_DENORMAL( x ) ) \
//	{ \
//	gameLocal.Warning( "Bad floating point number in %s: %i", __FILE__, __LINE__ ); \
//	}


	#define VEC_CHECK_BAD( vec ) ( FLOAT_CHECK_BAD( ( vec ).x ) || FLOAT_CHECK_BAD( ( vec ).y ) || FLOAT_CHECK_BAD( ( vec ).z ) )
	#define MAT_CHECK_BAD( m ) ( VEC_CHECK_BAD( m[ 0 ] ) || VEC_CHECK_BAD( m[ 1 ] ) || VEC_CHECK_BAD( m[ 2 ] ) )
#else
	#define MAT_CHECK_BAD( m )
#endif

const float MAX_PREDICTION_TIME_OFFSET = 3.f;

/*
===============================================================================

	Quartic spline utility classes

===============================================================================
*/
class sdQuarticFunction {
public:
	float A, B, C, D, E;

	inline void Construct( float x0, float dx0, float ddx0, float x1, float dx1 ) {
		E = x0;
		D = dx0;
		C = ddx0 * 0.5f;
		B = 4.0f*x1 - 2.0f*C - 3.0f*D - 4.0f*E - dx1;
		A = x1 - B - C - D - E;
	}

	inline float EvaluateX( float t, float t2, float t3, float t4 ) const {
		return A*t4 + B*t3 + C*t2 + D*t + E;
	}
	inline float EvaluateDX( float t, float t2, float t3 ) const {
		return 4.0f*A*t3 + 3.0f*B*t2 + 2.0f*C*t + D;
	}
	inline float EvaluateDDX( float t, float t2 ) const {
		return 12.0f*A*t2 + 6.0f*B*t + 2.0f*C;
	}
};

class sdQuarticSpline {
public:
	sdQuarticFunction x, y, z;

	inline void Construct( const idVec3& x0, const idVec3& dx0, const idVec3& ddx0, const idVec3& x1, const idVec3& dx1 ) {
		x.Construct( x0.x, dx0.x, ddx0.x, x1.x, dx1.x );
		y.Construct( x0.y, dx0.y, ddx0.y, x1.y, dx1.y );
		z.Construct( x0.z, dx0.z, ddx0.z, x1.z, dx1.z );
	}

	inline idVec3 EvaluateX( float t, float t2, float t3, float t4 ) const {
		return idVec3( x.EvaluateX( t, t2, t3, t4 ), y.EvaluateX( t, t2, t3, t4 ), z.EvaluateX( t, t2, t3, t4 ) );
	}
	inline idVec3 EvaluateDX( float t, float t2, float t3 ) const {
		return idVec3( x.EvaluateDX( t, t2, t3 ), y.EvaluateDX( t, t2, t3 ), z.EvaluateDX( t, t2, t3 ) );
	}
	inline idVec3 EvaluateDDX( float t, float t2 ) const {
		return idVec3( x.EvaluateDDX( t, t2 ), y.EvaluateDDX( t, t2 ), z.EvaluateDDX( t, t2 ) );
	}
};

class sdQuarticAngleSpline {
public:
	sdQuarticFunction pitch, yaw, roll;
	idAngles baseAngles;

	inline void Construct( const idAngles& x0, const idAngles& dx0, const idAngles& ddx0, const idAngles& x1, const idAngles& dx1 ) {
		baseAngles = x0;

		// angles wrap around
		idAngles diffAngles = x1 - x0;
		//diffAngles.Normalize180();

		pitch.Construct( ang_zero.pitch, dx0.pitch, ddx0.pitch, diffAngles.pitch, dx1.pitch );
		yaw.Construct( ang_zero.yaw, dx0.yaw, ddx0.yaw, diffAngles.yaw, dx1.yaw );
		roll.Construct( ang_zero.roll, dx0.roll, ddx0.roll, diffAngles.roll, dx1.roll );
	}

	inline idAngles EvaluateX( float t, float t2, float t3, float t4 ) const {
		return idAngles( pitch.EvaluateX( t, t2, t3, t4 ), yaw.EvaluateX( t, t2, t3, t4 ), roll.EvaluateX( t, t2, t3, t4 ) ) + baseAngles;
	}
	inline idAngles EvaluateDX( float t, float t2, float t3 ) const {
		return idAngles( pitch.EvaluateDX( t, t2, t3 ), yaw.EvaluateDX( t, t2, t3 ), roll.EvaluateDX( t, t2, t3 ) );
	}
	inline idAngles EvaluateDDX( float t, float t2 ) const {
		return idAngles( pitch.EvaluateDDX( t, t2 ), yaw.EvaluateDDX( t, t2 ), roll.EvaluateDDX( t, t2 ) );
	}
};



/*
===============================================================================

	sdPredictionErrorDecay_Origin

===============================================================================
*/

/*
===============
sdPredictionErrorDecay_Origin::sdPredictionErrorDecay_Origin
===============
*/
sdPredictionErrorDecay_Origin::sdPredictionErrorDecay_Origin( void ) {
	lastReturnedOrigin.Zero();
	lastReturnedVelocity.Zero();
	lastReturnedAcceleration.Zero();

	networkTime = 0;
	networkOrigin.Zero();

	lastNetworkTime = 0;
	lastNetworkOrigin.Zero();

	lastLastNetworkTime = 0;
	lastLastNetworkOrigin.Zero();

	networkPacketDelay = 0;
	lastUpdateTime = 0;
	lastDecayTime = 0;

	isNew = true;
	owner = NULL;
	
	numDuplicates = 0;
	lastDuplicateTime = 0;
	isStationary = true;

#ifdef PREDICTION_ERROR_DECAY_DEBUG
	debugFile = NULL;
#endif
}

/*
===============
sdPredictionErrorDecay_Origin::~sdPredictionErrorDecay_Origin
===============
*/
sdPredictionErrorDecay_Origin::~sdPredictionErrorDecay_Origin( void ) {
#ifdef PREDICTION_ERROR_DECAY_DEBUG
	if ( debugFile != NULL ) {
		fileSystem->CloseFile( debugFile );
	}
#endif
}

/*
===============
sdPredictionErrorDecay_Origin::Init
===============
*/
void sdPredictionErrorDecay_Origin::Init( idEntity* _owner ) {
	owner = _owner;

#ifdef PREDICTION_ERROR_DECAY_DEBUG
	myIndex = -1;
	if ( net_debugPredictionError.GetBool() && debugFile == NULL && _owner->entityNumber == 0 ) {
		debugFile = fileSystem->OpenFileWrite( va( "ped_origin_%i.csv", pedDebugCount++ ) );
		if ( debugFile != NULL ) {
			myIndex = pedDebugCount - 1;

			debugFile->Printf( "entNum,currentTime,prevTime,networkPacketDelay,timeSinceNewPacket," );
			debugFile->Printf( "matchingTime," );
			debugFile->Printf( "netOrigin.x,netOrigin.y,netOrigin.z,renderOrigin.x,renderOrigin.y,renderOrigin.z," );
			debugFile->Printf( "resultOrigin.x,resultOrigin.y,resultOrigin.z," );
			debugFile->Printf( "futureSpot.x,futureSpot.y,futureSpot.z," );
			debugFile->Printf( "futureVelocity.x,futureVelocity.y,futureVelocity.z," );
			debugFile->Printf( "lastNetworkTime,lastNetworkOrigin.x,lastNetworkOrigin.y,lastNetworkOrigin.z," );
			debugFile->Printf( "lastLastNetworkTime,lastLastNetworkOrigin.x,lastLastNetworkOrigin.y,lastLastNetworkOrigin.z," );
			debugFile->Printf( "\n" );
		}
	}
#endif
}

/*
===============
sdPredictionErrorDecay_Origin::Reset
===============
*/
void sdPredictionErrorDecay_Origin::Reset( const idVec3& newOrigin ) {
	lastReturnedOrigin = newOrigin;
	lastReturnedVelocity.Zero();
	lastReturnedAcceleration.Zero();

	networkTime = gameLocal.time;
	networkOrigin = newOrigin;

	lastNetworkTime = networkTime;
	lastNetworkOrigin = networkOrigin;

	lastLastNetworkTime = lastNetworkTime;
	lastLastNetworkOrigin = lastNetworkOrigin;

	numDuplicates = 0;
	lastDuplicateTime = 0;
	isStationary = true;

	lastDecayTime = gameLocal.time - gameLocal.msec;
}

/*
===============
sdPredictionErrorDecay_Origin::Update
===============
*/
void sdPredictionErrorDecay_Origin::Update( const sdPredictionErrorDecay_Origin::CustomDecayInfo* customInfo ) {
	lastUpdateTime = gameLocal.time;

	bool hasLocalPhysics;
	int prediction;
	int packetSpread;
	idVec3 origin;

	if ( customInfo == NULL ) {
		hasLocalPhysics = !owner->IsPhysicsInhibited();
		prediction = networkSystem->ClientGetPrediction();
		packetSpread = owner->aorPacketSpread;
		origin = owner->GetPhysics()->GetOrigin();
	} else {
		hasLocalPhysics = customInfo->hasLocalPhysics;
		prediction = customInfo->currentPrediction;
		packetSpread = customInfo->packetSpread;
		origin = customInfo->origin;
	}

	if ( hasLocalPhysics ) {
		int timeSinceNewPacket = gameLocal.time - networkTime;
		timeSinceNewPacket -= gameLocal.msec;
		timeSinceNewPacket -= prediction;
		int reAddTime = packetSpread + packetSpread / 5;
		if ( reAddTime < gameLocal.msec * 4 ) {
			reAddTime = gameLocal.msec * 4;
		}

		if ( timeSinceNewPacket > reAddTime ) {
			// insert a new "packet" if its been too long and we're running physics locally
			// its generally close enough that this all averages out
			OnNewInfo( origin, gameLocal.time, customInfo );
		}
	}

#ifdef PREDICTION_ERROR_DECAY_DEBUG
	if ( net_debugPredictionError.GetBool() && debugFile == NULL && owner->entityNumber == 0 ) {
		debugFile = fileSystem->OpenFileWrite( va( "ped_origin_%i.csv", pedDebugCount++ ) );
		if ( debugFile != NULL ) {
			myIndex = pedDebugCount - 1;

			debugFile->Printf( "entNum,currentTime,prevTime,networkPacketDelay,timeSinceNewPacket," );
			debugFile->Printf( "matchingTime," );
			debugFile->Printf( "netOrigin.x,netOrigin.y,netOrigin.z,renderOrigin.x,renderOrigin.y,renderOrigin.z," );
			debugFile->Printf( "resultOrigin.x,resultOrigin.y,resultOrigin.z," );
			debugFile->Printf( "futureSpot.x,futureSpot.y,futureSpot.z," );
			debugFile->Printf( "futureVelocity.x,futureVelocity.y,futureVelocity.z," );
			debugFile->Printf( "lastNetworkTime,lastNetworkOrigin.x,lastNetworkOrigin.y,lastNetworkOrigin.z," );
			debugFile->Printf( "lastLastNetworkTime,lastLastNetworkOrigin.x,lastLastNetworkOrigin.y,lastLastNetworkOrigin.z," );
			debugFile->Printf( "\n" );
		}
	}
	if ( !net_debugPredictionError.GetBool() && debugFile != NULL ) {
		fileSystem->CloseFile( debugFile );
		debugFile = NULL;
	}
#endif
}

/*
===============
sdPredictionErrorDecay_Origin::LimitApparentVelocity
===============
*/
float sdPredictionErrorDecay_Origin::LimitApparentVelocity( const idVec3& oldOrigin, const idVec3& idealNewOrigin, idVec3& newOrigin, const idVec3& physicsVelocity, int currentPrediction ) {
	idVec3 apparentVelocity = ( newOrigin - oldOrigin ) / MS2SEC( gameLocal.msec );
	float apparentSpeedSqr = apparentVelocity.LengthSqr();

	float error = ( idealNewOrigin - oldOrigin ).Length();
	float localSpeed = physicsVelocity.Length();
	float speedAllowanceDueToLag = Min( localSpeed * net_limitApparentMaxLagAllowance.GetFloat(), 0.5f * currentPrediction * localSpeed / 350.0f );
	float speedAllowanceDueToError = Min( localSpeed * net_limitApparentMaxErrorAllowance.GetFloat(), 2.0f * error * localSpeed / 350.0f );
	float maxSpeed = localSpeed + speedAllowanceDueToLag + speedAllowanceDueToError;
	maxSpeed = Max( net_limitApparentMinSpeed.GetFloat(), Min( localSpeed * 1.25f, maxSpeed ) );

	if ( apparentSpeedSqr > Square( maxSpeed ) && apparentSpeedSqr > idMath::FLT_EPSILON ) {
		// apparent velocity is too high
		// try to tone that back linearly
		float apparentSpeed = idMath::Sqrt( apparentSpeedSqr );
		float lerpValue = maxSpeed / apparentSpeed;

		newOrigin = Lerp( oldOrigin, newOrigin, lerpValue );
		return lerpValue;
	}

	return 1.0f;
}

/*
===============
sdPredictionErrorDecay_Origin::Decay
===============
*/
void sdPredictionErrorDecay_Origin::Decay( idVec3& renderOrigin, const sdPredictionErrorDecay_Origin::CustomDecayInfo* customInfo ) {
	//
	// TWTODO: This whole function needs a good old cleanup!!
	//
	if ( IsNew() ) {
		return;
	}

	assert( owner );

	if ( !net_predictionErrorDecay.GetBool() ) {
		Reset( renderOrigin );
		return;
	}

	if ( gameLocal.time <= lastDecayTime || !gameLocal.isNewFrame || gameLocal.msec == 0 || !owner->fl.allowPredictionErrorDecay ) {
		renderOrigin = lastReturnedOrigin;
		return;
	}

	idVec3 predictedOrigin = renderOrigin;

	//
	// Collect information to be used
	//
	bool	hasLocalPhysics;
	bool	isPlayer;
	int		currentPrediction;
	int		packetSpread;
	bool	boxDecayClip;
	bool	pointDecayClip;
	bool	heightMapDecayClip;
	idVec3	limitVelocity;
	bool	hasGroundContacts;
	float	physicsCutoffSqr;
	float	ownerAorDistSqr;

	if ( customInfo == NULL ) {
		hasLocalPhysics = !owner->IsPhysicsInhibited();
		isPlayer = owner->IsType( idPlayer::Type );
		currentPrediction = networkSystem->ClientGetPrediction();
		packetSpread = owner->aorPacketSpread;
		boxDecayClip = ( owner->aorFlags & AOR_BOX_DECAY_CLIP ) != 0;
		pointDecayClip = ( owner->aorFlags & AOR_POINT_DECAY_CLIP ) != 0;
		heightMapDecayClip = ( owner->aorFlags & AOR_HEIGHTMAP_DECAY_CLIP ) != 0;
		limitVelocity = owner->GetPhysics()->GetLinearVelocity();
		hasGroundContacts = owner->GetPhysics()->HasGroundContacts();
		physicsCutoffSqr = 1.0f;
		ownerAorDistSqr = 0.0f;
		if ( owner->aorLayout != NULL ) {
			physicsCutoffSqr = owner->aorLayout->GetPhysicsCutoffSqr();
			ownerAorDistSqr = owner->aorDistanceSqr;
		}
	} else {
		// using custom info
		hasLocalPhysics		= customInfo->hasLocalPhysics;
		isPlayer			= customInfo->isPlayer;
		currentPrediction	= customInfo->currentPrediction;
		packetSpread		= customInfo->packetSpread;
		boxDecayClip		= customInfo->boxDecayClip;
		pointDecayClip		= customInfo->pointDecayClip;
		heightMapDecayClip	= customInfo->heightMapDecayClip;
		limitVelocity		= customInfo->limitVelocity;
		hasGroundContacts	= customInfo->hasGroundContacts;
		physicsCutoffSqr	= customInfo->physicsCutoffSqr;
		ownerAorDistSqr		= customInfo->ownerAorDistSqr;
	}

	if ( networkPacketDelay > 0 && networkTime != lastNetworkTime ) {

		// TODO: clean up this timing stuff!
		int timeSinceNewPacket = gameLocal.time - networkTime;
		if ( timeSinceNewPacket < gameLocal.msec ) {
			timeSinceNewPacket = gameLocal.msec;
		}

		int lerpDuration = packetSpread*2;
		if ( lerpDuration < gameLocal.msec ) {
			lerpDuration = gameLocal.msec;
		}

		int matchingTimeInt = gameLocal.msec * 3;
		if ( matchingTimeInt < lerpDuration ) {
			matchingTimeInt = lerpDuration;
		}

		float matchingTime = MS2SEC( matchingTimeInt );

		//
		// Predict a position and velocity for the future
		//
		idVec3 futureVelocity;
		int timeOffset = timeSinceNewPacket;

		idVec3 hasPhysicsVelocity = vec3_origin;
		idVec3 noPhysicsVelocity = vec3_origin;

		//
		// estimate velocity from local physics
		if ( hasLocalPhysics ) {
			if ( !isStationary && timeSinceNewPacket + ( networkTime - lastNetworkTime ) > 0 ) {
				if ( isPlayer && timeSinceNewPacket > 0 ) {
					hasPhysicsVelocity = ( predictedOrigin - networkOrigin ) / MS2SEC( timeSinceNewPacket );
				} else {
					// velocity averaged over last couple of packets plus the prediction so far
					hasPhysicsVelocity = ( predictedOrigin - lastNetworkOrigin ) / MS2SEC( timeSinceNewPacket + ( networkTime - lastNetworkTime )  );
				}
			} else {
				hasPhysicsVelocity = ( predictedOrigin - networkOrigin ) / MS2SEC( gameLocal.msec * 4 );
				timeOffset = gameLocal.msec * 5;
			}
		}

		//
		// estimate velocity from what has been sent on the network
		{
			idVec3 networkVelocity = ( networkOrigin - lastNetworkOrigin ) / MS2SEC( networkTime - lastNetworkTime );

			if ( lastNetworkTime != lastLastNetworkTime ) {
				idVec3 lastNetworkVelocity = ( lastNetworkOrigin - lastLastNetworkOrigin ) / MS2SEC( lastNetworkTime - lastLastNetworkTime );

				float recentBiasValue = idMath::ClampFloat( 0.0f, 1.0f, timeSinceNewPacket / ( lerpDuration * 0.5f ) );
				noPhysicsVelocity = Lerp( lastNetworkVelocity, networkVelocity, 0.5f + 0.5f * recentBiasValue );

				idVec3 networkAcceleration = ( networkVelocity - lastNetworkVelocity ) / MS2SEC( 0.5f * ( ( networkTime + lastNetworkTime ) - ( lastNetworkTime + lastLastNetworkTime )  ) );

				// decay networkAcceleration based on how long its been since the acceleration took place
				networkAcceleration *= 1.0f - recentBiasValue;

				noPhysicsVelocity += networkAcceleration * MS2SEC( timeSinceNewPacket );
			} else {
				noPhysicsVelocity = networkVelocity;
			}

			// scale back a bit for less overshoot
			noPhysicsVelocity *= 0.6f;
			
			// decay velocity down if timeSinceNewPacket is getting big
			int velocityDecayStart = lerpDuration + lerpDuration / 2 + currentPrediction;
			if ( timeSinceNewPacket > velocityDecayStart ) {
				float scale = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, ( timeSinceNewPacket - velocityDecayStart ) / ( lerpDuration * 0.5f ) );
				noPhysicsVelocity *= scale;
			}
		}


		// blend the two together based on distance to the physics cutoff point
		// to smooth the change between prediction modes
		float lerpToPhysicsCutoff;
		if ( owner->aorLayout != NULL ) {
			if ( physicsCutoffSqr > idMath::FLT_EPSILON ) {
				lerpToPhysicsCutoff = idMath::ClampFloat( 0.0f, 1.0f, ownerAorDistSqr / physicsCutoffSqr );
				futureVelocity = Lerp( hasPhysicsVelocity, noPhysicsVelocity, lerpToPhysicsCutoff );
			} else {
				lerpToPhysicsCutoff = 1.0f;
				futureVelocity = noPhysicsVelocity;
			}
		} else {
			if ( hasLocalPhysics ) {
				lerpToPhysicsCutoff = 0.0f;
				futureVelocity = hasPhysicsVelocity;
			} else {
				lerpToPhysicsCutoff = 1.0f;
				futureVelocity = noPhysicsVelocity;
			}
		}

		idVec3 futureOrigin = networkOrigin + futureVelocity * ( matchingTime + MS2SEC( timeOffset ) );

		if ( !hasLocalPhysics && ( boxDecayClip || pointDecayClip || heightMapDecayClip ) ) {
			// crude clipping to try to stop it dipping through the ground
			idBounds bounds = owner->GetPhysics()->GetBounds();
			idVec3 center = bounds.GetCenter(); 
			idVec3 size = bounds.GetSize();
			for ( int i = 0; i < 3; i++ ) {
				if ( size[ i ] > 48.0f ) {
					size[ i ] -= 16.0f;
				}
			}
			
			bounds = idBounds( center - size * 0.5f, center + size * 0.5f );

			idVec3 direction = futureOrigin - predictedOrigin;
			float distance = direction.Normalize();
			float sizeInDirection = idMath::Fabs( size * direction );

			// limit the length of the trace
			if ( distance > 512.0f ) {
				distance = 512.0f;
			}

			idVec3 rotatedCenter = center * owner->GetPhysics()->GetAxis();
			idVec3 start = rotatedCenter + predictedOrigin;
			idVec3 end = start + distance * direction;

			// TWTODO: use proper contents masks requested from the physics
			trace_t trace;
			if ( boxDecayClip ) {
				gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS trace, start, end, bounds, owner->GetPhysics()->GetAxis(), CONTENTS_SOLID | CONTENTS_VEHICLECLIP, owner );
			} else if ( pointDecayClip ) { 
				end += direction * sizeInDirection * 2.0f;
				gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, start, end, CONTENTS_SOLID | CONTENTS_VEHICLECLIP, owner );
			} else {
				end += direction * sizeInDirection * 2.0f;
				trace.fraction = 1.0f;
				trace.endpos = end;

				const sdPlayZone* playZoneHeight = gameLocal.GetPlayZone( start, sdPlayZone::PZF_HEIGHTMAP );
				if ( playZoneHeight != NULL ) {
					const sdHeightMapInstance& heightMap = playZoneHeight->GetHeightMap();
					if ( heightMap.IsValid() ) {
						trace.fraction = heightMap.TracePoint( start, end, trace.endpos );
						trace.c.normal.Set( 0.0f, 0.0f, 1.0f );
					}
				}
			}

			if ( trace.fraction < 1.0f ) {
				// scale back the velocity in the direction of the bump too
				futureVelocity -= ( futureVelocity * trace.c.normal * 0.95f ) * trace.c.normal;
				futureOrigin = networkOrigin + futureVelocity * ( matchingTime + MS2SEC( timeOffset ) );
			}
		}

		//
		// Construct & evaluate splines for output
		//
		float t = MS2SEC( gameLocal.msec ) / matchingTime;
		float spreadBasedScaleUp = Lerp( 1.0f, 1.5f, ( 1000 - lerpDuration + 150 ) / 1000.0f );
		t *= spreadBasedScaleUp;
		if ( t > 0.5f ) {
			t = 0.5f;
		}
		float t2 = t*t;
		float t3 = t2*t;
		float t4 = t2*t2;

		sdQuarticSpline spline;
		spline.Construct( lastReturnedOrigin, lastReturnedVelocity, lastReturnedAcceleration, futureOrigin, futureVelocity );
		idVec3 tweenOrigin = spline.EvaluateX( t, t2, t3, t4 );
		idVec3 tweenVelocity = spline.EvaluateDX( t, t2, t3 );
		idVec3 tweenAcceleration = spline.EvaluateDDX( t, t2 );

		//
		// Try to prevent the apparent velocity being unrealistic
		//
		if ( net_limitApparentVelocity.GetBool() ) {
			float originalTweenOriginZ = tweenOrigin.z;
			float originalTweenVelocityZ = tweenVelocity.z;
			float originalTweenAccelerationZ = tweenAcceleration.z;

			// since this does a bunch of masking to cover up jerking, can blend in more of what is REALLY happening
			tweenOrigin = Lerp( predictedOrigin, tweenOrigin, lerpToPhysicsCutoff );

			float lerpValue = LimitApparentVelocity( lastReturnedOrigin, predictedOrigin, tweenOrigin, 
													limitVelocity, currentPrediction );
			tweenVelocity = Lerp( lastReturnedVelocity, tweenVelocity, lerpValue );
			tweenAcceleration = Lerp( lastReturnedAcceleration, tweenAcceleration, lerpValue );

			if ( isPlayer ) {
				// HACK: players don't limit z velocity as much - looks really weird when they jump
				tweenOrigin.z = Lerp( tweenOrigin.z, originalTweenOriginZ, 0.75f );
				tweenVelocity.z = Lerp( tweenVelocity.z, originalTweenVelocityZ, 0.75f );
				tweenAcceleration.z = Lerp( tweenAcceleration.z, originalTweenAccelerationZ, 0.75f );
			}
		}

		if ( hasLocalPhysics ) {
			if ( hasGroundContacts ) {
				if ( tweenOrigin.z < predictedOrigin.z ) {
					tweenOrigin.z = predictedOrigin.z;
				}
			}
		}

		tweenOrigin.FixDenormals();
		tweenVelocity.FixDenormals();
		tweenAcceleration.FixDenormals();

		// cache output
		renderOrigin = tweenOrigin;
		lastReturnedOrigin = tweenOrigin;
		lastReturnedVelocity = tweenVelocity;
		lastReturnedAcceleration = tweenAcceleration;

		//
		// Debugging info
		//
	#ifdef PREDICTION_ERROR_DECAY_DEBUG
		if ( debugFile != NULL ) {
			if ( owner != NULL ) {
				debugFile->Printf( "%i,", owner->entityNumber );
			} else {
				debugFile->Printf( "-1," );
			}

			// timing
			debugFile->Printf( "%i,%i,%i,%i,", gameLocal.time, networkTime, networkPacketDelay, timeSinceNewPacket );
			debugFile->Printf( "%.2f,", matchingTime );

			// origin
			debugFile->Printf( "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", networkOrigin.x, networkOrigin.y, networkOrigin.z,
																predictedOrigin.x, predictedOrigin.y, predictedOrigin.z );
			debugFile->Printf( "%.2f,%.2f,%.2f,", renderOrigin.x, renderOrigin.y, renderOrigin.z );
			debugFile->Printf( "%.2f,%.2f,%.2f,", futureOrigin.x, futureOrigin.y, futureOrigin.z );
			debugFile->Printf( "%.2f,%.2f,%.2f,", futureVelocity.x, futureVelocity.y, futureVelocity.z );

			debugFile->Printf( "%i,%.2f,%.2f,%.2f,", lastNetworkTime, lastNetworkOrigin.x, lastNetworkOrigin.y, lastNetworkOrigin.z );
			debugFile->Printf( "%i,%.2f,%.2f,%.2f,", lastLastNetworkTime, lastLastNetworkOrigin.x, lastLastNetworkOrigin.y, lastLastNetworkOrigin.z );

			debugFile->Printf( "\n" );
		}
	#endif
	} else {
		// do nothing - it hasn't calibrated itself appropriately yet
		lastReturnedOrigin = renderOrigin;
	}

	lastDecayTime = gameLocal.time;

	// gracefully handle bad values
	// TWTODO: Proper fix!
	if ( VEC_CHECK_BAD( renderOrigin ) ) {
		gameLocal.Warning( "sdPredictionErrorDecay_Origin::Decay - Bad float!" );
		assert( false );
		renderOrigin = predictedOrigin;
		Reset( renderOrigin );
	}
}

/*
===============
sdPredictionErrorDecay_Origin::OnNewInfo
===============
*/
void sdPredictionErrorDecay_Origin::OnNewInfo( const idVec3& serverOrigin, int& time, const CustomDecayInfo* customInfo ) {
	assert( owner );

	if ( !net_predictionErrorDecay.GetBool() ) {
		return;
	}

	int newDelay = gameLocal.time - networkTime;

	int packetSpread = customInfo == NULL ? owner->aorPacketSpread : customInfo->packetSpread;
	bool physicsInhibited = customInfo == NULL ? owner->IsPhysicsInhibited() : !customInfo->hasLocalPhysics;
	if ( newDelay < packetSpread ) {
		if ( physicsInhibited ) {
			networkOrigin = serverOrigin;
			networkTime = time;
		}
		return;
	}

	// see if it falls beneath the interesting threshold
	float diffSqr = ( serverOrigin - networkOrigin ).LengthSqr();
	if ( diffSqr < Square( 0.5f ) ) {
		if ( diffSqr < idMath::FLT_EPSILON && time > lastDuplicateTime ) {
			// its basically identical to the last origin!
			numDuplicates++;
			lastDuplicateTime = time;
			if ( numDuplicates >= 2 ) {
				// 2 duplicates - we must be stationary
				isStationary = true;
			}
		}
		networkOrigin = serverOrigin;
		networkTime = time;
		return;
	}

	// see if the distance is too far - ie, looks like its a respawn or teleport that hasn't been
	// caught properly by a reset
	idVec3 velocity = owner->GetPhysics()->GetLinearVelocity();
	if ( customInfo != NULL ) {
		velocity = customInfo->limitVelocity;
	}

	float thresholdDistanceSqr = velocity.LengthSqr() * Square( 4.0f );
	thresholdDistanceSqr = Max( thresholdDistanceSqr, Square( 1024.0f ) );
	float returnedDiffSqr = ( serverOrigin - lastReturnedOrigin ).LengthSqr();
	if ( returnedDiffSqr > thresholdDistanceSqr ) {
		Reset( serverOrigin );
		return;
	}

	if ( isStationary ) {
		newDelay = packetSpread;
		isStationary = false;
		numDuplicates = 0;
	}

	if ( newDelay > 0 ) {
//		networkPacketDelay = ( newDelay + networkPacketDelay ) * 0.5f;
		networkPacketDelay = newDelay;
	}

	// don't let the delay get too big
	if ( networkPacketDelay > 1000 ) {
		networkPacketDelay = 1000;
	}

	if ( isNew ) {
		networkPacketDelay = 0;
		isNew = false;

		networkTime = time;
		networkOrigin = serverOrigin;

		lastNetworkTime = networkTime;
		lastNetworkOrigin = networkOrigin;

		lastLastNetworkTime = lastNetworkTime;
		lastLastNetworkOrigin = lastNetworkOrigin;
	} else {
		lastLastNetworkTime = lastNetworkTime;
		lastLastNetworkOrigin = lastNetworkOrigin;

		lastNetworkTime = networkTime;
		lastNetworkOrigin = networkOrigin;

		networkTime = time;
		networkOrigin = serverOrigin;
	}
}

/*
===============
sdPredictionErrorDecay_Origin::NeedsUpdate
===============
*/
bool sdPredictionErrorDecay_Origin::NeedsUpdate( void ) {
	return gameLocal.time >= lastDecayTime && !IsNew() && ( ( gameLocal.time - networkTime ) < SEC2MS( MAX_PREDICTION_TIME_OFFSET ) );
}






/*
===============================================================================

	sdPredictionErrorDecay_Angles

===============================================================================
*/



/*
===============
sdPredictionErrorDecay_Angles::sdPredictionErrorDecay_Angles
===============
*/
sdPredictionErrorDecay_Angles::sdPredictionErrorDecay_Angles( void ) {
	lastReturnedAngles.Zero();
	lastReturnedAngVelocity.Zero();
	lastReturnedAngAcceleration.Zero();

	networkTime = 0;
	networkAngles.Zero();

	lastNetworkTime = 0;
	lastNetworkAngles.Zero();

	lastLastNetworkTime = 0;
	lastLastNetworkAngles.Zero();

	networkPacketDelay = 0;
	lastUpdateTime = 0;
	lastDecayTime = 0;

	isNew = true;
	owner = NULL;
	
	numDuplicates = 0;
	lastDuplicateTime = 0;
	isStationary = true;

#ifdef PREDICTION_ERROR_DECAY_DEBUG
	debugFile = NULL;
#endif
}

/*
===============
sdPredictionErrorDecay_Angles::~sdPredictionErrorDecay_Angles
===============
*/
sdPredictionErrorDecay_Angles::~sdPredictionErrorDecay_Angles( void ) {
#ifdef PREDICTION_ERROR_DECAY_DEBUG
	if ( debugFile != NULL ) {
		fileSystem->CloseFile( debugFile );
	}
#endif
}

/*
===============
sdPredictionErrorDecay_Angles::Init
===============
*/
void sdPredictionErrorDecay_Angles::Init( idEntity* _owner ) {
	owner = _owner;

#ifdef PREDICTION_ERROR_DECAY_DEBUG
	debugFile = NULL;
	if ( owner->IsType( idPlayer::Type ) && net_debugPredictionError.GetBool() ) {
		debugFile = fileSystem->OpenFileWrite( va( "ped_angles_%i.csv", pedDebugCount++ ) );
	}

	if ( debugFile != NULL ) {
		debugFile->Printf( "entNum,currentTime,prevTime,networkPacketDelay,timeSinceNewPacket," );
		debugFile->Printf( "netAngles.pitch,netAngles.yaw,netAngles.roll,renderAngles.pitch,renderAngles.yaw,renderAngles.roll," );
		debugFile->Printf( "resultAngles.pitch,resultAngles.yaw,resultAngles.roll," );
		debugFile->Printf( "\n" );
	}
#endif
}

/*
===============
sdPredictionErrorDecay_Angles::Reset
===============
*/
void sdPredictionErrorDecay_Angles::Reset( const idMat3& newAxis ) {
	lastReturnedAngles = newAxis.ToAngles();
	lastReturnedAngVelocity.Zero();
	lastReturnedAngAcceleration.Zero();

	networkTime = gameLocal.time;
	networkAngles = newAxis.ToAngles();

	lastNetworkTime = networkTime;
	lastNetworkAngles = networkAngles;

	lastLastNetworkTime = lastNetworkTime;
	lastLastNetworkAngles = lastNetworkAngles;

	numDuplicates = 0;
	lastDuplicateTime = 0;
	isStationary = true;

	lastDecayTime = gameLocal.time - gameLocal.msec;
}

/*
===============
sdPredictionErrorDecay_Angles::Update
===============
*/
void sdPredictionErrorDecay_Angles::Update() {
	lastUpdateTime = gameLocal.time;

/*	bool hasLocalPhysics = !owner->IsPhysicsInhibited();
	if ( hasLocalPhysics ) {
		int timeSinceNewPacket = gameLocal.time - networkTime;
		timeSinceNewPacket -= gameLocal.msec;
		if ( timeSinceNewPacket > owner->aorPacketSpread + owner->aorPacketSpread / 5 ) {
			// insert a new "packet" if its been too long and we're running physics locally
			// its generally close enough that this all averages out
			OnNewInfo( owner->GetPhysics()->GetAxis() );
		}
	}
*/
#ifdef PREDICTION_ERROR_DECAY_DEBUG
	if ( net_debugPredictionError.GetBool() && debugFile == NULL ) {
		debugFile = fileSystem->OpenFileWrite( va( "ped_angles_%i.csv", pedDebugCount++ ) );
		if ( debugFile != NULL ) {
			debugFile->Printf( "entNum,currentTime,prevTime,networkPacketDelay,timeSinceNewPacket," );
			debugFile->Printf( "netAngles.pitch,netAngles.yaw,netAngles.roll,renderAngles.pitch,renderAngles.yaw,renderAngles.roll," );
			debugFile->Printf( "resultAngles.pitch,resultAngles.yaw,resultAngles.roll," );
			debugFile->Printf( "\n" );
		}
	}
	if ( !net_debugPredictionError.GetBool() && debugFile != NULL ) {
		fileSystem->CloseFile( debugFile );
		debugFile = NULL;
	}
#endif
}

/*
===============
sdPredictionErrorDecay_Angles::Decay
===============
*/
void sdPredictionErrorDecay_Angles::Decay( idMat3& renderAxis, bool yawOnly ) {
	//
	// TWTODO: This whole function needs a good old cleanup!!
	//

	if ( IsNew() ) {
		return;
	}

	assert( owner );

	if ( !net_predictionErrorDecay.GetBool() ) {
		Reset( renderAxis );
		return;
	}

	if ( gameLocal.time <= lastDecayTime || !gameLocal.isNewFrame || gameLocal.msec == 0 || !owner->fl.allowPredictionErrorDecay ) {
		renderAxis = lastReturnedAngles.ToMat3();
		return;
	}

	idMat3 predictedAxes = renderAxis;
	idAngles predictedAngles = predictedAxes.ToAngles();
	predictedAngles = networkAngles - ( networkAngles - predictedAngles ).Normalize180();

	bool hasLocalPhysics = !owner->IsPhysicsInhibited();
	if ( networkPacketDelay > 0 && networkTime != lastNetworkTime ) {

		// TODO: clean up this timing stuff!
		int timeSinceNewPacket = gameLocal.time - networkTime;
		if ( timeSinceNewPacket < gameLocal.msec ) {
			timeSinceNewPacket = gameLocal.msec;
		}

		int lerpDuration = owner->aorPacketSpread*2;
		if ( lerpDuration < gameLocal.msec ) {
			lerpDuration = gameLocal.msec;
		}

		int matchingTimeInt = gameLocal.msec * 3;
		if ( matchingTimeInt < lerpDuration ) {
			matchingTimeInt = lerpDuration;
		}

		float matchingTime = MS2SEC( matchingTimeInt );

		//
		// Predict an angles and angular velocity for the future
		//
		idAngles futureAngVelocity;
		int timeOffset = timeSinceNewPacket;

		idAngles hasPhysicsVelocity = ang_zero;
		idAngles noPhysicsVelocity = ang_zero;

		int currentPrediction = networkSystem->ClientGetPrediction();

		//
		// estimate velocity from local physics
		if ( hasLocalPhysics ) {
			if ( !isStationary && timeSinceNewPacket + ( networkTime - lastNetworkTime ) > 0 ) {
				if ( owner->IsType( idPlayer::Type ) && timeSinceNewPacket > 0 ) {
					hasPhysicsVelocity = ( predictedAngles - networkAngles) / MS2SEC( timeSinceNewPacket );
				} else {
					// velocity averaged over last couple of packets plus the prediction so far
					hasPhysicsVelocity = ( predictedAngles - lastNetworkAngles ) / MS2SEC( timeSinceNewPacket + ( networkTime - lastNetworkTime )  );
				}
			} else {
				hasPhysicsVelocity = ( predictedAngles - networkAngles ) / MS2SEC( gameLocal.msec * 4 );
				timeOffset = gameLocal.msec * 5;
			}
		} 
		
		//
		// estimate velocity from what has been sent on the network
		{
			idAngles networkAngVelocity = ( networkAngles - lastNetworkAngles ) / MS2SEC( networkTime - lastNetworkTime );

			if ( lastNetworkTime != lastLastNetworkTime ) {
				idAngles lastNetworkAngVelocity = ( lastNetworkAngles - lastLastNetworkAngles ) / MS2SEC( lastNetworkTime - lastLastNetworkTime );

				float recentBiasValue = idMath::ClampFloat( 0.0f, 1.0f, timeSinceNewPacket / ( lerpDuration * 0.5f ) );
				noPhysicsVelocity = Lerp( lastNetworkAngVelocity, networkAngVelocity, 0.5f + 0.5f * recentBiasValue );

				idAngles networkAngAcceleration = ( networkAngVelocity - lastNetworkAngVelocity ) / MS2SEC( 0.5f * ( ( networkTime + lastNetworkTime ) - ( lastNetworkTime + lastLastNetworkTime )  ) );

				// decay networkAngAcceleration based on how long its been since the acceleration took place
				networkAngAcceleration *= 1.0f - recentBiasValue;

				noPhysicsVelocity += networkAngAcceleration * MS2SEC( timeSinceNewPacket );
			} else {
				noPhysicsVelocity = networkAngVelocity;
			}

			// scale back a bit for less overshoot
			noPhysicsVelocity.pitch *= 0.1f;
			noPhysicsVelocity.yaw *= 0.4f;
			noPhysicsVelocity.roll *= 0.1f;

			// decay velocities down if timeSinceNewPacket is getting big
			int velocityDecayStart = lerpDuration + lerpDuration / 2 + currentPrediction;
			if ( timeSinceNewPacket > velocityDecayStart ) {
				float scale = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, ( timeSinceNewPacket - velocityDecayStart ) / ( lerpDuration * 0.5f ) );
				noPhysicsVelocity *= scale;
			}
		}


		// blend the two together based on distance to the physics cutoff point
		// to smooth the change between prediction modes
		if ( owner->aorLayout != NULL ) {
			float physicsCutoffSqr = owner->aorLayout->GetPhysicsCutoffSqr();
			if ( physicsCutoffSqr > idMath::FLT_EPSILON ) {
				float lerpValue = idMath::ClampFloat( 0.0f, 1.0f, owner->aorDistanceSqr / physicsCutoffSqr );
				futureAngVelocity = Lerp( hasPhysicsVelocity, noPhysicsVelocity, lerpValue );
			} else {
				futureAngVelocity = noPhysicsVelocity;
			}
		} else {
			if ( hasLocalPhysics ) {
				futureAngVelocity = hasPhysicsVelocity;
			} else {
				futureAngVelocity = noPhysicsVelocity;
			}
		}

		idAngles futureAngles = networkAngles + futureAngVelocity * ( matchingTime + MS2SEC( timeOffset ) );

		//
		// Construct & evaluate splines for output
		//
		float t = MS2SEC( gameLocal.msec ) / matchingTime;
		float spreadBasedScaleUp = Lerp( 1.0f, 1.5f, ( 1000 - lerpDuration + 150 ) / 1000.0f );
		t *= spreadBasedScaleUp;
		if ( t > 0.5f ) {
			t = 0.5f;
		}
		float t2 = t*t;
		float t3 = t2*t;
		float t4 = t2*t2;

		// angles
		idAngles tweenAngles = ang_zero;
		idAngles tweenAngVelocity = ang_zero;
		idAngles tweenAngAcceleration = ang_zero;

		if ( !yawOnly ) {
			sdQuarticAngleSpline angSpline;
			angSpline.Construct( lastReturnedAngles, lastReturnedAngVelocity, lastReturnedAngAcceleration, futureAngles, futureAngVelocity );
			tweenAngles = angSpline.EvaluateX( t, t2, t3, t4 );
			tweenAngVelocity = angSpline.EvaluateDX( t, t2, t3 );
			tweenAngAcceleration = angSpline.EvaluateDDX( t, t2 );
		} else {
			// player only operates on yaw
			float diffYaw = idMath::AngleNormalize180( futureAngles.yaw - lastReturnedAngles.yaw );
			sdQuarticFunction yaw;
			yaw.Construct( 0.0f, lastReturnedAngVelocity.yaw, lastReturnedAngAcceleration.yaw, diffYaw, futureAngVelocity.yaw );
			tweenAngles.yaw = yaw.EvaluateX( t, t2, t3, t4 ) + lastReturnedAngles.yaw;
			tweenAngVelocity.yaw = yaw.EvaluateDX( t, t2, t3 );
			tweenAngAcceleration.yaw = yaw.EvaluateDDX( t, t2 );
		}

		tweenAngles.FixDenormals();
		tweenAngVelocity.FixDenormals();
		tweenAngAcceleration.FixDenormals();

		// cache output
		renderAxis = tweenAngles.ToMat3();
		lastReturnedAngles = tweenAngles;
		lastReturnedAngVelocity = tweenAngVelocity;
		lastReturnedAngAcceleration = tweenAngAcceleration;

		//
		// Debugging info
		//
#ifdef PREDICTION_ERROR_DECAY_DEBUG
		if ( debugFile != NULL ) {
			if ( owner != NULL ) {
				debugFile->Printf( "%i,", owner->entityNumber );
			} else {
				debugFile->Printf( "-1," );
			}

			// timing
			debugFile->Printf( "%i,%i,%i,%i,", gameLocal.time, networkTime, networkPacketDelay, timeSinceNewPacket );

			// angles
			idAngles renderAngles = renderAxis.ToAngles();
			debugFile->Printf( "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", networkAngles.pitch, networkAngles.yaw, networkAngles.roll,
																predictedAngles.pitch, predictedAngles.yaw, predictedAngles.roll );
			debugFile->Printf( "%.2f,%.2f,%.2f,", renderAngles.pitch, renderAngles.yaw, renderAngles.roll );
			debugFile->Printf( "\n" );
		}
#endif
	} else {
		// do nothing - it hasn't calibrated itself appropriately yet
		lastReturnedAngles = renderAxis.ToAngles();
	}

	lastDecayTime = gameLocal.time;

	// gracefully handle bad values
	// TWTODO: Proper fix!
	if ( MAT_CHECK_BAD( renderAxis ) ) {
		gameLocal.Warning( "sdPredictionErrorDecay_Angles::Decay - Bad float!" );
		assert( false );
		renderAxis = predictedAxes;
		Reset( renderAxis );
	}
}

/*
===============
sdPredictionErrorDecay_Angles::OnNewInfo
===============
*/
void sdPredictionErrorDecay_Angles::OnNewInfo( const idMat3& serverAxes ) {
	assert( owner );

	if ( !net_predictionErrorDecay.GetBool() ) {
		return;
	}

	int newDelay = gameLocal.time - networkTime;

	if ( newDelay < owner->aorPacketSpread ) {
		if ( owner->IsPhysicsInhibited() ) {
			networkAngles = serverAxes.ToAngles();
			networkTime = gameLocal.time;
			FixAngles();
		}
		return;
	}

	// see if it falls beneath the interesting threshold
	idAngles serverAngles = serverAxes.ToAngles();
	idAngles diffAngles = networkAngles - serverAngles;
	diffAngles.Normalize180();
	idVec3 diffAnglesVec( diffAngles[ 0 ], diffAngles[ 1 ], diffAngles[ 2 ] );
	float diffSqr = diffAnglesVec.LengthSqr();
	if ( diffSqr < Square( 0.05f ) ) {
		if ( diffSqr < idMath::FLT_EPSILON && gameLocal.time > lastDuplicateTime ) {
			// its basically identical to the last angles!
			numDuplicates++;
			lastDuplicateTime = gameLocal.time;
			if ( numDuplicates >= 2 ) {
				// 2 duplicates - we must be stationary
				isStationary = true;
			}
		}
		networkAngles = serverAngles;
		networkTime = gameLocal.time;
		FixAngles();
		return;
	}

	if ( isStationary ) {
		newDelay = owner->aorPacketSpread;
		isStationary = false;
		numDuplicates = 0;
	}

	if ( newDelay > 0 ) {
//		networkPacketDelay = ( newDelay + networkPacketDelay ) * 0.5f;
		networkPacketDelay = newDelay;
	}

	// don't let the delay get too big
	if ( networkPacketDelay > 1000 ) {
		networkPacketDelay = 1000;
	}

	if ( isNew ) {
		networkPacketDelay = 0;
		isNew = false;

		networkTime = gameLocal.time;
		networkAngles = serverAxes.ToAngles();

		lastNetworkTime = networkTime;
		lastNetworkAngles = networkAngles;

		lastLastNetworkTime = lastNetworkTime;
		lastLastNetworkAngles = lastNetworkAngles;

	} else {

		lastLastNetworkTime = lastNetworkTime;
		lastLastNetworkAngles = lastNetworkAngles;

		lastNetworkTime = networkTime;
		lastNetworkAngles = networkAngles;

		networkTime = gameLocal.time;
		networkAngles = serverAxes.ToAngles();

		FixAngles();
	}
}

/*
===============
sdPredictionErrorDecay_Angles::FixAngles
===============
*/
void sdPredictionErrorDecay_Angles::FixAngles( void ) {
	lastNetworkAngles = networkAngles - ( networkAngles - lastNetworkAngles ).Normalize180();
	lastLastNetworkAngles = lastNetworkAngles - ( lastNetworkAngles - lastLastNetworkAngles ).Normalize180();
	lastReturnedAngles = lastNetworkAngles - ( lastNetworkAngles - lastReturnedAngles ).Normalize180();
}

/*
===============
sdPredictionErrorDecay_Angles::NeedsUpdate
===============
*/
bool sdPredictionErrorDecay_Angles::NeedsUpdate( void ) {
	return gameLocal.time >= lastDecayTime && !IsNew() && ( ( gameLocal.time - networkTime ) < SEC2MS( MAX_PREDICTION_TIME_OFFSET ) );
}
