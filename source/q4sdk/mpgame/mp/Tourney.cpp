//----------------------------------------------------------------
// Tourney.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "Tourney.h"


/*
===============================================================================

rvTourneyArena

===============================================================================
*/

/*
================
rvTourneyArena::rvTourneyArena
================
*/
rvTourneyArena::rvTourneyArena() {
	players[ 0 ] = NULL;
	players[ 1 ] = NULL;
	winner = NULL;
	nextStateTime = 0;
	fragLimitTimeout = 0;
	matchStartTime = 0;
}

/*
================
rvTourneyArena::AddPlayer
================
*/
void rvTourneyArena::AddPlayers( idPlayer* playerOne, idPlayer* playerTwo ) {
	players[ 0 ] = playerOne;
	players[ 1 ] = playerTwo;
	if( playerOne ) {
		playerOne->SetTourneyStatus( PTS_PLAYING );
	}

	if( playerTwo ) {
		playerTwo->SetTourneyStatus( PTS_PLAYING );
	}
}

/*
================
rvTourneyArena::ClearPlayers
Clears player references if clearPlayer is NULL (client-side)
Clears specific player if clearPlayer is not NULL (server-side)
================
*/
void rvTourneyArena::ClearPlayers( idPlayer* clearPlayer /* = NULL */ ) {
	if( gameLocal.isServer ) {
		if( clearPlayer ) {
			assert( clearPlayer == players[ 0 ] || clearPlayer == players[ 1 ] );
			if( clearPlayer == players[ 0 ] ) {
				players[ 0 ] = NULL;
			} else {
				players[ 1 ] = NULL;
			}
		}
		return;
	} else {
		players[ 0 ] = NULL;
		players[ 1 ] = NULL;
	}
}

/*
================
rvTourneyArena::Ready
================
*/
void rvTourneyArena::Ready( void ) {
	arenaState = AS_WARMUP;
	nextStateTime = gameLocal.time + ( gameLocal.serverInfo.GetInt( "si_countDown" ) * 1000 ) + 5000;

	matchStartTime = 0;

	if( !players[ 0 ] || !players[ 1 ] ) {
		// if we don't have both players available, bye this arena
		NewState( AS_DONE );
	}
}

/*
================
rvTourneyArena::Clear
Clears player list and state, but not round # or arena ID
================
*/
void rvTourneyArena::Clear( bool respawnPlayers ) {
	if( gameLocal.isServer && respawnPlayers ) {
		if( players[ 0 ] ) {
			gameLocal.mpGame.SetPlayerTeamScore( players[ 0 ], 0 );		
			players[ 0 ]->JoinInstance( ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetNextActiveArena( arenaID ) );
			players[ 0 ]->ServerSpectate( true );
		}

		if( players[ 1 ] ) {
			gameLocal.mpGame.SetPlayerTeamScore( players[ 1 ], 0 );		
			players[ 1 ]->JoinInstance( ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetNextActiveArena( arenaID ) );
			players[ 1 ]->ServerSpectate( true );
		}

		// This arena is being cleared so we must also clear out any spectators
		for( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = (idPlayer*)gameLocal.entities[ i ];

			if( player == NULL ) {
				continue;
			}

			if( player->GetArena() == arenaID ) {
				player->JoinInstance( ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetNextActiveArena( arenaID ) );
			}
		}
	}

	players[ 0 ] = NULL;
	players[ 1 ] = NULL;
	winner = NULL;
	nextStateTime = 0;
	fragLimitTimeout = 0;
	matchStartTime = 0;
	SetState( AS_INACTIVE );
}

/*
================
rvTourneyArena::GetLeader
Returns winning player, or NULL if there's a tie/or its undefined
================
*/
idPlayer* rvTourneyArena::GetLeader( void ) {
	if( players[ 0 ] == NULL || players[ 1 ] == NULL ) {
		if( players[ 0 ] ) {
			return players[ 0 ];
		} else if( players[ 1 ] ) {
			return players[ 1 ];
		}

		return NULL;
	}

	int playerOneScore = gameLocal.mpGame.GetTeamScore( players[ 0 ]->entityNumber );
	int playerTwoScore = gameLocal.mpGame.GetTeamScore( players[ 1 ]->entityNumber );

	if ( playerOneScore == playerTwoScore ) {
		return NULL;
	}

	return ( playerOneScore > playerTwoScore ? players[ 0 ] : players[ 1 ] );
}

/*
================
rvTourneyArena::GetLoser
Returns losing player, or NULL if there's a tie/or its undefined
================
*/
idPlayer* rvTourneyArena::GetLoser( void ) {
	if( winner == NULL ) {
		return NULL;
	} else {
		return ( winner == players[ 0 ] ? players[ 1 ] : players[ 0 ] );
	}
}

