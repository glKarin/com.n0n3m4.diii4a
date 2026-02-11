
#ifndef __GAME_GRAVITYZONE_H__
#define __GAME_GRAVITYZONE_H__

class hhDock;

class hhZone : public hhTrigger {
public:
	ABSTRACT_PROTOTYPE( hhZone );

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	Think( void );
	virtual void	Present( void ) { } // HUMANHEAD mdl:  Not used by zones

	// hhTrigger interface
	void			TriggerAction(idEntity *activator);

	// hhZone interface
	void			ResetZoneList();
	void			ApplyToEncroachers();
	bool			ContainsEntityOfType(const idTypeInfo &t);
	virtual void	EntityEntered(idEntity *ent) {}
	virtual void	EntityLeaving(idEntity *ent) {}
	virtual void	EntityEncroaching(idEntity *ent) {}
	virtual bool	ValidEntity(idEntity *ent);
	virtual void	Empty();

protected:
	void			Event_TurnOff();
	void			Event_Enable( void );
	void			Event_Disable( void );
	void			Event_Touch( idEntity *other, trace_t *trace );

protected:
	idList<int>		zoneList;		// List of valid entities in zone last frame
	float			slop;
};

class hhTriggerZone : public hhZone {
public:
	CLASS_PROTOTYPE( hhTriggerZone );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual bool			ValidEntity(idEntity *ent);
	virtual void			EntityEntered(idEntity *ent);
	virtual void			EntityLeaving(idEntity *ent);
	virtual void			EntityEncroaching(idEntity *ent);

	hhFuncParmAccessor		funcRefInfo;
};

class hhGravityZoneBase : public hhZone {
public:
	ABSTRACT_PROTOTYPE( hhGravityZoneBase );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual bool			ValidEntity(idEntity *ent);
	virtual void			EntityEntered(idEntity *ent);
	virtual void			EntityEncroaching(idEntity *ent);
	virtual void			EntityLeaving(idEntity *ent);
	virtual const idVec3 	GetGravityOrigin() const;
	virtual const idVec3	GetCurrentGravity(const idVec3 &location) const = 0;

	virtual bool			TouchingOtherZones(idEntity *ent, bool traceCheck, idVec3 &otherInfluence);

	//rww - network code
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );

protected:
	bool					bReorient;
	bool					bKillsMonsters;
	bool					bShowVector;
	idVec3					gravityOriginOffset; //rww - avoid dictionary lookup
};

class hhGravityZone : public hhGravityZoneBase {
public:
	CLASS_PROTOTYPE( hhGravityZone );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual void			Think( void );
	virtual const idVec3	GetDestinationGravity() const;
	virtual const idVec3	GetCurrentGravity(const idVec3 &location) const;
	virtual void			SetGravityOnZone( idVec3 &newGravity );

	//rww - network code
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );

protected:
	void					Event_SetNewGravity( idVec3 &newgrav );

protected:
	idInterpolate<idVec3>	gravityInterpolator;
	int						interpolationTime;
	bool					zeroGravOnChange;
};

class hhAIWallwalkZone : public hhGravityZone {
public:
	CLASS_PROTOTYPE( hhAIWallwalkZone );
	virtual void	EntityEncroaching(idEntity *ent);
	virtual bool	ValidEntity(idEntity *ent);
};

class hhGravityZoneInward : public hhGravityZoneBase {
public:
	CLASS_PROTOTYPE( hhGravityZoneInward );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			EntityEntered(idEntity *ent);
	virtual void			EntityLeaving(idEntity *ent);
	virtual void			EntityEncroaching(idEntity *ent);

	virtual const idVec3	GetCurrentGravity(const idVec3 &location) const;

protected:
	virtual void			Event_SetNewGravityFactor( float newFactor );

protected:
	idInterpolate<float>	factorInterpolator;
	int						interpolationTime;
	float					monsterGravityFactor;
};


#define GRAVITATIONAL_CONSTANT	1.03416206832413664e-7f

class hhGravityZoneSinkhole : public hhGravityZoneInward {
public:
	CLASS_PROTOTYPE( hhGravityZoneSinkhole );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual const idVec3	GetCurrentGravity(const idVec3 &location) const;
	const idVec3			GetCurrentGravityEntity(const idEntity *ent) const;

protected:
	void					Event_SetNewGravityFactor( float newFactor );

protected:
	float					maxMagnitude;
	float					minMagnitude;
};


class hhVelocityZone : public hhZone {
public:
	CLASS_PROTOTYPE( hhVelocityZone );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual void			Think( void );
	virtual void			EntityEncroaching(idEntity *ent);
	virtual void			EntityLeaving(idEntity *ent);

protected:
	void					Event_SetNewVelocity( idVec3 &newvel );

protected:
	idInterpolate<idVec3>	velocityInterpolator;
	bool					bKillsMonsters;
	bool					bReorient;
	bool					bShowVector;
	int						interpolationTime;
};

class hhShuttleRecharge : public hhZone {
public:
	CLASS_PROTOTYPE( hhShuttleRecharge );

	void					Spawn(void);
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual bool			ValidEntity(idEntity *ent);
	virtual void			EntityEntered(idEntity *ent);
	virtual void			EntityLeaving(idEntity *ent);
	virtual void			EntityEncroaching(idEntity *ent);

protected:
	int						amountHealth;
	int						amountPower;
};

class hhDockingZone : public hhZone {
public:
	CLASS_PROTOTYPE( hhDockingZone );

	void					Spawn(void);
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual bool			ValidEntity(idEntity *ent);
	virtual void			EntityEntered(idEntity *ent);
	virtual void			EntityEncroaching(idEntity *ent);
	virtual void			EntityLeaving(idEntity *ent);
	void					RegisterDock(hhDock *d);

	//rww - network code
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );

protected:
	idEntityPtr<hhDock>		dock;
};

class hhShuttleDisconnect : public hhZone {
public:
	CLASS_PROTOTYPE( hhShuttleDisconnect );

	void					Spawn(void);
	virtual bool			ValidEntity(idEntity *ent);
	virtual void			EntityEntered(idEntity *ent);
	virtual void			EntityEncroaching(idEntity *ent);
	virtual void			EntityLeaving(idEntity *ent);

protected:
};

class hhShuttleSlingshot : public hhZone {
public:
	CLASS_PROTOTYPE( hhShuttleSlingshot );

	void					Spawn(void);
	virtual bool			ValidEntity(idEntity *ent);
	virtual void			EntityEntered(idEntity *ent);
	virtual void			EntityEncroaching(idEntity *ent);
	virtual void			EntityLeaving(idEntity *ent);

protected:
};

class hhRemovalVolume : public hhZone {
public:
	CLASS_PROTOTYPE( hhRemovalVolume );

	void					Spawn(void);
	virtual bool			ValidEntity(idEntity *ent);
	virtual void			EntityEntered(idEntity *ent);
	virtual void			EntityEncroaching(idEntity *ent);
	virtual void			EntityLeaving(idEntity *ent);

protected:
};

#endif /* __GAME_GRAVITYZONE_H__ */
