#ifndef __PREY_TRIGGER_H
#define __PREY_TRIGGER_H

class hhFuncParmAccessor : public idClass {
	CLASS_PROTOTYPE( hhFuncParmAccessor );

public:
							hhFuncParmAccessor();
	explicit				hhFuncParmAccessor( const hhFuncParmAccessor* accessor );
	explicit				hhFuncParmAccessor( const hhFuncParmAccessor& accessor );

	void					SetInfo( const char* returnKey, const function_t* func, const idList<idStr>& parms );

	const char*				GetFunctionName() const;
	const function_t*		GetFunction() const;
	const char*				GetReturnKey() const;
	idTypeDef*				GetReturnType() const;
	const char*				GetParm( int index ) const;
	idTypeDef*				GetParmType( int index ) const;
	const idList<idStr>&	GetParms() const;
	idList<idStr>&			GetParms();

	void					Verify();

	static function_t*		FindFunction( const char* funcname );
	void					CallFunction( idDict& returnDict );
	void					ParseFunctionKeyValue( const char* value );
	void					ParseFunctionKeyValue( idList<idStr>& valueList );

public:
	void					SetFunction( const function_t* func );
	void					SetParms( const idList<idStr>& parms );
	void					SetReturnKey( const char* key );

	void					SetParm_Entity( const idEntity *ent, int index );
	void					SetParm_String( const char *str, int index );

	void					InsertParm_String( const char *text, int index );
	void					InsertParm_Float( float value, int index );
	void					InsertParm_Int( int value, int index );
	void					InsertParm_Vector( const idVec3 &vec, int index );
	void					InsertParm_Entity( const idEntity *ent, int index );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

protected:
	void					InsertParm( const char *text, int index );

protected:
	idList<idStr>			parms;
	idStr					returnKey;
	const function_t*		function;
};

enum triggerBehavior_t {
	TB_PLAYER_ONLY,
	TB_FRIENDLIES_ONLY,
	TB_MONSTERS_ONLY,
	TB_PLAYER_MONSTERS_FRIENDLIES,
	TB_SPECIFIC_ENTITIES,
	TB_ANY,
	NUM_BEHAVIORS
};

class hhTrigger : public idEntity
{
public:
	CLASS_PROTOTYPE( hhTrigger );

	void			Spawn( void );

	virtual void	Activate( idEntity *activator );
	virtual void	TriggerAction( idEntity *activator );
	virtual void	UnTriggerAction();
	bool			IsEncroaching( const idEntity* entity );
	bool			IsEncroached();

	void			SetTriggerClasses(idList<idStr>& list);
	virtual bool	CheckTriggerClass(idEntity* activator);
	virtual bool	CheckUnTriggerClass(idEntity* activator);
	void			GetTriggerClasses(idDict& Args);

	virtual void	Enable();
	virtual void	Disable();

	const bool		IsActive() const { return bActive; }
	const bool		IsEnabled() const { return bEnabled; }

	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

protected:
	void			CallFunctions( idEntity *activator );
	void			UncallFunctions( idEntity *activator );

	virtual void	Event_Enable();
	virtual void	Event_Disable();
	void			Event_Activate( idEntity *activator );
	void			Event_Deactivate();
	virtual void	Event_Touch( idEntity *other, trace_t *trace );
	void			Event_TriggerAction( idEntity *activator );
	virtual void	Event_Retrigger( idEntity *activator );
	void			Event_PollForUntouch();
	virtual void	Event_UnTriggerAction();
	void			Event_PostSpawn();

public:
	bool			bActive;						// Whether the trigger is currently touched
	bool			bEnabled;						// whether the trigger is currently triggerable
	float			untouchGranularity;				// Seconds between untouch polls
	float			wait;							// Seconds before trigger is triggerable again
	float			random;							// Random wait variance
	float			delay;							// Seconds to delay the triggering
	float			randomDelay;
	float			refire;							// Seconds before refiring trigger
	int				nextTriggerTime;				// Time in ms when the trigger will be triggerable again
	bool			alwaysTrigger;					// Do we want to always trigger, instead of just once
	bool			isSimpleBox;					// Simple boxes can use cheaper bounds test for encroach checks
	bool			bNoVehicles;

	hhFuncParmAccessor	funcInfo;
	hhFuncParmAccessor	unfuncInfo;
	hhFuncParmAccessor	funcRefInfo;
	hhFuncParmAccessor	unfuncRefInfo;
	hhFuncParmAccessor	funcRefActivatorInfo;
	hhFuncParmAccessor	unfuncRefActivatorInfo;

	idStr			requires;						// item that the trigger requires of the player
	int				removeItem;						// whether to remove the required item from players inventory
	bool			noTouch;						// whether to disregard touch as triggering mechanism
	bool			initiallyEnabled;				// whether the trigger is initially triggerable
	bool			bUntrigger;						// whether to resend the trigger message when untriggered
	idEntityPtr<idEntity>	unTriggerActivator;			// Used solely for sending to Activate() when bUntrigger is set

	idList<idStr>	TriggerClasses;

	triggerBehavior_t triggerBehavior;
};

class hhDamageTrigger : public hhTrigger
{
public:
	CLASS_PROTOTYPE( hhDamageTrigger );

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location);
	virtual void	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

protected:
	virtual void	Event_Enable();
	void			Event_Disable();

	hhFuncParmAccessor	funcRefInfoDamage;
};


class hhTriggerPain : public hhTrigger {
public :
	CLASS_PROTOTYPE( hhTriggerPain );

	void			Spawn( void );
	virtual void	TriggerAction(idEntity *activator);

	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

protected:
	void			Event_Activate( idEntity *activator );
	void			Event_DamageBox( idEntity *activator );
	
protected:
	bool 	applyImpulse;
};


class hhTriggerEnabler : public hhTrigger {
public :
	CLASS_PROTOTYPE( hhTriggerEnabler );

	void			Spawn( void );
	virtual void	TriggerAction(idEntity *activator);
	virtual void	UnTriggerAction(void);
};


class hhTriggerSight : public hhTrigger {
public:
	CLASS_PROTOTYPE( hhTriggerSight );

	void Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

protected:
	virtual void	Think( void );
	virtual void	Event_Enable( void );
	void			Event_Disable( void );

	int						pvsArea;
};

/*
===============================================================================

  Trigger which fires targets after being activated a specific number of times.

===============================================================================
*/
class hhTrigger_Count : public hhTrigger {
public:
	CLASS_PROTOTYPE( hhTrigger_Count );

	// save games
	void				Save( idSaveGame *savefile ) const;					// archives object for save game file
	void				Restore( idRestoreGame *savefile );					// unarchives object from save game file

	void				Spawn( void );

	virtual void		Activate( idEntity *activator );
	virtual void		TriggerAction( idEntity *activator );

protected:
	int					goal;
	int					count;
	float				delay;
};

class hhTrigger_Event : public hhTrigger {
public:
	CLASS_PROTOTYPE( hhTrigger_Event );

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const { }
	void				Restore( idRestoreGame *savefile ) { Spawn(); }

protected:
	virtual void		ActivateTargets( idEntity *activator ) const;

	const idEventDef*	FindEventDef( const char* eventDefName ) const;
	static function_t*	FindFunction( const char* funcname );

protected:
	const idEventDef*	eventDef;
};

class hhMineTrigger : public hhTrigger {
	CLASS_PROTOTYPE( hhMineTrigger );

public:
	virtual bool	CheckTriggerClass(idEntity* activator);
};

#endif