/*
================
rvTourneyArena::UpdateState
Updates this arena's state
================
*/
void rvTourneyArena::UpdateState( void ) {
	if( (players[ 0 ] == NULL || players[ 1 ] == NULL) && arenaState != AS_DONE ) {
		gameLocal.Warning( "rvTourneyArena::UpdateState() - UpdateState() called on non-full and non-done arena\n" );
		NewState( AS_DONE );
		return;
	}

	switch( arenaState ) {
		case AS_DONE: {
			// not both players will neccesarily be valid here (if a player wins the arena then his opponent disconnects)

			break;
		}
		case AS_WARMUP: {
			if( gameLocal.time > nextStateTime ) {
				SetState( AS_ROUND );
				
				// allow damage in warmup
				//players[ 0 ]->fl.takedamage = true;
				//players[ 1 ]->fl.takedamage = true;
				// respawn the players
				//players[ 0 ]->ServerSpectate( false );
				//players[ 1 ]->ServerSpectate( false );
				// respawn items
				gameLocal.mpGame.EnableDamage( true );
				gameLocal.LocalMapRestart( arenaID );

				gameLocal.mpGame.SetPlayerTeamScore( players[ 0 ], 0 );
				gameLocal.mpGame.SetPlayerTeamScore( players[ 1 ], 0 );
				matchStartTime = gameLocal.time;
			}

			break;
		}

		case AS_ROUND: {
			if( GetLeader() && gameLocal.serverInfo.GetInt( "si_fragLimit" ) > 0 && gameLocal.mpGame.GetTeamScore( GetLeader()->entityNumber ) >= gameLocal.serverInfo.GetInt( "si_fraglimit" ) ) {
				if( fragLimitTimeout && fragLimitTimeout < gameLocal.time ) {
					NewState( AS_DONE );
				} else if ( !fragLimitTimeout ) {
					fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
				}
			} else if( fragLimitTimeout ) {
				if( !GetLeader() && gameLocal.mpGame.GetTeamScore( players[ 0 ]->entityNumber ) >= gameLocal.serverInfo.GetInt( "si_fraglimit" ) ) {
					// players tied at fraglimit
					NewState( AS_SUDDEN_DEATH );
				} else {
					// player hit fraglimit, but then killed himself and dropped below fraglimit, return to normal play
					fragLimitTimeout = 0;
				}
				
				//back to normal play?
			} else if ( TimeLimitHit() ) {
				// only send to clients in this arena, the times may be getting out of sync between the arenas during a round ( #13196 )
				int i;
				for ( i = 0; i < MAX_CLIENTS; i++ ) {
					idPlayer* player = (idPlayer*)gameLocal.entities[ i ];
					if ( !player ) {
						continue;
					}
					if ( player->GetArena() == arenaID ) {
						gameLocal.mpGame.PrintMessageEvent( i, MSG_TIMELIMIT );
					}
				}
				if( GetLeader() == NULL ) {
					// if tied at timelimit hit, goto sudden death
					NewState( AS_SUDDEN_DEATH );
				} else {
					// or just end the game
					NewState( AS_DONE );
				}
			}
			break;
		}
		case AS_SUDDEN_DEATH: {
			if ( GetLeader() ) {
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter sudden death FragLeader timeout, player %d is leader\n", GetLeader()->entityNumber );
					fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
				}
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( AS_DONE );
					gameLocal.mpGame.PrintMessageEvent( -1, MSG_FRAGLIMIT, GetLeader()->entityNumber );
				}
			} else if ( fragLimitTimeout ) {
				gameLocal.mpGame.PrintMessageEvent( -1, MSG_HOLYSHIT );
				fragLimitTimeout = 0;
			}
			break;
		}
	}
}

/*
================
rvTourneyArena::NewState
================
*/
void rvTourneyArena::NewState( arenaState_t newState ) {
	switch( newState ) {
		case AS_DONE: {
			winner = GetLeader();
			assert( winner );
			gameLocal.Printf( "rvTourneyArena::UpdateState() - %s has won this arena\n", GetLeader()->GetUserInfo()->GetString( "ui_name" ) );	

			winner->SetTourneyStatus( PTS_ADVANCED );

			if( GetLoser() ) {
				GetLoser()->SetTourneyStatus( PTS_ELIMINATED );
			}


			if( players[ 0 ] ) {
				players[ 0 ]->ServerSpectate( true );
				players[ 0 ]->JoinInstance( ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetNextActiveArena( arenaID ) );
			}

			if( players[ 1 ] ) {
				players[ 1 ]->ServerSpectate( true );
				players[ 1 ]->JoinInstance( ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetNextActiveArena( arenaID ) );
			}

			// when we're done, put anyone who was spectating into another arena
			for( int i = 0; i < MAX_CLIENTS; i++ ) {
				idPlayer* player = (idPlayer*)gameLocal.entities[ i ];

				if( player == NULL ) {
					continue;
				}

				if( player->GetArena() == arenaID ) {
					player->JoinInstance( ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetNextActiveArena( arenaID ) );
				}
			}

			matchStartTime = 0;
			break;
		}
		case AS_SUDDEN_DEATH: {
			fragLimitTimeout = 0;

			// respawn the arena
			players[ 0 ]->ServerSpectate( false );
			players[ 1 ]->ServerSpectate( false );
			gameLocal.LocalMapRestart( arenaID );
			break;
		}
		default: {
			gameLocal.Error( "rvTourneyArena::NewState() - Unknown state '%d'\n", newState );
			return;
		}
	}
	SetState( newState );	
}

