#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"
//#include "Light.h"
#include "Mover.h"

#define CAMERASPLICE_BUTTONCOUNT	3

class idTeletext : public idAnimated
{
public:
	CLASS_PROTOTYPE(idTeletext);

							idTeletext(void);
	virtual					~idTeletext(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual void			Think(void);

private:

	enum					{TT_IDLE, TT_MOVINGTOPOSITION, TT_PROJECTING, TT_DORMANT};
	int						state;
	int						stateTimer;
	
	idLight*				spotlights[2] = {};
	idMover*				mover = nullptr;


	void					SetTeletextActive(int value);
	void					SetTeletextMaterial(const char* materialName);
	void					StartSlideShow();

	idEntity*				arrowProp = nullptr;
};
