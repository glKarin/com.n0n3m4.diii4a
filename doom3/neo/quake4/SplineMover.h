#ifndef __RV_SPLINE_MOVER_H
#define __RV_SPLINE_MOVER_H

extern const idEventDef EV_SetSpline;

struct splinePState_t {
	idVec3					origin;
	idVec3					localOrigin;
	idMat3					axis;
	idMat3					localAxis;

	float					speed;
	float					idealSpeed;
	float					dist;

	float					acceleration;
	float					deceleration;

	bool					ShouldAccelerate() const;
	bool					ShouldDecelerate() const;
	
	void					ApplyAccelerationDelta( float timeStepSec );

	void					ApplyDecelerationDelta( float timeStepSec );

	void					UpdateDist( float timeStepSec );
	void					Clear();
	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	splinePState_t&			Assign( const splinePState_t* state );
	splinePState_t&			operator=( const splinePState_t& state );
	splinePState_t&			operator=( const splinePState_t* state );
};

//=======================================================
//
//	rvPhysics_Spline
//
//=======================================================
class rvPhysics_Spline : public idPhysics_Base {

public:

	CLASS_PROTOTYPE( rvPhysics_Spline );

							rvPhysics_Spline( void );
	virtual					~rvPhysics_Spline( void );

	void					Save( idSaveGame *savefile ) const;
	void					Event_PostRestore( void );
	void					Restore( idRestoreGame *savefile );

	void					SetSpline( idCurve_Spline<idVec3>* spline );
	const idCurve_Spline<idVec3>* GetSpline() const { return spline; }
	idCurve_Spline<idVec3>* GetSpline() { return spline; }

	void					SetSplineEntity( idSplinePath* spline );
	const idSplinePath*		GetSplineEntity() const { return splineEntity; }
	idSplinePath*			GetSplineEntity() { return splineEntity; }

	void					SetLinearAcceleration( const float accel );
	void					SetLinearDeceleration( const float decel );

	void					SetSpeed( float speed );
	float					GetSpeed( void ) const;

	virtual bool			StartingToMove( void ) const;
	virtual bool			StoppedMoving( void ) const;

	float					ComputeDecelFromSpline( void ) const;

public:	// common physics interface
	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const idBounds &		GetBounds( int id = -1 ) const;
	const idBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	bool					EvaluateSpline( idVec3& newOrigin, idMat3& newAxis, const splinePState_t& previous );
	bool					EvaluateMaster( idVec3& newOrigin, idMat3& newAxis, const splinePState_t& previous );

	void					Activate( void );
	void					Rest( void );

	bool					IsAtRest( void ) const;
	bool					IsAtEndOfSpline( void ) const;
	bool					IsAtBeginningOfSpline( void ) const;

	virtual bool			IsPushable( void ) const;

