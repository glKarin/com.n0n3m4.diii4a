
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( hhFireController, hhWeaponFireController )
END_CLASS

/*
================
hhWeaponFireController::Clear
================
*/
void hhWeaponFireController::Clear() {
	hhFireController::Clear();

	self = NULL;
	owner = NULL;

	brassDef = NULL;
	brassDelay = -1;

	ammoType = 0;
	clipSize = 0;			// 0 means no reload
	ammoClip = 0;
	lowAmmo = 0;

	// weapon kick
	muzzle_kick_time = 0;
	muzzle_kick_maxtime = 0;
	muzzle_kick_angles.Zero();
	muzzle_kick_offset.Zero();

	aimDist.Zero();

	// joints from models
	barrelJoints.Clear();
	ejectJoints.Clear();
}

/*
================
hhWeaponFireController::Init
================
*/
void hhWeaponFireController::Init( const idDict* viewDict, hhWeapon* self, hhPlayer* owner ) {
	const char *brassDefName	= NULL;

	hhFireController::Init( viewDict );

	this->self = self;
	this->owner = owner;

	muzzleFlash.lightId = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
	//muzzleFlash.allowLightInViewID = owner->entityNumber+1;

	if( !dict ) {
		return;
	}

	muzzle_kick_time	= SEC2MS( dict->GetFloat( "muzzle_kick_time" ) );
	muzzle_kick_maxtime	= SEC2MS( dict->GetFloat( "muzzle_kick_maxtime" ) );
	muzzle_kick_angles	= dict->GetAngles( "muzzle_kick_angles" );
	muzzle_kick_offset	= dict->GetVector( "muzzle_kick_offset" );

	// find some joints in the model for locating effects
	SetWeaponJointHandleList( "joint_barrel", barrelJoints );
	SetWeaponJointHandleList( "joint_eject", ejectJoints );

	// get the brass def
	SetBrassDict( dict->GetString("def_ejectBrass") );

	ammoType			= GetAmmoType( dict->GetString( "ammoType" ) );
	clipSize			= dict->GetInt( "clipSize" );
	lowAmmo				= dict->GetInt( "lowAmmo" );

	if( ( ammoType < 0 ) || ( ammoType >= AMMO_NUMTYPES ) ) {
		gameLocal.Warning( "Unknown ammotype in object '%s'", dict->GetString("classname") );
	}

	//HUMANHEAD bjk: set in hhWeapon::GetWeaponDef
	ammoClip = 0;
	/*if ( clipSize ) {
		ammoClip = owner->HasAmmo( ammoType, ammoRequired );
		if ( ( ammoClip < 0 ) || ( ammoClip > clipSize ) ) {
			// first time using this weapon so have it fully loaded to start
			ammoClip = clipSize;
		}
	}*/

	aimDist = dict->GetVec2( "aimDist", "20 50" );
	scriptFunction = dict->GetString( "script_function", "Fire" );
}

/*
================
hhWeaponFireController::SetWeaponJointHandleList
================
*/
void hhWeaponFireController::SetWeaponJointHandleList( const char* keyPrefix, hhCycleList<weaponJointHandle_t>& jointList ) {
	weaponJointHandle_t handle;
	const idKeyValue* kv = dict->MatchPrefix( keyPrefix );
	while( kv ) {
		self->GetJointHandle( kv->GetValue().c_str(), handle );
		jointList.Append( handle );
		kv = dict->MatchPrefix( keyPrefix, kv );
	}
}

/*
================
hhWeaponFireController::SaveWeaponJointHandleList
================
*/
void hhWeaponFireController::SaveWeaponJointHandleList( const hhCycleList<weaponJointHandle_t>& jointList, idSaveGame *savefile ) const {
	const weaponJointHandle_t* handle = NULL;

	int num = jointList.Num();
	savefile->WriteInt( num );
	for( int ix = 0; ix < num; ++ix ) {
		handle = &jointList[ix];
		if( handle ) {
			savefile->WriteInt( handle->view );
			savefile->WriteInt( handle->world );
		}
	}
}

/*
================
hhWeaponFireController::RestoreWeaponJointHandleList
================
*/
void hhWeaponFireController::RestoreWeaponJointHandleList( hhCycleList<weaponJointHandle_t>& jointList, idRestoreGame *savefile ) {
	//weaponJointHandle_t* handle = NULL;

	jointList.Clear();

	int num;
	savefile->ReadInt( num );
	weaponJointHandle_t handle;
	for( int ix = 0; ix < num; ++ix ) {
		savefile->ReadInt( (int&)handle.view );
		savefile->ReadInt( (int&)handle.world );
		jointList.Append( handle );
	}
}

