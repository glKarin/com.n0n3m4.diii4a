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



#include "ListGUILocal.h"
#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "../sound/snd_local.h"

extern idCVar r_skipGuiShaders;		// 1 = don't render any gui elements on surfaces

idUserInterfaceManagerLocal	uiManagerLocal;
idUserInterfaceManager *	uiManager = &uiManagerLocal;

//maximum number of slots for subtitles (stgatilov #2454)
static const int SUBTITLE_SLOTS = 3;

/*
===============================================================================

	idUserInterfaceManagerLocal

===============================================================================
*/

void idUserInterfaceManagerLocal::Init() {
	screenRect = idRectangle(0, 0, 640, 480);
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
	common->Printf( "===========\n  %i total Guis ( %i copies, %i unique ), %.2f total Mbytes\n", c, copies, unique, total / ( 1024.0f * 1024.0f ) );
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

idUserInterface *idUserInterfaceManagerLocal::FindGui( const char *qpath, bool autoLoad, bool needUnique, bool forceNOTUnique, idDict presetDefines ) {
	int c = guis.Num();

	for ( int i = 0; i < c; i++ ) {
		idUserInterfaceLocal *gui = guis[i];
		if ( !idStr::Icmp( gui->GetSourceFile(), qpath ) ) {
			if ( !forceNOTUnique && ( needUnique || gui->IsInteractive() ) ) {
				break;
			}
			gui->AddRef();
			return guis[i];
		}
	}

	if ( autoLoad ) {
		idUserInterface *gui = Alloc();
		((idUserInterfaceLocal*)gui)->presetDefines = presetDefines;
		if ( gui->InitFromFile( qpath, true ) ) {
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


bool idUserInterfaceManagerLocal::IsBindHandlerActive() const {
	for ( idUserInterfaceLocal *gui : guis ) {
		if ( gui->bindHandler != nullptr ) {
			return true;
		}
	}
	return false;
}

/*
===============================================================================

	idUserInterfaceLocal

===============================================================================
*/

idUserInterfaceLocal::idUserInterfaceLocal() {
	cursorX = 320; // duzenko #4403
	cursorY = 240;
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

bool idUserInterfaceLocal::InitFromFile( const char *qpath, bool rebuild ) { 

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

	// stgatilov #5869: removed LEXFL_ALLOWBACKSLASHSTRINGCONCAT,
	//since it breaks multiline macros with a line ending on string literal
	idParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS );

	//Load the timestamp so reload guis will work correctly
	fileSystem->ReadFile(qpath, NULL, &timeStamp);

	src.LoadFile( qpath );

	if ( src.IsLoaded() ) {
		for (int i = 0; i < presetDefines.GetNumKeyVals(); i++) {
			const idKeyValue *kv = presetDefines.GetKeyVal(i);
			idStr line = kv->GetKey() + " " + kv->GetValue();
			src.AddDefine(line.c_str());
		}

		// stgatilov #5869: read one desktop window, from start to end
		// note: original code skipped tokens and tried to parse any found windowDefs over and over again =(
		if ( src.ExpectTokenString("windowDef") ) {
			// parse desktop window
			desktop->SetDC( &uiManagerLocal.dc );
			desktop->Parse( &src, rebuild );
			desktop->SetFlag( WIN_DESKTOP );
			desktop->FixupParms();

			// check that there is no trash afterwards
			// this is typical issue if braces balance is broken (closed without opening)
			idToken token;
			if ( src.ReadToken(&token) ) {
				src.Warning( "Excessive token '%s' after desktop ended", token.c_str() );
			}
		}

		state.Set( "name", qpath );

		//stgatilov: make custom defines by mapper visible in C++
		idStrList defNames = src.GetAllDefineNames();
		for (int i = 0; i < defNames.Num(); i++) {
			const char *name = defNames[i];
			idStr value = src.GetDefineValueString(name);
			defines.Set(name, value);
		}
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

	if ( event->evType == SE_MOUSE ) {
		//stgatilov #4768: taken from dhewm3
		if (!desktop || (desktop->GetFlags() & WIN_MENUGUI)) {
			// DG: this is a fullscreen GUI, scale the mousedelta added to cursorX/Y
			// by 640/w, because the GUI pretends that everything is 640x480
			// even if the actual resolution is higher => mouse moved too fast
			float w = renderSystem->GetScreenWidth();
			float h = renderSystem->GetScreenHeight();
			if (w <= 0.0f || h <= 0.0f) {
				w = 640.0f;
				h = 480.0f;
			}
			float dx = event->evValue * (640.0f / w);
			float dy = event->evValue2 * (480.0f / h);

			//stgatilov #4768: allow customizing menu sensitivity
			if (!cvarSystem->GetCVarBool("sensitivityMenuOverride")) {
				//perform OS-specific adjustments, given that player did not disable them
				Sys_AdjustMouseMovement(dx, dy);
			}
			float sens = cvarSystem->GetCVarFloat("sensitivityMenu");
			cursorX += dx * sens;
			cursorY += dy * sens;
		}
		else {
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
	if (varName[0] == '#')
		return defines.GetString(varName + 1, defaultString);
	return state.GetString(varName, defaultString);
}

bool idUserInterfaceLocal::GetStateBool( const char *varName, const char* defaultString ) const {
	if (varName[0] == '#')
		return defines.GetBool(varName + 1, defaultString);
	return state.GetBool(varName, defaultString); 
}

int idUserInterfaceLocal::GetStateInt( const char *varName, const char* defaultString ) const {
	if (varName[0] == '#')
		return defines.GetInt(varName + 1, defaultString);
	return state.GetInt(varName, defaultString);
}

float idUserInterfaceLocal::GetStateFloat( const char *varName, const char* defaultString ) const {
	if (varName[0] == '#')
		return defines.GetFloat(varName + 1, defaultString);
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

	desktop->ReadFromSaveGame( savefile );

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
	idWinVar *v = window->GetThisWinVarByName( "bind" );
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


/*
==============
idUserInterfaceLocal::RunGuiScript
==============
*/
const char *idUserInterfaceLocal::RunGuiScript(const char *windowName, int scriptNum) {
	idWindow *rootWin = GetDesktop();
	if (!rootWin)
		return NULL;
	drawWin_t dw = rootWin->FindChildByName(windowName);
	if (!dw.win)
		return NULL;
	bool ok = dw.win->RunScript(scriptNum);
	if (!ok)
		return NULL;
	return dw.win->cmd.c_str();
}

/*
==============
idUserInterfaceLocal::ResetWindowTime
==============
*/
bool idUserInterfaceLocal::ResetWindowTime(const char *windowName, int startTime) {
	idWindow *rootWin = GetDesktop();
	if (!rootWin)
		return false;
	drawWin_t dw = rootWin->FindChildByName(windowName);
	if (!dw.win)
		return false;
	dw.win->ResetTime(startTime);
	dw.win->EvalRegs(-1, true);
	return true;
}

idCVar cv_tdm_subtitles_ring(
	"tdm_subtitles_ring", "1", CVAR_BOOL | CVAR_SOUND | CVAR_ARCHIVE,
	"Set to 1 to show subtitles location ring"
);
idCVar tdm_subtitles_ringRadius(
	"tdm_subtitles_ringRadius", "5", CVAR_FLOAT | CVAR_SOUND | CVAR_ARCHIVE,
	"Distance to speaker which corresponds to a point on the location ring boundary (in meters)."
);
idCVar tdm_subtitles_debug(
	"tdm_subtitles_debug", "0", CVAR_BOOL | CVAR_SOUND | CVAR_ARCHIVE,
	"If set to 1, then internal debug information is displayed for subtitles."
);
idCVar tdm_subtitles_volumeMin(
	"tdm_subtitles_volumeMin", "0.003", CVAR_FLOAT | CVAR_SOUND | CVAR_ARCHIVE,
	"Sound of this volume have ring alpha = 0, quieter sounds are not displayed",
	1e-9f, 1.0f
);
idCVar tdm_subtitles_volumeMax(
	"tdm_subtitles_volumeMax", "0.1", CVAR_FLOAT | CVAR_SOUND | CVAR_ARCHIVE,
	"Sound of this volume have ring alpha = 1, louder sounds look the same",
	1e-9f, 1.0f
);
idCVar tdm_subtitles_volumeMinDisappear(
	"tdm_subtitles_volumeMinDisappear", "0.001", CVAR_FLOAT | CVAR_SOUND | CVAR_ARCHIVE,
	"Sounds with less volume get hidden even if they are displayed right now",
	1e-9f, 1.0f
);

// stgatilov #6491
static float ComputeSubtitleAlphaForVolume( float volume ) {
	float vmin = tdm_subtitles_volumeMin.GetFloat();
	float vmax = tdm_subtitles_volumeMax.GetFloat();
	volume = idMath::ClampFloat( vmin, vmax, volume );
	float alpha = idMath::Log( volume / vmin ) / idMath::Log( vmax / vmin );
	return alpha;
}

/*
==============
idUserInterfaceLocal::UpdateSubtitles
==============
*/
void idUserInterfaceLocal::UpdateSubtitles() {
	// make sure all slots for subtitles are allocated
	while ( subtitleSlots.Num() < SUBTITLE_SLOTS ) {
		SubtitleMatch empty = { 0 };
		subtitleSlots.Append( empty );
	}

	idList<SubtitleMatch> matches;
	// fetch active subtitles from sound world
	if ( cv_tdm_subtitles.GetBool() ) {
		if ( idSoundWorld *soundWorld = soundSystem->GetPlayingSoundWorld() ) {
			soundWorld->GetSubtitles( matches );
		}
	}

	// #6491: drop sounds which are too quiet
	int k = 0;
	for ( int i = 0; i < matches.Num(); i++ ) {
		// sounds which are playing have lower threshold to be hidden
		// this is done to reduce subtitles blinking near the threshold
		bool displayedNow = false;
		for ( int j = 0; j < subtitleSlots.Num(); j++ )
			if ( subtitleSlots[j].sample == matches[i].sample )
				displayedNow = true;

		float threshold = ( displayedNow ? tdm_subtitles_volumeMinDisappear : tdm_subtitles_volumeMin ).GetFloat();
		if ( matches[i].volume < threshold )
			continue;

		matches[k++] = matches[i];
	}
	matches.SetNum( k, false );

	// clear all statuses to start fresh assignment
	enum status {
		STATUS_ASSIGNED,
		STATUS_VACANT
	};
	for ( SubtitleMatch &m : matches )
		m.status = STATUS_VACANT;
	for ( SubtitleMatch &m : subtitleSlots )
		m.status = STATUS_VACANT;
	auto SweepAndMatch = [&matches, this](auto compatible) {
		for ( int i = 0; i < matches.Num(); i++ )
			if ( matches[i].status == STATUS_VACANT )
				for ( int j = 0; j < subtitleSlots.Num(); j++ )
					if ( subtitleSlots[j].status == STATUS_VACANT )
						if ( compatible( matches[i], subtitleSlots[j] ) ) {
							matches[i].status = STATUS_ASSIGNED;
							subtitleSlots[j] = matches[i];
							break;
						}
	};

	// assign subtitles in order of decreasing verbosity
	for ( int level = SUBL_STORY; level <= SUBL_EFFECT; level++ ) {
		// match by: subtitle entry, sound sample, sound channel, sound emitter
		SweepAndMatch( [&]( auto &match, auto &slot ) { return match.verbosity == level && match.subtitle == slot.subtitle; } );
		SweepAndMatch( [&]( auto &match, auto &slot ) { return match.verbosity == level && match.sample == slot.sample; } );
		SweepAndMatch( [&]( auto &match, auto &slot ) { return match.verbosity == level && match.channel == slot.channel; } );
		SweepAndMatch( [&]( auto &match, auto &slot ) { return match.verbosity == level && match.emitter == slot.emitter; } );
		// assign the rest to arbitrary places
		SweepAndMatch( [&]( auto &match, auto &slot ) { return match.verbosity == level; } );
	}

	// clear unused slots
	for ( int j = 0; j < subtitleSlots.Num(); j++ )
		if ( subtitleSlots[j].status == STATUS_VACANT )
			subtitleSlots[j] = SubtitleMatch{0};
	// check if we have not enough slots to display everything
	int overflow = 0;
	for ( int i = 0; i < matches.Num(); i++ )
		if ( matches[i].status == STATUS_VACANT )
			overflow++;

	// update GUI variables
	char textVar[] = "subtitleN";
	char enabledVar[] = "subtitleN_nonempty";
	char debugVar[] = "subtitleN_debug";
	char verbosityVar[] = "subtitleN_verbosity";
	char alphaVar[] = "subtitleN_alpha";
	char spatialVar[] = "subtitleN_spatialized";
	char locationXVar[] = "subtitleN_locationXclamped";
	char locationYVar[] = "subtitleN_locationYclamped";
	for ( int j = 0; j < SUBTITLE_SLOTS; j++ ) {
		textVar[8] = enabledVar[8] = debugVar[8] = alphaVar[8] = char('0' + j);
		verbosityVar[8] = spatialVar[8] = locationXVar[8] = locationYVar[8] = char('0' + j);

		const SubtitleMatch &m = subtitleSlots[j];
		const Subtitle *sub = m.subtitle;

		// update enabled flag
		const char *enabled = (sub ? "1" : "0");
		if ( idStr::Cmp( GetStateString( enabledVar ), enabled ) )
			SetStateString( enabledVar, enabled );
		if (!sub)
			continue;	// if disabled, other properties don't matter

		// update text
		if ( idStr::Cmp( GetStateString( textVar ), sub->text.c_str() ) )
			SetStateString( textVar, sub->text.c_str() );

		SetStateInt( verbosityVar, int( m.verbosity ) );

		// update alpha according to volume
		float alpha = ComputeSubtitleAlphaForVolume( m.volume );
		SetStateFloat( alphaVar, alpha );

		// update direction cue
		idVec2 normalizedLocation = idVec2( m.spatializedDirection.x, m.spatializedDirection.y ) / tdm_subtitles_ringRadius.GetFloat();
		if ( normalizedLocation.Length() > 1.0f )
			normalizedLocation.Normalize();
		SetStateString( spatialVar, ( m.spatializedDirection.Length() > 0.0f && cv_tdm_subtitles_ring.GetBool() ? "1" : "0" ) );
		SetStateFloat( locationXVar, normalizedLocation.x );
		SetStateFloat( locationYVar, normalizedLocation.y );

		// update debug text
		idStr debugMessage;
		if ( tdm_subtitles_debug.GetBool() ) {
			idHashFunction<const void*> hashFunc;
			// (yeah, this is a mess, but it's OK only for debug purposes)
			sprintf( debugMessage,
				"[%04x %04x %04x %04x] V%5.3f (%5.2f %5.2f %5.2f) : %s '%s'",
				hashFunc( m.emitter ) >> 16, hashFunc( m.channel ) >> 16, hashFunc( m.sample ) >> 16, hashFunc( m.subtitle ) >> 16,
				m.volume,
				m.spatializedDirection.x, m.spatializedDirection.y, m.spatializedDirection.z,
				( m.verbosity == SUBL_STORY ? "story" : m.verbosity == SUBL_SPEECH ? "speech" : "effect" ), m.sample->name.c_str()
			);
		}
		SetStateString( debugVar, debugMessage.c_str() );
	}
}
