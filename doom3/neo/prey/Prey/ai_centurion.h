#ifndef __PREY_AI_CENTURION_H__
#define __PREY_AI_CENTURION_H__

/***********************************************************************
  hhCenturion.
	Centurion AI.
***********************************************************************/
class hhCenturion : public hhMonsterAI {

public:	
	CLASS_PROTOTYPE(hhCenturion);	

#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	void				Event_CenturionLaunch( const idList<idStr>* parmList ) {};
	void				Event_ScriptedRoar() {};
	void				Event_ScriptedArmChop() {};
	void				Event_CenturionLooseArm() {};
	void				Event_ForceFieldNotify() {};
	void				Event_ForceFieldToggle( int toggle ) {};
	void				Event_PlayerInTunnel( int toggle, idEntity* ent ) {};
	void				Event_ReachedTunnel() {};
	void				Event_MoveToTunnel() {};

	bool				AttackMelee( const char *meleeDefName ) { return true; };
	void				Event_CheckForObstruction( int checkPathToPillar ) {};
	void				Event_MoveToObstruction() {};
	void				Event_DestroyObstruction() {};
	void				Event_ReachedObstruction() {};
	void				Event_CloseToObstruction() {};
	void				Event_EnemyCloseToObstruction() {};
	void				Event_BackhandImpulse( idEntity* ent ) {};
	virtual void		Event_PostSpawn( void ) {};
	void				Event_TakeDamage( int takeDamage ) {};
	void				Event_FindNearbyEnemy( float distance ) {};
	virtual void		Event_Touch( idEntity *other, trace_t *trace ) {};
#else

public:
	void				Spawn();
	void				Think();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
protected:
	void				Event_CenturionLaunch( const idList<idStr>* parmList );
	void				Event_ScriptedRoar();
	void				Event_ScriptedArmChop();
	void				Event_CenturionLooseArm();
	void				Event_ForceFieldNotify();
	void				Event_ForceFieldToggle( int toggle );
	void				Event_PlayerInTunnel( int toggle, idEntity* ent );
	void				Event_ReachedTunnel();
	void				Event_MoveToTunnel();

	void				LinkScriptVariables( void );
	bool				AttackMelee( const char *meleeDefName );
	void				Event_CheckForObstruction( int checkPathToPillar );
	void				Event_MoveToObstruction();
	void				Event_DestroyObstruction();
	void				Event_ReachedObstruction();
	void				Event_CloseToObstruction();
	void				Event_EnemyCloseToObstruction();
	void				Event_BackhandImpulse( idEntity* ent );
	virtual void		Event_PostSpawn( void );
	void				Event_TakeDamage( int takeDamage );
	void				Event_FindNearbyEnemy( float distance );
	virtual void		Event_Touch( idEntity *other, trace_t *trace );

	void				AimedAttackMissile( const char *jointname, const idDict *projDef );

	idEntityPtr<idEntity>	armchop_Target;
	idEntityPtr<idEntity>	pillarEntity;

	idScriptBool		AI_CENTURION_ARM_MISSING;
	idScriptBool		AI_CENTURION_REQUIRE_ROAR;
	idScriptBool		AI_CENTURION_ARM_TUNNEL;
	idScriptBool		AI_CENTURION_FORCEFIELD_WAIT;
	idScriptFloat		AI_CENTURION_SCRIPTED_ROAR;
	idScriptFloat		AI_CENTURION_SCRIPTED_TUNNEL;

	idList< idEntityPtr<idEntity> > obstacles;
#endif
};

#endif //__PREY_AI_CENTURION_H__