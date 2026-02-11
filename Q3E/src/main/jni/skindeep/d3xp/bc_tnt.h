#include "Misc.h"
#include "Entity.h"

#include "Item.h"

const int SPHEREPOINTCOUNT = 6;

class idTNT : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idTNT);

	void					Save(idSaveGame* savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame* savefile);

	void					Spawn(void);


	virtual void			Think(void);
	virtual void			Damage(idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual bool			DoFrob(int index = 0, idEntity* frobber = NULL);

	virtual bool			Collide(const trace_t& collision, const idVec3& velocity);

	virtual void			JustThrown();

	virtual void			Teleport(const idVec3& origin, const idAngles& angles, idEntity* destination);

private:

	int						state;
	enum
	{
		TNT_MOVEABLE,
		TNT_ARMED,
		TNT_TRIGGERED,
		TNT_EXPLODING,
		TNT_EXPLODED,
		TNT_DISARMING,
		TNT_PACKUPDONE
	};

	void					Deploy();

	idAnimated* animatedEnt = nullptr;
	jointHandle_t			clockJoint, originJoint;

	int						detonationTimer;

	void					Explode(void);
	void					TriggerBuzzer(void);

	bool					loudtick0;
	bool					loudtick1;
	bool					loudtick2;

	idVec3					damageRayPositions[SPHEREPOINTCOUNT];
	idVec3					damageRayAngles[SPHEREPOINTCOUNT];
	int						explosionIndex;
	int						explosionArrayTimer;

	int						lastThrowTime;

};
//#pragma once