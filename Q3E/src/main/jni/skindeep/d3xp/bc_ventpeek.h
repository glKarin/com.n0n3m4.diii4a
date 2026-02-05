#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"

const idEventDef EV_StopPeek("stopPeek", "d");

class idVentpeek : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idVentpeek);

							idVentpeek(void);
	virtual					~idVentpeek(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					GetAngleRestrictions(float &yaw_min, float &yaw_max, float &pitch_min, float &pitch_max);
	virtual float			GetRotateScale(void);
	float					GetTurningSoundThreshold(void);
	int						GetTurningSoundDelay(void);
	bool					GetLockListener(void);
	bool					CanFrobExit(void);
	idEntityPtr<idEntity>	peekEnt;

	void					StopPeek(bool fast = false);

	bool					forVentDoor;
	bool					forTelescope;

	idEntityPtr<idEntity>	ownerEnt;

	bool					Event_Activate(idEntity *activator);
	void					Event_SetVentpeekCanExit(bool toggle);

protected:
	int						harc;
	int						varc;
	float					rotateScale; // Context-dependent scaling for mouse sensitivity while looking through this peek

private:

	virtual void			Event_PostSpawn(void);
	
	

	int						peekTimer;
	
	int						peekState;
	enum					{ PEEK_NONE, PEEK_MOVECAMERATOWARD, PEEK_FADEOUT, PEEK_PEEKING };



	enum					{ PEEKTYPE_NORMAL, PEEKTYPE_CEILING, PEEKTYPE_GROUND };
	int						peekType;
	bool					bidirectional;
	bool					stopIfOpen;

	float					turningSoundThreshold;
	int						turningSoundDelay;
	bool					lockListener; // SW: Force game to place the player's ears at their location, rather than wherever they're peeking into

	bool					canFrobExit; // Allow the player to frob to exit, like a normal ventpeek
};

const idEventDef EV_SetPanAndZoom("setPanAndZoom", "d");
const idEventDef EV_LockOn("lockOn", "e");
const idEventDef EV_CrackLens("crackLens", "");
const idEventDef EV_ForceSetTarget("forceSetTarget", "e");
const idEventDef EV_SetVentpeekFOVLerp("setVentpeekFOVLerp", "fdd");
const idEventDef EV_StartFastForward("startFastForward", "");
const idEventDef EV_StopFastForward("stopFastForward", "");

// Extension of the ventpeek used for the telescope in vig_surveil
class idVentpeekTelescope : public idVentpeek
{


	public:
		CLASS_PROTOTYPE(idVentpeekTelescope);

		void Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
		void Restore(idRestoreGame *savefile);

		void Event_PostSpawn(void);
		bool HasNextTarget(void);
		void MoveToNextTarget(void);
		bool HasPreviousTarget(void);
		void MoveToPreviousTarget(void);
		void ZoomIn();
		void ZoomOut();
		float GetCurrentFOV();
		float GetCurrentBlur();
		bool CanTakePhoto();
		void SetPanAndZoom(bool);
		bool CanPanAndZoom();
		void LockOn(idEntity*);
		void TakePhoto();
		float GetApertureSize(void);
		float GetRotateScale(void) override;
		void Think();
		bool DoFrob(int index, idEntity* frobber);
		bool IsLensCracked();
		void CrackLens();
		int GetPhotoCount();

	private:
		class idLookTarget
		{
			public:
				idLookTarget() { entity = NULL; focusFunction = NULL; seenFunction = NULL; seenTime = 0; seen = false; }
				idLookTarget(const idEntity* ent, const function_t* focusFunc, const function_t* seenFunc) { entity = ent; focusFunction = focusFunc; seenFunction = seenFunc; seenTime = 0; seen = false; }

				const idEntity* entity;
				const function_t* focusFunction;
				const function_t* seenFunction;
				int seenTime;
				bool seen;

