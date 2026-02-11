#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


class idInfoStation : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idInfoStation);

							idInfoStation(void);
	virtual					~idInfoStation(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual void			DoGenericImpulse(int index);

	void					SetToMode(int value);

	virtual void			DoRepairTick(int amount);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					DoDelayedDoorlockDisplayUpdate();

	void					SetupRoomLabels();

private:

	virtual void			Event_PostSpawn(void);

	enum					{ INFOSTAT_IDLE, INFOSTAT_DAMAGED };
	int						infoState;
	

	//void					Event_Activate(idEntity *activator);

	idFuncEmitter			*idleSmoke = 0;

	idEntityPtr<idEntity>	FTLDrive_ptr;

	bool					lastFTLPauseState;

	void					OnClickLegendButton(int index);

	
	bool					hasSetupSelfMarker;

	renderLight_t			headlight;
	qhandle_t				headlightHandle;
	void					SetLight(bool value, idVec3 color);
	
	idEntity				*hackboxEnt = 0;
	void					UpdateSoulsLocation();
	int						soulUpdateTimer;
	//bool					hackboxHacked;

	bool					trackingHuman;
	bool					trackingCat;


	
	bool					initialLockdoorDisplayDone;
	int						initialLockdoorDisplayTimer;

	int						lastSaveTime;

	void					UpdateLockedDoorDisplay();

    void                    SummonRepairbot();

};
//#pragma once