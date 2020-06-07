// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_ITEM_H__
#define __GAME_ITEM_H__


/*
===============================================================================

  Items the player can pick up or use.

===============================================================================
*/

class idItem : public idEntity {
public:
	CLASS_PROTOTYPE( idItem );

							idItem();
	virtual					~idItem();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	void					GetAttributes( idDict &attributes );
	virtual bool			GiveToPlayer( idPlayer *player );
	virtual bool			Pickup( idPlayer *player );
	virtual void			Think( void );
	virtual void			Present();

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

private:
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

class idItemPowerup : public idItem {
public:
	CLASS_PROTOTYPE( idItemPowerup );

							idItemPowerup();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();
	virtual bool			GiveToPlayer( idPlayer *player );

private:
	int						time;
	int						type;
};

class idObjective : public idItem {
public:
	CLASS_PROTOTYPE( idObjective );

							idObjective();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();

private:
	idVec3					playerPos;

	void					Event_Trigger( idEntity *activator );
	void					Event_HideObjective( idEntity *e );
	void					Event_GetPlayerPos();
	void					Event_CamShot();
};

class idVideoCDItem : public idItem {
public:
	CLASS_PROTOTYPE( idVideoCDItem );

	void					Spawn();
	virtual bool			GiveToPlayer( idPlayer *player );
};

class idPDAItem : public idItem {
public:
	CLASS_PROTOTYPE( idPDAItem );

	virtual bool			GiveToPlayer( idPlayer *player );
};

class idMoveableItem : public idItem {
public:
	CLASS_PROTOTYPE( idMoveableItem );

							idMoveableItem();
	virtual					~idMoveableItem();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	virtual bool			Pickup( idPlayer *player );

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

class idMoveablePDAItem : public idMoveableItem {
public:
	CLASS_PROTOTYPE( idMoveablePDAItem );

	virtual bool			GiveToPlayer( idPlayer *player );
};

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

class idObjectiveComplete : public idItemRemover {
public:
	CLASS_PROTOTYPE( idObjectiveComplete );

							idObjectiveComplete();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();

private:
	idVec3					playerPos;

	void					Event_Trigger( idEntity *activator );
	void					Event_HideObjective( idEntity *e );
	void					Event_GetPlayerPos();
};

#endif /* !__GAME_ITEM_H__ */
