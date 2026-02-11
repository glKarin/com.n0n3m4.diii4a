#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idSignPrompt : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idSignPrompt);

							idSignPrompt(void);
	virtual					~idSignPrompt(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual void			DoRepairTick(int amount);

	void					UpdatePrompts();

	void					GuiUpdate(idStr keyname, idStr value);


private:

	enum					{ SP_IDLE, SP_DAMAGED };
	int						infoState;
	int						thinktimer;
	

	
	idFuncEmitter			*idleSmoke = nullptr;
	
	renderLight_t			headlight;
	qhandle_t				headlightHandle;
	
	void					SetLight(bool value, idVec3 color);

	void					Event_SetSignEnable(int value);


	enum					{ SPS_ACTIVE, SPS_ACTIVATING, SPS_DEACTIVATED };
	int						spsState;
	int						spsTimer;

};
//#pragma once