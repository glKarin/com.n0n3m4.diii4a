// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#ifndef __GAME_ACTOR_H__
#define __GAME_ACTOR_H__

/*
===============================================================================

	idActor

===============================================================================
*/

extern const idEventDef AI_EnableEyeFocus;
extern const idEventDef AI_DisableEyeFocus;
extern const idEventDef EV_Footstep;
extern const idEventDef EV_FootstepLeft;
extern const idEventDef EV_FootstepRight;
extern const idEventDef EV_EnableWalkIK;
extern const idEventDef EV_DisableWalkIK;
extern const idEventDef EV_EnableLegIK;
extern const idEventDef EV_DisableLegIK;
extern const idEventDef AI_SetAnimPrefix;
extern const idEventDef AI_PlayAnim;
extern const idEventDef AI_PlayCycle;
extern const idEventDef AI_AnimDone;
extern const idEventDef AI_SetBlendFrames;
extern const idEventDef AI_GetBlendFrames;
extern const idEventDef AI_ScriptedMove;
extern const idEventDef AI_ScriptedDone;
extern const idEventDef AI_ScriptedStop;

// RAVEN BEGIN
// bdube: added flashlight
extern const idEventDef AI_Flashlight;
extern const idEventDef AI_EnterVehicle;
extern const idEventDef AI_ExitVehicle;
// nmckenzie:
extern const idEventDef AI_OverrideAnim;
extern const idEventDef AI_IdleAnim;
extern const idEventDef AI_SetState;
// jshepard: adjust animation speed
extern const idEventDef AI_SetAnimRate;
//MCG: damage over time
extern const idEventDef EV_DamageOverTime;
extern const idEventDef EV_DamageOverTimeEffect;
//MCG: script-callable joint crawl effect
extern const idEventDef EV_JointCrawlEffect;

// abahr:
extern const idEventDef AI_LookAt;
extern const idEventDef AI_FaceEnemy;
extern const idEventDef AI_FaceEntity;
extern const idEventDef	AI_JumpDown;
extern const idEventDef AI_SetLeader;
// RAVEN END

class idAnimState {
public:

	bool					idleAnim;
	int						animBlendFrames;
	int						lastAnimBlendFrames;		// allows override anims to blend based on the last transition time

public:
							idAnimState();
							~idAnimState();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Init( idEntity *owner, idAnimator *_animator, int animchannel );
	void					Shutdown( void );
	void					SetState		( const char *name, int blendFrames, int flags = 0 );
	void					PostState		( const char* name, int blendFrames = 0, int delay = 0, int flags = 0 );
	void					StopAnim( int frames );
	void					PlayAnim( int anim );
	void					CycleAnim( int anim );
	void					BecomeIdle( void );
	bool					UpdateState( void );
	bool					Disabled( void ) const;
	void					Enable( int blendFrames );
	void					Disable( void );
	bool					AnimDone( int blendFrames ) const;
	bool					IsIdle( void ) const;
	animFlags_t				GetAnimFlags( void ) const;

	rvStateThread&			GetStateThread	( void );

	idAnimator *			GetAnimator( void ) const {return animator;};
private:
// RAVEN BEGIN
// bdube: converted self to entity ptr so any entity can use it
	idEntity *				self;
// RAVEN END
	idAnimator *			animator;
	int						channel;
	bool					disabled;
	
	rvStateThread			stateThread;
};

ID_INLINE rvStateThread& idAnimState::GetStateThread ( void ) {
	return stateThread;
}

class idAttachInfo {
public:
	idEntityPtr<idEntity>	ent;
	int						channel;
};

class idActor : public idAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( idActor );

	int						team;
	idLinkList<idActor>		teamNode;
	int						rank;				// monsters don't fight back if the attacker's rank is higher
	idMat3					viewAxis;			// view axis of the actor

	idLinkList<idActor>		enemyNode;			// node linked into an entity's enemy list for quick lookups of who is attacking him
	idLinkList<idActor>		enemyList;			// list of characters that have targeted the player as their enemy

public:
							idActor( void );
	virtual					~idActor( void );

	void					Spawn( void );
	virtual void			Restart( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Hide( void );
	virtual void			Show( void );
	virtual int				GetDefaultSurfaceType( void ) const;
	virtual void			ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material );

	virtual bool			LoadAF( const char* keyname = NULL, bool purgeAF = false );
	void					SetupBody( void );

	virtual void			CheckBlink( void );

	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

							// script state management
	void					ShutdownThreads		( void );
	void					UpdateState			( void );
	
	virtual void			OnStateThreadClear ( const char *statename, int flags = 0 );
	void					SetState		( const char *statename, int flags = 0 );
	void					PostState		( const char* statename, int delay = 0, int flags = 0 );
	void					InterruptState	( const char* statename, int delay = 0, int flags = 0 );

							// vision testing
	void					SetEyeHeight( float height );
	void					SetChestHeight ( float height );
	float					EyeHeight( void ) const;

	virtual idVec3			GetEyePosition( void ) const;
	virtual idVec3			GetChestPosition ( void ) const;
	idEntity*				GetGroundEntity ( void ) const;
	virtual idEntity*		GetGroundElevator( idEntity* testElevator=NULL ) const;

	void					Present( void );

	virtual void			GetViewPos	( idVec3 &origin, idMat3 &axis ) const;
	void					SetFOV		( float fov, float fovClose );
	bool					CheckFOV	( const idVec3 &pos, float ang = -1.0f ) const;
	virtual bool			HasFOV		( idEntity *ent );
	virtual	bool			CanSee		( const idEntity *ent, bool useFOV ) const;
	virtual	bool			CanSeeFrom	( const idVec3& from, const idEntity *ent, bool useFOV ) const;
	virtual	bool			CanSeeFrom	( const idVec3& from, const idVec3& toPos, bool useFOV ) const;

							// damage
	void					SetupDamageGroups( void );

	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