/*
================
rvTourneyArena::RemovePlayer
The specified client is being removed from the server
================
*/
void rvTourneyArena::RemovePlayer( idPlayer* player ) {
	// when we call Clear() the arena will clean up (set player instances to 0, spectate them, etc)
	// we don't want it to do this for the disconnecting player, since he may not be entirely valid
	// anymore.  If we NULL out the disconnecting player, Clear() will properly reset the remaining player
	// but will leave the volatile disconnecting player's data lone.
		
	bool playerInArena = false;
	if( player == players[ 0 ] ) {
		players[ 0 ] = NULL;
		playerInArena = true;
	} 
	
	if( player == players[ 1 ] ) {
		players[ 1 ] = NULL;
		playerInArena = true;
	}

	if( player == winner ) {
		winner = NULL;
	}

	if( !playerInArena ) {
		// occasionally happens if a client has dropped ( the bystander maybe )
		common->Warning( "rvTourneyArena::RemovePlayer() - Called on player who is not in arena '%d'\n", arenaID );
	}
}

/*
================
rvTourneyArena::PackState
================
*/
void rvTourneyArena::PackState( idBitMsg& outMsg ) {
	if ( players[ 0 ] ) {
		outMsg.WriteChar( players[ 0 ]->entityNumber );
	} else {
		outMsg.WriteChar( -1 );
	}
	
	if ( players[ 1 ] ) {
		outMsg.WriteChar( players[ 1 ]->entityNumber );
	} else {
		outMsg.WriteChar( -1 );
	}

	if ( winner ) {
		outMsg.WriteChar( winner->entityNumber );
	} else {
		outMsg.WriteChar( -1 );
	}
	
	outMsg.WriteByte( arenaState );
	outMsg.WriteLong( nextStateTime );
	outMsg.WriteLong( fragLimitTimeout );
	outMsg.WriteLong( matchStartTime );
}

/*
================
rvTourneyArena::UnpackState
================
*/
void rvTourneyArena::UnpackState( const idBitMsg& inMsg ) {
	int playerOneNum = inMsg.ReadChar();
	int playerTwoNum = inMsg.ReadChar();
	int winnerNum = inMsg.ReadChar();

	if ( playerOneNum >= 0 && playerOneNum < MAX_CLIENTS ) {
		players[ 0 ] = (idPlayer*)gameLocal.entities[ playerOneNum ];
	} else {
		players[ 0 ] = NULL;
	}

	if ( playerTwoNum >= 0 && playerTwoNum < MAX_CLIENTS ) {
		players[ 1 ] = (idPlayer*)gameLocal.entities[ playerTwoNum ];
	} else {
		players[ 1 ] = NULL;
	}

	if ( winnerNum >= 0 && winnerNum < MAX_CLIENTS ) {
		winner = (idPlayer*)gameLocal.entities[ winnerNum ];
	} else {
		winner = NULL;
	}

	arenaState = (arenaState_t)inMsg.ReadByte();
	nextStateTime = inMsg.ReadLong();
	fragLimitTimeout = inMsg.ReadLong();
	matchStartTime = inMsg.ReadLong();
}

/*
================
rvTourneyArena::SetState
Set's this arena's state - client side only based on UpdateState() results from server
================
*/
void rvTourneyArena::SetState( arenaState_t newState ) {
	arenaState = newState;
}

/*
================
rvTourneyArena::SetNextStateTime
================
*/
void rvTourneyArena::SetNextStateTime( int time ) {
	nextStateTime = time;
}

/*
================
rvTourneyArena::GetNextStateTime
================
*/
int rvTourneyArena::GetNextStateTime( void ) {
	return nextStateTime;
}


/*
================
rvTourneyArena::GetState
================
*/
arenaState_t rvTourneyArena::GetState( void ) const {
	return arenaState;
}

/*
================
rvTourneyArena::GetPlayerName
================
*/
const char* rvTourneyArena::GetPlayerName( int player ) {
	assert( player >= 0 && player < 2 );

	if( players[ player ] ) {
		return players[ player ]->GetUserInfo()->GetString( "ui_name" );
	} else {
		return NULL;
	}
}

/*
================
rvTourneyArena::GetPlayerScore
================
*/
int rvTourneyArena::GetPlayerScore( int player ) {
	assert( player >= 0 && player < 2 );


	if( players[ player ] ) {
		return gameLocal.mpGame.GetTeamScore( players[ player ] );
	} else {
		return 0;
	}
}

/*
================
rvTourneyArena::TimeLimitHit
================
*/
bool rvTourneyArena::TimeLimitHit( void ) {	
	int timeLimit = gameLocal.serverInfo.GetInt( "si_timeLimit" );
	if ( timeLimit ) {
		if ( gameLocal.time >= matchStartTime + timeLimit * 60000 ) {
			return true;
		}
	}
	return false;
}

/*
===============================================================================

rvTourneyGUI

===============================================================================
*/

rvTourneyGUI::rvTourneyGUI() {
	tourneyGUI = NULL;
	tourneyScoreboard = NULL;
}

