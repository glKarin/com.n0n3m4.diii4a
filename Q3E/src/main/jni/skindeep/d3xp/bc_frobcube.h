#include "Misc.h"
#include "Entity.h"

class idFrobcube : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idFrobcube);

							idFrobcube(void);
	virtual					~idFrobcube(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					SetIndex(int value);

	virtual void			Show(void);

	virtual void			SetPostFlag(customPostFlag_t flag, bool enable);

private:

	int						index;
};
//#pragma once