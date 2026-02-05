#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idWalkietalkie : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idWalkietalkie);

							idWalkietalkie(void);
	virtual					~idWalkietalkie(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	
	//virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual void			Think(void);

	virtual void			Hide(void);
	virtual void			Show(void);

	int						GetBatteryAmount();	// SW 24th Feb 2025



private:

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	
	enum					{ WTK_IDLE, WTK_PLAYERSPEAKING };
	int						walkietalkieState;
	int						timer;

	//bool					LureEnemy();

	bool					lureAvailable;
	

	void					UpdateButtonVisualStates();
	int						visualTimer;

	//RADAR
	void					UpdateRadar();
	int						radarUpdateTimer;
	int						unitsDrawOnRadar;

	void					InterruptWalkietalkie();

	int						buttonDisplayState;
	enum					{WB_UNITIALIZED, WB_GREYED, WB_ENABLED};

	idAnimatedEntity*		flagModel = nullptr;

	int						batteryAmount;
	bool					DoBatteryLogic();

};
//#pragma once