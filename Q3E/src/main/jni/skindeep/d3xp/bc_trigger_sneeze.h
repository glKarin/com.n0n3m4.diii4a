#include "Trigger.h"

class idTrigger_sneeze : public idTrigger_Multi
{
public:
	CLASS_PROTOTYPE(idTrigger_sneeze);

	idTrigger_sneeze();
	void				Spawn();
	void				Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void				Restore(idRestoreGame *savefile);

	virtual void		Think(void);

private:
	void				Event_Touch(idEntity* other, trace_t* trace);
	int					lastUpdatetime;
	float				multiplier;

	idFuncEmitter		*particleEmitter = nullptr;

	int					maxlifetime;

	void				Despawn();

	bool				active;
	bool				touchedByAI;
};
