
#ifndef __PREY_AI_DROID_H__
#define __PREY_AI_DROID_H__

class hhDroid : public hhMonsterAI {

public:

	CLASS_PROTOTYPE(hhDroid);

#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	void Event_StartBurst(void) {};
	void Event_EndBurst(void) {};
	void Event_StartStaticBeam(void) {};
	void Event_EndStaticBeam(void) {};
	void Event_StartChargeShot(void) {};
	void Event_EndChargeShot(void) {};
	void Event_StartPathing(void) {};
	void Event_EndPathing(void) {};
	void Event_FlyZip() {};
	void Event_EndZipBeams() {};
	void Event_HealerReset() {}
#else
	hhDroid();
	~hhDroid();
	void				Spawn( void );
	virtual	void		Think( void );
	void				FlyTurn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	void				AdjustFlySpeed( idVec3 &vel );
protected:
	idList< idEntityPtr<hhBeamSystem> >	beamBurstList;
	idEntityPtr<hhBeamSystem> beamZip;
	idEntityPtr<idEntity> staticPoint;
	idEntityPtr<hhProjectileTracking> chargeShot;
	void Event_StartBurst(void);
	void Event_EndBurst(void);
	void Event_StartStaticBeam(void);
	void Event_EndStaticBeam(void);
	void Event_StartChargeShot(void);
	void Event_EndChargeShot(void);
	void Event_StartPathing(void);
	void Event_EndPathing(void);
	void Event_FlyZip();
	void Event_EndZipBeams();
	void Event_HealerReset();
	idProjectile		*LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, const idDict* desiredProjectileDef );
	virtual bool		Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void		Show( void );
	void				FlyMove( void );
	void				Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void				UpdateModelTransform();

	idVec3				savedGravity;
	float				chargeShotSize;
	float				burstLength;
	float				burstSpread;
	float				burstDuration;
	float				staticDuration;				//how long staticbeam lasts
	float				staticRange;				//distance to check for staticbeam points
	int					numBurstBeams;
	idVec3				beamOffset;
	float				old_fly_bob_strength;
	float				enemyRange;
	float				flyDampening;
	int					lives;
	float				spinAngle;
	bool				bHealer;
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
};

#endif