#ifndef __GAME_GRAVITYSWITCH_H__
#define __GAME_GRAVITYSWITCH_H__

class hhGravitySwitch : public hhDamageTrigger {
public:
	CLASS_PROTOTYPE( hhGravitySwitch );

	virtual			~hhGravitySwitch();
	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	// Overridden methods
	virtual void	Ticker();
	virtual void	Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location);
	virtual void	TriggerAction(idEntity *activator);
	virtual void	Event_Enable();
	virtual void	Event_Disable();
	virtual void	Event_PostSpawn();
	void			Event_CheckAgain();

	//rww - network code
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );

protected:
	void			SetGravityVector(idEntity *activator);
	idVec3			GetGravityVector();

private:
	idEntityPtr<hhFuncEmitter>		effect;

};

#endif	// __GAME_GRAVITYSWITCH_H__
