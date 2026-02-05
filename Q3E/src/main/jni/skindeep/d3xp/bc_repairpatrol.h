#include "Misc.h"
#include "Entity.h"

class idRepairPatrolNode : public idEntity
{
public:
	CLASS_PROTOTYPE(idRepairPatrolNode);

							idRepairPatrolNode(void);
	virtual					~idRepairPatrolNode(void);

	void					Spawn(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	

//private:

	//int						lastTimeUsed;


};
//#pragma once