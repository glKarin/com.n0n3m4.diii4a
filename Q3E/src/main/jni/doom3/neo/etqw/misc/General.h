// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_MOVER_GENERAL_H__
#define __GAME_MOVER_GENERAL_H__

#include "../ScriptEntity.h"
#include "../physics/Physics_Base.h"

/*
===============================================================================

  General movers.

===============================================================================
*/

class sdGeneralMoverPhysicsNetworkData : public sdEntityStateNetworkData {
public:
							sdGeneralMoverPhysicsNetworkData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	float					currentFraction;
};

class sdPhysics_GeneralMover : public idPhysics_Base {
public:
							sdPhysics_GeneralMover( void );
							~sdPhysics_GeneralMover( void );

	virtual void			SetClipModel( idClipModel *model, float density, int id, bool freeOld = true );
	virtual idClipModel*	GetClipModel( int id ) const;
	virtual int				GetNumClipModels( void ) const;
	virtual void			SetContents( int contents, int id );
	virtual int				GetContents( int id ) const;
	virtual const idBounds&	GetBounds( int id ) const;
	virtual const idBounds&	GetAbsBounds( int id ) const;
	virtual bool			Evaluate( int timeStepMSec, int endTimeMSec );
	virtual bool			IsAtRest( void ) const;
	virtual bool			IsPushable( void ) const;
	virtual bool			EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY );

	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;

	virtual const idVec3&	GetOrigin( int id = 0 ) const;
	virtual const idMat3&	GetAxis( int id = 0 ) const;

	virtual void			EnableClip( void );
	virtual void			DisableClip( bool activateContacting = true );

	virtual int				GetClipMask( int id ) const;
	virtual void			SetClipMask( int mask, int id );

	void					SetCurrentPos( float newPos, bool force = false );
	void					UpdateClipModel( void );
	void					StartMove( const idVec3& startPos, const idVec3& endPos, const idAngles& startAngles, const idAngles& endAngles, int startTime, int length );

	void					SetInitialPosition( const idVec3& org, const idMat3& axes );
private:

	struct move_t {
		idVec3					startPos;
		idVec3					endPos;

		idAngles				startAngles;
		idAngles				endAngles;
	};

	move_t					move;

	int						clipMask;
	idClipModel*			clipModel;
	float					currentFraction;
	float					rate;
};

class sdGeneralMover : public sdScriptEntity {
public:

	typedef enum {
		GMS_MOVING,
		GMS_WAITING,
		GMS_NUM_STATES,
	} state_t;

	static const int		net_moverStateBits;

	CLASS_PROTOTYPE( sdGeneralMover );

							sdGeneralMover( void );
	virtual					~sdGeneralMover( void );

	void					Spawn( void );

	virtual void			PostMapSpawn( void );

	int						AddPosition( const idVec3& pos, const idAngles& angles );
	state_t					GetState( void ) const;

	void					StartTimedMove( int from, int to, int ms, int startTime );

	virtual void			WriteDemoBaseData( idFile* file ) const;
	virtual void			ReadDemoBaseData( idFile* file );

	virtual void			ReachedPosition( void );
	virtual void			OnTeamBlocked( idEntity* blockedPart, idEntity* blockingEntity );

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg& msg );

	enum {
		EVENT_MOVE = sdScriptEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

protected:

	typedef struct positionInfo_s {
		idVec3				pos;
		idAngles			angles;
	} positionInfo_t;

	typedef struct moveInfo_s { 
		int					startPos;
		int					endPos;
		int					startTime;
		int					moveTime;
		bool				rotateOnly;
	} moveInfo_t;

	moveInfo_t				_curMove;
	idList<positionInfo_t>	_positions;

	sdPhysics_GeneralMover	_physicsObj;

	bool					_rotateOnly;
	bool					_killBlocked;

	void					Event_AddPosition( const idVec3& pos, const idAngles& angles );
	void					Event_GetState();
	void					Event_StartTimedMove( int from, int to, float seconds );
	void					Event_SetPosition( int index );
	void					Event_KillBlockingEntity( bool kill );
	void					Event_GetNumPositions( void );
};

class sdGeneralMoverBroadcastData : public sdScriptEntityBroadcastData {
public:
							sdGeneralMoverBroadcastData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	sdGeneralMover::state_t state;
	int						startPos;
	int						endPos;
	int						startTime;
	int						moveTime;
	bool					rotateOnly;
};

#endif // __GAME_MOVER_GENERAL_H__
