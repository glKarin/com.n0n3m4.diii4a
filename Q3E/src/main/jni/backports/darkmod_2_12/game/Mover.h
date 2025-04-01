/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __GAME_MOVER_H__
#define __GAME_MOVER_H__

extern const idEventDef EV_TeamBlocked;
extern const idEventDef EV_PartBlocked;
extern const idEventDef EV_ReachedPos;
extern const idEventDef EV_ReachedAng;

/*
===============================================================================

  General movers.

===============================================================================
*/

class idMover : public idEntity {
public:
	CLASS_PROTOTYPE( idMover );

							idMover( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;

	virtual void			Hide( void ) override;
	virtual void			Show( void ) override;

	void					SetPortalState( bool open );

	bool					IsBlocked( void );

	float					GetMoveSpeed( void ); // grayman #3029

	ID_INLINE idPhysics_Parametric*	GetMoverPhysics()
	{
		return &physicsObj;
	}

	// SteveL #3712
	bool					SetCanPushPlayer( bool newSetting ); // return old setting

protected:
	typedef enum {
		ACCELERATION_STAGE,
		LINEAR_STAGE,
		DECELERATION_STAGE,
		FINISHED_STAGE
	} moveStage_t;

	typedef enum {
		MOVER_NONE,
		MOVER_ROTATING,
		MOVER_MOVING,
		MOVER_SPLINE
	} moverCommand_t;

	//
	// mover directions.  make sure to change script/tdm_defs.script if you add any, or change their order
	//
	typedef enum {
		DIR_UP				= -1,
		DIR_DOWN			= -2,
		DIR_LEFT			= -3,
		DIR_RIGHT			= -4,
		DIR_FORWARD			= -5,
		DIR_BACK			= -6,
		DIR_REL_UP			= -7,
		DIR_REL_DOWN		= -8,
		DIR_REL_LEFT		= -9,
		DIR_REL_RIGHT		= -10,
		DIR_REL_FORWARD		= -11,
		DIR_REL_BACK		= -12
	} moverDir_t;

	typedef struct moveStage_s {
		moveStage_t			stage;
		int					acceleration;
		int					movetime;
		int					deceleration;
		idVec3				dir;
	} moveState_t;

	typedef struct rotationState_s {
		moveStage_t			stage;
		int					acceleration;
		int					movetime;
		int					deceleration;
		idAngles			rot;
	} rotationState_t;

	idPhysics_Parametric	physicsObj;

	void					Event_OpenPortal( void );
	void					Event_ClosePortal( void );
	virtual void			OpenPortal();
	virtual void			ClosePortal();
	void					Event_PartBlocked( idEntity *blockingEntity );
	virtual void			OnTeamBlocked(idEntity* blockedEntity, idEntity* blockingEntity);

	void					MoveToPos( const idVec3 &pos);
	void					MoveToLocalPos( const idVec3 &pos );

	void					UpdateMoveSound( moveStage_t stage );
	void					UpdateRotationSound( moveStage_t stage );
	void					SetGuiStates( const char *state );
	void					FindGuiTargets( void );
	void					SetGuiState( const char *key, const char *val ) const;

	virtual void			DoneMoving( void );
	virtual void			DoneRotating( void );
	virtual void			BeginMove( idThread *thread = NULL );
	virtual void			BeginRotation( idThread *thread, bool stopwhendone );

	/**
	 * greebo: Calculates the fraction of the move_time (the time it takes to complete the whole
	 *         movement) based on the current state. For standard-movers, this does nothing (returns 1),
	 *         but subclasses might do more complicated things based on the current rotation state,
	 *         like BinaryFrobMovers do.
	 *
	 * Note: This is an internal function used by BeginRotation() only
	 */
	virtual float			GetMoveTimeRotationFraction(); // grayman #3711

	/**
	 * grayman #3711 - Need a version of GetMoveTimeRotationFraction() for translation
	 */
	virtual float			GetMoveTimeTranslationFraction();

	moveState_t				move;

protected:
	rotationState_t			rot;

	int						move_thread;
	int						rotate_thread;
	idAngles				dest_angles;
	idAngles				angle_delta;
	idVec3					dest_position;
	idVec3					move_delta;
	float					move_speed;
	int						move_time;
	int						prevMoveTime; // grayman #3755 - previous move_time
	float					prevTransSpeed; // grayman #3755
	int						deceltime;
	int						acceltime;
	bool					stopRotation;
	bool					useSplineAngles;
	idEntityPtr<idEntity>	splineEnt;
	moverCommand_t			lastCommand;
	float					damage;
	int						nextBounceTime; // grayman #4370 - next time you can bounce off an AI
	qhandle_t				areaPortal;		// 0 = no portal

	idList< idEntityPtr<idEntity> >	guiTargets;

	void					VectorForDir( float dir, idVec3 &vec );
	idCurve_Spline<idVec3> *GetSpline( idEntity *splineEntity ) const;

public:
	void					Event_SetCallback( void );	
	void					Event_TeamBlocked( idEntity *blockedPart, idEntity *blockingEntity );
	void					Event_StopMoving( void );
	void					Event_StopRotating( void );
	void					Event_UpdateMove( void );
	void					Event_UpdateRotation( void );
	void					Event_SetMoveSpeed( float speed );
	void					Event_SetMoveTime( float time );
	void					Event_GetMoveSpeed( void ) const;
	void					Event_GetMoveTime( void ) const;
	void					Event_SetDecelerationTime( float time );
	void					Event_SetAccellerationTime( float time );
	void					Event_MoveTo( idEntity *ent );
	void					Event_MoveToPos( idVec3 &pos );
	void					Event_MoveDir( float angle, float distance );
	void					Event_MoveAccelerateTo( float speed, float time );
	void					Event_MoveDecelerateTo( float speed, float time );
	void					Event_RotateDownTo( int axis, float angle );
	void					Event_RotateUpTo( int axis, float angle );
	void					Event_RotateTo( idAngles &angles );
	void					Event_Rotate( idAngles &angles );
	void					Event_RotateOnce( idAngles &angles );
	void					Event_Bob( float speed, float phase, idVec3 &depth );
	void					Event_Sway( float speed, float phase, idAngles &depth );
	void					Event_SetAccelSound( const char *sound );
	void					Event_SetDecelSound( const char *sound );
	void					Event_SetMoveSound( const char *sound );
	void					Event_FindGuiTargets( void );
	void					Event_InitGuiTargets( void );
	void					Event_EnableSplineAngles( void );
	void					Event_DisableSplineAngles( void );
	void					Event_RemoveInitialSplineAngles( void );
	void					Event_StartSpline( idEntity *splineEntity );
	void					Event_StopSpline( void );
	void					Event_Activate( idEntity *activator );
	void					Event_PostRestore( int start, int total, int accel, int decel, int useSplineAng );
	void					Event_IsMoving( void );
	void					Event_IsRotating( void );
};

class idSplinePath : public idEntity {
public:
	CLASS_PROTOTYPE( idSplinePath );

