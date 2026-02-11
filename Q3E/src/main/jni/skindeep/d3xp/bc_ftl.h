#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idFTL : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idFTL);

							idFTL(void);
	virtual					~idFTL(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual void			DoRepairTick(int amount);

	bool					IsJumpActive(bool includesCountdownAndCountdown, bool includesCountdown = false);

	int						GetPublicTimer();
	int						GetPublicPauseTimer();
	int						normalizedHealth;

	void					SetPipePauseState();
	bool					GetPipePauseState();

	void					StartCountdown();

    void                    FastforwardCountdown();

	bool					AddToCountdown(int millisec);

	void					StartShutdown();

	int						GetTotalCountdowntime();

private:

	virtual void			Event_PostSpawn(void);

	void					DoCloseSequence();

	idAnimated*				sheatheEnt = nullptr;
	idLight *				rodLight = nullptr;

	int						ftlTimer;
	int						ftlState;
							//IDLE = nothing happening. COUNTDOWN = counting down; this is the one we'll be in most of the time. CHARGING = the little chargeup bit before the sheathe opens. ACTIVE = sheathe is open/opening. CLOSING = sheathe is closing. DAMAGED = is offline.
	enum					{ FTL_IDLE, FTL_DISPATCHDELAY, FTL_DISPATCH, FTL_COUNTDOWN, FTL_OPENING, FTL_ACTIVE, FTL_CLOSEDELAY, FTL_CLOSING, FTL_DAMAGED, FTL_PIPEPAUSE, FTL_DORMANT };
	int						ftlPauseTime;

	
	

	idFuncEmitter			*smokeEmitters[3] = {};

    idFuncEmitter			*seamEmitter = nullptr;

	void					UpdateHealthGUI();

	bool					initializedHealth; //Call this on first frame so the health gui gets updated.
	int						lastHealth;



	idEntityPtr<idEntity>	healthGUI;

	
	void					StartIdle();
	void					StartActiveState();

	enum					{ VO_240SEC, VO_210SEC, VO_180SEC, VO_150SEC, VO_120SEC, VO_90SEC, VO_60SEC, VO_45SEC, VO_30SEC, VO_20SEC, VO_10SEC, VO_9SEC, VO_8SEC, VO_7SEC, VO_6SEC, VO_5SEC, VO_4SEC, VO_3SEC, VO_2SEC, VO_1SEC, VO_DORMANT };
	int						voiceoverIndex;

	int						pipepauseTimer;

	bool					openAnimationStarted;

	int						outsideDamageTimer;

	int						additionalCountdownTime;

	int						ReinitializeVOIndex(int timeDelta);

	int						defaultJumpTime;

	void					Event_SetState(int value);
	void					Event_GetState();


};
//#pragma once