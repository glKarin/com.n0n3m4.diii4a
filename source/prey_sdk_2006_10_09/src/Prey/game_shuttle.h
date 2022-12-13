#ifndef __GAME_SHUTTLE_H__
#define __GAME_SHUTTLE_H__

class hhBindController;
class hhShuttle;

#define TIP_STATE_NONE			0
#define TIP_STATE_DOCKED		1
#define TIP_STATE_UNDOCKED		2

//==========================================================================
//
//	hhTractorBeam
//
//==========================================================================

class hhTractorBeam : public idClass {
	CLASS_PROTOTYPE( hhTractorBeam );

	friend		hhShuttle;

public:
				hhTractorBeam();
	void		SpawnComponents( hhShuttle *shuttle );

	void		Save( idSaveGame *savefile ) const;
	void		Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );

	bool		Exists();
	bool		IsAllowed();
	bool		IsActive();
	bool		IsLocked();

	idEntity *	GetTarget();
	void		SetAllow( bool allow )			{	bAllow = allow;		}
	void		Update();
	idEntity *	GetTraceTarget()				{	return traceTarget.GetEntity();	}
	void		RequestState( bool wantsActive );
	idEntity *	TraceForTarget( trace_t &results );
	void		LaunchTarget(float speed);

protected:
	void		SetOwner( hhShuttle *shuttle )	{	owner = shuttle;	}
	void		Activate();
	void		Deactivate();
	void		AttachTarget( idEntity *ent, trace_t &results );
	bool		ValidTarget( idEntity *ent );
	void		ReleaseTarget();

protected:
	idEntityPtr<hhShuttle>				owner;
	idEntityPtr<hhBeamSystem>			beam;
	idEntityPtr<idEntity>				traceTarget;					// Results of target trace this frame
	idEntityPtr<hhBindController>		bindController;					// Bind controller for tractor beam
	idEntityPtr<idEntity>				traceController;				// Point for traces
	hhForce_Converge		feedbackForce;					// feedback force applied to shuttle
	int						tractorCost;					// Power cost of tractor at 10 Hz
	bool					bAllow;							// false if tractor beam is not allowed
	bool					bActive;						// whether beam is active
	int						beamClientPredictTime;			//rww - don't flicker the beam due to snapshot timings
	idVec3					offsetTraceStart;				// Offset to start of trace
	idVec3					offsetTraceEnd;					// Offset to end of trace
	idVec3					offsetEquilibrium;				// Offset to equilibrium point for targets
	idVec3					targetLocalOffset;				// Offset to attach point in entity local coordinates
	int						targetID;						// Body ID of target entity
};

//==========================================================================
//
//	hhShuttle
//
//==========================================================================

class hhShuttle : public hhVehicle {
	CLASS_PROTOTYPE( hhShuttle );

	friend					hhTractorBeam;

public:
	virtual					~hhShuttle();

	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );

	idEntity *				GetTractorTarget()				{	return tractor.GetTarget();	}
	void					ReleaseTractorTarget()			{	tractor.ReleaseTarget();	}
	void					AllowTractor( bool allow )		{	tractor.SetAllow(allow);	}
	virtual bool			TractorIsActive(void)			{	return tractor.IsActive();	} //rww
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void			PerformDeathAction( int deathAction, idActor *savedPilot, idEntity *attacker, idVec3 &dir );
	void					LaunchTractorTarget(float mag)	{	tractor.LaunchTarget(mag);	}

	// Static pilot assessor, for queries when you don't have a vehicle instantiated
	static bool				ValidPilot( idActor *act );

	virtual bool			WillAcceptPilot( idActor *act );
	virtual void			ProcessPilotInput( const usercmd_t* cmds, const idAngles* viewAngles );
	virtual void			AcceptPilot( hhPilotVehicleInterface* pilotInterface );
	virtual void			EjectPilot();

	virtual void			FireThrusters( const idVec3& impulse );
	void					ApplyBoost( float magnitude );

	virtual void			DrawHUD( idUserInterface* _hud );

	virtual void			RemoveVehicle();

	virtual	void			InitiateRecharging();
	virtual void			FinishRecharging();
	virtual void			SetDock( const hhDock* dock );
	virtual void			Undock();
	void					RecoverFromDying();

	int						noTractorTime; //HUMANHEAD rww
protected:
	virtual void			Ticker();
	hhVehicleThruster *		SpawnThruster( idVec3 &offset, idVec3 &dir, const char *thrusterName, bool master );
	void					UpdateEffects( const idVec3& impulse );

	virtual void			SetConsoleModel();
	virtual void			SetVehicleModel();

protected:
	void					Event_PostSpawn();
	virtual void			Event_FireCannon();
	void					Event_ActivateTractorBeam();
	void					Event_DeactivateTractorBeam();

protected:
	hhTractorBeam					tractor;
	idEntityPtr<hhVehicleThruster>	thrusters[ THRUSTER_DIRECTIONS ];
	float							terminalVelocitySquared;		// Maximum speed of vehicle
	idVec3							teleportDest;					// teleport destination upon death
	idAngles						teleportAngles;					// teleport destination upon death
	float							malfunctionThrustFactor;
	bool							bSputtering;
	int								idealTipState;					// desired tip state
	int								curTipState;					// current tip state
	int								nextTipChange;					// next time a tip change can happen
};

#endif
