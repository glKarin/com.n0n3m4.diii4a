/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "ListGUILocal.h"
#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"

extern idCVar r_skipGuiShaders;		// 1 = don't render any gui elements on surfaces
// BOYETTE MOUSE ACCELERATION BEGIN - this was added 10/17/2016
extern idCVar m_menu_acceleration;		// how much acceleration is applied to the mouse on the menus
// BOYETTE MOUSE ACCELERATION END

idUserInterfaceManagerLocal	uiManagerLocal;
idUserInterfaceManager *	uiManager = &uiManagerLocal;

/*
===============================================================================

	idUserInterfaceManagerLocal

===============================================================================
*/

void idUserInterfaceManagerLocal::Init() {
	screenRect = idRectangle(0, 0, 1280, 960);
	dc.Init();
}

void idUserInterfaceManagerLocal::Shutdown() {
	guis.DeleteContents( true );
	demoGuis.DeleteContents( true );
	dc.Shutdown();
}

void idUserInterfaceManagerLocal::Touch( const char *name ) {
	idUserInterface *gui = Alloc();
	gui->InitFromFile( name );
//	delete gui;
}

void idUserInterfaceManagerLocal::WritePrecacheCommands( idFile *f ) {

	int c = guis.Num();
	for( int i = 0; i < c; i++ ) {
		char	str[1024];
		sprintf( str, "touchGui %s\n", guis[i]->Name() );
		common->Printf( "%s", str );
		f->Printf( "%s", str );
	}
}

void idUserInterfaceManagerLocal::SetSize( float width, float height ) {
	dc.SetSize( width, height );
}

void idUserInterfaceManagerLocal::BeginLevelLoad() {
	int c = guis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( (guis[ i ]->GetDesktop()->GetFlags() & WIN_MENUGUI) == 0 ) {
			guis[ i ]->ClearRefs();
			/*
			delete guis[ i ];
			guis.RemoveIndex( i );
			i--; c--;
			*/
		}
	}
}

void idUserInterfaceManagerLocal::EndLevelLoad() {
	int c = guis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( guis[i]->GetRefs() == 0 ) {
			//common->Printf( "purging %s.\n", guis[i]->GetSourceFile() );

			// use this to make sure no materials still reference this gui
			bool remove = true;
			for ( int j = 0; j < declManager->GetNumDecls( DECL_MATERIAL ); j++ ) {
				const idMaterial *material = static_cast<const idMaterial *>(declManager->DeclByIndex( DECL_MATERIAL, j, false ));
				if ( material->GlobalGui() == guis[i] ) {
					remove = false;
					break;
				}
			}
			if ( remove ) {
				delete guis[ i ];
				guis.RemoveIndex( i );
				i--; c--;
			}
		}
	}
}

void idUserInterfaceManagerLocal::Reload( bool all ) {
	ID_TIME_T ts;

	int c = guis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( !all ) {
			fileSystem->ReadFile( guis[i]->GetSourceFile(), NULL, &ts );
			if ( ts <= guis[i]->GetTimeStamp() ) {
				continue;
			}
		}

		guis[i]->InitFromFile( guis[i]->GetSourceFile() );
		common->Printf( "reloading %s.\n", guis[i]->GetSourceFile() );
	}
}

void idUserInterfaceManagerLocal::ListGuis() const {
	int c = guis.Num();
	common->Printf( "\n   size   refs   name\n" );
	size_t total = 0;
	int copies = 0;
	int unique = 0;
	for ( int i = 0; i < c; i++ ) {
		idUserInterfaceLocal *gui = guis[i];
		size_t sz = gui->Size();
		bool isUnique = guis[i]->interactive;
		if ( isUnique ) {
			unique++;
		} else {
			copies++;
		}
		common->Printf( "%6.1fk %4i (%s) %s ( %i transitions )\n", sz / 1024.0f, guis[i]->GetRefs(), isUnique ? "unique" : "copy", guis[i]->GetSourceFile(), guis[i]->desktop->NumTransitions() );
		total += sz;
	}
	common->Printf( "===========\n  %i total Guis ( %i copies, %i unique ), %.2f total Mbytes", c, copies, unique, total / ( 1024.0f * 1024.0f ) );
}

bool idUserInterfaceManagerLocal::CheckGui( const char *qpath ) const {
	idFile *file = fileSystem->OpenFileRead( qpath );
	if ( file ) {
		fileSystem->CloseFile( file );
		return true;
	}
	return false;
}

idUserInterface *idUserInterfaceManagerLocal::Alloc( void ) const {
	return new idUserInterfaceLocal();
}

