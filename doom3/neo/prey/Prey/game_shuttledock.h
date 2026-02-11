
#ifndef __GAME_SHUTTLEDOCK_H__
#define __GAME_SHUTTLEDOCK_H__

class hhShuttleDock : public hhDock {
public:
	CLASS_PROTOTYPE( hhShuttleDock );

						hhShuttleDock();
	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void		ClientPredictionThink( void );

	virtual void		Think();
	virtual bool		ValidEntity(idEntity *ent);
	virtual void		EntityEntered(idEntity *ent);
	virtual void		EntityEncroaching(idEntity *ent);
	virtual void		EntityLeaving(idEntity *ent);
	virtual void		ShuttleExit(hhShuttle *shuttle);
	virtual bool		IsLocked()		{	return bLocked;			}
	virtual bool		CanExitLocked()	{	return bCanExitLocked;	}
	virtual bool		AllowsBoost()	{	return !bLocked;		}
	virtual bool		AllowsFiring()	{	return false;			}
	virtual bool		AllowsExit();
	virtual bool		IsTeleportDest(){	return true;			}
	virtual void		UpdateAxis( const idMat3 &newAxis )	{				}

	virtual bool		Recharges() const { return true; }

	virtual hhShuttle	*GetDockedShuttle(void);

protected:
	void				AttachShuttle(hhShuttle *shuttle);
	void				DetachShuttle(hhShuttle *shuttle);
	hhBeamSystem *		SpawnDockingBeam(idVec3 &offset);
	void				SpawnConsole();
	virtual void		Lock();
	virtual void		Unlock();

	void				Event_SpawnConsole();
	virtual void		Event_Remove();
	virtual void		Event_PostSpawn();

protected:
	idVec3				offsetNozzle;
	idVec3				offsetConsole;
	idVec3				offsetShuttlePoint;
	idEntityPtr<hhShuttle>	dockedShuttle;
	idEntityPtr<hhBeamSystem> dockingBeam;
	hhForce_Converge	dockingForce;
	int					shuttleCount;
	int					lastConsoleAttempt;
	int					amountHealth;
	int					amountPower;
	float				maxDistance;
	bool				bLocked;
	bool				bLockOnEntry;
	bool				bCanExitLocked;
	bool				bPlayingRechargeSound;
};

#endif
