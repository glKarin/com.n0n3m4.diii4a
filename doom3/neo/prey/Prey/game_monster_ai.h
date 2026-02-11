
#ifndef __PREY_MONSTER_AI_H__
#define __PREY_MONSTER_AI_H__

extern const idEventDef MA_AttackMissileEx;
extern const idEventDef MA_FindReaction;
extern const idEventDef MA_InitialWallwalk;
extern const idEventDef MA_EnemyOnSpawn;
extern const idEventDef MA_SetVehicleState;
extern const idEventDef MA_OnProjectileLaunch;
extern const idEventDef MA_OnProjectileHit;
extern const idEventDef MA_FallNow;
extern const idEventDef MA_EnemyIsSpirit;
extern const idEventDef MA_EnemyIsPhysical;
extern const idEventDef MA_EnemyPortal;

//
// hhNoClipEnt
//
// Dummy entity that sets contents to 0 - created for use for ForceAAS entities
//
class hhNoClipEnt : public idEntity {
public:
	CLASS_PROTOTYPE(hhNoClipEnt);

	void				Spawn( void );
};

class hhAINode : public idEntity {
public:
	CLASS_PROTOTYPE( hhAINode );
						hhAINode();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	idEntityPtr< idEntity>	user;
};

//! Somewhat of a hack.  This should in an a .h file someplace.
//   Not in a .cpp file as id has it now.
//#define CONDITION( name ) name.PointTo( scriptObject, #name )

struct hhMonsterReaction {
	hhMonsterReaction(void) : reactionIndex(-1) { }
	inline hhReaction *GetReaction() const { return ( entity.IsValid() && reactionIndex >= 0 ? entity->GetReaction(reactionIndex) : NULL ); }
	int reactionIndex;
	idEntityPtr<idEntity> entity;
};

//
// hhMonsterHealthTrigger
//
struct hhMonsterHealthTrigger {
	hhMonsterHealthTrigger()	{healthThresh = 0; triggerEnt = NULL; triggerCount = 0;}

	int						healthThresh;		// When our health drops below this value, trigger our ent
	idEntityPtr<idEntity>	triggerEnt;			// The entity to trigger
	int						triggerCount;		// Number of times we have tripped this trigger
};

class hhMonsterAI : public idAI { 

public:			
	CLASS_PROTOTYPE(hhMonsterAI);

	hhMonsterAI();
	~hhMonsterAI();
	virtual void	Think();
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	const char *	GetJointForFrameCommand( const char *cmd ) { idStr tmp( cmd ); int i = tmp.Find( " " );  if ( i < 0 ) { return( va( "%s", cmd ) ); } else { return( va( "%s", tmp.Left( i ).c_str() ) ); } };
	void			Spawn();
	virtual void	LinkScriptVariables(void);
	bool			HasPathTo(const idVec3 &destPt);
	void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
	virtual void	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void	EnterVehicle( hhVehicle* vehicle );
	virtual bool	TestMelee( void ) const;
	virtual bool	MoveToPosition( const idVec3 &pos, bool enemyBlocks = false );
	virtual bool	TurnToward( const idVec3 &pos );
	ID_INLINE virtual bool TurnToward( float yaw ) { return idAI::TurnToward( yaw ); } // HUMANHEAD mdl:  Needed because of bizarre inheritance issue that resulted in TurnToward(idVec3) being called 
	virtual bool	CanSee( idEntity *ent, bool useFov );
	idPlayer*		GetClosestPlayer( void );
	void			Show();
	bool			GetFacePosAngle( const idVec3 &pos, float &delta );
	virtual void	SetEnemy( idActor *newEnemy );			//made public because hhAI does
	void			Pickup( hhPlayer *player );
	bool			GiveToPlayer( hhPlayer* player );
	idVec3			GetTouchPos(idEntity *ent, const hhReactionDesc *desc );
	void			UpdateFromPhysics( bool moveBack );
	virtual bool	GetTouchPosBound( const hhReactionDesc *desc, idBounds& bounds );
   	virtual	void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	void			CreateHealthTriggers();
	void			UpdateHealthTriggers(int oldHealth, int currHealth);
	int				ReactionTo( const idEntity *ent );
	virtual bool	UpdateAnimationControllers( void );
	virtual bool	ShouldTouchTrigger( idEntity* entity ) const { return fl.touchTriggers; }
	virtual void	FlyTurn( void );
	bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
	void			UpdateEnemyPosition( void );
	void			BecameBound(hhBindController *b);
	void			BecameUnbound(hhBindController *b);
	void			UpdateModelTransform();
	virtual // HUMANHEAD CJR
	void			CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity );
	hhEntityFx*		SpawnFxLocal( const char *fxName, const idVec3 &origin, const idMat3& axis = mat3_identity, const hhFxInfo* const fxInfo = NULL, bool forceClient = false );
	bool			AttackMelee( const char *meleeDefName );
	bool			TestMeleeDef( const char *meleeDefName ) const;
	virtual void	PrintDebug();
	int				GetTurnDir();
	bool			FacingEnemy( float range );
	virtual void	Activate( idEntity *activator );
	void			BecomeActive( int flags );
	bool			OverrideKilledByGravityZones()	{ return bOverrideKilledByGravityZones;	}
	virtual void	Hide();
	virtual void	HideNoDormant();
	void			AddLocalMatterWound( jointHandle_t jointNum, const idVec3 &localOrigin, const idVec3 &localNormal, const idVec3 &localDir, int damageDefIndex, const idMaterial *collisionMaterial );
	ID_INLINE virtual bool IsDead() { return AI_DEAD != 0; }
	bool			Pushes() { return( pushes && move.moveCommand != MOVE_NONE ); }		// only push if moving
	void			GravClipModelAxis( bool enable ) { physicsObj.SetGravClipModelAxis( enable ); }
	virtual void	Distracted( idActor *newEnemy );

