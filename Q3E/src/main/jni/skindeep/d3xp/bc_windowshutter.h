#include "Misc.h"
#include "Entity.h"



class idWindowShutter : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idWindowShutter);

							idWindowShutter();
	virtual					~idWindowShutter(void);

	void					Spawn(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					SetShutterOpen(bool value);

	virtual void			Event_PostSpawn(void);

	virtual void			Think(void);

private:

	bool					DoesWindowIntersect(idEntity *windowEnt);


	idEntityPtr<idEntity>	assignedWindow;

	enum                    {SHT_IDLE, SHT_SHUTTERING, SHT_SHUTTERED};
	int						shutterState;
	int						shutterTimer;

	//BC 3-26-2025: locbox.
	idEntity*				locbox = nullptr;

	// SW 22nd April 2025
	void						Event_SpectatorTouch(idEntity* other, trace_t* trace);
};
//#pragma once#pragma once
