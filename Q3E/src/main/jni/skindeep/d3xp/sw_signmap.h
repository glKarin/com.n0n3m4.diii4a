#include "Misc.h"
#include "Entity.h"

// Self-illuminated map object for sh_cargo02. Can be inspected, frobbed, and damaged
class idSignMap : public idLight
{
	CLASS_PROTOTYPE(idSignMap);

public:
							idSignMap(void);
	virtual					~idSignMap(void);

	void					Save(idSaveGame* savefile) const;
	void					Restore(idRestoreGame* savefile);

	void					Spawn(void);

	virtual bool			DoFrob(int index = 0, idEntity* frobber = NULL);
};