							idSplinePath();

	void					Spawn( void );
};


struct floorInfo_s {
	idVec3					pos;
	idStr					door;
	int						floor;
};

/*
===============================================================================

  Binary movers.

===============================================================================
*/

typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverState_t;

class idMover_Binary : public idEntity {
public:
	CLASS_PROTOTYPE( idMover_Binary );

							idMover_Binary();
	virtual					~idMover_Binary() override;

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			PreBind( void ) override;
	virtual void			PostBind( void ) override;

	void					Enable( bool b );
	void					InitSpeed( idVec3 &mpos1, idVec3 &mpos2, float mspeed, float maccelTime, float mdecelTime );
	void					InitTime( idVec3 &mpos1, idVec3 &mpos2, float mtime, float maccelTime, float mdecelTime );
	void					GotoPosition1( void );
	void					GotoPosition2( void );
	void					Use_BinaryMover( idEntity *activator );
	void					SetGuiStates( const char *state );
	void					UpdateBuddies( int val );
	idMover_Binary *		GetActivateChain( void ) const { return activateChain; }
	idMover_Binary *		GetMoveMaster( void ) const { return moveMaster; }
	void					BindTeam( idEntity *bindTo );
	void					SetBlocked( bool b );
	bool					IsBlocked( void );
	idEntity *				GetActivator( void ) const;

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;

