#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"
//#include "Light.h"


class idEmailflag : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE(idEmailflag);

							idEmailflag(void);
	virtual					~idEmailflag(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

private:
	
	enum					{UNDEPLOYED, DEPLOYED};
	int						state;	


	int						checkTimer;
	int						flagType;

	int						idleTimer;

	//BC 3-22-2025: locbox for emailflag.
	idEntity*				locbox = nullptr;

};
//#pragma once