#ifndef __HH_SECURITYEYE_H
#define __HH_SECURITYEYE_H

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
extern const idEventDef EV_Notify;

/**********************************************************************

hhSecurityEyeBase

**********************************************************************/
class hhSecurityEyeBase : public idEntity {
	CLASS_PROTOTYPE( hhSecurityEyeBase );

public:
	void						Spawn();
	void						Save( idSaveGame *savefile ) const;
	void						Restore( idRestoreGame *savefile );
	void						SpawnEye();
	void						TransferArg(idDict &Args, const char *key);

protected:
	void						Event_Activate( idEntity* pActivator );
	void						Event_Enable();
	void						Event_Disable();

protected:
	idEntityPtr<idEntity>		m_pEye;
};
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build

/**********************************************************************

hhSecurityEye

**********************************************************************/
class hhSecurityEye : public idEntity {
	CLASS_PROTOTYPE( hhSecurityEye );
#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
		void					Event_Enable() {};
		void					Event_Disable() {};
		void					Event_Notify( idEntity* pActivator ) {};
		void					Event_Activate( idEntity* pActivator ) {};
#else
	public:
								hhSecurityEye() : m_pTrigger( NULL ) { }
		virtual					~hhSecurityEye();

		void					Spawn();
		void					Save( idSaveGame *savefile ) const;
		void					Restore( idRestoreGame *savefile );
		void					SetBase(idEntity *ent);
		
		virtual renderView_t*	GetRenderView();

	protected:
		void					Ticker();
		idDict*					GetTriggerArgs( idDict* pArgs );
		void					SpawnTrigger();
		void					SpawnBase();
		bool					InitPathList();

		virtual void			DormantBegin();
		virtual void			DormantEnd();

		void					EnableTrigger();
		void					DisableTrigger();

		idVec3					DetermineEntityPosition( idEntity* pEntity );
		bool					CanSeeTarget( idEntity* pEntity );
		virtual void			Killed( idEntity *pInflictor, idEntity *pAttacker, int iDamage, const idVec3 &Dir, int iLocation );

		void					StartScanning();
		idEntity*				VerifyPathScanNode( int& iIndex );

	protected:
		void					Event_Enable();
		void					Event_Disable();
		void					Event_Notify( idEntity* pActivator );
		void					Event_Activate( idEntity* pActivator );

	protected:
		enum States {
			StateIdle = 0,
			StateAreaScanning,
			StatePathScanning,
			StateTracking,		// Tracking state is now just a stopover state that means it was triggered.
								// It is needed so that the next time it's triggered, it will become inactive
			StateDead
		} state;

		void					EnterDeadState();
		void					EnterIdleState();
		void					EnterAreaScanningState();
		void					EnterPathScanningState();
		void					EnterTrackingState();

		void					UpdateRotation();
		void					LookAtLinear(idVec3 &pos);
		void					TurnTo(idAngles &ang);
		void					SetupRotationParms();
		idMat3					GetBaseAxis();

	protected:
		hhTriggerTripwire*		m_pTrigger;

		bool					m_bTriggerOnce;
		float					m_fCachedScanFovCos;
		int						m_iPVSArea;

		idAngles				m_StartAngles;
		idAngles				m_MaxLookAngles;
		idAngles				m_MinLookAngles;
		idAngles				m_LookAngles;
		bool					m_bPitchDirection;
		bool					m_bYawDirection;

		idList<idStr>			m_PathScanNodes;
		int						m_iPathScanNodeIndex;
		bool					m_bUsePathScan;
		float					m_fPathScanRate;

		idEntity				*m_pBase;
		idEntityPtr<idEntity>	m_Target;

		idInterpolate<float>	currentYaw;			// These are local angles, relative to base rotation
		idInterpolate<float>	currentPitch;		// These are local angles, relative to base rotation
		idAngles				m_RotationRate;
		float					m_lengthBeam;
		idVec3					m_offsetTrigger;
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
};

#endif