// Events.
	virtual void	Event_PostSpawn();
	void			Event_AttackMissileEx(const char *params, int boneDir);	
	virtual void	Event_AttackMissile( const char *jointname, const idDict *projDef, int boneDir );
	void			Event_SetMeleeRange( float newMeleeRange );
	virtual void	Event_FindReaction( const char* effect );
	virtual void	Event_UseReaction();
	void			Event_EnemyOnSide();
	void			Event_HitCheck( idEntity *ent, const char *animname );
	void			Event_CreateMonsterPortal();
	void			Event_GetShootTarget();
	void			Event_TriggerReactEnt();
	virtual void	Event_Remove( void );
	void			Event_OnProjectileLaunch(hhProjectile *proj);
	void			Event_InitialWallwalk( void );
	void			Event_GetVehicle();
	void			Event_EnemyAimingAtMe();
	void			Event_ReachedEntity( idEntity *ent );
	void			Event_EnemyOnSpawn();
	void			Event_SpawnFX( char *fxFile );
	void			Event_SplashDamage(char *damage);
	void			Event_SetVehicleState();
	void			Event_TestAnimMoveTowardEnemy( const char *animname );
	void			Event_GetLastReachableEnemyPos();
	void			Event_FollowPath( const char *pathName );
	void			Event_EnemyIsA( const char* testclass );
	void			Event_Subtitle( idList<idStr>* parmList );
	void			Event_SubtitleOff();
	void			Event_EnableHeadlook();
	void			Event_DisableHeadlook();
	void			Event_EnableEyelook();
	void			Event_DisableEyelook();
	void			Event_FacingEnemy(float range);
	void			Event_BossBar( int onOff );
	virtual void	Event_GetCombatNode();
	void			Event_FallNow();
	void			Event_AllowFall( int allowFall );
	virtual void	Event_EnemyIsSpirit( hhPlayer *player, hhSpiritProxy *proxy );
	virtual void	Event_EnemyIsPhysical( hhPlayer *player, hhSpiritProxy *proxy );
	void			Event_InPlayerFov();
	void			Event_IsRagdoll();
	void			Event_MoveDone();
	void			Event_SetShootTarget(idEntity *ent);
	void			Event_EnemyInGravityZone(void);
	void			Event_LookAtEntity( idEntity *ent, float duration );
	void			Event_LookAtEnemy( float duration );
	void			Event_SetLookOffset( idAngles const &ang );
	void			Event_SetHeadFocusRate( float rate );
	void			Event_HeardSound( int ignore_team );
	virtual void	Event_FlyZip();
	void			Event_UseConsole( idEntity *ent );
	void			Event_TestMeleeDef( const char *meleeDefName ) const;
	void			Event_EnemyInVehicle();
	void			Event_EnemyOnWallwalk();
	void			Event_AlertAI( idEntity *ent, float radius );
	void			Event_TestAnimMoveBlocked( const char *animname );
	void			Event_InGravityZone();
	void			Event_StartSoundDelay( const char *soundName, int channel, int netSync, float delay );
	void			Event_SetTeam( int new_team );
	virtual void	Event_GetAttackPoint();
	void			Event_HideNoDormant();
	void			Event_SoundOnModel();
	void			Event_ActivatePhysics();
	void			Event_IsVehicleDocked();
	void			Event_SetNeverDormant( int enable );
	void			Event_EnemyInSpirit();
	void			Event_EnemyPortal( idEntity *ent );

	// CJR DDA TEST
	int				numDDADamageSamples;
	float			totalDDADamage;

	void			DamagedPlayer( int damage ) { numDDADamageSamples++; totalDDADamage += damage; }
	void			SendDamageToDDA();
	// END CJR DDA TEST

	//passageway code
	bool				IsInsidePassageway(void)	const		{return currPassageway != NULL;}
	hhAIPassageway*		GetCurrPassageway(void)					{return currPassageway.GetEntity();}