// RAVEN BEGIN
// nmckenzie: a final hook in the middle of the damage function
	virtual void			AdjustHealthByDamage ( int inDamage ){health -= inDamage;}
// RAVEN END

	virtual int				GetDamageForLocation( int damage, int location );
	const char *			GetDamageGroup( int location );
	void					ClearPain( void );
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor );

							// model/combat model/ragdoll
	void					SetCombatModel( void );
	idClipModel *			GetCombatModel( void ) const;
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );
	bool					StartRagdoll( void );
	void					StopRagdoll( void );
	virtual bool			UpdateAnimationControllers( void );

							// delta view angles to allow movers to rotate the view of the actor
	const idAngles &		GetDeltaViewAngles( void ) const;
	void					SetDeltaViewAngles( const idAngles &delta );

	bool					HasEnemies( void ) const;
	idActor *				ClosestEnemyToPoint( const idVec3 &pos, float maxRange=0.0f, bool returnFirst=false, bool checkPVS=false );
	idActor *				EnemyWithMostHealth();

	virtual bool			OnLadder			( void ) const;
	virtual void			OnStateChange		( int channel );
	virtual void			OnFriendlyFire		( idActor* attacker );

	virtual void			GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const;

	void					Attach( idEntity *ent );
	idEntity*				FindAttachment( const char* attachmentName );
	void					HideAttachment( const char* attachmentName );
	void					ShowAttachment( const char* attachmentName );
	idEntity*				GetHead() { return head; }

	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );

	virtual	renderView_t *	GetRenderView();	

	// Animation
	int						PlayAnim				( int channel, const char *name, int blendFrames );
	bool					PlayCycle				( int channel, const char *name, int blendFrames );
	void					IdleAnim				( int channel, const char *name, int blendFrames );
	void					OverrideAnim			( int channel );
	bool					HasAnim					( int channel, const char *name, bool forcePrefix = false );
	int						GetAnim					( int channel, const char *name, bool forcePrefix = false );
	bool					AnimDone				( int channel, int blendFrames );
	
	// animation state control
	void					UpdateAnimState			( void );
	void					SetAnimState			( int channel, const char *name, int blendFrames = 0, int flags = 0 );
	void					PostAnimState			( int channel, const char *name, int blendFrames = 0, int delay = 0, int flags = 0 );
	void					StopAnimState			( int channel );
	bool					InAnimState				( int channel, const char *name );
	
	virtual void			SpawnGibs( const idVec3 &dir, const char *damageDefName );

// RAVEN BEGIN
// bdube: added for vehicle
	bool					IsInVehicle ( void ) const;
	rvVehicleController&	GetVehicleController ( void );
	virtual void			GuidedProjectileIncoming( idGuidedProjectile * projectile );

	bool					DebugFilter	(const idCVar& test) const;
// RAVEN END
	virtual bool			IsCrouching				( void ) const {return false;};

	virtual bool			SkipImpulse( idEntity* ent, int id );

	int						lightningNextTime;
	int						lightningEffects;
#ifdef _QUAKE4
	bool					PointVisible(const idVec3& point) const;
#endif


protected:
	friend class			idAnimState;

	float					fovDot;				// cos( fovDegrees )
	float					fovCloseDot;		// cos( fovDegreesClose )
	float					fovCloseRange;		// range within to use fovCloseDot
	idVec3					eyeOffset;			// offset of eye relative to physics origin
	idVec3					chestOffset;		// offset of chest relative to physics origin
	idVec3					modelOffset;		// offset of visual model relative to the physics origin

	idAngles				deltaViewAngles;	// delta angles relative to view input angles

	int						pain_debounce_time;	// next time the actor can show pain
	int						pain_delay;			// time between playing pain sound

	idStrList				damageGroups;		// body damage groups
	idList<float>			damageScale;		// damage scale per damage gruop
	bool					inDamageEvent;		// hacky-ass bool to prevent us from starting a new EV_DamageOverTime in our ::Damage

	bool					use_combat_bbox;	// whether to use the bounding box for combat collision
	
	// joint handles
	jointHandle_t			leftEyeJoint;
	jointHandle_t			rightEyeJoint;
	jointHandle_t			soundJoint;
	jointHandle_t			eyeOffsetJoint;
	jointHandle_t			chestOffsetJoint;
	jointHandle_t			neckJoint;
	jointHandle_t			headJoint;

	idIK_Walk				walkIK;

	idStr					animPrefix;
	idStr					painType;
	idStr					painAnim;

	// blinking
	int						blink_anim;
	int						blink_time;
	int						blink_min;
	int						blink_max;

	idAnimState				headAnim;
	idAnimState				torsoAnim;
	idAnimState				legsAnim;
	
	rvStateThread			stateThread;

	idEntityPtr<idAFAttachment>	head;				// safe pointer to attached head

	bool					disablePain;
	bool					allowEyeFocus;
	bool					finalBoss;

	int						painTime;

	idList<idAttachInfo>	attachments;

	virtual void			Gib( const idVec3 &dir, const char *damageDefName );
	void					CheckDeathObjectives( void );

