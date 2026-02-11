#include "Misc.h"
#include "Entity.h"
#include "Item.h"



class idCatcage: public idEntity
{
public:
	CLASS_PROTOTYPE(idCatcage);
public:
							idCatcage(void);
	virtual					~idCatcage(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	
	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual bool			DoFrobHold(int index = 0, idEntity * frobber = NULL);

	//virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	bool					IsOpened();
	bool					IsOpenedCompletely();

	virtual void			DoHack(); //for the hackgrenade.

	void					ReleaseCat();

	void					Event_SetCallForHelp(int toggle);

	void					CatLookAtPlayer();

private:

	enum					{ CATCAGE_IDLE, CATCAGE_OPENING, CATCAGE_COMPLETED };
	int						state;
	int						timer;

	idFuncEmitter			*soundwaves = nullptr;

	idEntityPtr<idEntity>	catPtr;

	idEntity				*meowOverlay = nullptr;
	int						overlayTimer;
	enum					{MEOWSTATE_TRANSITIONON, MEOWSTATE_IDLE, MEOWSTATE_TRANSITIONOFF, MEOWSTATE_DONE};
	int						overlayState;

	int						helpme_VO_timer;

	void					SpawnGoodie();

	bool					awaitingPlayerResponse;
	bool					callForHelp; // SW: spawnarg flag that lets us selectively silence cat cages (e.g. for vignettes)
	
	idEntity*				wordbubble;
	enum					{WBS_OFF, WBS_TRANSITIONON, WBS_ON, WBS_TRANSITIONOFF};
	int						wordbubbleState;
	int						wordbubbleTimer;
	void					SetWordbubble(idStr text);
	
	idVec3					wordbubbleBasePosition;
	int						wordbubbleJiggleTimer;

	idAnimated*				catprisonerModel;
	idAnimated*				catcage_animated;
	

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	int						helpmeRadius;

	idStr					DoAskItemdefCheck();

	//BC 2-23-2025: fallback arrow for key in room with no location.
	idEntity*				arrowProp = nullptr;
	int						arrowTimer;
	bool					arrowActive;
};
