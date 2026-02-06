#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idCat : public idAnimated
{
public:
	CLASS_PROTOTYPE(idCat);

							idCat(void);
	virtual					~idCat(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	void					DoCageBurst();

	bool					IsAvailable();
	bool					IsRescued();
	bool					IsCaged();
	bool					IsInPod();

	void					PutInPod(idVec3 spawnPos, idVec3 cubbyPos, int delaytime);

private:

	int						catState;
	enum					{ CAT_CAGED, CAT_RESCUEJUMP, CAT_AVAILABLE, CAT_IDLE, CAT_JUMP_LOOP, CAT_FAILSAFEJUMP, CAT_JUMPING_TO_CATPOD_DELAY, CAT_JUMPING_TO_CATPOD, CAT_JUMP_TO_CATPOD_DONE };

	void					DoSafetyJump(void);

	idVec3					startPosition;
	idVec3					targetPosition;
	idAngles				targetAngle;
	idVec3					targetMovedir;
	idVec3					targetNormal;

	bool					hasPlayedUnstretchAnimation;
	int						stateTimer;

	idEntityPtr<idEntity>	lookEnt;
	jointHandle_t			headJoint;


	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	void					SetHeadlight(bool value);

	idVec3					GetHeadPos();

};
//#pragma once