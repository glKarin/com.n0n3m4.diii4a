//----------------------------------------------------------------
// IconManager.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "IconManager.h"

rvIconManager	iconManagerLocal;
rvIconManager*	iconManager = &iconManagerLocal;

/*
===============================================================================

	rvIconManager

===============================================================================
*/

void rvIconManager::AddIcon( int clientNum, const char* iconName ) {
	assert( gameLocal.GetLocalPlayer() );

	idPlayer* player = gameLocal.GetLocalPlayer();
  
  	icons[ clientNum ].Append( rvPair<rvIcon*, int>(new rvIcon(), gameLocal.time + ICON_STAY_TIME) );
  	icons[ clientNum ][ icons[ clientNum ].Num() - 1 ].First()->CreateIcon( player->spawnArgs.GetString( iconName ), (clientNum == gameLocal.localClientNum ? gameLocal.localClientNum + 1 : 0) );
}

void rvIconManager::UpdateIcons( void ) {
	if( gameLocal.GetLocalPlayer() == NULL || !gameLocal.GetLocalPlayer()->GetRenderView() ) {
		return;
	}

	// draw team icons
	if( gameLocal.IsTeamGame() ) {
		UpdateTeamIcons();
	}

	// draw chat icons
	UpdateChatIcons();

	// remove old icons and icons not in our snapshot
	// ** if you want to have permanent icons, you'll have to add support
	// ** for the icons to re-appear when the player comes back in your snapshot (like team icons and chat icons)
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		for( int j = 0; j < icons[ i ].Num(); j++ ) {
			if( gameLocal.time > icons[ i ][ j ].Second() || (gameLocal.entities[ i ] && gameLocal.entities[ i ]->fl.networkStale) ) {
				rvIcon* oldIcon = icons[ i ][ j ].First();
				oldIcon->FreeIcon();
				icons[ i ].RemoveIndex( j-- );
				delete oldIcon;
			}
		}
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();

    // draw extra icons
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if( gameLocal.localClientNum == i ) {
			continue;
		}

		if( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::GetClassType() ) ) {
			idPlayer* player = static_cast<idPlayer*>(gameLocal.entities[ i ]);

			if( player->IsHidden() || !icons[ i ].Num() ) {
				continue;
			}

			// distribute the icons appropriately
			int maxHeight = 0;
			int totalWidth = 0;

			for( int j = 0; j < icons[ i ].Num(); j++ ) {
				if( icons[ i ][ j ].First()->GetHeight() > maxHeight ) {
					maxHeight = icons[ i ][ j ].First()->GetHeight();
				}
				totalWidth += icons[ i ][ j ].First()->GetWidth();
			}
			
			idVec3 centerIconPosition = player->spawnArgs.GetVector( (player->team ? "team_icon_height_strogg" : "team_icon_height_marine") );
			

			if( teamIcons[ player->entityNumber ].GetHandle() >= 0 ) {
				centerIconPosition[ 2 ] += teamIcons[ player->entityNumber ].GetHeight();
			}

			int incrementalWidth = 0;
			for( int j = 0; j < icons[ i ].Num(); j++ ) {
				idVec3 iconPosition = centerIconPosition;
				iconPosition += ( (-totalWidth / 2) + incrementalWidth + (icons[ i ][ j ].First()->GetWidth() / 2) ) * localPlayer->GetRenderView()->viewaxis[ 1 ];
				incrementalWidth += icons[ i ][ j ].First()->GetWidth();
				icons[ i ][ j ].First()->UpdateIcon( player->GetPhysics()->GetOrigin() + iconPosition, localPlayer->GetRenderView()->viewaxis );
			}
		}
	}
}

