#ifndef __HH_FX_INFO_H
#define __HH_FX_INFO_H

class hhFxInfo : public idClass {
	CLASS_PROTOTYPE( hhFxInfo );

public:
					hhFxInfo();
					hhFxInfo( const hhFxInfo* fxInfo );
					hhFxInfo( const hhFxInfo& fxInfo );
	hhFxInfo&		Assign( const hhFxInfo* fxInfo );
	hhFxInfo&		operator=( const hhFxInfo* fxInfo );
	hhFxInfo&		operator=( const hhFxInfo& fxInfo );

	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	//HUMANHEAD rww
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );
	//HUMANHEAD END

	void			SetNormal( const idVec3 &v );
	void			SetIncomingVector( const idVec3 &v );
	void			SetBounceVector( const idVec3 &v );
	void			SetEntity( idEntity *e );
	void			SetBindBone( const char* bindBone );
	void			SetStart( const bool start );
	void			RemoveWhenDone( const bool removeWhenDone );
	void			Toggle( const bool tf );
	void			OnlyVisibleInSpirit( const bool spirit );
	void			OnlyInvisibleInSpirit( const bool spirit );
	void			UseWeaponDepthHack( const bool weaponDepthHack );
	void			Triggered( const bool tf );

	void			NoRemoveWhenUnbound( const bool noRemove );

	const idVec3&	GetNormal( ) const;
	const idVec3&	GetIncomingVector( ) const;
	const idVec3&	GetBounceVector( ) const;
	const char*		GetBindBone() const;
	idEntity* const	GetEntity( ) const;

	bool			RemoveWhenDone() const;
	bool			StartIsSet() const;
	bool			NormalIsSet( ) const;
	bool			IncomingVectorIsSet( ) const;
	bool			BounceVectorIsSet( ) const;
	bool			BindBoneIsSet() const;
	bool			EntityIsSet() const;
	bool			Toggle() const;
	bool			OnlyVisibleInSpirit( void ) const;
	bool			OnlyInvisibleInSpirit( void ) const;
	bool			UseWeaponDepthHack() const;
	bool			Triggered() const;

	bool			NoRemoveWhenUnbound() const;

	void			Reset();

	bool			GetAxisFor( int which, idVec3& dir ) const;

	void			WriteToBitMsg( idBitMsg* msg ) const;
	void			ReadFromBitMsg( const idBitMsg* bitMsg );
							  			
protected:
	struct fxInfoFlags_s {
		bool				normalIsSet;
		bool				incomingIsSet;
		bool				bounceIsSet;
		bool				start;
		bool				removeWhenDone;
		bool				toggle;
		bool				onlyVisibleInSpirit; // CJR
		bool				onlyInvisibleInSpirit; // tmj
		bool				useWeaponDepthHack;
		bool				bNoRemoveWhenUnbound;	//mdc
		bool				triggered; // mdl
	} flags;

	idVec3					normal;
	idVec3					incomingVector;
	idVec3					bounceVector;
	idStr					bindBone;
	idEntityPtr<idEntity>	entity;
};

ID_INLINE void hhFxInfo::WriteToBitMsg( idBitMsg* msg ) const {
	bool varIsSet = false;
	
	varIsSet = NormalIsSet();
	msg->WriteBool( varIsSet );
	if( varIsSet ) {
		msg->WriteDir( GetNormal(), 24 );
	}

	varIsSet = IncomingVectorIsSet();
	msg->WriteBool( varIsSet );
	if( varIsSet ) {
		msg->WriteDir( GetIncomingVector(), 24 );
	}

	varIsSet = BounceVectorIsSet();
	msg->WriteBool( varIsSet );
	if( varIsSet ) {
		msg->WriteVec3( GetBounceVector() );
	}

	varIsSet = EntityIsSet();
	msg->WriteBool( varIsSet );
	if( varIsSet ) {
		msg->WriteBits( GetEntity()->entityNumber, GENTITYNUM_BITS );
	}

	varIsSet = BindBoneIsSet();
	msg->WriteBool( varIsSet );
	if( varIsSet ) {
		msg->WriteString( GetBindBone() );
	}

	msg->WriteBool( StartIsSet() );

	msg->WriteBool( RemoveWhenDone() );

	msg->WriteBool( Toggle() );

	msg->WriteBool( OnlyVisibleInSpirit() );
	msg->WriteBool( OnlyInvisibleInSpirit() );
	msg->WriteBool( UseWeaponDepthHack() );
	msg->WriteBool( NoRemoveWhenUnbound() );
}

ID_INLINE void hhFxInfo::ReadFromBitMsg( const idBitMsg* msg ) {
	bool varIsSet = false;
	
	varIsSet = msg->ReadBool();
	if( varIsSet ) {
		SetNormal( msg->ReadDir(24) );
	}

	varIsSet = msg->ReadBool();
	if( varIsSet ) {
		SetIncomingVector( msg->ReadDir(24) );
	}

	varIsSet = msg->ReadBool();
	if( varIsSet ) {
		SetBounceVector( msg->ReadVec3() );
	}

	varIsSet = msg->ReadBool();
	if( varIsSet ) {
		SetEntity( gameLocal.entities[ msg->ReadBits(GENTITYNUM_BITS) ] );
	}

	varIsSet = msg->ReadBool();
	if( varIsSet ) {
		char boneName[256];
		msg->ReadString( boneName, 256 );
		SetBindBone( boneName );
	}

	SetStart( msg->ReadBool() );

	RemoveWhenDone( msg->ReadBool() );

	Toggle( msg->ReadBool() );

	OnlyVisibleInSpirit( msg->ReadBool() );
	OnlyInvisibleInSpirit( msg->ReadBool() );
	UseWeaponDepthHack( msg->ReadBool() );
	NoRemoveWhenUnbound( msg->ReadBool() );
}

#endif