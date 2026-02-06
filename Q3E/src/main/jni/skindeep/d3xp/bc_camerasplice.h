#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"
//#include "Light.h"

#define CAMERASPLICE_BUTTONCOUNT	3

class idCameraSplice : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE(idCameraSplice);

							idCameraSplice(void);
	virtual					~idCameraSplice(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual void			Think(void);

	void					AssignCamera(idEntity *cameraEnt);

	virtual void			ListByClassNameDebugDraw();

	idEntityPtr<idEntity>	assignedCamera;

private:

	void					SwitchToCamera(idEntity *cameraEnt);
	void					SwitchToNextCamera(int direction);	
	int						currentCamIdx;
	
	enum					{CS_CLOSED, CS_OPENING, CS_IDLEOPEN};
	int						state;
	int						stateTimer;
	void					ConnectToCamera();

	renderLight_t			headlight;
	qhandle_t				headlightHandle;
	
	idFuncEmitter			*soundParticle;



	int						fanfareState;
	enum					{ CSF_NONE, CSF_INITIALIZE, CSF_ACTIVE, CSF_DONE };
	idEntity				*fanfareIcon = nullptr;
	idBeam*					beamStart = nullptr;
	idBeam*					beamEnd = nullptr;
	int						fanfareTimer;

	enum					{CSB_OVERRIDE, CSB_LEFT, CSB_RIGHT};
	idEntity*				buttons[CAMERASPLICE_BUTTONCOUNT] = {};

	idEntityPtr<idEntity>	transcriptNote;
	idVec3					transcriptStartPos;
	idVec3					transcriptEndPos;
	bool					transcriptLerping;
	int						transcriptTimer;

	bool					waitingForTranscriptRead;
	int						waitingForTranscriptReadTimer;

	

};
//#pragma once