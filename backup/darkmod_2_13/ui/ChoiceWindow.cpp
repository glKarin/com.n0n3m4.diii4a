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



#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "ChoiceWindow.h"
#include "StdString.h"

/*
============
idChoiceWindow::InitVars
============
*/
void idChoiceWindow::InitVars( ) {
	if ( cvarStr.Length() ) {
		cvar = cvarSystem->Find( cvarStr );
		if ( !cvar ) {
			common->Warning( "idChoiceWindow::InitVars: gui '%s' window '%s' references undefined cvar '%s'", gui->GetSourceFile(), name.c_str(), cvarStr.c_str() );
			return;
		}
		updateStr.Append( &cvarStr );
	}
	if ( guiStr.Length() ) {
		updateStr.Append( &guiStr );
	}
	updateStr.SetGuiInfo( gui->GetStateDict() );
	updateStr.Update();
}

/*
============
idChoiceWindow::CommonInit
============
*/
void idChoiceWindow::CommonInit() {
	currentChoice = 0;
	choiceType = 0;
	cvar = NULL;
	liveUpdate = true;
	choices.Clear();
}

idChoiceWindow::idChoiceWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g) {
	dc = d;
	gui = g;
	CommonInit();
}

idChoiceWindow::idChoiceWindow(idUserInterfaceLocal *g) : idWindow(g) {
	gui = g;
	CommonInit();
}

idChoiceWindow::~idChoiceWindow() {

}

void idChoiceWindow::RunNamedEvent( const char* eventName ) {
	idStr event, group;
	
	if ( !idStr::Cmpn( eventName, "cvar read ", 10 ) ) {
		event = eventName;
		group = event.Mid( 10, event.Length() - 10 );
		if ( !group.Cmp( updateGroup ) ) {
			UpdateVars( true, true );
		}
	} else if ( !idStr::Cmpn( eventName, "cvar write ", 11 ) ) {
		event = eventName;
		group = event.Mid( 11, event.Length() - 11 );
		if ( !group.Cmp( updateGroup ) ) {
			UpdateVars( false, true );
		}
	}
}

void idChoiceWindow::UpdateVars( bool read, bool force ) {
	if ( force || liveUpdate ) {
		if ( cvar && cvarStr.NeedsUpdate() ) {
			if ( read ) {
				cvarStr.Set( cvar->GetString() );
			} else {
				cvar->SetString( cvarStr.c_str() );
			}	
		}
		if ( !read && guiStr.NeedsUpdate() ) {
			guiStr.Set( va( "%i", currentChoice ) );
		}
	}
}

const char *idChoiceWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {
	int key;
	bool runAction = false;
	bool runAction2 = false;

	if ( event->evType == SE_KEY ) {
		key = event->evValue;

		if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW || key == K_MOUSE1)  {
			// never affects the state, but we want to execute script handlers anyway
			if ( !event->evValue2 ) {
				RunScript( ON_ACTIONRELEASE );
				return cmd;
			}
			currentChoice++;
			if (currentChoice >= choices.Num()) {
				currentChoice = 0;
			}
			runAction = true;
		}

		if ( key == K_LEFTARROW || key == K_KP_LEFTARROW || key == K_MOUSE2) {
			// never affects the state, but we want to execute script handlers anyway
			if ( !event->evValue2 ) {
				RunScript( ON_ACTIONRELEASE );
				return cmd;
			}
			currentChoice--;
			if (currentChoice < 0) {
				currentChoice = choices.Num() - 1;
			}
			runAction = true;
		}

		if ( !event->evValue2 ) {
			// is a key release with no action catch
			return "";
		}

	} else if ( event->evType == SE_CHAR ) {

		key = event->evValue;

		int potentialChoice = -1;
		for ( int i = 0; i < choices.Num(); i++ ) {
			if ( toupper(key) == toupper(choices[i][0]) ) {
				if ( i < currentChoice && potentialChoice < 0 ) {
					potentialChoice = i;
				} else if ( i > currentChoice ) {
					potentialChoice = -1;
					currentChoice = i;
					break;
				}
			}
		}
		if ( potentialChoice >= 0 ) {
			currentChoice = potentialChoice;
		}

		runAction = true;
		runAction2 = true;

	} else {
		return "";
	}

	if ( runAction ) {
		RunScript( ON_ACTION );
	}

	if ( choiceType == 0 ) {
		cvarStr.Set( va( "%i", currentChoice ) );
	} else if ( values.Num() ) {
		cvarStr.Set( values[ currentChoice ] );
	} else {
		cvarStr.Set( choices[ currentChoice ] );
	}

	UpdateVars( false );

	if ( runAction2 ) {
		RunScript( ON_ACTIONRELEASE );
	}
	
	return cmd;
}

void idChoiceWindow::ValidateChoice() {
	if ( currentChoice < 0 || currentChoice >= choices.Num() ) {
		currentChoice = 0;
	}
	if ( choices.Num() == 0 ) {
		choices.Append( "No Choices Defined" );
	}
}

