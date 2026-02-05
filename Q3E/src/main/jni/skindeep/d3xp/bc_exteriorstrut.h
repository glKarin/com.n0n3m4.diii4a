#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idExteriorStrut : public idAnimated
{
public:
	CLASS_PROTOTYPE(idExteriorStrut);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	

private:

	virtual void			Event_PostSpawn(void);

	//idEntityPtr<idEntity>	lookEnt;
	jointHandle_t			armJoint;



	//idEntityPtr<idEntity>	targetEnt;

	idEntity				*nullEnt = nullptr;

};
//#pragma once