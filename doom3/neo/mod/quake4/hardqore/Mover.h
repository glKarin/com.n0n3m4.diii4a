
#ifndef __GAME_MOVER_H__
#define __GAME_MOVER_H__

extern const idEventDef EV_TeamBlocked;
extern const idEventDef EV_PartBlocked;
extern const idEventDef EV_ReachedPos;
extern const idEventDef EV_ReachedAng;

// RAVEN BEGIN
// bdube: more externs
extern const idEventDef EV_Door_Lock;
extern const idEventDef EV_Door_IsLocked;
// abahr:
extern const idEventDef EV_GetSplineEntity;
extern const idEventDef EV_MoveAlongVector;
extern const idEventDef EV_Door_Open;
extern const idEventDef EV_Door_Close;
extern const idEventDef EV_Door_IsOpen;
extern const idEventDef EV_Move;
extern const idEventDef EV_RotateOnce;
// twhitaker:
extern const idEventDef EV_Speed;
extern const idEventDef EV_IsActive;
extern const idEventDef EV_IsMoving;

// mekberg: spline sampling
#define SPLINE_SAMPLE_RATE	60.0f
// RAVEN END

/*
===============================================================================

  General movers.

===============================================================================
*/

class idMover : public idEntity {
public:
	CLASS_PROTOTYPE( idMover );

							idMover( void );
							~idMover( void );

	void					Spawn( void );

// RAVEN BEGIN
// mekberg: added
	void					Think( void );
// RAVEN END

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual void			Hide( void );
	virtual void			Show( void );

	void					SetPortalState( bool open );

// RAVEN BEGIN
// mekberg: sounds for splines
	void					UpdateSplineStage ( void );
// RAVEN END

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
	// mover directions.  make sure to change script/doom_defs.script if you add any, or change their order
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

	typedef struct {
		moveStage_t			stage;
		int					acceleration;
		int					movetime;
		int					deceleration;
		idVec3				dir;
	} moveState_t;

	typedef struct {
		moveStage_t			stage;
		int					acceleration;
		int					movetime;
		int					deceleration;
		idAngles			rot;
	} rotationState_t;

	idPhysics_Parametric	physicsObj;

	void					Event_OpenPortal( void );
	void					Event_ClosePortal( void );
	void					Event_PartBlocked( idEntity *blockingEntity );

	void					MoveToPos( const idVec3 &pos);
	void					UpdateMoveSound( moveStage_t stage );
	void					UpdateRotationSound( moveStage_t stage );
	void					SetGuiStates( const char *state );
	void					FindGuiTargets( void );
	void					SetGuiState( const char *key, const char *val ) const;

	virtual void			DoneMoving( void );
	virtual void			DoneRotating( void );
	virtual void			BeginMove( idThread *thread = NULL );
	virtual void			BeginRotation( idThread *thread, bool stopwhendone );
	moveState_t				move;

	rvStateThread			splineStateThread;
	float					damage;

private:
	rotationState_t			rot;

	int						move_thread;
	int						rotate_thread;
	idAngles				dest_angles;
	idAngles				angle_delta;
	idVec3					dest_position;
	idVec3					move_delta;
	float					move_speed;
	int						move_time;
	int						deceltime;
	int						acceltime;
	bool					stopRotation;
	bool					useSplineAngles;
	bool					attenuate;
	bool					useIdleSound;
	idEntityPtr<idEntity>	splineEnt;
	moverCommand_t			lastCommand;

	qhandle_t				areaPortal;		// 0 = no portal

// mekberg: attenution and sound additions
	float					maxAttenuation;
	float					attenuationScale;
	idVec3					lastOrigin;
	int						lastTime;
	int						splineStartTime;
// RAVEN END

	idList< idEntityPtr<idEntity> >	guiTargets;

// RAVEN BEGIN
// abahr:
	bool					GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	void					MoveAlongVector( const idVec3& vec );
// RAVEN END

	void					VectorForDir( float dir, idVec3 &vec );
	idCurve_Spline<idVec3> *GetSpline( idEntity *splineEntity ) const;

