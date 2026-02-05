#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idSignalkit : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idSignalkit);

							idSignalkit(void);
	virtual					~idSignalkit(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

private:


	enum					{ SK_ALIVE, SK_BROKEN };
	int						state;

	idEntityPtr<idEntity>	myItem;

	idPhysics_RigidBody		physicsObj;

	

};
//#pragma once