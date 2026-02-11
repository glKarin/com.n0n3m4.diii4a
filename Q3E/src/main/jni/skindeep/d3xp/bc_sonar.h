#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idSonar : public idAnimated
{
public:
	CLASS_PROTOTYPE(idSonar);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	
	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

private:

	int						state;
	int						maxHealth;

	enum
	{
		SONAR_OPENING = 0,
		SONAR_IDLE,
		SONAR_CLOSING,
		SONAR_PACKUPDONE,
		SONAR_DESTROYED
	};

	int						timer;
	
	idFuncEmitter			*particleEmitter = nullptr;

	int						sonarPingTime;

	void					DrawMarkerAtJoint(const char *jointName, idEntity *ent, int markerType = 0);
	bool					hasPlayedPingSound;

	int						offsets[256];
	int						offsetIndex;
	int						glitchTimer;

	bool					damageParticles;

};
//#pragma once