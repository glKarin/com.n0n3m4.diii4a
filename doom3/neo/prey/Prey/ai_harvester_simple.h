
#ifndef __PREY_AI_HARVESTERSIMPLE_H__
#define __PREY_AI_HARVESTERSIMPLE_H__

extern const idEventDef AI_OnProjectileLaunch;
extern const idEventDef MA_EnterPassageway;
extern const idEventDef MA_ExitPassageway;

#define MAX_HARVESTER_LEGS 4

class hhHarvesterSimple : public hhMonsterAI {

public:	
	CLASS_PROTOTYPE(hhHarvesterSimple);	

#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	virtual void		Event_StartPreDeath(void) {};
	void				Event_OnProjectileLaunch(hhProjectile *proj) {};
	void				Event_AntiProjectileAttack(hhProjectile *proj) {};
	void				Event_EnterPassageway(hhAIPassageway *pn) {};
	void				Event_ExitPassageway(hhAIPassageway *pn) {};
	void				Event_HandlePassageway(void) {};
	void				Event_UseThisPassageway(hhAIPassageway *pn, bool force) {};
	void				Event_AppendFxToList(hhEntityFx *fx) {};
	void				Event_GibOnDeath(const idList<idStr>* parmList) {};
	virtual void		Event_Activate(idEntity *activator) {};
	virtual void		Event_CanBecomeSolid(void) {};
#else
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void		LinkScriptVariables(void);
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

protected:
	
	void				Spawn(void);

	virtual int			EvaluateReaction( const hhReaction *react );

	void				SpawnSmoke(void);
	void				ClearSmoke(void);

	//predeath torso code
	virtual void		Event_StartPreDeath(void);
	virtual bool		Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	bool				allowPreDeath;					// TRUE if we will allow predeath

	virtual void		AnimMove(void);

	//anti-projectile chaff
	void				Event_OnProjectileLaunch(hhProjectile *proj);
	void				Event_AntiProjectileAttack(hhProjectile *proj);
	void				Event_EnterPassageway(hhAIPassageway *pn);
	void				Event_ExitPassageway(hhAIPassageway *pn);
	void				Event_HandlePassageway(void);
	void				Event_UseThisPassageway(hhAIPassageway *pn, bool force);
	void				Event_AppendFxToList(hhEntityFx *fx);
	void				Event_GibOnDeath(const idList<idStr>* parmList);
	virtual void		Event_Activate(idEntity *activator);
	virtual void		Event_CanBecomeSolid(void);

	int					lastAntiProjectileAttack;
	int					lastPassagewayTime;
	idEntityPtr<idEntity>	lastPassageway;
	idEntityPtr<idEntity>	nextPassageway;
	idScriptFloat		AI_CLIMB_RANGE;
	idScriptFloat		AI_PASSAGEWAY_HEALTH;

	idEntityPtr<hhEntityFx>	fxSmoke[MAX_HARVESTER_LEGS];

	int passageCount;
	bool bSmokes;
	bool bGibOnDeath;
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
};

#endif