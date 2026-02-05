//#include "Player.h"

#include "Light.h"
#include "ai/AI.h"

const int SCANBEAMS = 12;

class idAI_Spearbot : public idAI
{
public:
	CLASS_PROTOTYPE(idAI_Spearbot);
	
							idAI_Spearbot(void);
	virtual					~idAI_Spearbot(void);

	void					Spawn(void);
	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	virtual void			DoHack(); //for the hackgrenade.

private:

	enum
	{
		SPEARSTATE_WANDERING,
		SPEARSTATE_SCANCHARGE,
		SPEARSTATE_SCANNING,
		SPEARSTATE_RAMWARNING,
		SPEARSTATE_DEATHFANFARE,
		SPEARSTATE_DONE
	};

	
	int						stateTimer;

	int						suspicionPips;
	int						suspicionTimer;

	int						beamTimer;

	idEntityPtr<idEntity>	ramTargetEnt;
	idVec3					ramPosition;
	idVec3					GetTargetPosition(idEntity *targetEnt);
	idVec3					ramDirection;

	idBeam*					beamOrigin[SCANBEAMS] = {};
	idBeam*					beamTarget[SCANBEAMS] = {};

	idVec3					beamOffsets[SCANBEAMS];
	idVec3					beamRandomAngles[SCANBEAMS];

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	idFuncEmitter			*scanchargeParticles = nullptr;

	idBeam*					targetinglineStart = nullptr;
	idBeam*					targetinglineEnd = nullptr;

	void					Launch();

	idVec3					FindDeathTarget();
	bool					isplayingLockBuzz;

};
//#pragma once