#include "Misc.h"
#include "Entity.h"
#include "physics/Physics_RigidBody.h"
//#include "Mover.h"
//#include "Item.h"

//#include "bc_frobcube.h"


class idSoundspeaker : public idEntity
{
public:
	CLASS_PROTOTYPE(idSoundspeaker);

							idSoundspeaker(void);
	virtual					~idSoundspeaker(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);	



private:

	idPhysics_RigidBody		physicsObj;

	idFuncEmitter			*soundwaves = nullptr;

	int						deathtimer;



};
//#pragma once