#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
#ifndef __HH_TRIGGER_TRIPWIRE_H
#define __HH_TRIGGER_TRIPWIRE_H

class hhTriggerTripwire : public idEntity {
	CLASS_PROTOTYPE( hhTriggerTripwire );

public:
					~hhTriggerTripwire();
	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	void			ToggleBeam( bool bOn );
	void			SetOwner( idEntity* pOwner );
	const idEntity*	GetOwner() const { return m_pOwner; }

protected:
	virtual void	NotifyTargets( idEntity* pActivator );

	void			SetTriggerClasses( idList<idStr>& List );
	virtual bool	CheckTriggerClass( idEntity* pActivator );
	void			GetTriggerClasses( idDict& Args );

	void			CallFunctions( idEntity* activator );

protected:
	virtual void	Ticker();
	void			Event_Enable();	 
	void			Event_Disable();
	void			Event_Activate( idEntity *activator );
	void			Event_Deactivate();

public:
	float			m_fMaxBeamDistance;

protected:
	idEntity*			m_pOwner;
	hhFuncParmAccessor	m_CallFunc;
	hhFuncParmAccessor	m_CallFuncRef;
	hhFuncParmAccessor	m_CallFuncRefActivator;

	idEntityPtr<hhBeamSystem>	m_pBeamEntity;

	idList<idStr>	m_TriggerClasses;
	triggerBehavior_t m_TriggerBehavior;
};

#endif
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build