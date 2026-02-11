#include "Misc.h"
#include "Entity.h"



class idStressTester : public idEntity
{
public:
	CLASS_PROTOTYPE(idStressTester);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	virtual void			Think(void);
	

private:

	enum					{ CRYOPOD_DELAY, POSTCRYO_DELAY, PUSHLEDGE, WANDER };
	int						state;
	int						timer;


	void					ExitCryopod();

	int						pushledgeCount;

	int						doorTimer;
	void					FrobDoorAttempt();

	int						jockeyTimer;
	void					AttemptJockey();
	idEntity*				FindVisibleEnemy();
	int						jockeySlamTimer;
	void					AttemptJockeySlam();
	idEntityPtr<idEntity>	meleeTarget;

	int						globalRestartTimer;

	int						globalUnlockFuseboxTimer;
	bool					hasUnlockedGlobalFusebox;
	void					UnlockAllFusebox();

	int						GetUpTimer;
	void					AttemptGetUp();

	void					UpdateStuckLogic();
	int						stuckTimer;
	idVec3					stuckLastPosition;
	int						stuckCounter;


	int						saveloadTimer;

	void					LoadLatestSave();
	void					LoadRandomSave();

	enum					{STS_NONE, STS_RANDOMSAVELOAD, STS_RANDOMLOAD, STS_LOADHUBS, STS_LOADALLMAPS};
};
//#pragma once