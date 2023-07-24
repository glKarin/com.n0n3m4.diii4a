// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "LimboProperties.h"
#include "../idlib/PropertiesImpl.h"
#include "Player.h"
#include "rules/GameRules.h"
#include "Waypoints/LocationMarker.h"

/*
===============================================================================

	sdLimboProperties

===============================================================================
*/

/*
============
sdLimboProperties::sdLimboProperties
============
*/
sdLimboProperties::sdLimboProperties( void ) :
	proficiencySource( NULL ) {
}

/*
============
sdLimboProperties::~sdLimboProperties
============
*/
sdLimboProperties::~sdLimboProperties( void ) {
}

/*
============
sdLimboProperties::GetProperty
============
*/
sdProperties::sdProperty* sdLimboProperties::GetProperty( const char* name ) {
	return properties.GetProperty( name, sdProperties::PT_INVALID, false );
}

/*
============
sdLimboProperties::GetProperty
============
*/
sdProperties::sdProperty* sdLimboProperties::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	sdProperties::sdProperty* prop = properties.GetProperty( name, sdProperties::PT_INVALID, false );
	if ( prop && prop->GetValueType() != type && type != sdProperties::PT_INVALID ) {
		gameLocal.Error( "sdLimboProperties::GetProperty: type mismatch for property '%s'", name );
	}
	return prop;
}

/*
============
sdLimboProperties::Init
============
*/
void sdLimboProperties::Init( void ) {
	properties.RegisterProperty( "role",			role );
	properties.RegisterProperty( "name",			name );
	properties.RegisterProperty( "weaponIndex",		weaponIndex );
	properties.RegisterProperty( "teamName",		teamName );
	properties.RegisterProperty( "rank",			rank );
	properties.RegisterProperty( "rankMaterial",	rankMaterial );
	properties.RegisterProperty( "xp",				xp );
	properties.RegisterProperty( "matchTime",		matchTime );
	properties.RegisterProperty( "availablePlayZones",		availablePlayZones );
	properties.RegisterProperty( "defaultPlayZone",			defaultPlayZone );
	properties.RegisterProperty( "spawnLocation",			spawnLocation );


	int numProficiency = gameLocal.declProficiencyTypeType.Num();
	proficiency.SetNum( numProficiency );
	proficiencyID.SetNum( numProficiency );
	proficiencyXP.SetNum( numProficiency );
	proficiencyTitle.SetNum( numProficiency );
	proficiencyLevels.SetNum( numProficiency );
	proficiencyPercent.SetNum( numProficiency );
	proficiencyName.SetNum( numProficiency );

	for( int i = 0; i < numProficiency; i++ ) {
		properties.RegisterProperty( va( "proficiency%i", i ),			proficiency[ i ] );
		properties.RegisterProperty( va( "proficiencyXP%i", i ),		proficiencyXP[ i ] );
		properties.RegisterProperty( va( "proficiencyID%i", i ),		proficiencyID[ i ] );
		properties.RegisterProperty( va( "proficiencyPercent%i", i ),	proficiencyPercent[ i ] );
		properties.RegisterProperty( va( "proficiencyTitle%i", i ),		proficiencyTitle[ i ] );
		properties.RegisterProperty( va( "proficiencyLevels%i", i ),	proficiencyLevels[ i ] );
		properties.RegisterProperty( va( "proficiencyName%i", i ),		proficiencyName[ i ] );
	}	
}


/*
============
sdLimboProperties::Shutdown
============
*/
void sdLimboProperties::Shutdown( void ) {
	properties.Clear();
}

