#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"

const int MAXTRASHFISH = 6;

class idEscapePod : public idAnimated
{
public:
	CLASS_PROTOTYPE(idEscapePod);

							idEscapePod(void);
	virtual					~idEscapePod(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

private:

	int						stateTimer;
	int						state;
	enum					{ IDLE, INITIALPAUSE, SINK_INSIDE, DONE };

	void					StartDeploymentSequence(idEntity *ent);

	idEntityPtr<idEntity>	skullPtr;
	idVec3					skullStartPos;
	idVec3					skullEndPos;

	
};
//#pragma once