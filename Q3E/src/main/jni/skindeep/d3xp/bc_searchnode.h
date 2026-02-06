#include "Misc.h"
#include "Entity.h"

class idSearchNode : public idEntity
{
public:
	CLASS_PROTOTYPE(idSearchNode);

							idSearchNode(void);
	virtual					~idSearchNode(void);

	void					Spawn(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );


	int						lastTimeUsed;

	int						lastNodeAnimTime;

    idList<idStr>           nodeAnimList;

//private:


};
//#pragma once