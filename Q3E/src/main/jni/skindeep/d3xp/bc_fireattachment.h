#include "Misc.h"
#include "Entity.h"



class idFireAttachment : public idEntity
{
public:
	CLASS_PROTOTYPE(idFireAttachment);

                            idFireAttachment(void);
    virtual					~idFireAttachment(void);

	void					Spawn(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );

	void					AttachTo(idEntity *ent);
    

private:

    int                     lifetimer;
    int                     lifetimeMax;

    //int                     damageTimer;
    int                     damageTimerMax;

    idStr					damageDefname;

	idEntityPtr<idEntity>	attachOwner;
	idFuncEmitter			*particleEmitter = nullptr;

	idBounds				damageBounds;

	void					Extinguish();

	

};
//#pragma once