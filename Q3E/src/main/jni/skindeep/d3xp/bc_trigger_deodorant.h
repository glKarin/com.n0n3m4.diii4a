#include "Trigger.h"

class idTrigger_deodorant : public idTrigger_Multi
{
public:
	CLASS_PROTOTYPE(idTrigger_deodorant);

						idTrigger_deodorant();
	virtual				~idTrigger_deodorant(void);
	
	void				Spawn();
	void				Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void				Restore(idRestoreGame *savefile);

	virtual void		Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual void		Think(void);
	

private:

	bool				active;

	void				Event_Touch(idEntity* other, trace_t* trace);

	void				BurstIntoFlames(idEntity *inflictor);

	idEntityPtr<idFuncEmitter>		particleEmitter;

	int					maxlifetime;
};