void rvIconManager::UpdateTeamIcons( void ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( !localPlayer ) {
		return;
	}
	int localTeam = localPlayer->team;
	bool spectating = localPlayer->spectating;

	if( localPlayer->spectating ) { 
		idPlayer* spec = (idPlayer*)gameLocal.entities[ localPlayer->spectator ];
		if( spec ) {
			localTeam = spec->team;
			localPlayer = spec;
		} else {
			localTeam = -1;
		}
	}
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if( gameLocal.localClientNum == i ) {
			continue;
		}

		//if entity i is a player, manage his icon.
		if( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::GetClassType() ) && !gameLocal.entities[ i ]->fl.networkStale && !spectating ) {
			idPlayer* player = static_cast<idPlayer*>(gameLocal.entities[ i ]);
			
			//if the player is alive and not hidden, show his icon.
			if( player->team == localTeam && !player->IsHidden() && !player->pfl.dead && gameLocal.mpGame.IsInGame( i ) ) {
				if( teamIcons[ i ].GetHandle() < 0 ) {
					teamIcons[ i ].CreateIcon( player->spawnArgs.GetString( player->team ? "mtr_team_icon_strogg" : "mtr_team_icon_marine" ), (player == localPlayer ? localPlayer->entityNumber + 1 : 0) );
				}
				teamIcons[ i ].UpdateIcon( player->GetPhysics()->GetOrigin() + player->spawnArgs.GetVector( (player->team ? "team_icon_height_strogg" : "team_icon_height_marine") ), localPlayer->GetRenderView()->viewaxis );
			//else, the player is hidden, dead, or otherwise not needing an icon-- free it.
			} else {
				if( teamIcons[ i ].GetHandle() >= 0 ) {
					teamIcons[ i ].FreeIcon();
				}
			}
		//if entity i is not a player, free icon i from the map.
		} else if( teamIcons[ i ].GetHandle() >= 0 ) {
			teamIcons[ i ].FreeIcon();	
		}
	}
}

void rvIconManager::UpdateChatIcons( void ) {

	int localInst = gameLocal.GetLocalPlayer()->GetInstance();
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( gameLocal.localClientNum == i ) {
			continue;
		}

		if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idPlayer::GetClassType() ) && !gameLocal.entities[ i ]->fl.networkStale ) {
			idPlayer *player = static_cast< idPlayer* >( gameLocal.entities[ i ] );

			if ( player->isChatting && 
				!player->IsHidden() && 
				!( ( idPhysics_Player* )player->GetPhysics() )->IsDead() && 
				gameLocal.mpGame.IsInGame( i ) 
				&& (localInst == player->GetInstance())) {
				if ( chatIcons[ i ].GetHandle() < 0 ) {
					chatIcons[ i ].CreateIcon( player->spawnArgs.GetString( "mtr_icon_chatting" ), ( player == gameLocal.GetLocalPlayer() ? player->entityNumber + 1 : 0) );
				}
				int maxHeight = 0;
				for ( int j = 0; j < icons[ i ].Num(); j++ ) {
					if ( icons[ i ][ j ].First()->GetHeight() > maxHeight ) {
						maxHeight = icons[ i ][ j ].First()->GetHeight();
					}
				}
				if ( teamIcons[ i ].GetHandle() >= 0 && teamIcons[ i ].GetHeight() > maxHeight ) {
					maxHeight = teamIcons[ i ].GetHeight();
				}
				idVec3 centerIconPosition = player->spawnArgs.GetVector( ( player->team ? "team_icon_height_strogg" : "team_icon_height_marine") );
				centerIconPosition[ 2 ] += maxHeight;
				chatIcons[ i ].UpdateIcon( player->GetPhysics()->GetOrigin() + centerIconPosition, gameLocal.GetLocalPlayer()->GetRenderView()->viewaxis );
			} else if ( chatIcons[ i ].GetHandle() >= 0 ) {
				chatIcons[ i ].FreeIcon();
			}
		} else if ( chatIcons[ i ].GetHandle() >= 0 ) {
			chatIcons[ i ].FreeIcon();
		}
	}
}

/*
===============
rvIconManager::Shutdown
===============
*/
void rvIconManager::Shutdown( void ) {
	int i, j;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		for ( j = 0; j < icons[ i ].Num(); j++ ) {
			icons[ i ][ j ].First()->FreeIcon();
		}
		icons[ i ].Clear();
		teamIcons[ i ].FreeIcon();
		chatIcons[ i ].FreeIcon();
	}
}