	bool					HasValidSpline( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void					SetAxis( const idMat3 &newAxis, int id = -1 );

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	const idVec3 &			GetOrigin( int id = 0 ) const;
	const idMat3 &			GetAxis( int id = 0 ) const;

	idVec3 &				GetOrigin( int id = 0 );
	idMat3 &				GetAxis( int id = 0 );

	void					SetMaster( idEntity *master, const bool orientated );

	void					ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const;
	void					ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const;
	int						ClipContents( const idClipModel *model ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual const trace_t*	GetBlockingInfo( void ) const;
	virtual idEntity*		GetBlockingEntity( void ) const;

public:
	stateResult_t			State_Accelerating( const stateParms_t& parms );
	stateResult_t			State_Decelerating( const stateParms_t& parms );
	stateResult_t			State_Cruising( const stateParms_t& parms );

protected:
	const idVec3 &			GetLocalOrigin( int id = 0 ) const;
	const idMat3 &			GetLocalAxis( int id = 0 ) const;

	idVec3 &				GetLocalOrigin( int id = 0 );
	idMat3 &				GetLocalAxis( int id = 0 );

protected:
	splinePState_t			current;
	splinePState_t			saved;

	float					splineLength;
	idCurve_Spline<idVec3>*	spline;
	idEntityPtr<idSplinePath> splineEntity;

	trace_t					pushResults;

	idClipModel*			clipModel;

	rvStateThread			accelDecelStateThread;

	CLASS_STATES_PROTOTYPE( rvPhysics_Spline );
};

extern const idEventDef EV_DoneMoving;

//=======================================================
//
//	rvSplineMover
//
//=======================================================
class rvSplineMover : public idAnimatedEntity {
	CLASS_PROTOTYPE( rvSplineMover );

public:
	void					Spawn();
	virtual					~rvSplineMover();

	virtual void			SetSpeed( float newSpeed );
	virtual float			GetSpeed() const;
	virtual void			SetIdealSpeed( float newIdealSpeed );
	virtual float			GetIdealSpeed() const;

	virtual void			SetSpline( idSplinePath* spline );
	virtual const idSplinePath*	GetSpline() const;
	virtual idSplinePath*	GetSpline();

	virtual void			SetAcceleration( float accel );
	virtual void			SetDeceleration( float decel );

	virtual void			CheckSplineForOverrides( const idCurve_Spline<idVec3>* spline, const idDict* args );
	virtual void			RestoreFromOverrides( const idDict* args );

	int						PlayAnim( int channel, const char* animName, int blendFrames );
	void					CycleAnim( int channel, const char* animName, int blendFrames );
	void					ClearChannel( int channel, int clearFrame );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

protected:// TramCar utility functions
	void					AddSelfToGlobalList();
	void					RemoveSelfFromGlobalList();
	bool					InGlobalList() const;
	bool					WhosVisible( const idFrustum& frustum, idList<rvSplineMover*>& list ) const;

protected:
	virtual idStr			GetTrackInfo( const idSplinePath* track ) const;
	rvSplineMover*			ConvertToMover( idEntity* mover ) const;
	idSplinePath*			ConvertToSplinePath( idEntity* spline ) const;

	void					CallScriptEvents( const idSplinePath* spline, const char* prefixKey, idEntity* parm );
	virtual	void			PreBind();

	virtual void			PreDoneMoving();
	virtual void			PostDoneMoving();

protected:
	void					Event_PostSpawn();

	void					Event_SetSpline( idEntity* spline );
	void					Event_GetSpline();

	void					Event_SetAcceleration( float accel );
	void					Event_SetDeceleration( float decel );

	void					Event_OnAcceleration();
	void					Event_OnDeceleration();
	void					Event_OnCruising();

	void					Event_OnStopMoving();
	void					Event_OnStartMoving();

	void					Event_SetSpeed( float speed );
	void					Event_GetSpeed();
	void					Event_SetIdealSpeed( float speed );
	void					Event_GetIdealSpeed();
	void					Event_ApplySpeedScale( float scale );

	void					Event_SetCallBack();
	void					Event_DoneMoving();

	void					Event_GetCurrentTrackInfo();
	void					Event_GetTrackInfo( idEntity* track );

	void					Event_Activate( idEntity* activator );

	void					Event_StartSoundPeriodic( const char* sndKey, const s_channelType channel, int minDelay, int maxDelay );
	void					Event_PartBlocked( idEntity *blockingEntity );

protected:
	rvPhysics_Spline		physicsObj;

	float					idealSpeed;

	int						waitThreadId;

private:
	idLinkList<rvSplineMover>	splineMoverNode;
	static idLinkList<rvSplineMover>	splineMovers;
};

//=======================================================
//
//	rvTramCar
//
//=======================================================
class rvTramCar : public rvSplineMover {
	CLASS_PROTOTYPE( rvTramCar );

public:
	void				Spawn();
	virtual				~rvTramCar();

	virtual void		Think();
	virtual void		MidThink();

	virtual float		GetNormalSpeed() const { return spawnArgs.GetFloat("normalSpeed"); }

	virtual void		SetIdealTrack( int track );
	virtual int			GetIdealTrack() const;

	virtual void		AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor );
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void		Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	virtual float		GetDamageScale() const;
	virtual float		GetHealthScale() const;

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				WriteToSnapshot( idBitMsgDelta &msg ) const;
	void				ReadFromSnapshot( const idBitMsgDelta &msg );

public:
	stateResult_t		State_Idle( const stateParms_t& parms );
	stateResult_t		State_NormalSpeed( const stateParms_t& parms );
	stateResult_t		State_ExcessiveSpeed( const stateParms_t& parms );

	stateResult_t		State_RandomTrack( const stateParms_t& parms );
	stateResult_t		State_AssignedTrack( const stateParms_t& parms );

protected:
	void				SpawnDriver( const char* driverKey );
	void				SpawnWeapons( const char* partKey );
	void				SpawnOccupants( const char* partKey );
	idEntity*			SpawnPart( const char* partDefName, const char* subPartDefName );
	void				SpawnDoors();
	idMover*			SpawnDoor( const char* key );

	int					SortSplineTargets( idSplinePath* spline ) const;
	idEntity*			GetSplineTarget( idSplinePath* spline, int index ) const;

	void				HeadTowardsIdealTrack();
	idSplinePath*		FindSplineToTrack( idSplinePath* spline, const idStr& track ) const;
	idSplinePath*		FindSplineToIdealTrack( idSplinePath* spline ) const;
	idSplinePath*		GetRandomSpline( idSplinePath* spline ) const;

	char				ConvertToTrackLetter( int trackNum ) const { return trackNum + 'A'; }
	int					ConvertToTrackNumber( char trackLetter ) const { return trackLetter - 'A'; }
	int					ConvertToTrackNumber( const idStr& trackInfo ) const { return ConvertToTrackNumber(idStr::ToUpper(trackInfo.Right(1)[0])); }
	int					GetCurrentTrack() const;

	virtual void		UpdateChannel( const s_channelType channel, const soundShaderParms_t& parms );
	virtual void		AttenuateTrackChannel( float attenuation );
	virtual void		AttenuateTramCarChannel( float attenuation );

	virtual void		RegisterStateThread( rvStateThread& stateThread, const char* name );

	bool				LookForward( idList<rvSplineMover*>& list ) const;
	bool				LookLeft( idList<rvSplineMover*>& list ) const;
	bool				LookRight( idList<rvSplineMover*>& list ) const;

	bool				LookLeftForTrackChange( idList<rvSplineMover*>& list ) const;
	bool				LookRightForTrackChange( idList<rvSplineMover*>& list ) const;

	bool				Look( const idFrustum& fov, idList<rvSplineMover*>& list ) const;

	virtual void		LookAround();
	virtual bool		AdjustSpeed( idList<rvSplineMover*>& moverList );

	virtual bool		OnSameTrackAs( const rvSplineMover* tram ) const;
	virtual bool		SameIdealTrackAs( const rvSplineMover* tram ) const;

	virtual idEntity*	DriverSpeak( const char *speechDecl, bool random = false );
	virtual idEntity*	OccupantSpeak( const char *speechDecl, bool random = false );

	virtual void		PostDoneMoving();

	virtual void		DeployRamp();
	virtual void		RetractRamp();
	void				OperateRamp( const char* operation );
	void				OperateRamp( const char* operation, idMover* door );

protected:
	void				Event_DriverSpeak( const char* voKey );
	void				Event_GetDriver();
	void				Event_Activate( idEntity* activator );
	void				Event_RadiusDamage( const idVec3& origin, const char* damageDefName );
	void				Event_SetIdealTrack( const char* track );

	void				Event_OnStopMoving();
	void				Event_OnStartMoving();

	void				Event_OpenDoors();
	void				Event_CloseDoors();

	void				Event_SetHealth( float health );

protected:
	int					idealTrack;
	idStr				idealTrackTag;// HACK

	mutable idFrustum	collisionFov;

	rvStateThread		idealTrackStateThread;
	rvStateThread		speedSoundEffectsStateThread;

	idEntityPtr<idAI>					driver;
	idList< idEntityPtr<idAI> >			occupants;
	idList< idEntityPtr<rvVehicle> >	weapons;

	int									numTracksOnMap;

	idEntityPtr<idMover>				leftDoor;
	idEntityPtr<idMover>				rightDoor;

	CLASS_STATES_PROTOTYPE( rvTramCar );
};

//=======================================================
//
//	rvTramCar_Marine
//
//=======================================================
class rvTramCar_Marine : public rvTramCar {
	CLASS_PROTOTYPE( rvTramCar_Marine );

public:
	void				Spawn();

