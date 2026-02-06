#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
#include "Light.h"


class idSupplyStation : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idSupplyStation);

							idSupplyStation(void);
	virtual					~idSupplyStation(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual void			DoGenericImpulse(int index);

private:

	//enum					{ INFOSTAT_IDLE, INFOSTAT_DAMAGED };
	//int						infoState;

	idEntityPtr<idEntity>	podOwner;

    void                    PurchaseItem(const char *defName, int cost);


};
//#pragma once