#pragma once

#include "Misc.h"
#include "Target.h"

#define				RADIOCHECK_MAX 12 //this needs to match up with the amount of nato alphabet letter states we have. Refer to the UNIT_ enum in this file.

class idRadioCheckin : public idEntity
{
public:
	CLASS_PROTOTYPE(idRadioCheckin);

							idRadioCheckin();
							~idRadioCheckin();
	void					Spawn();
	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	virtual void			Think(void);
	void					StartCheckin(idVec3 _pos);
	void					InitializeEnemyCount();

	void					StopCheckin();

	void					ResetCooldown();

	void					DoPlayerFakeCheckin();
	bool					GetPlayingStatic();

	bool					CheckinActive();

	void					SetEnable(bool value);

private:
	
	int						state;
	enum					{ RDC_NONE, RDC_STARTCHECK, RDC_CHECKINS, RDC_COMBATALERT, RDC_ALLCLEAR };
	int						timer;
	bool					aliveKnowledge[RADIOCHECK_MAX]; //the AI's 'knowledge' of who they think is alive/dead. false = they know they're dead. true = they think they're alive.
	int						unitMaxCount;
	int						unitEntityNumbers[RADIOCHECK_MAX];
	int						checkinIndex;
	enum					{UNIT_ALPHA, UNIT_BRAVO, UNIT_CHARLIE, UNIT_DELTA, UNIT_ECHO, UNIT_FOX, UNIT_GAMMA, UNIT_HAVANA, UNIT_KILO, UNIT_LAMBDA, UNIT_ROMEO, UNIT_TANGO };
	bool					isPlayingStatic;

	int						GetNextAvailableCheckinIndex(int startingIndex);
	bool					IsUnitActuallyAlive(int unitDesignation, int *voiceprint);

	int						cooldownTimer;
};
