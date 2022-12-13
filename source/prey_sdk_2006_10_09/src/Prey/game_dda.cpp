/*
===============================================================================
game_dda.cpp

	This contains the generic functionality for the dynamic difficulty adjustment system,
		as well as a statistic tracking system.	
===============================================================================
*/

// HEADER FILES ---------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


//=============================================================================
//
// hhDDAManager::hhDDAManager
//
//=============================================================================

hhDDAManager::hhDDAManager() {
	bForcedDifficulty	= false;
	difficulty			= 0.5f;

	ClearTracking();
}

//=============================================================================
//
// hhDDAManager::~hhDDAManager
//
//=============================================================================

hhDDAManager::~hhDDAManager() {
	ClearTracking();
}

//=============================================================================
//
// hhDDAManager::Save
//
//=============================================================================

void hhDDAManager::Save(idSaveGame *savefile) const {
	int			i;
	int			num;

	savefile->WriteBool( bForcedDifficulty );
	savefile->WriteFloat( difficulty );

	for ( i = 0; i < DDA_INDEX_MAX; i++ ) {
		ddaProbability[i].Save( savefile );
	}

	savefile->WriteStringList( locationNameData );
	savefile->WriteStringList( locationData );

	num = healthData.Num();
	savefile->WriteInt( num );
	for( i = 0; i < num; i++ ) {
		savefile->WriteInt( healthData[i] );
	}

	savefile->WriteStringList( healthSpiritData );
	savefile->WriteStringList( ammoData );
	savefile->WriteStringList( miscData );

	num = deathData.Num();
	savefile->WriteInt( num );
	for( i = 0; i < num; i++ ) {
		savefile->WriteString( deathData[i].location );
		savefile->WriteInt( deathData[i].time );
	}
}

//=============================================================================
//
// hhDDAManager::Restore
//
//=============================================================================

void hhDDAManager::Restore( idRestoreGame *savefile ) {
	int			i;
	int			num;

	savefile->ReadBool( bForcedDifficulty );
	savefile->ReadFloat( difficulty );

	for ( i = 0; i < DDA_INDEX_MAX; i++ ) {
		ddaProbability[i].Restore( savefile );
	}

	savefile->ReadStringList( locationNameData );
	savefile->ReadStringList( locationData );

	savefile->ReadInt( num );
	healthData.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadInt( healthData[i] );
	}

	savefile->ReadStringList( healthSpiritData );
	savefile->ReadStringList( ammoData );
	savefile->ReadStringList( miscData );

	savefile->ReadInt( num );
	deathData.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadString( deathData[i].location );
		savefile->ReadInt( deathData[i].time );
	}
}

//=============================================================================
//
// hhDDAManager::ClearTracking
//
//=============================================================================

void hhDDAManager::ClearTracking() {
	locationNameData.Clear();
	locationData.Clear();
	healthData.Clear();
	healthSpiritData.Clear();
	ammoData.Clear();
	miscData.Clear();
}

//=============================================================================
//
// hhDDAManager::GetDifficulty
//
//=============================================================================

float hhDDAManager::GetDifficulty() {
	return difficulty;
}

//=============================================================================
//
// hhDDAManager::RecalculateDifficulty
//
// Recalculate overall difficulty from the individual creature difficulties
//
// Must be done after any damaged are added or deaths are recorded
//=============================================================================

void hhDDAManager::RecalculateDifficulty( int updateFlags ) {
	int count = 0;
	float accum = 0.0f;

	float oldDiff = difficulty; // TEMP

	if ( g_wicked.GetBool() ) { // Wicked mode, force the DDA to 1.0
		difficulty = 1.0f;
		return;
	}		

	if ( !g_useDDA.GetBool() ) { // Skip the DDA calculations.  After g_wicked so that is still a valid mode
		difficulty = 0.5f;
		return;
	}

	if ( bForcedDifficulty ) { // Don't recalculate if the difficulty is forced
		return;
	}

	for( int i = 0; i < DDA_INDEX_MAX; i++ ) {
		if ( !ddaProbability[i].IsUsed() ) { // Ignore any difficulties that shouldn't count yet
			continue;
		}

		if ( (1 << i ) & updateFlags ) { // Only update the difficulty based upon the creatures currently attacking
			accum += ddaProbability[i].GetIndividualDifficulty();
			count++;
		}
	}

	if ( count == 0 ) { // No individual difficulties, so return the default
		difficulty = 0.5f;
	} else {
		difficulty = accum / count;
	}

	if ( count && g_printDDA.GetBool() ) {
		common->Printf( "difficulty: [%.2f]\n", difficulty );
	}
}

