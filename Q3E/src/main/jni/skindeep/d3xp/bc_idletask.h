#pragma once

#include "Misc.h"
#include "Entity.h"


class idIdleTask : public idEntity
{
public:
	CLASS_PROTOTYPE(idIdleTask);

							idIdleTask(void);
	virtual					~idIdleTask(void);

	void					Spawn(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	//virtual void			Think( void );	

	bool					SetActor(idAI *actor);

	bool					IsClaimed();
	idEntityPtr<idAI>		assignedActor;

	idEntityPtr<idEntity>	assignedOwner; //example: the health station

private:

	idStr					idleAnim;

	
	

	

};
//#pragma once