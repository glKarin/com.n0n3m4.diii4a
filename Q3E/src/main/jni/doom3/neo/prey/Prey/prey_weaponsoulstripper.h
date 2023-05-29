#ifndef __HH_WEAPON_SOULSTRIPPER_H
#define __HH_WEAPON_SOULSTRIPPER_H


/***********************************************************************

  hhSoulStripperAltFireController
	
***********************************************************************/
class hhSoulStripperAltFireController : public hhWeaponFireController {
	CLASS_PROTOTYPE(hhSoulStripperAltFireController);
public:
	virtual void		LaunchProjectiles( const idVec3& launchOrigin, const idMat3& aimAxis, const idVec3& pushVelocity, idEntity* projOwner );
};


/***********************************************************************

  hhBeamBasedFireController
	
***********************************************************************/
class hhBeamBasedFireController : public hhWeaponFireController {
	CLASS_PROTOTYPE(hhBeamBasedFireController);
public:
	virtual				~hhBeamBasedFireController();
	virtual void		Init( const idDict* viewDict, hhWeapon* self, hhPlayer* owner );
	virtual void		LaunchProjectiles( const idVec3& launchOrigin, const idMat3& aimAxis, const idVec3& pushVelocity, idEntity* projOwner );

	void				KillBeam();
	void				Think();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

protected:
	idEntityPtr<hhBeamSystem>		shotbeam;
	int								projTime;
	idEntityPtr<hhEntityFx>			impactFx;
};

/***********************************************************************

  hhSunbeamFireController
	
***********************************************************************/
class hhSunbeamFireController : public hhBeamBasedFireController {
	CLASS_PROTOTYPE(hhSunbeamFireController);
public:
	virtual void		LaunchProjectiles( const idVec3& launchOrigin, const idMat3& aimAxis, const idVec3& pushVelocity, idEntity* projOwner );
	void				Think();
};

/***********************************************************************

  hhPlasmaFireController
	
***********************************************************************/
class hhPlasmaFireController : public hhWeaponFireController {
	CLASS_PROTOTYPE(hhPlasmaFireController);
public:
	virtual bool		LaunchProjectiles( const idVec3& pushVelocity );
};

/***********************************************************************

  hhWeaponSoulStripper
	
***********************************************************************/
#define		MAX_SOUL_AMMO	3

typedef enum 
{
	CAP_NONE = 0,
	CAP_ENTITY,
	CAP_INTAKE
} captureType_t;

class hhWeaponSoulStripper : public hhWeapon {
	CLASS_PROTOTYPE( hhWeaponSoulStripper );

	public:
						hhWeaponSoulStripper();
		void			Spawn();
		virtual 		~hhWeaponSoulStripper();

		virtual void	ParseDef( const char* objectname );

		virtual void	UpdateGUI();

		virtual void	Show();
		virtual void	Hide();

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

		//rww - network friendliness
		virtual void	ClientUpdateFC(int fcType, int fcDefNumber);
		virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );

		virtual int		GetClipBits(void) const;

		// beam light
		renderLight_t	beamLight;
		int				beamLightHandle;

		bool			GiveEnergy( const char *energyType, bool fill );

		void			CheckCans(void); //rww
	protected:
		virtual void	Ticker();
		char		 	GetAnimPostfix();
		virtual void	PresentWeapon( bool showViewModel );
		virtual hhWeaponFireController*	CreateAltFireController();

		void			UpdateBeam( idVec3 start, bool struckEntity );
		void			UpdateCanisterBeam( hhBeamSystem *system, const char *top, const char *bottom );

		int				CaptureEnergy( trace_t &results );

		void			Event_PostSpawn();
		void			Event_GetAnimPostFix();
		void			Event_Leech();
		void			Event_EndLeech();

		void			Event_PlayAnim( int channel, const char *animname );
		void			Event_PlayCycle( int channel, const char *animname );

		void			SpawnCans();
		void			DestroyCans();

		hhBeamSystem	*SpawnCanisterBeam( const char *bottom, const char *top, const idStr &beamName );
		idEntity		*SpawnCanisterSprite( const char *attach, const char *spriteName );
		hhEntityFx		*SpawnCanisterFx( const char *attach, const idStr &name );

		void			Event_LightFadeIn( float fadetime );
		void			Event_LightFadeOut( float fadetime );
		void			Event_LightFade( float fadeOut, float pause, float fadeIn );
		void			Event_GetFireFunction();
		void			Event_KillBeam();

	protected:
		idEntityPtr<hhBeamSystem>		beam;

		// Beams for each canister
		idEntityPtr<hhBeamSystem>		beamCanA1; // bottom to center
		idEntityPtr<hhBeamSystem>		beamCanB1; // bottom to center
		idEntityPtr<hhBeamSystem>		beamCanC1; // bottom to center
		idEntityPtr<hhBeamSystem>		beamCanA2; // top to center
		idEntityPtr<hhBeamSystem>		beamCanB2; // top to center
		idEntityPtr<hhBeamSystem>		beamCanC2; // top to center
		idEntityPtr<hhBeamSystem>		beamCanA3; // top to bottom
		idEntityPtr<hhBeamSystem>		beamCanB3; // top to bottom
		idEntityPtr<hhBeamSystem>		beamCanC3; // top to bottom

		idEntityPtr<hhEntityFx>			fxCanA;
		idEntityPtr<hhEntityFx>			fxCanB;
		idEntityPtr<hhEntityFx>			fxCanC;

		float				beamLength;
		float				maxBeamLength;

		hhEnergyNode		*targetNode; // Target struck by the beam
		idVec3				targetOffset; // Offset from the origin of target
		int					targetTime;

		idStr				lastAnim; // For smoothing the animation when picking up an ammo_soul -mdl
		char				lastCanState;

		bool				empty;

		int					fcDeclNum; //rww - needed for networking (and a good idea to have around anyway)

		idStr				beam_canTop;
		idStr				beam_canBot;
		idStr				beam_canGlow;
		idStr				fx_can;
		bool				netInitialized; //rww
		bool				cansValid; //rww
};

#endif