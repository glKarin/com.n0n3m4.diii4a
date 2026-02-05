#include "Misc.h"
#include "Entity.h"
#include "Fx.h"

class idVRVisor : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idVRVisor);

							idVRVisor(void);
	virtual					~idVRVisor(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	bool					SetExitVisor();

private:

	virtual void			Event_PostSpawn(void);

	int						state;
	enum					{VRV_IDLE, VRV_MOVINGTOPLAYER, VRV_MOVEPAUSE, VRV_MOVINGTOEYES, VRV_ATTACHEDTOPLAYER, VRV_RETURNINGTOSTARTPOSITION};
	int						stateTimer;

	idVec3					visorStartPosition;
	idAngles				visorStartAngle;

	idVec3					visorFinalPosition;
	idAngles				visorFinalAngle;

	idVec3					playerStartPosition;
	float					playerStartAngle;


	idEntity*				arrowProp = nullptr;

	//BC 6-11-2025 fix for going out of bounds when exiting vr visor
	idVec3					GetSafeStartPosition();

};
