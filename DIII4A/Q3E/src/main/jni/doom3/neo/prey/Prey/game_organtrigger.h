#ifndef __GAME_ORGANTRIGGER_H__
#define __GAME_ORGANTRIGGER_H__

extern const idEventDef EV_ModelDoorOpen;
extern const idEventDef EV_ModelDoorClose;

class hhOrganTrigger : public hhAnimatedEntity {
public:
	CLASS_PROTOTYPE( hhOrganTrigger );

	virtual			~hhOrganTrigger();

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location );

protected:
	void			SpawnTrigger();

	virtual void	Enable();
	virtual void	Disable();

	void			Event_Enable();
	void			Event_Disable();
	void			Event_PlayIdle( void );
	void			Event_PlayPainIdle( void );
	void			Event_ResetOrgan( void );
	virtual void	Event_PostSpawn();

private:
	int				idleAnim;
	int				painAnim;
	int				resetAnim;
	int				painIdleAnim;

	idEntityPtr<hhTrigger> trigger;
};

#endif /* __GAME_ORGANTRIGGER_H__ */
