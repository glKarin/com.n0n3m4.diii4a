#include "Misc.h"
#include "Entity.h"



class idWindowseal : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idWindowseal);


							idWindowseal();
	virtual					~idWindowseal(void);

	void					Spawn(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	virtual void			Think( void );

	virtual void			DoRepairTick(int amount);

	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					PostFTLReset();

private:

	idStr					fxActivate;

	void					Event_Activate( void );
	void					Event_PostSpawn(void);
	void					Event_Touch(idEntity* other, trace_t* trace);

	int						timer;

	int						autoResealTimer;
	bool					autoReseal;

	int						state;
	enum					
	{
		WINDOWSEAL_NONE,				//not sealed.
		WINDOWSEAL_WAITINGFORREPAIR,	//obsolete.
		WINDOWSEAL_MATERIALIZING,		//seal is materializing.
		WINDOWSEAL_DONE,				//completely sealed.
		WINDOWSEAL_LEVERACTIVE			//lever is active, waiting to be pulled.
	};

	idVec3					pushDir;
	int						pushTimer;

	void					DoGravityCheck();

	idEntity				*leverEnt = nullptr;

	void					Event_isSealed();
	void					Event_setSeal();
	void					Event_isWaitingForSeal();

	int						breachAnnounceTimer;


	idVec3					FindInteriorLerpPosition();


};
//#pragma once#pragma once