void rvTourneyGUI::SetupTourneyGUI( idUserInterface* newTourneyGUI, idUserInterface* newTourneyScoreboard ) {
	tourneyGUI = newTourneyGUI;
	tourneyScoreboard = newTourneyScoreboard;
}

void rvTourneyGUI::RoundStart( void ) {
	if( tourneyGUI == NULL ) {
		common->Warning( "rvTourneyGUI::RoundStart() - Invalid tourneyGUI" );
		return;
	}

	tourneyGUI->SetStateInt( "round", ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetRound() );
	tourneyGUI->StateChanged( gameLocal.time );
	tourneyGUI->HandleNamedEvent( "roundTransitionIn" );

	if( tourneyScoreboard == NULL ) {
		common->Warning( "rvTourneyGUI::RoundStart() - Invalid tourneyScoreboard" );
		return;
	}

	tourneyScoreboard->SetStateInt( "round", ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetRound() );
	tourneyScoreboard->StateChanged( gameLocal.time );
	tourneyScoreboard->HandleNamedEvent( "roundTransitionIn" );

	// set bye player for new round - the actual byePlayer may not have changed, so the gamestate won't move him over
//	UpdateByePlayer();
}

void rvTourneyGUI::ArenaStart( int arena ) {
	if( tourneyGUI == NULL ) {
		common->Warning( "rvTourneyGUI::ArenaStart() - Invalid tourneyGUI" );
		return;
	}

	//arenaInit sets names to white and scores to orange. needs values "gui::round" , "gui::bracket" and "gui::empty"
	idPlayer** players = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArenaPlayers( arena );
	int round = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetRound();

	tourneyGUI->SetStateInt( "round", round );
	tourneyGUI->SetStateInt( "bracket", arena + 1 );
	if( players[ 0 ] == NULL && players[ 1 ] == NULL ) {
		tourneyGUI->SetStateBool( "empty", true );
	} else {
		tourneyGUI->SetStateBool( "empty", false );	
		tourneyGUI->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), players[ 0 ] ? players[ 0 ]->GetUserInfo()->GetString( "ui_name" ) : common->GetLocalizedString( "#str_107705" ) );
		tourneyGUI->SetStateString( va( "score1_round%d_bracket%d", round, arena + 1 ), players[ 0 ] ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 0 ] ) ) : "" );

		tourneyGUI->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), players[ 1 ] ? players[ 1 ]->GetUserInfo()->GetString( "ui_name" ) : common->GetLocalizedString( "#str_107705" ) );
		tourneyGUI->SetStateString( va( "score2_round%d_bracket%d", round, arena + 1 ), players[ 1 ] ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 1 ] ) ) : "" );
	}
	tourneyGUI->StateChanged( gameLocal.time );

	tourneyGUI->HandleNamedEvent( "arenaInit" );


	if( tourneyScoreboard == NULL ) {
		common->Warning( "rvTourneyGUI::ArenaStart() - Invalid tourneyScoreboard" );
		return;
	}

	tourneyScoreboard->SetStateInt( "round", round );
	tourneyScoreboard->SetStateInt( "bracket", arena + 1 );
	if( players[ 0 ] == NULL && players[ 1 ] == NULL ) {
		tourneyScoreboard->SetStateBool( "empty", true );
	} else {
		tourneyScoreboard->SetStateBool( "empty", false );	
		tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), players[ 0 ] ? players[ 0 ]->GetUserInfo()->GetString( "ui_name" ) : common->GetLocalizedString( "#str_107705" ) );
		tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", round, arena + 1 ), players[ 0 ] ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 0 ] ) ) : "" );

		tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), players[ 1 ] ? players[ 1 ]->GetUserInfo()->GetString( "ui_name" ) : common->GetLocalizedString( "#str_107705" ) );
		tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", round, arena + 1 ), players[ 1 ] ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 1 ] ) ) : "" );
	}
	tourneyScoreboard->StateChanged( gameLocal.time );

	tourneyScoreboard->HandleNamedEvent( "arenaInit" );
}

