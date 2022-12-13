#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


CLASS_DECLARATION( idClass, hhFxInfo )
END_CLASS


hhFxInfo::hhFxInfo() {
	Reset();	
}

hhFxInfo::hhFxInfo( const hhFxInfo* fxInfo ) {
	Assign( fxInfo );
}

hhFxInfo::hhFxInfo( const hhFxInfo& fxInfo ) {
	Assign( &fxInfo );
}

void hhFxInfo::Save(idSaveGame *savefile) const {
	fxInfoFlags_s infoFlags = flags;
	LittleBitField( &infoFlags, sizeof( infoFlags ) );
	savefile->Write( &infoFlags, sizeof( infoFlags ) );

	savefile->WriteVec3( normal );
	savefile->WriteVec3( incomingVector );
	savefile->WriteVec3( bounceVector );
	savefile->WriteString( bindBone );
	entity.Save(savefile);
}

void hhFxInfo::Restore( idRestoreGame *savefile ) {
	savefile->Read( &flags, sizeof( flags ) );
	LittleBitField( &flags, sizeof( flags ) );

	savefile->ReadVec3( normal );
	savefile->ReadVec3( incomingVector );
	savefile->ReadVec3( bounceVector );
	savefile->ReadString( bindBone );
	entity.Restore(savefile);
}

void hhFxInfo::WriteToSnapshot( idBitMsgDelta &msg ) const {
	/*
	msg.WriteBits(flags.normalIsSet, 1);
	msg.WriteBits(flags.incomingIsSet, 1);
	msg.WriteBits(flags.bounceIsSet, 1);
	msg.WriteBits(flags.start, 1);
	msg.WriteBits(flags.removeWhenDone, 1);
	msg.WriteBits(flags.toggle, 1);
	msg.WriteBits(flags.onlyVisibleInSpirit, 1);
	msg.WriteBits(flags.onlyInvisibleInSpirit, 1);
	msg.WriteBits(flags.useWeaponDepthHack, 1);
	msg.WriteBits(flags.bNoRemoveWhenUnbound, 1);

	msg.WriteFloat(normal.x);
	msg.WriteFloat(normal.y);
	msg.WriteFloat(normal.z);
	msg.WriteFloat(incomingVector.x);
	msg.WriteFloat(incomingVector.y);
	msg.WriteFloat(incomingVector.z);
	msg.WriteFloat(bounceVector.x);
	msg.WriteFloat(bounceVector.y);
	msg.WriteFloat(bounceVector.z);
	msg.WriteString(bindBone);
	msg.WriteBits(entity.GetSpawnId(), 32);
	*/

	msg.WriteBits(flags.normalIsSet, 1);
	msg.WriteBits(flags.removeWhenDone, 1);
	msg.WriteFloat(normal.x);
	msg.WriteFloat(normal.y);
	msg.WriteFloat(normal.z);
}

void hhFxInfo::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	/*
	flags.normalIsSet = !!msg.ReadBits(1);
	flags.incomingIsSet = !!msg.ReadBits(1);
	flags.bounceIsSet = !!msg.ReadBits(1);
	flags.start = !!msg.ReadBits(1);
	flags.removeWhenDone = !!msg.ReadBits(1);
	flags.toggle = !!msg.ReadBits(1);
	flags.onlyVisibleInSpirit = !!msg.ReadBits(1);
	flags.onlyInvisibleInSpirit = !!msg.ReadBits(1);
	flags.useWeaponDepthHack = !!msg.ReadBits(1);
	flags.bNoRemoveWhenUnbound = !!msg.ReadBits(1);

	normal.x = msg.ReadFloat();
	normal.y = msg.ReadFloat();
	normal.z = msg.ReadFloat();
	incomingVector.x = msg.ReadFloat();
	incomingVector.y = msg.ReadFloat();
	incomingVector.z = msg.ReadFloat();
	bounceVector.x = msg.ReadFloat();
	bounceVector.y = msg.ReadFloat();
	bounceVector.z = msg.ReadFloat();

	char buf[128];
	msg.ReadString(buf, 128);
	bindBone = buf;
	entity.SetSpawnId(msg.ReadBits(32));
	*/

	flags.normalIsSet = !!msg.ReadBits(1);
	flags.removeWhenDone = !!msg.ReadBits(1);
	normal.x = msg.ReadFloat();
	normal.y = msg.ReadFloat();
	normal.z = msg.ReadFloat();
}

hhFxInfo& hhFxInfo::Assign( const hhFxInfo* fxInfo ) {
	Reset();

	if( !fxInfo ) {
		return *this;
	}

	if( fxInfo->NormalIsSet() ) {
		SetNormal( fxInfo->GetNormal() );
	}

	if( fxInfo->IncomingVectorIsSet() ) {
		SetIncomingVector( fxInfo->GetIncomingVector() );
	}

	if( fxInfo->BounceVectorIsSet() ) {
		SetBounceVector( fxInfo->GetBounceVector() );
	}

	if( fxInfo->EntityIsSet() ) {
		SetEntity( fxInfo->GetEntity() );
	}

	if( fxInfo->BindBoneIsSet() ) {
		SetBindBone( fxInfo->GetBindBone() );
	}

	SetStart( fxInfo->StartIsSet() );

	RemoveWhenDone( fxInfo->RemoveWhenDone() );

	Toggle( fxInfo->Toggle() );

	OnlyVisibleInSpirit( fxInfo->OnlyVisibleInSpirit() );
	OnlyInvisibleInSpirit( fxInfo->OnlyInvisibleInSpirit() );
	UseWeaponDepthHack( fxInfo->UseWeaponDepthHack() );
	NoRemoveWhenUnbound( fxInfo->NoRemoveWhenUnbound() );

	return *this;
}