void idUserInterfaceManagerLocal::DeAlloc( idUserInterface *gui ) {
	if ( gui ) {
		int c = guis.Num();
		for ( int i = 0; i < c; i++ ) {
			if ( guis[i] == gui ) {
				delete guis[i];
				guis.RemoveIndex( i );
				return;
			}
		}
	}
}

idUserInterface *idUserInterfaceManagerLocal::FindGui( const char *qpath, bool autoLoad, bool needUnique, bool forceNOTUnique ) {
	int c = guis.Num();

	for ( int i = 0; i < c; i++ ) {
		idUserInterfaceLocal *gui = guis[i];
		if ( !idStr::Icmp( guis[i]->GetSourceFile(), qpath ) ) {
			if ( !forceNOTUnique && ( needUnique || guis[i]->IsInteractive() ) ) {
				break;
			}
			guis[i]->AddRef();
			return guis[i];
		}
	}

	if ( autoLoad ) {
		idUserInterface *gui = Alloc();
		if ( gui->InitFromFile( qpath ) ) {
			gui->SetUniqued( forceNOTUnique ? false : needUnique );
			return gui;
		} else {
			delete gui;
		}
	}
	return NULL;
}

idUserInterface *idUserInterfaceManagerLocal::FindDemoGui( const char *qpath ) {
	int c = demoGuis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( !idStr::Icmp( demoGuis[i]->GetSourceFile(), qpath ) ) {
			return demoGuis[i];
		}
	}
	return NULL;
}

idListGUI *	idUserInterfaceManagerLocal::AllocListGUI( void ) const {
	return new idListGUILocal();
}

void idUserInterfaceManagerLocal::FreeListGUI( idListGUI *listgui ) {
	delete listgui;
}

/*
===============================================================================

	idUserInterfaceLocal

===============================================================================
*/

idUserInterfaceLocal::idUserInterfaceLocal() {
	cursorX = cursorY = 0.0;
	previous_cursorX_delta = previous_cursorY_delta = 0.0f;
	desktop = NULL;
	loading = false;
	active = false;
	interactive = false;
	uniqued = false;
	bindHandler = NULL;
	//so the reg eval in gui parsing doesn't get bogus values
	time = 0;
	refs = 1;
	// boyette space command begin
	multiselect_box_active = false;
	multiselect_box_origin_x = 0.0;
	multiselect_box_origin_y = 0.0;

	listwindow = NULL;

	time_Activated = 0;
	frame_Activated = 0;

	last_cursor_move_time = 0;
	old_cursor_x = 0.0f;
	old_cursor_y = 0.0f;
	tooltip_checked_this_mouse_position = true;
	tooltip_text = "";
	tooltip_visible = false;
	tooltip_width = 0.0f;
	tooltip_x_offset = 0.0f;
	tooltip_y_offset = 0.0f;

	handle_time_commands_when_not_active = false;
	// boyette space command end
}

idUserInterfaceLocal::~idUserInterfaceLocal() {
	delete desktop;
	desktop = NULL;
}

const char *idUserInterfaceLocal::Name() const {
	return source;
}

const char *idUserInterfaceLocal::Comment() const {
	if ( desktop ) {
		return desktop->GetComment();
	}
	return "";
}

bool idUserInterfaceLocal::IsInteractive() const {
	return interactive;
}

bool idUserInterfaceLocal::InitFromFile( const char *qpath, bool rebuild, bool cache ) { 

	if ( !( qpath && *qpath ) ) { 
		// FIXME: Memory leak!!
		return false;
	}

	int sz = sizeof( idWindow );
	sz = sizeof( idSimpleWindow );
	loading = true;

	if ( rebuild ) {
		delete desktop;
		desktop = new idWindow( this );
	} else if ( desktop == NULL ) {
		desktop = new idWindow( this );
	}

	source = qpath;
	state.Set( "text", "Test Text!" );

	idParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	//Load the timestamp so reload guis will work correctly
	fileSystem->ReadFile(qpath, NULL, &timeStamp);

	src.LoadFile( qpath );

	if ( src.IsLoaded() ) {
		idToken token;
		while( src.ReadToken( &token ) ) {
			if ( idStr::Icmp( token, "windowDef" ) == 0 ) {
				desktop->SetDC( &uiManagerLocal.dc );
				if ( desktop->Parse( &src, rebuild ) ) {
					desktop->SetFlag( WIN_DESKTOP );
					desktop->FixupParms();
				}
				continue;
			}
		}

		state.Set( "name", qpath );
	} else {
		desktop->SetDC( &uiManagerLocal.dc );
		desktop->SetFlag( WIN_DESKTOP );
		desktop->name = "Desktop";
		desktop->text = va( "Invalid GUI: %s", qpath );
		desktop->rect = idRectangle( 0.0f, 0.0f, 1280.0f, 960.0f );
		desktop->drawRect = desktop->rect;
		desktop->foreColor = idVec4( 1.0f, 1.0f, 1.0f, 1.0f );
		desktop->backColor = idVec4( 0.0f, 0.0f, 0.0f, 1.0f );
		desktop->SetupFromState();
		common->Warning( "Couldn't load gui: '%s'", qpath );
	}

	interactive = desktop->Interactive();

	if ( uiManagerLocal.guis.Find( this ) == NULL ) {
		uiManagerLocal.guis.Append( this );
	}

	// BOYETTE SPACE COMMAND BEGIN
	listwindow = NULL;
	// BOYETTE SPACE COMMAND END

	loading = false;

	return true; 
}

