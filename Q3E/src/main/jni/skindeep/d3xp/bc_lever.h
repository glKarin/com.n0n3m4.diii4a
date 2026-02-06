#include "Misc.h"
#include "Entity.h"

class idLever : public idAnimated
{
public:
	CLASS_PROTOTYPE( idLever );

							idLever();
	virtual					~idLever(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );
	virtual void			Think( void );
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	bool					Event_Activate( idEntity *activator );

	void					SetActive(bool value);
	

	virtual void		Hide(void);
	virtual void		Show(void);

private:

	enum					{ IDLE, PRESSED };
	int						state;

	int						nextStateTime;

	void					UpdateStates( void );

	bool					isActive;

	idFuncEmitter			*soundParticle = nullptr;

	
};
