#ifndef __GAME_ENERGYNODE_H__
#define __GAME_ENERGYNODE_H__

class hhEnergyNode : public idStaticEntity {
public:
	CLASS_PROTOTYPE( hhEnergyNode );

	hhEnergyNode(void);
	~hhEnergyNode();

	void			Spawn( void );

	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	inline bool		CanLeech() { return !disabled; }
	void			LeechTrigger(idEntity *activator, const char* type);
	void			Finish();

	virtual void	Event_Enable();
	virtual void	Event_Disable();

	//rww - network code
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );
	
	idVec3			leechPoint;

protected:
	bool disabled;

	idEntityPtr<hhEntityFx>		energyFx;
};

#endif	// __GAME_ENERGYNODE_H__
