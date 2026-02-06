#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idAlgaeball : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idAlgaeball);

							idAlgaeball(void);
	virtual					~idAlgaeball(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	virtual void			Think(void);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	//void					Event_Touch(idEntity *other, trace_t *trace);	
	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	virtual bool			DoFrob(int index = 0, idEntity* frobber = NULL);
	virtual void			Hide(void);
	virtual void			Show(void);

private:

	enum					{ SG_AIRBORNE, SG_DEPLOYED, SG_TRIPPED, SG_DONE };
	int						state;
	int						stateTimer;

	idAnimated*				displayModel = nullptr;

	idVec3					wallNormal;

	int						spawnTime;

	void					DoGasBurst();
	void					SpawnSingleCloud(idVec3 _pos, float _cloudRadius);

};
//#pragma once