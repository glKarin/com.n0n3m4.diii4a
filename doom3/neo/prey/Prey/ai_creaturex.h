
#ifndef __PREY_AI_CREATURE_H__
#define __PREY_AI_CREATURE_H__


class hhCreatureX : public hhMonsterAI {

public:

	CLASS_PROTOTYPE(hhCreatureX);
#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	void				Event_GunRecharge( int onOff ) {};
	void				Event_LaserOn() {};
	void				Event_LaserOff() {};
	void				Event_UpdateLasers() {};
	void				Event_SetGunOffset( const idAngles &ang ) {};
	void				Event_EndLeftBeams() {};
	void				Event_EndRightBeams() {};
	void				Event_AssignRightMuzzleFx( hhEntityFx* fx ) {};
	void				Event_AssignLeftMuzzleFx( hhEntityFx* fx ) {};
	void				Event_AssignRightImpactFx( hhEntityFx* fx ) {};
	void				Event_AssignLeftImpactFx( hhEntityFx* fx ) {};
	void				Event_AssignLeftRechargeFx( hhEntityFx* fx ) {};
	void				Event_AssignRightRechargeFx( hhEntityFx* fx ) {};
	void				Event_AttackMissile( const char *jointname, const idDict *projDef, int boneDir ) {};
	void				Event_HudEvent( const char *eventName ) {};
	void				Event_RechargeHealth() {};
	void				Event_EndRecharge() {};
	void				Event_ResetRechargeBeam() {};
	void				Event_SparkLeft() {};
	void				Event_SparkRight() {};
	void				Event_LeftGunDeath() {};
	void				Event_RightGunDeath() {};
	void				Event_StartRechargeBeams() {};
#else

						~hhCreatureX();
	void				Spawn();
	void				LinkScriptVariables( void );
	void				Think();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	void				Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

	idScriptBool		AI_GUN_TRACKING;
	idScriptBool		AI_RECHARGING;
	idScriptBool		AI_LEFT_FIRE;
	idScriptBool		AI_RIGHT_FIRE;
	idScriptBool		AI_LEFT_DAMAGED;
	idScriptBool		AI_RIGHT_DAMAGED;
	idScriptBool		AI_FIRING_LASER;
	idScriptBool		AI_HEALTH_TICK;
	idScriptBool		AI_GUN_EXPLODE;
protected:
	void				Event_GunRecharge( int onOff );
	void				Event_LaserOn();
	void				Event_LaserOff();
	void				Event_UpdateLasers();
	void				Event_SetGunOffset( const idAngles &ang );
	void				Event_EndLeftBeams();
	void				Event_EndRightBeams();
	void				Event_AssignRightMuzzleFx( hhEntityFx* fx );
	void				Event_AssignLeftMuzzleFx( hhEntityFx* fx );
	void				Event_AssignRightImpactFx( hhEntityFx* fx );
	void				Event_AssignLeftImpactFx( hhEntityFx* fx );
	void				Event_AssignLeftRechargeFx( hhEntityFx* fx );
	void				Event_AssignRightRechargeFx( hhEntityFx* fx );
	void				Event_AttackMissile( const char *jointname, const idDict *projDef, int boneDir );
	void				Event_HudEvent( const char *eventName );
	void				Event_RechargeHealth();
	void				Event_EndRecharge();
	void				Event_ResetRechargeBeam();
	void				Event_SparkLeft();
	void				Event_SparkRight();
	void				Event_LeftGunDeath();
	void				Event_RightGunDeath();
	void				Event_StartRechargeBeams();

	void				MuzzleLeftOn();
	void				MuzzleRightOn();
	void				MuzzleLeftOff();
	void				MuzzleRightOff();
	bool				UpdateAnimationControllers( void );
	virtual void		Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void				Show();
	void				Activate( idEntity *activator );
	idEntityPtr<hhBeamSystem> laserRight;
	idEntityPtr<hhBeamSystem> laserLeft;
	idEntityPtr<hhBeamSystem> preLaserRight;
	idEntityPtr<hhBeamSystem> preLaserLeft;
	idList< idEntityPtr<hhBeamSystem> >	leftBeamList;
	idList< idEntityPtr<hhBeamSystem> >	rightBeamList;
	idEntityPtr<hhBeamSystem> leftRechargeBeam;
	idEntityPtr<hhBeamSystem> rightRechargeBeam;
	idEntityPtr<hhBeamSystem> leftDamageBeam;
	idEntityPtr<hhBeamSystem> rightDamageBeam;
	idEntityPtr<hhBeamSystem> leftRetractBeam;
	idEntityPtr<hhBeamSystem> rightRetractBeam;
	idEntityPtr<hhEntityFx> muzzleLeftFx;
	idEntityPtr<hhEntityFx> muzzleRightFx;
	idEntityPtr<hhEntityFx> impactLeftFx;
	idEntityPtr<hhEntityFx> impactRightFx;
	idEntityPtr<hhEntityFx> rechargeLeftFx;
	idEntityPtr<hhEntityFx> rechargeRightFx;
	idEntityPtr<hhEntityFx> retractLeftFx;
	idEntityPtr<hhEntityFx> retractRightFx;
	int					numBurstBeams;
	bool				bLaserLeftActive;
	bool				bLaserRightActive;
	idVec3				targetStart_L;
	idVec3				targetEnd_L;
	idVec3				targetCurrent_L;
	idVec3				targetStart_R;
	idVec3				targetEnd_R;
	idVec3				targetCurrent_R;
	float				targetAlpha_L;
	float				targetAlpha_R;
	int					leftGunHealth;
	int					rightGunHealth;
	int					leftGunLives;
	int					rightGunLives;
	idAngles			gunShake;
	int					nextBeamTime;
	int					nextLeftZapTime;
	int					nextRightZapTime;
	idVec3				laserEndLeft;
	idVec3				laserEndRight;
	int					nextLaserLeft;
	int					nextLaserRight;
	int					nextHealthTick;
	int					leftRechargerHealth;
	int					rightRechargerHealth;
	idEntityPtr<hhMonsterAI> leftRecharger;
	idEntityPtr<hhMonsterAI> rightRecharger;
	bool				bScripted;
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
};

#endif /* __PREY_AI_CREATURE_H__ */
