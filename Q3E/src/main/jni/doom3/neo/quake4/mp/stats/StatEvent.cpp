//----------------------------------------------------------------
// StatEvent.cpp
//
// Copyright 2002-2005 Raven Software
//----------------------------------------------------------------

#include "../../../idlib/precompiled.h"
#pragma hdrstop

#include "../../Game_local.h"
#include "StatManager.h"


rvStatHit::rvStatHit( int t, int p, int v, int w, bool countForAccuracy ) : rvStat( t ) { 
	playerClientNum = p;
	victimClientNum = v;
	weapon = w;
	trackAccuracy = countForAccuracy;
	type = ST_HIT; 
	RegisterInGame( statManager->GetPlayerStats() );
}

void rvStatHit::RegisterInGame( rvPlayerStat* stats ) {
	if( playerClientNum >= 0 && playerClientNum < MAX_CLIENTS && weapon >= 0 && weapon < MAX_WEAPONS && trackAccuracy ) {
		stats[ playerClientNum ].weaponHits[ weapon ]++;	
	}
	idPlayer* player = (idPlayer*)gameLocal.entities[ playerClientNum ];
	if( !idStr::Icmp( player->spawnArgs.GetString( va( "def_weapon%d", weapon ) ), "weapon_railgun" ) ) {
	
		// Apparently it does the rail hit event before the rail fired event, so this is backwards for a reason
		if ( rvStatManager::comboKillState[ playerClientNum ] == CKS_ROCKET_HIT ) {
			rvStatManager::comboKillState[ playerClientNum ] = CKS_RAIL_HIT;
		}
		
		if ( rvStatManager::lastRailShot[ playerClientNum ] < stats[ playerClientNum ].weaponShots[ weapon ] - 1 ) {
			rvStatManager::lastRailShot[ playerClientNum ] = stats[ playerClientNum ].weaponShots[ weapon ];
		} else {
			if ( rvStatManager::lastRailShot[ playerClientNum ] >= stats[ playerClientNum ].weaponShots[ weapon ] - 1 ) {
				if ( (rvStatManager::lastRailShotHits[ playerClientNum ] % 2) == 0 ) {
					statManager->GiveInGameAward( IGA_IMPRESSIVE, playerClientNum );
				}
				++rvStatManager::lastRailShotHits[ playerClientNum ];
			} else {
				rvStatManager::lastRailShot[ playerClientNum ] = stats[ playerClientNum ].weaponShots[ weapon ];
			}
		}
		
/*		
		rvStatHit* lastHits[ 2 ] = { NULL, NULL };
		statManager->GetLastClientStats( playerClientNum, ST_HIT, gameLocal.time - 8000, 2, (rvStat**)lastHits );
		if( lastHits[0] ) {
			if(  !idStr::Icmp(player->spawnArgs.GetString( va( "def_weapon%d", ((rvStatHit*)lastHits[0])->weapon ) ), "weapon_railgun" ) ) {

				//Check to make sure we don't chain impressive awards.
				if(lastHits[1] 
				&& !idStr::Icmp(player->spawnArgs.GetString( va( "def_weapon%d", ((rvStatHit*)lastHits[1])->weapon ) ), "weapon_railgun" )
					&& (lastHits[1]->GetTimeStamp() - lastHits[0]->GetTimeStamp()) < 4000) {
						return;
				
				}

				if(gameLocal.time - lastHits[0]->GetTimeStamp() < 4000){
					statManager->GiveInGameAward( IGA_IMPRESSIVE, playerClientNum );
				}
			}
		}
*/	
	} else if( !idStr::Icmp( player->spawnArgs.GetString( va( "def_weapon%d", weapon ) ), "weapon_rocketlauncher" ) ) {
		
		if ( rvStatManager::comboKillState[ playerClientNum ] == CKS_ROCKET_FIRED ) {
			rvStatManager::comboKillState[ playerClientNum ] = CKS_ROCKET_HIT;
		}
		
	}
}

rvStatKill::rvStatKill( int t, int p, int v, bool g, int mod ) : rvStat( t ) { 
	type = ST_KILL; 
	playerClientNum = p;
	victimClientNum = v;
	gibbed = g;
	methodOfDeath = mod;
	RegisterInGame( statManager->GetPlayerStats() );
}

