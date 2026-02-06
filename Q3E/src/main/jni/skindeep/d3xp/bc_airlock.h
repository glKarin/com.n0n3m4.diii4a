#include "Misc.h"
#include "Entity.h"
#include "Mover.h"
//#include "Item.h"
#include "Light.h"


#define MAX_GAUGES 2

class idAirlock : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idAirlock);

							idAirlock(void);
	virtual					~idAirlock(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	virtual void			Event_PostSpawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					DoAccumulatorStatusUpdate(int gaugeIndex);
	void					InitializeAccumulator(idEntity *accumulator);

	//Airlock lockdown.
	bool					IsAirlockLockdownActive(bool includePreamble = true);
	void					DoAirlockLockdown(bool value, idEntity *interestpoint = NULL);

	void					SetWarningSign(bool value);
	void					SetWarningSignText(const char *text);

	void					SetFTLDoorGuiNamedEvent(const char* eventName);
	void					SetFTLDoorGuiDisplaytime(const char* value);

	void					StartBoardingSequence();
	void					StartBoardingDoorOpen();


	void					SetFuseboxGateOpen(bool value);

	void					SetCanEmergencyPurge(bool value);

	bool					IsFuseboxgateShut();

	//BC PUBLIC END

private:

	bool					canEmergencyPurge;

	enum					{	AIRLOCKSTATE_IDLE, //both doors closed.

								AIRLOCKSTATE_INNERDOOR_PRIMING, //delay & attempt to open innerdoor.
								AIRLOCKSTATE_INNERDOOR_OPEN,

								AIRLOCKSTATE_OUTERDOOR_PRIMING, //delay & attempt to open outerdoor.
								AIRLOCKSTATE_OUTERDOOR_OPEN,

								AIRLOCKSTATE_EMERGENCY_OPEN,
								AIRLOCKSTATE_WAITINGFORINNERCLOSE,
								AIRLOCKSTATE_WAITINGFORCLOSE,

								
								AIRLOCKSTATE_LOCKDOWN_WAITINGFORINNERCLOSE,
								AIRLOCKSTATE_LOCKDOWN_OUTERDOORDELAY,
								AIRLOCKSTATE_LOCKDOWN_WAITINGFORFTL,

								AIRLOCKSTATE_BOARDING_DOCKED,
								AIRLOCKSTATE_BOARDING_DOORSOPENING
							};

	int						airlockState;

	int						thinkTimer;

	idLight *				sirenLight = 0;

	idVacuumSeparatorEntity* vacuumSeparators[2] = {};

	idDoor*					innerDoor[2] = {};
	idDoor*					outerDoor[2] = {};

	bool					lastOuterdoorOpenState; //true = open;

	int						pullTimer; //when in emergency open, how often to update the physics pull logic.

	void					DoPhysicsPull(bool totalPull, bool affectPlayer);

	//idEntity *				emergencyButton;

	int						doorOffset;

	void					DoEmergencyPurge();
	idList<int>				accumulatorList;

	idEntity*				gauges[MAX_GAUGES] = {};
	int						gaugeCount;

	bool					accumulatorsAllBroken;
	bool					doorslowmodeActivated;

	//Airlock lockdown.
	idAnimatedEntity*		gateProps[2] = {};
	int						lockdownPreambleState;
	enum					{LDPA_NONE, LDPA_DISPATCH, LDPA_ANNOUNCER};
	int						lockdownPreambleTimer;

	idAnimatedEntity*		warningsigns[2] = {};

	//idEntity *				ftlDoorSign;
	idEntity *				CCTV_monitor = 0;
	idEntity *				CCTV_camera = 0;

	bool					ShouldButtonsWork();
	
	int						locationEntNum;

	void					VacuumSuckItems(idVec3 suctionInteriorPosition, idVec3 suctionExteriorPosition);
	int						itemVacuumSuckTimer;

	bool					hasPulledActors;

	int						lastVoiceprint;


	idEntity*				fuseboxGates[2] = {};

	//BC 2-18-2025: fusebox shutter
	enum { SHT_IDLE, SHT_SHUTTERING, SHT_SHUTTERED };
	int						shutterState;
	int						shutterTimer;


	//private end

};
//#pragma once