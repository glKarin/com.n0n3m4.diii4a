#pragma once

#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"

class idProximityAnnouncer
{
public:
	idProximityAnnouncer() : idProximityAnnouncer(nullptr){}
	idProximityAnnouncer(idEntity* ent);
	void					Start();

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	bool					IsProximityNearSomeone();
	void					Update();
	void					DoSoundwaves();
	bool					Ready();

	idEntity*				sensor = nullptr;
	int						proximityCheckTimer;
	bool					canProximityAnnounce;
	int						coolDownTimer;
	bool					checkHealth;
	int						activationRadius;
	int						checkPeriod;
	int						coolDownPeriod;
};

class idHealthstation : public idAnimated
{
public:
	CLASS_PROTOTYPE(idHealthstation);

							idHealthstation(void);
	virtual					~idHealthstation(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	void					SetCombatLockdown(bool value);

	virtual void			DoRepairTick(int amount);

	virtual void			Event_PostSpawn(void);

	virtual void			DoHack(); //for the hackgrenade.
	void					DispenseHealcloud();

private:

	enum					{ HEALTHSTATION_IDLE, HEALTHSTATION_DISPENSEDELAY, HEALTHSTATION_DISPENSING, HEALTHSTATION_COMBATLOCKED, HEALTHSTATION_DAMAGED };
	int						state;

	int						stateTimer;

	void					DoNozzleParticle(const char *particleName, bool pointTowardPlayer);

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	bool					doBloodDispense;
	bool					playerWasLastFrobber;
	int						bloodDispenseTimer;
	int						bloodDispenseTickInterval;

	idProximityAnnouncer    proximityAnnouncer;

	void					UnlockMachine();

	void					Event_SetHealthstationAnnouncer(int value);
	bool					proximityannouncerActive;





	
};
//#pragma once