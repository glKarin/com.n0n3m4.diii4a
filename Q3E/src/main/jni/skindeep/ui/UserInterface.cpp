/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

#include "sys/platform.h"
#include "framework/FileSystem.h"
#include "framework/DemoFile.h"
#include "framework/KeyInput.h"
#include "framework/UsercmdGen.h"
#include "gamesys/SaveGame.h"
#include "ui/ListGUILocal.h"
#include "ui/DeviceContext.h"
#include "ui/Window.h"

#include "ui/UserInterfaceLocal.h"

extern idCVar r_skipGuiShaders;		// 1 = don't render any gui elements on surfaces
extern idCVar joy_mouseFriction;
extern idCVar r_scaleMenusTo169; // DG: for the "scale menus to 4:3" hack

idUserInterfaceManagerLocal	uiManagerLocal;
idUserInterfaceManager *	uiManager = &uiManagerLocal;

/*
===============================================================================

	idUserInterfaceManagerLocal

===============================================================================
*/

void idUserInterfaceManagerLocal::Init() {
	screenRect = idRectangle(0, 0, 640, 480);
	dc.Init();
	groupName = "mainmenu";
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

void idUserInterfaceManagerLocal::SetGroup( const char* group, bool persists ) {
	groupName = group;
	groupName.ToLower();
	if (persists) {
		groupPersist.AddUnique( groupName );
	}
}

void idUserInterfaceManagerLocal::BeginLevelLoad( ) {
	for ( int i = guis.Num()-1; i  >= 0; i--) {
		if (groupPersist.FindIndex(guis[ i ]->GetGroup()) < 0) {

			// use this to make sure no materials still reference this gui
			bool remove = true;
			for ( int j = 0; j < declManager->GetNumDecls( DECL_MATERIAL ); j++ ) {
				const idMaterial *material = static_cast<const idMaterial *>(declManager->DeclByIndex( DECL_MATERIAL, j, false ));
				if ( material->GlobalGui() == guis[i] ) {
					remove = false;
					break;
				}
			}
			if (remove) {
				DeAlloc(guis[i]);
			}
		} else if ( (guis[ i ]->GetDesktop()->GetFlags() & WIN_MENUGUI) == 0 ) {
			guis[ i ]->ClearRefs();
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
		common->DPrintf( "reloading %s.\n", guis[i]->GetSourceFile() );
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
		guisGroup.Remove((idUserInterfaceLocal*)gui);
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

idUserInterface *idUserInterfaceManagerLocal::FindGui( const char *qpath, bool autoLoad, bool needUnique, bool forceShared, bool * owned ) {
	assert( (!needUnique && !forceShared) || (needUnique != forceShared) );
#if 0
	assert( !(needUnique && forceShared) ); // contradictory params
	if (owned != nullptr) {
		*owned = false;
	}

	if (forceShared || !needUnique) {
		for (int i = 0; i < guis.Num(); i++) {
			if (!idStr::Icmp(guis[i]->GetSourceFile(), qpath)) {
				if ( guis[i]->IsUniqued() ) {
					continue;
				}

				if ( needUnique || guis[i]->IsInteractive() ) {
					continue;
				}

				guis[i]->AddRef();
				return guis[i];
			}
		}
	}

	if ( autoLoad ) { // duplicate/unique
		idUserInterface *gui = Alloc();
		if ( gui->InitFromFile( qpath ) ) {
			if (owned != nullptr) {
				*owned = true; // the calling obj is the creator of this class
			}
			gui->SetUniqued( !forceShared ? false : needUnique );
			return gui;
		} else {
			delete gui;
		}
	}
#else
	if (!needUnique)
	{
		for (int i = 0; i < guis.Num(); i++) {
			if (!idStr::Icmp(guis[i]->GetSourceFile(), qpath)) {
				if (!forceShared && (guis[i]->IsUniqued() || guis[i]->IsInteractive())) {
					break;
				}
				guis[i]->AddRef();
				return guis[i];
			}
		}
	}

	if ( autoLoad ) {
		idUserInterface *gui = Alloc();
		if ( gui->InitFromFile( qpath ) ) {
			gui->SetUniqued( forceShared ? false : needUnique );
			return gui;
		} else {
			delete gui;
		}
	}
#endif
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


const idStr& idUserInterfaceManagerLocal::GetGroup() const
{
	return groupName;
}

/*
===============================================================================

	idUserInterfaceLocal

===============================================================================
*/

idUserInterfaceLocal::idUserInterfaceLocal() {
	groupName = idStr();

	cursorX = cursorY = 0.0;
	desktop = NULL;
	loading = false;
	active = false;
	interactive = false;
	uniqued = false;
	bindHandler = NULL;
	//so the reg eval in gui parsing doesn't get bogus values
	time = 0;
	refs = 1;
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
		desktop->rect = idRectangle( 0.0f, 0.0f, 640.0f, 480.0f );
		desktop->drawRect = desktop->rect;
		desktop->foreColor = idVec4( 1.0f, 1.0f, 1.0f, 1.0f );
		desktop->backColor = idVec4( 0.0f, 0.0f, 0.0f, 1.0f );
		desktop->SetupFromState();
		common->Warning( "Couldn't load gui: '%s'", qpath );
	}

	interactive = desktop->Interactive();

	if ( uiManagerLocal.guis.Find( this ) == NULL ) {
		uiManagerLocal.guis.Append( this );
		groupName = uiManagerLocal.GetGroup();
		uiManagerLocal.guisGroup.AddUnique( this );
	}

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

	if ( event->evType == SE_MOUSE || event->evType == SE_MOUSEGAMEPAD ) {
		if ( !desktop || (desktop->GetFlags() & WIN_MENUGUI) ) {
			// DG: this is a fullscreen GUI, scale the mousedelta added to cursorX/Y
			//     by 640/w, because the GUI pretends that everything is 640x480
			//     even if the actual resolution is higher => mouse moved too fast
			float w = renderSystem->GetScreenWidth();
			float h = renderSystem->GetScreenHeight();
			if( w <= 0.0f || h <= 0.0f ) {
				w = VIRTUAL_WIDTH;
				h = VIRTUAL_HEIGHT;
			}

			float speedMultiplier = 1.0f;
			idWindow* overChild = desktop->GetOverChild();
			if (event->evType == SE_MOUSEGAMEPAD && overChild && overChild->OverchildInteractive())
			{
				speedMultiplier = joy_mouseFriction.GetFloat();
			}
			else
			{
#if 0
				// blendo eric: TODO fix this for high dpi/sens mice which move too fast on the UI
				// preferably actually using windows sens instead of raw input

				const float GUIScreenArcDeg = 90.0f; // assume fullscreen gui is equivalent to turning 90 deg from side to side
				idVec2 GUIMouseDegToCursorPixels = 1.0f;//idVec2( m_yaw.GetFloat() , m_pitch.GetFloat() )/ GUIScreenArcDeg;

				idVec2 GUIMouseDegToCursorPixels = idVec2(GUIMouseDegToCursorPixels,GUIMouseDegToCursorPixels);
				speedMultiplier *= (float(VIRTUAL_WIDTH) / w) * GUIMouseDegToCursorPixels.x;
				speedMultiplier *= (float(VIRTUAL_HEIGHT) / h) * GUIMouseDegToCursorPixels.y;			
#endif
			}

			if (r_scaleMenusTo169.GetBool()) {
				// in case we're scaling menus to 4:3, we need to take that into account
				// when scaling the mouse events.
				// no, we can't just call uiManagerLocal.dc.GetFixScaleForMenu() or sth like that,
				// because when we're here dc.SetMenuScaleFix(true) is not active and it'd just return (1, 1)!
				float aspectRatio = w / h;
				static const float virtualAspectRatio = 16.0f/9.0f;
				if (aspectRatio > (virtualAspectRatio + 0.1f)) {
					// widescreen (4:3 is 1.333 3:2 is 1.5, 16:10 is 1.6, 16:9 is 1.7778)
					// => we need to modify cursorX scaling, by modifying w
					w *= virtualAspectRatio / aspectRatio;
				}
			}

			cursorX += event->evValue * (float(VIRTUAL_WIDTH) / w) * speedMultiplier;
			cursorY += event->evValue2 * (float(VIRTUAL_HEIGHT) / h) * speedMultiplier;

			if (cursorX > w) {
				cursorX = w;
			}
			if (cursorY > h) {
				cursorY = h;
			}
		} else {
			// not a fullscreen GUI but some ingame thing - no scaling needed
			cursorX += event->evValue;
			cursorY += event->evValue2;
		}

		if (cursorX < 0) {
			cursorX = 0;
		}
		if (cursorY < 0) {
			cursorY = 0;
		}
	}

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

// SW: Draws text directly into the GUI without going through any of the usual channels.
// This is more difficult to maintain than using GUI definitions, 
// so it is generally reserved for situations where we don't know the exact number of strings we're drawing beforehand.
idRectangle idUserInterfaceLocal::DrawArbitraryText(const idStr& text, float scale, idVec4 colour, float x, float y, idStr fontName, idVec4 shadowColor, int textAlign /* = 0 */, float letterSpace /* = 0 */, idVec4 boxColor)
{
	class idFont* font = renderSystem->RegisterFont( fontName );

	uiManagerLocal.dc.SetSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
	uiManagerLocal.dc.SetFont( font );

	float width = uiManagerLocal.dc.TextWidth(text, scale, 0);
	float height = uiManagerLocal.dc.TextHeight(text, scale, 0);

	idRectangle drawRect(x, y, width, height);

	// SM: Have to account for text alignment here because the rectangle is tightly bound
	if (textAlign == idDeviceContext::ALIGN_RIGHT)
	{
		drawRect.x -= drawRect.w;
	}
	else if (textAlign == idDeviceContext::ALIGN_CENTER)
	{
		drawRect.x -= drawRect.w / 2.0f;
	}

	if (boxColor.w > 0)
	{
		uiManagerLocal.dc.DrawFilledRect(drawRect.x - 1.5f, drawRect.y + 2, drawRect.w + 4, drawRect.h, boxColor);
	}

	if (shadowColor.w > 0)
	{
		uiManagerLocal.dc.DrawText(text.c_str(), scale, textAlign, shadowColor, idRectangle(drawRect.x + 1, drawRect.y + 1, drawRect.w, drawRect.h), false);
	}

	uiManagerLocal.dc.DrawText(text.c_str(), scale, textAlign, colour, drawRect, false);

	return drawRect;
}


idRectangle idUserInterfaceLocal::DrawSubtitleText( const idStr& text, float scale, idVec4 colour, float x, float y, float maxWidth, float maxY, float bgOpacity, idStr fontName,
	bool noDraw )
{
	class idFont* font = renderSystem->RegisterFont( fontName );

	uiManagerLocal.dc.SetSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
	uiManagerLocal.dc.SetFont( font );
	if (r_scaleMenusTo169.GetBool())
	{
		uiManagerLocal.dc.SetMenuScaleFix(true);
	}

	float textWidth = uiManagerLocal.dc.TextWidth( text, scale, 0 );
	float height = uiManagerLocal.dc.MaxCharHeight( scale );

	idRectangle drawRect( x, y, maxWidth, height );
	if ( textWidth <= maxWidth )
	{
		drawRect.w = textWidth + uiManagerLocal.dc.MaxCharWidth(scale);
		drawRect.h += height * 0.4f;
	}

	drawRect.x -= drawRect.w / 2.0f;

	if ( textWidth > maxWidth )
	{
		// Figure out how many lines we need
		idList<int> breaks;
		uiManagerLocal.dc.DrawText( text.c_str(), scale, idDeviceContext::ALIGN_CENTER, colour, drawRect, true, -1, true, &breaks );
		drawRect.h = (breaks.Num() - 1) * (height + 6.0f);
	}

	float yOverflow = ( drawRect.y + drawRect.h ) - maxY;
	if ( yOverflow > 0.0f )
	{
		drawRect.y -= yOverflow;
	}

	if (!noDraw) // blendo eric: allow drawrect size calc without drawing
	{
		// Draw background rectangle
		uiManagerLocal.dc.DrawFilledRect( drawRect.x, drawRect.y, drawRect.w, drawRect.h, idVec4(0.0f, 0.0f, 0.0f, bgOpacity) );

		// Draw actual text
		uiManagerLocal.dc.DrawText( text.c_str(), scale, idDeviceContext::ALIGN_CENTER, colour, drawRect, true );
	}

	if (r_scaleMenusTo169.GetBool())
	{
		uiManagerLocal.dc.SetMenuScaleFix(false);
	}

	return drawRect;
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

void idUserInterfaceLocal::SetStateVec4(const char* varName, const idVec4& value) {
	state.SetVec4(varName, value);
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

idVec4 idUserInterfaceLocal::GetStateVec4(const char* varName, const char* defaultString) const {
	return state.GetVec4(varName, defaultString);
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

void idUserInterfaceLocal::WriteToSaveGame( idSaveGame *savefile ) const {
	idSaveGamePtr::WriteToSaveGame( savefile );

	savefile->WriteCheckSizeMarker();

	savefile->WriteString( groupName );

	savefile->WriteBool( active ); // bool active
	savefile->WriteBool( loading ); // bool loading
	savefile->WriteBool( interactive ); // bool interactive
	savefile->WriteBool( uniqued ); // bool uniqued

	savefile->WriteDict( &state ); // idDict state

	savefile->WriteCheckSizeMarker();

	desktop->WriteToSaveGame( savefile ); // idWindow * desktop
	//idWindow * bindHandler; // idWindow * bindHandler

	savefile->WriteCheckSizeMarker();

	savefile->WriteString( source ); // idString source
	savefile->WriteString( activateStr ); // idString activateStr
	savefile->WriteString( pendingCmd ); // idString pendingCmd
	savefile->WriteString( returnCmd ); // idString returnCmd

	savefile->Write( &timeStamp, sizeof(timeStamp)  ); // ID_TIME_T timeStamp

	savefile->WriteFloat( cursorX ); // float cursorX
	savefile->WriteFloat( cursorY ); // float cursorY

	savefile->WriteInt( time ); // int time

	//savefile->WriteInt( refs ); // int refs

	savefile->WriteCheckSizeMarker();
}

void idUserInterfaceLocal::ReadFromSaveGame( idRestoreGame *savefile ) {
	idSaveGamePtr::ReadFromSaveGame( savefile );

	savefile->ReadCheckSizeMarker();

	savefile->ReadString( groupName );

	savefile->ReadBool( active ); // bool active
	savefile->ReadBool( loading ); // bool loading
	savefile->ReadBool( interactive ); // bool interactive
	savefile->ReadBool( uniqued ); // bool uniqued

	savefile->ReadDict( &state ); // idDict state

	savefile->ReadCheckSizeMarker();

	desktop->ReadFromSaveGame( savefile );  // idWindow * desktop
	//idWindow * bindHandler; // idWindow * bindHandler

	savefile->ReadCheckSizeMarker();

	savefile->ReadString( source ); // idString source
	savefile->ReadString( activateStr ); // idString activateStr
	savefile->ReadString( pendingCmd ); // idString pendingCmd
	savefile->ReadString( returnCmd ); // idString returnCmd

	savefile->Read( &timeStamp, sizeof(timeStamp) ); // ID_TIME_T timeStamp

	savefile->ReadFloat( cursorX ); // float cursorX
	savefile->ReadFloat( cursorY ); // float cursorY

	savefile->ReadInt( time ); // int time

	//savefile->ReadInt( refs ); // int refs

	if (active)
	{

	}

	savefile->ReadCheckSizeMarker();
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
		SetStateString( v->GetName(), idKeyInput::KeysFromBinding( v->GetName(), window->controllerBinding ) );
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
