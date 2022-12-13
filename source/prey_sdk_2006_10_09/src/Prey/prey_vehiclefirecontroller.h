#ifndef __HH_VEHICLE_FIRE_CONTROLLER_H
#define __HH_VEHICLE_FIRE_CONTROLLER_H

#include "gamesys/Class.h"

class hhVehicleFireController : public hhFireController {
	CLASS_PROTOTYPE(hhVehicleFireController);

public:
	virtual void		Clear();
	virtual void		Init( const idDict* viewDict, hhVehicle* self, idActor* owner );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual bool		LaunchProjectiles( const idVec3& pushVelocity );
	virtual void		WeaponFeedback();
	virtual idMat3		DetermineAimAxis( const idVec3& muzzlePos, const idMat3& weaponAxis );

	virtual int			AmmoAvailable() const;
	virtual void		UseAmmo();

	virtual float		GetRecoil() const { return recoil; }
	virtual bool		UsesCrosshair() const;

	//rww - made public
	hhCycleList<idVec3>	barrelOffsets;

protected:
	virtual const idBounds& GetCollisionBBox();
	virtual idEntity*	GetProjectileOwner() const;
	void				SetBarrelOffsetList( const char* keyPrefix, hhCycleList<idVec3>& offsetList );
	void				SaveBarrelOffsetList( const hhCycleList<idVec3>& offsetList, idSaveGame *savefile ) const;
	void				RestoreBarrelOffsetList( hhCycleList<idVec3>& offsetList, idRestoreGame *savefile );

	virtual void		CalculateMuzzlePosition( idVec3& origin, idMat3& axis );
	virtual hhRenderEntity *GetSelf();
	virtual const hhRenderEntity *GetSelfConst() const;

protected:
	idEntityPtr<hhVehicle> self;
	idEntityPtr<idActor> owner;

	float				recoil;
	int					nextFireTime;
};

#endif