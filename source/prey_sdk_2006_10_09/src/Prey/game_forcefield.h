#ifndef __GAME_FORCEFIELD_H__
#define __GAME_FORCEFIELD_H__

class hhForceField : public idEntity {
	CLASS_PROTOTYPE( hhForceField );

public:
	void		Spawn();
	void		ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
	void		Save( idSaveGame *savefile ) const;
	void		Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );

protected:
	void		Ticker();

	int			DetermineThinnestAxis();
	idVec3		DetermineForce();

	virtual bool IsAtRest( int id ) const;

	void		EnterDamagedState();
	void		EnterPreTurningOnState();
	void		EnterTurningOnState();
	void		EnterOnState();
	void		EnterTurningOffState();
	void		EnterOffState();

protected:
	void		Event_Activate( idEntity *activator );

protected:
	enum States {
		StatePreTurningOn = 0,
		StateTurningOn,
		StateOn,
		StateTurningOff,
		StateOff
	} fieldState;
	bool		damagedState;

	float		activationRate;
	float		deactivationRate;
	float		undamageFadeRate;

	int			applyImpulseAttempts;

	int			cachedContents;

	float		fade;

	int			nextCollideFxTime;

	hhPhysics_StaticForceField physicsObj;
};


class hhShuttleForceField : public idEntity {
	CLASS_PROTOTYPE( hhShuttleForceField );

public:
	void		Spawn();
	void		ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
	void		Save( idSaveGame *savefile ) const;
	void		Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );

	void		Ticker();

protected:
	void		Event_Activate(idEntity *activator);

	enum States {
		StatePreTurningOn = 0,
		StateTurningOn,
		StateOn,
		StateTurningOff,
		StateOff
	} fieldState;

private:
	int						nextCollideFxTime;
	idInterpolate<float>	fade;
};

#endif /* __GAME_FORCEFIELD_H__ */
