
#include "Entity.h"


class idSpectateNode : public idEntity
{
public:
	CLASS_PROTOTYPE(idSpectateNode);

	idSpectateNode();
	~idSpectateNode();

	void					Spawn(void);
	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);
	virtual void			Think(void);

	void					Draw();


private:

	idStr					text;
	idStr					timestamp;
	
};