// RAVEN BEGIN
// bdube: vehicles
	virtual bool			EnterVehicle ( idEntity* vehicle );
	virtual bool			ExitVehicle	 ( bool force = false );
// RAVEN END

							// removes attachments with "remove" set for when character dies
	void					RemoveAttachments( void );
	
// RAVEN BEGIN
// bdube: vehicles
	rvVehicleController		vehicleController;
// bdube: flashlights
	renderLight_t			flashlight;
	int						flashlightHandle;
	jointHandle_t			flashlightJoint;
	idVec3					flashlightOffset;

// bdube: death force
	int						deathPushTime;
	idVec3					deathPushForce;
	jointHandle_t			deathPushJoint;

	void					FlashlightUpdate	( bool forceOn = false );
	void					InitDeathPush		( const idVec3& dir, int location, const idDict* damageDict, float pushScale = 1.0f );
	void					DeathPush			( void );

	// Add some dynamic externals for debugging
	virtual void			GetDebugInfo		( debugInfoProc_t proc, void* userData );
// RAVEN END	

protected:

	virtual void			FootStep			( void );
	virtual void			SetupHead( const char* headDefName = "", idVec3 headOffset = idVec3(0, 0, 0) );

private:
	void					SyncAnimChannels( int channel, int syncToChannel, int blendFrames );
	void					FinishSetup( void );
	
	void					Event_EnableEyeFocus( void );
	void					Event_DisableEyeFocus( void );
	void					Event_EnableBlink( void );
	void					Event_DisableBlink( void );
	void					Event_Footstep( void );
	void					Event_EnableWalkIK( void );
	void					Event_DisableWalkIK( void );
	void					Event_EnableLegIK( int num );
	void					Event_DisableLegIK( int num );
	void					Event_SetAnimPrefix( const char *name );
	void					Event_LookAtEntity( idEntity *ent, float duration );
	void					Event_PreventPain( float duration );
	void					Event_DisablePain( void );
	void					Event_EnablePain( void );
	void					Event_StopAnim( int channel, int frames );
	void					Event_PlayAnim( int channel, const char *name );
	void					Event_PlayCycle( int channel, const char *name );
	void					Event_IdleAnim( int channel, const char *name );
	void					Event_SetSyncedAnimWeight( int channel, int anim, float weight );
	void					Event_OverrideAnim( int channel );
	void					Event_EnableAnim( int channel, int blendFrames );
	void					Event_SetBlendFrames( int channel, int blendFrames );
	void					Event_GetBlendFrames( int channel );
	void					Event_HasEnemies( void );
	void					Event_NextEnemy( idEntity *ent );
	void					Event_ClosestEnemyToPoint( const idVec3 &pos );
	void					Event_StopSound( int channel, int netsync );
	void					Event_GetHead( void );

	void					Event_Teleport		( idVec3 &newPos, idVec3 &newAngles );
	void					Event_Flashlight	( bool enable );
	void					Event_EnterVehicle	( idEntity* vehicle );
	void					Event_ExitVehicle	( bool force );
	void					Event_PreExitVehicle( bool force );

	void					Event_SetAnimRate	( float multiplier );
	void					Event_DamageOverTime ( int endTime, int interval, idEntity *inflictor, idEntity *attacker, idVec3 &dir, const char *damageDefName, const float damageScale, int location );
	virtual void			Event_DamageOverTimeEffect	( int endTime, int interval, const char *damageDefName );
	void					Event_JointCrawlEffect ( const char *effectKeyName, float crawlSecs );

	CLASS_STATES_PROTOTYPE ( idActor );

protected:

	// Wait states
	stateResult_t			State_Wait_LegsAnim		( const stateParms_t& parms );
	stateResult_t			State_Wait_TorsoAnim	( const stateParms_t& parms );
	stateResult_t			State_Wait_Frame		( const stateParms_t& parms );

	void					DisableAnimState		( int channel );
	void					EnableAnimState			( int channel );
	idAnimState&			GetAnimState			( int channel );
};

ID_INLINE bool idActor::IsInVehicle( void ) const {
	return vehicleController.IsDriving();
}

ID_INLINE rvVehicleController& idActor::GetVehicleController( void ) {
	return vehicleController;
}

#endif /* !__GAME_ACTOR_H__ */

// RAVEN END
