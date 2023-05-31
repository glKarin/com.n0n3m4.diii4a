// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#ifndef __GAME_ITEM_H__
#define __GAME_ITEM_H__

extern const int ARENA_POWERUP_MASK;
extern const idEventDef EV_ResetFlag;
extern const idEventDef EV_RespawnItem;
extern const idEventDef EV_SetGravity;

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
	virtual	void			InstanceJoin( void );
	virtual void			InstanceLeave( void );
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );

// RAVEN BEGIN
// mekberg: added
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
// RAVEN END
	
	enum {
		EVENT_PICKUP = idEntity::EVENT_MAXEVENTS,
		EVENT_RESPAWNFX,
		EVENT_MAXEVENTS
	};

	virtual void			ClientPredictionThink( void );
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	// networking
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual void			Hide( void );
	virtual void			Show( void );

	virtual bool			ClientStale( void );
	virtual void			ClientUnstale( void );

	int						IsVisible() { return srvReady; }

	rvClientEntityPtr<rvClientEffect>	effectIdle;
	bool					simpleItem;
	bool					pickedUp;
	const idDeclSkin*		pickupSkin;
	void					Event_DropToFloor	( void );
	
#ifdef _QUAKE4
// jmarshall
	int						GetModelIndex() const
	{
		return modelindex;
	}
// jmarshall end
#endif

protected:

	void					UpdateTrigger( void );
	void					SendPickupMsg( int clientNum );

	idClipModel *			trigger;
	bool					spin;
	// only a small subset of idItem need their physics object synced
	bool					syncPhysics;

	bool					pulse;
	bool					canPickUp;
	const idDeclSkin*		skin;

private:
	idVec3					orgOrigin;

	rvPhysics_Particle		physicsObj;

	// for item pulse effect
	int						itemShellHandle;
	const idMaterial *		shellMaterial;

	// used to update the item pulse effect
	mutable bool			inView;
	mutable int				inViewTime;
	mutable int				lastCycle;
	mutable int				lastRenderViewTime;

	// synced through snapshots to indicate show/hide or pickupSkin state
	// -1 on a client means undef, 0 not ready, 1 ready
public: // FIXME: Temp hack while Eric gets back to me about why GameState.cpp is trying to access this directly
	int						srvReady;
private: // FIXME: Temp hack while Eric gets back to me about why GameState.cpp is trying to access this directly
	int						clReady;

	int						itemPVSArea;

	bool					UpdateRenderEntity	( renderEntity_s *renderEntity, const renderView_t *renderView ) const;
	static bool				ModelCallback		( renderEntity_s *renderEntity, const renderView_t *renderView );


	void					Event_Touch			( idEntity *other, trace_t *trace );
	void					Event_Trigger		( idEntity *activator );
	void					Event_Respawn		( void );
	void					Event_RespawnFx		( void );
	void					Event_Pickup		( int clientNum );
  
// RAVEN BEGIN
// abahr
	void					Event_SetGravity();
// RAVEN END

#ifdef _QUAKE4
// jmarshall
	int						modelindex;
// jmarshall end
#endif
};

/*
===============================================================================

  idItemPowerup

===============================================================================
*/

class idItemPowerup : public idItem {
public:
	CLASS_PROTOTYPE( idItemPowerup );

							idItemPowerup();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();
	virtual bool			GiveToPlayer( idPlayer *player );
	virtual void			Think( void );
	virtual bool			Pickup( idPlayer *player );

protected:

	int						time;
	int						type;
	int						droppedTime;
	int						team;
	bool					unique;
};

/*
===============================================================================

  riDeadZonePowerup

===============================================================================
*/

class riDeadZonePowerup : public idItemPowerup {
public:
	CLASS_PROTOTYPE( riDeadZonePowerup );

							riDeadZonePowerup();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual bool			Pickup( idPlayer *player );

	void					Spawn();
	void					Show();

	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );

	int						powerup;

protected:
	void					ResetSpawn( int powerup );

private:
	void					Event_ResetSpawn( void );
};

/*
===============================================================================

  rvItemCTFFlag

===============================================================================
*/

class rvItemCTFFlag : public idItem {
public:
	CLASS_PROTOTYPE( rvItemCTFFlag );
	
							rvItemCTFFlag();
							
	void					Spawn();
	virtual bool			GiveToPlayer ( idPlayer* player );
	virtual bool			Pickup( idPlayer *player );
	
	static void				ResetFlag( int type );
	virtual void			Think( void );

	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );

private:
	int						team;
	int						powerup;
	bool					dropped;
	
	void					Event_ResetFlag( void );
	void					Event_LinkTrigger( void );
};

/*
===============================================================================

  idObjective

===============================================================================
*/

class idObjective : public idItem {
public:
	CLASS_PROTOTYPE( idObjective );

							idObjective();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();

private:
	idVec3					playerPos;

// RAVEN BEGIN
// mekberg: store triggered time for timed removal.
	int						triggerTime;
// RAVEN END

	void					Event_Trigger( idEntity *activator );
	void					Event_HideObjective( idEntity *e );
	void					Event_GetPlayerPos();
 	void					Event_CamShot();
};

/*
===============================================================================

  idMoveableItem

===============================================================================
*/

class idMoveableItem : public idItem {
public:
	CLASS_PROTOTYPE( idMoveableItem );

							idMoveableItem();
	virtual					~idMoveableItem();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );

 	static void				DropItems( idAnimatedEntity *ent, const char *type, idList<idEntity *> *list );
	static idEntity*		DropItem( const char *classname, const idVec3 &origin, const idMat3 &axis, const idVec3 &velocity, int activateDelay, int removeDelay );

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

private:
	idPhysics_RigidBody		physicsObj;

 	void					Gib( const idVec3 &dir, const char *damageDefName );
 	void					Event_Gib( const char *damageDefName );
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

/*
===============================================================================

  idObjectiveComplete

===============================================================================
*/

class idObjectiveComplete : public idItemRemover {
public:
	CLASS_PROTOTYPE( idObjectiveComplete );

							idObjectiveComplete();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();

private:
	idVec3					playerPos;

// RAVEN BEGIN
// mekberg: store triggered time for timed removal.
	int						triggerTime;
// RAVEN END

	void					Event_Trigger( idEntity *activator );
	void					Event_HideObjective( idEntity *e );
	void					Event_GetPlayerPos();
};

/*
===============================================================================

  rvObjectiveFailed

===============================================================================
*/

class rvObjectiveFailed : public idItemRemover {
public:
	CLASS_PROTOTYPE( rvObjectiveFailed );

							rvObjectiveFailed ();
private:

	void					Event_Trigger( idEntity *activator );
};

#endif /* !__GAME_ITEM_H__ */


// RAVEN END
