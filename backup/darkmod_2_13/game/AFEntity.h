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
	virtual					~idMultiModelAF( void ) override;

	virtual void			Think( void ) override;
	virtual void			Present( void ) override;

protected:
	/**
	* Parsing attachments happens at a different time in the spawn routine for
	* idAFEntities.  To accomplish this, the function is overloaded to do
	* nothing and a new function is called at the proper time.
	**/
	virtual void			ParseAttachments( void ) override;

	/**
	* Same as idEntity::ParseAttachments, but called at a different point in spawn routine
	**/
	virtual void			ParseAttachmentsAF( void );

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

class idAFAttachment : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( idAFAttachment );

							idAFAttachment( void );
	virtual					~idAFAttachment( void ) override;

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					SetBody( idEntity *bodyEnt, const char *headModel, jointHandle_t attachJoint );
	void					ClearBody( void );
	idEntity *				GetBody( void ) const;
	// ishtvan: Added this
	jointHandle_t			GetAttachJoint( void ) const;

	bool					IsMantleable( void ) const override;

	virtual void			Think( void ) override;

	virtual void			Hide( void ) override;
	virtual void			Show( void ) override;

	void					PlayIdleAnim( int blendTime );

	virtual void			GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info ) override;
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) override;
	virtual void			AddForce( idEntity *ent, int bodyId, const idVec3 &point, const idVec3 &force, const idForceApplicationId &applId ) override;

	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
									const char *damageDefName, const float damageScale,
									const int location, trace_t *tr = NULL ) override;
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName ) override;

	void					SetCombatModel( void );
	idClipModel *			GetCombatModel( void ) const;
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

	/**
	 * greebo: Virtual override of idEntity::GetResponseEntity(). This is used
	 * to relay stims to the "body" entity.
	 */
	virtual idEntity* GetResponseEntity() override;

	/**
	* Overload bind notify so that when another idAFAttachment is
	* bound to us, we copy over our data on the actor we're bound to
	**/
	virtual void BindNotify( idEntity *ent , const char *jointName) override; // grayman #3074

	/**
	* Also overload UnbindNotify to update the clipmodel physics on the AF we're attacehd to
	**/
	virtual void UnbindNotify( idEntity *ent ) override;

	/** Also overload PostUnBind to clear the body information **/
	virtual void PostUnbind( void ) override;

	/** Use this to set up stuff attached to AI's heads when they go ragdoll **/
	virtual void DropOnRagdoll( void );

protected:
	idEntity *				body;
	idClipModel *			combatModel;	// render model for hit detection of head
	int						idleAnim;
	jointHandle_t			attachJoint;

protected:
	/**
	* Copy idActor bindmaster information to another idAFAttachment
	**/
	void	CopyBodyTo( idAFAttachment *other );
};


/*
===============================================================================

idAFEntity_Base

===============================================================================
*/

#define NO_PROP_SOUND -1 // grayman #4609

/**
* TDM: Used for dynamically adding ents with clipmodels to the AF
**/
typedef struct SAddedEnt_s
{
	idEntityPtr<idEntity> ent; // associated entity

	// Must store string name because body ID's get reassigned when any are deleted
	idStr bodyName;
	// don't need to store constraint since they are deleted along with body

	idStr AddedToBody; // original body we added on to

	int entContents; // original entity clipmodel contents
	int entClipMask; // original entity clipmask
	int bodyContents; // AF body contents (for saving/restoring)
	int bodyClipMask; // AF body clipmask (for saving/restoring)
} SAddedEnt;


class idAFEntity_Base : public idAnimatedEntity 
{
public:
	CLASS_PROTOTYPE( idAFEntity_Base );

