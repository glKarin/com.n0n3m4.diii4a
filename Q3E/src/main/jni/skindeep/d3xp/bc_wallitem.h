#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idWallItem : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idWallItem);

							idWallItem(void);
	virtual					~idWallItem(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

private:

	int						state;
	enum					{ IDLE, DEAD, REMOVED };

	idEntity *				SpawnTheItem();


	void					DoDecalEffects();

	void					DamageTargets();

	int						targetTimer;

};
//#pragma once