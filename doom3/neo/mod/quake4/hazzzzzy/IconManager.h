//----------------------------------------------------------------
// IconManager.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __ICONMANAGER_H__
#define __ICONMANAGER_H__

#include "Icon.h"

const int ICON_STAY_TIME = 2000;

class rvIconManager {
public:
	void				AddIcon( int clientNum, const char* iconName );
	void				UpdateIcons( void );
	void				UpdateTeamIcons( void );
	void				UpdateChatIcons( void );
	void				Shutdown( void );

private:
	idList<rvPair<rvIcon*, int> >	icons[ MAX_CLIENTS ];
	rvIcon							teamIcons[ MAX_CLIENTS ];
	rvIcon							chatIcons[ MAX_CLIENTS ];
};

extern rvIconManager*	iconManager;

#endif
