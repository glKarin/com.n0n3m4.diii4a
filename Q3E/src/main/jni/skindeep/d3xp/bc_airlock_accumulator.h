#include "Misc.h"
#include "Entity.h"

class idAirlockAccumulator : public idAnimated
{
public:
	CLASS_PROTOTYPE(idAirlockAccumulator);

							idAirlockAccumulator(void);
	virtual					~idAirlockAccumulator(void);

	void					Save(idSaveGame *savefile) const;
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual void			Event_PostSpawn(void);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	bool					IsDeflated();

private:

	idEntityPtr<idEntity>	airlockEnt;

	int						stateTimer;
	int						state;
	enum					{ APA_IDLE, APA_BUTTONANIMATING, APA_DEFLATING, APA_DEAD };

	void					SetDeflate();

	idEntity *				emergencyButton = nullptr;


	bool					hasUpdatedAccumStatus;
	int						accumStatusTimer;

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	idEntityPtr<idEntity>	cableEnt;
	
};
//#pragma once