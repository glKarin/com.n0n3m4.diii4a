
#ifndef __GAME_BRITTLEFRACTURE_H__
#define __GAME_BRITTLEFRACTURE_H__


/*
===============================================================================

B-rep Brittle Fracture - Static entity using the boundary representation
of the render model which can fracture.

===============================================================================
*/

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
	virtual						~idBrittleFracture( void );

	void						Save( idSaveGame *savefile ) const;
	void						Restore( idRestoreGame *savefile );

	void						Spawn( void );

	virtual void				Present( void );
	virtual void				Think( void );
	virtual void				ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash = false );
	virtual void				AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
	virtual void				AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor );
	virtual void				Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	void						ProjectDecal( const idVec3 &point, const idVec3 &dir, const int time, const char *damageDefName );
	bool						IsBroken( void ) const;

	enum {
		EVENT_PROJECT_DECAL = idEntity::EVENT_MAXEVENTS,
		EVENT_SHATTER,
		EVENT_MAXEVENTS
	};

	virtual void				ClientPredictionThink( void );
	virtual bool				ClientReceiveEvent( int event, int time, const idBitMsg &msg );

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

	// state
	idPhysics_StaticMulti		physicsObj;
	idList<shard_t *>			shards;
	idBounds					bounds;
	bool						disableFracture;

	// for rendering
	mutable int					lastRenderEntityUpdate;
	mutable bool				changed;

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
};

#endif /* !__GAME_BRITTLEFRACTURE_H__ */
