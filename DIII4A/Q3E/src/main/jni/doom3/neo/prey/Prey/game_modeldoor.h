#ifndef __GAME_MODELDOOR_H__
#define __GAME_MODELDOOR_H__

extern const idEventDef EV_ModelDoorOpen;
extern const idEventDef EV_ModelDoorClose;

extern const idEventDef EV_SetBuddiesShaderParm;

class hhDoorTrigger;	// Stupid C++ forward decl for hhModelDoor

//--------------------------------
// hhModelDoor
//--------------------------------
class hhModelDoor : public hhAnimatedEntity {

public:
	CLASS_PROTOTYPE( hhModelDoor );

					hhModelDoor();
	virtual			~hhModelDoor();
	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	// Overridden methods
	virtual void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void 	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void	ClientPredictionThink( void );
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );

	void			ToggleDoorState( void );
	void			OpenDoor();
	void			CloseDoor();
	void			Lock( int f );
	bool			IsLocked() const { return locked != 0; }
	ID_INLINE bool	IsOpen() const { return ( bOpen || bTransition ); }
	ID_INLINE bool	IsClosed() const { return ( !bOpen && !bTransition ); }
	bool			CanOpen() const;
	bool			CanClose() const;
	void			ForceAirLockTeamClosed();
	void			ForceAirLockTeamOpen();
	const char*		GetAirLockTeamName() const { return airlockTeamName.c_str(); }
	hhModelDoor*	GetAirLockMaster() const { return airlockMaster; }
	idEntity *		GetActivator() const;

	idLinkList<hhModelDoor> airlockTeam;

protected:
	void			SetBlocking( bool on );
	void			ClosePortal( void );
	void			OpenPortal( void );
	void			InformDone();

	void			SetBuddiesShaderParm( int parm, float value );
	void			ToggleBuddiesShaderParm( int parm, float firstValue, float secondValue, float toggleDelay );
	void			WakeTouchingEntities();

	idEntity*		DetermineTeamMaster( const char* teamName );
	void			JoinAirLockTeam( hhModelDoor *master );
	void			VerifyAirlockTeamStatus();

	void			StartOpen();
	void			StartClosed();
	void			TryOpen( idEntity *whoTrying );
	bool			EntitiesInTrigger();

	void			Event_TeamBlocked( idEntity *blockedEntity, idEntity *blockingEntity );
	void			Event_PartBlocked( idEntity *blockingEntity );
	void			Event_SpawnNewDoorTrigger( void );
	void			Event_SetCallback( void );
	void			Event_SetBuddiesShaderParm( int parm, float value );
	void			Event_Touch( idEntity *other, trace_t* trace );
	void			Event_Activate( idEntity *activator );
	void			Event_ToggleDoorState( void );
	void			Event_OpenDoor();
	void			Event_CloseDoor();
	void			Event_STATE_ClosedBegin();
	void			Event_STATE_OpeningBegin();
	void			Event_STATE_OpenBegin();
	void			Event_STATE_ClosingBegin();

protected:
	float			damage;
	float			wait;
	float			triggersize;
	qhandle_t		areaPortal;			// 0 = no portal
	idList<idStr>	buddyNames;
	int				locked;
	int				openAnim;
	int				closeAnim;
	int				idleAnim;
	int				painAnim;
	bool			forcedOpen;			// Is the door forced open
	bool			bOpenForMonsters;	// Door opens for monsters
	bool			noTouch;			// Can you touch this door.
	bool			bOpen;				// HUMANHEAD mdl:  True if door is open
	bool			bTransition;		// HUMANHEAD mdl:  True if door is in transition between opening and closing
	bool			bShuttleDoors;
	int				threadNum;			// Thread used for sys.WaitFor() calls
	hhDoorTrigger * doorTrigger;
	idClipModel *	sndTrigger;
	float			airLockSndWait;		// Time in milliseconds between airlock sounds from spawnarg airlockwait
	float			nextAirLockSnd;		// Next time to play an airlock locked door sound
	int				nextSndTriggerTime;	// next time to play door locked sound

	idStr			airlockTeamName;
	hhModelDoor		*airlockMaster;

	idBounds		crusherBounds;

	idEntityPtr<idEntity>	activatedBy;
	
	bool			finishedSpawn;		// Used to determine if we are still spawning in or not.

};

class hhDoorTrigger : public idEntity {
	CLASS_PROTOTYPE( hhDoorTrigger );

public:
	void			Disable() { enabled = false; }
	void			Enable() { enabled = true; }
	bool			IsEnabled() { return( enabled ); }
	int				GetEntitiesWithin( idEntity **ents, int entsLength );

	hhModelDoor		*door;
	bool			enabled;

protected:
					hhDoorTrigger();

	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	void			Event_TriggerDoor( idEntity *other, trace_t *trace );
};

#endif /* __GAME_MODELDOOR_H__ */