/*
================
hhWeaponFireController::SetBrassDict
================
*/
void hhWeaponFireController::SetBrassDict( const char* name ) {
	brassDefName = name;
	if ( name[0] ) {
		brassDef = gameLocal.FindEntityDefDict( name, false );
		if ( !brassDef ) {
			gameLocal.Warning( "Unknown brass '%s'", name );
		}
	} else {
		brassDef = NULL;
	}
}

/*
================
hhWeaponFireController::LaunchProjectiles
================
*/
bool hhWeaponFireController::LaunchProjectiles( const idVec3& pushVelocity ) {
	if( !hhFireController::LaunchProjectiles(pushVelocity) ) {
		return false;
	}

	if( !gameLocal.isClient ) {
		owner->AddProjectilesFired( numProjectiles );
	}

	//HUMANHEAD bjk PATCH 7-27-06
	if( gameLocal.isMultiplayer ) {
		owner->inventory.lastShot[owner->GetWeaponNum( self->spawnArgs.GetString( "classname" ) )] = gameLocal.time + 1000;
	}

	return true;
}

/*
=================
hhWeaponFireController::DetermineAimAxis
=================
*/
idMat3 hhWeaponFireController::DetermineAimAxis( const idVec3& muzzlePos, const idMat3& weaponAxis ) {
	float traceDist = 1024.0f;	// was CM_MAX_TRACE_DIST
	float aimTraceDist;
	trace_t aimTrace = self->GetEyeTraceInfo();
	
	//HUMANHEAD bjk
	if( aimTrace.fraction < 1.0f ) {
		idVec3 eyePos = owner->GetEyePosition();

		// Perform eye trace
		gameLocal.clip.TracePoint( aimTrace, eyePos, eyePos + weaponAxis[0] * traceDist * 4.0f, 
			MASK_SHOT_RENDERMODEL | CONTENTS_GAME_PORTAL, owner.GetEntity() );

		if ( aimTrace.fraction < 1.0f ) { // CJR:  Check for portals
			idEntity *ent = gameLocal.entities[ aimTrace.c.entityNum ]; // cannot use GetTraceEntity() as the portal could be bound to something
			if ( ent && ent->IsType( hhPortal::Type ) ) {
				aimTrace.endpos = eyePos + weaponAxis[0] * traceDist;
				aimTrace.fraction = 1.0f;
			}
		}

		aimTraceDist = aimTrace.fraction * traceDist * 4.0f;
	} else {
		aimTraceDist = aimTrace.fraction * traceDist;
	} //HUMANHEAD END

	idVec3		aimVector( aimTrace.endpos - muzzlePos );
	idVec3		aimDir = aimVector;
	
	aimDir.Normalize();
	aimDir.Lerp( weaponAxis[0], aimDir, hhUtils::CalculateScale(aimTraceDist, aimDist[0], aimDist[1]) );
	
	idAngles	aimAngles( aimDir.ToAngles() );

	aimAngles.roll = weaponAxis.ToAngles().roll;
	return aimAngles.ToMat3();
}

/*
================
hhWeaponFireController::WeaponFeedback
================
*/
void hhWeaponFireController::WeaponFeedback() {
	if( owner.IsValid() ) {
		owner->WeaponFireFeedback( dict );
	}
}

/*
================
hhWeaponFireController::EjectBrass
================
*/
void hhWeaponFireController::EjectBrass() {
	idMat3				axis;
	idVec3				origin;
	idEntity*			ent = NULL;
	idDebris*			brass = NULL;

	if( !g_showBrass.GetBool() || !brassDef ) {
		return;
	}

	if (gameLocal.isClient && !gameLocal.isNewFrame) { //rww
		return;
	}

	if( TransformBrass(ejectJoints.Next(), origin, axis) ) {
		gameLocal.SpawnEntityDef( *brassDef, &ent, true, gameLocal.isClient ); //rww - localized debris
		HH_ASSERT( ent && ent->IsType(idDebris::Type) );
			
		brass = static_cast<idDebris*>( ent );
		brass->Create( owner.GetEntity(), origin, axis );
		brass->Launch();
		brass->fl.networkSync = false; //rww
		brass->fl.clientEvents = true; //rww
		//HACK: this should be in debris object.  Just not sure if I should re-write it or not.
		brass->SetShaderParm( SHADERPARM_TIME_OF_DEATH, MS2SEC(gameLocal.GetTime()) );
	}
}

