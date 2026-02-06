#include "Misc.h"
#include "Entity.h"

#include "Item.h"

#define CLEANUP_RADIUS 32 //when glass is stepped on, remove all other glass pieces in XX radius.

class idLightbulb : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idLightbulb);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);



	void					Event_Touch(idEntity *other, trace_t *trace);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);

	//virtual void			Think(void);	

	void					ShatterAndRemove(trace_t collision);

	virtual bool			JustBashed(trace_t tr);


//private:
//
//
//	int						spawnTimer;

	

};
//#pragma once