//----------------------------------------------------------------
// StatManager.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../../idlib/precompiled.h"
#pragma hdrstop

#include "../../Game_local.h"
#include "StatManager.h"

// TTimo - *????????*
#include <new>

rvStatManager	statManagerLocal;
rvStatManager*	statManager = &statManagerLocal;

comboKillState_t rvStatManager::comboKillState[ MAX_CLIENTS ] = { CKS_NONE };
int				rvStatManager::lastRailShot[ MAX_CLIENTS ] = { -2 };
int				rvStatManager::lastRailShotHits[ MAX_CLIENTS ] = { 0 };

inGameAwardInfo_t inGameAwardInfo[ IGA_NUM_AWARDS ] = {
	//IGA_INVALID
	{ 
		NULL
	},
	//IGA_CAPTURE
	{
		"capture"
	},
	//IGA_HUMILIATION
	{
		"humiliation"
	},
	//IGA_IMPRESSIVE
	{
		"impressive"
	},
	//IGA_EXCELLENT
	{
		"excellent"
	},
	//IGA_ASSIST
	{
		"assist"
	},
	//IGA_DEFENSE
	{
		"defense"
	},
	//IGA_COMBO_KILL
	{
		"combo_kill"
	},
	//IGA_RAMPAGE
	{
		"rampage"
	},
	//IGA_HOLY_SHIT
	{
		"holy_shit"
	}
};

// RAVEN BEGIN
// rhummer: localized these strings.
endGameAwardInfo_t endGameAwardInfo[ EGA_NUM_AWARDS ] = {
	//EGA_INVALID
	{ 
		NULL
	},
	//EGA_LEMMING
	{
		"#str_107260"
	},
	//EGA_RAIL_MASTER
	{
		"#str_107261"
	},
	//EGA_ROCKET_SAUCE
	{
		"#str_107262"
	},
	//EGA_BRAWLER
	{
		"#str_107263"
	},
	//EGA_SNIPER
	{
		"#str_107264"
	},
	//EGA_CRITICAL_FAILURE
	{
		"#str_107265"
	},
	//EGA_TEAM_PLAYER
	{
		"#str_107266"
	},
	//EGA_ACCURACY
	{
		"#str_107267"
	},
	//EGA_FRAGS
	{
		"#str_107268"
	},
	//EGA_PERFECT
	{
		"#str_107269"
	}
};
// RAVEN END

void showStats_f( const idCmdArgs &args ) {

	statManager->DebugPrint();
}


/*
===============================================================================

	rvStatAllocator

	Handles allocating stat events

===============================================================================
*/

void *rvStatAllocator::GetBlock( size_t blockSize, int* blockNumOut /* = NULL  */  ) {
	// if not enough bytes for this allocation, get a new block
	if ( GetBytesLeftInBlock() < blockSize ) {
		// recycle & reuse a block
		currentBlock++;
		placeInBlock = 0;
		if( currentBlock >= MAX_BLOCKS ) {
			currentBlock = 0;
			if( statManager->GetStat( 0 ) && gameLocal.isMultiplayer ) {
//				int delta = gameLocal.time - statManager->GetStat( 0 )->GetTimeStamp();
//				gameLocal.Printf( "rvStatAllocator::GetBlock() - Tracked %g seconds of stats before recycle\n", delta / 1000.0f );
			}
		}
//		gameLocal.Printf( "rvStatAllocator::GetBlock() - Using block %d\n", currentBlock );
//		statManager->DebugPrint();
//		int numFreed = 
			statManager->FreeEvents( currentBlock );
//		statManager->DebugPrint();
//		gameLocal.Printf( "rvStatAllocator::GetBlock() - stat manager freed %d events which used block %d\n", numFreed, currentBlock );
	}

	// if this fires our objects are too large or our alloc size is too small
	assert( GetBytesLeftInBlock() >= blockSize );

	byte *newBlock = blocks + ( BLOCK_SIZE * currentBlock ) + placeInBlock;
	placeInBlock += blockSize;
	totalAllocations++;
	totalBytesUsed += blockSize;
	if( blockNumOut ) {
		// return the current block number if requested
		(*blockNumOut) = currentBlock;
	}
	return newBlock;
}

void rvStatAllocator::Reset() {
	currentBlock = 0;
	placeInBlock = 0;

	totalBytesUsed = 0;
	totalAllocations = 0;
	totalBytesAllocated = 0;
	for ( int i = 0; i < ST_COUNT; i++ ) {
		allocationsByType[ i ] = 0;
	}
}

rvStatAllocator::rvStatAllocator() {
	Reset();
}

void rvStatAllocator::Report()
{
	// shouchard:  for debugging and tuning only
	common->Printf( "rvStatAllocator:  dump of usage stats\n" );
	common->Printf( "\t%d total bytes handed out in %d requests\n", GetTotalBytesUsed(), GetTotalAllocations() );
	common->Printf( "\tbegin game:    %3d;  end game:      %3d\n", 
		GetAllocationsByType( ST_BEGIN_GAME ), 
		GetAllocationsByType( ST_END_GAME ) );
	common->Printf( "\tplayer hit:    %3d;  player kill:   %3d\n", 
		GetAllocationsByType( ST_HIT ), 
		GetAllocationsByType( ST_KILL ) );
	common->Printf( "\tplayer death:  %3d;\n", 
		GetAllocationsByType( ST_DEATH ) );
	common->Printf( "\tdamage dealt:  %3d;  damage taken:  %3d\n", 
		GetAllocationsByType( ST_DAMAGE_DEALT ),
		GetAllocationsByType( ST_DAMAGE_TAKEN ) );
	common->Printf( "\tstat team:     %3d\n",
		GetAllocationsByType( ST_STAT_TEAM ) );
	common->Printf( "\tflag capture:  %3d;\n",
		GetAllocationsByType( ST_CTF_FLAG_CAPTURE ) ),
	common->Printf( "\tflag drop:     %3d;  flag return:   %3d\n",
		GetAllocationsByType( ST_CTF_FLAG_DROP ),
		GetAllocationsByType( ST_CTF_FLAG_RETURN ) );
}

// object allocators

#if defined(_INLINEDEBUGMEMORY)
// Because we need inplace new.
#undef new
#define new	new
#endif // _INLINEDEBUGMEMORY

rvStatBeginGame *rvStatAllocator::AllocStatBeginGame( int t, int* blockNumOut /* = NULL  */ ) {
 	void *newBlock = GetBlock( sizeof( rvStatBeginGame ), blockNumOut );
	assert( newBlock );
	rvStatBeginGame *stat = new( newBlock ) rvStatBeginGame( t );
	allocationsByType[ ST_BEGIN_GAME ]++;
	return stat;
}

