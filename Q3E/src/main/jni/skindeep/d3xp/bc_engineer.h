#pragma once

#include "bc_gunner.h"
#include "ai/AI.h"

class idEngineerMonster : public idGunnerMonster
{
public:
	CLASS_PROTOTYPE(idEngineerMonster);
	
	void					Spawn(void);
	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );
	virtual void			Resurrect();

protected:

    virtual void			Think();
	virtual void			State_Idle();
    virtual void			State_Combat();


private:
	
	enum					{MINEPLACE_COOLDOWN, MINEPLACE_QUERYING};
	int						mineplaceStatus;

	void					UpdateLandmineCheck(); //Check if we need/want to place landmine.

	int						placedMines;	

	int						minecheckTimer;

	bool					DoMineClearanceCheck(idVec3 _position);


    //radardish
    //int                     nextRadardishPingTime;



};