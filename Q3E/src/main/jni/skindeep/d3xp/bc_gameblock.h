#include "Misc.h"
#include "Entity.h"


class idGameblock : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idGameblock);

							idGameblock(void);
	virtual					~idGameblock(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					SetLowerState();

private:

	int						state;
	enum					{GB_IDLE, GB_RAISINGUP, GB_UP, GB_LOWERINGDOWN};

	idVec3					startPos;
	idAngles				startAngle;

	idVec3					targetPos;
	idAngles				targetAngle;

	int						stateTimer;

	float					distanceToPlayerEye;

	idVec3					GetFrontOfBlock();
	float					GetDistanceFromBlock();

};
//#pragma once