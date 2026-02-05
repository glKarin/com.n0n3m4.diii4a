#include "Misc.h"
#include "Entity.h"
#include "Item.h"



class idGrabRing: public idEntity
{
public:
	CLASS_PROTOTYPE(idGrabRing);
public:
							idGrabRing(void);
	virtual					~idGrabRing(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	
	virtual void			Think(void);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);


private:

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	idVec3					GetClearancePosition(int rightOffset, int forwardOffset);

};
