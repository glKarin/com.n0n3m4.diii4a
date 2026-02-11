#include "Misc.h"
#include "Entity.h"
#include "physics/Physics_RigidBody.h"
//#include "Mover.h"
//#include "Item.h"

//#include "bc_frobcube.h"


class idWallspeaker : public idEntity
{
public:
	CLASS_PROTOTYPE(idWallspeaker);

							idWallspeaker(void);
	virtual					~idWallspeaker(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);	

	void					ActivateSpeaker(const char *soundshaderName, const s_channelType soundchannel = SND_CHANNEL_VOICE);

private:

	idPhysics_RigidBody		physicsObj;

	idFuncEmitter			*soundwaves = nullptr;

	int						deathtimer;



};
//#pragma once