/*
=================
hhWeaponFireController::TransformBrass
=================
*/
bool hhWeaponFireController::TransformBrass( const weaponJointHandle_t& handle, idVec3 &origin, idMat3 &axis ) {
	if( !self->GetJointWorldTransform(handle, origin, axis) ) {
		return false;
	}

	axis = self->GetAxis();

	return true;
}

/*
================
hhWeaponFireController::CalculateMuzzleRise
================
*/
void hhWeaponFireController::CalculateMuzzleRise( idVec3& origin, idMat3& axis ) {
	int			time;
	float		amount;
	idAngles	ang;
	idVec3		offset;

	time = self->GetKickEndTime() - gameLocal.GetTime();
	if ( time <= 0 ) {
		return;
	}

	if ( muzzle_kick_maxtime <= 0 ) {
		return;
	}

	if ( time > muzzle_kick_maxtime ) {
		time = muzzle_kick_maxtime;
	}
	
	amount = ( float )time / ( float )muzzle_kick_maxtime;
	ang		= muzzle_kick_angles * amount;
	offset	= muzzle_kick_offset * amount;

	origin = origin - axis * offset;
	axis = ang.ToMat3() * axis;
}

/*
================
hhWeaponFireController::UpdateMuzzleKick
================
*/
void hhWeaponFireController::UpdateMuzzleKick() {
	// add some to the kick time, incrementally moving repeat firing weapons back
	if ( self->GetKickEndTime() < gameLocal.GetTime() ) {
		self->SetKickEndTime( gameLocal.GetTime() );
	}
	self->SetKickEndTime( self->GetKickEndTime() + muzzle_kick_time );
	if ( self->GetKickEndTime() > gameLocal.GetTime() + muzzle_kick_maxtime ) {
		self->SetKickEndTime( gameLocal.GetTime() + muzzle_kick_maxtime );
	}
}

/*
================
hhWeaponFireController::CalculateMuzzlePosition
================
*/
void hhWeaponFireController::CalculateMuzzlePosition( idVec3& origin, idMat3& axis ) {
	if( !barrelJoints.Num() || !self->GetJointWorldTransform(barrelJoints.Next(), origin, axis) ) {
		origin = GetMuzzlePosition();
		axis = self->GetAxis();
	}
}

/*
================
hhWeaponFireController::UseAmmo
================
*/
void hhWeaponFireController::UseAmmo() {
	if( owner.IsValid() ) {
		owner->UseAmmo( GetAmmoType(), AmmoRequired() );
		if ( ClipSize() && AmmoRequired() ) {
			ammoClip--;
		}
	}
}

/*
================
hhWeaponFireController::AddToClip
================
*/
void hhWeaponFireController::AddToClip( int amount ) {
	ammoClip += amount;
	if ( ammoClip > clipSize ) {
		ammoClip = clipSize;
	}

	if ( ammoClip > AmmoAvailable() ) {
		ammoClip = AmmoAvailable();
	}
}

/*
================
hhWeaponFireController::GetAmmoType
================
*/
ammo_t hhWeaponFireController::GetAmmoType( const char *ammoname ) {
	int num;
	const idDict *ammoDict;

	assert( ammoname );

	ammoDict = gameLocal.FindEntityDefDict( "ammo_types", false );
	if ( !ammoDict ) {
		gameLocal.Error( "Could not find entity definition for 'ammo_types'\n" );
	}

	if ( !ammoname[ 0 ] ) {
		ammoname = "ammo_none";
	}

	if ( !ammoDict->GetInt( ammoname, "-1", num ) ) {
		gameLocal.Error( "Unknown ammo type '%s'", ammoname );
	}

	if ( ( num < 0 ) || ( num >= AMMO_NUMTYPES ) ) {
		gameLocal.Error( "Ammo type '%s' value out of range.  Maximum ammo types is %d.\n", ammoname, AMMO_NUMTYPES );
	}

	return ( ammo_t )num;
}

