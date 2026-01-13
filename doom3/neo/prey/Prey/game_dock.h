#ifndef __GAME_DOCK_H__
#define __GAME_DOCK_H__

extern const idEventDef EV_DockLock;
extern const idEventDef EV_DockUnlock;

class hhShuttle;
class hhDockingZone;

class hhDock : public idEntity {
public:
	ABSTRACT_PROTOTYPE( hhDock );

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual bool		ValidEntity(idEntity *ent) = 0;
	virtual void		EntityEntered( idEntity *ent ) = 0;
	virtual void		EntityEncroaching( idEntity *ent ) = 0;
	virtual void		EntityLeaving( idEntity *ent ) = 0;
	virtual void		ShuttleExit( hhShuttle *shuttle ) = 0;
	virtual bool		IsLocked() = 0;
	virtual bool		CanExitLocked() = 0;
	virtual bool		AllowsBoost() = 0;
	virtual bool		AllowsFiring() = 0;
	virtual bool		AllowsExit() = 0;
	virtual bool		IsTeleportDest() = 0;
	virtual void		UpdateAxis( const idMat3 &newAxis ) = 0;

	virtual bool		Recharges() const { return false; }

protected:
	//rww - needs a delay for mp
	virtual void		Event_PostSpawn();

	hhDockingZone*		SpawnDockingZone();
	virtual void		Lock()=0;
	virtual void		Unlock()=0;

	void				Event_Lock();
	void				Event_Unlock();
	void				Event_Activate(idEntity *activator);

protected:
	idEntityPtr<hhDockingZone>	dockingZone;
};

#endif