//=============================================================================
//
// hhDDAManager::ForceDifficulty
//
// Force the difficulty to a given value -- used for debugging and Wicked mode
//=============================================================================

void hhDDAManager::ForceDifficulty( float newDifficulty ) {
	if ( newDifficulty > 1.0f ) { // Clamp the max
		newDifficulty = 1.0f;
	} else if ( newDifficulty < 0.0f ) { // negative difficulty values resets the difficulty
		common->Printf( "Difficulty defaulted back to 0.5\n" );
		difficulty = 0.5f;
		bForcedDifficulty = false;
		return;
	}

	bForcedDifficulty = true;
	difficulty = newDifficulty;
}

//=============================================================================
//
// hhDDAManager::DDA_Heartbeat
//
//=============================================================================

void hhDDAManager::DDA_Heartbeat( hhPlayer* player ) {
	idStr		locText;
	assert( player );

	if ( !g_trackDDA.GetBool() || gameLocal.isMultiplayer ) {
		return;
	}

	ammo_t ammoType;
	ammoType							= idWeapon::GetAmmoNumForName( "ammo_rifle" );
	int ammo_rifle_count				= player->inventory.ammo[ ammoType ];
	float ammo_rifle					= 100 * player->inventory.AmmoPercentage( player, ammoType );

	ammoType							= idWeapon::GetAmmoNumForName( "ammo_sniper" );
	int ammo_sniper_count				= player->inventory.ammo[ ammoType ];
	float ammo_sniper					= 100 * player->inventory.AmmoPercentage( player, ammoType );

	ammoType							= idWeapon::GetAmmoNumForName( "ammo_crawler" );
	int ammo_crawler_count				= player->inventory.ammo[ ammoType ];
	float ammo_crawler					= 100 * player->inventory.AmmoPercentage( player, ammoType );

	ammoType							= idWeapon::GetAmmoNumForName( "ammo_autocannon" );
	int ammo_autocannon_count			= player->inventory.ammo[ ammoType ];
	float ammo_autocannon				= 100 * player->inventory.AmmoPercentage( player, ammoType );

	ammoType							= idWeapon::GetAmmoNumForName( "ammo_autocannon_grenade" );
	int ammo_autocannon_grenade_count	= player->inventory.ammo[ ammoType ];
	float ammo_autocannon_grenade		= 100 * player->inventory.AmmoPercentage( player, ammoType );

	ammoType							= idWeapon::GetAmmoNumForName( "ammo_acid" );
	int	ammo_acid_count					= player->inventory.ammo[ ammoType ];
	float ammo_acid						= 100 * player->inventory.AmmoPercentage( player, ammoType );

	ammoType							= idWeapon::GetAmmoNumForName( "ammo_crawler_red" );
	int ammo_crawler_red_count			= player->inventory.ammo[ ammoType ];
	float ammo_crawler_red				= 100 * player->inventory.AmmoPercentage( player, ammoType );

	ammoType							= idWeapon::GetAmmoNumForName( "ammo_energy" );
	int ammo_energy_count				= player->inventory.ammo[ ammoType ];
	float ammo_energy					= 100 * player->inventory.AmmoPercentage( player, ammoType );


	idStr location;
	player->GetLocationText( location );

	idVec3 playerOrigin = player->GetOrigin();
	
	bool bTalonAttack = false;
	if ( player->talon.IsValid() && player->talon->IsAttacking() ) {
		bTalonAttack = true;
	}

	locationNameData.Append( location );

	locationData.Append( idStr( va( "%.2f %.2f %.2f", playerOrigin.x, playerOrigin.y, playerOrigin.z ) ) );

	healthData.Append( player->GetHealth() );

	healthSpiritData.Append( idStr( va( "%d, %d, %.2f, %.2f", player->GetHealth(), player->GetSpiritPower(), player->ddaProbabilityAccum, gameLocal.GetDDA()->GetDifficulty() ) ) );

	ammoData.Append( idStr( va( "%d, %d, %d, %d, %d, %d, %d, %d, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f", 
		ammo_rifle_count, ammo_sniper_count, ammo_crawler_count, ammo_autocannon_count, ammo_autocannon_grenade_count, ammo_acid_count, ammo_crawler_red_count, ammo_energy_count, // Ammo totals
		ammo_rifle, ammo_sniper, ammo_crawler, ammo_autocannon, ammo_autocannon_grenade, ammo_acid, ammo_crawler_red, ammo_energy ) ) ); // Ammo percentages

	miscData.Append( idStr( va( "%d, %d, %d, %d, %d", player->GetCurrentWeapon(), player->IsLighterOn(), bTalonAttack, player->IsSpiritOrDeathwalking(), player->ddaNumEnemies ) ) ); // currentWeapon, lighter, talon attacking, IsSpiritOrDeathwalking, numEnemies
}