const char *idUserInterfaceLocal::HandleEvent( const sysEvent_t *event, int _time, bool *updateVisuals ) {

	time = _time;

	if ( bindHandler && event->evType == SE_KEY && event->evValue2 == 1 ) {
		const char *ret = bindHandler->HandleEvent( event, updateVisuals );
		bindHandler = NULL;
		return ret;
	}

	if ( event->evType == SE_MOUSE ) {
		// BOYETTE MOUSE ACCELERATION BEGIN - this was added 10/17/2016
		if ( ( !desktop || desktop->GetFlags() & WIN_MENUGUI ) && m_menu_acceleration.GetFloat() > 0.0f ) {
			float mouse_acceleration = m_menu_acceleration.GetFloat();

			// this one works great with m_menu_acceleration as 0.35
			cursorX += ((float)event->evValue / 2.0f) + (previous_cursorX_delta * mouse_acceleration);
			cursorY += ((float)event->evValue2 / 2.0f) + (previous_cursorY_delta * mouse_acceleration);

			previous_cursorX_delta = ((float)event->evValue / 2.0f) + (previous_cursorX_delta * mouse_acceleration);
			previous_cursorY_delta = ((float)event->evValue2 / 2.0f) + (previous_cursorY_delta * mouse_acceleration);

			/* // this was the original but it leaves it too oversensitive for small movements
			cursorX += (float)event->evValue + (previous_cursorX_delta * mouse_acceleration);
			cursorY += (float)event->evValue2 + (previous_cursorY_delta * mouse_acceleration);

			previous_cursorX_delta = (float)event->evValue + (previous_cursorX_delta * mouse_acceleration);
			previous_cursorY_delta = (float)event->evValue2 + (previous_cursorY_delta * mouse_acceleration);
			*/

			/* this work pretty good with 0.5
			cursorX += ((float)event->evValue * mouse_acceleration) + (previous_cursorX_delta * mouse_acceleration);
			cursorY += ((float)event->evValue2 * mouse_acceleration) + (previous_cursorY_delta * mouse_acceleration);

			previous_cursorX_delta = ((float)event->evValue * mouse_acceleration) + (previous_cursorX_delta * mouse_acceleration);
			previous_cursorY_delta = ((float)event->evValue2 * mouse_acceleration) + (previous_cursorY_delta * mouse_acceleration);
			*/

			//common->Printf("x change:" + idStr(previous_cursorX_delta) + "\n");
			//common->Printf("y change:" + idStr(previous_cursorY_delta) + "\n");
		} else {
		// BOYETTE MOUSE ACCELERATION END

			// BOYETTE NOTE IMPORTANT BEGIN: originally it was just this before 10/17/2016
			cursorX += event->evValue;
			cursorY += event->evValue2;
			//common->Printf("x change:" + idStr(event->evValue) + "\n");
			//common->Printf("y change:" + idStr(event->evValue2) + "\n");
			// BOYETTE NOTE IMPORTANT END
		}

		if (cursorX < 0) {
			cursorX = 0;
		}
		if (cursorY < 0) {
			cursorY = 0;
		}
	}
// boyette begin
//	if ( desktop->focusedChild ) {
//		return desktop->focusedChild->HandleEvent( event, updateVisuals );
//	}
// boyette end - commented out because it screws up the menu - disables it.

	if ( desktop ) {
		return desktop->HandleEvent( event, updateVisuals );
	} 

	return "";
}

void idUserInterfaceLocal::HandleNamedEvent ( const char* eventName ) {
	desktop->RunNamedEvent( eventName );
}