	void					Event_SetCallback( void );	
	void					Event_TeamBlocked( idEntity *blockedPart, idEntity *blockingEntity );
	void					Event_StopMoving( void );
	void					Event_StopRotating( void );
	void					Event_UpdateMove( void );
	void					Event_UpdateRotation( void );
	void					Event_SetMoveSpeed( float speed );
	void					Event_SetMoveTime( float time );
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
// RAVEN BEGIN
// cnicholson: added stop sound support
	void					Event_SetStoppedSound( const char *sound );
// RAVEN END
	void					Event_SetMoveSound( const char *sound );
	void					Event_FindGuiTargets( void );
	void					Event_InitGuiTargets( void );
	void					Event_EnableSplineAngles( void );
	void					Event_DisableSplineAngles( void );
	void					Event_RemoveInitialSplineAngles( void );
	void					Event_StartSpline( idEntity *splineEntity );
	void					Event_StopSpline( void );
	void					Event_Activate( idEntity *activator );
// RAVEN BEGIN
	void					Event_PostRestoreExt( int start, int total, int accel, int decel, bool useSplineAng );
// RAVEN END
	void					Event_IsMoving( void );
	void					Event_IsRotating( void );
// RAVEN BEGIN
// abahr:
	void					Event_GetSplineEntity();
	void					Event_MoveAlongVector( const idVec3& vec );

// mekberg: spline states
	CLASS_STATES_PROTOTYPE ( idMover );

	stateResult_t			State_Accel( const stateParms_t& parms );
	stateResult_t			State_Linear( const stateParms_t& parms );
	stateResult_t			State_Decel( const stateParms_t& parms );
// RAVEN END
};

class idSplinePath : public idEntity {
public:
	CLASS_PROTOTYPE( idSplinePath );

							idSplinePath();
							~idSplinePath();

	void					Spawn( void );

// RAVEN BEGIN
// abahr: so we can ignore these if needed
	void					Toggle() { SetActive(!active); }
	void					Activate() { SetActive(true); }
	void					Deactivate() { SetActive(false); }

	bool					IsActive() const { return active; }

	int						SortTargets();
	int						SortBackwardsTargets();
	int						SortTargets( idList< idEntityPtr<idEntity> >& list );
	void					RemoveNullTargets( void );
	void					ActivateTargets( idEntity *activator ) const;

	void					RemoveNullTargets( idList< idEntityPtr<idEntity> >& list );
	void					ActivateTargets( idEntity *activator, const idList< idEntityPtr<idEntity> >& list ) const;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	idList< idEntityPtr<idEntity> >	backwardPathTargets;

// mekberg: spline sampling
	float					GetSampledTime ( float distance ) const;

protected:
	void					SetActive( bool activate ) { active = activate; }
	virtual void			FindTargets();

	void					Event_Toggle( idEntity* activator ) { Toggle(); }
	void					Event_IsActive();

// mekberg: spline sampling
	void					SampleSpline ( void );

protected:
	bool					active;

// mekberg: sample splines.
	float*					sampledTimes;
	float					sampledSpeed;
	int						numSamples;
// RAVEN END
};


struct floorInfo_s {
	idVec3					pos;
	idStr					door;
	int						floor;
};

class idElevator : public idMover {
public:
	CLASS_PROTOTYPE( idElevator );

							idElevator( void );

	void					Spawn();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual bool			HandleSingleGuiCommand	( idEntity *entityGui, idLexer *src );
	floorInfo_s *			GetFloorInfo			( int floor );

protected:
	virtual void			DoneMoving( void );
	virtual void			BeginMove( idThread *thread = NULL );
	void					SpawnTrigger( const idVec3 &pos );
	void					GetLocalTriggerPosition();
	void					Event_Touch( idEntity *other, trace_t *trace );

// RAVEN BEGIN
// bdube: added
	void					SetAASAreaState	 ( bool enable );
	void					InitStatusGui	 ( idUserInterface* gui );
	void					UpdateStatusGui	 ( idUserInterface* gui );
	void					UpdateStatusGuis ( void );
	void					UpdateFloorInfo	 ( void );
// RAVEN END

private:
	typedef enum {
		INIT,
		IDLE,
		WAITING_ON_DOORS
	} elevatorState_t;

	elevatorState_t			state;
	idList<floorInfo_s>		floorInfo;
	int						currentFloor;
	int						pendingFloor;
	int						lastFloor;
	bool					controlsDisabled;
//	bool					waitingForPlayerFollowers;
	float					returnTime;
	int						returnFloor;
	int						lastTouchTime;

