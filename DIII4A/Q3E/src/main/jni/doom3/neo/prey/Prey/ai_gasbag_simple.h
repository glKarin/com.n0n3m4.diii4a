#ifndef __PREY_AI_GASBAG_SIMPLE_H__
#define __PREY_AI_GASBAG_SIMPLE_H__

extern const idEventDef EV_NewPod;

class hhPod;

class hhGasbagSimple : public hhMonsterAI {
public:
	CLASS_PROTOTYPE(hhGasbagSimple);

#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	void Event_AcidBlast(void) {};
	void Event_AcidDrip(void) {};
	void Event_DeathCloud(void) {};
	void Event_NewPod(hhPod *pod) {};
	void Event_LaunchPod(void) {};
	void Event_SpawnBlastDebris(void) {};
	void Event_ChargeEnemy(void) {};
	void Event_GrabEnemy(void) {};
	void Event_BiteEnemy(void) {};
	void Event_DirectMoveToPosition(const idVec3 &pos) {};
	void Event_BindUnfroze(idEntity *unfrozenBind) {};
	virtual void Event_EnemyIsSpirit( hhPlayer *player, hhSpiritProxy *proxy ) {};
	virtual void Event_EnemyIsPhysical( hhPlayer *player, hhSpiritProxy *proxy ) {};
	void Event_GrabCheck(void) {};
	void Event_MoveToGrabPosition(void) {};
	void Event_CheckRange(void) {};
	void Event_EnemyRangeZ(void) {};
#else
	virtual ~hhGasbagSimple(void);

	void Spawn(void);
	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);

	virtual void FlyTurn(void);

	virtual void Gib(const idVec3 &dir, const char *damageDefName);
	int ReactionTo( const idEntity *ent );

protected:
	void Event_AcidBlast(void);
	void Event_AcidDrip(void);
	void Event_DeathCloud(void);
	void Event_NewPod(hhPod *pod);
	void Event_LaunchPod(void);
	void Event_SpawnBlastDebris(void);
	void Event_ChargeEnemy(void);
	void Event_GrabEnemy(void);
	void Event_BiteEnemy(void);
	void Event_DirectMoveToPosition(const idVec3 &pos);
	void Event_BindUnfroze(idEntity *unfrozenBind);
	virtual void Event_EnemyIsSpirit( hhPlayer *player, hhSpiritProxy *proxy );
	virtual void Event_EnemyIsPhysical( hhPlayer *player, hhSpiritProxy *proxy );
	void Event_GrabCheck(void);
	void Event_MoveToGrabPosition(void);
	void Event_CheckRange(void);
	void Event_EnemyRangeZ(void);

	bool GrabEnemy(void);

	virtual void LinkScriptVariables(void);

	virtual void Ticker(void);
	virtual idProjectile *LaunchProjectileAtVec(const char *jointname, const idVec3 &target, bool clampToAttackCone, const idDict* desiredProjectileDef);
	virtual idProjectile *LaunchProjectileAtVec(const idVec3 &startOrigin, const idMat3 &startAxis, const idVec3 &target, bool clampToAttackCone, const idDict* desiredProjectileDef);
	virtual void Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	virtual	void Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location);
	virtual void SetEnemy(idActor *newEnemy);

	idVec3 podOffset;
	float podRange;
	int dripCount;

	idList< idEntityPtr<hhPod> > podList;

	idScriptInt AI_PODCOUNT;
	idScriptBool AI_CHARGEDONE;
	idScriptBool AI_SWOOP;
	idScriptInt AI_DODGEDAMAGE; 

	int nextWoundTime;

	idEntityPtr<hhBindController>		bindController;					// Bind controller for tractor beam
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
};

#endif

