#include "Misc.h"
#include "Entity.h"



class idDamageJet : public idEntity
{
public:
	CLASS_PROTOTYPE(idDamageJet);

                            idDamageJet(void);
    virtual					~idDamageJet(void);

	void					Spawn(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );
	//virtual void			Present(void);
    

private:

    int                     lifetimer;
    int                     lifetimeMax;
	bool					noLifetime; // SW: ignore lifetime, run forever (for vignette work)

    int                     damageTimer;
    int                     damageTimerMax;

    float                   range;
	float                   enemyRange;

    idStr					damageDefname;

    idVec3                  direction;
	bool					allowLerpRotate; // SW: jet won't rotate to face targets if this is disabled (again, for vignette work)

    int                     ownerIndex;

	idFuncEmitter			*emitterParticles = nullptr;

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	enum 
	{
		JET_DORMANT,
		JET_LERPING,
		JET_LERPDONE
	};
	int						jetLerpstate;
	int						jetLerpTimer;
	idVec3					jetInitialTarget;
	idVec3					jetDestinationTarget;

	idAngles				jetCurrentAngle;
	   

	bool					hasCloud;
	bool					hasSpark;
	//idStr					cloudName;
	const idDeclEntityDef	*cloudDef = nullptr;
	int						cloudTimer;

	idAngles				FindSafeInitialAngle();


	//TODO: move this to gamelocal so anyone can use it.
	void					SpawnCloud(const idDeclEntityDef	*cloudEntityDef, idVec3 pos);

	void					Event_SetAngles(idAngles const& angles);
	

};
//#pragma once