	virtual void		MidThink();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

public:
	stateResult_t		State_Occupied( const stateParms_t& parms );
	stateResult_t		State_NotOccupied( const stateParms_t& parms );

	stateResult_t		State_UsingMountedGun( const stateParms_t& parms );
	stateResult_t		State_NotUsingMountedGun( const stateParms_t& parms );

protected:
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void		Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	virtual void		LookAround();

	bool				LookOverLeftShoulder( idList<rvSplineMover*>& list );
	bool				LookOverRightShoulder( idList<rvSplineMover*>& list );

	bool				EntityIsInside( const idEntity* entity ) const;
	bool				PlayerIsInside() const { return EntityIsInside(gameLocal.GetLocalPlayer()); }

	void				ActivateTramHud( idPlayer* player );
	void				DeactivateTramHud( idPlayer* player );
	void				UpdateTramHud( idPlayer* player );

	virtual void		DeployRamp();
	virtual void		RetractRamp();

	void				UseMountedGun( idPlayer* player );

	void				Event_UseMountedGun( idEntity* ent );
	void				Event_SetPlayerDamageEntity(float f);	

protected:
	idList< idEntityPtr<rvSplineMover> > visibleEnemies;

	rvStateThread		playerOccupationStateThread;
	rvStateThread		playerUsingMountedGunStateThread;

	int						maxHealth;
	int						lastHeal;
	int						healDelay;
	int						healAmount;

	CLASS_STATES_PROTOTYPE( rvTramCar_Marine );
};

//=======================================================
//
//	rvTramCar_Strogg
//
//=======================================================
class rvTramCar_Strogg : public rvTramCar {
	CLASS_PROTOTYPE( rvTramCar_Strogg );

public:
	void				Spawn();

	virtual void		SetTarget( idEntity* newTarget );
	virtual const rvSplineMover*	GetTarget() const;

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				WriteToSnapshot( idBitMsgDelta &msg ) const;
	void				ReadFromSnapshot( const idBitMsgDelta &msg );

public:
	stateResult_t		State_LookingForTarget( const stateParms_t& parms );
	stateResult_t		State_TargetInSight( const stateParms_t& parms );

protected:	
	bool				TargetIsToLeft();
	bool				TargetIsToRight();

	virtual void		LookAround();

protected:
	void				Event_PostSpawn();

protected:
	idEntityPtr<rvSplineMover> target;

	rvStateThread		targetSearchStateThread;

	CLASS_STATES_PROTOTYPE( rvTramCar_Strogg );
};

#endif
