#include "Misc.h"
#include "Entity.h"

#include "Item.h"


class idSeekerGrenade : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idSeekerGrenade);

							idSeekerGrenade(void);
	virtual					~idSeekerGrenade(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	virtual void			Think(void);

	//virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	//void					Event_Touch(idEntity *other, trace_t *trace);	
	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

private:

	enum					{ SG_AIRBORNE, SG_DEPLOYDELAY, SG_DEPLOYING, SG_DONEDELAY, SG_DONE };
	int						state;
	int						stateTimer;
	int						ballCount;

	bool					DeployBall();
	idVec3					wallNormal;

};
//#pragma once