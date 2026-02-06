#include "Misc.h"
#include "Entity.h"

#include "Item.h"

class idTeleportPuck : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idTeleportPuck);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	//void					Create(const idVec3 &start, const idMat3 &axis);

	virtual void			Think(void);
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

private:

	bool					HasCollided;
	//void					TeleportThingsTo(int listedEntities, idEntity *entityList[], idVec3 sourcePosition, idVec3 targetPosition);

	idVec3					FindClearDestination(idVec3 pos, trace_t collisionInfo);

	idVec3					startPosition;

};
//#pragma once