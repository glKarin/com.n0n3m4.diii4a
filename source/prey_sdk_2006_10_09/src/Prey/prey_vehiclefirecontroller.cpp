
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( hhFireController, hhVehicleFireController )
END_CLASS

/*
================
hhVehicleFireController::Init
================
*/
void hhVehicleFireController::Init( const idDict* viewDict, hhVehicle* self, idActor* owner ) {
	hhFireController::Init( viewDict );

	this->owner = owner;
	this->self = self;

	recoil = dict->GetFloat( "recoil" );

	if ( owner && owner->IsType( idAI::Type ) ) {
		spread = dict->GetFloat( "monster_spread" );
	}

	SetBarrelOffsetList( "muzzleOffset", barrelOffsets );
}

/*
================
hhVehicleFireController::UsesCrosshair
================
*/
bool hhVehicleFireController::UsesCrosshair() const {
	return bCrosshair;
}

/*
================
hhVehicleFireController::Clear
================
*/
void hhVehicleFireController::Clear() {
	hhFireController::Clear();

	nextFireTime = gameLocal.GetTime();
	barrelOffsets.Clear();

	owner = NULL;
	self = NULL;
}

/*
================
hhVehicleFireController::Save
================
*/
void hhVehicleFireController::Save( idSaveGame *savefile ) const {
	self.Save( savefile );
	owner.Save( savefile );
	savefile->WriteFloat( recoil );

	int num = barrelOffsets.Num();
	savefile->WriteInt( num );
	for( int i = 0; i < num; i++ ) {
		savefile->WriteVec3( barrelOffsets[i] );
	}
	
	savefile->WriteInt( nextFireTime );
}

/*
================
hhVehicleFireController::Restore
================
*/
void hhVehicleFireController::Restore( idRestoreGame *savefile ) {
	self.Restore( savefile );
	owner.Restore( savefile );
	savefile->ReadFloat( recoil );

	int num;
	savefile->ReadInt( num );
	idVec3 tmp;
	for( int i = 0; i < num; i++ ) {
		savefile->ReadVec3( tmp );
		barrelOffsets.Append( tmp );
	}

	savefile->ReadInt( nextFireTime );
}

/*
================
hhVehicleFireController::WeaponFeedback
================
*/
void hhVehicleFireController::WeaponFeedback() {
	if( self.IsValid() && self->GetPhysics() ) {
		hhPhysics_Vehicle* selfPhysics = static_cast<hhPhysics_Vehicle*>( self->GetPhysics() );
		self->ApplyImpulse( gameLocal.world, 0, self->GetOrigin() + selfPhysics->GetCenterOfMass(), -self->GetAxis()[0] * GetRecoil() * selfPhysics->GetMass() );
	}
}

/*
================
hhVehicleFireController::SetBarrelOffsetList
================
*/
void hhVehicleFireController::SetBarrelOffsetList( const char* keyPrefix, hhCycleList<idVec3>& offsetList ) {
	const idKeyValue* kv = dict->MatchPrefix( keyPrefix );
	while( kv ) {
		offsetList.Append( dict->GetVector(kv->GetKey().c_str()) );
		kv = dict->MatchPrefix( keyPrefix, kv );
	}
}

/*
=================
hhVehicleFireController::DetermineAimAxis
=================
*/
idMat3 hhVehicleFireController::DetermineAimAxis( const idVec3& muzzlePos, const idMat3& weaponAxis ) {
	idAngles	aimAngles;
	idVec3		aimPos;
	
	aimPos = idVec3( 0.0f, 0.0f, owner->EyeHeight() ) + dict->GetVector( "offset_gunTarget" );
	aimPos *= self->GetFireAxis();
	aimPos += self->GetFireOrigin();

	aimAngles = (aimPos - muzzlePos).ToAngles();
	aimAngles[2] = weaponAxis.ToAngles()[2];
	return aimAngles.ToMat3();
}

/*
================
hhVehicleFireController::LaunchProjectiles
================
*/
bool hhVehicleFireController::LaunchProjectiles( const idVec3& pushVelocity ) {
	if( nextFireTime > gameLocal.GetTime() ) {
		return false;
	}

	if( !hhFireController::LaunchProjectiles(pushVelocity) ) {
		return false;
	}

	gameLocal.AlertAI( owner.GetEntity() );
	nextFireTime = gameLocal.GetTime() + SEC2MS( GetFireDelay() );
	return true;
}

/*
================
hhFireController::AmmoAvailable
================
*/
int hhVehicleFireController::AmmoAvailable() const {
	if ( self.IsValid() ) {
		return self->HasPower( AmmoRequired() );
	} else {
		return 0;
	}
}

/*
================
hhFireController::UseAmmo
================
*/
void hhVehicleFireController::UseAmmo() {
	if( self.IsValid() ) {
		self->ConsumePower( AmmoRequired() );
	}
}

/*
================
hhVehicleFireController::GetCollisionBBox
================
*/
const idBounds& hhVehicleFireController::GetCollisionBBox() {
	return self->GetPhysics()->GetAbsBounds();
}

/*
================
hhVehicleFireController::CalculateMuzzlePosition
================
*/
void hhVehicleFireController::CalculateMuzzlePosition( idVec3& origin, idMat3& axis ) {
	axis = self->GetFireAxis();
	origin = barrelOffsets.Next() * axis + self->GetFireOrigin();
}

/*
================
hhVehicleFireController::GetProjectileOwner
================
*/
idEntity *hhVehicleFireController::GetProjectileOwner() const {
	 return self.GetEntity();
} 

/*
================
hhVehicleFireController::GetSelf
================
*/
hhRenderEntity *hhVehicleFireController::GetSelf() {
	return self.GetEntity();
}

/*
================
hhVehicleFireController::GetSelfConst
================
*/
const hhRenderEntity *hhVehicleFireController::GetSelfConst() const {
	return self.GetEntity();
}

