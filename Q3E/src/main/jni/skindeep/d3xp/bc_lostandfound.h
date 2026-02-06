#pragma once

#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"
#include "bc_healthstation.h"


class idLostAndFound : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idLostAndFound);

							idLostAndFound(void);
	virtual					~idLostAndFound(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	//virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			DoRepairTick(int amount);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	virtual void			DoGenericImpulse(int index);
	void					UpdateLostEntityCache();

	idList<int>				cachedEntityDefNums; //this is public so that the lostandfound monitors can see it

	idVec3					GetDispenserPosition();

private:

	enum					{ VENDSTATE_IDLE, VENDSTATE_DELAY };
	int						vendState;
	int						stateTimer;
	int						updateCacheTimer;

	idListGUI *				itemList = nullptr;		// easy map list handling

	const idDeclEntityDef	*itemDef;

	idProximityAnnouncer	proximityAnnouncer;

	//BC 3-25-2025: locbox.
	idEntity* locbox = nullptr;

public:
	//BC 4-10-2025: more robust handling of finding a candidate position for spawning objects.
	idVec3					FindValidSpawnPosition(idBounds itemBounds);

};
//#pragma once