//==========================================================================
//
//	hhShuttle
//
//==========================================================================

class hhRailShuttle : public hhVehicle {
	CLASS_PROTOTYPE( hhRailShuttle );

#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	virtual void			Event_FireCannon() {};
	void					Event_HideArcs() {};
	void					Event_PostSpawn() {};
	void					Event_SetOrbitRadius(float radius, int lerpTime) {};
	void					Event_Jump() {};
	void					Event_FinalJump() {};
	void					Event_IsJumping() {};
	void					Event_Disengage() {};
	void					Event_RestorePower() {};
	void					Event_GetCockpit() {};
	idEntity *				GetTurret() {	return NULL;	}
	virtual bool			WillAcceptPilot( idActor *act ) { return false; }
#else

public:
	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void			ClientPredictionThink( void );
	virtual void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

	// Static pilot assessor, for queries when you don't have a vehicle instantiated
	static bool				ValidPilot( idActor *act );
	virtual bool			WillAcceptPilot( idActor *act );
	virtual void			AcceptPilot( hhPilotVehicleInterface* pilotInterface );
	virtual void			ProcessPilotInput( const usercmd_t* cmds, const idAngles* viewAngles );
	virtual void			BecomeConsole();
	virtual void			BecomeVehicle();
	idEntity *				GetTurret() {	return turret.GetEntity();	}

	virtual idVec3			GetFireOrigin();				// Called by firecontroller
	virtual idMat3			GetFireAxis();					// Called by firecontroller

protected:
	void					PlayWallMovementSound();
	void					StopWallMovementSound();
	void					PlayAirMovementSound();
	void					StopAirMovementSound();
	virtual void			Event_FireCannon();
	void					Event_HideArcs();
	void					Event_PostSpawn();
	void					Event_SetOrbitRadius(float radius, int lerpTime);
	void					Event_Jump();
	void					Event_FinalJump();
	void					Event_IsJumping();
	void					Event_Disengage();
	void					Jump();
	void					Event_RestorePower();
	float					UpdateJump( const usercmd_t* cmds );
	void					SetCockpitParm( int parmNumber, float value );
	void					Event_GetCockpit();

protected:
	float					defaultRadius;
	idInterpolate<float>	sphereRadius;
	idAngles				sphereAngles;
	idAngles				sphereVelocity;
	idAngles				sphereAcceleration;
	idVec3					sphereCenter;
	float					spherePitchMin;
	float					spherePitchMax;
	float					turretYawLimit;
	float					turretPitchLimit;
	float					linearFriction;
	float					terminalVelocity;
	idAngles				localViewAngles;
	idAngles				oldViewAngles;
	idEntityPtr<idEntity>	turret;
	idEntityPtr<idEntity>	canopy;
	idEntityPtr<hhBeamSystem>	leftArc;
	idEntityPtr<hhBeamSystem>	rightArc;
	bool					bDisengaged;
	bool					bJumping;
	bool					bBounced;
	int						jumpStartTime;
	int						jumpStage;
	float					jumpSpeed;
	float					jumpPosition;
	int						jumpNumBounces;
	float					jumpInitSpeed;
	float					jumpAccel;
	float					jumpAccelTime;
	float					jumpReturnForce;
	int						jumpBounceCount;
	float					jumpBounceFactor;
	float					jumpThrustMovementScale;
	float					jumpBounceMovementScale;
	bool					bWallMovementSoundPlaying;
	bool					bAirMovementSoundPlaying;
	//HUMANHEAD PCF mdl 04/29/06 - Added bUpdateViewAngles to prevent view angles from changing on load
	bool					bUpdateViewAngles;
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
};
