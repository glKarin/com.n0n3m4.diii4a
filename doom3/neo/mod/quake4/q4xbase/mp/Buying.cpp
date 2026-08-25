//----------------------------------------------------------------
// Buying.cpp
//
// Copyright 2005 Ritual Entertainment
//
// This file essentially serves as an extension to the Game DLL
// source files Multiplayer.cpp and Player.cpp, in an attempt
// to isolate, as much as possible, these changes from the main
// body of code (for merge simplification, etc).
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "Buying.h"


riBuyingManager::riBuyingManager() :
	_buyingGameBalanceConstants( NULL ),
	opponentKillCashAward( 0 ),
	opponentKillFragCount( -1 ) { }

riBuyingManager::~riBuyingManager() { }

int riBuyingManager::GetIntValueForKey( const char* keyName, int defaultValue ) {
	if( !keyName )
	{
		return defaultValue;
	}

	if( !_buyingGameBalanceConstants )
	{
		_buyingGameBalanceConstants = static_cast<const idDeclEntityDef*>( declManager->FindType( DECL_ENTITYDEF, "BuyingGameBalanceConstants", false ) );

		if( !_buyingGameBalanceConstants )
		{
			return defaultValue;
		}
	}

	for( int i = 0; i < _buyingGameBalanceConstants->dict.GetNumKeyVals(); i++ )
	{
		const idKeyValue* keyValuePair = _buyingGameBalanceConstants->dict.GetKeyVal( i );
		if( !keyValuePair->GetKey().Icmp( keyName ) )
		{
			return atoi( keyValuePair->GetValue() );
		}
	}

	return defaultValue;
}

int riBuyingManager::GetOpponentKillCashAward( void ) {
	int targetFragCount = gameLocal.serverInfo.GetInt( "si_fragLimit" );
	if ( opponentKillFragCount != targetFragCount ) {
		opponentKillFragCount = targetFragCount;
		if ( idStr::Icmp( gameLocal.serverInfo.GetString( "si_gameType" ), "DM" ) && idStr::Icmp( gameLocal.serverInfo.GetString( "si_gameType" ), "Team DM" ) ) {
			// only do frag reward scaling in DM/TDM
			opponentKillCashAward = GetIntValueForKey( "playerCashAward_killingOpponent", 600 );
		} else {
			targetFragCount = idMath::ClampInt( GetIntValueForKey( "killingOpponent_minFragAdjust", 10 ), GetIntValueForKey( "killingOpponent_maxFragAdjust",50 ), targetFragCount );
			int baseVal = GetIntValueForKey( "playerCashAward_killingOpponent", 600 );
			int fragTarget = GetIntValueForKey( "killingOpponent_bestFragCount", 25 );
			opponentKillCashAward = ( baseVal * fragTarget ) / targetFragCount;
		}
	}
	return opponentKillCashAward;
}