//=============================================================================
//
// hhDDAManager::DDA_AddDamage
//
//=============================================================================

void hhDDAManager::DDA_AddDamage( int ddaIndex, int damage ) {
	if ( ddaIndex < 0 || ddaIndex >= DDA_INDEX_MAX ) {
		common->Warning( "hhDDAManager::DDA_AddDamage:  ddaIndex out of range %d [%d]\n", ddaIndex, DDA_INDEX_MAX );
		ddaIndex = 0;
	}

	ddaProbability[ddaIndex].AddDamage( damage * 2 );
}

//=============================================================================
//
// hhDDAManager::DDA_AddSurvivalHealth
//
//=============================================================================

void hhDDAManager::DDA_AddSurvivalHealth( int ddaIndex, int health ) {
	if ( ddaIndex < 0 || ddaIndex >= DDA_INDEX_MAX ) {
		common->Warning( "hhDDAManager::DDA_AddSurvivalHealth:  ddaIndex out of range %d [%d]\n", ddaIndex, DDA_INDEX_MAX );
		ddaIndex = 0;
	}

	ddaProbability[ddaIndex].AddSurvivalValue( health );

	// Adjust DDA test
	float prob = DDA_GetProbability( ddaIndex, health );
	float probAdjust = ( prob - 0.5f ) * 0.05f; // Difficulty adjustment raises and lowers based upon how far the probability is from the dda level

	ddaProbability[ddaIndex].AdjustDifficulty( probAdjust );
}

//=============================================================================
//
// hhDDAManager::DDA_AddDeath
//
//=============================================================================

void hhDDAManager::DDA_AddDeath( hhPlayer *player, idEntity *attacker ) {
	hhMonsterAI		*monster;
	int				ddaIndex;

	if ( g_trackDDA.GetBool() && !gameLocal.isMultiplayer ) {
		ddaDeath_t death;
		death.time = gameLocal.GetTime();
		player->GetLocationText( death.location );
		deathData.Append( death );
	}

	if ( !attacker->IsType( hhMonsterAI::Type ) ) {
		return;
	}

	monster = static_cast<hhMonsterAI *>(attacker);

	if ( monster ) {
		ddaIndex = monster->spawnArgs.GetInt( "ddaIndex", "0" );

		if ( ddaIndex < 0 ) { // Player could be killed by monsters that shouldn't be included in the DDA
			return;
		}

		if ( ddaIndex >= DDA_INDEX_MAX ) {
			ddaIndex = 0;
		}
	}

	ddaProbability[ddaIndex].AdjustDifficulty( -0.075f );
}

//=============================================================================
//
// hhDDAManager::DDA_GetProbability
//
// Return the probability that this index of creature will do enough damage to reduce the player's health to zero
//=============================================================================

float hhDDAManager::DDA_GetProbability( int ddaIndex, int value ) {
	if ( ddaIndex < 0 || ddaIndex >= DDA_INDEX_MAX ) {
		common->Warning( "hhDDAManager::DDA_GetProbability:  ddaIndex out of range %d [%d]\n", ddaIndex, DDA_INDEX_MAX );
		ddaIndex = 0;
	}

	return ddaProbability[ddaIndex].GetProbability( value );
}

//=============================================================================
//
// hhDDAManager::Export
//
//=============================================================================

