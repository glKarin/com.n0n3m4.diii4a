// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DemoAnalyzer.h"
#include "../structures/TeamManager.h"
#include "../Player.h"
#include "../vehicles/Transport.h"

idCVar sdDemoAnalyzer::g_demoAnalysisSectorSize( "g_demoAnalysisSectorSize", "64", CVAR_SYSTEM | CVAR_INTEGER, "sector size for stat generation" );	

/*
================
sdDemoAnalyzer::sdDemoAnalyzer
================
*/
sdDemoAnalyzer::sdDemoAnalyzer() :
	sectors( NULL ) {
}

/*
================
sdDemoAnalyzer::Start
================
*/
void sdDemoAnalyzer::Start() {
	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();

	int numTeams = manager.GetNumTeams();
	int* numClasses = (int *)_alloca( numTeams * sizeof( int ) );

	memset( numClasses, 0, numTeams * sizeof( int ) );

	int numTotalClasses = gameLocal.declPlayerClassType.Num();

	classIndexToTeamClassIndex = new int[ numTotalClasses ];

	for ( int i = 0; i < numTotalClasses; i++ ) {
		const sdDeclPlayerClass* pc = gameLocal.declPlayerClassType[ i ];

		for ( int j = 0; j < numTeams; j++ ) {
			if ( pc->GetTeam() == &manager.GetTeamByIndex( j ) ) {
				classIndexToTeamClassIndex[ i ] = numClasses[ j ];
				numClasses[ j ]++;
			}
		}
	}

	teamClassIndexToClassIndex = new int*[ numTeams ];
	for ( int i = 0; i < numTeams; i++ ) {
		int numTeamClasses = 0;

		teamClassIndexToClassIndex[ i ] = new int[ numClasses[ i ] ];

		for ( int j = 0; j < numTotalClasses; j++ ) {
			const sdDeclPlayerClass* pc = gameLocal.declPlayerClassType[ j ];

			if ( pc->GetTeam() == &manager.GetTeamByIndex( i ) ) {
				teamClassIndexToClassIndex[ i ][ numTeamClasses++ ] = j;
			}
		}
	}

	// generate grid
	sectorSize = g_demoAnalysisSectorSize.GetInteger();
	sectorDimX = ( gameLocal.clip.GetWorldBounds().GetMaxs().x - gameLocal.clip.GetWorldBounds().GetMins().x ) / sectorSize;
	sectorDimY = ( gameLocal.clip.GetWorldBounds().GetMaxs().y - gameLocal.clip.GetWorldBounds().GetMins().y ) / sectorSize;

	sectors = new sector_t[ sectorDimX * sectorDimY ];

	for ( int y = 0; y < sectorDimY; y++ ) {
		for ( int x = 0; x < sectorDimX; x++ ) {
			sector_t* sector = &sectors[ y * sectorDimX + x ];

			// allocate player team/class stat storage
			sector->playerTeamClassStats = new playerTeamClassStats_t*[ numTeams ];
			for ( int i = 0; i < numTeams; i++ ) {
				sector->playerTeamClassStats[i] = new playerTeamClassStats_t[ numClasses[ i ] ];
				memset( sector->playerTeamClassStats[i], 0, sizeof( playerTeamClassStats_t ) * numClasses[ i ] );
			}
		}
	}

	memset( playerHasDied, 0, sizeof( playerHasDied ) );

	maxPlayerLocation = 0;
	maxTransportLocation = 0;
	maxPlayerDeath = 0;
}

