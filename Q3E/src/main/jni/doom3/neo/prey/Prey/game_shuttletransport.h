
#ifndef __GAME_SHUTTLETRANSPORT_H__
#define __GAME_SHUTTLETRANSPORT_H__

class hhShuttleTransport : public hhDock {
public:
	CLASS_PROTOTYPE( hhShuttleTransport );

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Think();
	virtual bool		ValidEntity(idEntity *ent);
	virtual void		EntityEntered(idEntity *ent);
	virtual void		EntityEncroaching(idEntity *ent);
	virtual void		EntityLeaving(idEntity *ent);
	virtual void		ShuttleExit(hhShuttle *shuttle);	
	virtual bool		IsLocked()		{	return bLocked;			}
	virtual bool		CanExitLocked()	{	return bCanExitLocked;	}
	virtual bool		AllowsBoost()	{	return false;			}
	virtual bool		AllowsFiring()	{	return bAllowFiring;	}
	virtual bool		AllowsExit()	{	return false;			}
	virtual bool		IsTeleportDest(){	return false;			}
	virtual void		UpdateAxis( const idMat3 &newAxis );

protected:
	void				AttachShuttle(hhShuttle *shuttle);
	void				DetachShuttle(hhShuttle *shuttle);
	hhBeamSystem *		SpawnDockingBeam(idVec3 &offset);
	virtual void		Lock();
	virtual void		Unlock();
	void				Event_FadeOut();
	virtual void		Event_Remove();

protected:
	idVec3				offsetNozzle;
	idVec3				offsetShuttlePoint;
	hhShuttle *			dockedShuttle;
	hhBeamSystem *		dockingBeam;
	hhForce_Converge	dockingForce;
	int					shuttleCount;
	int					amountHealth;
	int					amountPower;
	bool				bLocked;
	bool				bLockOnEntry;
	bool				bCanExitLocked;
	bool				bAllowFiring;
};

#endif
