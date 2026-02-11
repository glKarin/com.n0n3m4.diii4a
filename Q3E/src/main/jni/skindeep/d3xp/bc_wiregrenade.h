#include "Misc.h"
#include "Entity.h"

#include "Item.h"

const int MAXWIRES = 5;

enum
{
	WIREGRENADE_IDLE = 0,
	WIREGRENADE_HASBOUNCED,
	WIREGRENADE_ISARMED,
	WIREGRENADE_HASTRIPPED,
	WIREGRENADE_HASEXPLODED,
	WIREGRENADE_NEUTRALIZED
};

class idWiregrenade : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idWiregrenade);

							~idWiregrenade(void);
	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);
	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	virtual void			Killed(idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location);

	virtual void			PostSaveRestore( idRestoreGame * savefile ) override;

private:
//
//	int						nextTouchTime;

	//void					TeleportThingsTo(int listedEntities, idEntity *entityList[], idVec3 sourcePosition, idVec3 targetPosition);
	bool					SpawnWire(idBeam *startBeam, idBeam *endBeam, idEntity *hook, idVec3 targetPos, idVec3 targetNormal, jointHandle_t grenadeJoint);

	void					TripTheBomb(void);

	bool					hasBounced;

	int						grenadeTimer;
	int						grenadeState;

	idAnimated*				animatedEnt = nullptr;

	idBeam*					wireOrigin[MAXWIRES] = {};
	idBeam*					wireTarget[MAXWIRES] = {};

	idStaticEntity*			wireHooks[MAXWIRES] = {};

	int						checkTimer;

	void					BecomeArmed(bool immediateDeploy);
	int						DeployWires(idVec3 hitPoints[MAXWIRES], idVec3 hitNormals[MAXWIRES], jointHandle_t grenadeJoint);

    int                     playerLeniencyTime;
	bool					ShouldDoLeniencyCheck();

	idVec3 *				GetWirepointsOnSphere(const int num, idVec3 startpoint);

};
//#pragma once