void rvStatKill::RegisterInGame( rvPlayerStat* stats ) {
	if( playerClientNum < 0 || playerClientNum >= MAX_CLIENTS || victimClientNum < 0 || victimClientNum >= MAX_CLIENTS ) {
		return;
	}

	idPlayer* player = (idPlayer*)gameLocal.entities[ playerClientNum ];
	idPlayer* victim = (idPlayer*)gameLocal.entities[ victimClientNum ];

	// no award processing for suicides
	if( victimClientNum == playerClientNum ) {
		stats[ playerClientNum ].suicides++;		
		return;
	}

	bool teamKill = false;

	// don't track team kills
	if( !gameLocal.IsTeamGame() || player->team != victim->team ) {
		stats[ playerClientNum ].kills++;

		if( methodOfDeath >= 0 && methodOfDeath < MAX_WEAPONS ) {
			stats[ playerClientNum ].weaponKills[ methodOfDeath ]++;
		}
	}

	if( gameLocal.IsTeamGame() && player->team == victim->team ) {
		teamKill = true;
	}


	// check for humiliation award
	if( !teamKill && !idStr::Icmp( player->spawnArgs.GetString( va( "def_weapon%d", methodOfDeath ) ), "weapon_gauntlet" ) ) {
		statManager->GiveInGameAward( IGA_HUMILIATION, playerClientNum );
	}

	rvStatKill* lastKills[ 2 ] = { NULL, NULL };

	statManager->GetLastClientStats( playerClientNum, ST_KILL, gameLocal.time - 5000, 2, (rvStat**)lastKills );

	// check for excellent award
	if( !teamKill && lastKills[ 0 ] && lastKills[ 0 ]->GetTimeStamp() >= (gameLocal.time - 2000) && lastKills[ 0 ]->victimClientNum != playerClientNum && victimClientNum != playerClientNum ) {
		statManager->GiveInGameAward( IGA_EXCELLENT, playerClientNum );
	}

	// check for rampage award
	if( !teamKill && ( gibbed && victimClientNum != playerClientNum ) && 
		( lastKills[ 0 ] && lastKills[ 0 ]->gibbed && lastKills[ 0 ]->victimClientNum != playerClientNum ) && 
		( lastKills[ 1 ] && lastKills[ 1 ]->gibbed && lastKills[ 1 ]->victimClientNum != playerClientNum ) ) {
			statManager->GiveInGameAward( IGA_RAMPAGE, playerClientNum );
	}


	// check for combo kill award
	if( victimClientNum != playerClientNum && !idStr::Icmp( player->spawnArgs.GetString( va( "def_weapon%d", methodOfDeath ) ), "weapon_railgun" ) ) {
		// the rail killing shot is the last hit, so look for the one past that
		rvStatHit* lastHits[ 2 ] = { NULL, NULL };

		statManager->GetLastClientStats( playerClientNum, ST_HIT, gameLocal.time - 3000, 2, (rvStat**)lastHits );

		if( lastHits[ 1 ] && lastHits[ 1 ]->GetVictimClientNum() != playerClientNum && !idStr::Icmp( player->spawnArgs.GetString( va( "def_weapon%d", lastHits[ 1 ]->GetWeapon() ) ), "weapon_rocketlauncher" )  ) {
			if ( rvStatManager::comboKillState[ playerClientNum ] == CKS_RAIL_HIT ) {
				statManager->GiveInGameAward( IGA_COMBO_KILL, playerClientNum );
			}
		}
	}
	
	rvStatManager::comboKillState[ playerClientNum ] = CKS_NONE;


	// check for defense award
	if( gameLocal.IsFlagGameType() && player->team != victim->team && player != victim ) {
		// defense is given for two conditions
//		assert( gameLocal.mpGame.GetFlagEntity( TEAM_STROGG ) && gameLocal.mpGame.GetFlagEntity( TEAM_MARINE ) );
		assert( player->team >= 0 && player->team < TEAM_MAX );
		assert( gameLocal.mpGame.GetGameState()->IsType( rvCTFGameState::GetClassType() ) );

		if ( gameLocal.mpGame.GetFlagEntity( player->team ) ) {		
			// defending your flag
			if( ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetFlagState( player->team ) == FS_AT_BASE || ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetFlagState( player->team ) == FS_DROPPED ) {
				if( ( gameLocal.mpGame.GetFlagEntity( player->team )->GetPhysics()->GetOrigin() - player->GetPhysics()->GetOrigin() ).LengthSqr() < 250000 ) {
					// give award if you're close to flag and you kill an enemy
					statManager->GiveInGameAward( IGA_DEFENSE, playerClientNum );
				} else if ( ( gameLocal.mpGame.GetFlagEntity( player->team )->GetPhysics()->GetOrigin() - victim->GetPhysics()->GetOrigin() ).LengthSqr() < 250000 ) {
					// give award if enemy is close to flag and you kill them
					statManager->GiveInGameAward( IGA_DEFENSE, playerClientNum );
				}
			} 
		}
		
		// defending your teammate carrying the enemy flag
		if( ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetFlagState( gameLocal.mpGame.OpposingTeam( player->team ) ) == FS_TAKEN ) {
			int clientNum = ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetFlagCarrier( gameLocal.mpGame.OpposingTeam( player->team ) );

			idPlayer* flagCarrier = NULL;

			if( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
				flagCarrier = (idPlayer*)gameLocal.entities[ clientNum ];
			}

			if( flagCarrier ) {
				if( (flagCarrier->GetPhysics()->GetOrigin() - player->GetPhysics()->GetOrigin()).LengthSqr() < 250000 ) {
					// killed enemy while close to the flag carrier
					statManager->GiveInGameAward( IGA_DEFENSE, playerClientNum );
				} else if( (flagCarrier->GetPhysics()->GetOrigin() - victim->GetPhysics()->GetOrigin()).LengthSqr() < 250000 ) {
					// killed an enemy who was close to teh flag carrier
					statManager->GiveInGameAward( IGA_DEFENSE, playerClientNum );
				}
			}
		}
	}

	// check for holy shit award
	if( gameLocal.IsFlagGameType() && player->team != victim->team && player != victim ) {
		if( (player->team == TEAM_MARINE && victim->PowerUpActive( POWERUP_CTF_MARINEFLAG )) ||
			(player->team == TEAM_STROGG && victim->PowerUpActive( POWERUP_CTF_STROGGFLAG )) ) {
			if( ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetFlagState( victim->team ) == FS_AT_BASE ) {
				
				// fixme: something is broken with the mpgame state
				if ( gameLocal.mpGame.GetFlagEntity( victim->team ) ) {
					if( (gameLocal.mpGame.GetFlagEntity( victim->team )->GetPhysics()->GetOrigin() - victim->GetPhysics()->GetOrigin()).LengthSqr() < 40000 ) {
						statManager->GiveInGameAward( IGA_HOLY_SHIT, playerClientNum );					
					}
				}
			}
		}
	}
}

