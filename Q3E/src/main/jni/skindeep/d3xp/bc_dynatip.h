#include "Misc.h"
#include "Entity.h"

class idDynaTip : public idEntity
{
public:
	CLASS_PROTOTYPE(idDynaTip);

	idDynaTip(void);
	virtual					~idDynaTip(void);

	void					Spawn(void);

	void					Save(idSaveGame* savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame* savefile);

	virtual void			Think(void);

	void					Draw(void);
	void					DrawText(void);

	float					GetDotAngle() { return dotAng; }

	bool					OnScreen(){ return tipState == TIP_ONSCREEN; }

	idEntity*				GetTarget() { return targetEnt.GetEntity(); }

private:

	virtual void			Event_PostSpawn(void);
	void					SetDynatipActive(int value);
	void					SetDynatipComplete();
	idVec3					GetIconPos();

	void					RecalculatePosition();

	void					Event_SetTarget(idEntity* entity);

	idStr					displayText;
	idEntityPtr<idEntity>   targetEnt;
	const idMaterial *		iconMaterial = nullptr;
	const idMaterial *		arrowMaterial = nullptr;
	idVec2					offscreenDrawPos;
	idVec2					actualscreenDrawPos;

	idVec3					drawOffset;

	float					dotAng;

	
	enum					
	{
		TIP_OFFSCREEN,				//0
		TIP_ONSCREEN,				//1
		TIP_LERPINGTO_ONSCREEN,		//2
		TIP_LERPINGTO_OFFSCREEN,	//3
		TIP_NODRAW,					//4 is active, but just isn't being drawn at the moment. ie. if in another room
		TIP_DORMANT					//5 if it starts off
	};

	int						tipState;
	int						lerpTimer;

	bool					hasGainedInitialLOS;
	int						losTimer;

	bool					initialized;
};
//#pragma once