rvStatEndGame *rvStatAllocator::AllocStatEndGame( int t, int* blockNumOut /* = NULL  */ ) {
	void *newBlock = GetBlock( sizeof( rvStatEndGame ), blockNumOut );
	assert( newBlock );
	rvStatEndGame *stat = new( newBlock ) rvStatEndGame( t );
	allocationsByType[ ST_END_GAME ]++;
	return stat;
}

rvStatClientConnect *rvStatAllocator::AllocStatClientConnect( int t, int client, int* blockNumOut /* = NULL  */ ) {
	void *newBlock = GetBlock( sizeof( rvStatClientConnect ), blockNumOut );
	assert( newBlock );
	rvStatClientConnect *stat = new( newBlock ) rvStatClientConnect( t, client );
	allocationsByType[ ST_CLIENT_CONNECT ]++;
	return stat;
}

rvStatHit *rvStatAllocator::AllocStatHit( int t, int p, int v, int w, bool countForAccuracy, int* blockNumOut /* = NULL  */ ) {
	void *newBlock = GetBlock( sizeof( rvStatHit ), blockNumOut );
	assert( newBlock );
	rvStatHit *stat = new( newBlock ) rvStatHit( t, p, v, w, countForAccuracy );
	allocationsByType[ ST_HIT ]++;
	return stat;
}

rvStatKill *rvStatAllocator::AllocStatKill( int t, int p, int v, bool g, int mod, int* blockNumOut /* = NULL  */ ) {
	void *newBlock = GetBlock( sizeof( rvStatKill ), blockNumOut );
	assert( newBlock );
	rvStatKill *stat = new( newBlock ) rvStatKill( t, p, v, g, mod );
	allocationsByType[ ST_KILL ]++;
	return stat;
}

rvStatDeath *rvStatAllocator::AllocStatDeath( int t, int p, int mod, int* blockNumOut /* = NULL  */ ) {
	void *newBlock = GetBlock( sizeof( rvStatDeath ), blockNumOut );
	assert( newBlock );
	rvStatDeath *stat = new( newBlock ) rvStatDeath( t, p, mod );
	allocationsByType[ ST_DEATH ]++;
	return stat;
}

rvStatDamageDealt *rvStatAllocator::AllocStatDamageDealt( int t, int p, int w, int d, int* blockNumOut /* = NULL  */ ) {
	void *newBlock = GetBlock( sizeof( rvStatDamageDealt ), blockNumOut );
	assert( newBlock );
	rvStatDamageDealt *stat = new( newBlock ) rvStatDamageDealt( t, p, w, d );
	allocationsByType[ ST_DAMAGE_DEALT ]++;
	return stat;
}

rvStatDamageTaken *rvStatAllocator::AllocStatDamageTaken( int t, int p, int w, int d, int* blockNumOut /* = NULL  */ ) {
	void *newBlock = GetBlock( sizeof( rvStatDamageTaken ), blockNumOut );
	assert( newBlock );
	rvStatDamageTaken *stat = new( newBlock ) rvStatDamageTaken( t, p, w, d );
	allocationsByType[ ST_DAMAGE_TAKEN ]++;
	return stat;
}

rvStatFlagDrop *rvStatAllocator::AllocStatFlagDrop( int t, int p, int a, int tm, int* blockNumOut /* = NULL  */ ) {
	void *newBlock = GetBlock( sizeof( rvStatFlagDrop ), blockNumOut );
	assert( newBlock );
	rvStatFlagDrop *stat = new( newBlock ) rvStatFlagDrop( t, p, a, tm );
	allocationsByType[ ST_CTF_FLAG_DROP ]++;
	return stat;
}

rvStatFlagReturn *rvStatAllocator::AllocStatFlagReturn( int t, int p, int tm, int* blockNumOut /* = NULL  */ ) {
	void *newBlock = GetBlock( sizeof( rvStatFlagReturn ), blockNumOut );
	assert( newBlock );
	rvStatFlagReturn *stat = new( newBlock ) rvStatFlagReturn( t, p, tm );
	allocationsByType[ ST_CTF_FLAG_CAPTURE ]++;
	return stat;
}

rvStatFlagCapture *rvStatAllocator::AllocStatFlagCapture( int t, int p, int f, int tm, int* blockNumOut /* = NULL  */ ) {
	void *newBlock = GetBlock( sizeof( rvStatFlagCapture ), blockNumOut );
	assert( newBlock );
	rvStatFlagCapture *stat = new( newBlock ) rvStatFlagCapture( t, p, f, tm );
	allocationsByType[ ST_CTF_FLAG_RETURN ]++;
	return stat;
}

#if defined(_INLINEDEBUGMEMORY)
// Because we need inplace new.
#undef new
#define new	ID_DEBUG_NEW
#endif // _INLINEDEBUGMEMORY

/*
===============================================================================

	rvStatManager

	Stores game statistic events in statQueue

===============================================================================
*/

// shouchard:  stat manager start with 1 meg; we'll tune as we get better data
rvStatManager::rvStatManager() {
	memset( localInGameAwards, 0, sizeof( int ) * (int)IGA_NUM_AWARDS );
	inGameAwardHudTime = 0;
}

void rvStatManager::Init( void ) {
	Shutdown();
	statQueue.Clear();
	awardQueue.Clear();
	statQueue.SetGranularity( 1024 );
	endGameSetup = false;
	cmdSystem->AddCommand( "ShowInGameStats", showStats_f, CMD_FL_SYSTEM, "show in game stats." );
	memset( localInGameAwards, 0, sizeof( int ) * (int)IGA_NUM_AWARDS );
	inGameAwardHudTime = 0;
}

void rvStatManager::Shutdown( void ) {
	statAllocator.Report();
	statAllocator.Reset();
	statQueue.Clear();
	awardQueue.Clear();

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		playerStats[ i ] = rvPlayerStat();
	}

	for ( int q = 0; q < MAX_CLIENTS; ++q ) {
		comboKillState[ q ] = CKS_NONE;
		lastRailShot[ q ] = -2;
		lastRailShotHits[ q ] = 0;
	}
}

void rvStatManager::BeginGame( void ) {
	int blockNum;
	rvStatBeginGame* stat = statAllocator.AllocStatBeginGame( gameLocal.time, &blockNum );
	statQueue.Append( rvPair<rvStat*, int>( (rvStat*)(stat), blockNum ) );
	endGameSetup = false;
#if ID_TRAFFICSTATS
	startTime = gameLocal.time;
	networkSystem->GetTrafficStats( startSent, startPacketsSent, startReceived, startPacketsReceived );
#endif

	for ( int q = 0; q < MAX_CLIENTS; ++q ) {
		comboKillState[ q ] = CKS_NONE;
		lastRailShot[ q ] = -2;
		lastRailShotHits[ q ] = 0;
	}
}

