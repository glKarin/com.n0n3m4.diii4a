//----------------------------------------------------------------
// ClientAFEntity.h
//
// Copyright 2002-2006 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_CLIENT_AFENTITY_H__
#define __GAME_CLIENT_AFENTITY_H__

/*
===============================================================================

rvClientAFEntity - a regular idAFEntity_Base spawned client-side

===============================================================================
*/

class rvClientAFEntity : public rvAnimatedClientEntity {
public:
	CLASS_PROTOTYPE( rvClientAFEntity );

							rvClientAFEntity( void );
	virtual					~rvClientAFEntity( void );

	void					Spawn( void );

	virtual void			Think( void );
	virtual void			GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info );
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash = false );
	virtual void			AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
	virtual bool			CanPlayImpactEffect ( idEntity* attacker, idEntity* target );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual bool			UpdateAnimationControllers( void );
	virtual void			FreeEntityDef( void );

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

	bool					GetNoPlayerImpactFX( void );

protected:
	idAF					af;				// articulated figure
	idClipModel *			combatModel;	// render model for hit detection
	int						combatModelContents;
	idVec3					spawnOrigin;	// spawn origin
	idMat3					spawnAxis;		// rotation axis used when spawned
	int						nextSoundTime;	// next time this can make a sound
};


/*
===============================================================================

rvClientAFAttachment - a regular idAFAttachment spawned client-side - links to an 
						idAnimatedEntity rather than rvClientAnimatedEntity

===============================================================================
*/
class rvClientAFAttachment : public rvClientAFEntity {
public:
	CLASS_PROTOTYPE( rvClientAFAttachment );

							rvClientAFAttachment( void );
	virtual					~rvClientAFAttachment( void );

	void					Spawn( void );

	void					SetBody			( idAnimatedEntity* body, const char *headModel, jointHandle_t damageJoint );
	void					SetDamageJoint	( jointHandle_t damageJoint );
	void					ClearBody		( void );
	idEntity *				GetBody			( void ) const;

	virtual void			Think						( void );

	virtual void			Hide						( void );
	virtual void			Show						( void );

	virtual bool			UpdateAnimationControllers	( void );

	void					PlayIdleAnim( int channel, int blendTime );

	virtual void			GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info );
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash = false );
	virtual void			AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );

	virtual bool			CanPlayImpactEffect ( idEntity* attacker, idEntity* target );
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor );

	void					SetCombatModel( void );
	idClipModel *			GetCombatModel( void ) const;
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

	// Lipsync
	void					InitCopyJoints			( void );

	void					CopyJointsFromBody		( void );

protected:
	idEntityPtr<idAnimatedEntity>	body;
	idClipModel *					combatModel;	// render model for hit detection of head
	int								idleAnim;
	jointHandle_t					damageJoint;

	idList<copyJoints_t>			copyJoints;		// copied from the body animation to the head model
};

#endif
