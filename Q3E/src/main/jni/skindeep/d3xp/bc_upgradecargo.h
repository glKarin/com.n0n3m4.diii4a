#include "Misc.h"
#include "Entity.h"
#include "Item.h"



class idUpgradecargo: public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idUpgradecargo);
public:
							idUpgradecargo(void);
	virtual					~idUpgradecargo(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);
	
	virtual void			Think(void);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	//virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	//virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	virtual void			DoGenericImpulse(int index);

	int						GetState();

	bool					IsLocked();

	void					SetAvailable();
	bool					IsAvailable();
	
	void					SetDormant();
	bool					IsDormant();

	bool					SetInfo(const char *upgradeName);
	
		
	const idDeclEntityDef *upgradeDef = nullptr;

private:

	enum					{ UC_LOCKED, UC_AVAILABLE, UC_DORMANT };
	int						state;


	idEntity				*lockplateEnt = nullptr;


	
	
	

};
//#pragma once