/*
================
hhWeaponFireController::Save
================
*/
void hhWeaponFireController::Save( idSaveGame *savefile ) const {
	self.Save( savefile );
	owner.Save( savefile );

	savefile->WriteString( brassDefName );
	savefile->WriteInt( brassDelay );
	savefile->WriteString( scriptFunction );

	savefile->WriteInt( ammoType );
	savefile->WriteInt( clipSize );
	savefile->WriteInt( ammoClip );
	savefile->WriteInt( lowAmmo );

	savefile->WriteInt( muzzle_kick_time );
	savefile->WriteInt( muzzle_kick_maxtime );
	savefile->WriteAngles( muzzle_kick_angles );
	savefile->WriteVec3( muzzle_kick_offset );

	savefile->WriteVec2( aimDist );

	SaveWeaponJointHandleList( barrelJoints, savefile );
	SaveWeaponJointHandleList( ejectJoints, savefile );

}

/*
================
hhWeaponFireController::Restore
================
*/
void hhWeaponFireController::Restore( idRestoreGame *savefile ) {
	self.Restore( savefile );
	owner.Restore( savefile );
	savefile->ReadString( brassDefName );
	savefile->ReadInt( brassDelay );
	savefile->ReadString( scriptFunction );

	savefile->ReadInt( reinterpret_cast<int &> ( ammoType ) );
	savefile->ReadInt( clipSize );
	savefile->ReadInt( ammoClip );
	savefile->ReadInt( lowAmmo );

	savefile->ReadInt( muzzle_kick_time );
	savefile->ReadInt( muzzle_kick_maxtime );
	savefile->ReadAngles( muzzle_kick_angles );
	savefile->ReadVec3( muzzle_kick_offset );

	savefile->ReadVec2( aimDist );

	RestoreWeaponJointHandleList( barrelJoints, savefile );
	RestoreWeaponJointHandleList( ejectJoints, savefile );

	SetBrassDict( brassDefName );
}

/*
================
hhWeaponFireController::AmmoAvailable
================
*/
int hhWeaponFireController::AmmoAvailable() const {
	if ( owner.IsValid() ) {
		return owner->HasAmmo( GetAmmoType(), AmmoRequired() );
	} else {
		return 0;
	}
}

/*
================
hhWeaponFireController::GetProjectileOwner
================
*/
idEntity *hhWeaponFireController::GetProjectileOwner() const {
	return owner.GetEntity();
}

/*
================
hhWeaponFireController::GetCollisionBBox
================
*/
const idBounds& hhWeaponFireController::GetCollisionBBox() {
	return owner->GetPhysics()->GetAbsBounds();
}

/*
================
hhWeaponFireController::GetSelf
================
*/
hhRenderEntity *hhWeaponFireController::GetSelf() {
	return self.GetEntity(); 
}

/*
================
hhWeaponFireController::GetSelfConst
================
*/
const hhRenderEntity *hhWeaponFireController::GetSelfConst() const {
	return self.GetEntity(); 
}

/*
================
hhWeaponFireController::UsesCrosshair
================
*/
bool hhWeaponFireController::UsesCrosshair() const {
	return (self.IsValid() && bCrosshair && self->ShowCrosshair());
}

//rww - net friendliness
void hhWeaponFireController::WriteToSnapshot( idBitMsgDelta &msg ) const
{
	msg.WriteBits(self.GetSpawnId(), 32);
	msg.WriteBits(owner.GetSpawnId(), 32);

	msg.WriteBits(ammoClip, self->GetClipBits());

	//RWWTODO: Added lowAmmo int in case it needs to be sent --paul
}

void hhWeaponFireController::ReadFromSnapshot( const idBitMsgDelta &msg )
{
	self.SetSpawnId(msg.ReadBits(32));
	owner.SetSpawnId(msg.ReadBits(32));

	ammoClip = msg.ReadBits(self->GetClipBits());
}

/*
================
hhWeaponFireController::UpdateWeaponJoints
================
*/
void hhWeaponFireController::UpdateWeaponJoints(void) { //rww
	barrelJoints.Clear();
	ejectJoints.Clear();

	SetWeaponJointHandleList( "joint_barrel", barrelJoints );
	SetWeaponJointHandleList( "joint_eject", ejectJoints );
}

/*
================
hhWeaponFireController::CheckThirdPersonMuzzle
================
*/
bool hhWeaponFireController::CheckThirdPersonMuzzle(idVec3 &origin, idMat3 &axis) { //rww
	if (barrelJoints.Num() <= 0) {
		return false;
	}
	
	int i = barrelJoints.GetCurrentIndex();
	return self->GetJointWorldTransform(barrelJoints[i], origin, axis, true);
}
