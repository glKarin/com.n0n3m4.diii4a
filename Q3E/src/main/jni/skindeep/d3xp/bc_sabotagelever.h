#include "Misc.h"
#include "Entity.h"

class idSabotageLever : public idAnimated
{
public:
	CLASS_PROTOTYPE(idSabotageLever);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );
	virtual void			Think( void );
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	int						typeIndex;
	void					PostFTLReset();
	void					SetSabotagedArmed();

private:

	enum					{ IDLE, PRIMED, ARMED, EXHAUSTED };
	int						state;

	void					UpdateStates( void );

	int						nextThinkTime;	

	void					Event_PostPress();
	void					ChangeAllLevers(idVec4 newcolor);
};
