#include "Trigger.h"

class idTrigger_gascloud : public idTrigger_Multi
{
public:
	CLASS_PROTOTYPE(idTrigger_gascloud);

	idTrigger_gascloud();
	void				Spawn();
	void				Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void				Restore(idRestoreGame *savefile);

	virtual void		Think(void);

private:
	void				Event_Touch(idEntity* other, trace_t* trace);
	int					lastUpdatetime;	

	idFuncEmitter		*particleEmitter = nullptr;

	int					maxlifetime;

	void				Despawn();

	bool				active;
	bool				touchedBySomeone;
};