void rvStatManager::EndGame( void ) {
	int blockNum;
	rvStatEndGame* stat = statAllocator.AllocStatEndGame( gameLocal.time, &blockNum );
	statQueue.Append( rvPair<rvStat*, int>( (rvStat*)(stat), blockNum ) );
	CalculateEndGameStats();
	awardQueue.Clear();
#if ID_TRAFFICSTATS
	int sent, packetsSent, received, packetsReceived, time;
	networkSystem->GetTrafficStats( sent, packetsSent, received, packetsReceived );
	sent -= startSent;
	packetsSent -= startPacketsSent;
	received -= startReceived;
	packetsReceived -= startPacketsReceived;
	time = gameLocal.time - startTime;
	common->Printf( "EndGame. bytes sent: %d packets sent: %d bytes received: %d packets received: %d\n", sent, packetsSent, received, packetsReceived );
	// compute averages, including packet overhead
	// adjust the UDP overhead, may depend on your TCP stack implementation ( 42 comes from ethereal analysis of the traffic )
	sent += packetsSent * 42;
	received += packetsReceived * 42;
	float sentBps, recvBps;
	sentBps = (float)( sent ) * 1000.0f / time;
	recvBps = (float)( received ) * 1000.0f / time;
	common->Printf( "avg sent %g B/s, received %g B/s\n", sentBps, recvBps );
#endif
}

void rvStatManager::ClientConnect( int clientNum ) {
	// push a client connected event into the queue so we don't get confused with old
	// events detailing the previous owner of this clientNum
	int blockNum;
	rvStatClientConnect* stat = statAllocator.AllocStatClientConnect( gameLocal.time, clientNum, &blockNum );
	statQueue.Append( rvPair<rvStat*,int>( (rvStat*)stat, blockNum ) );
}

void rvStatManager::ClientDisconnect( int clientNum ) {
	// re-init player stats
	playerStats[ clientNum ] = rvPlayerStat();
}

void rvStatManager::Kill( const idPlayer* victim, const idEntity* killer, int methodOfDeath ) {
	int deathBlock, killBlock;
	rvStatDeath* statDeath = statAllocator.AllocStatDeath( gameLocal.time, victim->entityNumber, methodOfDeath, &deathBlock );
	
	statQueue.Append( rvPair<rvStat*, int>( (rvStat*)statDeath, deathBlock ) );
	
	if( killer && killer->IsType( idPlayer::GetClassType() ) ) {
		rvStatKill* statKill = statAllocator.AllocStatKill( gameLocal.time, killer->entityNumber, victim->entityNumber, victim->IsGibbed(), methodOfDeath, &killBlock );
		statQueue.Append( rvPair<rvStat*, int>( (rvStat*)statKill, killBlock ) );
	} else if( !killer ) {
		// basically a suicide
		rvStatKill* statKill = statAllocator.AllocStatKill( gameLocal.time, victim->entityNumber, victim->entityNumber, victim->IsGibbed(), methodOfDeath, &killBlock );
		statQueue.Append( rvPair<rvStat*, int>( (rvStat*)statKill, killBlock ) );
	}
}

void rvStatManager::FlagCaptured( const idPlayer* player, int flagTeam ) {
	int blockNum;
	rvStatFlagCapture* stat = statAllocator.AllocStatFlagCapture( gameLocal.time, player->entityNumber, flagTeam, player->team, &blockNum );
	statQueue.Append( rvPair<rvStat*, int>( (rvStat*)(stat), blockNum ) );
}

void rvStatManager::WeaponFired( const idPlayer* player, int weapon, int num ) {
	playerStats[ player->entityNumber ].weaponShots[ weapon ] += num;
	lastRailShotHits[ player->entityNumber ] = 0;
	
	comboKillState_t cks = comboKillState[ player->entityNumber ];
	comboKillState[ player->entityNumber ] = CKS_NONE;
	if ( player->GetWeaponIndex( "weapon_rocketlauncher" ) == weapon ) {
		if ( cks == CKS_NONE ) {
			comboKillState[ player->entityNumber ] = CKS_ROCKET_FIRED;
		}
	} else if ( player->GetWeaponIndex( "weapon_railgun" ) == weapon ) {
		// apparently it processes hits before it does the fire....
		if ( cks == CKS_RAIL_HIT ) {
			comboKillState[ player->entityNumber ] = CKS_RAIL_FIRED;
		}
	}
	
}

void rvStatManager::WeaponHit( const idActor* attacker, const idEntity* victim, int weapon, bool countForAccuracy ) {
	if( victim && attacker && ( victim == attacker || gameLocal.IsTeamGame( ) && victim->IsType( idPlayer::GetClassType( ) ) && attacker->IsType( idPlayer::GetClassType( ) )
		&& static_cast<const idPlayer*>( victim )->team == static_cast<const idPlayer*>( attacker )->team ) ) {
		return;
	}

	if( victim && victim != attacker ) {
		if( attacker->IsType( idPlayer::GetClassType() ) ) {
			// if attacker was a player, track hit and damage dealt
			int hitBlock;
			rvStatHit* statHit = statAllocator.AllocStatHit( gameLocal.time, attacker->entityNumber, victim->entityNumber, weapon, countForAccuracy, &hitBlock );

			statQueue.Append( rvPair<rvStat*, int>( (rvStat*)(statHit), hitBlock ) );
		}
	}
}

//asalmon modified to work for single player stats on Xenon
void rvStatManager::Damage( const idEntity* attacker, const idEntity* victim, int weapon, int damage ) {
	if( victim && attacker && ( victim == attacker || gameLocal.IsTeamGame( ) && victim->IsType( idPlayer::GetClassType( ) ) && attacker->IsType( idPlayer::GetClassType( ) )
		&& static_cast<const idPlayer*>( victim )->team == static_cast<const idPlayer*>( attacker )->team ) ) {
		return;
	}
	
	if(attacker)
	{
		if( attacker->IsType( idPlayer::GetClassType() ) ) {
			int damageBlock;
			rvStatDamageDealt* statDamage = statAllocator.AllocStatDamageDealt( gameLocal.time, attacker->entityNumber, weapon, damage, &damageBlock );
			statQueue.Append( rvPair<rvStat*, int>( (rvStat*)(statDamage), damageBlock ) );
		}
	}

	if(victim)
	{
		if( victim->IsType( idPlayer::GetClassType() ) ) {
			// if victim was a player, track damage taken
			int blockNum;
			rvStatDamageTaken* stat = statAllocator.AllocStatDamageTaken( gameLocal.time, victim->entityNumber, weapon, damage, &blockNum );
			statQueue.Append( rvPair<rvStat*, int>( (rvStat*)(stat), blockNum ) );
		}
	}
}