hhFxInfo& hhFxInfo::operator=( const hhFxInfo* fxInfo ) {
	return Assign( fxInfo );
}

hhFxInfo& hhFxInfo::operator=( const hhFxInfo& fxInfo ) {
	return Assign( &fxInfo );
}

void hhFxInfo::SetNormal( const idVec3 &v ) {
	assert( v.Length() ); 
	normal = v; 
	flags.normalIsSet = true; 
}

void hhFxInfo::SetIncomingVector( const idVec3 &v ) {
	assert( v.Length() ); 
	incomingVector = v; 
	flags.incomingIsSet = true;
}

void hhFxInfo::SetBounceVector( const idVec3 &v ) {
	assert( v.Length() );
	bounceVector = v;
	flags.bounceIsSet = true; 
}

void hhFxInfo::SetEntity( idEntity *e ) {
	entity = e; 
}

void hhFxInfo::SetBindBone( const char* bindBone ) {
	this->bindBone = bindBone; 
}

void hhFxInfo::SetStart(const bool start) { 
	flags.start = start;
}

void hhFxInfo::RemoveWhenDone( const bool removeWhenDone ) {
	flags.removeWhenDone = removeWhenDone; 
}

void hhFxInfo::Toggle( const bool tf ) {
	flags.toggle = tf;
}

void hhFxInfo::OnlyVisibleInSpirit( const bool spirit ) {
	flags.onlyVisibleInSpirit = spirit;
}

void hhFxInfo::OnlyInvisibleInSpirit( const bool spirit ) {
	flags.onlyInvisibleInSpirit = spirit; 
}

void hhFxInfo::UseWeaponDepthHack( const bool weaponDepthHack ) {
	flags.useWeaponDepthHack = weaponDepthHack;
}

void hhFxInfo::Triggered( const bool tf ) {
	flags.triggered = tf;
}

const idVec3&	hhFxInfo::GetNormal( ) const {
	assert( flags.normalIsSet ); 
	return( normal );
}

const idVec3&	hhFxInfo::GetIncomingVector( ) const {
	assert( flags.incomingIsSet );
	return( incomingVector );
}

const idVec3&	hhFxInfo::GetBounceVector( ) const {
	assert( flags.bounceIsSet );
	return( bounceVector );
}

const char*		hhFxInfo::GetBindBone() const {
	return bindBone.c_str();
}

idEntity* const	hhFxInfo::GetEntity( ) const {
	return( entity.GetEntity() );
}

bool hhFxInfo::RemoveWhenDone() const {
	return flags.removeWhenDone;
}

bool hhFxInfo::StartIsSet() const {
	return flags.start;
}

bool hhFxInfo::NormalIsSet( ) const {
	return flags.normalIsSet;
}

bool hhFxInfo::IncomingVectorIsSet( ) const {
	return flags.incomingIsSet;
}

bool hhFxInfo::BounceVectorIsSet( ) const {
	return flags.bounceIsSet; 
}

bool hhFxInfo::BindBoneIsSet() const {
	return bindBone.Length() > 0; 
}

bool hhFxInfo::EntityIsSet() const {
	return entity.IsValid();
}

bool hhFxInfo::Toggle() const {
	return flags.toggle;
}

bool hhFxInfo::OnlyVisibleInSpirit( void ) const {
	return flags.onlyVisibleInSpirit;
}

bool hhFxInfo::OnlyInvisibleInSpirit( void ) const {
	return flags.onlyInvisibleInSpirit; 
}

bool hhFxInfo::UseWeaponDepthHack() const {
	return flags.useWeaponDepthHack;
}

bool hhFxInfo::Triggered() const {
	return flags.triggered;
}

void hhFxInfo::NoRemoveWhenUnbound( const bool noRemove ) {
	flags.bNoRemoveWhenUnbound = noRemove;
}

bool hhFxInfo::NoRemoveWhenUnbound() const {
	return flags.bNoRemoveWhenUnbound;
}

void hhFxInfo::Reset() {
	entity = NULL;
	normal.Zero(); bounceVector.Zero(); 
	incomingVector.Zero();
	bindBone.Empty();
	flags.normalIsSet = false;
	flags.bounceIsSet = false; 
	flags.incomingIsSet = false;
	flags.start = true;
	flags.removeWhenDone = true;
	flags.toggle = false; // jrm
	flags.onlyVisibleInSpirit = false;
	flags.onlyInvisibleInSpirit = false;	// tmj
	flags.useWeaponDepthHack = false; 
	flags.triggered = false; // mdl
	flags.bNoRemoveWhenUnbound = false;	// mdc
}

bool hhFxInfo::GetAxisFor( int which, idVec3& dir ) const {
	switch ( which ) {
		case AXIS_NORMAL:
			if ( NormalIsSet() ) { dir = GetNormal(); return true; }
			else { 
				if ( g_debugFX.GetBool() ) gameLocal.Warning("Tried to get fxInfo Normal when it isn't set"); 
			}
		break;
		case AXIS_BOUNCE:
			if ( BounceVectorIsSet() ) { dir = GetBounceVector(); return true; }
			else { 
				if ( g_debugFX.GetBool() ) gameLocal.Warning("Tried to get fxInfo Bounce Vector when it isn't set"); 
			}
		break;
		case AXIS_CUSTOMLOCAL:
			// handled in alternate path
			if ( g_debugFX.GetBool() ) {
				gameLocal.Warning("Using customlocal without axis");
			}
		break;
		case AXIS_INCOMING:
			if ( IncomingVectorIsSet() ) { dir = GetIncomingVector(); return true; }
			else { 
				if ( g_debugFX.GetBool() ) gameLocal.Warning("Tried to get fxInfo Incoming Vector when it isn't set"); 
			}
		break;
	}
	return false;
}