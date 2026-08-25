//----------------------------------------------------------------
// StatWindow.h
//
// Copyright 2002-2005 Raven Software
//----------------------------------------------------------------

#ifndef __STATWINDOW_H__
#define __STATWINDOW_H__

/*
===============================================================================

Stat selection window

===============================================================================
*/

class rvStatWindow {
public:
	rvStatWindow();
	void						SetupStatWindow( idUserInterface* statHud, bool useSpectator = false );
	void						SelectPlayer( int clientNum );
	int							ClientNumFromSelection( int selectionIndex, int selectionTeam );
	void						ClearWindow( void );
	int							GetSelectedClientNum( int* selectionIndexOut, int* selectionTeamOut );
private:
	idList<idPlayer*>			stroggPlayers;
	idList<idPlayer*>			marinePlayers;
	idList<idPlayer*>			players;
	idList<idPlayer*>			spectators;
	
	idUserInterface*			statHud;
};


#endif
