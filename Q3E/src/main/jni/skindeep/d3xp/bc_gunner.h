#pragma once


//#include "Player.h"
#include "ai/AI.h"



class idGunnerMonster : public idAI
{
public:
	CLASS_PROTOTYPE(idGunnerMonster);

							idGunnerMonster();
	
	void					Spawn(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	
	virtual void			Resurrect();

	virtual void			Event_PostSpawn(void);
    

	idVec3					FindValidPosition(idVec3 targetPos);

	virtual void			StartStunState(const char *damageDefName);
	virtual void			SetJockeyState(bool value);

	virtual void			DoJockeySlamDamage();
	virtual void			DoJockeyBrutalSlam();
	virtual void			DoWorldDamage();

	int						GetSuspicionCounter();
	int						GetSuspicionMaxValue();
	bool					GetSightedFlashTime();


	bool					StartDodge(idVec3 _trajectoryPosition);
	bool					drawDodgeUI;


	
	idMat3					GetJockeySmashAxis();
	trace_t					GetJockeySmashTrace(); //The surface that the AI will be jockey-slammed into.
	idEntityPtr<idEntity>	jockeyKillEntity; //The context sensitive entity that the AI can be killed with.
	int						jockeyAttackCurrentlyAvailable; //If player presses LMB, what kind of attack will happen. Refer to enum JOCKATKTYPE_
	bool					IsJockeyBeingAttacked();
	bool					IsJockeyBeingSlammed();
	bool					IsObjectKillEntity(idEntity *ent);

	virtual bool			CanAcceptStimulus();

	void					ResetCallresponseTimer();

	void					DoPickpocketReaction(idVec3 investigatePosition);

	//BC PUBLIC END

protected:
    virtual void			Think(void);
    virtual void			State_Idle();
    virtual void			State_Combat();

private:

	

	enum
	{
		SEARCHMODE_SEEKINGPOSITION,
		SEARCHMODE_OBSERVING,	//aim my viewangle to look directly at specific interestpoint.
		SEARCHMODE_PANNING,		//reset orientation and look straight ahead and pan around for a bit - a general look around the room.
	};



	//Variables.
	int						suspicionIntervalTimer;
	int						suspicionCounter;
	int						suspicionStartTime;
	idVec3					lastFireablePosition;
	int						combatFiringTimer;
	bool					combatStalkInitialized;
	int						stateTimer;
	int						intervalTimer;
	idEntityPtr<idEntity>	lastInterest;
	int						lastInterestPriority;
	int						lastInterestSwitchTime;
	bool					ignoreInterestPoints; // SW: for vignettes/scripting
	idBeam*					interestBeam;
	idBeam*					interestBeamTarget;
	void					UpdateInterestBeam( idEntity *interestpoint );
	int						searchMode;	
	int						searchWaitTimer;
	int						specialTraverseTimer;
	int						flailDamageTimer;
	int						searchStartTime;


	//Functions.
	void					State_Suspicious();	
	void					State_CombatStalk();
	void					State_CombatObserve();
	void					State_Searching();
	void					State_Overwatch();
	void					State_Spaceflail();
	void					State_Victory();
	void					State_Stunned();
	void					State_Jockeyed();
	virtual void			GotoState(int _state);

	virtual bool			InterestPointCheck(idEntity *interestpoint);
	virtual void			InterestPointReact(idEntity *interestpoint, int roletype);
	virtual void			ClearInterestPoint();
	virtual idEntityPtr<idEntity> GetLastInterest();
	virtual void			SetAlertedState(bool investigateLKP);
	void					UpdateLKP(idEntity *enemyEnt);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	void					OnArriveAtLKP();

	int						lastState;

	int						fidgetTimer;

	void					UpdateGrenade();
	void					UpdatePet();
	idVec3					FindPetSpawnPos();

	bool					firstSearchnodeHint;

	idEntity *				GetBiasedSearchNode();

	idLight *				headLight;

    idVec3                  GetEyePositionIfPossible(idEntity *ent);

	bool					TargetIsInDarkness(idEntity *ent);


	//customidle stuff
	idEntityPtr<idEntity>	lastIdletaskEnt;
	int						customidleResetTimer;
	bool					waitingforCustomidleReset;
	int						idletaskStartTime;
	bool					idletaskFrobHappened;



	//Energyshield
	bool					energyshieldModuleAvailable;
	void					UpdateEnergyshieldActivationCheck();
	void					SetEnergyshieldActive(bool value);
	virtual void			OnShieldDestroyed();
	

	//Searchnode anim.
	idEntityPtr<idEntity>	currentSearchNode;
    idStr					lastIdlenodeAnim; //last idle anim used. So that we don't repeat anims twice in a row.
    idStr                   GetParsedAnim(idEntity *searchnode);
	idStr                   GetPathcornerParsedAnim(idEntity *searchnode);
	int						idlenodeStartTime;
	bool					idlenodePositionSnapped;
	idVec3					idlenodeSnapPosition;
	idVec3					idlenodeOriginalPosition;
	idMat3					idlenodeSnapAxis;


	//Jockey behavior.
	int						jockeyMoveTimer;
	int						jockeyBehavior;
	int						jockeyTurnTimer;
	enum					{JB_BACKWARD, JB_FORWARD, JB_STRAFELEFT, JB_STRAFERIGHT };
	int						jockeyDamageTimer;
	bool					DoJockeyNearbyDamage(trace_t damageTr, idVec3 impulseDir);
	void					StartJockeySlam();
    int                     jockeyWorldfrobTimer;
    void                    DoJockeyWorldfrob();
	void					DoJockeyWorldDamage();
	void					DoJockeyHazardpipeDamage(idEntity *frobEnt);

	enum					{ JOCKATK_NONE, JOCKATK_SLAMATTACK, JOCKATK_KILLENTITYATTACK };
	int						jockStateTimer;
	int						jockattackState;
	idVec3					FindJockeyShootTarget();
	trace_t					FindJockeyClosestSurface();
	int						jocksurfacecheckTimer;
	void					UpdateJockeyAttackAvailability();
	idEntity *				FindJockeyKillEntity();
	trace_t					jockeySmashTr; //trace of the surface that the AI will be jockey-slammed into.
	int						lastJockeyBehavior;

	

	//Airlock purge behavior.
	int						airlockLockdownCheckTimer;
	void					DoAirlockLockdownCheck();

	
	
	bool					hasAlertedFriends;

	int						sighted_flashTimer; //when I sight the player, we want to draw a ui element that flashes. This is the timer for that.

	void					AddEventlog_Interestpoint(idEntity *interestpoint);


	//idle sounds.
	int						idleSoundTimer;

	//Flashlight
	void					SetFlashlight(bool value);

	bool					radiocheckinPrimed;
	int						radiocheckinTimer;


	bool					ShouldExitOverwatchState();

	int						meleeAttackTimer; //melee kick cooldown timer.
	int						meleeChargeTimer; //we don't want the melee to happen immediately, we want a short delay before it happens.
	int						meleeModeActive; //are we getting ready to do a melee attack

	bool					hasPathNodes;
	idEntityPtr<idEntity>	currentPathTarget; //what is my currently active path node target.
	void					HasArriveAtPathNode();
	void					GotoNextPathNode();

	void					PrintInterestpointEvent(idEntity *interestpoint, idStr rolename);

	void					FindPositionalIdlePathcorner();


	//Idle call and response.
	enum					{ICR_NONE, ICR_CALLER, ICR_REPONDER};
	int						icr_state;
	int						icr_timer;
	idEntityPtr<idEntity>	icr_responderEnt;
	idEntity *				IsNearSomeone();


	//Pickpocket reaction.
	enum					{PPR_NONE, PPR_DELAY};
	int						pickpocketReactionState;
	int						pickpocketReactionTimer;
	idVec3					pickpocketReactionPosition;

	virtual void			Event_SetPathEntity(idEntity* pathEnt);

	bool					IsTargetLookingAtMe(idEntity* enemyEnt);


	int						softfailCooldown;
	idVec3					lastSoftfailPosition;

	void					UpdatePickpocketReaction();

	int						highlySuspiciousTimer;

	void					Eventlog_StartCombatAlert();

	void					DoZeroG_Unvacuumable_Check();
	bool					CanDoUnvacuumableCheck;

	//BC private end
};