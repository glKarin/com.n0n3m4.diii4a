
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( idPhysics_Static, hhPhysics_StaticWeapon )
END_CLASS

/*
============
hhPhysics_StaticWeapon::hhPhysics_StaticWeapon
============
*/
hhPhysics_StaticWeapon::hhPhysics_StaticWeapon() {
	castSelf = NULL;
	selfOwner = NULL;
}

/*
============
hhPhysics_StaticWeapon::SetSelfOwner
============
*/
void hhPhysics_StaticWeapon::SetSelfOwner( idActor* a ) {
	if( self && self->IsType(hhWeapon::Type) ) {
		castSelf = static_cast<hhWeapon*>( self );
	}

	if( a && a->IsType(hhPlayer::Type) ) {
		selfOwner = static_cast<hhPlayer*>( a );
	}
}

/*
============
hhPhysics_StaticWeapon::SetLocalAxis
============
*/
void hhPhysics_StaticWeapon::SetLocalAxis( const idMat3& newLocalAxis ) {
	current.localAxis = newLocalAxis;
}

/*
============
hhPhysics_StaticWeapon::SetLocalOrigin
============
*/
void hhPhysics_StaticWeapon::SetLocalOrigin( const idVec3& newLocalOrigin ) {
	current.localOrigin = newLocalOrigin;
}

/*
============
hhPhysics_StaticWeapon::Evaluate
============
*/
bool hhPhysics_StaticWeapon::Evaluate( int timeStepMSec, int endTimeMSec ) {
	if( !selfOwner ) {
		return false;
	}

	idMat3 localAxis;
	idVec3 localOrigin( 0.0f, 0.0f, selfOwner->EyeHeight() );
	idAngles pitchAngles( selfOwner->GetUntransformedViewAngles().pitch, 0.0f, 0.0f );

	if( selfOwner->InVehicle() ) {
		localAxis = pitchAngles.ToMat3();
	} else {
		localAxis = ( pitchAngles + selfOwner->GunTurningOffset() ).ToMat3();
		localOrigin += selfOwner->GunAcceleratingOffset();
		if ( castSelf ) {
			castSelf->MuzzleRise( localOrigin, localAxis );
		}
	}

	SetLocalAxis( localAxis );
	SetLocalOrigin( localOrigin );

	return idPhysics_Static::Evaluate( timeStepMSec, endTimeMSec );
}

//================
//hhPhysics_StaticWeapon::Save
//================
void hhPhysics_StaticWeapon::Save( idSaveGame *savefile ) const {
	savefile->WriteObject( castSelf );
	savefile->WriteObject( selfOwner );
}

//================
//hhPhysics_StaticWeapon::Restore
//================
void hhPhysics_StaticWeapon::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<idClass *&>( castSelf ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( selfOwner ) );
}

CLASS_DECLARATION( idPhysics_Static, hhPhysics_StaticForceField )
END_CLASS

/*
================
hhPhysics_StaticForceField::hhPhysics_StaticForceField
================
*/
hhPhysics_StaticForceField::hhPhysics_StaticForceField() {
}

/*
================
hhPhysics_StaticForceField::Evaluate
================
*/
bool hhPhysics_StaticForceField::Evaluate( int timeStepMSec, int endTimeMSec ) {
	bool moved = idPhysics_Static::Evaluate( timeStepMSec, endTimeMSec );

	EvaluateContacts();

	return moved;
}

/*
================
hhPhysics_StaticForceField::AddContactEntitiesForContacts
================
*/
void hhPhysics_StaticForceField::AddContactEntitiesForContacts( void ) {
	int i;
	idEntity *ent;

	for ( i = 0; i < contacts.Num(); i++ ) {
		ent = gameLocal.entities[ contacts[i].entityNum ];
		if ( ent && ent != self ) {
			ent->AddContactEntity( self );
		}
	}
}

