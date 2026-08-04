/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "Window.h"
#include "Winvar.h"
#include "UserInterfaceLocal.h"

idWinVar::idWinVar() { 
	guiDict = NULL; 
	name = NULL; 
	eval = true;
}

idWinVar::~idWinVar() { 
	delete name;
	name = NULL;
}

void idWinVar::SetGuiInfo(idDict *gd, const char *_name) { 
	guiDict = gd; 
	SetName(_name); 
}


void idWinVar::Init(const char *_name, idWindow *win) {
	idStr key = _name;
	guiDict = NULL;
	int len = key.Length();
	if (len > 5 && key[0] == 'g' && key[1] == 'u' && key[2] == 'i' && key[3] == ':') {
		key = key.Right(len - VAR_GUIPREFIX_LEN);
		SetGuiInfo( win->GetGui()->GetStateDict(), key );
		win->AddUpdateVar(this);
	} else {
		//stgatilov: this is needed because
		//string literals and variables look same during parsing
		//whether it is a variable becomes known later
		if (dynamic_cast<idWinStr*>(this))
			Set(_name);
	}
}

void idMultiWinVar::Set( const char *val ) {
	for ( int i = 0; i < Num(); i++ ) {
		(*this)[i]->Set( val );
	}
}

void idMultiWinVar::Update( void ) {
	for ( int i = 0; i < Num(); i++ ) {
		(*this)[i]->Update();
	}
}

void idMultiWinVar::SetGuiInfo( idDict *dict ) {
	for ( int i = 0; i < Num(); i++ ) {
		(*this)[i]->SetGuiInfo( dict, (*this)[i]->c_str() );
	}
}

