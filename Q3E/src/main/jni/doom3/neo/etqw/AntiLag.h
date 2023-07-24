// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ANTILAG_H__
#define __GAME_ANTILAG_H__

static const int	MAX_ANTILAG_SETS = 48;
static const int	MAX_ANTILAG_POINTS = 20;

// #define	ANTILAG_LOGGING

/*
===============================================================================

	sdAntiLagSet
		Set of points the player could be at after continuing moving from one branch point

===============================================================================
*/
class sdAntiLagEntity;

class sdAntiLagSet {
public:
						sdAntiLagSet();

	virtual void		Reset();
	virtual void		Setup( sdAntiLagEntity& antiLagEntity );
	virtual void		Setup( sdAntiLagEntity& antiLagEntity, sdAntiLagSet& baseSet );
	virtual void		Update() = 0;
	virtual void		DebugDraw();

	const idVec3*		GetPointForTime( int time );

	inline bool				IsValid() const { return branchTime > 0; };
	inline const idVec3&	GetBranchPoint() const { return points[ 0 ]; }
	inline int				GetBranchTime() const { return branchTime; }
	inline const idBounds&	GetBounds() const { return bounds; }
	inline const idBounds&	GetEntityBounds() const { return entityBounds; }
	inline const idVec3&	GetVelocity() const { return velocity; }
	inline bool				OnGround() const { return onGround; }

	bool				IsValidFor( int clientNum ) const { return clientsValidFor.Get( clientNum ) > 0; }
	void				SetValidFor( int clientNum ) { clientsValidFor.Set( clientNum ); }

	bool				IsUserCommandOnly() const { return userCommandOnly; }
	int					GetBaseBranchTime() const { return baseBranchTime; }
protected:
	idVec3				velocity;

	int					branchTime;
	int					timeUpto;
	bool				onGround;

	idVec3				points[ MAX_ANTILAG_POINTS ];
	idBounds			entityBounds;
	idBounds			bounds;

	sdAntiLagEntity*	antiLagEntity;

	sdBitField< MAX_CLIENTS >	clientsValidFor;

	// support for branching off branches based on user command updates
	bool				userCommandOnly;
	int					baseBranchTime;
};

class sdAntiLagSetGeneric : public sdAntiLagSet {
public:
	virtual void		Setup( sdAntiLagEntity& antiLagEntity );
	virtual void		Setup( sdAntiLagEntity& antiLagEntity, sdAntiLagSet& baseSet );
	virtual void		Update();

	const idVec3&		GetAcceleration() const { return acceleration; }

protected:
	idVec3				acceleration;
};

class sdAntiLagSetPlayer : public sdAntiLagSet {
public:
	virtual void		Setup( sdAntiLagEntity& antiLagEntity );
	virtual void		Setup( sdAntiLagEntity& antiLagEntity, sdAntiLagSet& baseSet );
	virtual void		Update();

	const idVec3&		GetGravity() const { return gravity; }
	const idVec3&		GetGroundNormal() const { return groundNormal; }
	waterLevel_t		GetWaterLevel() const { return waterLevel; }

protected:
	// simulate player move!
	float				CmdScale( int fwd, int right, int up, bool noVertical ) const;
	idVec2				CalcDesiredWalkMove( int fwd, int right ) const;
	void				Accelerate( const idVec3 &wishdir, const float wishspeed, const float accel );
	void				WalkMove();
	void				Friction();

	idVec3				gravity;
	idVec3				groundNormal;

	waterLevel_t		waterLevel;

	float				walkSpeedFwd;
	float				walkSpeedBack;
	float				walkSpeedSide;

	idVec3				viewForward;
	idVec3				viewRight;
	float				playerSpeed;

	int					cmdForwardMove;
	int					cmdRightMove;
};

/*
===============================================================================

	sdAntiLagEntity

===============================================================================
*/
class sdAntiLagEntity {
public:
	sdAntiLagEntity() {
		for ( int i = 0; i < MAX_ANTILAG_SETS; i++ ) {
			antiLagSets[ i ] = NULL;
		}
	}

	virtual void		Create();
	virtual void		Destroy();

	virtual void		Reset();
	void				Init( const idEntity* self );

	virtual void		Update();
	void				CreateBranch( int clientNum );
	void				CreateUserCommandBranch( int clientNum );
	idBounds			GetBounds();
	sdAntiLagSet*		GetAntiLagSet( int clientNum );

	const idVec3&		GetClientEstimate( int clientNum ) { return clientEstimates[ clientNum ].GetLastReturned(); }

	sdAntiLagSet&		GetMostRecentBranch() { return *antiLagSets[ ( antiLagUpto + MAX_ANTILAG_SETS - 1 ) % MAX_ANTILAG_SETS ]; }

	idEntity*			GetSelf() const { return self; }



#ifdef ANTILAG_LOGGING
	float				antiLagDistances[ MAX_CLIENTS ];
#endif

protected:

	idEntityPtr< idEntity >	self;

	// antilag branches
	sdAntiLagSet*		antiLagSets[ MAX_ANTILAG_SETS ];
	int					antiLagUpto;

	// estimate of where all the other clients think this entity is
	sdPredictionErrorDecay_Origin	clientEstimates[ MAX_CLIENTS ];
};

/*
===============================================================================

	sdAntiLagPlayer

===============================================================================
*/
class sdAntiLagPlayer : public sdAntiLagEntity {
public:
	virtual void		Create();
	virtual void		Destroy();

	void				Trace( trace_t& result, const idVec3& start, const idVec3& end, int shooterIndex );

protected:
	sdAntiLagSetPlayer	playerSets[ MAX_ANTILAG_SETS ];
};

/*
===============================================================================

	sdAntiLagManager

===============================================================================
*/
class sdAntiLagManagerLocal {
public:
	sdAntiLagManagerLocal();

	void				OnMapLoad();
	void				OnMapShutdown();

	void				Think();

	void				ResetForPlayer( const idPlayer* self );
	void				CreateBranch( const idPlayer* self );
	void				CreateUserCommandBranch( const idPlayer* self );
	void				Trace( trace_t& result, const idVec3& start, const idVec3& end, int mask, idPlayer* clientShooting, idEntity* ignore );

	sdAntiLagPlayer&	GetAntiLagPlayer( int index );

	idClipModel*		GetModelForBounds( const idBounds& bounds );

	void				OnNetworkEvent( int clientNum, const idBitMsg& msg );

	const idVec3&		GetLastTraceHitOrigin( int clientId ) const { return lastTraceHitOrigin[ clientId ]; }
	void				SetLastTraceHitOrigin( int clientId, const idVec3& origin ) { lastTraceHitOrigin[ clientId ] = origin; }

protected:
#ifdef ANTILAG_LOGGING
	idFile*				logFile;
	idFile*				clientLogFile;

	idVec3				lastStart;
	idVec3				lastEnd;
	int					lastTime;

	void				SendTrace( const idVec3& start, const idVec3& end );
	void				RecordServerTrace( trace_t& result, const idVec3& start, const idVec3& end, idPlayer* clientShooting );
	void				RecordTrace( int time, bool server, const idVec3& start, const idVec3& end, int shooter );
#endif


	sdAntiLagPlayer		players[ MAX_CLIENTS ];
	idVec3				lastTraceHitOrigin[ MAX_CLIENTS ];

	idList< idClipModel* >	cachedPlayerModels;
};

typedef sdSingleton< sdAntiLagManagerLocal > sdAntiLagManager;


#endif /* !__GAME_ANTILAG_H__ */
