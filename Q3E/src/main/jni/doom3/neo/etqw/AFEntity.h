// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_AFENTITY_H__
#define __GAME_AFENTITY_H__

#include "physics/Physics_AF.h"
#include "physics/Force_Constant.h"
#include "AF.h"
#include "AnimatedEntity.h"

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

	virtual void			Think( void );
	virtual void			GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info );
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse );
	virtual void			AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity, int bodyId );
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	virtual bool			UpdateAnimationControllers( void );

	virtual bool			LoadAF( void );
	bool					IsActiveAF( void ) const { return af.IsActive(); }
	const char *			GetAFName( void ) const { return af.GetName(); }
	idPhysics_AF *			GetAFPhysics( void ) { return af.GetPhysics(); }
	const idAF&				GetAF( void ) { return af; }

	void					SetCombatModel( void );
							// contents of combatModel can be set to 0 or re-enabled (mp)
	void					SetCombatContents( bool enable );
	virtual void			LinkCombat( void );
	virtual void			UnLinkCombat( void );

	int						BodyForClipModelId( int id ) const;

	void					SaveState( idDict &args ) const;
	void					LoadState( const idDict &args );

	virtual void			Unbind( void );
	virtual bool			InitBind( idEntity *master );
	void					AddBindConstraints( void );
	void					RemoveBindConstraints( void );

	virtual void			ShowEditingDialog( void );

	static void				DropAFs( idEntity *ent, const char *type, idList<idEntity *> *list );

protected:
	idAF					af;				// articulated figure
	int						combatModelContents;
	idVec3					spawnOrigin;	// spawn origin
	idMat3					spawnAxis;		// rotation axis used when spawned
	int						nextSoundTime;	// next time this can make a sound
};

/*
===============================================================================

idAFEntity_Generic

===============================================================================
*/

class idAFEntity_Generic : public idAFEntity_Base {
public:
	CLASS_PROTOTYPE( idAFEntity_Generic );

	idAFEntity_Generic( void );
	~idAFEntity_Generic( void );

	void					Spawn( void );

	virtual void			Think( void );
	void					KeepRunningPhysics( void ) { keepRunningPhysics = true; }

private:
	void					Event_Activate( idEntity *activator );

	bool					keepRunningPhysics;
};

#endif /* !__GAME_AFENTITY_H__ */