void idUserInterfaceLocal::Redraw( int _time ) {
	if ( r_skipGuiShaders.GetInteger() > 5 ) {
		return;
	}
	if ( !loading && desktop ) {
		time = _time;
		uiManagerLocal.dc.PushClipRect( uiManagerLocal.screenRect );
		desktop->Redraw( 0, 0 );
		uiManagerLocal.dc.PopClipRect();
	}
}

void idUserInterfaceLocal::DrawCursor() {
	if ( !desktop || desktop->GetFlags() & WIN_MENUGUI ) {
		uiManagerLocal.dc.DrawCursor(&cursorX, &cursorY, 32.0f );
	} else {
		uiManagerLocal.dc.DrawCursor(&cursorX, &cursorY, 64.0f );
	}
}

const idDict &idUserInterfaceLocal::State() const {
	return state;
}

void idUserInterfaceLocal::DeleteStateVar( const char *varName ) {
	state.Delete( varName );
}

void idUserInterfaceLocal::SetStateString( const char *varName, const char *value ) {
	state.Set( varName, value );
}

void idUserInterfaceLocal::SetStateBool( const char *varName, const bool value ) {
	state.SetBool( varName, value );
}

void idUserInterfaceLocal::SetStateInt( const char *varName, const int value ) {
	state.SetInt( varName, value );
}

void idUserInterfaceLocal::SetStateFloat( const char *varName, const float value ) {
	state.SetFloat( varName, value );
}

const char* idUserInterfaceLocal::GetStateString( const char *varName, const char* defaultString ) const {
	return state.GetString(varName, defaultString);
}

bool idUserInterfaceLocal::GetStateBool( const char *varName, const char* defaultString ) const {
	return state.GetBool(varName, defaultString); 
}

int idUserInterfaceLocal::GetStateInt( const char *varName, const char* defaultString ) const {
	return state.GetInt(varName, defaultString);
}

float idUserInterfaceLocal::GetStateFloat( const char *varName, const char* defaultString ) const {
	return state.GetFloat(varName, defaultString);
}

void idUserInterfaceLocal::StateChanged( int _time, bool redraw ) {
	time = _time;
	if (desktop) {
		desktop->StateChanged( redraw );
	}
	if ( state.GetBool( "noninteractive" ) ) {
		interactive = false;
	}
	else {
		if (desktop) {
			interactive = desktop->Interactive();
		} else {
			interactive = false;
		}
	}
}

const char *idUserInterfaceLocal::Activate(bool activate, int _time) {
	time = _time;
	active = activate;
	// BOYETTE BEGIN
	time_Activated = _time;
	frame_Activated = idLib::frameNumber;
	// BOYETTE END
	if ( desktop ) {
		activateStr = "";
		desktop->Activate( activate, activateStr );
		return activateStr;
	}
	return "";
}

void idUserInterfaceLocal::Trigger(int _time) {
	time = _time;
	if ( desktop ) {
		desktop->Trigger();
	}
}

void idUserInterfaceLocal::ReadFromDemoFile( class idDemoFile *f ) {
	idStr work;
	f->ReadDict( state );
	source = state.GetString("name");

	if (desktop == NULL) {
		f->Log("creating new gui\n");
		desktop = new idWindow(this);
	   	desktop->SetFlag( WIN_DESKTOP );
	   	desktop->SetDC( &uiManagerLocal.dc );
		desktop->ReadFromDemoFile(f);
	} else {
		f->Log("re-using gui\n");
		desktop->ReadFromDemoFile(f, false);
	}

	f->ReadFloat( cursorX );
	f->ReadFloat( cursorY );

	bool add = true;
	int c = uiManagerLocal.demoGuis.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( uiManagerLocal.demoGuis[i] == this ) {
			add = false;
			break;
		}
	}

	if (add) {
		uiManagerLocal.demoGuis.Append(this);
	}
}

void idUserInterfaceLocal::WriteToDemoFile( class idDemoFile *f ) {
	idStr work;
	f->WriteDict( state );
	if (desktop) {
		desktop->WriteToDemoFile(f);
	}

	f->WriteFloat( cursorX );
	f->WriteFloat( cursorY );
}