void hhDDAManager::Export( const char* filename ) {
	idStr ExportFile, ExportBase;

	if( !filename ) {
		ExportBase = gameLocal.GetMapName();
		ExportBase.StripPath();
		ExportBase.StripFileExtension();
		ExportBase = va("%s_%s", cvarSystem->GetCVarString("win_username"), ExportBase.c_str() );
		ExportBase.DefaultPath( "statfiles/" );
	}
	else {
		ExportBase = va( "%s_", filename );
		ExportBase.StripFileExtension();
		ExportBase.DefaultPath( "statfiles/" );
	}

	gameLocal.Printf( "Exporting stats to csv files\n" );
	
	// Export to .cvs files and to a .lin file
	ExportDDAData( ExportBase, "Location Name, Health, Spirit Power, Survival Chance, DDA", "_HEALTH", healthSpiritData );

	ExportDDAData( ExportBase, 
		"Location Name, Ammo Rifle, Ammo Sniper, Ammo Crawler, Ammo Autocannon, Ammo Grenade, Ammo Acid, Ammo Launcher, Ammo Energy, Ammo Rifle %, Ammo Sniper %, Ammo Crawler %, Ammo Autocannon %, Ammo Grenade %, Ammo Acid %, Ammo Launcher %, Ammo Energy %",
		"_AMMO", ammoData );

	ExportDDAData( ExportBase, "Location Name, Current Weapon, Lighter On, Talon Attacking, IsSpiritOrDeathwalking, Num Enemies", "_MISC", miscData );

	ExportLINData( ExportBase );
}

//=============================================================================
//
// hhDDAManager::ExportDDAData
//
//=============================================================================

void hhDDAManager::ExportDDAData( idStr fileName, const char* header, const char *fileAddition, idList<idStr> data ) {
	idFile* f = NULL;

	//Build the filename
	fileName.Append( fileAddition );

	fileName.DefaultFileExtension("csv");
	f = fileSystem->OpenFileWrite( fileName.c_str() );
	if( !f ) {
		common->Warning( "Failed to open stat tracking file '%s'", fileName.c_str() );
		return;
	}

	//Export the header for this file...
	f->Printf( "%s\n", header );

	for( int i = 0; i < data.Num(); i++ ) {
		f->Printf( "%s, %s\n", locationNameData[i].c_str(), data[i].c_str() );
	}

	//Close the output file
	fileSystem->CloseFile( f );
}

//=============================================================================
//
// hhDDAManager::ExportLINData
//
//=============================================================================

void hhDDAManager::ExportLINData( idStr fileName ) {
	idFile* f = NULL;

	//Build the filename
	fileName.DefaultFileExtension("lin");
	f = fileSystem->OpenFileWrite( fileName.c_str() );
	if( !f ) {
		common->Warning( "Failed to open stat tracking file '%s'", fileName.c_str() );
		return;
	}

	for( int i = 0; i < locationData.Num(); i++ ) {
		if ( healthData[i] > 0 ) { // Only store living health values in the .lin file
			f->Printf( "%s\n", locationData[i].c_str() );
		}
	}

	//Close the output file
	fileSystem->CloseFile( f );
}

//=============================================================================
//
// hhDDAManager::PrintDDA
//
//=============================================================================

void hhDDAManager::PrintDDA( void ) {
	int					i;
	hhDDAProbability	*prob;

	for( i = 0; i < DDA_INDEX_MAX; i++ ) {
		prob = &ddaProbability[i];
		if ( !prob->IsUsed() ) {
			continue;
		}

		common->Printf( "DDA Index: %d\n", i );
		common->Printf( "Ind. Difficulty: %.2f\n", prob->GetIndividualDifficulty() );
/*
		common->Printf( "Last Damages: [%.1f] [%.1f] [%.1f] [%.1f] [%.1f] [%.1f] [%.1f] [%.1f]\n",
			prob->damages[0], prob->damages[1], prob->damages[2], prob->damages[3], 
			prob->damages[4], prob->damages[5], prob->damages[6], prob->damages[7] );
*/
	}
}

//=============================================================================
// NEW DDA
//=============================================================================

//=============================================================================
//
// hhDDAProbability::hhDDAProbability
//
//=============================================================================

hhDDAProbability::hhDDAProbability() {
	damageRover = 0;
	survivalRover = 0;

	mean = 25.0f;
	stdDeviation = 1.0f;
	bUsed = false;
	individualDifficulty = 0.5f;

	for( int i = 0; i < NUM_DAMAGES; i++ ) {
		damages[i] = mean; // Initialize to a low, but reasonable value
		survivalValues[i] = 0.5f; // Initialize to a middle value
	}
}

