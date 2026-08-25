//----------------------------------------------------------------
// Buying.h
//
// Copyright 2005 Ritual Entertainment
//
// This file essentially serves as an extension to the Game DLL
// source files Multiplayer.h and Player.h, in an attempt
// to isolate, as much as possible, these changes from the main
// body of code (for merge simplification, etc).
//----------------------------------------------------------------

#ifndef __BUYING_H__
#define __BUYING_H__

#include "../Game_local.h"
#include "../MultiplayerGame.h"


class riBuyingManager
{
private:
	const idDeclEntityDef*	_buyingGameBalanceConstants;
	int						opponentKillCashAward;	// latch
	int						opponentKillFragCount;

public:
	riBuyingManager();
	~riBuyingManager();

	int GetIntValueForKey( const char* keyName, int defaultValue );
	int GetOpponentKillCashAward( void );

	void Reset( void ) { opponentKillFragCount = -1; }
};


#endif // __BUYING_H__