bool idUserInterfaceLocal::WriteToSaveGame( idFile *savefile ) const {
	int len;
	const idKeyValue *kv;
	const char *string;

	int num = state.GetNumKeyVals();
	savefile->Write( &num, sizeof( num ) );

	for( int i = 0; i < num; i++ ) {
		kv = state.GetKeyVal( i );
		len = kv->GetKey().Length();
		string = kv->GetKey().c_str();
		savefile->Write( &len, sizeof( len ) );
		savefile->Write( string, len );

		len = kv->GetValue().Length();
		string = kv->GetValue().c_str();
		savefile->Write( &len, sizeof( len ) );
		savefile->Write( string, len );
	}

	savefile->Write( &active, sizeof( active ) );
	savefile->Write( &interactive, sizeof( interactive ) );
	savefile->Write( &uniqued, sizeof( uniqued ) );
	savefile->Write( &time, sizeof( time ) );
	len = activateStr.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( activateStr.c_str(), len );
	len = pendingCmd.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( pendingCmd.c_str(), len );
	len = returnCmd.Length();
	savefile->Write( &len, sizeof( len ) );
	savefile->Write( returnCmd.c_str(), len );

	savefile->Write( &cursorX, sizeof( cursorX ) );
	savefile->Write( &cursorY, sizeof( cursorY ) );

	// BOYETTE SAVE BEGIN
	savefile->Write( &time_Activated, sizeof( time_Activated ) );
	savefile->Write( &frame_Activated, sizeof( frame_Activated ) );
	savefile->Write( &handle_time_commands_when_not_active, sizeof( handle_time_commands_when_not_active ) );
	savefile->Write( &last_cursor_move_time, sizeof( last_cursor_move_time ) );
	savefile->Write( &old_cursor_x, sizeof( old_cursor_x ) );
	savefile->Write( &old_cursor_y, sizeof( old_cursor_y ) );
	// BOYETTE SAVE END

	desktop->WriteToSaveGame( savefile );

	return true;
}

bool idUserInterfaceLocal::ReadFromSaveGame( idFile *savefile ) {
	int num;
	int i, len;
	idStr key;
	idStr value;

	savefile->Read( &num, sizeof( num ) );

	state.Clear();
	for( i = 0; i < num; i++ ) {
		savefile->Read( &len, sizeof( len ) );
		key.Fill( ' ', len );
		savefile->Read( &key[0], len );

		savefile->Read( &len, sizeof( len ) );
		value.Fill( ' ', len );
		savefile->Read( &value[0], len );
		
		state.Set( key, value );
	}

	savefile->Read( &active, sizeof( active ) );
	savefile->Read( &interactive, sizeof( interactive ) );
	savefile->Read( &uniqued, sizeof( uniqued ) );
	savefile->Read( &time, sizeof( time ) );

	savefile->Read( &len, sizeof( len ) );
	activateStr.Fill( ' ', len );
	savefile->Read( &activateStr[0], len );
	savefile->Read( &len, sizeof( len ) );
	pendingCmd.Fill( ' ', len );
	savefile->Read( &pendingCmd[0], len );
	savefile->Read( &len, sizeof( len ) );
	returnCmd.Fill( ' ', len );
	savefile->Read( &returnCmd[0], len );

	savefile->Read( &cursorX, sizeof( cursorX ) );
	savefile->Read( &cursorY, sizeof( cursorY ) );

	// BOYETTE RESTORE BEGIN
	savefile->Read( &time_Activated, sizeof( time_Activated ) );
	savefile->Read( &frame_Activated, sizeof( frame_Activated ) );
	savefile->Read( &handle_time_commands_when_not_active, sizeof( handle_time_commands_when_not_active ) );
	savefile->Read( &last_cursor_move_time, sizeof( last_cursor_move_time ) );
	savefile->Read( &old_cursor_x, sizeof( old_cursor_x ) );
	savefile->Read( &old_cursor_y, sizeof( old_cursor_y ) );
	// BOYETTE RESTORE END

	desktop->ReadFromSaveGame( savefile );

	// BOYETTE SPACE COMMAND BEGIN
	listwindow = NULL;
	// BOYETTE SPACE COMMAND END

	return true;
}

size_t idUserInterfaceLocal::Size() {
	size_t sz = sizeof(*this) + state.Size() + source.Allocated();
	if ( desktop ) {
		sz += desktop->Size();
	}
	return sz;
}

void idUserInterfaceLocal::RecurseSetKeyBindingNames( idWindow *window ) {
	int i;
	idWinVar *v = window->GetWinVarByName( "bind" );
	if ( v ) {
		SetStateString( v->GetName(), idKeyInput::KeysFromBinding( v->GetName() ) );
	}
	i = 0;
	while ( i < window->GetChildCount() ) {
		idWindow *next = window->GetChild( i );
		if ( next ) {
			RecurseSetKeyBindingNames( next );
		}
		i++;
	}
}

/*
==============
idUserInterfaceLocal::SetKeyBindingNames
==============
*/
void idUserInterfaceLocal::SetKeyBindingNames( void ) {
	if ( !desktop ) {
		return;
	}
	// walk the windows
	RecurseSetKeyBindingNames( desktop );
}

