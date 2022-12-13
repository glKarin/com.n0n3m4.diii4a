
#ifndef __PREY_AI_HUNTER_SIMPLE_H__
#define __PREY_AI_HUNTER_SIMPLE_H__

#define HUNTER_N		0
#define HUNTER_NE		1
#define HUNTER_E		2
#define HUNTER_SE		3
#define HUNTER_S		4
#define HUNTER_SW		5
#define HUNTER_W		6
#define HUNTER_NW		7

class hhHunterSimple : public hhMonsterAI {

public:

	CLASS_PROTOTYPE(hhHunterSimple);

	hhHunterSimple();
	void Event_OnProjectileLaunch(hhProjectile *proj);
	void Event_OnProjectileHit(hhProjectile *proj);
	void Event_LaserOn();
	void Event_LaserOff();
	void Event_FindReaction( const char* effect );
	
	void Spawn();
	void Think();
	idProjectile *LaunchProjectile( const char *jointname, idEntity *target, bool clampToAttackCone, const idDict* desiredProjectileDef );
	void AnimMove();
	bool UpdateAnimationControllers();
	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );
	virtual void Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void PrintDebug();
	virtual void LinkScriptVariables();
	bool TurnToward( const idVec3 &pos );
	virtual	void Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	bool StartSound( const char *soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length );
	void SetNextVoiceTime( int newTime ) { nextVoiceTime = newTime; }
	void Show();
	void BlockedFailSafe();
	void UpdateEnemyPosition();
	void Distracted( idActor *newEnemy );
	bool ReachedPos( const idVec3 &pos, const moveCommand_t moveCommand ) const;

	void Event_EscapePortal();
	void Event_AssignSniperFx( hhEntityFx* fx );
	void Event_AssignMuzzleFx( hhEntityFx* fx );
	void Event_AssignFlashLightFx( hhEntityFx* fx );
	void Event_KickAngle( const idAngles &ang );
	void Event_FlashLightOn();
	void Event_FlashLightOff();
	void Event_PostSpawn();
	void Event_EnemyCanSee();
	void Event_PrintAction( const char *action );
	void Event_GetAlly();
	void Event_TriggerDelay( idEntity *ent, float delay );
	void Event_CallBackup( float delay );
	virtual bool CanSee( idEntity *ent, bool useFov );
	void Event_GetAdvanceNode();
	void Event_GetRetreatNode();
	void Event_OnProjectileLand(hhProjectile *proj);
	void Event_SaySound( const char *soundName );
	void Event_CheckRush();
	void Event_CheckRetreat();
	void Event_SayEnemyInfo();
	void Event_SetNextVoiceTime( int nextTime );
	void Event_GiveOrders( const char *orders );
	void Event_AllowOrders( bool orders );
	void Event_GetCoverNode();
	void Event_GetCoverPoint();
	void Event_GetSightNode();
	void Event_GetNearSightPoint();
	void Event_Blocked();
	void Event_EnemyPortal( idEntity *portal );
	void Event_GetEnemyPortal();
	void Event_EnemyVehicleDocked();
	void Event_EnemyIsSpirit( hhPlayer *player, hhSpiritProxy *proxy );		//HUMANHEAD jsh PCF 5/2/06 hunter combat fixes

	idEntityPtr<hhMonsterAI> ally;
	idScriptBool AI_ALLOW_ORDERS;
	idScriptBool AI_SHOTBLOCK;
protected:
	idVec3 nextMovePos;
	idEntityPtr<hhBeamSystem> beamLaser;
	idAngles kickAngles;
	idAngles kickSpeed;
	idEntityPtr<hhEntityFx> sniperFx;
	idEntityPtr<hhEntityFx> muzzleFx;
	idEntityPtr<hhEntityFx> flashLightFx;
	idList< idEntityPtr< idEntity > > nodeList;
	bool alternateAccuracy;
	bool bFlashLight;
	idScriptInt AI_DIR;
	idScriptFloat AI_LAST_DAMAGE_TIME;
	idStr currentAction;
	idStr currentSpeech;
	int endSpeechTime;
	idScriptBool AI_DIR_MOVEMENT;
	idScriptBool AI_ENEMY_RUSH;
	idScriptBool AI_ENEMY_RETREAT;
	idScriptBool AI_ENEMY_HEALTH_LOW;
	idScriptBool AI_ENEMY_RESURRECTED;
	idScriptBool AI_ONGROUND;
	idScriptBool AI_ENEMY_SHOOTABLE;
	idScriptBool AI_BLOCKED_FAILSAFE;
	idScriptInt AI_ENEMY_LAST_SEEN;
	idScriptInt AI_NEXT_DIR_TIME;
	idScriptBool AI_KNOCKBACK;
	int nextEnemyCheck;
	float lastEnemyDistance;
	int enemyRushCount;
	int enemyRetreatCount;
	int enemyHiddenCount;
	int lastChargeTime;
	int nextVoiceTime;
	idVec3 initialOrigin;
	float flashlightLength;
	int flashlightTime;
	idEntityPtr<idEntity> enemyPortal;
	idVec3 lastMoveOrigin;
	int nextBlockCheckTime;
	int nextSpiritCheck;	//HUMANHEAD jsh PCF 5/2/06 hunter combat fixes
};

#endif /* __PREY_AI_HUNTER_SIMPLE_H__ */