void rvStatManager::FlagDropped( const idPlayer* player, const idEntity* attacker ) {
	int blockNum;
	rvStatFlagDrop* stat = statAllocator.AllocStatFlagDrop( gameLocal.time, player->entityNumber, attacker->entityNumber, player->team, &blockNum );
	statQueue.Append( rvPair<rvStat*, int>( (rvStat*)(stat), blockNum ) );
}
void rvStatManager::FlagReturned( const idPlayer* player ) {
	int blockNum;
	rvStatFlagReturn* stat = statAllocator.AllocStatFlagReturn( gameLocal.time, player->entityNumber, player->team, &blockNum );
	statQueue.Append( rvPair<rvStat*, int>( (rvStat*)(stat), blockNum ) );
}

void rvStatManager::DebugPrint( void ) {
	if( !gameLocal.isMultiplayer ) {
		return;
	}
	//gameLocal.Printf( "Begin statistics debug dump:\n" );

	//gameLocal.Printf( "Statistics queue:\n" );
	for( int i = 0; i < Min( statQueue.Num(), 50 ); i++ ) {
		gameLocal.Printf( "{%d, %d} ", statQueue[i].First()->GetType(), statQueue[i].First()->GetTimeStamp() );
	}
	gameLocal.Printf("\n");

	//gameLocal.Printf( "In-game statistics\n\n" );
	//for( int i = 0; i < inGameStats.Num(); i++ ) {
	//	gameLocal.Printf( "\t%d - %s:\n", i, statIndex->index[ i ].GetName().c_str() );
	//	gameLocal.Printf( "\t\tKills: %d\n", inGameStats[i].kills );
	//	gameLocal.Printf( "\t\tDeaths: %d\n", inGameStats[i].deaths );
	//	gameLocal.Printf( "\t\tFlag Caps: %d\n", inGameStats[i].flagCaptures );

	//	for( int j = 0; j < MAX_WEAPONS; j++ ) {
	//		gameLocal.Printf( "\t\tWeapon %d hits: %d\n\t\tWeapon %d shots: %d\n", j, inGameStats[i].weaponHits[j], j, inGameStats[i].weaponShots[j] );
	//	}
	//}
}

int rvStatManager::FreeEvents( int blockNum ) {
	int blockStart = -1;
	int blockEnd = -1;
	
	for( int i = 0; i < statQueue.Num(); i++ ) {
		if( blockStart == -1 && statQueue[ i ].Second() == blockNum ) {
			blockStart = i;
		} else if( blockStart != -1 && statQueue[ i ].Second() != blockNum ) {
			blockEnd = i;
			break;
		}
	}

	if( blockStart == -1 || blockEnd == -1 ) {
// rjohnson: commented out warning - I take it this is not a bad message?
//		gameLocal.Warning( "rvStatManager::FreeEvents() - Could not find events with block num '%d'\n", blockNum );
		return 0;
	}
	
	statQueue.RemoveRange( blockStart, blockEnd - 1 );

	return (blockEnd - blockStart);
}

void rvStatManager::SendInGameAward( inGameAward_t award, int clientNum ) {
	assert( gameLocal.isServer );

	idBitMsg msg;
	byte msgBuf[1024];
	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.WriteByte( GAME_RELIABLE_MESSAGE_INGAMEAWARD );

	msg.WriteByte( award );
	msg.WriteByte( clientNum );

	networkSystem->ServerSendReliableMessage( -1, msg );

	if( gameLocal.isListenServer ) {
		msg.ReadByte();
		ReceiveInGameAward( msg );
	}
}