				ID_INLINE bool operator==(const idLookTarget& other)
				{
					return (
						this->entity == other.entity && 
						this->focusFunction == other.focusFunction && 
						this->seenFunction == other.seenFunction &&
						this->seenTime == other.seenTime &&
						this->seen == other.seen
					);
				};
		};

		

		const float DEFAULT_FOV_STEP = 1;
		const int MAX_LOOK_TARGETS = 16;
		const int ZOOM_TRANSITION_MAXTIME = 1000; // How long it takes for a zoom transition to complete (in ms)
		const float ZOOM_FOV_PERCENT_CHANGE = 0.7; // How much to change the FOV during a zoom transition. 1.0 = 100% of the current FOV 
		const int CAMERA_CLOSE_SHUTTER_TIMER = 50;
		const int CAMERA_OPEN_SHUTTER_TIMER = 50;
		const int CAMERA_WARMUP_TIMER = 250;
		const int LOCKON_LERP_TIME = 500;
		const int MAX_FASTFORWARD_SPEEDUPS = 5;

		void GetParametersFromTarget(idEntityPtr<idEntity> target);
		void HandleZoomTransitions(void);
		void HandleLookTargets(void);
		void HandleCamera(void);
		void HandleFastForward(void);
		bool IsFocusedOn(const idEntity* target);
		bool CanSee(const idEntity* target);
		void BuildViewFrustum(idPlane(&viewFrustum)[4]);
		void ForceSetTarget(idEntity* ent);
		void SetVentpeekFOVLerp(float endFov, int timeMS, int lerpType);
		void StartFastForward();
		void StopFastForward();
		void AutoAdvance();

		
		float maxFov; // How zoomed out can we be at this point?
		float minFov; // How zoomed in can we be at this point?
		float fovStep; // How much does the zoom level change when the player scrolls the mouse wheel?
		float currentFov; // How zoomed in are we at this exact moment?

		float fovLerpStart;
		float fovLerpEnd;
		float fovLerpStartTime;
		float fovLerpEndTime;
		int fovLerpType;
		enum { LERPTYPE_LINEAR, LERPTYPE_CUBIC_EASE_IN, LERPTYPE_CUBIC_EASE_OUT, LERPTYPE_CUBIC_EASE_INOUT };
		bool fovIsLerping;
		
		bool canUseCamera;
		int cameraTimer;
		int cameraState;
		enum { CAMERA_IDLE, CAMERA_WARMUP, CAMERA_CLOSE_SHUTTER, CAMERA_OPEN_SHUTTER };
		int photoCount; // Number of photos the player has fictionally 'taken' (images are not actually stored)

		bool canPanAndZoom; // Prevent the player from making panning/zooming inputs (camera may still move with lerps, etc)
		bool isLockedOn;
		const function_t* lockOnFunction; // Script function to call if we take a photo while locked-on (this should end the sequence, effectively)
		
		idEntityPtr<idEntity> nextTarget; // If we zoom all the way in, where will our viewpoint go next?
		idEntityPtr<idEntity> previousTarget; // If we zoom all the way out, where will our viewpoint go next?

		int zoomTransitionState;
		int zoomTransitionTimer;
		int zoomTransitionTimerStart;
		enum { ZOOM_NONE, ZOOMIN_BLURIN, ZOOMIN_BLUROUT, ZOOMOUT_BLURIN, ZOOMOUT_BLUROUT };

		idList<idLookTarget> lookTargets;
		idLookTarget* currentFocusTarget = nullptr;
		int focusTime;

		enum { FOCUS_STYLE_SIMPLE, FOCUS_STYLE_LOOSE, FOCUS_STYLE_PRECISE };

		bool isCracked;
		bool isFastForward; // Auto-advances the zooming-in sequence until stopped
		int fastForwardIteration;
		int fastForwardWait; // How long after entering a target should we auto-advance (this can happen before the ZOOMIN_BLUROUT has finished!)
};
//#pragma once