	void					SetPortalState( bool open );

protected:
	idVec3					pos1;
	idVec3					pos2;
	moverState_t			moverState;
	idMover_Binary *		moveMaster;
	idMover_Binary *		activateChain;
	int						soundPos1;
	int						sound1to2;
	int						sound2to1;
	int						soundPos2;
	int						soundLoop;
	float					wait;
	float					damage;
	int						duration;
	int						accelTime;
	int						decelTime;
	idEntityPtr<idEntity>	activatedBy;
	int						stateStartTime;
	idStr					team;
	bool					enabled;
	int						move_thread;
	int						updateStatus;		// 1 = lock behaviour, 2 = open close status
	idStrList				buddies;
	idPhysics_Parametric	physicsObj;
	qhandle_t				areaPortal;			// 0 = no portal
	bool					blocked;
	idList< idEntityPtr<idEntity> >	guiTargets;

	void					MatchActivateTeam( moverState_t newstate, int time );
	void					JoinActivateTeam( idMover_Binary *master );

	void					UpdateMoverSound( moverState_t state );
	void					SetMoverState( moverState_t newstate, int time );
	moverState_t			GetMoverState( void ) const { return moverState; }
	void					FindGuiTargets( void );
	void					SetGuiState( const char *key, const char *val ) const;

	void					Event_SetCallback( void );
	void					Event_ReturnToPos1( void );
	void					Event_Use_BinaryMover( idEntity *activator );
	void					Event_Reached_BinaryMover( void );
	void					Event_MatchActivateTeam( moverState_t newstate, int time );
	void					Event_Enable( void );
	void					Event_Disable( void );
	void					Event_OpenPortal( void );
	void					Event_ClosePortal( void );
	void					Event_FindGuiTargets( void );
	void					Event_InitGuiTargets( void );

	static void				GetMovedir( float dir, idVec3 &movedir );
};

class idPlat : public idMover_Binary {
public:
	CLASS_PROTOTYPE( idPlat );

							idPlat( void );
	virtual					~idPlat( void ) override;

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void ) override;
	virtual void			PreBind( void ) override;
	virtual void			PostBind( void ) override;

private:
	idClipModel *			trigger;
	idVec3					localTriggerOrigin;
	idMat3					localTriggerAxis;

	void					GetLocalTriggerPosition( const idClipModel *trigger );
	void					SpawnPlatTrigger( idVec3 &pos );

	void					Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity );
	void					Event_PartBlocked( idEntity *blockingEntity );
	void					Event_Touch( idEntity *other, trace_t *trace );
};


/*
===============================================================================

  Special periodic movers.

===============================================================================
*/

class idMover_Periodic : public idEntity {
public:
	CLASS_PROTOTYPE( idMover_Periodic );

							idMover_Periodic( void );

	void					Spawn( void );
	
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void ) override;

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;

protected:
	idPhysics_Parametric	physicsObj;
	float					damage;

	void					Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity );
	void					Event_PartBlocked( idEntity *blockingEntity );
};

class idRotater : public idMover_Periodic {
public:
	CLASS_PROTOTYPE( idRotater );

							idRotater( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	// greebo: Allows callers to invert the rotation direction
	void					SetDirection(bool forward);

private:
	// Reads the rotation axis and speed from the spawnargs and starts/stops to rotate
	void					SetRotationFromSpawnargs(bool forward);

	idEntityPtr<idEntity>	activatedBy;

	// If true, the next activation will let the mover rotate in forward direction
	// this is only used when "invert_on_trigger" is set to "1"
	bool					nextTriggerDirectionIsForward;

	void					Event_Activate( idEntity *activator );
};

class idBobber : public idMover_Periodic {
public:
	CLASS_PROTOTYPE( idBobber );

							idBobber( void );

	void					Spawn( void );

private:
};

class idPendulum : public idMover_Periodic {
public:
	CLASS_PROTOTYPE( idPendulum );

							idPendulum( void );

	void					Spawn( void );

private:
};

class idRiser : public idMover_Periodic {
public:
	CLASS_PROTOTYPE( idRiser );

	idRiser( void );

	void					Spawn( void );

private:
	void					Event_Activate( idEntity *activator );
};

#endif /* !__GAME_MOVER_H__ */