public:
		idScriptBool	AI_HAS_RANGE_ATTACK;		// TRUE if this monster has a range attack (required for reaction system)
		idScriptBool	AI_HAS_MELEE_ATTACK;		// TRUE if this monster has a melee attack (required for reaction system)
		idScriptBool	AI_USING_REACTION;			// TRUE if monster is currently using reaction
		idScriptBool	AI_REACTION_FAILED;			// TRUE if the last reaction attempt failed (path blocked, exclusive problem, etc)
		idScriptBool	AI_REACTION_ANIM;			// TRUE if the current reaction is using an anim to 'cause'
		idScriptBool	AI_BACKWARD;
		idScriptBool	AI_STRAFE_LEFT;
		idScriptBool	AI_STRAFE_RIGHT;
		idScriptBool	AI_UPWARD;
		idScriptBool	AI_DOWNWARD;
		idScriptBool	AI_SHUTTLE_DOCKED;
		idScriptBool	AI_VEHICLE_ATTACK;
		idScriptBool	AI_VEHICLE_ALT_ATTACK;
		idScriptBool	AI_WALLWALK;
		idScriptBool	AI_FALLING;
		idScriptBool	AI_PATHING;
		idScriptFloat	AI_TURN_DIR;
		idScriptBool	AI_FLY_NO_SEEK;
		idScriptBool	AI_FOLLOWING_PATH;

	bool					bNeverTarget;
	static				idList<idAI*> allSimpleMonsters;	// Global list of all hhMonsterAI's currently created
protected:
	bool			NearEnoughTouchPos( idEntity* ent, const idVec3& targetPos, idBounds& bounds );
	bool			CheckValidReaction();
	void			FinishReaction( bool bFailed = false );
	virtual int 	EvaluateReaction( const hhReaction *react );
	bool			NewWanderDir( const idVec3 &dest );
	void			FlyMove();
	virtual void	HandleNoGore();
	bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );
protected:
	hhMonsterReaction		targetReaction;			// Info about our current reaction
	idEntityPtr<idEntity>	shootTarget;
	idEntityPtr<hhAIPassageway> currPassageway;					// The passage node this monster is currently in
	idList<hhMonsterHealthTrigger> healthTriggers;
    int						lastContactTime;
	int						hearingRange;
	int						nextSpeechTime;
	idAngles				lookOffset;
	int						nextTurnUpdate;
	idVec3					spawnOrigin;					//location of where this monster spawned
	int						spawnThinkFlags;
	int						fallDelay;
	bool					bCanFall;
	bool					bSeeThroughPortals;
	bool					bBossBar;
	bool					bBindOrient;					//orient monster to its bindmaster
	bool					bCanWallwalk;
	bool					bOverrideKilledByGravityZones;
	bool					soundOnModel;
	bool					bBindAxis;
	bool					bCustomBlood;
	bool					bNoCombat;
	int						nextSpiritProxyCheck;
};


#endif /* __PREY_MONSTER_AI_H__ */


