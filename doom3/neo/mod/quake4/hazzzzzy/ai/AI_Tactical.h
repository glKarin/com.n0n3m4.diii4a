/*
================

AI_Tactical.h

================
*/

#ifndef __AI_TACTICAL__
#define __AI_TACTICAL__

typedef enum {
	AIPOSTURE_DEFAULT = -1,
	AIPOSTURE_STAND,
	AIPOSTURE_CROUCH,
	AIPOSTURE_STAND_COVER_LEFT,
	AIPOSTURE_STAND_COVER_RIGHT,
	AIPOSTURE_CROUCH_COVER,
	AIPOSTURE_CROUCH_COVER_LEFT,
	AIPOSTURE_CROUCH_COVER_RIGHT,
	AIPOSTURE_RELAXED,
	AIPOSTURE_UNARMED,
	AIPOSTURE_AT_ATTENTION,
	AIPOSTURE_MAX
} aiPosture_t;

typedef struct {
	struct {
		bool	canMove;
		bool	canShoot;
		bool	canPeek;
		bool	canReload;
		bool	canTurn;
		bool	canKillswitch;
	} fl;

} aiPostureInfo_t;

class rvAITactical : public idAI {
public:

	CLASS_PROTOTYPE( rvAITactical );

	rvAITactical ( void );

	void				InitSpawnArgsVariables			( void );
	void				Spawn							( void );
	void				Think							( void );
	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );

	virtual bool		CheckActions					( void );

	virtual void		GetDebugInfo					( debugInfoProc_t proc, void* userData );

	virtual bool		CanTurn							( void ) const;
	virtual bool		CanMove							( void ) const;

	virtual bool		IsCrouching						( void ) const;

protected:

	virtual const char*	GetIdleAnimName					( void );
	virtual void		OnStopMoving					( aiMoveCommand_t oldMoveCommand );
	virtual void		OnPostureChange					( void );
	virtual void		OnSetKey						( const char* key, const char* value );

	bool				CheckRelaxed					( void ) const;

	void				InitPostureInfo					( void );
	bool				UpdatePosture					( void );
	void				CalculateShots					( const char* fireAnim );
	void				UseAmmo							( int amount );
	void				SetPosture						( aiPosture_t newPosture );
	void				UpdateAnimPrefix				( void );

	int					ammo;
	int					maxShots;	
	int					minShots;
	int					shots;
	float				fireRate;
	
	int					playerFocusTime;
	int					playerAnnoyTime;

	aiPosture_t			postureIdeal;
	aiPosture_t			postureCurrent;
	aiPosture_t			postureForce;
	aiPostureInfo_t		postureInfo[AIPOSTURE_MAX];

	rvAIAction			actionElbowAttack;
	rvAIAction			actionKillswitchAttack;

	rvAIActionTimer		actionTimerPeek;

private:

	int					healthRegen;
	bool				healthRegenEnabled;
	int					healthRegenNextTime;
	int					maxHealth;
	
	int					nextWallTraceTime;

	// Custom actions
	bool				CheckAction_Reload					( rvAIAction* action, int animNum );
	bool				CheckAction_Relax					( rvAIAction* action, int animNum );

	// Torso States
	stateResult_t		State_Torso_SetPosture				( const stateParms_t& parms );
	
	stateResult_t		State_Torso_RangedAttack			( const stateParms_t& parms );
	stateResult_t		State_Torso_MovingRangedAttack		( const stateParms_t& parms );
	stateResult_t		State_Torso_Reload					( const stateParms_t& parms );

	stateResult_t		State_Torso_Cover_LeanLeftAttack	( const stateParms_t& parms );
	stateResult_t		State_Torso_Cover_LeanRightAttack	( const stateParms_t& parms );	
	stateResult_t		State_Torso_Cover_LeanAttack		( const stateParms_t& parms );	
	stateResult_t		State_Torso_Cover_Peek				( const stateParms_t& parms );

	// Frame Commands
	stateResult_t		State_Frame_Peek					( const stateParms_t& parms );

	// Events
	void				Event_ForcePosture					( int posture );

	virtual void		Event_PostSpawn						( void );

	CLASS_STATES_PROTOTYPE ( rvAITactical );
};

extern const idEventDef AI_ForcePosture;

#endif /* !__AI_TACTICAL__ */
