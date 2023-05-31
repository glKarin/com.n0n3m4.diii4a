
#ifndef __PREY_AI_SPHEREBOSS_H__
#define __PREY_AI_SPHEREBOSS_H__

class hhSphereBoss : public hhMonsterAI {

public:
	CLASS_PROTOTYPE(hhSphereBoss);
#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	void Event_UpdateTarget() {};
	void Event_GetCombatNode() {};
	void Event_GetCircleNode() {};
	void Event_DirectMoveToPosition(const idVec3 &pos) {};
	void Event_SpinClouds( float shouldSpin ) {};
	void Event_SetSeekScale( float new_scale ) {};
#else
	void Spawn();
	void FlyTurn();
	void Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	idProjectile *LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, const idDict* desiredProjectileDef );
	void AdjustFlySpeed( idVec3 &vel );
	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );
	void FlyMove( void );
	bool ReachedPos( const idVec3 &pos, const moveCommand_t moveCommand ) const;
	void LinkScriptVariables();
	void Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	void AddLocalMatterWound( jointHandle_t jointNum, const idVec3 &localOrigin, const idVec3 &localNormal, const idVec3 &localDir, int damageDefIndex, const idMaterial *collisionMaterial );

	void Event_UpdateTarget();
	void Event_GetCombatNode();
	void Event_GetCircleNode();
	void Event_DirectMoveToPosition(const idVec3 &pos);
	void Event_SpinClouds( float shouldSpin );
	void Event_SetSeekScale( float new_scale );
protected:
	idScriptBool AI_FACE_ENEMY;
	idScriptBool AI_CAN_DAMAGE;
	idList<idVec3> lastTargetPos;
	int lastNodeIndex;
	int nextShieldImpact;
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
};

#endif