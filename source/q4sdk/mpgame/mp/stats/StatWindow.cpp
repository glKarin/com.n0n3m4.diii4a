//----------------------------------------------------------------
// StatWindow.cpp
//
// Copyright 2002-2005 Raven Software
//----------------------------------------------------------------

#include "../../../idlib/precompiled.h"
#pragma hdrstop

#include "../../Game_local.h"

#include "StatWindow.h"
#include "StatManager.h"

/*
================
rvStatWindow::rvStatWindow()
================
*/
rvStatWindow::rvStatWindow() {
	statHud = NULL;
}

/*
================
rvStatWindow::SetupStatWindow()

Sets up a selectable window of current stats
================
*/
void rvStatWindow::SetupStatWindow( idUserInterface* hud, bool useSpectator ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	assert( player );
	if ( !player ) {
		return;
	}

// mekberg: added
	idList<int> marineScores;
	idList<int> stroggScores;
	
	int selectionIndex = -1;
	int selectionTeam = -1;

	statHud = hud;

	if( gameLocal.IsTeamGame() ) {
		if( gameLocal.IsFlagGameType() ) {
			statHud->SetStateInt( "ctf_awards", 1 );
		} else {
			statHud->SetStateInt( "ctf_awards", 0 );
		}

		stroggPlayers.Clear();
		marinePlayers.Clear();

		for( int i = 0; i < gameLocal.mpGame.GetNumRankedPlayers(); i++ ) {
			idPlayer* player = gameLocal.mpGame.GetRankedPlayer( i );
			
			if( player->team == TEAM_MARINE ) {
				marinePlayers.Append( player );
				marineScores.Append( gameLocal.mpGame.GetRankedPlayerScore( i ) );
			} else if ( player->team == TEAM_STROGG ) {
				stroggPlayers.Append( player );
				stroggScores.Append( gameLocal.mpGame.GetRankedPlayerScore( i ) );
			}
		}

		for( int i = 0; i < MAX_CLIENTS; i++ ) {
			if( i < marinePlayers.Num() ) {
				statHud->SetStateString( va( "team_1_names_item_%d", i ), 
										 va( "%s\t%s\t%s\t%d\t",	( player->IsFriend( marinePlayers[ i ] ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),
																	( player->IsPlayerMuted( marinePlayers[ i ] ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ),
																	marinePlayers[ i ]->GetUserInfo()->GetString( "ui_name"), 
																	marineScores[ i ] ) );

				if( useSpectator ) { 
					if( marinePlayers[ i ]->entityNumber == player->spectator ) {
						selectionTeam = TEAM_MARINE;
						selectionIndex = i;
					}
				} else {
					if( marinePlayers[ i ] == player ) {
						selectionTeam = TEAM_MARINE;
						selectionIndex = i;
					}
				}
			} else {
				statHud->SetStateString( va( "team_1_names_item_%d", i ), "" );
			}
		}

		for( int i = 0; i < MAX_CLIENTS; i++ ) {
			if( i < stroggPlayers.Num() ) {
				statHud->SetStateString( va( "team_2_names_item_%d", i ), 
					    				 va( "%s\t%s\t%s\t%d\t",	( player->IsFriend( stroggPlayers[ i ] ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),
																	( player->IsPlayerMuted( stroggPlayers[ i ] ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ),
																	stroggPlayers[ i ]->GetUserInfo()->GetString( "ui_name"), 
																	stroggScores[ i ] ) );
				if( useSpectator ) { 
					if( stroggPlayers[ i ]->entityNumber == player->spectator ) {
						selectionTeam = TEAM_STROGG;
						selectionIndex = i;
					}
				} else {
					if( stroggPlayers[ i ] == player ) {
						selectionTeam = TEAM_STROGG;
						selectionIndex = i;
					}
				}
			} else {
				statHud->SetStateString( va( "team_2_names_item_%d", i ), "" );
			}
		}

		statHud->SetStateInt ( "num_strogg_players", stroggPlayers.Num() );
		statHud->SetStateInt ( "num_marine_players", marinePlayers.Num() );
	} else {
		statHud->SetStateInt( "ctf_awards", 0 );

		players.Clear();

		for( int i = 0; i < gameLocal.mpGame.GetNumRankedPlayers(); i++ ) {
			players.Append( gameLocal.mpGame.GetRankedPlayer( i ) );
		}

		for( int i = 0; i < MAX_CLIENTS; i++ ) {
			if( i < players.Num() ) {
				statHud->SetStateString( va( "dm_names_item_%d", i ), 
					    				 va( "%s\t%s\t%s\t%d\t",	( player->IsFriend( players[ i ] ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),
																	( player->IsPlayerMuted( players[ i ] ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ),
																	players[ i ]->GetUserInfo()->GetString( "ui_name"), 
								
																	gameLocal.mpGame.GetScore( players[ i ] ) ) );
				if( useSpectator ) { 
					if( players[ i ]->entityNumber == player->spectator ) {
						selectionTeam = 0;
						selectionIndex = i;
					}
				} else {
					if( players[ i ] == player ) {
						selectionTeam = 0;
						selectionIndex = i;
					}
				}
			} else {
				statHud->SetStateString( va( "dm_names_item_%d", i ), "" );
			}
		}
		statHud->SetStateInt ( "num_players", players.Num() );
	}

	// spectators
	spectators.Clear();

	for( int i = 0; i < gameLocal.mpGame.GetNumUnrankedPlayers(); i++ ) {
		spectators.Append( gameLocal.mpGame.GetUnrankedPlayer( i ) );
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if( i < spectators.Num() ) {
			statHud->SetStateString( va( "spec_names_item_%d", i ), 
									 va( "%s\t%s\t%s\t",	( player->IsFriend( spectators[ i ] ) ? I_FRIEND_ENABLED : I_FRIEND_DISABLED ),
															( player->IsPlayerMuted( spectators[ i ] ) ? I_VOICE_DISABLED : I_VOICE_ENABLED ),
															spectators[ i ]->GetUserInfo()->GetString( "ui_name") ) );
			
			if( useSpectator ) { 
				if( spectators[ i ]->entityNumber == player->spectator ) {
					selectionTeam = TEAM_MAX;
					selectionIndex = i;
				}
			} else {
				if( spectators[ i ] == player ) {
					selectionTeam = TEAM_MAX;
					selectionIndex = i;
				}
			}
		} else {
			statHud->SetStateString( va( "spec_names_item_%d", i ), "" );
			
		}
	}

	statHud->SetStateInt( "gametype", gameLocal.gameType );
	statHud->SetStateInt( "playerteam", gameLocal.GetLocalPlayer()->team );

	statHud->StateChanged ( gameLocal.time );
	statHud->Redraw( gameLocal.time );

	// we shouldn't ever draw a hud unless we're in-game
	if ( selectionIndex >= 0 && selectionTeam >= 0 ) {
		statManager->SelectStatWindow( selectionIndex, selectionTeam );
	}
}

/*
================
rvStatWindow::ClearWindow()

Clears the stat part of the stat window, but not the player lists boxes
================
*/
void rvStatWindow::ClearWindow( void ) {
	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		statHud->SetStateString( va( "stat_%d_pct", i ), "" );
	}

	for( int i = 0; i < IGA_NUM_AWARDS; i++ ) {
		statHud->SetStateString( inGameAwardInfo[ i ].name, "" );
	}

	// end-game awards
	for( int i = 0; i < EGA_NUM_AWARDS; i++ ) {
		statHud->SetStateInt( va( "eg_award%d", i ), 0 );
		statHud->SetStateString( va( "eg_award%d_text", i ), "" );
	}

	// kills
	statHud->SetStateString( "stat_frags", "" );

	// deaths
	statHud->SetStateString( "stat_deaths", "" );

	// score
	statHud->SetStateString( "stat_score", "" );

	statHud->StateChanged ( gameLocal.time );
	statHud->Redraw( gameLocal.time );
}


/*
================
rvStatWindow::SelectPlayer()

Selects the specified player
================
*/
void rvStatWindow::SelectPlayer( int clientNum ) {
	if( statHud == NULL ) {
		return;
	}

	if( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		ClearWindow();
		return;
	}

	assert( gameLocal.GetLocalPlayer() );

	rvPlayerStat* clientStat = statManager->GetPlayerStat( clientNum );

	if( gameLocal.isClient && ( clientStat == NULL || ( gameLocal.time - clientStat->lastUpdateTime ) > 5000 ) ) {
		// get new stats
		idBitMsg	outMsg;
		byte		msgBuf[ 128 ];

		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_STAT );
		outMsg.WriteByte( clientNum );
		networkSystem->ClientSendReliableMessage( outMsg );
		return;
	}

	// weapon accuracy
	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		int weaponAccuracy = 0;
		if( clientStat->weaponShots[ i ] != 0 ) {
			weaponAccuracy = (int)(((float)clientStat->weaponHits[ i ] / (float)clientStat->weaponShots[ i ]) * 100.0f);
		}
		statHud->SetStateString( va( "stat_%d_pct", i ), va( "%d%%", weaponAccuracy ) );
	}

	int igAwardCount[ IGA_NUM_AWARDS ];
	memset( igAwardCount, 0, sizeof( int ) * IGA_NUM_AWARDS );
	for( int i = 0; i < IGA_NUM_AWARDS; i++ ) {
		igAwardCount[i] = clientStat->inGameAwards[i];
	}

	for( int i = 0; i < IGA_NUM_AWARDS; i++ ) {
		statHud->SetStateString( inGameAwardInfo[ i ].name, va( "%d", igAwardCount[ i ] ) );
	}

	// end-game awards
	for( int i = 0; i < EGA_NUM_AWARDS; i++ ) {
		if( i < clientStat->endGameAwards.Num() ) {
			statHud->SetStateInt( va( "eg_award%d", i ), 1 );
// RAVEN BEGIN
// rhummer: Localized the award strings..
			statHud->SetStateString( va( "eg_award%d_text", i ), common->GetLocalizedString( endGameAwardInfo[ clientStat->endGameAwards[ i ] ].name ) );
// RAVEN END
		} else {
			statHud->SetStateInt( va( "eg_award%d", i), 0 );
		}
	}

	// kills
	statHud->SetStateString( "stat_frags", va( "%d", clientStat->kills ) );

	// deaths
	statHud->SetStateString( "stat_deaths", va( "%d", clientStat->deaths ) );

	// score
	int score = gameLocal.IsTeamGame() ? (gameLocal.mpGame.GetTeamScore( clientNum ) + gameLocal.mpGame.GetScore( clientNum )): gameLocal.mpGame.GetScore( clientNum );
	statHud->SetStateString( "stat_score", va( "%d", score ) );

	if( gameLocal.GetLocalPlayer()->IsFriend( clientNum ) ) {
		// remove friend label
		statHud->SetStateString( "friend_button", common->GetLocalizedString( "#str_200249" ) );
	} else {
		// add friend label
		statHud->SetStateString( "friend_button", common->GetLocalizedString( "#str_200248" ) );
	}

	if( gameLocal.GetLocalPlayer()->IsPlayerMuted( clientNum ) ) {
		// unmute label
		statHud->SetStateString( "mute_button", common->GetLocalizedString( "#str_200251" ) );
	} else {
		// mute label
		statHud->SetStateString( "mute_button", common->GetLocalizedString( "#str_200250" ) );
	}

	statHud->StateChanged ( gameLocal.time );
	statHud->Redraw( gameLocal.time );
}

/*
================
rvStatWindow::ClientNumFromSelection()

Parses a selection index and team into a clientNum
================
*/
int rvStatWindow::ClientNumFromSelection( int selectionIndex, int selectionTeam ) {
	int clientNum = -1;

	if( gameLocal.IsTeamGame() ) {
		if( selectionTeam == TEAM_MARINE ) {
			if( selectionIndex < 0 || selectionIndex >= marinePlayers.Num() ) {
				gameLocal.Warning( "rvStatManager::SelectPlayerStats() - invalid selection '%d'\n", selectionIndex );
				return -1;
			}
			clientNum = marinePlayers[ selectionIndex ]->entityNumber;
			
			// explicitly set the gui selection if we called in here not from a GUI
			statHud->SetStateInt( "team_1_names_sel_0", selectionIndex );
			statHud->SetStateString( "dm_names_sel_0", "-1" );
			statHud->SetStateString( "spec_names_sel_0", "-1" );
			statHud->SetStateString( "team_2_names_sel_0", "-1" );
		} else if( selectionTeam == TEAM_STROGG ) {
			if( selectionIndex < 0 || selectionIndex >= stroggPlayers.Num() ) {
				gameLocal.Warning( "rvStatManager::SelectPlayerStats() - invalid selection '%d'\n", selectionIndex );
				return -1;
			}
			clientNum = stroggPlayers[ selectionIndex ]->entityNumber;

			// explicitly set the gui selection if we called in here not from a GUI
			statHud->SetStateInt( "team_2_names_sel_0", selectionIndex );
			statHud->SetStateString( "dm_names_sel_0", "-1" );
			statHud->SetStateString( "spec_names_sel_0", "-1" );
			statHud->SetStateString( "team_1_names_sel_0", "-1" );
		} else {
			if( selectionIndex < 0 || selectionIndex >= spectators.Num() ) {
				gameLocal.Warning( "rvStatManager::SelectPlayerStats() - invalid selection '%d'\n", selectionIndex );
				return -1;
			}
			clientNum = spectators[ selectionIndex ]->entityNumber;

			// explicitly set the gui selection if we called in here not from a GUI
			statHud->SetStateInt( "spec_names_sel_0", selectionIndex );
			statHud->SetStateString( "dm_names_sel_0", "-1" );
			statHud->SetStateString( "team_1_names_sel_0", "-1" );
			statHud->SetStateString( "team_2_names_sel_0", "-1" );
		}
	} else {
		if( selectionTeam == TEAM_MAX ) {
			if( selectionIndex < 0 || selectionIndex >= spectators.Num() ) {
				gameLocal.Warning( "rvStatManager::SelectPlayerStats() - invalid selection '%d'\n", selectionIndex );
				return -1;
			}
			clientNum = spectators[ selectionIndex ]->entityNumber;

			// explicitly set the gui selection if we called in here not from a GUI
			statHud->SetStateInt( "spec_names_sel_0", selectionIndex );
			statHud->SetStateString( "dm_names_sel_0", "-1" );
			statHud->SetStateString( "team_1_names_sel_0", "-1" );
			statHud->SetStateString( "team_2_names_sel_0", "-1" );
		} else {
			if( selectionIndex < 0 || selectionIndex >= players.Num() ) {
				gameLocal.Warning( "rvStatManager::SelectPlayerStats() - invalid selection '%d'\n", selectionIndex );
				return -1;
			}
			clientNum = players[ selectionIndex ]->entityNumber;

			// explicitly set the gui selection if we called in here not from a GUI
			statHud->SetStateInt( "dm_names_sel_0", selectionIndex );
			statHud->SetStateString( "spec_names_sel_0", "-1" );
			statHud->SetStateString( "team_1_names_sel_0", "-1" );
			statHud->SetStateString( "team_2_names_sel_0", "-1" );
		}
	}

	return clientNum;
}

/*
================
rvStatWindow::GetSelectedClientNum()

Queries the gui dict to figure out the currently selected selectionIndex/selectionTeam.
Uses the selectionIndex/selectionTeam to return the selected client num.
================
*/
int rvStatWindow::GetSelectedClientNum( int* selectionIndexOut, int* selectionTeamOut ) {
	if( statHud == NULL ) {
		return -1;
	}
	int selectionIndex = -1;
	int selectionTeam = -1;

	// StatWindow update code should assure that only one selection of all these listDefs is is valid
	// Find the valid one
	if( statHud->State().GetInt( "spec_names_sel_0", "-1" ) >= 0 ) {
		selectionIndex = statHud->State().GetInt( "spec_names_sel_0", "-1" );
		selectionTeam = TEAM_MAX;
	}

	if ( statHud->State().GetInt( "dm_names_sel_0", "-1" ) >= 0 ) {
		// if this assert fails, more than one selection is valid
		assert( selectionIndex == -1 && selectionTeam == -1 );

		selectionIndex = statHud->State().GetInt( "dm_names_sel_0", "-1" );
		selectionTeam = 0;
	}

	if ( statHud->State().GetInt( "team_1_names_sel_0", "-1" ) >= 0 ) {
		// if this assert fails, more than one selection is valid
		assert( selectionIndex == -1 && selectionTeam == -1 );

		selectionIndex = statHud->State().GetInt( "team_1_names_sel_0", "-1" );
		selectionTeam = TEAM_MARINE;
	}

	if ( statHud->State().GetInt( "team_2_names_sel_0", "-1" ) >= 0 ) {
		// if this assert fails, more than one selection is valid
		assert( selectionIndex == -1 && selectionTeam == -1 );

		selectionIndex = statHud->State().GetInt( "team_2_names_sel_0", "-1" );
		selectionTeam = TEAM_STROGG;
	}

	// return the selection index & team
	if( selectionIndexOut ) {
		(*selectionIndexOut) = selectionIndex;
	}
	if( selectionTeamOut ) {
		(*selectionTeamOut) = selectionTeam;
	}

	return ClientNumFromSelection( selectionIndex, selectionTeam );
}