//=============================================================================
//
// hhDDAProbability::Save
//
//=============================================================================

void hhDDAProbability::Save( idSaveGame *savefile ) const {
	int			i;

	savefile->WriteBool( bUsed );

	for ( i = 0; i < NUM_DAMAGES; i++ ) {
		savefile->WriteInt( damages[i] );
	}

	savefile->WriteInt( damageRover );

	for ( i = 0; i < NUM_DAMAGES; i++ ) {
		savefile->WriteFloat( survivalValues[i] );
	}

	savefile->WriteInt( survivalRover );

	savefile->WriteFloat( mean );
	savefile->WriteFloat( stdDeviation );
	savefile->WriteFloat( individualDifficulty );
}

//=============================================================================
//
// hhDDAProbability::Restore
//
//=============================================================================

void hhDDAProbability::Restore( idRestoreGame *savefile ) {
	int			i;

	savefile->ReadBool( bUsed );

	for ( i = 0; i < NUM_DAMAGES; i++ ) {
		savefile->ReadInt( damages[i] );
	}

	savefile->ReadInt( damageRover );

	for ( i = 0; i < NUM_DAMAGES; i++ ) {
		savefile->ReadFloat( survivalValues[i] );
	}

	savefile->ReadInt( survivalRover );

	savefile->ReadFloat( mean );
	savefile->ReadFloat( stdDeviation );
	savefile->ReadFloat( individualDifficulty );
}

//=============================================================================
//
// hhDDAProbability::AddDamage
//
//=============================================================================

void hhDDAProbability::AddDamage( int damage ) {
	// Add the damage to the list
	damages[ damageRover ] = damage;
	damageRover = ( damageRover + 1 ) % NUM_DAMAGES;

	bUsed = true;

	CalculateProbabilityCurve();
}

//=============================================================================
//
// hhDDAProbability::AddSurvivalValue
//
//=============================================================================

void hhDDAProbability::AddSurvivalValue( int playerHealth ) {
	// Get the survival probability against this creature and store it
	survivalValues[ survivalRover ] = GetProbability( playerHealth );
	survivalRover = ( survivalRover + 1 ) % NUM_DAMAGES;
}

//=============================================================================
//
// hhDDAProbability::GetProbability
//
//=============================================================================

float hhDDAProbability::GetProbability( int value ) {
	float	z;
	float	x;
	float	probability = 0;

	if ( stdDeviation <= 0.0f ) {
		stdDeviation = 1.0f;
	}

	z = idMath::Fabs( value - mean ) / stdDeviation;
	x = 1 + z * ( 0.049867347 + z * ( 0.0211410061 + z * ( 0.0032776263 )));
	probability = ( idMath::Exp( idMath::Log( x ) * -16 ) / 2 );
	if ( value >= mean ) { 
		probability = 1.0f - probability;
	}

	return probability;
}

//=============================================================================
//
// hhDDAProbability::CalculateProbabilityCurve
//
//=============================================================================

void hhDDAProbability::CalculateProbabilityCurve() {
	int			i;
	float		diff;
	float		sumDiffSqr = 0;

	// Recompute mean and std deviation when new damage information is obtained
	mean = 0;
	for ( i = 0; i < NUM_DAMAGES; i++ ) {
		mean += damages[i];
	}
	mean /= NUM_DAMAGES;

	// calculate standard deviation
	for ( i = 0; i < NUM_DAMAGES; i++ ) {
		diff = mean - damages[i];
		sumDiffSqr += diff * diff;
	}

	stdDeviation = idMath::Sqrt( sumDiffSqr / NUM_DAMAGES );
}

//=============================================================================
//
// hhDDAProbability::GetSurvivalMean
//
//=============================================================================

float hhDDAProbability::GetSurvivalMean( void ) {
	float sum = 0;

	for ( int i = 0; i < NUM_DAMAGES; i++ ) {
		sum += survivalValues[i];
	}

	return sum / NUM_DAMAGES;
}

//=============================================================================
//
// hhDDAProbability::AdjustDifficulty
//
//=============================================================================

void hhDDAProbability::AdjustDifficulty( float diff ) {
	individualDifficulty = idMath::ClampFloat( 0.0f, 1.0f, individualDifficulty + diff );
}