/*
================
sdDemoAnalyzer::Stop
================
*/
void sdDemoAnalyzer::Stop() {
	byte* pic = new byte[ sectorDimX * sectorDimY * 4 ];

	// figure out team and class counts
	sdTeamManagerLocal& manager = sdTeamManager::GetInstance();

	int numTeams = manager.GetNumTeams();
	int* numClasses = (int *)_alloca( numTeams * sizeof( int ) );

	memset( numClasses, 0, numTeams * sizeof( int ) );

	int numTotalClasses = gameLocal.declPlayerClassType.Num();
	for ( int i = 0; i < numTotalClasses; i++ ) {
		const sdDeclPlayerClass* pc = gameLocal.declPlayerClassType[ i ];

		for ( int j = 0; j < numTeams; j++ ) {
			if ( pc->GetTeam() == &manager.GetTeamByIndex( j ) ) {
				numClasses[ j ]++;
			}
		}
	}

	// write player location/death images
	for ( int i = 0; i < numTeams; i++ ) {
		for ( int j = 0; j < numClasses[ i ]; j++ ) {
			memset( pic, 0, sizeof( byte ) * sectorDimX * sectorDimY * 4 );

			for ( int y = 0; y < sectorDimY; y++ ) {
				for ( int x = 0; x < sectorDimX; x++ ) {
					sector_t* sector = &sectors[ y * sectorDimX + x ];

					byte b;

					if ( sector->playerTeamClassStats[i][j].playerLocation > 0 ) {
						b = Max( (byte)0x01, idMath::Ftob( ( sector->playerTeamClassStats[i][j].playerLocation / (float)maxPlayerLocation ) * 255.0f ) );
					} else {
						b = 0x00;
					}
					pic[ ( ( sectorDimY - 1 - y ) * sectorDimX * 4 ) + x * 4 + 0 ] = b;

					if ( sector->playerTeamClassStats[i][j].playerTransportLocation > 0 ) {
						b = Max( (byte)0x01, idMath::Ftob( ( sector->playerTeamClassStats[i][j].playerTransportLocation / (float)maxTransportLocation ) * 255.0f ) );
					} else {
						b = 0x00;
					}
					pic[ ( ( sectorDimY - 1 - y ) * sectorDimX * 4 ) + x * 4 + 1 ] = b;

					if ( sector->playerTeamClassStats[i][j].playerDeath > 0 ) {
						b = Max( (byte)0x01, idMath::Ftob( ( sector->playerTeamClassStats[i][j].playerDeath / (float)maxPlayerDeath ) * 255.0f ) );
					} else {
						b = 0x00;
					}
					pic[ ( ( sectorDimY - 1 - y ) * sectorDimX * 4 ) + x * 4 + 2 ] = b;

					pic[ ( ( sectorDimY - 1 - y ) * sectorDimX * 4 ) + x * 4 + 3 ] = 0x00;
				}
			}

			fileSystem->WriteTGA( va( "imagedump/demo_analysis/%s/loc_%s_%s.tga",
										networkSystem->GetDemoName(),
										manager.GetTeamByIndex( i ).GetLookupName(),
										gameLocal.declPlayerClassType[ teamClassIndexToClassIndex[ i ][ j ] ]->GetName() ),
									pic, sectorDimX, sectorDimY );
		}
	}

	// clean up
	delete [] pic;

	for ( int y = 0; y < sectorDimY; y++ ) {
		for ( int x = 0; x < sectorDimX; x++ ) {
			sector_t* sector = &sectors[ y * sectorDimX + x ];

			// free player location storage
			for ( int i = 0; i < numTeams; i++ ) {
				delete [] sector->playerTeamClassStats[ i ];
			}
			delete [] sector->playerTeamClassStats;
		}
	}

	delete [] classIndexToTeamClassIndex;

	for ( int i = 0; i < numTeams; i++ ) {
		delete [] teamClassIndexToClassIndex[ i ];
	}
	delete [] teamClassIndexToClassIndex;

	delete [] sectors;
	sectors = NULL;
}

/*
================
sdDemoAnalyzer::RunFrame
================
*/
void sdDemoAnalyzer::RunFrame() {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		if ( player->GetTeam() == NULL || player->GetInventory().GetClass() == NULL ) {
			continue;
		}

		sector_t* sector = GetSector( player->GetPhysics()->GetOrigin() );

		int teamIndex = player->GetTeam()->GetIndex();
		int classIndex = classIndexToTeamClassIndex[ player->GetInventory().GetClass()->Index() ];

		// log kill
		if ( playerHasDied[ player->entityNumber ] ) {
			playerHasDied[ player->entityNumber ] = false;
			sector->playerTeamClassStats[teamIndex][classIndex].playerDeath++;
			maxPlayerDeath = Max( maxPlayerDeath, sector->playerTeamClassStats[teamIndex][classIndex].playerDeath );
		}

		if ( player->IsDead() || player->IsSpectator() ) {
			continue;
		}

		// log movement location
		sdTransport* transport = player->GetProxyEntity()->Cast< sdTransport >();

		if ( transport!= NULL ) {
			sector->playerTeamClassStats[teamIndex][classIndex].playerTransportLocation++;
			maxTransportLocation = Max( maxTransportLocation, sector->playerTeamClassStats[teamIndex][classIndex].playerTransportLocation );
		} else {
			sector->playerTeamClassStats[teamIndex][classIndex].playerLocation++;
			maxPlayerLocation = Max( maxPlayerLocation, sector->playerTeamClassStats[teamIndex][classIndex].playerLocation );
		}
	}
}

/*
================
sdDemoAnalyzer::GetSector
================
*/
sdDemoAnalyzer::sector_t* sdDemoAnalyzer::GetSector( const idVec3& origin ) {
	int sectorX = idMath::Ftoi( idMath::Floor( ( origin.x - gameLocal.clip.GetWorldBounds().GetMins().x ) / sectorSize ) );
	int sectorY = idMath::Ftoi( idMath::Floor( ( origin.y - gameLocal.clip.GetWorldBounds().GetMins().y ) / sectorSize ) );

	if ( sectorX < 0 || sectorX >= sectorDimX ) {
		return NULL;
	}
	if ( sectorY < 0 || sectorY >= sectorDimY ) {
		return NULL;
	}

	return &sectors[ sectorY * sectorDimX + sectorX ];
}

/*
================
sdDemoAnalyzer::LogPlayerDeath
================
*/
void sdDemoAnalyzer::LogPlayerDeath( idPlayer* player, idEntity* inflictor, idEntity* attacker ) {
	if ( !IsActive() ) {
		return;
	}

	playerHasDied[ player->entityNumber ] = true;
}