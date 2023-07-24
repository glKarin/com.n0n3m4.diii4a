// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_PREDICTIONERRORDECAY_H__
#define __GAME_PREDICTIONERRORDECAY_H__

// #define PREDICTION_ERROR_DECAY_DEBUG

class sdPredictionErrorDecay_Origin {
public:
	
	class CustomDecayInfo {
	public:
		bool	hasLocalPhysics;
		bool	isPlayer;
		int		currentPrediction;
		int		packetSpread;
		bool	boxDecayClip;
		bool	pointDecayClip;
		bool	heightMapDecayClip;
		idVec3	limitVelocity;
		idVec3	origin;
		bool	hasGroundContacts;
		float	physicsCutoffSqr;
		float	ownerAorDistSqr;
	};

						sdPredictionErrorDecay_Origin( void );
						~sdPredictionErrorDecay_Origin( void );

	void				Init( idEntity* owner );
	void				SetOwner( idEntity* _owner ) { owner = _owner; }

	void				Reset( const idVec3& newOrigin );
	void				Update( const CustomDecayInfo* customInfo = NULL );
	void				Decay( idVec3& renderOrigin, const CustomDecayInfo* customInfo = NULL );
	void				OnNewInfo( const idVec3& serverOrigin, int& time = gameLocal.time, const CustomDecayInfo* customInfo = NULL );

	bool				NeedsUpdate( void );

	bool				IsNew( void ) { return isNew; }

	static float		LimitApparentVelocity( const idVec3& oldOrigin,  const idVec3& idealNewOrigin, idVec3& newOrigin, const idVec3& physicsVelocity, int currentPrediction );

	const idVec3&		GetLastReturned( void ) { return lastReturnedOrigin; }
	float				GetNetworkTime( void ) { return networkTime; }

private:

	bool				isNew;
	idEntity*			owner;

	// statistics
	int					networkPacketDelay;
	int					lastUpdateTime;
	int					lastDecayTime;

	// latest network state
	int					networkTime;
	idVec3				networkOrigin;

	// previous network state
	int					lastNetworkTime;
	idVec3				lastNetworkOrigin;

	// previous previous network state
	int					lastLastNetworkTime;
	idVec3				lastLastNetworkOrigin;

	// previously returned data
	idVec3				lastReturnedOrigin;
	idVec3				lastReturnedVelocity;
	idVec3				lastReturnedAcceleration;

	int					numDuplicates;
	int					lastDuplicateTime;
	bool				isStationary;

	// DEBUG STUFF
#ifdef PREDICTION_ERROR_DECAY_DEBUG
	idFile*				debugFile;
	static int pedDebugCount;
	int					myIndex;
#endif
};

class sdPredictionErrorDecay_Angles {
public:

						sdPredictionErrorDecay_Angles( void );
						~sdPredictionErrorDecay_Angles( void );

	void				Init( idEntity* owner );

	void				Reset( const idMat3& newAxis );
	void				Update();
	void				Decay( idMat3& renderAxis, bool yawOnly = false );
	void				OnNewInfo( const idMat3& serverAxis );

	bool				NeedsUpdate( void );

	bool				IsNew( void ) { return isNew; }

private:
	void				FixAngles( void );

	bool				isNew;
	idEntity*			owner;

	// statistics
	int					networkPacketDelay;
	int					lastUpdateTime;
	int					lastDecayTime;

	// latest network state
	int					networkTime;
	idAngles			networkAngles;

	// previous network state
	int					lastNetworkTime;
	idAngles			lastNetworkAngles;

	// previous previous network state
	int					lastLastNetworkTime;
	idAngles			lastLastNetworkAngles;

	// previously returned data
	idAngles			lastReturnedAngles;
	idAngles			lastReturnedAngVelocity;
	idAngles			lastReturnedAngAcceleration;

	int					numDuplicates;
	int					lastDuplicateTime;
	bool				isStationary;

	// DEBUG STUFF
#ifdef PREDICTION_ERROR_DECAY_DEBUG
	idFile*				debugFile;
	static int pedDebugCount;
#endif
};

#endif /* !__GAME_PREDICTIONERRORDECAY_H__ */
