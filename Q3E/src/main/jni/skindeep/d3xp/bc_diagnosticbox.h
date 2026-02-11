#include "Misc.h"
#include "Entity.h"
#include "Mover.h"

class idDiagnosticBox : public idAnimated
{
public:
	CLASS_PROTOTYPE(idDiagnosticBox);

							idDiagnosticBox(void);
	virtual					~idDiagnosticBox(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	//virtual void			Event_PostSpawn(void);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	//bool					IsDeflated();

	virtual void			DoGenericImpulse(int index);

	virtual void			Event_PostSpawn(void);

private:

	void					InitializeDiagnosticCamera();
	idEntity				*GetAnyCamera();
	int						currentCamIdx;
	void					SwitchToNextCamera(int direction);
	void					SwitchToCamera(idEntity *cameraEnt);

	int						stateTimer;
	int						state;
	enum					{ DB_CLOSED, DB_OPEN };

    int                     updateTimer;

	idMover *				pillar = nullptr;

	bool					isUpsideDown;


	
};
//#pragma once