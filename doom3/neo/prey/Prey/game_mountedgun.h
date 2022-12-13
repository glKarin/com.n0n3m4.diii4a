#ifndef __HH_MOUNTEDGUN_H
#define __HH_MOUNTEDGUN_H

typedef enum {
	GM_DORMANT,
	GM_IDLE,
	GM_PREATTACKING,
	GM_ATTACKING,
	GM_RELOADING,
	GM_DEAD
} gunMode_t;

/**********************************************************************

hhMountedGun

**********************************************************************/
class hhMountedGun: public hhAnimatedEntity {
	CLASS_PROTOTYPE( hhMountedGun );
	
public:
							hhMountedGun();
	virtual					~hhMountedGun();
	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Hide();
	virtual void			Show();

protected:
	void					SpawnParts();
	idVec3					DetermineTargetingLaserEndPoint();
	void					Ticker();

	virtual void			Killed( idEntity *pInflictor, idEntity *pAttacker, int iDamage, const idVec3 &Dir, int iLocation );

	int						DetermineNextFireTime();
	bool					ReadyToFire();
	void					Fire();


	void					MuzzleFlash();
	void					UpdateMuzzleFlash();
	void					AttemptToRemoveMuzzleFlash();
	void					UpdateMuzzleFlashPosition();

	void					UpdateOrientation();
	void					UpdateBoneInfo();

	bool					GetFacePosAngle( const idVec3 &sourceOrigin, float angle1, const idVec3 &targetOrigin, float &delta );
	void					SetIdealLookAngle( const idAngles& LookAngle );

	idActor					*ScanForClosestTarget();

	bool					TraceTargetVerified( const idActor* target, int traceEntNum ) const;

protected:
	bool					ClearLineOfFire();
	bool					EnemyIsVisible();
	bool					EnemyIsInPVS();

	void					PlayAnim( int iChannelNum, const char* pAnim, int iBlendTime );
	void					PlayAnim( int iChannelNum, int pAnim, int iBlendTime );
	void					PlayCycle( int iChannelNum, const char* pAnim, int iBlendTime );
	void					PlayCycle( int iChannelNum, int pAnim, int iBlendTime );
	void					ClearAnims( int iChannelNum, int iBlendTime );
	int						GetAnimDoneTime() { return animDoneTime; }

	void					NextCycleAnim( int iChannelNum, const char* pAnim );
	void					Event_NextCycleAnim( int iChannelNum, const char* pAnim );

protected:
	void					Awaken();

	void					Sleep();

	void					Event_PostSpawn();
	void					Event_Activate( idEntity* pActivator );
	void					Event_Deactivate();
	void					Event_StopRotating();

	void					Event_ScanForClosestTarget();
	void					Event_StartAttack();

protected:
	// CJR STUFF
	int						team;
	gunMode_t				gunMode;
	void					Think();
	void					TurnTowardsEnemy( float maxYawVelocity, float yawAccel );

	bool					EnemyCanBeAttacked();
	bool					EnemyIsInRange();
	void					PreAttack();
	void					Attack();
	void					Reload();

protected:
	hhBoneController		pitchController;

	const idDict*			projectileDict;

	idEntityPtr<hhTrackMover>	trackMover;

	idEntityPtr<hhBeamSystem>	targetingLaser;

	struct jointInfo_t {
		idStr			name;
		jointHandle_t	handle;
		idVec3			origin;
		idMat3			axis;
						
						jointInfo_t() { Clear(); }
		void			Clear() { handle = INVALID_JOINT; origin.Zero(); axis = mat3_identity; }
		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );
	};

	jointInfo_t				boneHub;
	jointInfo_t				boneGun;
	jointInfo_t				boneExhaust;
	jointInfo_t				boneOrigin;

	idEntityPtr<idActor>	enemy;
	float					enemyRange;

	float					firingRange;

	int						nextFireTime;
	float					burstRateOfFire;
	int						shotsPerBurst;
	int						shotsPerBurstCounter;

	renderLight_t			muzzleFlash;
	int						muzzleFlashHandle;
	int						muzzleFlashEnd;
	int						muzzleFlashDuration;

	idAngles				idealLookAngle;
	idAngles				prevIdealLookAngle;

	idPhysics_Parametric	physicsObj;

	int						animDoneTime;

	int						PVSArea;
	
	// nla - Other vars 
	int						channelBody;
	int						channelBarrel;
	int						deathwalkDelay;		//time to wait in msecs after killing something

	// cjr vars
	float					yawVelocity;
};

#endif