void rvStatManager::ReceiveInGameAward( const idBitMsg& msg ) {
	assert( gameLocal.isClient || gameLocal.isListenServer );
	int numAwards = 0;

	inGameAward_t award = (inGameAward_t)msg.ReadByte();
	int client = msg.ReadByte();

	// display award on hud		
	idPlayer* player = gameLocal.GetLocalPlayer();
	idPlayer* remote = gameLocal.GetClientByNum(client);
	bool justSound = false;
	if ( client != gameLocal.localClientNum ) {
		justSound = true;
	}

	if ( ( gameLocal.time - inGameAwardHudTime ) < 3000 || awardQueue.Num() > 0 ) {
		if ( gameLocal.GetDemoFollowClient() == client || ( player != NULL && remote != NULL && player->GetInstance() == remote->GetInstance() ) ) {
			rvPair<int,bool> awardPair(award, justSound);
			awardQueue.StackAdd(awardPair);
			return;
		}
	}
	
	if( client == gameLocal.localClientNum ) {
		// don't count awards during warmup

		if( !player || (gameLocal.mpGame.GetGameState()->GetMPGameState() != WARMUP && 
			(gameLocal.gameType != GAME_TOURNEY || ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArena( player->GetArena() ).GetState() != AS_WARMUP )) ) {
			localInGameAwards[ award ]++;
			numAwards = localInGameAwards[ award ];
		} else {
			numAwards = 1;
		}

		if( player && player->mphud ) {
			player->mphud->HandleNamedEvent( "clearIGA" );
			player->mphud->SetStateInt( "ig_awards", idMath::ClampInt( 0, 10, numAwards ) );
			player->mphud->SetStateString( "ig_award", va( "gfx/mp/awards/%s", inGameAwardInfo[ award ].name ) );	
			if( numAwards < 10 ) {
				player->mphud->SetStateString( "ig_award_num", "");
				for( int i = 0; i < idMath::ClampInt( 0, 10, numAwards ); i++ )  {
					player->mphud->SetStateInt( va( "ig_awards_%d", i + 1 ), 1 );
				}
			} else {
				player->mphud->SetStateInt( "ig_award_num", numAwards );
				player->mphud->SetStateInt( "ig_awards", 1  );
				player->mphud->SetStateInt( va( "ig_awards_%d", 1 ), 1 );
			}
			//inGameAwardHudTime = gameLocal.time;
			player->mphud->HandleNamedEvent( "giveIGA" );
			player->mphud->StateChanged( gameLocal.time );

		}

		if ( player ) {
			player->StartSound( va( "snd_award_%s", inGameAwardInfo[ award ].name ), SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
	else if ( player && remote && ( player->GetInstance() == remote->GetInstance() ) && (award == IGA_HOLY_SHIT || award == IGA_CAPTURE || award == IGA_HUMILIATION )) {
			player->StartSound( va( "snd_award_%s", inGameAwardInfo[ award ].name ), SND_CHANNEL_ANY, 0, false, NULL );
	}
	inGameAwardHudTime = gameLocal.time;

	idPlayer* awardee = gameLocal.GetClientByNum( client );
	if ( awardee ) {
		if ( player && player->GetInstance() == awardee->GetInstance() ) {
			iconManager->AddIcon( client, va( "mtr_award_%s", inGameAwardInfo[ award ].name ) );
		}
	}
}

void rvStatManager::CheckAwardQueue() {

	if(((gameLocal.time - inGameAwardHudTime) < 3000 || awardQueue.Num() == 0))
	{
		return;
	}

	idPlayer* player = gameLocal.GetLocalPlayer();

	int award = awardQueue.StackTop().First();
	bool justSound = awardQueue.StackTop().Second();
	awardQueue.StackPop();

	if ( player ) {
			if(!justSound || award == IGA_HOLY_SHIT || award == IGA_CAPTURE)
			{
				player->StartSound( va( "snd_award_%s", inGameAwardInfo[ award ].name ), SND_CHANNEL_ANY, 0, false, NULL );
			}
	}

	if( player && player->mphud && !justSound) {

		player->mphud->HandleNamedEvent( "clearIGA" );
		localInGameAwards[ award ]++;
		player->mphud->SetStateInt( "ig_awards", idMath::ClampInt( 0, 10, localInGameAwards[ award ] ) );
		player->mphud->SetStateString( "ig_award", va( "gfx/mp/awards/%s", inGameAwardInfo[ award ].name ) );	
		if(localInGameAwards[ award ] < 10) {
			player->mphud->SetStateString( "ig_award_num", "");
			for( int i = 0; i < idMath::ClampInt( 0, 10, localInGameAwards[ award ] ); i++ )  {
				player->mphud->SetStateInt( va( "ig_awards_%d", i + 1 ), 1 );
			}
			
		}
		else {
			player->mphud->SetStateInt( "ig_award_num", localInGameAwards[ award ]);
			player->mphud->SetStateInt( "ig_awards", 1  );
			player->mphud->SetStateInt( va( "ig_awards_%d", 1 ), 1 );
		}
		inGameAwardHudTime = gameLocal.time;
		player->mphud->HandleNamedEvent( "giveIGA" );
		player->mphud->StateChanged( gameLocal.time );

	}

}


void rvStatManager::GivePlayerCashForAward( idPlayer* player, inGameAward_t award )
{
	if( !player )
		return;
	
	if( !gameLocal.isMultiplayer )
		return;
	
	if( !gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() )
		return;

	mpGameState_t mpGameState = gameLocal.mpGame.GetGameState()->GetMPGameState();
	if( mpGameState != GAMEON && mpGameState != SUDDENDEATH )
		return;

	const char* awardCashValueName = NULL;
	switch( award )
	{
		case IGA_CAPTURE:		awardCashValueName = "playerCashAward_mpAward_capture";			break;
		case IGA_HUMILIATION:	awardCashValueName = "playerCashAward_mpAward_humiliation";		break;
		case IGA_IMPRESSIVE:	awardCashValueName = "playerCashAward_mpAward_impressive";		break;
		case IGA_EXCELLENT:		awardCashValueName = "playerCashAward_mpAward_excellent";		break;
		case IGA_ASSIST:		awardCashValueName = "playerCashAward_mpAward_assist";			break;
		case IGA_DEFENSE:		awardCashValueName = "playerCashAward_mpAward_defense";			break;
		case IGA_COMBO_KILL:	awardCashValueName = "playerCashAward_mpAward_combo_kill";		break;
		case IGA_RAMPAGE:		awardCashValueName = "playerCashAward_mpAward_rampage";			break;
		case IGA_HOLY_SHIT:		awardCashValueName = "playerCashAward_mpAward_holy_shit";		break;
	}

	if( awardCashValueName )
	{
		player->GiveCash( (float) gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( awardCashValueName, 0 ) );
	}
}


void rvStatManager::GiveInGameAward( inGameAward_t award, int clientNum ) {
	idPlayer* player = (idPlayer*)gameLocal.entities[ clientNum ];

	if( gameLocal.isMultiplayer ) {
		// show in-game awards during warmup, but don't actually let players accumulate them
		if( !player || (gameLocal.mpGame.GetGameState()->GetMPGameState() != WARMUP && 
			(gameLocal.gameType != GAME_TOURNEY || ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArena( player->GetArena() ).GetState() != AS_WARMUP )) ) {
			playerStats[ clientNum ].inGameAwards[ award ]++;
			GivePlayerCashForAward( player, award );
		}
		SendInGameAward( award, clientNum );	
	}
}

void rvStatManager::SetupStatWindow( idUserInterface* statHud ) {
	statWindow.SetupStatWindow( statHud );
}

void rvStatManager::SelectStatWindow( int selectionIndex, int selectionTeam ) {
	statWindow.SelectPlayer( statWindow.ClientNumFromSelection( selectionIndex, selectionTeam ) );
}

int rvStatManager::GetSelectedClientNum( int* selectionIndexOut, int* selectionTeamOut ) {
	return statWindow.GetSelectedClientNum( selectionIndexOut, selectionTeamOut );
}

void rvStatManager::UpdateInGameHud( idUserInterface* statHud, bool visible ) {
	idPlayer* player = NULL;

	if( gameLocal.GetLocalPlayer() ) {
		player = gameLocal.GetLocalPlayer();
		player->GetHud()->SetStateInt( "stat_visible", visible? 1 : 0);
	}

	if( !visible ) {
		statHud->SetStateInt( "stat_visible", 0 );
		return;
	} else {
		statHud->SetStateInt( "stat_visible", 1 );
	}

	if( player ) {
		statWindow.SetupStatWindow( statHud, player->spectating );
	}
}

void rvStatManager::SendStat( int toClient, int statClient ) {
	if( statClient < 0 || statClient >= MAX_CLIENTS ) {
		gameLocal.Warning( "rvStatManager::SendStat() - Stats requested for invalid client num '%d'\n", statClient );
		return;
	}

	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_STAT );
	outMsg.WriteByte( statClient );
	
	playerStats[ statClient ].PackStats( outMsg );

	networkSystem->ServerSendReliableMessage( toClient, outMsg );
}

void rvStatManager::ReceiveStat( const idBitMsg& msg ) {
	//asalmon: added because this is used to restore single player stats from saves on Xbox 360
	if(gameLocal.IsMultiplayer())
	{
		assert( gameLocal.isClient );
	}
	
	int client = msg.ReadByte();

	playerStats[ client ].UnpackStats( msg );
	playerStats[ client ].lastUpdateTime = gameLocal.time;

	// display the updated stat
	if(gameLocal.IsMultiplayer())
	{
		statWindow.SelectPlayer( client );
	}
}

void rvStatManager::SendAllStats( int clientNum, bool full ) {
	
	assert( gameLocal.isServer );

	idBitMsg	outMsg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_ALL_STATS );

	assert( MAX_CLIENTS <= 32 );

	unsigned	sentClients = 0;
	for(int i=0; i < MAX_CLIENTS; i++)
	{
		if ( gameLocal.entities[ i ] ) {
			sentClients |= 1 << i;
		}
	}

	outMsg.WriteBits( sentClients, MAX_CLIENTS );

	for(int i=0; i < MAX_CLIENTS; i++)
	{
		if ( sentClients & ( 1 << i ) ) {
			playerStats[ i ].PackStats( outMsg );
		}
	}

	networkSystem->ServerSendReliableMessage( clientNum, outMsg );

	//common->Printf("SENT ALL STATS %i\n", Sys_Milliseconds());
	
}


void rvStatManager::ReceiveAllStats( const idBitMsg& msg ) {
	assert( gameLocal.isClient );

	assert( MAX_CLIENTS <= 32 );

	unsigned	sentClients = msg.ReadBits( MAX_CLIENTS );

	for(int i=0; i < MAX_CLIENTS; i++)
	{
		if ( sentClients & ( 1 << i ) ) {
			playerStats[ i ].UnpackStats( msg );
		} else {
			playerStats[ i ].Clear();
		}

		playerStats[ i ].lastUpdateTime = gameLocal.time;
	}
	if ( gameLocal.mpGame.GetGameState()->GetMPGameState() == GAMEREVIEW ) {
		gameLocal.mpGame.ShowStatSummary();
	}
}


void rvStatManager::ClearStats( void ) {
	// clear connected stats
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		playerStats[ i ] = rvPlayerStat();
	}

	for ( int q = 0; q < MAX_CLIENTS; ++q ) {
		lastRailShot[ q ] = -2;
		lastRailShotHits[ q ] = 0;
	}
#ifdef _XENON
	lastFullUpdate = -50000;
#endif
}

