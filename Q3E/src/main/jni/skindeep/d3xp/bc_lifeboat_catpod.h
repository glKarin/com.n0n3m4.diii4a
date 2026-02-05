#include "Moveable.h"
#include "bc_lifeboat.h"


class idCatpod : public idLifeboat
{
public:
	CLASS_PROTOTYPE(idCatpod);

							idCatpod(void);
	virtual					~idCatpod(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );


	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

protected:

	virtual void			Think_Landed(void);
	virtual void			OnLanded(void);
	virtual void			OnTakeoff(void);

private:


	enum					{CTP_IDLE, CTP_PLAYERINSIDE, CTP_PRECAREPACKAGE, CTP_CAREPACKAGESPAWNED, CTP_READYFORTAKEOFF};
	int						catpodState;
	int						catpodTimer;

	int						guiUpdateTimer;
	int						GetCatsRescued();
	int						GetCatsAwaitingRescue();
	int						GetCatsAwaitingDeposit();
	bool					IsAllCatsRescued();

	int						DepositAvailableCats();

	idVec3					GetBestCarepackageSpot();
	void					SpawnCarepackage();

	idVec3					GetBestCatCubby();
	idVec3					FindCatOriginPoint(idVec3 _destination);


	bool					doFinalCatSequence;
	int						finalCatSequenceTimer;
	bool					finalCatSequenceActivated;



	//For the scripted functionality.
	bool					awaitingScriptCall;
	int						scriptCallTimer;

	void					EnterCatpod();

};
