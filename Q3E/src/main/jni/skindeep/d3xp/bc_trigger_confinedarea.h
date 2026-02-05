#include "Trigger.h"

class idTrigger_confinedarea : public idTrigger_Multi
{
public:
	CLASS_PROTOTYPE(idTrigger_confinedarea);

						idTrigger_confinedarea();
	virtual				~idTrigger_confinedarea(void);
	void				Spawn();
	void				Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void				Restore(idRestoreGame *savefile);

	float				adjustedBaseAngle;
	float				baseAngle;
	float				playerEnterAngle;

private:
	void				Event_Touch(idEntity* other, trace_t* trace);
	int					lastUpdatetime;
	
};
