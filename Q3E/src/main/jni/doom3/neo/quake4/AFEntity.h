
#ifndef __GAME_AFENTITY_H__
#define __GAME_AFENTITY_H__


/*
===============================================================================

idMultiModelAF

Entity using multiple separate visual models animated with a single
articulated figure. Only used for debugging!

===============================================================================
*/
const int GIB_DELAY = 200;  // only gib this often to keep performace hits when blowing up several mobs

class idMultiModelAF : public idEntity {
public:
	CLASS_PROTOTYPE( idMultiModelAF );

	void					Spawn( void );
							~idMultiModelAF( void );

	virtual void			Think( void );
	virtual void			Present( void );

protected:
	idPhysics_AF			physicsObj;

	void					SetModelForId( int id, const idStr &modelName );

private:
	idList<idRenderModel *>	modelHandles;
	idList<int>				modelDefHandles;
};


/*
===============================================================================

idChain

Chain hanging down from the ceiling. Only used for debugging!

===============================================================================
*/

class idChain : public idMultiModelAF {
public:
	CLASS_PROTOTYPE( idChain );

	void					Spawn( void );

protected:
	void					BuildChain( const idStr &name, const idVec3 &origin, float linkLength, float linkWidth, float density, int numLinks, bool bindToWorld = true );
};


/*
===============================================================================

idAFAttachment

===============================================================================
*/

typedef struct {
	jointModTransform_t		mod;
	jointHandle_t			from;
	jointHandle_t			to;
} copyJoints_t;

class idAFAttachment : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( idAFAttachment );

							idAFAttachment( void );
	virtual					~idAFAttachment( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					SetBody			( idAnimatedEntity* body, const char *headModel, jointHandle_t damageJoint );
	void					SetDamageJoint	( jointHandle_t damageJoint );
	void					ClearBody		( void );
	idEntity *				GetBody			( void ) const;

	virtual void			Think						( void );

	virtual void			Hide						( void );
	virtual void			Show						( void );

// RAVEN BEGIN
// bdube: added channel
	virtual bool			UpdateAnimationControllers	( void );

	void					PlayIdleAnim( int channel, int blendTime );

							// Returns the entity that should take damage for this entity
	virtual idEntity*		GetDamageEntity ( void );
							// for getting th speaker position
	virtual	bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

// jshepard: animations for heads
	void					Event_PlayAnim ( int channel, const char *animname );
// jdischler: animations for heads
	void					Event_PlayCycle ( int channel, const char *animname );
	void					Event_ClearAnims ( void );


// RAVEN END

	virtual void			GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info );
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash = false );
	virtual void			AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );

	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual bool			CanPlayImpactEffect ( idEntity* attacker, idEntity* target );
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor );

	void					SetCombatModel( void );
	idClipModel *			GetCombatModel( void ) const;
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

	// Lipsync
	int						StartLipSyncing( const char *speechDecl );
	void					HandleLipSync( void );
	void					EndLipSyncing( void );
	bool					IsLipSyncing( void ) const;

	void					InitCopyJoints			( void );

	void					CopyJointsFromBody		( void );

	bool					GetNoPlayerImpactFX( void );

protected:


	idEntityPtr<idAnimatedEntity>	body;
	idClipModel *					combatModel;	// render model for hit detection of head
	int								idleAnim;
	jointHandle_t					damageJoint;

	jointHandle_t					soundJoint;

	int								lipSyncAnim;	// Anim that contains the visemes
	class rvLipSyncData*			lipSyncData;	// The current instance of lip syncing data

	idList<copyJoints_t>			copyJoints;		// copied from the body animation to the head model

	bool							noPlayerImpactFX;
};

// RAVEN BEGIN
// bdube: inlines
ID_INLINE bool idAFAttachment::IsLipSyncing( void ) const {
	return !!lipSyncData;
}

ID_INLINE void idAFAttachment::SetDamageJoint ( jointHandle_t _damageJoint ) {
	damageJoint = _damageJoint;
}
// RAVEN END

/*
===============================================================================

idAFEntity_Base

===============================================================================
*/

class idAFEntity_Base : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( idAFEntity_Base );

							idAFEntity_Base( void );
	virtual					~idAFEntity_Base( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info );
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash = false );
	virtual void			AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
	virtual bool			CanPlayImpactEffect ( idEntity* attacker, idEntity* target );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual bool			UpdateAnimationControllers( void );
	virtual void			FreeModelDef( void );

	virtual bool			LoadAF( const char* keyname = NULL );
	bool					IsActiveAF( void ) const { return af.IsActive(); }
	const char *			GetAFName( void ) const { return af.GetName(); }
	idPhysics_AF *			GetAFPhysics( void ) { return af.GetPhysics(); }

	void					SetCombatModel( void );
	idClipModel *			GetCombatModel( void ) const;
							// contents of combatModel can be set to 0 or re-enabled (mp)
	void					SetCombatContents( bool enable );
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

	int						BodyForClipModelId( int id ) const;

	void					SaveState( idDict &args ) const;
	void					LoadState( const idDict &args );

	void					AddBindConstraints( void );
	void					RemoveBindConstraints( void );

	virtual void			ShowEditingDialog( void );

	static void				DropAFs( idEntity *ent, const char *type, idList<idEntity *> *list );

	bool					GetNoPlayerImpactFX( void );