void idChoiceWindow::UpdateChoice() {
	if ( !updateStr.Num() ) {
		return;
	}
	UpdateVars( true );	
	updateStr.Update();
	if ( choiceType == 0 ) {
		// ChoiceType 0 stores current as an integer in either cvar or gui
		// If both cvar and gui are defined then cvar wins, but they are both updated
		if ( updateStr[ 0 ]->NeedsUpdate() ) {
			currentChoice = atoi( updateStr[ 0 ]->c_str() );
		}
		ValidateChoice();
	} else {
		// ChoiceType 1 stores current as a cvar string
		int c = ( values.Num() ) ? values.Num() : choices.Num();
		int i;
		for ( i = 0; i < c; i++ ) {
			if ( idStr::Icmp( cvarStr.c_str(), ( values.Num() ) ? values[i] : choices[i] ) == 0 ) {
				break;
			}
		}
		if (i == c) {
			i = 0;
		}
		currentChoice = i;
		ValidateChoice();
	}
}

bool idChoiceWindow::ParseInternalVar(const char *_name, idParser *src) {
	if (idStr::Icmp(_name, "choicetype") == 0) {
		choiceType = src->ParseInt();
		return true;
	}
	if (idStr::Icmp(_name, "currentchoice") == 0) {
		currentChoice = src->ParseInt();
		return true;
	}
	return idWindow::ParseInternalVar(_name, src);
}


idWinVar *idChoiceWindow::GetThisWinVarByName(const char *varname) {
	if ( idStr::Icmp( varname, "choices" ) == 0 ) {
		return &choicesStr;
	}
	if ( idStr::Icmp( varname, "values" ) == 0 ) {
		return &choiceVals;
	}
	if ( idStr::Icmp( varname, "cvar" ) == 0 ) {
		return &cvarStr;
	}
	if ( idStr::Icmp( varname, "gui" ) == 0 ) {
		return &guiStr;
	}
	if ( idStr::Icmp( varname, "liveUpdate" ) == 0 ) {
		return &liveUpdate;
	}
	if ( idStr::Icmp( varname, "updateGroup" ) == 0 ) {
		return &updateGroup;
	}
	
	return idWindow::GetThisWinVarByName(varname);
}

// update the lists whenever the WinVar have changed
void idChoiceWindow::UpdateChoicesAndVals( void ) {
	if ( latchedChoices.Icmp( choicesStr ) ) {
		choices.Clear();

		// split the choices into a list
		std::vector<std::string> choiceParts;
		std::string choices_std = choicesStr.c_str();
		stdext::split(choiceParts, choices_std, ";");

		if (choiceParts.size() <= 0)
		{
			common->Warning("The choices string array is empty for %s.", cvarStr.c_str());
			values.Clear();
			return;
		}
		for (unsigned int i = 0; i < choiceParts.size(); i++)
		{
			choices.Append( idStr( choiceParts[i].c_str() ) );
//			common->Printf("Cur choice: %s (index: %i)\n", choiceParts[i].c_str(), i);
		}
		latchedChoices = choicesStr.c_str();
	}
	if ( choiceVals.Length() && latchedVals.Icmp( choiceVals ) ) {
		values.Clear();

		// split the values into a list
		std::vector<std::string> valuesParts;
		std::string values_std = choiceVals.c_str();
		stdext::split(valuesParts, values_std, ";");

		if (valuesParts.size() <= 0)
		{
			common->Warning("The values string array is empty for %s.", cvarStr.c_str());
			return;
		}
		for (unsigned int i = 0; i < valuesParts.size(); i++)
		{
			values.Append( idStr( valuesParts[i].c_str() ) );
//			common->Printf("Cur value: %s (index: %i)\n", valuesParts[i].c_str(), i);
		}

		if (choices.Num() != values.Num())
		{
			common->Warning("idChoiceWindow:: gui '%s' window '%s' has value count unequal to choices count",
			gui->GetSourceFile(), name.c_str() ); 
		}
		latchedVals = choiceVals.c_str();
	}
}

void idChoiceWindow::PostParse() {
	idWindow::PostParse();
	UpdateChoicesAndVals();

	InitVars();
	UpdateChoice();
	UpdateVars(false);

	flags |= WIN_CANFOCUS;
}

void idChoiceWindow::Draw(int time, float x, float y) {
	idVec4 color = foreColor;

	UpdateChoicesAndVals();
	UpdateChoice();

	if ( textShadow ) {
		idStr shadowText = choices[currentChoice];
		idRectangle shadowRect = textRect;

		shadowText.RemoveColors();
		shadowRect.x += textShadow;
		shadowRect.y += textShadow;

		dc->DrawText( shadowText, textScale, textAlign, colorBlack, shadowRect, false, -1 );
	}

	if ( hover && !noEvents && Contains(gui->CursorX(), gui->CursorY()) ) {
		color = hoverColor;
	} else {
		hover = false;
	}
	if ( flags & WIN_FOCUS ) {
		color = hoverColor;
	}

	dc->DrawText( choices[currentChoice], textScale, textAlign, color, textRect, false, -1 );
}

void idChoiceWindow::Activate( bool activate, idStr &act ) {
	idWindow::Activate( activate, act );
	if ( activate ) {
		// sets the gui state based on the current choice the window contains
		UpdateChoice();
	}
}
