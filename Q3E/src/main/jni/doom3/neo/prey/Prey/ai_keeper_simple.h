
#ifndef __PREY_AI_KEEPER_SIMPLE_H__
#define __PREY_AI_KEEPER_SIMPLE_H__

class hhKeeperSimple : public hhMonsterAI {

public:	
	CLASS_PROTOTYPE(hhKeeperSimple);

#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	void				Event_GetTriggerEntity() {};
	void				Event_TriggerEntity( idEntity *ent ) {};
	void				Event_GetThrowEntity() {};
	void				Event_ThrowEntity( idEntity *ent ) {};
	void				Event_StartBlast( idEntity *ent ) {};
	void				Event_EndBlast() {};
	void				Event_BeginTelepathicThrow(idEntity *throwMe) {};
	void				Event_UpdateTelepathicThrow() {};
	void				Event_TelepathicThrow() {};	
	void				Event_StartTeleport() {};
	void				Event_EndTeleport() {};
	void				Event_TeleportExit() {};
	void				Event_TeleportEnter() {};
	void				Event_CreatePortal() {};
	void				Event_EnableShield() {};
	void				Event_DisableShield() {};
	void				Event_ForceDisableShield() {};
	void				Event_AssignShieldFx( hhEntityFx* fx ) {};
	void				Event_StartHeadFx() {};
	void				Event_EndHeadFx() {};
	void				Event_AssignHeadFx( hhEntityFx* fx ) {};
	void				Event_KeeperTrigger() {};
#else


	hhKeeperSimple();
	~hhKeeperSimple();
	void				Spawn( void );
	virtual void		Think();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	void				LinkScriptVariables();
	virtual void		Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void				Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	idProjectile*		LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, const idDict* desiredProjectileDef );
	void				Hide();
	void				Show();
	void				HideNoDormant();
	idScriptBool		AI_LANDED;
	idScriptBool		AI_SHIELD;
protected:
	void				Event_GetTriggerEntity();
	void				Event_TriggerEntity( idEntity *ent );
	void				Event_GetThrowEntity();
	void				Event_ThrowEntity( idEntity *ent );

	// Attack blast
	void				Event_StartBlast( idEntity *ent );
	void				Event_EndBlast();

	// Telepathic Throwing
	void				Event_BeginTelepathicThrow(idEntity *throwMe);
	void				Event_UpdateTelepathicThrow();
	void				Event_TelepathicThrow();	

	// Teleporting
	void				Event_StartTeleport();
	void				Event_EndTeleport();
	void				Event_TeleportExit();
	void				Event_TeleportEnter();
	void				Event_CreatePortal();

	// Shield
	void				Event_EnableShield();
	void				Event_DisableShield();
	void				Event_ForceDisableShield();
	void				Event_AssignShieldFx( hhEntityFx* fx );
	void				Event_StartHeadFx();
	void				Event_EndHeadFx();
	void				Event_AssignHeadFx( hhEntityFx* fx );

	// Telepathic Triggering
	void				Event_KeeperTrigger();

	float				GetTeleportDestRating(const idVec3 &pos);
	idEntityPtr<hhBeamSystem>	beamTelepathic;		// Beam used when we are telepathically controlling an object
	idEntityPtr<hhBeamSystem>	beamAttack;
	idEntityPtr<hhEntityFx>		shieldFx;
	idEntityPtr<hhEntityFx>		headFx;
	idEntityPtr<idEntity>		triggerEntity;
	idEntityPtr<idEntity>		throwEntity;
	int					nextShieldImpact;
	float				shieldAlpha;
	bool				bThrowing;
#endif
};

#endif 