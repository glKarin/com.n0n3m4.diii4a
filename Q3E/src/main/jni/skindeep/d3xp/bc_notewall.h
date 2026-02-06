#include "Misc.h"
#include "Entity.h"
#include "Fx.h"

class idNoteWall : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idNoteWall);

							idNoteWall(void);
	virtual					~idNoteWall(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	//virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					SetRead();
	void					SetMemorypalaceClone();

	bool					IsMemorypalaceClone();

	void					ShowMemoryPalace(bool value);

	bool					GetRead();

	void					SetMarkDoneToggle();
	bool					GetMarkedDone();

	void					Event_GetNoteRead(void);

private:
		
	bool					isRead;
	bool					isMemorypalaceClone;

	bool					markedDone;

};
//#pragma once