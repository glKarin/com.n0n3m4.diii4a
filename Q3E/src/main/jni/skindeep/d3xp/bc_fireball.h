#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idFireball : public idAnimated
{
public:
	CLASS_PROTOTYPE(idFireball);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	

private:

	enum					{ INITIALDELAY, ACTIVE, DEAD };
	int						state;

	int						timer;


};
//#pragma once