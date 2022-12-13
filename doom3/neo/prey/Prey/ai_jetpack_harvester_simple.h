
#ifndef __PREY_AI_JETPACK_HARVESTER_SIMPLE_H__
#define __PREY_AI_JETPACK_HARVESTER_SIMPLE_H__


class hhJetpackHarvesterSimple : public hhMonsterAI {

public:	
	CLASS_PROTOTYPE(hhJetpackHarvesterSimple);	

#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	void				Event_AppendHoverFx( hhEntityFx* fx ) {};
	void				Event_AppendToThrusterList( hhEntityFx* fx ) {};
	void				Event_RemoveJetFx(void) {};
	void				Event_CreateJetFx(void) {};
	void				Event_LaunchMine( const idList<idStr>* parmList ) {};
	void				Event_OnProjectileLaunch( hhProjectile *proj ) {};
	void				Event_DodgeProjectile( hhProjectile *proj ) {};
	void				Event_IsEnemySniping(void) {};
	void				Event_NumMines(void) {};
#else
protected:
	enum ThrustSide
	{
		ThrustSide_Left, 
		ThrustSide_Right,
		ThrustSide_Total
	};

	enum ThrustType
	{
		ThrustType_Idle = 0,
		ThrustType_Hover,		
		ThrustType_Total
	};
	
	~hhJetpackHarvesterSimple();
	void				Spawn(void);
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	void				AnimMove( void );
	void				Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	idProjectile*		LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, const idDict* desiredProjectileDef );
	void				Event_AppendHoverFx( hhEntityFx* fx );
	void				Event_AppendToThrusterList( hhEntityFx* fx );
	void				Event_RemoveJetFx(void);
	void				Event_CreateJetFx(void);
	void				Event_LaunchMine( const idList<idStr>* parmList );
	void				Event_OnProjectileLaunch( hhProjectile *proj );
	void				Event_DodgeProjectile( hhProjectile *proj );
	void				Event_IsEnemySniping(void);
	void				Event_NumMines(void);
	virtual void		Hide( void );
	virtual void		Show( void );
	virtual bool		Collide( const trace_t &collision, const idVec3 &velocity );
	void				Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	idEntityPtr<hhEntityFx>		fxThrusters[ThrustType_Total][ThrustSide_Total];
	int					lastAntiProjectileAttack;
	bool				allowPreDeath;
	idList< idEntityPtr<hhProjectile> >	mineList;
	bool				freezeDamage;
	bool				specialDamage;
#endif
};

#endif