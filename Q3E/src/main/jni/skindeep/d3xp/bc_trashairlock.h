#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idTrashAirlock : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idTrashAirlock);

							idTrashAirlock(void);
	virtual					~idTrashAirlock(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	bool					GetReadyStatus(void);

private:

	enum					{ AIRLOCKSTATE_IDLE, AIRLOCKSTATE_PRIMED, AIRLOCKSTATE_WAITFORINNERDOORCLOSE, AIRLOCKSTATE_CHARGEDELAY, AIRLOCKSTATE_CHARGING, AIRLOCKSTATE_EJECTING, AIRLOCKSTATE_WAITFOROUTERDOORCLOSE, AIRLOCKSTATE_POSTEJECTION };
	int						airlockState;

	idDoor *				innerDoor[3] = {};
	idDoor *				outerDoor[3] = {};

	void					Event_airlock_prime(idEntity * ent);

	int						thinkTimer;

	idEntityPtr<idEntity>	trashcubeEnt;

	idLight *				sirenLight = nullptr;

	bool					cubeIsSpinning;

	idVacuumSeparatorEntity * vacuumSeparators[2] = {};

};
//#pragma once