/*
==============
idUserInterfaceLocal::SetCursor
==============
*/
void idUserInterfaceLocal::SetCursor( float x, float y ) {
	cursorX = x;
	cursorY = y;
}

// boyette space command begin
/*
==============
idUserInterfaceLocal::SetCursorImage
==============
*/
void idUserInterfaceLocal::SetCursorImage( int cursortochangeto ) {
	uiManagerLocal.dc.SetCursor(cursortochangeto);
}

/*
==============
idUserInterfaceLocal::ScrollListGUIToBottom
==============
*/
void idUserInterfaceLocal::ScrollListGUIToBottom( idStr winName ) {
	idWindow* listEntity = NULL;
	if ( listwindow && listwindow->win && listwindow->win->name == winName ) {
		listEntity = listwindow->win;
	} else {
		listwindow = GetDesktop()->FindChildByName(winName);
		if ( listwindow ) {
			listEntity = listwindow->win;
		}
	}

	if ( listEntity ) {
		const sysEvent_t* ev;
		bool updateVisuals = true;
		sysEvent_t tmpEv = sys->GenerateMouseButtonEvent( 9, true );
		ev = &tmpEv;

		listEntity->allow_multiselect_box_events = false;
		listEntity->HandleEvent(ev, &updateVisuals );


		listEntity->StateChanged(true);
	}
	//common->Printf( "The name of the scroll gui is %s.\n", listEntity->GetName() );
	//common->Printf( "The name of the parent scroll gui is %s.\n", listEntity->GetParent()->GetName() );
}

void idUserInterfaceLocal::ScrollListGUIToTop( idStr winName ) {
	idWindow* listEntity = NULL;
	if ( listwindow && listwindow->win && listwindow->win->name == winName ) {
		listEntity = listwindow->win;
	} else {
		listwindow = GetDesktop()->FindChildByName(winName);
		if ( listwindow ) {
			listEntity = listwindow->win;
		}
	}

	if ( listEntity ) {
		const sysEvent_t* ev;
		bool updateVisuals = true;
		sysEvent_t tmpEv = sys->GenerateMouseButtonEvent( 10, true );
		ev = &tmpEv;

		listEntity->allow_multiselect_box_events = false;
		listEntity->HandleEvent(ev, &updateVisuals );


		listEntity->StateChanged(true);
	}
	//common->Printf( "The name of the scroll gui is %s.\n", listEntity->GetName() );
	//common->Printf( "The name of the parent scroll gui is %s.\n", listEntity->GetParent()->GetName() );
}

void idUserInterfaceLocal::BeginMultiselectBox() {
	// BOYETTE NOTE TODO: just need to link the first two arguments up with the beginning onaction and for the second two arguments use the current location of the cursor. going to need a function called uiManagerLocal.dc.DrawFilledAndBorderedRect
	//if ( ( cursorX-multiselect_box_origin_x < 0 && cursorX-multiselect_box_origin_y > 0 ) || ( cursorX-multiselect_box_origin_x > 0 && cursorX-multiselect_box_origin_y < 0 ) ) {
		//uiManagerLocal.dc.DrawFilledRect(cursorX,cursorY,multiselect_box_origin_x - cursorX,multiselect_box_origin_y-cursorY,idDeviceContext::colorRed);
	//} else {
		if ( cursorX < multiselect_box_origin_x && cursorY > multiselect_box_origin_y ) {
			MultiSelectBox.x = cursorX;
			MultiSelectBox.y = multiselect_box_origin_y;
			MultiSelectBox.w = multiselect_box_origin_x-cursorX;
			MultiSelectBox.h = cursorY-multiselect_box_origin_y;
		} else if ( cursorX > multiselect_box_origin_x && cursorY < multiselect_box_origin_y ) {
			MultiSelectBox.x = multiselect_box_origin_x;
			MultiSelectBox.y = cursorY;
			MultiSelectBox.w = cursorX-multiselect_box_origin_x;
			MultiSelectBox.h = multiselect_box_origin_y-cursorY;
		} else if ( cursorX < multiselect_box_origin_x && cursorY < multiselect_box_origin_y ) {
			MultiSelectBox.x = cursorX;
			MultiSelectBox.y = cursorY;
			MultiSelectBox.w = multiselect_box_origin_x-cursorX;
			MultiSelectBox.h = multiselect_box_origin_y-cursorY;
		} else {
			MultiSelectBox.x = multiselect_box_origin_x;
			MultiSelectBox.y = multiselect_box_origin_y;
			MultiSelectBox.w = cursorX-multiselect_box_origin_x;
			MultiSelectBox.h = cursorY-multiselect_box_origin_y;
		}

		if ( ( MultiSelectBox.w * MultiSelectBox.h ) >= 64.0f ) {
			uiManagerLocal.dc.DrawRect(MultiSelectBox.x,MultiSelectBox.y,MultiSelectBox.w,MultiSelectBox.h,2.0f,idDeviceContext::colorMultiSelectBox);
		}
}