void rvTourneyGUI::ArenaDone( int arena ) {
	if( tourneyGUI == NULL ) {
		common->Warning( "rvTourneyGUI::ArenaDone() - Invalid tourneyGUI" );
		return;
	}

	rvTourneyArena& tourneyArena = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArena( arena );

	if( tourneyArena.GetPlayers()[ 0 ] == NULL || tourneyArena.GetPlayers()[ 1 ] == NULL ) {
		// we don't transition bye arenas to done
		return;
	}

	// arenaDone transitions the blue flash/fade and names of winner/loser. needs values "gui::round" , "gui::bracket" and "gui::winner" (winner is a 1 or 2 value, 1 top 2 bottom)
	tourneyGUI->SetStateInt( "round", ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetRound() );
	tourneyGUI->SetStateInt( "bracket", arena + 1 );
	
	idPlayer* winner = tourneyArena.GetWinner();
	idPlayer** players = tourneyArena.GetPlayers();

	if( winner == NULL ) {
		common->Error( "rvTourneyGUI::ArenaDone() - Called on arena '%d' which is not done!\n", arena );
		return;
	}

	if( winner != players[ 0 ] && winner != players[ 1 ] ) {
		common->Error( "rvTourneyGUI::ArenaDone() - Arena '%d' is reporting a winner that is not in the arena!\n", arena );
		return;
	}

	tourneyGUI->SetStateInt( "winner", winner == players[ 0 ] ? 1 : 2 );
	tourneyGUI->StateChanged( gameLocal.time );
	tourneyGUI->HandleNamedEvent( "arenaDone" );

	if( tourneyScoreboard == NULL ) {
		common->Warning( "rvTourneyGUI::ArenaDone() - Invalid tourneyScoreboard" );
		return;
	}

	// arenaDone transitions the blue flash/fade and names of winner/loser. needs values "gui::round" , "gui::bracket" and "gui::winner" (winner is a 1 or 2 value, 1 top 2 bottom)
	tourneyScoreboard->SetStateInt( "round", ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetRound() );
	tourneyScoreboard->SetStateInt( "bracket", arena + 1 );

	tourneyScoreboard->SetStateInt( "winner", winner == players[ 0 ] ? 1 : 2 );
	tourneyScoreboard->StateChanged( gameLocal.time );
	tourneyScoreboard->HandleNamedEvent( "arenaDone" );
}

void rvTourneyGUI::ArenaSelect( int arena, tourneyGUIHighlight_t highlightType ) {
	if( tourneyGUI == NULL ) {
		common->Warning( "rvTourneyGUI::ArenaSelect() - Invalid tourneyGUI" );
		return;
	}

	rvTourneyArena& thisArena = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArena( arena );

	// don't select empty arenas
	if( thisArena.GetPlayers()[ 0 ] == NULL || thisArena.GetPlayers()[ 1 ] == NULL ) {
		return;
	}

	//arenaFocus sets the green background on bracket, player 1 or player2 using value "gui::sel". ( 0 = bracket, 1 = player1, 2 = player2) arenaFocus also needs "gui::round" and "gui::bracket" values.
	tourneyGUI->SetStateInt( "round", ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetRound() );
	tourneyGUI->SetStateInt( "bracket", arena + 1 );	
	tourneyGUI->SetStateInt( "sel", (int)highlightType );
	tourneyGUI->StateChanged( gameLocal.time );
	tourneyGUI->HandleNamedEvent( "arenaFocus" );

	if( tourneyScoreboard == NULL ) {
		common->Warning( "rvTourneyGUI::ArenaSelect() - Invalid tourneyScoreboard" );
		return;
	}

	//arenaFocus sets the green background on bracket, player 1 or player2 using value "gui::sel". ( 0 = bracket, 1 = player1, 2 = player2) arenaFocus also needs "gui::round" and "gui::bracket" values.
	tourneyScoreboard->SetStateInt( "round", ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetRound() );
	tourneyScoreboard->SetStateInt( "bracket", arena + 1 );	
	tourneyScoreboard->SetStateInt( "sel", (int)highlightType );
	tourneyScoreboard->StateChanged( gameLocal.time );
	tourneyScoreboard->HandleNamedEvent( "arenaFocus" );
}

