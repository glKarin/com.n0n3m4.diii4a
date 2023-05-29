
#ifndef __PREY_SPIRITPROXY_H__
#define __PREY_SPIRITPROXY_H__

// nla - Forward declare 
class hhPlayer;

extern const idEventDef EV_OrientToGravity;

// SPIRIT PROXY ===============================================================

class hhSpiritProxy : public idActor {
public:
	CLASS_PROTOTYPE( hhSpiritProxy );
							hhSpiritProxy( void );
	void					Spawn( void );
	static hhSpiritProxy	*CreateProxy( const char *name, hhPlayer *owner, const idVec3& origin, const idMat3& bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis );

	virtual void			SetModel( const char *modelname ); //rww

	virtual void			UpdateModelForPlayer(void); //rww
	virtual void			Think();
	virtual void			ActivateProxy( hhPlayer *owner, const idVec3& origin, const idMat3& bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis );
	virtual void			DeactivateProxy( void );
	virtual void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual bool			IsWallWalking() const;
	virtual bool			IsCrouching() const;
	virtual bool			ShouldRemainAlignedToAxial();
	virtual void			RestorePlayerLocation( const idVec3& origin, const idMat3& bboxAxis, const idVec3& viewDir, const idAngles& angles );
	virtual void			StartAnimation();
	hhPlayer*				GetPlayer(void) const { return player.GetEntity(); }	// JRM
	void					OrientToGravity( bool orient );
	virtual void			ShouldRemainAlignedToAxial( bool remainAligned );
	virtual bool			ShouldRemainAlignedToAxial() const { return physicsObj.ShouldRemainAlignedToAxial(); }
	virtual void			UpdateModelTransform( void ); // mdl
	virtual bool			AllowCollision(const trace_t &collision); //rww

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );
	virtual void			NetZombify(void);

	//rww - for mp player icons transferring to the prox
	void					DrawPlayerIcons( void );
	void					HidePlayerIcons( void );
	bool					NeedsIcon( void );

	ID_INLINE float			GetActivationTime( void ) const { return activationTime; }

public:
	//Overridden methods
	virtual void			SetAxis( const idMat3& axis ) { idEntity::SetAxis( axis ); }

	void					Event_SpawnEffect();
	void					Event_OrientToGravity( bool orient );
	void					Event_ResetGravity();
	void					Event_ShouldRemainAlignedToAxial( bool remainAligned );
	void					Event_AssignSpiritFx( hhEntityFx* fx );

protected:
	void					CrashLand(const idVec3 &oldOrigin, const idVec3 &oldVelocity);

	hhPhysics_Player		physicsObj;

	idEntityPtr<hhPlayer>	player;

	idAngles				viewAngles;
	idMat3					eyeAxis;

	idEntityPtr<idEntityFx>	spiritFx;

	int						cachedCurrentWeapon;
	int						activationTime; // Saved to keep the player from being bounced back into the body too quickly if damaged

	//rww - only matters for client keeping track of if animation has started. no need to save/restore.
	bool					clientAnimated;
	//rww - only for telling clients which anim we are playing
	int						netAnimType;

	//rww - for keeping track of player model changes in mp
	int						playerModelNum;

	//rww - showing player status icons on the proxy in mp
	//proxy could optionally support lag/chat/etc if we wanted to sync those states on the proxy itself
	//idPlayerIcon			playerIcon;
	hhPlayerTeamIcon		playerTeamIcon;
};

/*
=====================
hhSpiritProxy::IsWallWalking
=====================
*/
ID_INLINE bool hhSpiritProxy::IsWallWalking() const { 
	return physicsObj.IsWallWalking();
}

/*
=====================
hhSpiritProxy::IsCrouching
=====================
*/
ID_INLINE bool hhSpiritProxy::IsCrouching() const { 
	return physicsObj.IsCrouching();
}

/*
=====================
hhSpiritProxy::ShouldRemainAlignedToAxial
=====================
*/
ID_INLINE bool hhSpiritProxy::ShouldRemainAlignedToAxial() {
	return physicsObj.ShouldRemainAlignedToAxial();
}

// DEATH PROXY ================================================================

class hhDeathProxy : public hhSpiritProxy {
public:
	CLASS_PROTOTYPE( hhDeathProxy );

	~hhDeathProxy();

	void				Spawn();
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void		ActivateProxy( hhPlayer *owner, const idVec3& origin, const idMat3 &bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis );
	virtual void		StartAnimation();

protected:
	void				Event_SpawnEffect();
	void				Event_Activate();

protected:
	idVec3				lastPhysicalLocation;				// location to return to from deathwalk
	idMat3				lastPhysicalAxis;					// orientation to return to from deathwalk
};

// MP DEATHWALK PROXY ================================================================
//rww - for corpses in multiplayer
class hhMPDeathProxy : public hhDeathProxy {
public:
	CLASS_PROTOTYPE( hhMPDeathProxy );

	void					Spawn();
	virtual void			ActivateProxy( hhPlayer *owner, const idVec3& origin, const idMat3 &bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis );

	virtual void			Event_CorpseRemove(void);

	void					SetFling(const idVec3 &point, const idVec3 &force);

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );

private:
	bool					hasInitial;
	bool					didFling;
	idVec3					initialPos;
	idCQuat					initialRot;
	idVec3					initialFlingForce;
};

// DEATHWALK PROXY ================================================================
//rww - this is the entity that sits in the middle of deathwalk and moves up/down.

class hhDeathWalkProxy : public hhSpiritProxy {
public:
	CLASS_PROTOTYPE( hhDeathWalkProxy );

	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual void			Think();
	virtual void			ActivateProxy( hhPlayer *owner, const idVec3& origin, const idMat3& bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis );
	void					SetInitialPos(const idVec3 &pos);
	virtual void			StartAnimation();

protected:
	idVec3					initialPos;
	float					bodyMoveScale;
	int						timeSinceStage2Started;
};

// POSSESSED PROXY ================================================================
// An invisible proxy that is spawned when the player is possessed

class hhPossessedProxy : public hhSpiritProxy {
public:
	CLASS_PROTOTYPE( hhPossessedProxy );

	virtual void			ActivateProxy( hhPlayer *owner, const idVec3& origin, const idMat3& bboxAxis, const idMat3& newViewAxis, const idAngles& newViewAngles, const idMat3& newEyeAxis );

protected:
};

#endif /* __PREY_SPIRITPROXY_H__ */