	class idDoor *			GetDoor( const char *name );
	void					Think( void );
	void					OpenInnerDoor( void );
	void					OpenFloorDoor( int floor );
	void					CloseAllDoors( void );
	void					DisableAllDoors( void );
	void					EnableProperDoors( void );

	void					Event_TeamBlocked		( idEntity *blockedEntity, idEntity *blockingEntity );
	void					Event_Activate			( idEntity *activator );
	void					Event_PostFloorArrival	( void );
	void					Event_GotoFloor			( int floor );
	void					Event_UpdateFloorInfo	( void );
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
							~idMover_Binary();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			PreBind( void );
	virtual void			PostBind( void );

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

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

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
	bool					deferedOpen;
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

class idDoor : public idMover_Binary {
public:
	CLASS_PROTOTYPE( idDoor );

							idDoor( void );
							~idDoor( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			PreBind( void );
	virtual void			PostBind( void );
	virtual void			Hide( void );
	virtual void			Show( void );

	bool					IsOpen( void );
	bool					IsNoTouch( void );
	int						IsLocked( void );
	void					Lock( int f );
	void					Use( idEntity *other, idEntity *activator );
	void					Close( void );
	void					Open( void );
	void					SetCompanion( idDoor *door );

// RAVEN BEGIN
// abahr:
	bool					IsClosed( void ) const { return moverState == MOVER_POS1; }
	void					SetDoorFrameController( idEntity* controller );
	virtual void			ActivateTargets( idEntity *activator ) const;
// RAVEN END

private:
	float					triggersize;
	bool					crusher;
	bool					noTouch;
	bool					aas_area_closed;
	idStr					buddyStr;
	idClipModel *			trigger;
	idClipModel *			sndTrigger;
	int						nextSndTriggerTime;
	idVec3					localTriggerOrigin;
	idMat3					localTriggerAxis;
	idStr					requires;
	int						removeItem;
	idStr					syncLock;
	int						normalAxisIndex;		// door faces X or Y for spectator teleports
	idDoor *				companionDoor;

// RAVEN BEGIN
// abahr: so we can route calls through a tramGate
	idEntityPtr<idEntity>	doorFrameController;
// RAVEN END

	void					SetAASAreaState( bool closed );

	void					GetLocalTriggerPosition( const idClipModel *trigger );
	void					CalcTriggerBounds( float size, idBounds &bounds );

	void					Event_Reached_BinaryMover( void );
	void					Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity );
	void					Event_PartBlocked( idEntity *blockingEntity );
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					Event_Activate( idEntity *activator );
	void					Event_StartOpen( void );
	void					Event_SpawnDoorTrigger( void );
	void					Event_SpawnSoundTrigger( void );
	void					Event_Close( void );
	void					Event_Open( void );
	void					Event_Lock( int f );
	void					Event_IsOpen( void );
	void					Event_Locked( void );
	void					Event_SpectatorTouch( idEntity *other, trace_t *trace );
	void					Event_OpenPortal( void );
	void					Event_ClosePortal( void );

// RAVEN BEGIN
// abahr:
	void					Event_ReturnToPos1( void );
// RAVEN END
};

class idPlat : public idMover_Binary {
public:
	CLASS_PROTOTYPE( idPlat );

							idPlat( void );
							~idPlat( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			PreBind( void );
	virtual void			PostBind( void );

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
							~idMover_Periodic( void );

	void					Spawn( void );
	
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

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

private:
	idEntityPtr<idEntity>	activatedBy;

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

// RAVEN BEGIN
// bdube: conveyor belts
class rvConveyor : public idEntity {
public:
	CLASS_PROTOTYPE( rvConveyor );

							rvConveyor ( void );

	void					Spawn( void );
	void					Think ( void );
	
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
private:

	idVec3					moveDir;
	float					moveSpeed;
	
	void					Event_FindTargets	 ( void );
};

// nrausch: 
class rvPusher : public idMover {
public:
	CLASS_PROTOTYPE( rvPusher );

	rvPusher( void );
	~rvPusher( void );

	virtual void Spawn( void );
	virtual void Think ( void );

private:
	jointHandle_t bindJointHandle;
	idEntity *parent;
	idVec3 pusherOrigin;
	idMat3 pusherAxis;
};

// RAVEN END

#endif /* !__GAME_MOVER_H__ */
