#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"

class idTrashfish : public idAnimated
{
public:
	CLASS_PROTOTYPE(idTrashfish);

							idTrashfish(void);
	virtual					~idTrashfish(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	void					SetFishInfo(idEntity * _airlock, int _index);
	void					SetFishHive(idEntity * _hive);

private:	

	bool					DoCubeCheck();

	idEntityPtr<idEntity>	airlockEnt;

	int						fishState;
	enum					{ TRASHFISHSTATE_IDLE, TRASHFISHSTATE_HEADING_TO_AIRLOCK, TRASHFISHSTATE_SUCKLING_ON_AIRLOCK, TRASHFISHSTATE_HEADING_TO_CUBE, TRASHFISHSTATE_SUCKLING_ON_CUBE };

	idEntity *				GetValidCube();
	int						fishIndex;

	idEntityPtr<idEntity>	targetCube;

	void					MoveToPos(idVec3 newPos);

	idVec3					GetValidPerchingSpot();

	int						totalMovetime;
	int						stateTimer;
	idVec3					startPosition;
	idVec3					endPosition;

	void					AttachFishToCube(idEntity * cube);

	idEntityPtr<idEntity>	hiveOwner;


	int						bubbleTimer;
};
//#pragma once