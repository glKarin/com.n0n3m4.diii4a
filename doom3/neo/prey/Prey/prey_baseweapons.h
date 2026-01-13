#ifndef __HH_PREY_BASE_WEAPONS_H
#define __HH_PREY_BASE_WEAPONS_H

class hhPlayer;

extern const idEventDef EV_Weapon_Aside;
extern const idEventDef EV_PlayAnimWhenReady;
extern const idEventDef EV_Weapon_Feedback;
extern const idEventDef EV_PlayCycle;

//HUMANHEAD: aob
extern const idEventDef EV_Weapon_FireAltProjectiles;
extern const idEventDef EV_Weapon_FireProjectiles;
//HUMANHEAD END

/***********************************************************************

  hhWeapon
	
	Base class for common weapon data and methods
***********************************************************************/
class hhWeapon: public hhAnimatedEntity {
	CLASS_PROTOTYPE( hhWeapon );

	//HUMANHEAD: aob - used to allow physics object to have access to hhWeaponBases' private methods and vars
	friend					hhPhysics_StaticWeapon;
	//HUMANHEAD END

	public:
							hhWeapon();
		virtual				~hhWeapon();
		void				Spawn();
		virtual void		Think();

		virtual void		Clear();
		virtual void		GetWeaponDef( const char *objectname, int ammoinclip ) { GetWeaponDef(objectname); }
		virtual void		GetWeaponDef( const char *objectname );
		virtual void		SetOwner( idPlayer *_owner );

		virtual void		RestoreGUI( const char* guiKey, idUserInterface** gui );

		virtual void		EnterCinematic();
		virtual void		ExitCinematic();
		void				NetCatchup();

		virtual int			GetClipBits(void) const;

		// save games
		void				Save( idSaveGame *savefile ) const;					// archives object for save game file
		void				Restore( idRestoreGame *savefile );	

		// GUIs
		const char *		Icon( void ) const;
		virtual void		UpdateGUI();

		virtual void		SetModel( const char *modelname );
		virtual void		FreeModelDef();
		void				GetJointHandle( const char* jointName, weaponJointHandle_t& handle );
		bool				GetJointWorldTransform( const char* jointName, idVec3 &offset, idMat3 &axis );
		bool				GetJointWorldTransform( const weaponJointHandle_t& handle, idVec3 &offset, idMat3 &axis, bool muzzleOnly = false ); //rww - added muzzleOnly parameter for mp
		void				SetPushVelocity( const idVec3 &pushVelocity );
		virtual void		SetSkin( const idDeclSkin *skin );
		virtual void		SetShaderParm( int parmnum, float value );

		bool				IsLinked( void );
		bool				IsWorldModelReady( void );

		// State control/player interface
		virtual void		Raise( void );
		virtual void		PutAway( void );
		virtual void		Reload( void );
		//HUMANHEAD: aob - put bodies here
		void				LowerWeapon( void ) {}
		void				RaiseWeapon( void ) {}
		//HUMANHEAD END
		virtual void		HideWeapon( void );
		void				ShowWeapon( void );
		void				HideWorldModel( void );
		void				ShowWorldModel( void );
		void				OwnerDied( void );
		void				BeginAttack( void );
		virtual void		EndAttack( void );
		virtual bool		IsReady( void ) const;
		virtual bool		IsReloading( void ) const;
		bool				IsChangable( void ) const;
		virtual bool		IsHolstered( void ) const;
		bool				IsLowered( void ) const;	// nla
		bool				IsRising( void ) const;		// nla
		bool				IsAside( void ) const;		// nla
		void				PutAside( void );			// nla
		void				PutUpright( void );			// nla
		virtual	int			GetAnimDoneTime( void ) const { return( animDoneTime ); }	// nla

		bool				ShowCrosshair( void ) const;
		idEntity*			DropItem( const idVec3 &velocity, int activateDelay, int removeDelay, bool died );
		bool				CanDrop( void ) const;
		void				WeaponStolen( void ) {}

		// Script state management
		virtual bool		ShouldConstructScriptObjectAtSpawn( void ) const;
		virtual idThread	*ConstructScriptObject( void );
		virtual void		DeconstructScriptObject( void );
		void				SetState( const char *statename, int blendFrames );
		void				UpdateScript( void );

