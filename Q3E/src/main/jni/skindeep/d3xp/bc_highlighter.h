#pragma once

#include "Mover.h"
#include "Camera.h"
#include "Misc.h"
#include "Target.h"

#define MAX_ARROWS 2

typedef struct {
	idVec3				position;
	idStr				name;
	idBounds			bounds;
} highlightEntity_t;

class idHighlighter : public idEntity
{
public:
	CLASS_PROTOTYPE(idHighlighter);

							idHighlighter();
							~idHighlighter();

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn();

	virtual void			Think(void);

	bool					DoHighlight(idEntity* ent0, idEntity* ent1);

	bool					IsHighlightActive();

	void					DrawBars();

	void					DoSkip();

private:
	
	bool					CanTraceSeeEntity(trace_t tr, idEntity* ent);
	idVec2					GetUpperBound(idBounds entBounds);
	idVec2					GetLowerBound(idBounds entBounds);

	int						state;
	enum					{ HLR_NONE, HLR_LERPON, HLR_PAUSED, HLR_LERPOFF };
	int						stateTimer;
	int						transitionTime;

	idStr					entNames[MAX_ARROWS];
	idVec2					arrowPositions[MAX_ARROWS];
	float					arrowAngles[MAX_ARROWS];

	int						barYPositions[2];
	int						letterboxTimer;
	bool					letterboxLerpActive;


	int						cooldownTimer;

	int						IsEventVisibleToPlayer(idEntity *ent0, idEntity *ent1);
	enum					{VIS_DIRECTLOOK, VIS_ALMOSTDIRECTLOOK, VIS_BEHIND, VIS_INVALID};
	idVec3					GetEntityLookpoint(idEntity* ent);

	idStr					ParseName(idEntity* ent);

	void					InitializeArrows(highlightEntity_t hlEnt0, highlightEntity_t hlEnt1);

	idCameraView*			highlightCamera = nullptr;
	highlightEntity_t		highlightEntityInfos[2];
	void					SetHighlightEntityInfo(idEntity* ent0, idEntity *ent1);
	bool					cameraRotateActive;
	//idMover*				cameraMover;
	idAngles				cameraStartAngle;
	idAngles				cameraEndAngle;
	
	
	idVec3					FindViableCameraOffset(idEntity *ent);
	bool					cameraOffsetActive;
	idVec3					cameraOffsetPosition;
	idVec3					cameraStartOffsetPosition;

	bool					worldIsPaused;
};