/*
============
sdLimboProperties::Update
============
*/
void sdLimboProperties::Update( void ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( !player ) {
		return;
	}

	availablePlayZones = gameLocal.GetNumChoosablePlayZones();
	defaultPlayZone = gameLocal.GetIndexForChoosablePlayZone( gameLocal.GetPlayZone( player->GetPhysics()->GetOrigin(), sdPlayZone::PZF_COMMANDMAP ) );

	if( const sdTeamInfo* team = player->GetGameTeam() ) {
		teamName = team->GetLookupName();
	} else {
		teamName = "spec";
	}

	const sdInventory& inv = player->GetInventory();

	name			= player->GetUserInfo().rawName;
	
	if( inv.GetCachedClass() != NULL ) {
		role = inv.GetCachedClass()->GetName();
		if( inv.GetCachedClassOptions().Num() > 0 ) {
			weaponIndex = inv.GetCachedClassOption( 0 );
		} else {			
			weaponIndex = 0;
		}
	} else if( inv.GetClass() != NULL ) {
		role = inv.GetClass()->GetName();
		if( inv.GetClassOptions().Num() > 0 ) {
			weaponIndex = inv.GetClassOption( 0 );
		} else {			
			weaponIndex = 0;
		}
	} else {
		role = "spectating";
		weaponIndex = 0;
	}

	// experience
	xp = idMath::Floor( player->GetProficiencyTable().GetXP() );

	const sdDeclRank* playerRank = player->GetProficiencyTable().GetRank();
	if ( playerRank != NULL ) {
		rank			= playerRank->GetTitle() != NULL ? playerRank->GetTitle()->Index() : -1;
		rankMaterial	= playerRank->GetMaterial();
	} else {
		rank = declHolder.declLocStrType.LocalFind( "blank" )->Index();
		rankMaterial = "nodraw";
	}

	if( gameLocal.rules != NULL ) {
		matchTime		= gameLocal.rules->GetGameTime();
	}	

	UpdateProficiency( player, proficiencySource );
}

/*
============
sdLimboProperties::UpdateProficiency
============
*/
void sdLimboProperties::UpdateProficiency( idPlayer* player, const sdDeclPlayerClass* pc ) {
	if ( pc != NULL ) {		
		const sdProficiencyTable& table = player->GetProficiencyTable();
		const sdTeamInfo* teamInfo = pc->GetTeam();	

		for( int i = 0; i < pc->GetNumProficiencies(); i++ ) {
			const sdDeclPlayerClass::proficiencyCategory_t& category = pc->GetProficiency( i );

			int profIndex = category.index;
			proficiencyID[ i ] = profIndex;
			proficiency[ i ] = static_cast< float >( table.GetLevel( profIndex ) );

			const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType.LocalFindByIndex( profIndex );
			const sdDeclLocStr* title = declHolder.declLocStrType.LocalFind( teamInfo->GetDict().GetString( va( "prof_%s", prof->GetLookupTitle() ) ) ); 
			proficiencyTitle[ i ]	= title->Index();
			proficiencyLevels[ i ]	= prof->GetNumLevels();
			proficiencyName[ i ]	= prof->GetName();
			proficiencyPercent[ i ]	= table.GetPercent( profIndex );
			proficiencyXP[ i ]		= table.GetPoints( profIndex );
		}
	} else {
		for( int i = 0; i < gameLocal.declProficiencyTypeType.Num(); i++ ) {
			proficiency[ i ]		= 0.0f;
			proficiencyPercent[ i ] = 0.0f;
			proficiencyTitle[ i ]	= -1;
			proficiencyLevels[ i  ]	= 0.0f;
		}
	}
}


/*
============
sdLimboProperties::SetProficiencySource
============
*/
void sdLimboProperties::SetProficiencySource( const char* className ) {
	proficiencySource = NULL;
	if( idStr::Length( className ) > 0 ) {
		proficiencySource = gameLocal.declPlayerClassType.LocalFind( className, false );
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL ) {
		UpdateProficiency( player, proficiencySource );
	}
}

/*
============
sdLimboProperties::OnSetActiveSpawn
============
*/
void sdLimboProperties::OnSetActiveSpawn( idEntity* newSpawn ) {
	if( newSpawn == NULL ) {
		spawnLocation = common->LocalizeText( "guis/mainmenu/defaultspawn" );
	} else {
		idWStr str;
		sdLocationMarker::GetLocationText( newSpawn->GetPhysics()->GetOrigin(), str );
		spawnLocation = str;
	}
}
