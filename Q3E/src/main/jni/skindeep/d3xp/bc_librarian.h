#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"
//#include "Light.h"


class idLibrarian : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE(idLibrarian);

							idLibrarian(void);
	virtual					~idLibrarian(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual void			Think(void);

private:
	
	idEntity *				DetectNoiseInRoom();
	virtual void			Event_PostSpawn(void);

	int						myLocEntNum;
	enum					{LBR_IDLE, LBR_SUSPICIOUS_DELAY, LBR_SUSPICIOUS, LBR_ALERTED};
	int						state;
	int						stateTimer;
	int						detectionTimer;	
	idFuncEmitter			*waterParticle = nullptr;

	int						suspicionDelayTime;
	int						suspicionTime;

	void					Event_Subvert(int value);

};
//#pragma once