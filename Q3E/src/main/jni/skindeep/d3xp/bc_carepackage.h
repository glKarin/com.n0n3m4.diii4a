//#include "physics/Physics_RigidBody.h"
//#include "script/Script_Thread.h"
//#include "gamesys/Event.h"
//#include "Entity.h"
//#include "Player.h"
//#include "Projectile.h"
#include "Misc.h"
//#include "Target.h"
//#include "Moveable.h"
//#include "Mover.h"

class idCarepackage : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE(idCarepackage);

							idCarepackage(void);
	virtual					~idCarepackage(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );
	virtual void			Think( void );
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);

private:

	typedef enum
	{
		CPK_MINIPACKAGE_STATIC,
		CPK_MINIPACKAGE_AIRBORNE,
		CPK_PACKAGE_DEPLOYED
	} carepackage_state_t;

	carepackage_state_t		state;
	int						stateTimer;

	idFuncEmitter			*idleSmoke = nullptr;

	renderLight_t			boatlight;
	qhandle_t				boatlightHandle;

	void					EquipPrimarySlot();
	void					EquipSecondarySlot();
	void					EquipTertiarySlot();

	idEntity *				packageMini = nullptr;

	void					DeployPackage(idVec3 _pos, idMat3 _rot);
	idVec3					FindClearSpot(idVec3 _pos, idMat3 _rot);


	void					DoTetherWire(idVec3 _pos, int _idx);
	idBeam*					wireOrigin[2] = {};
	idBeam*					wireTarget[2] = {};

	void					SpawnDebrisPanels();



};
