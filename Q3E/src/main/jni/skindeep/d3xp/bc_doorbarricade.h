#include "Misc.h"
#include "Entity.h"
#include "Mover.h"

class idDoorBarricade : public idStaticEntity
{
public:
	CLASS_PROTOTYPE( idDoorBarricade );

							idDoorBarricade(void);
	virtual					~idDoorBarricade(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	virtual void			Think(void);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	idEntityPtr<idDoor>		owningDoor;

	virtual void			DoHack(); //for the hackgrenade.

private:

	void					UnlockBarricade();

	void					UnlockAllBarricades();

	int						keyrequirementTimer;
	bool					keyrequirementLightActive;

	bool					UpdateProximity();
	int						proximityTimer;
	bool					proximityActive;
	bool					DoProxCheckPlayer();
	bool					DoProxCheckGround();
	bool					DoProxCheckAI();

	idBeam*					beamOrigin = nullptr;
	idBeam*					beamTarget = nullptr;
	void					UpdateLaserPosition(idVec3 targetPos);

	bool					particleDone;

	//BC 4-12-2025: locbox
	idEntity*				locboxes[2] = {};
};
//#pragma once