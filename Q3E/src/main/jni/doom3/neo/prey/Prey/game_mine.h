
#ifndef __GAME_MINE_H__
#define __GAME_MINE_H__

extern const idEventDef EV_ExplodeDamage;

class hhMineSpawner;

class hhMine : public hhMoveable {
public:
	CLASS_PROTOTYPE( hhMine );

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Launch(idVec3 &velocity, idVec3 &avelocity);
	virtual bool		AllowCollision( const trace_t &collision );
	virtual void		ApplyImpulse(idEntity * ent, int id, const idVec3 &point, const idVec3 &impulse);
	virtual bool		Collide( const trace_t &collision, const idVec3 &velocity );
	virtual void		Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void				Explode( idEntity *attacker );
	void				SetSpawner(hhMineSpawner *s) { spawner = s; }
	bool				WasSpawnedBy(idEntity *theSpawner);
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void		Think();

protected:
	void				SpawnTrigger();
	void				Event_ExplodeDamage( idEntity *attacker );
	void				Event_Trigger( idEntity *activator );
	virtual void		Event_Remove();
	void				Event_ExplodedBy( idEntity *activator);
	void				Event_MineHover();

protected:
	hhMineSpawner *			spawner;			// Entity that spawned this object
	bool					bDetonateOnCollision;
	bool					bDamageOnCollision;
	idInterpolate<float>	fadeAlpha;
	bool					bScaleIn;
	bool					bExploded;
};


extern const idEventDef EV_SpawnMine;

class hhMineSpawner : public hhAnimatedEntity {
public:
	CLASS_PROTOTYPE( hhMineSpawner );

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				MineRemoved(hhMine *mine);
	void				DormantBegin();
	void				DormantEnd();

protected:
	virtual void		SpawnMine();
	void				CheckPopulation();

	void				Event_Activate(idEntity *activator);
	void				Event_SpawnMine();
	virtual void		Event_Remove();

protected:
	int					population;
	int					targetPopulation;
	idVec3				mineVelocity;
	idVec3				mineAVelocity;
	bool				bRandomDirection;
	bool				bRandomRotation;
	float				speed;					// Precomputed speed based on mineVelocity
	float				aspeed;					// Precomputed speed based on mineAVelocity
	int					spawnDelay;
	bool				active;
};


#endif
