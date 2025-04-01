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
	virtual					~idItem() override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	void					GetAttributes( idDict &attributes );
	virtual bool			GiveToPlayer( idPlayer *player );
	virtual bool			Pickup( idPlayer *player );
	virtual void			Think( void ) override;
	virtual void			Present() override;


	enum {
		EVENT_PICKUP = idEntity::EVENT_MAXEVENTS,
		EVENT_RESPAWN,
		EVENT_RESPAWNFX,
		EVENT_MAXEVENTS
	};

	virtual void			ClientPredictionThink( void ) override;
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg ) override;

	// networking
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;

	// TDM: SZ: UpdateVisuals is overridden to check if the object moved in case
	// we have to spawn an absence entity
	virtual void			UpdateVisuals( void ) override;

	// This indicates which team owns the item (or thinks it does :P )
	int ownerTeam;

private:
	idVec3					orgOrigin;
	bool					spin;
	bool					pulse;
	bool					canPickUp;

	// This should scale from 0.0 (none) to 1.0 (hard to miss)
	float					noticeabilityIfAbsent;


	// Has the original origin been set?
	bool					b_orgOriginSet;
	idEntityPtr<idEntity>	absenceEntityPtr;

	// for item pulse effect
	int						itemShellHandle;
	const idMaterial *		shellMaterial;

	// used to update the item pulse effect
	mutable bool			inView;
	mutable int				inViewTime;
	mutable int				lastCycle;
	mutable int				lastRenderViewTime;

	bool					UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView );
	static bool				ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView );

	void					Event_DropToFloor( void );
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					Event_Trigger( idEntity *activator );
	void					Event_Respawn( void );
	void					Event_RespawnFx( void );
};


class idMoveableItem : public idItem {
public:
	CLASS_PROTOTYPE( idMoveableItem );

							idMoveableItem();
	virtual					~idMoveableItem() override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void ) override;
	virtual bool			Pickup( idPlayer *player ) override;

	static void				DropItems( idAnimatedEntity *ent, const char *type, idList<idEntity *> *list );
	static idEntity	*		DropItem( const char *classname, const idVec3 &origin, const idMat3 &axis, const idVec3 &velocity, int activateDelay, int removeDelay );

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;

protected:
	virtual void			Hide() override;
	virtual void			Show() override;

private:
	idPhysics_RigidBody		physicsObj;
	idClipModel *			trigger;
	const idDeclParticle *	smoke;
	int						smokeTime;

	void					Gib( const idVec3 &dir, const char *damageDefName );

	void					Event_DropToFloor( void );
	void					Event_Gib( const char *damageDefName );
};

#endif /* !__GAME_ITEM_H__ */