void rvTourneyGUI::UpdateScores( void ) {
	if( tourneyGUI == NULL ) {
		common->Warning( "rvTourneyGUI::UpdateScore() - Invalid tourneyGUI" );
		return;
	}

	int round = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetRound();

	if( round < 0 ) {
		// not now
		return;
	}

	for( int i = 0; i < MAX_ARENAS; i++ ) {
		rvTourneyArena& arena = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArena( i );
		idPlayer** players = arena.GetPlayers();
		arenaOutcome_t* arenaOutcome = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArenaOutcome( i );

		if( arena.GetState() != AS_DONE && arena.GetState() != AS_INACTIVE ) {
			tourneyGUI->SetStateString( va( "name1_round%d_bracket%d", round, i + 1 ), players[ 0 ] ? players[ 0 ]->GetUserInfo()->GetString( "ui_name" ) : "" );
			tourneyGUI->SetStateString( va( "score1_round%d_bracket%d", round, i + 1 ), players[ 0 ] ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 0 ] ) ) : "" );

			tourneyGUI->SetStateString( va( "name2_round%d_bracket%d", round, i + 1 ), players[ 1 ] ? players[ 1 ]->GetUserInfo()->GetString( "ui_name" ) : "" );
			tourneyGUI->SetStateString( va( "score2_round%d_bracket%d", round, i + 1 ), players[ 1 ] ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 1 ] ) ) : "" );		

			tourneyGUI->SetStateBool( "empty", false );
		} else if( arenaOutcome ) {
			bool isBye = ( (*(arenaOutcome->playerOne)) == '\0' && arenaOutcome->playerOneNum == -1) || 
							( (*(arenaOutcome->playerTwo)) == '\0' && arenaOutcome->playerTwoNum == -1);
			
			if( *(arenaOutcome->playerOne) ) {
				tourneyGUI->SetStateString( va( "name1_round%d_bracket%d", round, i + 1 ), arenaOutcome->playerOne );
				tourneyGUI->SetStateString( va( "score1_round%d_bracket%d", round, i + 1 ), isBye ? "" : va( "%d", arenaOutcome->playerOneScore ) );
			} else {
				tourneyGUI->SetStateString( va( "name1_round%d_bracket%d", round, i + 1 ), players[ 0 ] ? players[ 0 ]->GetUserInfo()->GetString( "ui_name" ) : common->GetLocalizedString( "#str_107705" ) );
				tourneyGUI->SetStateString( va( "score1_round%d_bracket%d", round, i + 1 ), (players[ 0 ] && !isBye) ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 0 ] ) ) : "" );
			}

			if( *(arenaOutcome->playerTwo) ) {
				tourneyGUI->SetStateString( va( "name2_round%d_bracket%d", round, i + 1 ), arenaOutcome->playerTwo );
				tourneyGUI->SetStateString( va( "score2_round%d_bracket%d", round, i + 1 ), isBye ? "" : va( "%d", arenaOutcome->playerTwoScore ) );			
			} else {
				tourneyGUI->SetStateString( va( "name2_round%d_bracket%d", round, i + 1 ), players[ 1 ] ? players[ 1 ]->GetUserInfo()->GetString( "ui_name" ) : common->GetLocalizedString( "#str_107705" ) );
				tourneyGUI->SetStateString( va( "score2_round%d_bracket%d", round, i + 1 ), (players[ 1 ] && !isBye) ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 1 ] ) ) : "" );		
			}

			tourneyGUI->SetStateBool( "empty", false );
		}
	}

	tourneyGUI->StateChanged( gameLocal.time );

	if( tourneyScoreboard == NULL ) {
		common->Warning( "rvTourneyGUI::UpdateScore() - Invalid tourneyScoreboard" );
		return;
	}

	for( int i = 0; i < MAX_ARENAS; i++ ) {
		rvTourneyArena& arena = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArena( i );
		idPlayer** players = arena.GetPlayers();
		arenaOutcome_t* arenaOutcome = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArenaOutcome( i );

		if( arena.GetState() != AS_DONE && arena.GetState() != AS_INACTIVE ) {
			tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, i + 1 ), players[ 0 ] ? players[ 0 ]->GetUserInfo()->GetString( "ui_name" ) : "" );
			tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", round, i + 1 ), players[ 0 ] ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 0 ] ) ) : "" );

			tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, i + 1 ), players[ 1 ] ? players[ 1 ]->GetUserInfo()->GetString( "ui_name" ) : "" );
			tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", round, i + 1 ), players[ 1 ] ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 1 ] ) ) : "" );		

			tourneyScoreboard->SetStateBool( "empty", false );
		} else if( arenaOutcome ) {
			bool isBye = ( (*(arenaOutcome->playerOne)) == '\0' && arenaOutcome->playerOneNum == -1) || 
				( (*(arenaOutcome->playerTwo)) == '\0' && arenaOutcome->playerTwoNum == -1);

			if( *(arenaOutcome->playerOne) ) {
				tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, i + 1 ), arenaOutcome->playerOne );
				tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", round, i + 1 ), isBye ? "" : va( "%d", arenaOutcome->playerOneScore ) );
			} else {
				tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, i + 1 ), players[ 0 ] ? players[ 0 ]->GetUserInfo()->GetString( "ui_name" ) : common->GetLocalizedString( "#str_107705" ) );
				tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", round, i + 1 ), (players[ 0 ] && !isBye) ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 0 ] ) ) : "" );
			}

			if( *(arenaOutcome->playerTwo) ) {
				tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, i + 1 ), arenaOutcome->playerTwo );
				tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", round, i + 1 ), isBye ? "" : va( "%d", arenaOutcome->playerTwoScore ) );			
			} else {
				tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, i + 1 ), players[ 1 ] ? players[ 1 ]->GetUserInfo()->GetString( "ui_name" ) : common->GetLocalizedString( "#str_107705" ) );
				tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", round, i + 1 ), (players[ 1 ] && !isBye) ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 1 ] ) ) : "" );		
			}

			tourneyScoreboard->SetStateBool( "empty", false );
		}
	}

	tourneyScoreboard->StateChanged( gameLocal.time );
}

void rvTourneyGUI::PreTourney( void ) {
	if( tourneyGUI == NULL ) {
		common->Warning( "rvTourneyGUI::PreTourney() - Invalid tourneyGUI" );
		return;
	}

	tourneyGUI->HandleNamedEvent( "warmupTransitionIn" );
}

