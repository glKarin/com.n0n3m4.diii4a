#include "Misc.h"
#include "Entity.h"

class idSabotageShutdown : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE(idSabotageShutdown);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );
	virtual void			Think( void );
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					SetEnabled(bool value);

private:

	enum					{ IDLE, PRESS_DELAY, COOLDOWN_WAIT };
	int						state;
	int						stateTimer;
};
