/*
================

AI_Medic.h

================
*/
#include "AI_Tactical.h"

#ifndef __AI_MEDIC__
#define __AI_MEDIC__

class rvAIMedic : public rvAITactical {
public:

	CLASS_PROTOTYPE( rvAIMedic );

	rvAIMedic ( void );

	void				InitSpawnArgsVariables			( void );
	void				Spawn							( void );
	void				Think							( void );
	void				Save							( idSaveGame *savefile ) const;
	void				Restore							( idRestoreGame *savefile );
	
	virtual void		Show							( void );

	virtual void		TalkTo							( idActor *actor );

	virtual void		GetDebugInfo					( debugInfoProc_t proc, void* userData );

	virtual bool		IsTethered						( void ) const;

	virtual void		OnStateThreadClear				( const char *statename, int flags );

	virtual bool		Pain							( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	bool				isTech;

protected:
	virtual void		OnStartMoving					( void );

private:

	idEntityPtr<idPlayer> patient;
	bool				healing;
	int					lastPatientCheckTime;
	bool				emergencyOverride;

	bool				noAutoHeal;
	bool				stationary;
	bool				silent;
	bool				healObeyTether;
	int					healAmt;
	float				patientRange;
	float				buddyRange;
	float				enemyRange;

	int					curHealValue;
	int					maxHealValue;
	int					minHealValue;
	int					healedAmount;
	int					maxPatientValue;

	int					healDebounceInterval;
	int					healDebounceTime;

	bool				healDisabled;
	bool				wasAware;
	bool				wasIgnoreEnemies;

	void				SetHealValues					( idPlayer* player );
 	
	void				TakePatient						( idPlayer* pPatient );
	void				DropPatient						( void );
 	bool				CheckTakePatient				( idPlayer* actor );
 	bool				SituationAllowsPatient			( void );
 	bool				AvailableToTakePatient			( void );

	stateResult_t		State_Medic						( const stateParms_t& parms );
	
	void				Event_EnableHeal				( void );
	void				Event_DisableHeal				( void );
	void				Event_EnableMovement			( void );
	void				Event_DisableMovement			( void );

	rvScriptFuncUtility	mPostHealScript;		// script to run after completing a heal

	CLASS_STATES_PROTOTYPE ( rvAIMedic );
};

#endif /* !__AI_MEDIC__ */
