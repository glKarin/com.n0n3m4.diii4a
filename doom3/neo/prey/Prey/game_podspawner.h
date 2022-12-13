
#ifndef __GAME_PODSPAWNER_H__
#define __GAME_PODSPAWNER_H__

class hhPodSpawner : public hhMineSpawner {
public:
	CLASS_PROTOTYPE( hhPodSpawner );

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	void			SpawnMine();

protected:
	void			Event_PlayIdle();
	void			Event_DropPod();
	void			Event_DoneSpawning();

private:
	int				idleAnim;
	int				painAnim;
	int				spawnAnim;

	bool			spawning;
	hhPod			*pod;
};


#endif /* __GAME_PODSPAWNER_H__ */
