
#ifndef __GAME_VOMITER_H__
#define __GAME_VOMITER_H__

class hhVomiter : public hhAnimatedEntity {
public:
	CLASS_PROTOTYPE( hhVomiter );

					hhVomiter();
	virtual			~hhVomiter();

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	void			Erupt(int repeat);
	void			UnErupt(void);
	virtual bool	Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual	void	DormantBegin( void );
	virtual	void	DormantEnd( void );

protected:
	void			StartIdle();
	void			StopIdle();
	void			SpawnTrigger();
	void			WakeUp();
	void			GoToSleep();

protected:
	void			Event_DamagePulse(void);
	void			Event_Trigger( idEntity *activator );
	void			Event_Erupt(int repeat);
	void			Event_UnErupt(void);
	void			Event_PlayIdle( void );
	void			Event_MovablePulse();
	void			Event_AssignFxErupt( hhEntityFx* fx );

private:
	bool			bErupting;
	bool			bIdling;
	bool			bAwake;
	float			mindelay;				// Delays until next vomit
	float			maxdelay;
	float			secBetweenDamage;
	float			secBetweenMovables;
	int				idleAnim;
	int				painAnim;
	int				spewAnim;
	int				deathAnim;
	float			minMovableVel;
	float			maxMovableVel;
	float			removeTime;
	int				numMovables;
	float			goAwayChance;
	idEntityFx		*fxErupt;

	idEntityPtr<hhTrigger>	detectionTrigger;
};

#endif /* __GAME_VOMITER_H__ */