void rvTourneyGUI::TourneyStart( void ) {
	if( tourneyGUI == NULL ) {
		common->Warning( "rvTourneyGUI::TourneyStart() - Invalid tourneyGUI" );
		return;
	}

	int round = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetRound();
	int maxRound = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetMaxRound();

	tourneyGUI->SetStateInt( "round", round );
	tourneyGUI->StateChanged( gameLocal.time );

	tourneyGUI->HandleNamedEvent( "warmupTransitionOut" );

	// setup and clear the scoreboard
	if( tourneyScoreboard == NULL ) {
		common->Warning( "rvTourneyGUI::TourneyStart() - Invalid tourneyScoreboard" );
		return;
	}

	for( int i = 1; i <= maxRound; i++ ) {
		for( int j = 0; j < MAX_ARENAS; j++ ) {
			tourneyScoreboard->SetStateInt( "round", i );
			tourneyScoreboard->SetStateInt( "bracket", j + 1 );

			if( i < round ) {
				// we aren't using these brackets
				tourneyScoreboard->SetStateBool( "empty", true );
				tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", i, j + 1 ), "" );
				tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", i, j + 1 ), "" );
				tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", i, j + 1 ), "" );
				tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", i, j + 1 ), "" );
			} else if ( i == round ) {
				// this is our initial round
				idPlayer** players = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArenaPlayers( j );

				if( players[ 0 ] == NULL && players[ 1 ] == NULL ) {
					tourneyScoreboard->SetStateBool( "empty", true );
				} else {
					tourneyScoreboard->SetStateBool( "empty", false );	
					tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", i, j + 1 ), players[ 0 ] ? players[ 0 ]->GetUserInfo()->GetString( "ui_name" ) : common->GetLocalizedString( "#str_107705" ) );
					tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", i, j + 1 ), players[ 0 ] ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 0 ] ) ) : "" );

					tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", i, j + 1 ), players[ 1 ] ? players[ 1 ]->GetUserInfo()->GetString( "ui_name" ) : common->GetLocalizedString( "#str_107705" ) );
					tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", i, j + 1 ), players[ 1 ] ? va( "%d", gameLocal.mpGame.GetTeamScore( players[ 1 ] ) ) : "" );
				}
			} else {	
				// this is our future bracket
				tourneyScoreboard->SetStateBool( "empty", false );
				tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", i, j + 1 ), "" );
				tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", i, j + 1 ), "" );
				tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", i, j + 1 ), "" );
				tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", i, j + 1 ), "" );
			}	
			tourneyScoreboard->StateChanged( gameLocal.time );
			tourneyScoreboard->HandleNamedEvent( "arenaInit" );
		}
	}

	// we might have overwritten a bye player, so update it again
//	UpdateByePlayer();
}

/*void rvTourneyGUI::UpdateByePlayer( void ) {
	if( tourneyGUI == NULL ) {
		common->Warning( "rvTourneyGUI::UpdateByePlayer() - Invalid tourneyGUI" );
		return;
	}

	int arena;
	for( arena = 0; arena < MAX_ARENAS; arena++ ) {
		if( ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArena( arena ).GetState() == AS_INACTIVE ) {
			break;
		}
	}

	if( arena >= MAX_ARENAS ) {
		common->Warning( "rvTourneyGUI::UpdateByePlayer() - Bye player with no inactive arenas!" );
		return;
	}

	idPlayer* byePlayer = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetByePlayer();

	int round = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetRound();
	tourneyGUI->SetStateInt( "round", round );
	tourneyGUI->SetStateInt( "bracket", arena + 1 );

	if( byePlayer ) {
		tourneyGUI->SetStateBool( "empty", false );	
		tourneyGUI->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), byePlayer->GetUserInfo()->GetString( "ui_name" ) );
		tourneyGUI->SetStateString( va( "score1_round%d_bracket%d", round, arena + 1 ), "" );

		tourneyGUI->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), common->GetLocalizedString( "#str_107705" ) );
		tourneyGUI->SetStateString( va( "score2_round%d_bracket%d", round, arena + 1 ), "" );
	} else {
		tourneyGUI->SetStateBool( "empty", true );	
		tourneyGUI->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), "" );
		tourneyGUI->SetStateString( va( "score1_round%d_bracket%d", round, arena + 1 ), "" );

		tourneyGUI->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), "" );
		tourneyGUI->SetStateString( va( "score2_round%d_bracket%d", round, arena + 1 ), "" );
	}


	tourneyGUI->StateChanged( gameLocal.time );

	tourneyGUI->HandleNamedEvent( "arenaInit" );

	if( tourneyScoreboard == NULL ) {
		common->Warning( "rvTourneyGUI::UpdateByePlayer() - Invalid tourneyScoreboard" );
		return;
	}

	tourneyScoreboard->SetStateInt( "round", round );
	tourneyScoreboard->SetStateInt( "bracket", arena + 1 );

	if( byePlayer ) {
		tourneyScoreboard->SetStateBool( "empty", false );	
		tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), byePlayer->GetUserInfo()->GetString( "ui_name" ) );
		tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", round, arena + 1 ), "" );

		tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), common->GetLocalizedString( "#str_107705" ) );
		tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", round, arena + 1 ), "" );
	} else {
		tourneyScoreboard->SetStateBool( "empty", true );	
		tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), "" );
		tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", round, arena + 1 ), "" );

		tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), "" );
		tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", round, arena + 1 ), "" );
	}

	tourneyScoreboard->StateChanged( gameLocal.time );

	tourneyScoreboard->HandleNamedEvent( "arenaInit" );
}*/

