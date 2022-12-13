
#ifndef __GAME_GUN_H__
#define __GAME_GUN_H__

class hhGun : public idEntity {
public:
	CLASS_PROTOTYPE( hhGun );

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	Think( void );
	virtual void	Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	void			SetEnemy(idEntity *ent);
	void			FindEnemy();
	idMat3			GetAimAxis();
	bool			ValidEnemy();

protected:
	void			Fire(idMat3 &axis);

	void			Event_Activate(idEntity *activator);
	void			Event_SetEnemy(idEntity *newEnemy);
	void			Event_FireAt(idEntity *victim);

protected:
	idEntityPtr<idEntity>	enemy;
	int						enemyRate;
	int						nextEnemyTime;
	idVec3					targetOffset;
	float					targetRadius;

	int						fireRate;
	int						fireDeviation;
	int						nextFireTime;

	int						burstRate;
	int						burstCount;
	int						nextBurstTime;
	int						curBurst;
	bool					firing;
	float					coneAngle;
};

#endif // __GAME_GUN_H__
