#include "Misc.h"
#include "Entity.h"
#include "Item.h"

enum
{
	BAFFLER_OFF = 0,
	BAFFLER_ACTIVATING,
	BAFFLER_ON,
	BAFFLER_DELETING
};

enum
{
	BAFFLERHEALTH_PRISTINE = 0,
	BAFFLERHEALTH_LIGHTDAMAGE,
	BAFFLERHEALTH_HEAVYDAMAGE,
	BAFFLERHEALTH_DEAD
};

class idBaffler: public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idBaffler);
public:
							idBaffler(void);
	virtual					~idBaffler(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	
	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	//virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:

	int						state;
	int						timer;

	idEntity*				bubble = nullptr;

	const idDeclParticle *	soundParticles = nullptr;
	int						soundParticlesFlyTime;

	int						healthState;
	int						maxHealth;

	const idDeclParticle *	damageSmoke = nullptr;
	int						damageSmokeFlyTime;
	

};
//#pragma once