void rvStatManager::CalculateEndGameStats( void ) {
	int maxKills = idMath::INT_MIN;
	int maxKillPlayer = -1;

	int maxSuicides = idMath::INT_MIN;
	int maxSuicidesPlayer = -1;

	int maxGauntletKills = idMath::INT_MIN;
	int maxGauntletKillsPlayer = -1;

	float maxDamageKillsRatio = 0.0;
	int maxDamageKillsRatioPlayer = -1;

//Dump the stats to a log file
	idFile *log = NULL;
	if(cvarSystem->GetCVarBool("com_logMPStats"))
	{
		log = fileSystem->OpenFileAppend("StatisticsLog.txt");
	}
	idStr toFile;
	if ( !log ) {
//		common->Warning("Statistics log will not be written\n");
	}
	else
	{
		struct tm *newtime;
		time_t aclock;
		time( &aclock );
		newtime = localtime( &aclock );
		toFile = va("Match on map %s played on %s\n", gameLocal.GetMapName(), asctime( newtime ));
		log->Write(toFile.c_str(), toFile.Length());
	}
	
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if( !gameLocal.entities[ i ] ) {
			continue;
		}

		if(log)
		{
			toFile = va("Statistics for %s\n",  gameLocal.userInfo[ i ].GetString("ui_name"));
			log->Write(toFile.c_str(), toFile.Length());
			toFile = va("Kills: %i Deaths: %i Suicides: %i\n",  playerStats[ i ].kills, playerStats[ i ].deaths, playerStats[ i ].suicides);
			log->Write(toFile.c_str(), toFile.Length());
		}

		gameLocal.Printf( "Calculating stats for client %d (%s)\n", i, gameLocal.userInfo[ i ].GetString("ui_name") );
		// overall accuracy award and sniper accuracy award
		idPlayer* player = (idPlayer*)gameLocal.entities[ i ];
		int railgunIndex = player->GetWeaponIndex( "weapon_railgun" );
		int rocketIndex = player->GetWeaponIndex( "weapon_rocketlauncher" );

		float accuracyAverage = 0.0f;
		int numAccuracies = 0;
		for( int j = 0; j < MAX_WEAPONS; j++ ) {
			if( playerStats[ i ].weaponShots[ j ] == 0 ) {
				if(log)
				{
					if(player->GetWeaponDef(j))
					{
						toFile = va("%s not used\n", common->GetLocalizedString(player->GetWeaponDef(j)->dict.GetString("inv_name")));
						log->Write(toFile.c_str(), toFile.Length()); 
					}
				}
				continue;
			}

			float weaponAccuracy = (float)playerStats[ i ].weaponHits[ j ] / (float)playerStats[ i ].weaponShots[ j ];
			if(log)
			{
				if(player->GetWeaponDef(j))
				{
					toFile = va("%s: %i%%\n", common->GetLocalizedString(player->GetWeaponDef(j)->dict.GetString("inv_name")), (int)(((float)playerStats[ i ].weaponHits[ j ] / (float)playerStats[ i ].weaponShots[ j ]) * 100.0f));
					log->Write(toFile.c_str(), toFile.Length()); 
				}
			}
			if( j == railgunIndex ) {
				// sniper award
				if( weaponAccuracy >= 0.9f && playerStats[ i ].weaponShots[ railgunIndex ] >= 10 ) {
					playerStats[ i ].endGameAwards.Append( EGA_SNIPER );
				}				
			}

			accuracyAverage += weaponAccuracy;
			numAccuracies++;
		}


		if ( numAccuracies && ( accuracyAverage / (float)numAccuracies >= 0.5f ) ) {
			playerStats[ i ].endGameAwards.Append( EGA_ACCURACY );
		}


		// rail master award
		if ( playerStats[ i ].kills && ( (float)playerStats[ i ].weaponKills[ railgunIndex ] / (float)playerStats[ i ].kills >= 0.8f ) ) {
			playerStats[ i ].endGameAwards.Append( EGA_RAIL_MASTER );
		}

		// rocket sauce award
		if ( playerStats[ i ].kills && ( (float)playerStats[ i ].weaponKills[ rocketIndex ] / (float)playerStats[ i ].kills >= 0.8f ) ) {
			playerStats[ i ].endGameAwards.Append( EGA_ROCKET_SAUCE );
		}

		// critical failure award
		if( playerStats[ i ].kills == 0 ) {
			playerStats[ i ].endGameAwards.Append( EGA_CRITICAL_FAILURE );
		}

		// frags award
//asalmon: Made the limit more reasonable for the shorter time limits and less players of Xenon
#ifdef _XENON
		if( playerStats[ i ].kills >= 50 ) {
			playerStats[ i ].endGameAwards.Append( EGA_FRAGS );
		}
#else
		if( playerStats[ i ].kills >= 100 ) {
			playerStats[ i ].endGameAwards.Append( EGA_FRAGS );
		}
#endif


		if( playerStats[ i ].kills > maxKills ) {
			maxKills = playerStats[ i ].kills;
			maxKillPlayer = i;
		}

		if( playerStats[ i ].suicides > maxSuicides ) {
			maxSuicides = playerStats[ i ].suicides;
			maxSuicidesPlayer = i;
		}

		if( playerStats[ i ].weaponKills[ player->GetWeaponIndex( "weapon_gauntlet" ) ] > maxGauntletKills ) {
			maxGauntletKills = playerStats[ i ].weaponKills[ player->GetWeaponIndex( "weapon_gauntlet" ) ];
			maxGauntletKillsPlayer = i;
		}

//asalmon: Calculate the damage ratio:
		if(playerStats[i].kills > 0)
		{
			playerStats[i].damageRatio = playerStats[i].damageGiven / playerStats[i].kills;
		}
		else
		{
			playerStats[i].damageRatio = playerStats[i].damageGiven;
		}

		if( playerStats[ i ].damageRatio > maxDamageKillsRatio ) {
			maxDamageKillsRatio = playerStats[ i ].damageRatio;
			maxDamageKillsRatioPlayer = i;
		}
		
		if(log)
		{
			toFile = "\n";
			log->Write(toFile.c_str(), toFile.Length());
		}


//asalmon: hack to test certain achievement awards.
#ifdef _XENON
		if(cvarSystem->GetCVarInteger("si_overrideFrags"))
		{
			common->Printf("Overriding Frags to: %i for player %i\n", cvarSystem->GetCVarInteger("si_overrideFrags"), i);
			playerStats[i].kills = cvarSystem->GetCVarInteger("si_overrideFrags");
		}
#endif

	}



	// Perfect award
	if( maxKillPlayer >= 0 && playerStats[ maxKillPlayer ].deaths == 0 ) {
		idPlayer* player = (idPlayer*)gameLocal.entities[ maxKillPlayer ];
		if( !gameLocal.IsTeamGame() || ( gameLocal.IsTeamGame() && player && gameLocal.mpGame.TeamLeader() == player->team ) ) {
			playerStats[ maxKillPlayer ].endGameAwards.Append( EGA_PERFECT );
		} 
	}

	// Lemming award
	if( maxSuicidesPlayer >= 0 && maxSuicides >= 5 ) {
		playerStats[ maxSuicidesPlayer ].endGameAwards.Append( EGA_LEMMING );
	}

	// Brawler award
	if( maxGauntletKillsPlayer >= 0 && maxGauntletKills >= 3 ) {
		playerStats[ maxGauntletKillsPlayer ].endGameAwards.Append( EGA_BRAWLER );
	}

	// Team player award
	if( maxDamageKillsRatioPlayer >= 0 && maxDamageKillsRatio > 500 ) {
		playerStats[ maxDamageKillsRatioPlayer ].endGameAwards.Append( EGA_TEAM_PLAYER );
	}	
	
	if(log)
	{
		toFile = "\n";
		log->Write(toFile.c_str(), toFile.Length());
		log->Flush();
		fileSystem->CloseFile(log);
	}
}