void idUserInterfaceLocal::EndMultiselectBox() {
	// BOYETTE NOTE TODO: need to figure out which windows are inside the multiselect box and then call the RunScript( ON_ACTION_MULTISELECT );
	//int child_count = GetDesktop()->GetChildCount();
	//common->Printf( "The number of child windows is %i.\n", child_count );
	//idRectangle MultiSelectRect( multiselect_box_origin_x, multiselect_box_origin_y, cursorX-multiselect_box_origin_x, cursorY-multiselect_box_origin_y);
	if ( ( MultiSelectBox.w * MultiSelectBox.h ) >= 25.0f ) { // this will help ensure that they are actually trying to do a multiselect box and not just clicking.
		if ( GetPendingCmd().Length() ) {
			GetPendingCmd() += " ; ";
			GetPendingCmd() += "clear_crew_multiselection"; // DONE: we still need to implement this command on idPlayer.
		} else {
			GetPendingCmd() += "clear_crew_multiselection"; // DONE: we still need to implement this command on idPlayer.
		}
	}
	LoopThroughChildrenAndRunScript( GetDesktop() );
}

void idUserInterfaceLocal::LoopThroughChildrenAndRunScript( idWindow* Parent ) {
	/*
	// BOYETTE NOTE TODO: need to figure out which windows are inside the multiselect box and then call the RunScript( ON_ACTION_MULTISELECT );
	int child_count = Parent->GetChildCount();
	idRectangle MultiSelectRect( multiselect_box_origin_x, multiselect_box_origin_y, cursorX-multiselect_box_origin_x, cursorY-multiselect_box_origin_y);
	for ( int i = 0; i < child_count; i++ ) {
		idWindow *next = Parent->GetChild( i );
		if ( next && next->visible && !next->noEvents && next->Contains( MultiSelectRect, next->GetActualX(), next->GetActualY() ) ) {
			common->Printf( "Test" );
			next->RunScript( idWindow::ON_ACTION_MULTISELECT );

		}
		//if ( Parent->GetChild( i )->GetChildCount() > 0 ) {
			//LoopThroughChildrenAndRunScript( Parent->GetChild( i ) );
		//}
	}
	*/
	int i;
	i = 0;
	//idRectangle MultiSelectRect( multiselect_box_origin_x, multiselect_box_origin_y, cursorX-multiselect_box_origin_x, cursorY-multiselect_box_origin_y);
	while ( i < Parent->GetChildCount() ) {
		idWindow *next = Parent->GetChild( i );
		if ( next ) {
			//if ( next && next->visible && !next->noEvents && GetDesktop()->Contains( MultiSelectRect, next->GetActualX(), next->GetActualY() ) ) {
			if ( next && next->visible && !next->noEvents && MultiSelectBox.Contains( next->GetActualX(), next->GetActualY() ) ) {
				//common->Printf( "%s.\n", next->GetName() );
				//common->Printf( "Has number of children: %i.\n", next->GetChildCount() );
				//common->Printf( "Has actual X of: %f.\n", next->GetActualX() );
				//common->Printf( "Has actual Y of: %f.\n", next->GetActualY() );
				//next->GainCapture();
				//next->GainFocus();
				//next->Activate(true);
				//GetDesktop()->SetFocus(next,true);
				//next->SetFlag(WIN_ACTIVE);
				//next->SetFlag(WIN_CAPTURE);
				//next->SetFlag(WIN_WANTENTER);
				//GetDesktop()->captureChild = next;
				//GetDesktop()->focusedChild = next;
				//next->cmd = "";



				next->RunScript( idWindow::ON_ACTION_MULTISELECT ); // BOYETTE NOTE TODO: apparently the mouse must be over it for this script to run effectively - not sure why yet - must be a check somewhere.
				//common->Printf( "%s has a command of: %s.\n", next->GetName(), next->cmd.c_str() );
				if ( next->cmd.Length() ) {
					if ( GetPendingCmd().Length() ) {
						GetPendingCmd() += " ; ";
						GetPendingCmd() += next->cmd; // this seems to have done it.
					} else {
						GetPendingCmd() += next->cmd; // this seems to have done it.
					}
				}
				//common->Printf( "The pending command is: %s.\n", GetPendingCmd().c_str() );
				//next->cmd = ""; // BOYETTE NOTE TODO: this doesn't seem to be necessary - but it seems like it might be a good idea so commands don't build up - but the cmd is getting cleared out somewhere so it doesn't seem to be a problem.



				/*
				const sysEvent_t* ev;
				bool updateVisuals = true;
				ev = &sys->GenerateMouseButtonEvent( 10, true );
				next->HandleEvent(ev, &updateVisuals );
				next->StateChanged(true);
				*/
				//next->HandleEvent(
				//next->Activate(true,
				//next->cmd = next->cmd + " ;";
				//next->cmd = "";
				//next->AddCommand(next->cmd.c_str());
				//common->Printf( "%s has a command of: %s.\n", next->GetName(), next->cmd.c_str() );
				//next->cmd = "multiselect_add_medical_officer_to_selection";
				//next->AddCommand
				//next->StateChanged(true);
				//desktop->StateChanged( true );
				//common->Printf( "%s has a command of: %s.\n", next->GetName(), next->cmd.c_str() );
				//common->Printf( "%s has a command of: %s.\n", next->GetName(), next->cmd.c_str() );
				//returnCmd = next->cmd;
				//next->AddCommand( "multiselect_add_medical_officer_to_selection" );
				//next->HandleEvent( sysEvent_t.evPtr(),true);
				//next->cmd = "multiselect_add_medical_officer_to_selection";
				//GetReturnCmd() = "multiselect_add_medical_officer_to_selection";
			}
			if ( next && next->visible && !next->noEvents ) {
				LoopThroughChildrenAndRunScript( next );
			}
		}
		i++;
	}
}