/*
================
hhPhysics_StaticForceField::ActivateContactEntities
================
*/
void hhPhysics_StaticForceField::ActivateContactEntities( void ) {
	int i;
	idEntity *ent;

	for ( i = 0; i < contactEntities.Num(); i++ ) {
		ent = contactEntities[i].GetEntity();
		if ( ent ) {
			ent->ActivatePhysics( self );
		} else {
			contactEntities.RemoveIndex( i-- );
		}
	}
}

/*
================
hhPhysics_StaticForceField::EvaluateContacts
================
*/
bool hhPhysics_StaticForceField::EvaluateContacts( void ) {
	idVec6 dir;
	int num;

	ClearContacts();

	contacts.SetNum( 10, false );

	dir.SubVec3(0).Zero();
	dir.SubVec3(1).Zero();
	//dir.SubVec3(0).Normalize();
	//dir.SubVec3(1).Normalize();
	num = gameLocal.clip.Contacts( &contacts[0], 10, clipModel->GetOrigin(),
					dir, CONTACT_EPSILON, clipModel, clipModel->GetAxis(), MASK_SOLID, self );
	contacts.SetNum( num, false );

	AddContactEntitiesForContacts();

	return ( contacts.Num() != 0 );
}

/*
================
hhPhysics_StaticForceField::GetNumContacts
================
*/
int hhPhysics_StaticForceField::GetNumContacts( void ) const {
	return contacts.Num();
}

/*
================
hhPhysics_StaticForceField::GetContact
================
*/
const contactInfo_t &hhPhysics_StaticForceField::GetContact( int num ) const {
	return contacts[num];
}

/*
================
hhPhysics_StaticForceField::ClearContacts
================
*/
void hhPhysics_StaticForceField::ClearContacts( void ) {
	int i;
	idEntity *ent;

	for ( i = 0; i < contacts.Num(); i++ ) {
		ent = gameLocal.entities[ contacts[i].entityNum ];
		if ( ent ) {
			ent->RemoveContactEntity( self );
		}
	}
	contacts.SetNum( 0, false );
}

/*
================
hhPhysics_StaticForceField::AddContactEntity
================
*/
void hhPhysics_StaticForceField::AddContactEntity( idEntity *e ) {
	int i;
	idEntity *ent;

	for ( i = 0; i < contactEntities.Num(); i++ ) {
		ent = contactEntities[i].GetEntity();
		if ( !ent ) {
			contactEntities.RemoveIndex( i-- );
			continue;
		}
		if ( ent == e ) {
			return;
		}
	}
	contactEntities.Alloc() = e;
}

/*
================
hhPhysics_StaticForceField::RemoveContactEntity
================
*/
void hhPhysics_StaticForceField::RemoveContactEntity( idEntity *e ) {
	int i;
	idEntity *ent;

	for ( i = 0; i < contactEntities.Num(); i++ ) {
		ent = contactEntities[i].GetEntity();
		if ( !ent ) {
			contactEntities.RemoveIndex( i-- );
			continue;
		}
		if ( ent == e ) {
			contactEntities.RemoveIndex( i-- );
			return;
		}
	}
}

//================
//hhPhysics_StaticForceField::Save
//================
void hhPhysics_StaticForceField::Save( idSaveGame *savefile ) const {
	int i;
	savefile->WriteInt( contacts.Num() );
	for ( i = 0; i < contacts.Num(); i++ ) {
		savefile->WriteContactInfo( contacts[i] );
	}

	savefile->WriteInt( contactEntities.Num() );
	for ( i = 0; i < contactEntities.Num(); i++ ) {
		//HUMANHEAD PCF mdl 04/26/06 - Changed '->' to a '.', as it should have been
		contactEntities[i].Save( savefile );
	}
}

//================
//hhPhysics_StaticForceField::Restore
//================
void hhPhysics_StaticForceField::Restore( idRestoreGame *savefile ) {
	int i;
	int num;

	savefile->ReadInt( num );
	contacts.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadContactInfo( contacts[i] );
	}

	savefile->ReadInt( num );
	contactEntities.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		//HUMANHEAD PCF mdl 04/26/06 - Changed '->' to a '.', as it should have been
		contactEntities[i].Restore( savefile );
	}
}