void rvStatManager::GetAccuracyLeaders( int accuracyLeaders[ MAX_WEAPONS ] ) {
	memset( accuracyLeaders, -1, sizeof( int ) * MAX_WEAPONS );

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		if( gameLocal.entities[ i ] == NULL ) {
			continue;
		}

		rvPlayerStat* playerStats = GetPlayerStat( i );

		for( int j = 0; j < MAX_WEAPONS; j++ ) {
			if( playerStats->weaponShots[ j ] == 0 ) {
				continue;
			}

			float playerAccuracy = (float)playerStats->weaponHits[ j ] / (float)playerStats->weaponShots[ j ];
			float leaderAccuracy = -1.0f;
			if( accuracyLeaders[ j ] != -1 ) {
				rvPlayerStat* leaderStats = GetPlayerStat( accuracyLeaders[ j ] );
				if( leaderStats->weaponShots[ j ] != 0 ) {
					leaderAccuracy = (float)leaderStats->weaponHits[ j ] / (float)leaderStats->weaponShots[ j ];
				}
			}
			if( playerAccuracy > leaderAccuracy ) {
				accuracyLeaders[ j ] = i;
			}
		}
	}
}

int rvStatManager::DamageGiven( int playerNum, int lowerBound, int upperBound ) {
	if( playerNum < 0 || playerNum >= MAX_CLIENTS ) {
		return 0;
	}

	int damage = 0;

	for( int i = 0; i < statQueue.Num(); i++ ) {
		if( statQueue[ i ].First()->GetType() == ST_DAMAGE_DEALT ) {
			if( statQueue[ i ].First()->GetPlayerClientNum() == playerNum && (statQueue[ i ].First()->GetTimeStamp() > lowerBound && statQueue[ i ].First()->GetTimeStamp() < upperBound) ) {
				damage += static_cast<rvStatDamageDealt*>(statQueue[ i ].First())->GetDamage();
			}
		}
	}

	return damage;
}

int rvStatManager::DamageTaken( int playerNum, int lowerBound, int upperBound ) {
	if( playerNum < 0 || playerNum >= MAX_CLIENTS ) {
		return 0;
	}

	int damage = 0;

	for( int i = 0; i < statQueue.Num(); i++ ) {
		if( statQueue[ i ].First()->GetType() == ST_DAMAGE_TAKEN ) {
			if( statQueue[ i ].First()->GetPlayerClientNum() == playerNum && ( statQueue[ i ].First()->GetTimeStamp() > lowerBound && statQueue[ i ].First()->GetTimeStamp() < upperBound ) ) {
				damage += static_cast<rvStatDamageTaken*>(statQueue[ i ].First())->GetDamage();
			}
		}
	}

	return damage;
}

rvStat* rvStatManager::GetLastClientStat( int clientNum, statType_t type, int time ) {
	for( int i = (statQueue.Num() - 1); i >= 0; i-- ) {
		if( statQueue[ i ].First()->GetTimeStamp() < time ) {
			return NULL;
		}

		if( statQueue[ i ].First()->GetType() == type ) {
			if( clientNum == -1 || statQueue[ i ].First()->GetPlayerClientNum() == clientNum ) {
				return statQueue[ i ].First();
			}
		}
	}

	return NULL;
}