class idEditWindow;
#include "EditWindow.h"

bool idUserInterfaceLocal::HasEditWindowAsFocus() {
	idWindow* focused_window = GetDesktop()->focusedChild;
	if ( focused_window ) {
		idEditWindow* edit_window = dynamic_cast<idEditWindow*>(focused_window);
		if ( edit_window ) {
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

void idUserInterfaceLocal::ClearFocusWindow() {
	GetDesktop()->focusedChild = NULL;
}



void idUserInterfaceLocal::UpdateTooltip() {
	bool cursor_moved = false;
	if ( old_cursor_x != CursorX() || old_cursor_y != CursorY() ) {
		cursor_moved = true;
		last_cursor_move_time = GetTime();
		tooltip_checked_this_mouse_position = false;
	}

	if ( !cursor_moved && GetTime() - last_cursor_move_time > TOOLTIP_ACTIVATION_DELAY_MS && !tooltip_checked_this_mouse_position ) {
		GetTooltipText( GetDesktop() );
		if ( tooltip_text != "" ) {
			tooltip_visible = true;
			tooltip_width = TOOLTIP_TEXT_PADDING + ( (float)strlen(tooltip_text) * TOOLTIP_LETTER_WIDTH );
			if ( (CursorX() + tooltip_width + TOOLTIP_TEXT_PADDING) > VIRTUAL_WIDTH ) {
				tooltip_x_offset = -tooltip_width - TOOLTIP_BOX_HEIGHT;
			}
			if ( (CursorY() + TOOLTIP_BOX_HEIGHT ) > VIRTUAL_HEIGHT ) {
				tooltip_y_offset = -(((CursorY() + TOOLTIP_TEXT_PADDING ) - (float)VIRTUAL_HEIGHT) + TOOLTIP_TEXT_PADDING);
			} else {
				tooltip_y_offset = 0.0f;
			}
		} else {
			tooltip_visible = false;
			tooltip_width = 0.0f;
			tooltip_x_offset = 0.0f;
			tooltip_y_offset = 0.0f;
		}
		tooltip_checked_this_mouse_position = true;
	} else if ( cursor_moved ) {
		tooltip_text = "";
		tooltip_visible = false;
		tooltip_width = 0.0f;
		tooltip_x_offset = 0.0f;
		tooltip_y_offset = 0.0f;
	}

	old_cursor_x = CursorX();
	old_cursor_y = CursorY();
}
void idUserInterfaceLocal::GetTooltipText( idWindow* Parent ) { // BOYETTE NOTE: this should find the child window farthest from the desktop
	for ( int i = 0; i < Parent->GetChildCount(); i++ ) {
		idWindow *next = Parent->GetChild( i );
		if ( next && next->visible && next->Contains(CursorX(),CursorY()) ) {
			if ( next->tooltip_text.Length() > 0 ) { //Length works better - THIS DOESN"T ALWAYS WORK FOR SOME REASON: next->tooltip_text.c_str() != "" ) {
				tooltip_text = next->tooltip_text.c_str();
			}
			GetTooltipText( next );
		}
	}
}
// boyette space command end