		// Visual presentation
		virtual // HUMANHEAD:  made virtual
		void				PresentWeapon( bool showViewModel );
		int					GetZoomFov();
		void				GetWeaponAngleOffsets( int *average, float *scale, float *max );
		void				GetWeaponTimeOffsets( float *time, float *scale );

		//Ammo
		ammo_t				GetAmmoType( void ) const;
		int					AmmoAvailable( void ) const;
		int					AmmoInClip( void ) const;
		void				ResetAmmoClip( void ) {}
		int					ClipSize( void ) const;
		int					AmmoRequired( void ) const;
		void				UseAmmo();
		int					LowAmmo();


		//HUMANHEAD: altAmmo
		ammo_t				GetAltAmmoType( void ) const;
		int					AltAmmoAvailable( void ) const;
		int					AltAmmoInClip( void ) const;
		int					AltClipSize( void ) const;
		int					LowAltAmmo() const;
		int					AltAmmoRequired( void ) const;
		void				UseAltAmmo();
		int					LowAltAmmo();
		//HUMANHEAD END

		bool				GetAltMode() const;

		virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );
		virtual void		ClientPredictionThink( void );

		// Visual presentation
		void				InitWorldModel( const idDict *dict );
		void				MuzzleRise( idVec3 &origin, idMat3 &axis );

		//HUMANHEAD: aob
		virtual void		ParseDef( const char* objectname );
		virtual void		InitScriptObject( const char* objectType );
		static idEntity*	SpawnWorldModel( const char* worldModelDict, idActor* _owner );
		virtual void		GetMasterDefaultPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const;
		virtual	hhPlayer*	GetOwner() const { return owner.GetEntity(); }
		virtual void		UpdateCrosshairs( bool &crosshair, bool &targeting );

		virtual int			GetHandedness() const { return( handedness ); }

		virtual void		BeginAltAttack( void );
		void				EndAltAttack( void );

		int					GetStatus() { return( status ); };
		const idDict *		GetDict() { return( dict ); };

		static const idDict*	GetDict( const char* objectname );
		static const idDict*	GetFireInfoDict( const char* objectname );
		static const idDict*	GetAltFireInfoDict( const char* objectname );

		int					GetKickEndTime() const { return kick_endtime; }
		void				SetKickEndTime( int endTime ) { kick_endtime = endTime; }
		idVec3				GetMuzzlePosition() const { return fireController->GetMuzzlePosition(); }
		idVec3				GetAltMuzzlePosition() const { return altFireController->GetMuzzlePosition(); }

		void				SnapDown();
		void				SnapUp();

		void				PrecomputeTraceInfo();
		const trace_t&		GetEyeTraceInfo() const { return eyeTraceInfo; }

		void				SetViewAnglesSensitivity( float fov );

		void				UpdateNozzleFx( void );
		//HUMANEHAD END

		void				CheckDeferredProjectiles(void); //HUMANHEAD rww

		virtual void		FillDebugVars( idDict *args, int page );	//HUMANHEAD bjk
		
	protected:
		virtual bool		GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );

		void				LaunchProjectiles( hhWeaponFireController* controller );

		//needed for overridding
		//FIXME: would like to make these templates just in case we change types.
		ID_INLINE virtual hhWeaponFireController* CreateFireController();
		ID_INLINE virtual hhWeaponFireController* CreateAltFireController();
		//HUMANHEAD END
	
	protected:
		//HUMANHEAD: aob
		void				Event_PlayAnimWhenReady( const char* animName );
		void 				Event_Raise();	// nla
		void				Event_Weapon_Aside();
		void				Event_Weapon_PuttingAside();
		void				Event_Weapon_Uprighting();
	
		//Called from frame command
		void				Event_SpawnFXAlongBone( idList<idStr>* fxParms );

		void				Event_WeaponOutOfAltAmmo();
		void				Event_AddToAltClip( int amount );
		void				Event_AltAmmoInClip();
		void				Event_AltAmmoAvailable();
		void				Event_AltClipSize();
		void				Event_HasAltAmmo();

		void				Event_HasAmmo();

		virtual void		Event_FireAltProjectiles();
		virtual void		Event_FireProjectiles();

		void				Event_EjectAltBrass( void );

		void				Event_IsAnimPlaying( const char *animname ); // CJR

		void				Event_GetFireDelay();
		void				Event_GetAltFireDelay();
		void				Event_GetSpread();
		void				Event_GetAltSpread();
		void				Event_GetString(const char *key);
		void				Event_GetAltString(const char *key);

		void				Event_SetViewAnglesSensitivity( float fov);
		void				Event_GetOwner( void );
		void				Event_UseAmmo( int amount );
		void				Event_UseAltAmmo( int amount );
		//HUMANHEAD END

		//idWeapon events
		// script events
		void				Event_WeaponState( const char *statename, int blendFrames );
		void				Event_SetWeaponStatus( float newStatus );
		void				Event_WeaponReady( void );
		void				Event_WeaponOutOfAmmo( void );
		void				Event_WeaponReloading( void );
		void				Event_WeaponHolstered( void );
		void				Event_WeaponRising( void );
		void				Event_WeaponLowering( void );
		void				Event_AddToClip( int amount );
		void				Event_AmmoInClip( void );
		void				Event_AmmoAvailable( void );
		void				Event_ClipSize( void );
		void				Event_PlayAnim( int channel, const char *animname );
		void				Event_PlayCycle( int channel, const char *animname );
		void				Event_AnimDone( int channel, int blendFrames );
		void				Event_WaitFrame( void );
		void				Event_Next( void );
		void				Event_SetSkin( const char *skinname );
		void				Event_Flashlight( int enable );
		void				Event_EjectBrass( void );
		void				Event_HideWeapon( void );
		void				Event_ShowWeapon( void );

	protected:
		hhWeaponFireController*	fireController;
		hhWeaponFireController*	altFireController;
		trace_t				eyeTraceInfo;

		idVec3				cameraShakeOffset;
		hhPhysics_StaticWeapon	physicsObj;

		// 0 - no hands, 1 - right, 2 - left, 3 - both
		int					handedness;  // nla - For determining which hands can be up with which weapons

		idScriptBool		WEAPON_ALTATTACK;
		idScriptBool		WEAPON_ASIDEWEAPON;		// nla
		idScriptBool		WEAPON_ALTMODE;			// for moded weapons like rifle, whether in alt mode

		//idWeapon vars
		// script control
		idScriptBool		WEAPON_ATTACK;
		idScriptBool		WEAPON_RELOAD;
		idScriptBool		WEAPON_RAISEWEAPON;
		idScriptBool		WEAPON_LOWERWEAPON;
		idScriptFloat		WEAPON_NEXTATTACK; //rww
		weaponStatus_t		status;
		idThread *			thread;
		idStr				state;
		idStr				idealState;
		int					animBlendFrames;
		int					animDoneTime;

		//HUMANHEAD: aob - made hhPlayer
		idEntityPtr<hhPlayer> owner;
		idEntityPtr<hhAnimatedEntity> worldModel;

		// weapon kick
		int					kick_endtime;

		idVec3				pushVelocity;

		// weapon definition
		const idDeclEntityDef *	weaponDef;
		const idDict *		dict;

		idStr				icon;

		bool				lightOn;

		// zoom
	    int					zoomFov;			// variable zoom fov per weapon

		// weighting for viewmodel angles
		int					weaponAngleOffsetAverages;
		float				weaponAngleOffsetScale;
		float				weaponAngleOffsetMax;
		float				weaponOffsetTime;
		float				weaponOffsetScale;

		// Nozzle FX
		bool					nozzleFx;
		weaponJointHandle_t		nozzleJointHandle;
		renderLight_t			nozzleGlow;			// nozzle light
		int						nozzleGlowHandle;	// handle for nozzle light
		idVec3					nozzleGlowColor;	// color of the nozzle glow
		const idMaterial *		nozzleGlowShader;	// shader for glow light
		float					nozzleGlowRadius;	// radius of glow light
		idVec3					nozzleGlowOffset;	// offset from bound bone
		
};

/*
================
hhWeapon::IsLinked
================
*/
ID_INLINE bool hhWeapon::IsLinked( void ) {
	return scriptObject.HasObject();
}

/*
================
hhWeapon::IsWorldModelReady
================
*/
ID_INLINE bool hhWeapon::IsWorldModelReady( void ) {
	return worldModel.IsValid();
}

/*
================
hhWeapon::CreateFireController
================
*/
ID_INLINE hhWeaponFireController* hhWeapon::CreateFireController() {
	return new hhWeaponFireController;
}

/*
================
hhWeapon::CreateAltFireController
================
*/
ID_INLINE hhWeaponFireController* hhWeapon::CreateAltFireController() {
	return CreateFireController();
}

#endif
