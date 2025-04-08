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

#ifndef __GAME_BRITTLEFRACTURE_H__
#define __GAME_BRITTLEFRACTURE_H__


/*
===============================================================================

B-rep Brittle Fracture - Static entity using the boundary representation
of the render model which can fracture.

===============================================================================
*/

extern const idEventDef EV_UpdateSoundLoss;
extern const idEventDef EV_DampenSound;

typedef struct shard_s {
	idClipModel *				clipModel;
	idFixedWinding				winding;
	idList<idFixedWinding *>	decals;
	idList<bool>				edgeHasNeighbour;
	idList<struct shard_s *>	neighbours;
	idPhysics_RigidBody			physicsObj;
	int							droppedTime;
	bool						atEdge;
	int							islandNum;
} shard_t;


class idBrittleFracture : public idEntity {

public:
	CLASS_PROTOTYPE( idBrittleFracture );

								idBrittleFracture( void );
	virtual						~idBrittleFracture( void ) override;

	void						Save( idSaveGame *savefile ) const;
	void						Restore( idRestoreGame *savefile );

	void						Spawn( void );

	virtual void				Present( void ) override;
	virtual void				Think( void ) override;
	virtual void				ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) override;
	virtual void				AddForce( idEntity *ent, int bodyId, const idVec3 &point, const idVec3 &force, const idForceApplicationId &applId ) override;
	virtual void				AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName ) override;
	virtual void				Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;
	// SteveL #4180: Let all damage types paint crack decals. Extend idEntity::Damage()
	virtual void				Damage( idEntity *inflictor, idEntity *attacker, 
										const idVec3 &dir, const char *damageDefName, 
										const float damageScale, const int location, trace_t *tr = NULL ) override;

	void						ProjectDecal( const idVec3 &point, const idVec3 &dir, const int time, const char *damageDefName );
	bool						IsBroken( void ) const;

	enum {
		EVENT_PROJECT_DECAL = idEntity::EVENT_MAXEVENTS,
		EVENT_SHATTER,
		EVENT_MAXEVENTS
	};

	virtual void				ClientPredictionThink( void ) override;
	virtual bool				ClientReceiveEvent( int event, int time, const idBitMsg &msg ) override;

	/**
	* Update soundprop to set losses in associated portal, if portal is present
	* Called on spawn and when it breaks
	**/
	void						UpdateSoundLoss( void );

	/**
	* grayman #3042 - Allow portal entities to set their sound loss as a base level for this entity's sound loss
	**/
	void						SetLossBase (float lossAI, float lossPlayer);

private:
	// setttings
	const idMaterial *			material;
	const idMaterial *			decalMaterial;
	float						decalSize;
	float						maxShardArea;
	float						maxShatterRadius;
	float						minShatterRadius;
	float						linearVelocityScale;
	float						angularVelocityScale;
	float						shardMass;
	float						density;
	float						friction;
	float						bouncyness;
	idStr						fxFracture;
	float						shardAliveTime;		// Replaced global constants with member 
	float						shardFadeStart;		// vars so mappers can tweak -- SteveL #4176

	// state
	idPhysics_StaticMulti		physicsObj;
	idList<shard_t *>			shards;
	idBounds					bounds;
	bool						disableFracture;

	/** TDM: Moss arrow dampens sound of shattering **/
	bool						m_bSoundDamped;

	// for rendering
	mutable int					lastRenderEntityUpdate;
	mutable bool				changed;

	// Keep track of when last crack decal was applied, so we don't do it twice in 1 frame -- SteveL #4180
	int							m_lastCrackFrameNum;

	/**
	* Contains the visportal handle that the breakable is touching, if portal is present
	* If no portal is present, is set to 0.
	**/
	qhandle_t					m_AreaPortal;

	/**
	* grayman #3042 - additional sound loss provided by portal entities touching the same portal
	**/
	float						m_lossBaseAI;
	float						m_lossBasePlayer;

	bool						UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) const;
	static bool					ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView );

	void						AddShard( idClipModel *clipModel, idFixedWinding &w );
	void						RemoveShard( int index );
	void						DropShard( shard_t *shard, const idVec3 &point, const idVec3 &dir, const float impulse, const int time );
	void						Shatter( const idVec3 &point, const idVec3 &impulse, const int time );
	void						DropFloatingIslands( const idVec3 &point, const idVec3 &impulse, const int time );
	void						Break( void );
	void						Fracture_r( idFixedWinding &w );
	void						CreateFractures( const idRenderModel *renderModel );
	void						FindNeighbours( void );

	void						Event_Activate( idEntity *activator );
	void						Event_Touch( idEntity *other, trace_t *trace );
	void						Event_DampenSound( bool bDampen );
};

#endif /* !__GAME_BRITTLEFRACTURE_H__ */
