#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idOxygenStation : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE(idOxygenStation);

							idOxygenStation(void);
	virtual					~idOxygenStation(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual void			Think(void);

private:

	int						animTimer;
	int						animState;
	enum					{ ANIM_IDLE, ANIM_FROBBING };

	enum					{ IDLE, DEPLETED };
	int						state;

	int						remainingAirTics;
	int						maxAirTics;
	
	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	void					DoParticleTowardPlayer(const char *jointname);

	float					armStartPos;
	float					armEndPos;

	int						announceTimer;
	bool					canAnnounce;

};
//#pragma once