							idAFEntity_Base( void );
	virtual					~idAFEntity_Base( void ) override;

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void ) override;
	virtual void			GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info ) override;
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) override;
	virtual void			AddForce( idEntity *ent, int bodyId, const idVec3 &point, const idVec3 &force, const idForceApplicationId &applId ) override;
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity ) override;
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) override;
	virtual bool			UpdateAnimationControllers( void ) override;
	virtual void			FreeModelDef( void ) override;
	virtual void			SetModel( const char *modelname ) override;

	virtual bool			LoadAF( void );
	bool					IsActiveAF( void ) const { return af.IsActive(); }
	const char *			GetAFName( void ) const { return af.GetName(); }
	idPhysics_AF *			GetAFPhysics( void ) { return af.GetPhysics(); }

	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
									const char *damageDefName, const float damageScale,
									const int location, trace_t *tr = NULL ) override;
	void					SetCombatModel( void );
	idClipModel *			GetCombatModel( void ) const;

	// set contents of combatModel to 0 (if false) or restore proper contents (if true)
	// returns the contents state before call (false = were disabled, true = were enabled)
	bool					SetCombatContents( bool enable );
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

	int						BodyForClipModelId( int id ) const;

	void					SaveState( idDict &args ) const;
	void					LoadState( const idDict &args );

	void					AddBindConstraints( void );
	void					RemoveBindConstraints( void );

	/**
	* Called when the given ent is about to be unbound/detached
	* Updates m_AddedEnts if this ent's clipmodel was added to the AF
	**/
	virtual void			UnbindNotify( idEntity *ent ) override;

	/**
	* Overloaded idAnimatedEntity::ReAttach methods to take into account AF body clipmask and contents
	**/
	virtual void			ReAttachToCoordsOfJoint( const char *AttName, idStr jointName, idVec3 offset, idAngles angles ) override;
	virtual void			ReAttachToPos( const char *AttName, const char *PosName  ) override;

	/**
	* TDM: Adds the clipmodel of the given entity to the AF structure
	* Called during the binding process
	* AddEntByBody is called by BindToBody, AddEntByJoint called by BindToJoint
	* AddEntByBody takes an option joint argument to control the positin of the body with a joint
	* If this is not specified, joint information is copied from the AF body we bind to
	**/
	void					AddEntByBody( idEntity *ent, int bodyID, jointHandle_t joint = INVALID_JOINT );
	void					AddEntByJoint( idEntity *ent, jointHandle_t joint );

	/**
	* TDM: Remove a dynamically added ent from the AF of this AFEntity
	* Called by UnBindNotify
	**/
	void					RemoveAddedEnt( idEntity *ent );

	/**
	* TDM: Get an AF body ID for a given joint, and vice versa
	**/
	jointHandle_t			JointForBody( int body );
	int						BodyForJoint( jointHandle_t joint );

	/**
	* TDM: Get AF body for the given added entity
	* returns NULL if added ent could not be found
	**/
	idAFBody *				AFBodyForEnt( idEntity *ent );

	virtual void			ShowEditingDialog( void ) override;

	static void				DropAFs( idEntity *ent, const char *type, idList<idEntity *> *list );

	/**
	* Return whether this entity should collide with its team when bound to a team
	**/
	bool					CollidesWithTeam( void );

public:
	/**
	* This AF should not be able to be picked up off the ground completely when dragged
	**/
	bool					m_bGroundWhenDragged;

	/**
	* List of integer Id's of "critical" bodies to check for touching the ground
	* when seeing if the AF is being lifted off the ground
	**/
	idList<int>				m_GroundBodyList;

	/**
	* Number of "critical" bodies that must be kept on the ground at all times
	* If the number is below this, dragging the AF up will not be allowed
	**/
	int						m_GroundBodyMinNum;

	/**
	* If set to true, this will use the "af" damping when being grabbed instead of the default damping
	**/
	bool					m_bDragAFDamping;

	/**
	* Next time this can make a sound
	**/
	int						nextSoundTime;	// grayman #4609 - made public

protected:
	idAF					af;				// articulated figure
	idClipModel *			combatModel;	// render model for hit detection
	int						combatModelContents;
	idVec3					spawnOrigin;	// spawn origin
	idMat3					spawnAxis;		// rotation axis used when spawned
	//int					nextSoundTime;	// next time this can make a sound // grayman #4609 - make public

	/**
	* List of ents that have been dynamically added to the AF via binding
	**/
	idList<SAddedEnt>		m_AddedEnts;

	/**
	* Set to true if this entity should collide with team members when bound to them
	**/
	bool					m_bCollideWithTeam;

	/**
	* Set to true if this animated AF should activate the AF body collision models
	*	and move them around to collide with the world when animating.
	* NOTE: This MUST be set on AI in order for animation-based tactile alert
	*		to work properly.
	**/
	bool					m_bAFPushMoveables;

