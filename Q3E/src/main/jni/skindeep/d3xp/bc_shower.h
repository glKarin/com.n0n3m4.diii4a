#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"
//#include "Light.h"


class idShower : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE(idShower);

							idShower(void);
	virtual					~idShower(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual void			Think(void);

private:
	
	enum					{SW_OFF, SW_ON, SW_BROKEN};
	int						state;
	
	idFuncEmitter			*waterParticle = nullptr;

	void					SetActive(bool value);

	int						cleanseTimer;

	int						interestTimer;

	idVec3					GetJointPosition(idStr jointname);

};
//#pragma once