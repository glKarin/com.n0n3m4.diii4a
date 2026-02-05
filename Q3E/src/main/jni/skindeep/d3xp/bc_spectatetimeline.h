
#include "Entity.h"


class idSpectateTimeline : public idEntity
{
public:
	CLASS_PROTOTYPE(idSpectateTimeline);

	idSpectateTimeline();
	~idSpectateTimeline();

	void					Spawn(void);
	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);
	virtual void			Think(void);

	void					ToggleAllNodes(bool enable);


private:
	
	int						rolloverDebounceIdx;
	bool					hasLOS(idEntity* nodeEnt);
};