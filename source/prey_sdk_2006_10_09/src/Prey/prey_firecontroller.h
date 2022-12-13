#ifndef __HH_FIRE_CONTROLLER_H
#define __HH_FIRE_CONTROLLER_H

#include "gamesys/Class.h"

#define MAX_NET_PROJECTILES		3 //rww

class hhFireController : public idClass {
	ABSTRACT_PROTOTYPE(hhFireController)

public:
						hhFireController();
	virtual				~hhFireController();

	virtual void		Clear();
	virtual void		Init( const idDict* viewDict );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual bool		UsesCrosshair() const = 0;
	virtual idVec3		GetMuzzlePosition() const;
	ID_INLINE virtual bool		HasAmmo() const;
	virtual void		UseAmmo() = 0;
	virtual int			AmmoAvailable() const = 0;
	ID_INLINE virtual int		AmmoRequired() const;
	ID_INLINE float		GetFireDelay() const;

	void				UpdateMuzzleFlash();
	virtual void		CreateMuzzleFx( const idVec3& pos, const idMat3& axis );

	ID_INLINE virtual idMat3		DetermineProjectileAxis( const idMat3& axis );
	virtual bool		LaunchProjectiles( const idVec3& pushVelocity );

	void				MuzzleFlash();
	virtual void		WeaponFeedback() = 0;

	virtual				//HUMANHEAD bjk
	const idDict*		GetProjectileDict() const { return projectile; }

	void				SetWeaponDict( const idDict *wdict ) { dict = wdict; yawSpread = dict->GetFloat("yawSpread"); }

	void				CheckDeferredProjectiles(void); //rww

	virtual bool		CheckThirdPersonMuzzle(idVec3 &origin, idMat3 &axis); //rww
protected:
	void				SetProjectileDict( const char* name );

	virtual void		LaunchProjectiles( const idVec3& launchOrigin, const idMat3& aimAxis, const idVec3& pushVelocity, idEntity* projOwner );
	virtual idEntity*	GetProjectileOwner() const = 0;
	ID_INLINE virtual hhProjectile* SpawnProjectile();

	virtual const idBounds& GetCollisionBBox() = 0;
	virtual void		CalculateMuzzlePosition( idVec3& origin, idMat3& axis );

	ID_INLINE void		AttemptToRemoveMuzzleFlash();
	void				UpdateMuzzleFlashPosition();

	idVec3				AssureInsideCollisionBBox( const idVec3& origin, const idMat3& axis, const idBounds& ownerAbsBounds, float projMaxHalfDim ) const;
	virtual idMat3		DetermineAimAxis( const idVec3& muzzlePos, const idMat3& weaponAxis );

	virtual hhRenderEntity *GetSelf() = 0;
	virtual const hhRenderEntity *GetSelfConst() const = 0;

protected:
	idDict				restoredDict;
	const idDict*		dict;

	const idDict *		projectile;
	idStr				projDictName;

	idVec3				muzzleOrigin;
	idMat3				muzzleAxis;

	// muzzle flash
	renderLight_t		muzzleFlash;				// positioned on view weapon bone
	int					muzzleFlashHandle;

	int					muzzleFlashEnd;
	int					flashTime;

	// ammo management
	int					ammoRequired;		// amount of ammo to use each shot.  0 means weapon doesn't need ammo.

	float				fireDelay;
	float				spread;

	float				yawSpread; //rww - extra spread on yaw

	bool				bCrosshair;

	float				projectileMaxHalfDimension;

	int					numProjectiles;

	//rww - projectile deferring
	int						deferProjNum;
	int						deferProjTime;
	idVec3					deferProjLaunchOrigin;
	idMat3					deferProjLaunchAxis;
	idVec3					deferProjPushVelocity;
	idEntityPtr<idEntity>	deferProjOwner;
};

/*
=================
hhFireController::DetermineAimAxis
=================
*/
ID_INLINE idMat3 hhFireController::DetermineAimAxis( const idVec3& muzzlePos, const idMat3& weaponAxis ) {
	return weaponAxis;
}

/*
================
hhFireController::HasAmmo
================
*/
ID_INLINE bool hhFireController::HasAmmo() const {
	return AmmoAvailable() != 0;
}

/*
================
hhFireController::SpawnProjectile
================
*/
ID_INLINE hhProjectile* hhFireController::SpawnProjectile() {
	return hhProjectile::SpawnProjectile( GetProjectileDict() );
}

/*
================
hhFireController::AttemptToRemoveMuzzleFlash
================
*/
ID_INLINE void hhFireController::AttemptToRemoveMuzzleFlash() {
	if( gameLocal.GetTime() >= muzzleFlashEnd ) {
		SAFE_FREELIGHT( muzzleFlashHandle );
	}
}

/*
================
hhFireController::AmmoRequired
================
*/
ID_INLINE int hhFireController::AmmoRequired() const {
	return ammoRequired;
}

/*
================
hhFireController::GetFireDelay
================
*/
ID_INLINE float hhFireController::GetFireDelay() const {
	return fireDelay;
}

#endif
