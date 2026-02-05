#include "Misc.h"
#include "Entity.h"
#include "Mover.h"

#include "Item.h"


class idVentdoor : public idDoor
{
public:
	CLASS_PROTOTYPE(idVentdoor);

							idVentdoor();
	virtual					~idVentdoor(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	idVec3					GetPlayerDestinationPos();
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual bool			IsFrobHoldable() const;
	//virtual const idStr&	GetFrobName() const;

	void					SetPurgeSign(bool value);
protected:

	virtual void			Event_Reached_BinaryMover(void);
	float					GetFacingResult() const;

private:

	int						ventTimer;
	int						ventState;

	enum
	{
		VENT_NONE = 0,
		VENT_PLAYERZIPPING
	};

	void					VentClose();
	idWinding*				GetAperture();

	void					SetAASAreaState(bool closed); // SW: Override idDoor's method to prevent vents from messing with any nearby cluster portals

	//idFuncEmitter			*peekSparkle;
	idAnimatedEntity		*warningsign[2] = {};
	idStr					displayNamePeek;

	//BC 3-25-2025: locbox.
	idEntity* locbox = nullptr;
};
//#pragma once