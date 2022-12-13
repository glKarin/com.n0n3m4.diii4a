// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_ITEM_H__
#define __GAME_ITEM_H__

#define ITEM_SPAWNRATE_DEFAULT		20.0f //HUMANHEAD rww (id used 20.0)

/*
===============================================================================

  Items the player can pick up or use.

===============================================================================
*/

//HUMANHEAD: aob
extern const idEventDef EV_RespawnItem;
extern const idEventDef EV_RespawnFx;
//HUMANHEAD END

class idItem : public idEntity {
public:
	CLASS_PROTOTYPE( idItem );

							idItem();
	virtual					~idItem();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	const idDeclSkin		*GetNonRespawnSkin(void) const; //HUMANHEAD rww
	void					Spawn( void );
	void					GetAttributes( idDict &attributes );
	virtual bool			GiveToPlayer( idPlayer *player );
	virtual bool			Pickup( idPlayer *player );
	virtual void			Think( void );
	virtual void			Present();
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );

	enum {
		EVENT_PICKUP = idEntity::EVENT_MAXEVENTS,
		EVENT_RESPAWN,
		EVENT_RESPAWNFX,
		EVENT_MAXEVENTS
	};

	virtual void			ClientPredictionThink( void );
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	// networking
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

protected:	// HUMANHEAD
	idVec3					orgOrigin;
	bool					spin;
	bool					pulse;
	bool					canPickUp;

	// for item pulse effect
	int						itemShellHandle;
	const idMaterial *		shellMaterial;

	// used to update the item pulse effect
	mutable bool			inView;
	mutable int				inViewTime;
	mutable int				lastCycle;
	mutable int				lastRenderViewTime;

	bool					UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) const;
	static bool				ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView );

	void					Event_DropToFloor( void );
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					Event_Trigger( idEntity *activator );
	void					Event_Respawn( void );
	void					Event_RespawnFx( void );
};

// idItemPowerup (HUMANHEAD pdm: removed)

// idObjective (HUMANHEAD pdm: removed)

// idVideoCDItem (HUMANHEAD pdm: removed)

// idPDAItem (HUMANHEAD pdm: removed)

// HUMANHEAD pdm: Now inherits from our hhItem
#include "../prey/prey_items.h"
class idMoveableItem : public hhItem {
/*
class idMoveableItem : public idItem {
*/
public:
	CLASS_PROTOTYPE( idMoveableItem );

							idMoveableItem();
	virtual					~idMoveableItem();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	virtual bool			Pickup( idPlayer *player );

	//HUMANHEAD
	virtual void			SquishedByDoor(idEntity *door);
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	//HUMANHEAD END

	static void				DropItems( idAnimatedEntity *ent, const char *type, idList<idEntity *> *list );
	static idEntity	*		DropItem( const char *classname, const idVec3 &origin, const idMat3 &axis, const idVec3 &velocity, int activateDelay, int removeDelay );

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

private:
	idPhysics_RigidBody		physicsObj;
	idClipModel *			trigger;
	const idDeclParticle *	smoke;
	int						smokeTime;

	void					Gib( const idVec3 &dir, const char *damageDefName );

	void					Event_DropToFloor( void );
	void					Event_Gib( const char *damageDefName );
};

// idMoveablePDAItem (HUMANHEAD pdm: removed)

/*
===============================================================================

  Item removers.

===============================================================================
*/

class idItemRemover : public idEntity {
public:
	CLASS_PROTOTYPE( idItemRemover );

	void					Spawn();
	void					RemoveItem( idPlayer *player );

private:
	void					Event_Trigger( idEntity *activator );
};

// idObjectiveComplete (HUMANHEAD pdm: removed)

#endif /* !__GAME_ITEM_H__ */