protected:
	/**
	* Set up grounding vars, which apply when the AF might not be able to be 
	* lifted completely off the ground by the player
	**/
	void					SetUpGroundingVars( void );

	void					Event_SetConstraintPosition( const char *name, const idVec3 &pos );

	/**
	* GetNumBodies returns the number of bodies in the AF.
	* If the AF physics pointer is NULL, it returns 0.
	**/
	void					Event_GetNumBodies( void );

	/**
	* Parsing attachments happens at a different time in the spawn routine for
	* idAFEntities.  To accomplish this, the function is overloaded to do
	* nothing and a new function is called at the proper time.
	**/
	virtual void			ParseAttachments( void ) override;

	/**
	* Same as idEntity::ParseAttachments, but called at a different point in spawn routine
	**/
	virtual void			ParseAttachmentsAF( void );

	/**
	* Restore attached entities that have been added to the AF after a save
	**/
	virtual void			RestoreAddedEnts( void );

	/**
	* Updates added ent constraints when going ragdoll, to resolve issue of ents bound to joints that can animate and move without the AF body moving
	**/
	virtual void			UpdateAddedEntConstraints( void );

	/**
	* Set the linear and angular velocities of a particular body given by ID argument
	* If the ID is invalid, no velocity is set.
	**/
	void					Event_SetLinearVelocityB( idVec3 &NewVelocity, int id );
	void					Event_SetAngularVelocityB( idVec3 &NewVelocity, int id );

	/**
	* Get the linear and angular velocities of a particular body given by int ID.  
	* If the body ID is invalid, returns (0,0,0)
	**/
	void					Event_GetLinearVelocityB( int id );
	void					Event_GetAngularVelocityB( int id );

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
	virtual					~idAFEntity_Gibbable( void ) override;

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual void			Present( void ) override;
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName,
									const float damageScale, const int location, trace_t *tr = NULL ) override;
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
	virtual					~idAFEntity_Generic( void ) override;

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void ) override;
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
	virtual					~idAFEntity_WithAttachedHead();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					SetupHead( void );

	virtual void			Think( void ) override;

	virtual void			Hide( void ) override;
	virtual void			Show( void ) override;
	virtual void			ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material, bool save = true ) override;

	virtual void			LinkCombat( void ) override;
	virtual void			UnlinkCombat( void ) override;

protected:
	virtual void			Gib( const idVec3 &dir, const char *damageDefName ) override;

private:
	idEntityPtr<idAFAttachment>	head;

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
	const idDeclParticle *	dustSmoke;

	float					GetSteerAngle( void );
};


/*
===============================================================================

idAFEntity_VehicleSimple

===============================================================================
*/

class idAFEntity_VehicleSimple : public idAFEntity_Vehicle {
public:
	CLASS_PROTOTYPE( idAFEntity_VehicleSimple );

							idAFEntity_VehicleSimple( void );
	virtual ~idAFEntity_VehicleSimple( void ) override;

	void					Spawn( void );
	virtual void			Think( void ) override;

protected:
	idClipModel *			wheelModel;
	idAFConstraint_Suspension *	suspension[4];
	jointHandle_t			wheelJoints[4];
	float					wheelAngles[4];
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
	virtual void			Think( void ) override;

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
	virtual void			Think( void ) override;

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
	virtual ~idAFEntity_SteamPipe( void ) override;

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void ) override;

private:
	int						steamBody;
	float					steamForce;
	float					steamUpForce;
	idForce_Constant		force;
	renderEntity_t			steamRenderEntity;
	qhandle_t				steamModelDefHandle;

	void					InitSteamRenderEntity( void );
};


#endif /* !__GAME_AFENTITY_H__ */
