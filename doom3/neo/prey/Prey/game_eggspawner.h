
#ifndef __GAME_EGGSPAWNER_H__
#define __GAME_EGGSPAWNER_H__

class hhEggSpawner : public hhAnimatedEntity {
	CLASS_PROTOTYPE( hhEggSpawner );
public:

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	void			SpawnEgg(idEntity *activator);
	virtual void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

protected:
	void			Event_Activate(idEntity *activator);
	void			Event_PlayIdle();

protected:
	int				idleAnim;
	int				hatchAnim;
	int				painAnim;
};


class hhEgg : public hhMoveable {
	CLASS_PROTOTYPE( hhEgg );
public:
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	virtual void			Ticker();
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	void					Hatch();
	void					SetActivator(idEntity *activator);

protected:
	bool					SpawnHatchling(const char *monsterName, const idVec3 &spawnLocation);
	void					Event_Activate(idEntity *activator);
	void					Event_Hatch();

protected:
	bool					bHatching;
	bool					bHatched;
	idInterpolate<float>	deformAlpha;
	const idDeclTable *		table;
	idEntityPtr<idActor>	enemy;
	float					hatchTime;
};

#endif
