#include "Misc.h"
#include "Entity.h"
#include "Mover.h"

class idTrashchute : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idTrashchute);

							idTrashchute(void);
	virtual					~idTrashchute(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	bool					IsChuteEnabled();
	void					Event_IsChuteEnabled(void);

	void					Event_ChuteEnable(int value);

private:

	virtual void			Event_PostSpawn(void);

	idMover *				platform = nullptr;

	int						detectionTimer;

	int						stateTimer;

	int						trashState;
	enum					{ TRASH_IDLE, TRASH_CHARGEUP, TRASH_SLAMMINGDOWN, TRASH_RESETTING , TRASH_DEACTIVATED };

	idBounds				GetTrashBounds();

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	
	idEntity				*gateModel = nullptr;
	enum					{ SHT_IDLE, SHT_SHUTTERING, SHT_SHUTTERED };
	int						shutterState;
	int						shutterTimer;

	void					Event_SpectatorTouch(idEntity* other, trace_t* trace);


//	idAnimated*				handle;
//	int						flushTimer;
//
//	int						flushMode;
};
//#pragma once