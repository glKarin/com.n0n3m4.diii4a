
#ifndef __GAME_PORTAL_H__
#define __GAME_PORTAL_H__

#if GAMEPORTAL_PVS

// Allows us to create a PVS link between two areas
class hhArtificialPortal : public idEntity {
	CLASS_PROTOTYPE( hhArtificialPortal );
public:
	void			Spawn();
	void			SetPortalState(bool openPVS, bool openSound);

	void			Save(idSaveGame *savefile) const;
	void			Restore(idRestoreGame *savefile);

protected:
	void			Event_SetPortalState(bool openPVS, bool openSound);

	qhandle_t		areaPortal;		// 0 = no portal
};

#endif

typedef struct proximityEntity_s {
	idEntityPtr<idEntity>	entity;
	idVec3					lastPortalPoint;
} proximityEntity_t;

// jsh - Changed from idEntity to hhAnimatedEntity
class hhPortal : public hhAnimatedEntity {
public:
	CLASS_PROTOTYPE( hhPortal );

	typedef enum {
		PORTAL_OPENING,
		PORTAL_OPENED,
		PORTAL_CLOSING,
		PORTAL_CLOSED,
	} portalStates_t;
	

	hhPortal(void);
	~hhPortal();

	void			Spawn(void);
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void	ClientPredictionThink( void );

	void			CheckPlayerDistances(void);
	void			Think( void );
	bool			PortalEntity( idEntity *ent, const idVec3 &point );
	bool			PortalTeleport( idEntity *ent, const idVec3 &origin, const idMat3 &axis, const idMat3 &sourceAxis, const idMat3 &destAxis );
	bool			IsActive(void) { return(portalState < PORTAL_CLOSING); }
	virtual void	SetGravity( const idVec3& newGravity ) { portalGravity = newGravity; }
	const idVec3&	GetGravity( void ) const { return portalGravity; }
	void			PostSpawn( void );

	bool			AttemptPortal( idPlane &plane, idEntity *hit, idVec3 location, idVec3 nextLocation );

	void			PortalProjectile( hhProjectile *projectile, idVec3 collideLocation, idVec3 nextLocation );

	virtual bool	CheckPortal( const idEntity *other, int contentMask );
	virtual bool	CheckPortal( const idClipModel *mdl, int contentMask );
	virtual void	CollideWithPortal( const idEntity *other );  // CJR PCF 04/26/06
	virtual void	CollideWithPortal( const idClipModel *mdl ); // CJR PCF 04/26/06

	void			AddProximityEntity( const idEntity *other);

protected:
	void			Event_Opened( void );
	void			Event_Closed( void );
	void			Event_Trigger(idEntity *activator);
	void			Event_ResetGravity() { portalGravity = hhUtils::GetLocalGravity(GetOrigin(), GetPhysics()->GetBounds(), gameLocal.GetGravity() ); }

	void			Event_PortalSpark( void );
	void			Event_PortalSparkEnd( void );

	void			Event_ShowGlowPortal( void );
	void			Event_HideGlowPortal( void );

	// Actual logic for the trigger functionality.  (Needed to break out for partering :)
	void			Trigger( idEntity *activator );

	// Methods used to sync up portals
	hhPortal *		GetMasterPortal() { return( masterPortal.GetEntity() ); }
	void			SetMasterPortal( hhPortal *master ) { masterPortal = master; }
	void			AddSlavePortal( hhPortal *slave ) { slavePortals.Append( slave ); }
	void			CheckForBuddy();
	
	void			TriggerTargets();

private:
#if GAMEPORTAL_PVS
	qhandle_t			areaPortal;		// 0 = no portal
#endif

	portalStates_t		portalState;

	bool				bNoTeleport; // Is purely a visual portal.  Will not try to teleport anything near it
	bool				bGlowPortal; // This portal has visual/audio effects

	idVec3				portalGravity; // Local gravity to this portal.  Will be changed by any zones the portal is within
	
	float				closeDelay;		// Secs to wait before automatically closing when opened - nla
	float				distanceToggle; //rww - toggles the associated game portal on and off based on nearest player distance, 0.0 means not enabled, otherwise value is the distance to turn on at.
	float				distanceCull; //rww - distance to shut off the vis gameportal but not the render portal
	bool				areaPortalCulling; //rww - if the portal is currently "culled" (off) due to distanceCull checking
	bool				monsterportal;
	bool				alertMonsters;

	idList<idEntityPtr<hhPortal> >	slavePortals;
	idEntityPtr<hhPortal>			masterPortal;

	idList<proximityEntity_t>		proximityEntities;

	idEntityPtr<hhEntityFx>			m_portalIdleFx; //rww - primarily for mp, the effect closed pop-open portals play
};

#endif /* __GAME_PORTAL_H__ */