void rvTourneyGUI::SetupTourneyHistory( int startHistory, int endHistory, arenaOutcome_t tourneyHistory[ MAX_ROUNDS ][ MAX_ARENAS ] ) {
	if ( tourneyScoreboard == NULL ) {
		common->Warning( "rvTourneyGUI::SetupTourneyHistory() - Invalid tourneyScoreboard" );
		return;
	}

	if( startHistory <= 0 ) {
		return;
	}

	for ( int round = startHistory; round <= endHistory; round++ ) {
		tourneyScoreboard->SetStateInt( "round", round );

		for ( int arena = 0; arena < MAX_ARENAS / round; arena++ ) {
			tourneyScoreboard->SetStateInt( "bracket", arena + 1 );

			// whether we want to send arenaDone to this arena
			// don't send for empty brackets or bye rounds
			bool arenaNotDone = tourneyHistory[ round - 1 ][ arena ].playerOneNum == -1 && tourneyHistory[ round - 1 ][ arena ].playerTwoNum == -1 && !(*tourneyHistory[ round - 1 ][ arena ].playerOne) && !(*tourneyHistory[ round - 1 ][ arena ].playerTwo);

			if( arenaNotDone ) {
				tourneyScoreboard->SetStateBool( "empty", true );
			} else {
				tourneyScoreboard->SetStateBool( "empty", false );

				bool firstSlotBye = (tourneyHistory[ round - 1 ][ arena ].playerOneNum == -1 && !(*tourneyHistory[ round - 1 ][ arena ].playerOne));
				bool secondSlotBye = (tourneyHistory[ round - 1 ][ arena ].playerTwoNum == -1 && !(*tourneyHistory[ round - 1 ][ arena ].playerTwo));

				assert( !(firstSlotBye && secondSlotBye) );

				if( firstSlotBye || secondSlotBye ) {
					// this was a bye round

					if( secondSlotBye ) {
						if( tourneyHistory[ round - 1 ][ arena ].playerOneNum == -1 || !gameLocal.GetUserInfo( tourneyHistory[ round - 1 ][ arena ].playerOneNum ) ) {
							tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), tourneyHistory[ round - 1 ][ arena ].playerOne );
						} else {
							tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), gameLocal.GetUserInfo( tourneyHistory[ round - 1 ][ arena ].playerOneNum )->GetString( "ui_name" ) );
						}

						tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", round, arena + 1 ), "" );

						tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), common->GetLocalizedString( "#str_107705" ) );
						tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", round, arena + 1 ), "" );
					} else {
						if( tourneyHistory[ round - 1 ][ arena ].playerTwoNum == -1 || !gameLocal.GetUserInfo( tourneyHistory[ round - 1 ][ arena ].playerTwoNum ) ) {
							tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), tourneyHistory[ round - 1 ][ arena ].playerTwo );
						} else {
							tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), gameLocal.GetUserInfo( tourneyHistory[ round - 1 ][ arena ].playerTwoNum )->GetString( "ui_name" ) );
						}

						tourneyScoreboard->SetStateString( va( "score2_round%d_bracket%d", round, arena + 1 ), "" );

						tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), common->GetLocalizedString( "#str_107705" ) );
						tourneyScoreboard->SetStateString( va( "score1_round%d_bracket%d", round, arena + 1 ), "" );
					}

					arenaNotDone = true;
				} else {
					// regular round
					if( tourneyHistory[ round - 1 ][ arena ].playerOneNum == -1 || !gameLocal.GetUserInfo( tourneyHistory[ round - 1 ][ arena ].playerOneNum ) ) {
						tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), tourneyHistory[ round - 1 ][ arena ].playerOne );
					} else {
						tourneyScoreboard->SetStateString( va( "name1_round%d_bracket%d", round, arena + 1 ), gameLocal.GetUserInfo( tourneyHistory[ round - 1 ][ arena ].playerOneNum )->GetString( "ui_name" ) );
					}
					
					tourneyScoreboard->SetStateInt( va( "score1_round%d_bracket%d", round, arena + 1 ), tourneyHistory[ round - 1 ][ arena ].playerOneScore );

					if( tourneyHistory[ round - 1 ][ arena ].playerTwoNum == -1 || !gameLocal.GetUserInfo( tourneyHistory[ round - 1 ][ arena ].playerTwoNum ) ) {
						tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), tourneyHistory[ round - 1 ][ arena ].playerTwo );
					} else {
						tourneyScoreboard->SetStateString( va( "name2_round%d_bracket%d", round, arena + 1 ), gameLocal.GetUserInfo( tourneyHistory[ round - 1 ][ arena ].playerTwoNum )->GetString( "ui_name" ) );
					}

					tourneyScoreboard->SetStateInt( va( "score2_round%d_bracket%d", round, arena + 1 ), tourneyHistory[ round - 1 ][ arena ].playerTwoScore );

					tourneyScoreboard->SetStateInt( "winner", tourneyHistory[ round - 1 ][ arena ].playerOneScore > tourneyHistory[ round - 1 ][ arena ].playerTwoScore ? 1 : 2 );
				}
			}
			tourneyScoreboard->StateChanged( gameLocal.time );
			tourneyScoreboard->HandleNamedEvent( "arenaInit" );
			if( !arenaNotDone ) {
				tourneyScoreboard->HandleNamedEvent( "arenaDone" );
			}
		}
	}

	UpdateScores();
}