void rvStatManager::GetLastClientStats( int clientNum, statType_t type, int time, int num, rvStat** results ) {
	int numFound = 0;

	for( int i = (statQueue.Num() - 1); i >= 0; i-- ) {
		if( statQueue[ i ].First()->GetTimeStamp() < time ) {
			return;
		}

		if( statQueue[ i ].First()->GetType() == type ) {
			if( clientNum == -1 || statQueue[ i ].First()->GetPlayerClientNum() == clientNum ) {
				results[ numFound++ ] = statQueue[ i ].First();
				if( numFound >= num ) {
					return;
				}
			}
		}
	}

	return;
}


rvStatTeam* rvStatManager::GetLastTeamStat( int team, statType_t type, int time ) {
	assert( type > ST_STAT_TEAM );

	for( int i = (statQueue.Num() - 1); i >= 0; i-- ) {
		if( statQueue[ i ].First()->GetTimeStamp() < time ) {
			return NULL;
		}

		if( statQueue[ i ].First()->GetType() == type ) {
			if( ((rvStatTeam*)statQueue[ i ].First())->GetTeam() == team ) {
				return (rvStatTeam*)statQueue[ i ].First();
			}
		}
	}

	return NULL;
}

void rvStatManager::SetupEndGameHud( idUserInterface* statHud ) {
	if( gameLocal.IsFlagGameType() ) {
		statHud->SetStateInt( "ctf_awards", 1 );
	} else {
		statHud->SetStateInt( "ctf_awards", 0 );
	}

	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* p = gameLocal.mpGame.GetRankedPlayer( i );

		if( p && gameLocal.mpGame.IsInGame( p->entityNumber ) ) {
			statHud->SetStateString( va( "player%d_name", i + 1 ), va( "%d. %s", i + 1, gameLocal.userInfo[ p->entityNumber ].GetString( "ui_name" ) ) );
			statHud->SetStateString( va( "player%d_score", i + 1 ), va( "%d", gameLocal.mpGame.GetScore( i ) ) );
			statHud->SetStateInt( va( "player%d_visible", i + 1 ), 1 );
			statHud->SetStateInt( va( "player%d_team", i + 1 ), gameLocal.IsTeamGame() ? p->team : 0 );
		} else {
			statHud->SetStateInt( va( "player%d_visible", i + 1 ), 0 );
		}
	}
	statHud->HandleNamedEvent( "Setup" );
}

rvPlayerStat* rvStatManager::GetPlayerStat( int clientNum ) {
	return &playerStats[ clientNum ];
}

void rvStatManager::UpdateEndGameHud( idUserInterface* statHud, int clientNum ) {
	/*if( !endGameSetup ) {
		// no info yet
		SetupEndGameHud( statHud );
		endGameSetup = true;
	}
	
	rvPlayerStat* clientStat = &(playerStats[ clientNum ]);

	statHud->HandleNamedEvent( "clear" );
	statHud->SetStateString( "stat_name", gameLocal.userInfo[ clientNum ].GetString( "ui_name" ) );

	// weapon accuracy
	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		statHud->SetStateString( va( "stat_%d_pct", i ), va( "%d%%", (int)(clientStat->weaponAccuracy[ i ] * 100) ) );
	}

	// in-game awards
	int igAwardCount[ IGA_NUM_AWARDS ];
	memset( igAwardCount, 0, sizeof( int ) * IGA_NUM_AWARDS );
	for( int i = 0; i < clientStat->inGameAwards.Num(); i++ ) {
		igAwardCount[ clientStat->inGameAwards[ i ] ]++;
	}

	for( int i = 0; i < IGA_NUM_AWARDS; i++ ) {
		statHud->SetStateString( inGameAwardInfo[ i ].name, va( "%d", igAwardCount[ i ] ) );
	}
	
	// end-game awards
	for( int i = 0; i < clientStat->endGameAwards.Num(); i++ ) {
		statHud->SetStateInt( va( "eg_award%d", i ), 1 );
		statHud->SetStateString( va( "eg_award%d_text", i ), endGameAwardInfo[ clientStat->endGameAwards[ i ] ].name );
	}

	// kills
	statHud->SetStateString( "stat_frags", va( "%d", clientStat->kills ) );

	// deaths
	statHud->SetStateString( "stat_deaths", va( "%d", clientStat->deaths ) );*/
}

/*
===============================================================================

	rvStatSummary

	Stores one player's summary information.  Transmitted to clients for 
	intermission summary screen.

===============================================================================
*/
rvPlayerStat::rvPlayerStat() {
	Clear();
}

void rvPlayerStat::Clear( void ) {
	memset( weaponShots, 0, sizeof(int) * MAX_WEAPONS );
	memset( weaponHits, 0, sizeof(int) * MAX_WEAPONS );
	memset( weaponKills, 0, sizeof(int) * MAX_WEAPONS );
	memset( inGameAwards, 0, sizeof( int ) * (int)IGA_NUM_AWARDS );

	kills = deaths = suicides = lastUpdateTime = damageTaken = damageGiven = 0;
	damageRatio = 0.0f;
}

void rvPlayerStat::PackStats( idBitMsg& msg ) {
	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		msg.WriteShort( weaponShots[ i ] );
	}

	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		msg.WriteShort( weaponHits[ i ] );
	}


	for( int i = 0; i < IGA_NUM_AWARDS; i++ ) {
		msg.WriteByte( inGameAwards[ i ] );
	}

	msg.WriteByte( endGameAwards.Num() );
	for( int i = 0; i < endGameAwards.Num(); i++ ) {
		msg.WriteByte( endGameAwards[ i ] );
	}

	msg.WriteBits( idMath::ClampInt( 0, MP_PLAYER_MAXDEATHS, deaths ), ASYNC_PLAYER_DEATH_BITS );
	msg.WriteBits( idMath::ClampInt( 0, MP_PLAYER_MAXKILLS, kills ), ASYNC_PLAYER_KILL_BITS );
}

void rvPlayerStat::UnpackStats( const idBitMsg& msg ) {
	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		weaponShots[ i ] = msg.ReadShort();
	}

	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		weaponHits[ i ] = msg.ReadShort();
	}

	for( int i = 0; i < IGA_NUM_AWARDS; i++ ) {
		inGameAwards[ i ] = msg.ReadByte();
	}

	endGameAwards.SetNum( msg.ReadByte() );
	for( int i = 0; i < endGameAwards.Num(); i++ ) {
		endGameAwards[ i ] = (endGameAward_t)msg.ReadByte();
	}

	deaths = msg.ReadBits( ASYNC_PLAYER_DEATH_BITS );
	kills = msg.ReadBits( ASYNC_PLAYER_KILL_BITS );
}