/*
================
rvStatDeath

A player died
================
*/
rvStatDeath::rvStatDeath( int t, int p, int mod ) : rvStat( t ) {
	type = ST_DEATH; 
	playerClientNum = p;
	RegisterInGame( statManager->GetPlayerStats() );
}

void rvStatDeath::RegisterInGame( rvPlayerStat* stats ) {
	stats[ playerClientNum ].deaths++;	
}

/*
================
rvStatDamageDealt

A player damaged another player
================
*/
rvStatDamageDealt::rvStatDamageDealt( int t, int p, int w, int d ) : rvStat( t ) { 
	playerClientNum = p;
	weapon = w;
	damage = d;
	type = ST_DAMAGE_DEALT; 
	RegisterInGame( statManager->GetPlayerStats() );
}

void rvStatDamageDealt::RegisterInGame( rvPlayerStat* stats ) {
	if( playerClientNum >= 0 && playerClientNum < MAX_CLIENTS ) {
		stats[ playerClientNum ].damageGiven += damage;	
	}
}

/*
================
rvStatDamageTaken

A player took damage from another player
================
*/
rvStatDamageTaken::rvStatDamageTaken( int t, int p, int w, int d ) : rvStat( t ) { 
	playerClientNum = p;
	weapon = w;
	damage = d;
	type = ST_DAMAGE_TAKEN; 
	RegisterInGame( statManager->GetPlayerStats() );
}

void rvStatDamageTaken::RegisterInGame( rvPlayerStat* stats ) {
	if( playerClientNum >= 0 && playerClientNum < MAX_CLIENTS ) {
		stats[ playerClientNum ].damageTaken += damage;	
	}
}

/*
================
rvStatFlagDrop

A player dropped the flag (CTF)
================
*/
rvStatFlagDrop::rvStatFlagDrop( int t, int p, int a, int tm ) : rvStatTeam( t, tm ) { 
	playerClientNum = p;
	attacker = a;
	type = ST_CTF_FLAG_DROP; 
}

/*
================
rvStatFlagReturn

A player returned his teams flag
================
*/
rvStatFlagReturn::rvStatFlagReturn( int t, int p, int tm ) : rvStatTeam( t, tm ) { 
	playerClientNum = p;
	type = ST_CTF_FLAG_RETURN; 
}

/*
================
rvStatFlagCapture

A player captured a flag (CTF)
================
*/
rvStatFlagCapture::rvStatFlagCapture( int t, int p, int f, int tm ) : rvStatTeam( t, tm ) { 
	playerClientNum = p;
	flagTeam = f;
	type = ST_CTF_FLAG_CAPTURE; 
	RegisterInGame( statManager->GetPlayerStats() );
}

void rvStatFlagCapture::RegisterInGame( rvPlayerStat* stats ) {
	statManager->GiveInGameAward( IGA_CAPTURE, playerClientNum );
	// figure out if we need to give an assist

	// see if someone carried the flag a bit, lost it, but it was capped
	rvStatFlagDrop* flagDrop = (rvStatFlagDrop*)statManager->GetLastTeamStat( team, ST_CTF_FLAG_DROP, gameLocal.time - 10000 );
	if( flagDrop && flagDrop->GetTeam() == team ) {
		// only give the award if there was no flag return between the flag drop and the flag capture
		rvStatFlagReturn* flagReturn = (rvStatFlagReturn*)statManager->GetLastTeamStat( flagTeam, ST_CTF_FLAG_RETURN, gameLocal.time - 10000 );
		if( flagReturn == NULL ) {
			statManager->GiveInGameAward( IGA_ASSIST, flagDrop->GetPlayerClientNum() );
		}
	}

	// see if someone returned the flag, allowing a cap
	rvStatFlagReturn* flagReturn = (rvStatFlagReturn*)statManager->GetLastTeamStat( team, ST_CTF_FLAG_RETURN, gameLocal.time - 10000 );
	if( flagReturn ) {
		statManager->GiveInGameAward( IGA_ASSIST, flagReturn->GetPlayerClientNum() );
	}
}
