#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idInfoScreen : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idInfoScreen);

							idInfoScreen(void);
	virtual					~idInfoScreen(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual void			DoRepairTick(int amount);

private:

	enum					{ INFOSTAT_IDLE, INFOSTAT_DAMAGED };
	int						infoState;
	
	idFuncEmitter			*idleSmoke = nullptr;

	renderLight_t			headlight;
	qhandle_t				headlightHandle;
	void					SetLight(bool value, idVec3 color);
};
//#pragma once