protected:
	idAF					af;				// articulated figure
	idClipModel *			combatModel;	// render model for hit detection
	int						combatModelContents;
	idVec3					spawnOrigin;	// spawn origin
	idMat3					spawnAxis;		// rotation axis used when spawned
	int						nextSoundTime;	// next time this can make a sound

	bool					noPlayerImpactFX;

	void					Event_SetConstraintPosition( const char *name, const idVec3 &pos );
};

/*
===============================================================================

idAFEntity_Gibbable

===============================================================================
*/

extern const idEventDef		EV_Gib;
extern const idEventDef		EV_Gibbed;

class idAFEntity_Gibbable : public idAFEntity_Base {
public:
	CLASS_PROTOTYPE( idAFEntity_Gibbable );

							idAFEntity_Gibbable( void );
							~idAFEntity_Gibbable( void );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual void			Present( void );
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			SpawnGibs( const idVec3 &dir, const char *damageDefName );

protected:
	idRenderModel *			skeletonModel;
	int						skeletonModelDefHandle;
	bool					gibbed;

	virtual void			Gib( const idVec3 &dir, const char *damageDefName );
	void					InitSkeletonModel( void );

	void					Event_Gib( const char *damageDefName );
};

/*
===============================================================================

	idAFEntity_Generic

===============================================================================
*/

class idAFEntity_Generic : public idAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( idAFEntity_Generic );

							idAFEntity_Generic( void );
							~idAFEntity_Generic( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );
	void					KeepRunningPhysics( void ) { keepRunningPhysics = true; }

private:
	void					Event_Activate( idEntity *activator );

	bool					keepRunningPhysics;
};


/*
===============================================================================

idAFEntity_WithAttachedHead

===============================================================================
*/

class idAFEntity_WithAttachedHead : public idAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( idAFEntity_WithAttachedHead );

							idAFEntity_WithAttachedHead();
							~idAFEntity_WithAttachedHead();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			SetupHead( const char* headDefName = "" );

	virtual void			Think( void );

	virtual void			Hide( void );
	virtual void			Show( void );
	virtual void			ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material );

	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

protected:
	virtual void			Gib( const idVec3 &dir, const char *damageDefName );

	idEntityPtr<idAFAttachment>	head;				// safe pointer to attached head

private:

	void					Event_Gib( const char *damageDefName );
	void					Event_Activate( idEntity *activator );
};


/*
===============================================================================

idAFEntity_Vehicle

===============================================================================
*/

class idAFEntity_Vehicle : public idAFEntity_Base {
public:
	CLASS_PROTOTYPE( idAFEntity_Vehicle );

							idAFEntity_Vehicle( void );

	void					Spawn( void );
	void					Use( idPlayer *player );

protected:
	idPlayer *				player;
	jointHandle_t			eyesJoint;
	jointHandle_t			steeringWheelJoint;
	float					wheelRadius;
	float					steerAngle;
	float					steerSpeed;
//	const idDeclParticle *	dustSmoke;

	float					GetSteerAngle( void );
};


/*
===============================================================================

idAFEntity_VehicleFourWheels

===============================================================================
*/

class idAFEntity_VehicleFourWheels : public idAFEntity_Vehicle {
public:
	CLASS_PROTOTYPE( idAFEntity_VehicleFourWheels );

							idAFEntity_VehicleFourWheels( void );

	void					Spawn( void );
	virtual void			Think( void );

protected:
	idAFBody *				wheels[4];
	idAFConstraint_Hinge *	steering[2];
	jointHandle_t			wheelJoints[4];
	float					wheelAngles[4];
};


/*
===============================================================================

idAFEntity_VehicleSixWheels

===============================================================================
*/

class idAFEntity_VehicleSixWheels : public idAFEntity_Vehicle {
public:
	CLASS_PROTOTYPE( idAFEntity_VehicleSixWheels );

							idAFEntity_VehicleSixWheels( void );

	void					Spawn( void );
	virtual void			Think( void );

private:
	idAFBody *				wheels[6];
	idAFConstraint_Hinge *	steering[4];
	jointHandle_t			wheelJoints[6];
	float					wheelAngles[6];
};


/*
===============================================================================

idAFEntity_SteamPipe

===============================================================================
*/

class idAFEntity_SteamPipe : public idAFEntity_Base {
public:
	CLASS_PROTOTYPE( idAFEntity_SteamPipe );

							idAFEntity_SteamPipe( void );
							~idAFEntity_SteamPipe( void );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );

private:
	int						steamBody;
	float					steamForce;
	float					steamUpForce;
	idForce_Constant		force;
	renderEntity_t			steamRenderEntity;
	qhandle_t				steamModelDefHandle;

	void					InitSteamRenderEntity( void );
};


/*
===============================================================================

idAFEntity_ClawFourFingers

===============================================================================
*/

class idAFEntity_ClawFourFingers : public idAFEntity_Base {
public:
	CLASS_PROTOTYPE( idAFEntity_ClawFourFingers );

							idAFEntity_ClawFourFingers( void );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

private:
	idAFConstraint_Hinge *	fingers[4];

	void					Event_SetFingerAngle( float angle );
	void					Event_StopFingers( void );
};

// RAVEN BEGIN
// bdube: AFAttractor

/*
===============================================================================

idAFAttractor

===============================================================================
*/

class rvAFAttractor : public idEntity {
public:
	CLASS_PROTOTYPE( rvAFAttractor );

							rvAFAttractor( void ) { }

private:
};

// RAVEN END

#endif /* !__GAME_AFENTITY_H__ */
