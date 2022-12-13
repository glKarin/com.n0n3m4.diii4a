
#ifndef __GAME_WRAITH_H__
#define __GAME_WRAITH_H__

typedef enum wraithState_s {
	WS_SPAWN = 0,
	WS_FLY,
	WS_FLEE,
	WS_POSSESS_CHARGE,
	WS_DEATH_CHARGE,
	WS_STILL
} wraithState_t;

class hhWraith : public hhMonsterAI {
	public:
		CLASS_PROTOTYPE(hhWraith);

		void			Spawn(void);
		virtual			~hhWraith();
		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

		void			UpdateEnemyPosition( void );
		virtual void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
		virtual void	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
		virtual void	Think( void );

		virtual	int		HasAmmo( ammo_t type, int amount ) { return 0; }
		virtual	bool	UseAmmo( ammo_t type, int amount ) { return false ; }

	protected:
		virtual void	EnemyDead();

		// State flight functions
		virtual void	FlyUp( void );
		virtual void	FlyMove( void );
		virtual void	FlyToEnemy( void );
		virtual void	FlyAway( void );

		void			CheckFleeRemove( void );
		void			WraithPossess( idActor *actor );
		void			CheckCollisions( void );

		void			PlayAnimMove( int anim, int blendTime );
		void			PlayAnimMoveEnd();
		void			TurnTowardEnemy();

		void			Event_FindEnemy( int useFOV );
		void			Event_TurnTowardEnemy();
		void			Event_Flee();
		void			Event_PlayAnimMoveEnd( );
		virtual void	Event_Activate(idEntity *activator);

		virtual void	Event_EnemyIsSpirit( hhPlayer *player, hhSpiritProxy *proxy );
		virtual void	Event_EnemyIsPhysical( hhPlayer *player, hhSpiritProxy *proxy );

		virtual void	TeleportIn( idEntity *activator );
		virtual void	StartDisposeCountdown() { } // Doesn't apply to wraiths

		virtual void	Portalled(idEntity *portal);

	protected:
		// Animations
		int			flyAnim;
		int			possessAnim;
		int			leftAnim;
		int			rightAnim;
		int			fleeAnim;
		int			fleeInAnim;

		int			lastAnim; // Used in turning flight logic

		// Variables
		bool			canPossess;

		idVec3			velocity;

		int				damageTicks; // # of ticks to act damaged (fly slower, flash red)
		int				straightTicks; // # of ticks to fly straight
		int				turnTicks; // # if ticks when turning before the wraith will be forced to fly straight

		int				nextCheckTime;
		idVec3			lastCheckOrigin;
		float			lastDamageTime;		// used to delay the damage check after applying damage

		wraithState_t	state;

		// .def file variables
		float			velocity_xy;
		float			velocity_z;
		float			velocity_z_fast;
		float			dist_z_close;
		float			dist_z_far;
		float			turn_threshold;
		float			turn_radius_max;
		int				straight_ticks; // Number of ticks to delay before turning
		int				damage_ticks;
		int				turn_ticks; // Max time to spend turning before going straight
		float			flee_speed_z;
		float			target_z_threshold;

		float			minDamageDist; // Damage distance check

		// Scale variables
		bool			isScaling;
		float			scaleStart;
		float			scaleEnd;
		float			scaleTime;
		float			lastScaleTime;

		int				countDownTimer;
		idMat3			desiredAxis;
		float			desiredVelocity;

		bool			bFaceEnemy;
		int				nextDrop;
		int				nextChatter;
		int				minChatter;
		int				maxChatter;

		idEntityPtr<idActor> lastActor;

		int				nextPossessTime;

		idEntityPtr<idEntityFx> fxFly;
};

#endif /* __GAME_WRAITH_H__ */
