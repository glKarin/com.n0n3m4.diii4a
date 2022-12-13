#ifndef __HH_WEAPON_FIRE_CONTROLLER_H
#define __HH_WEAPON_FIRE_CONTROLLER_H

struct weaponJointHandle_t {
	jointHandle_t	view;
	jointHandle_t	world;

					weaponJointHandle_t() { Clear(); }
	void			Clear() { view = INVALID_JOINT; world = INVALID_JOINT; }
};

/***********************************************************************

  hhWeaponFireController
	
	Base class that manages the firing projectiles.
***********************************************************************/
class hhWeaponFireController : public hhFireController {
	CLASS_PROTOTYPE(hhWeaponFireController)

public:
	virtual void		Clear();
	virtual void		Init( const idDict* viewDict, hhWeapon* self, hhPlayer* owner );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual bool		LaunchProjectiles( const idVec3& pushVelocity );
	virtual idMat3		DetermineAimAxis( const idVec3& muzzlePos, const idMat3& weaponAxis );
	virtual void		WeaponFeedback();

	void				CalculateMuzzleRise( idVec3& origin, idMat3& axis );
	void				UpdateMuzzleKick();

	virtual void		EjectBrass();
	virtual bool		TransformBrass( const weaponJointHandle_t& handle, idVec3& origin, idMat3& axis );
	idStr				GetScriptFunction() { return scriptFunction; }
	const char *		GetString( const char *key ) { return dict->GetString( key ); }

	ID_INLINE virtual bool		HasAmmo() const;
	virtual void		UseAmmo();
	virtual int			AmmoAvailable() const;
	void				AddToClip( int amount );
	static ammo_t		GetAmmoType( const char *ammoname );
	ID_INLINE ammo_t	GetAmmoType() const;
	ID_INLINE int		AmmoInClip() const;
	ID_INLINE int		ClipSize() const;
	virtual bool		UsesCrosshair() const;
	ID_INLINE int		LowAmmo() const;

	//rww - net friendliness
	void				WriteToSnapshot( idBitMsgDelta &msg ) const;
	void				ReadFromSnapshot( const idBitMsgDelta &msg );

	void				UpdateWeaponJoints(void); //rww

	virtual bool		CheckThirdPersonMuzzle(idVec3 &origin, idMat3 &axis); //rww
protected:
	void				SetWeaponJointHandleList( const char* keyPrefix, hhCycleList<weaponJointHandle_t>& jointList );
	void				SaveWeaponJointHandleList( const hhCycleList<weaponJointHandle_t>& jointList, idSaveGame *savefile ) const;
	void				RestoreWeaponJointHandleList( hhCycleList<weaponJointHandle_t>& jointList, idRestoreGame *savefile );

	void				SetBrassDict( const char* name );

	virtual void		CalculateMuzzlePosition( idVec3& origin, idMat3& axis );
	virtual idEntity*	GetProjectileOwner() const;
	virtual const idBounds& GetCollisionBBox();

	virtual hhRenderEntity *GetSelf();
	virtual const hhRenderEntity *GetSelfConst() const;

protected:
	idEntityPtr<hhWeapon> self;
	idEntityPtr<hhPlayer> owner;

	idStr				scriptFunction;
	idStr				brassDefName; // HUMANHEAD mdl:  Added to save/load brassDef dict
	const idDict *		brassDef;
	int					brassDelay;

	ammo_t				ammoType;
	int					clipSize;			// 0 means no reload
	int					ammoClip;
	int					lowAmmo;

	// weapon kick
	int					muzzle_kick_time;
	int					muzzle_kick_maxtime;
	idAngles			muzzle_kick_angles;
	idVec3				muzzle_kick_offset;

	idVec2				aimDist;

	// joints from models
	hhCycleList<weaponJointHandle_t>	barrelJoints;
	hhCycleList<weaponJointHandle_t>	ejectJoints;
};

/*
================
hhWeaponFireController::HasAmmo
================
*/
ID_INLINE bool hhWeaponFireController::HasAmmo() const {
	return hhFireController::HasAmmo() && ( (ClipSize() == 0) || (AmmoInClip() > 0) );
}

/*
================
hhWeaponFireController::GetAmmoType
================
*/
ammo_t hhWeaponFireController::GetAmmoType() const {
	return ammoType;
}

/*
================
hhWeaponFireController::AmmoInClip
================
*/
int hhWeaponFireController::AmmoInClip() const {
	return ammoClip;
}

/*
================
hhWeaponFireController::ClipSize
================
*/
int hhWeaponFireController::ClipSize() const {
	return clipSize;
}

int hhWeaponFireController::LowAmmo() const {
	return lowAmmo;
}

#endif