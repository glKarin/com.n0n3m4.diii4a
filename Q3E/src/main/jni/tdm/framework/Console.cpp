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



void SCR_DrawTextLeftAlign ( int &y, const char *text, ... ) id_attribute((format(printf,2,3)));
void SCR_DrawTextRightAlign( int &y, const char *text, ... ) id_attribute((format(printf,2,3)));

#define	NUM_CON_TIMES			4 // used for printing when the console is up (con_noPrint 0)
#define CONSOLE_FIRSTREPEAT		200 // delay before initial key repeat
#define CONSOLE_REPEAT			100 // delay between repeats - i.e typematic rate

idCVarBool con_legacyFont( "con_legacyFont", "0", CVAR_SYSTEM | CVAR_ARCHIVE, "0 - new 2.08 font, 1 - old D3 font" ); // grayman - add archive
idCVar con_fontSize( "con_fontSize", "8", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER, "font width in screen units (640x480)", 3.0f, 30.0f );
idCVar con_fontColor( "con_fontColor", "5", CVAR_SYSTEM | CVAR_ARCHIVE, "console color, 5 = cyan, 7 = white, 'r g b' = custom" );

struct idConsoleLine : idStr {
	idStr colorCodes;
	bool wrapped = false;

	idConsoleLine() {
	}
	idConsoleLine( int size ) {
		Fill( ' ', size );
		colorCodes.Fill( 0, size );
	}

	void Set( int index, char ch, char colorCode ) {
		data[index] = ch;
		colorCodes[index] = colorCode;
	}
};

// the console will query the cvar and command systems for
// command completion information

class idConsoleLocal : public idConsole {
public:
	virtual	void		Init( void ) override;
	virtual void		Shutdown( void ) override;
	virtual	void		LoadGraphics( void ) override;
	virtual	bool		ProcessEvent( const sysEvent_t *event, bool forceAccept ) override;
	virtual	bool		Active( void ) override;
	virtual	void		ClearNotifyLines( void ) override;
	virtual	void		Close( void ) override;
	virtual void		Open( const float frac ) override;
	virtual	void		Print( const char *text ) override;
	virtual	void		Draw( bool forceFullScreen ) override;

	// #3947: Add an optional "unwrap" keyword to Dump() that causes full lines to be continued by
	// the succeeding line without a line break. It's not possible to recover where the original line 
	// breaks were from the console text, but this optional keyword will fix the problem of file paths 
	// being broken up in the output.  
	void				Dump( const char *toFile, bool unwrap, bool modsavepath );
	void				Clear();

	//============================

	const idMaterial *	charSetOldMaterial;
	const idMaterial *	charSet24Material;
	const idMaterial *	charSetMaterial() { return con_legacyFont ? charSetOldMaterial : charSet24Material; }

private:
	void				KeyDownEvent( int key );

	void				Linefeed();

	void				PageUp();
	void				PageDown();
	void				Top();
	void				Bottom();

	void				DrawInput();
	void				DrawNotify();
	void				DrawSolidConsole( float frac );

	void				Scroll();
	void				SetDisplayFraction( float frac );
	void				UpdateDisplayFraction( void );

    virtual void		SaveHistory() override;
    virtual void		LoadHistory() override;

	//============================

	bool				keyCatching;

	idList<idConsoleLine>text;
	int					x;				// offset in current line for next print
	int					display;		// bottom of console displays this line
	int					lastKeyEvent;	// time of last key event for scroll delay
	int					nextKeyEvent;	// keyboard repeat rate

	float				displayFrac;	// approaches finalFrac at scr_conspeed
	float				finalFrac;		// 0.0 to 1.0 lines of console to display
	int					fracTime;		// time of last displayFrac update

	int					vislines;		// in scanlines

	int					times[NUM_CON_TIMES];	// cls.realtime time the line was generated

	idVec4				color;

	idStrList			historyStrings;

	int					historyLine;	// the line being displayed from history list

	idEditField			consoleField;

	idSysMutex			printMutex;		// lock the thread-unsafe console buffer

	static idCVar		con_speed;
	static idCVar		con_notifyTime;
	static idCVar		con_noPrint;

	const idMaterial *	whiteShader;
	const idMaterial *	consoleShader;

};

static idConsoleLocal localConsole;
idConsole	*console = &localConsole;

#ifdef NDEBUG
static const char *con_noPrint_defaultValue = "1";
#else
static const char *con_noPrint_defaultValue = "0";
#endif

idCVar idConsoleLocal::con_speed( "con_speed", "3", CVAR_SYSTEM, "speed at which the console moves up and down" );
idCVar idConsoleLocal::con_notifyTime( "con_notifyTime", "3", CVAR_SYSTEM, "time messages are displayed onscreen when console is pulled up" );
idCVar idConsoleLocal::con_noPrint( "con_noPrint", con_noPrint_defaultValue, CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "print on the console but not onscreen when console is pulled up");
idCVarBool con_noWrap( "con_noWrap", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "no wrap; long string to be truncated" );

/*
=============================================================================

	Misc stats

=============================================================================
*/

/*
==================
SCR_DrawTextLeftAlign
==================
*/
void SCR_DrawTextLeftAlign( int &y, const char *text, ... ) {
	char string[MAX_STRING_CHARS];
	va_list argptr;

	va_start( argptr, text );
	idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );

	renderSystem->DrawSmallStringExt( 0, y + 2, string, colorWhite, true, localConsole.charSetMaterial() );
	y += SMALLCHAR_HEIGHT + 4;
}

/*
==================
SCR_DrawTextRightAlign
==================
*/
void SCR_DrawTextRightAlign( int &y, const char *text, ... ) {
	char string[MAX_STRING_CHARS];
	va_list argptr;

	va_start( argptr, text );
	const int i = idStr::vsnPrintf( string, sizeof( string ), text, argptr );
	va_end( argptr );

	renderSystem->DrawSmallStringExt( SCREEN_WIDTH - i * SMALLCHAR_WIDTH, y + 2, string, colorWhite, true, localConsole.charSetMaterial() );
	y += SMALLCHAR_HEIGHT + 4;
}

/*
==================
SCR_DrawFPS
==================
*/
int showFPS_currentValue;	//for automation
int SCR_DrawFPS( int y ) {
	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	static int previous;
	int t = Sys_Milliseconds();
	int frameTime = t - previous;
	previous = t;

	static int previousTimes[128], index;
	int avgCnt = com_showFPSavg.GetInteger();
	previousTimes[index % avgCnt] = frameTime;
	index++;

	static int prevAvgCnt = 0;
	if (prevAvgCnt != avgCnt) {
		// com_showFPSavg was changed --- clear history
		prevAvgCnt = avgCnt;
		for (int i = 0; i < avgCnt; i++)
			previousTimes[i] = -1000000;
	}

	// average multiple frames together to smooth changes out a bit
	int total = 0;
	for (int i = 0; i < avgCnt; i++)
		total += previousTimes[i];
	int fps = (1000 * avgCnt) / idMath::Imax(total, 1);
	if (total < 0)
		fps = -1;

	// avoid quick switching between close values when com_showFPSavg is large
	static int lastFpsDisplayed = -666;
	static int lastFpsUpdate = -666;
	if (index - lastFpsUpdate >= avgCnt / 2 || idMath::Fabs(lastFpsDisplayed - fps) >= idMath::Fmax(1.5f, fps * (1.0f / 32.0f))) {
		lastFpsUpdate = index;
		lastFpsDisplayed = fps;
	}
	else {
		// revert to previous value for now
		fps = lastFpsDisplayed;
	}

	char *s = va("%ifps", fps);
	showFPS_currentValue = fps;

    renderSystem->DrawBigStringExt(SCREEN_WIDTH - static_cast<int>(strlen(s)*BIGCHAR_WIDTH), y + 2, s, colorWhite, true, localConsole.charSetMaterial());

	return (y + BIGCHAR_HEIGHT + 4);
}

/*
==================
SCR_DrawMemoryUsage
==================
*/
int SCR_DrawMemoryUsage( int y ) {
	memoryStats_t allocs, frees;
	
	Mem_GetStats( allocs );
	SCR_DrawTextRightAlign( y, "total allocated memory: %4d, %4zukB", allocs.num, allocs.totalSize>>10 );

	Mem_GetFrameStats( allocs, frees );
	SCR_DrawTextRightAlign( y, "frame alloc: %4d, %4zukB  frame free: %4d, %4zukB", allocs.num, allocs.totalSize>>10, frees.num, frees.totalSize>>10 );

	Mem_ClearFrameStats();

	return y;
}

/*
==================
SCR_DrawSoundDecoders
==================
*/
int SCR_DrawSoundDecoders( int y ) {
	unsigned int localTime, sampleTime, percent;
	int index = -1;
	int numActiveDecoders = 0;
	soundDecoderInfo_t decoderInfo;

	while( ( index = soundSystem->GetSoundDecoderInfo( index, decoderInfo ) ) != -1 ) {
		localTime = decoderInfo.current44kHzTime - decoderInfo.start44kHzTime;
		sampleTime = decoderInfo.num44kHzSamples / decoderInfo.numChannels;

		if ( localTime > sampleTime ) {
			if ( decoderInfo.looping ) {
				percent = ( localTime % sampleTime ) * 100 / sampleTime;
			} else {
				percent = 100;
			}
		} else {
			percent = localTime * 100 / sampleTime;
		}
		SCR_DrawTextLeftAlign( y, "%2d:%c%3d%% (%1.2f) %s: %s (%dkB)",
			numActiveDecoders, (decoderInfo.looping ? 'L' : ' '), percent, decoderInfo.lastVolume, 
			decoderInfo.format.c_str(), decoderInfo.name.c_str(), decoderInfo.numBytes >> 10 );
		numActiveDecoders++;
	}

	return y;
}

//=========================================================================

/*
==============
Con_Clear_f
==============
*/
static void Con_Clear_f( const idCmdArgs &args ) {
	localConsole.Clear();
}

/*
==============
Con_Dump_f
==============
*/
static void Con_Dump_f( const idCmdArgs &args ) {
	// #3947: added the "unwrap" logic. See declaration of idConsoleLocal::Dump. 
	// #5369: added "modsavepath" flag: save file in FM directory

	bool badargs = false, unwrap = false, modsavepath = false;
	const char *filename = 0;

	const int argc = args.Argc();
	for (int i = 1; i < argc; i++) {
		const char *arg = args.Argv( i );
		if ( idStr::Icmp( arg, "unwrap" ) == 0 ) {
			unwrap = true;
		} else if ( idStr::Icmp( arg, "modsavepath" ) == 0 ) {
			modsavepath = true;
		} else if (!filename) {
			filename = arg;
		} else {
			badargs = true;
		}
	}
	if (!filename)
		badargs = true;

	if ( badargs ) 
	{
		common->Printf( "usage: conDump [unwrap] <filename>\n\nunwrap prevents line breaks being added "
			"to the dump for full lines in the\nconsole. Fix for long file paths being broken up in the output.\n" );
		return;
	}

	idStr fileName = filename;
	fileName.DefaultFileExtension(".txt");

	common->Printf( "Dumped console text to %s.\n", fileName.c_str() );

	localConsole.Dump( fileName.c_str(), unwrap, modsavepath );
}

/*
==============
idConsoleLocal::Init
==============
*/
void idConsoleLocal::Init( void ) {
	keyCatching = false;

	lastKeyEvent = -1;
	nextKeyEvent = CONSOLE_FIRSTREPEAT;

	consoleField.Clear();
	consoleField.SetWidthInChars( SCREEN_WIDTH / SMALLCHAR_WIDTH - 1 );

	cmdSystem->AddCommand( "clear", Con_Clear_f, CMD_FL_SYSTEM, "clears the console" );
	cmdSystem->AddCommand( "conDump", Con_Dump_f, CMD_FL_SYSTEM, "dumps the console text to a file" );

	text.SetGranularity( 1024 );
}

/*
==============
idConsoleLocal::Shutdown
==============
*/
void idConsoleLocal::Shutdown( void ) {
	cmdSystem->RemoveCommand( "clear" );
	cmdSystem->RemoveCommand( "conDump" );
}

/*
==============
LoadGraphics

Can't be combined with init, because init happens before
the renderSystem is initialized
==============
*/
void idConsoleLocal::LoadGraphics() {
	charSetOldMaterial = declManager->FindMaterial( "textures/consolefont" );
	charSet24Material = declManager->FindMaterial( "textures/consolefont_24" );
	whiteShader = declManager->FindMaterial( "_white" );
	consoleShader = declManager->FindMaterial( "console" );
}

/*
================
idConsoleLocal::Active
================
*/
bool idConsoleLocal::Active( void ) {
	return keyCatching;
}

/*
================
idConsoleLocal::ClearNotifyLines
================
*/
void idConsoleLocal::ClearNotifyLines() {
	for ( int i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		times[i] = 0;
	}
}

/*
================
idConsoleLocal::Close
================
*/
void idConsoleLocal::Close() {
	keyCatching = false;
	SetDisplayFraction( 0 );
	displayFrac = 0;	// don't scroll to that point, go immediately

	ClearNotifyLines();
}

/*
================
idConsoleLocal::Open
================
*/
void idConsoleLocal::Open(const float frac) {
	consoleField.Clear();
	keyCatching = true;
	SetDisplayFraction( frac );
	displayFrac = frac;	// don't scroll to that point, go immediately
}

/*
================
idConsoleLocal::Clear
================
*/
void idConsoleLocal::Clear() {
	text.Clear();
	Bottom();		// go to end
}

/*
================
idConsoleLocal::Dump

Save the console contents out to a file
================
*/
void idConsoleLocal::Dump( const char *fileName, bool unwrap, bool modsavepath ) {
	int		l;
	idFile	*f;
	
	if (modsavepath) {
		f = fileSystem->OpenFileWrite( fileName, "fs_modSavePath" );
	} else {
		f = fileSystem->OpenFileWrite( fileName, "fs_devpath", "" );
	}
	if ( !f ) {
		common->Warning( "Couldn't open %s", fileName );
		return;
	}

	// write the remaining lines
	idStr prevWrappedLine;
	for ( l = 0; l < text.Num(); l++ ) {
		auto& line = text[l];
		idStr s = line;
		s.StripTrailingWhitespace();
		if ( unwrap ) {
			if ( prevWrappedLine.Length() ) {
				s.StripLeadingWhitespace();
				s = prevWrappedLine + ' ' + s;
			}
			if ( line.wrapped ) {
				prevWrappedLine = s;
			} else {
				f->Write( s.c_str(), s.Length() );
				f->Write( "\r\n", 2 );
				prevWrappedLine = "";
			}
		} else {
			f->Write( line.c_str(), line.Length() );
			f->Write( "\r\n", 2 );
		}
	}

	fileSystem->CloseFile( f );
}

void idConsoleLocal::SaveHistory()
{
    idFile *f = fileSystem->OpenFileWrite("consolehistory.dat");
	auto startIndex = idMath::Imax( 0, historyStrings.Num() - 100 );
	for ( int i = startIndex; i < historyStrings.Num(); i++ ) {
		const char *s = historyStrings[i].c_str();
		if ( s && s[0] )
			f->WriteString( s );
	}
    fileSystem->CloseFile(f);
}

void idConsoleLocal::LoadHistory()
{
    idFile *f = fileSystem->OpenFileRead("consolehistory.dat");
    if (f == NULL) // file doesn't exist
        return;

    idStr tmp;
	while ( 1 ) {
		if ( f->Tell() >= f->Length() ) {
			break; // EOF is reached
		}
		f->ReadString( tmp );
		historyStrings.AddUnique( tmp );
	}
	historyLine = historyStrings.Num();
    fileSystem->CloseFile(f);
}

/*
================
idConsoleLocal::PageUp
================
*/
void idConsoleLocal::PageUp( void ) {
	display -= 2;
	if ( display < 0 ) {
		display = 0;
	}
}

/*
================
idConsoleLocal::PageDown
================
*/
void idConsoleLocal::PageDown( void ) {
	display += 2;
	if ( display > ( text.Num() - 1 ) ) {
		display = ( text.Num() - 1 );
	}
}

/*
================
idConsoleLocal::Top
================
*/
void idConsoleLocal::Top( void ) {
	display = 0;
}

/*
================
idConsoleLocal::Bottom
================
*/
void idConsoleLocal::Bottom( void ) {
	display = text.Num() - 1;
	if ( display < 0 ) {
		display = 0;
	}
}


/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

/*
====================
KeyDownEvent

Handles history and console scrollback
====================
*/
void idConsoleLocal::KeyDownEvent( int key ) {
	
	// Execute F key bindings
	if ( key >= K_F1 && key <= K_F12 ) {
		idKeyInput::ExecKeyBinding( key );
		return;
	}

	// ctrl-L clears screen
	if ( key == 'l' && idKeyInput::IsDown( K_CTRL ) ) {
		Clear();
		return;
	}

	// enter finishes the line
	if ( key == K_ENTER || key == K_KP_ENTER ) {

		common->Printf ( "]%s\n", consoleField.GetBuffer() );

		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, consoleField.GetBuffer() ); // valid command
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );

		// add line to history list or swap if exists
		idStr s = consoleField.GetBuffer();
		int index = historyStrings.FindIndex( s );
		if( index >= 0 )
			historyStrings.RemoveIndex( index );
		historyStrings.Append( s );
		historyLine = historyStrings.Num();

		consoleField.Clear();
		consoleField.SetWidthInChars( SCREEN_WIDTH / SMALLCHAR_WIDTH - 1 );

		session->UpdateScreen(); // force an update, because the command may take some time

		return;
	}

	// command completion
	if ( key == K_TAB ) {
		consoleField.AutoComplete();
		return;
	}

	// command history (ctrl-p ctrl-n for unix style, scroll up/down command history)
	if ( key == K_UPARROW || ( key == 'p' && idKeyInput::IsDown( K_CTRL ) ) ) {
		if ( historyLine > 0 ) {
			historyLine--;
			consoleField.SetBuffer( historyStrings[historyLine] );
		}
		return;
	}

	if ( key == K_DOWNARROW || ( key == 'n' && idKeyInput::IsDown( K_CTRL ) ) ) {
		if ( historyLine > historyStrings.Num()-2 )
			return;
		historyLine++;
		consoleField.SetBuffer( historyStrings[historyLine] );
		return;
	}

	// console scrolling
	if ( key == K_PGDN ) {
		PageDown();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if ( key == K_PGUP ) {
		PageUp();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if ( key == K_MWHEELDOWN ) {
		PageDown();
		return;
	}

	if ( key == K_MWHEELUP ) {
		PageUp();
		return;
	}

	// ctrl-home = top of console
	if ( key == K_HOME && idKeyInput::IsDown( K_CTRL ) ) {
		Top();
		return;
	}

	// ctrl-end = bottom of console
	if ( key == K_END && idKeyInput::IsDown( K_CTRL ) ) {
		Bottom();
		return;
	}

	// pass to the normal editline routine
	consoleField.KeyDownEvent( key );
}

/*
==============
Scroll
deals with scrolling text because we don't have key repeat
duzenko #4409 - fix the last/next keyevnt system for variable frame time
==============
*/
void idConsoleLocal::Scroll( ) {

	if (lastKeyEvent == -1 || (lastKeyEvent + nextKeyEvent) > eventLoop->Milliseconds()) {
		return;
	}

	// console scrolling
	else if ( idKeyInput::IsDown( K_PGUP ) ) {
		PageUp();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}

	else if ( idKeyInput::IsDown( K_PGDN ) ) {
		PageDown();
		lastKeyEvent = eventLoop->Milliseconds();
		nextKeyEvent = CONSOLE_REPEAT;
		return;
	}
}

/*
==============
SetDisplayFraction

Causes the console to start opening the desired amount.
==============
*/
void idConsoleLocal::SetDisplayFraction( float frac ) {
	finalFrac = frac;
	fracTime = com_frameTime;
}

/*
==============
UpdateDisplayFraction

Scrolls the console up or down based on conspeed
==============
*/
void idConsoleLocal::UpdateDisplayFraction( void ) {
	const float consolespeed = con_speed.GetFloat();

	if ( consolespeed < 0.1f ) {
		fracTime = com_frameTime;
		displayFrac = finalFrac;
		return;
	} else if ( finalFrac < displayFrac ) { // scroll towards the destination height
		displayFrac -= consolespeed * ( com_frameTime - fracTime ) * 0.001f;
		if ( finalFrac > displayFrac ) {
			displayFrac = finalFrac;
		}
		fracTime = com_frameTime;
	} else if ( finalFrac > displayFrac ) {
		displayFrac += consolespeed * ( com_frameTime - fracTime ) * 0.001f;
		if ( finalFrac < displayFrac ) {
			displayFrac = finalFrac;
		}
		fracTime = com_frameTime;
	}
}

/*
==============
ProcessEvent
==============
*/
bool idConsoleLocal::ProcessEvent( const sysEvent_t *event, bool forceAccept ) {
	bool consoleKey;
	consoleKey = event->evType == SE_KEY && ( event->evValue == Sys_GetConsoleKey( false ) || event->evValue == Sys_GetConsoleKey( true ) );

#if ID_CONSOLE_LOCK
	// If the console's not already down, and we have it turned off, check for ctrl+alt
	if ( !keyCatching && !com_allowConsole.GetBool() && ( !idKeyInput::IsDown( K_CTRL ) || !idKeyInput::IsDown( K_ALT ) ) ) {
			consoleKey = false;
	}
#endif

	// we always catch the console key event
	if ( !forceAccept && consoleKey ) {
		// ignore up events
		if ( event->evValue2 == 0 ) {
			return true;
		}

		consoleField.ClearAutoComplete();

		// a down event will toggle the destination lines
		if ( keyCatching ) {
			Close();
			Sys_GrabMouseCursor( true );
			cvarSystem->SetCVarBool( "ui_chat", false );
		} else {
			consoleField.Clear();
			keyCatching = true;
			if ( idKeyInput::IsDown( K_SHIFT ) ) {
				if ( idKeyInput::IsDown( K_CTRL ) ) 
					SetDisplayFraction( 0.8f );
				else
					// if the shift key is down, don't open the console as much
					SetDisplayFraction( 0.2f );
			} else {
				SetDisplayFraction( 0.5f );
			}
			cvarSystem->SetCVarBool( "ui_chat", true );
		}
		return true;
	}

	// if we aren't key catching, dump all the other events
	if ( !forceAccept && !keyCatching ) {
		return false;
	}

	// handle key and character events
	if ( event->evType == SE_CHAR ) {
		// never send the console key as a character
		if ( event->evValue != Sys_GetConsoleKey( false ) && event->evValue != Sys_GetConsoleKey( true ) ) {
			consoleField.CharEvent( event->evValue );
		}
		return true;
	}

	if ( event->evType == SE_KEY ) {
		// ignore up key events
		if ( event->evValue2 == 0 ) {
			return true;
		} else {
			KeyDownEvent( event->evValue );
			return true;
		}
	}

	// we don't handle things like mouse, joystick, and network packets
	return false;
}

/*
==============================================================================

PRINTING

==============================================================================
*/

/*
===============
Linefeed
===============
*/
void idConsoleLocal::Linefeed() {
	// mark time for transparent overlay
	if ( text.Num() > 0 ) {
		times[( text.Num() - 1 ) % NUM_CON_TIMES] = com_frameTime;
	}

	x = 0;
	idConsoleLine s( SCREEN_WIDTH / SMALLCHAR_WIDTH - 1 );
	text.Append( s );

	if ( display == text.Num() - 2 ) {
		display++;
	}
}


/*
================
Print

Handles cursor positioning, line wrapping, etc
================
*/
void idConsoleLocal::Print( const char *txt ) {
	int		c, l;
	int		color;

	idScopedCriticalSection lock(printMutex);

#ifdef ID_ALLOW_TOOLS
	RadiantPrint( txt );

	if( com_editors & EDITOR_MATERIAL ) {
		MaterialEditorPrintConsole(txt);
	}
#endif

	if ( text.Num() == 0 )
		Linefeed();

	color = idStr::ColorIndex( C_COLOR_DEFAULT );

	while ( (c = *(const unsigned char*)txt) != 0 ) {
		if ( idStr::IsColor( txt ) ) {
			color = idStr::ColorIndex( *(txt+1) );
			txt += 2;
			continue;
		}

#define currentLine (&text[text.Num() - 1])

		// if we are about to print a new word, check to see
		// if we should wrap to the new line
		if ( c > ' ' && !con_noWrap ) {
			// count word length
			for (l=0; l<currentLine->Length(); l++) {
				if ( txt[l] <= ' ') {
					break;
				}
			}

			// word wrap
			if (l != currentLine->Length() && (x + l > currentLine->Length() ) ) {
				currentLine->wrapped = true;
				for ( ; x < currentLine->Length(); x++ )
					( *currentLine )[x] = 32;

				Linefeed();
			}
		}

		txt++;

		switch( c ) {
			case '\n':
				Linefeed ();
				break;
			case '\t':
				do {
					(*currentLine)[x] = ' ';
					x++;
					if ( x >= currentLine->Length() ) {
						Linefeed();
						x = 0;
					}
				} while ( x & 3 );
				break;
			case '\r':
				x = 0;
				break;
			default:	// display character and advance
				( *currentLine ).Set( x, c, color );
				x++;
				if ( x >= currentLine->Length() ) {
					if ( strcmp( txt, "\n" ) ) { // don't insert an empty line when txt is exactly LINE_WIDTH long and ends with a LF
						currentLine->wrapped = true;
						Linefeed();
					}
					x = 0;
				}
				break;
		}
		if ( con_noWrap && ( x == currentLine->Length() - 1 ) && txt[0] && txt[1] && ( txt[1] != '\n' ) ) { // don't truncate if exactly LINE_WIDTH long
			currentLine->Set( x, '>', C_COLOR_YELLOW );
			Linefeed();
			x = 0;
			while ( *txt && *txt != '\n' ) // only truncate until the next explicit line break
				txt++;
			if ( !*txt )
				break;
			else
				txt++;
		}
	}


	// mark time for transparent overlay
	if ( ( text.Num() - 1 ) >= 0 ) {
		times[( text.Num() - 1 ) % NUM_CON_TIMES] = com_frameTime;
	}
#undef currentLine
}


/*
==============================================================================

DRAWING

==============================================================================
*/

void SetColor(int i) {
	idVec4 c = idStr::ColorForIndex( i );
	if ( !i && ( con_fontColor.GetInteger() || idStr::Length( con_fontColor.GetString() ) > 1 ) ) {
		if ( sscanf( con_fontColor.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) != 3 ) {
			if ( con_fontColor.GetInteger() ) {
				c = idStr::ColorForIndex( con_fontColor.GetInteger() );
			}
		}
	}
	renderSystem->SetColor( c );
}

/*
================
DrawInput

Draw the editline after a ] prompt
================
*/
void idConsoleLocal::DrawInput() {

	const int y = vislines - ( SMALLCHAR_HEIGHT * 2 );

	if ( consoleField.GetAutoCompleteLength() != 0 ) {
        const int autoCompleteLength = static_cast<int>(strlen(consoleField.GetBuffer())) - consoleField.GetAutoCompleteLength();

		if ( autoCompleteLength > 0 ) {
			renderSystem->SetColor4( 0.8f, 0.2f, 0.2f, 0.45f );

			renderSystem->DrawStretchPic( 2 * SMALLCHAR_WIDTH + consoleField.GetAutoCompleteLength() * SMALLCHAR_WIDTH,
							y + 2, autoCompleteLength * SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT - 2, 0, 0, 0, 0, whiteShader );
		}
	}

	SetColor( idStr::ColorIndex( C_COLOR_DEFAULT ) );

	renderSystem->DrawSmallChar( 0.5 * SMALLCHAR_WIDTH, y, '>', charSetMaterial() );

	consoleField.Draw( 1.5 * SMALLCHAR_WIDTH, y, SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, true, charSetMaterial() );
}

/*
================
DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void idConsoleLocal::DrawNotify() {
	int		time;
	int		currentColor;

	if ( con_noPrint.GetBool() ) {
		return;
	}

	currentColor = idStr::ColorIndex( C_COLOR_WHITE );
	SetColor( currentColor );

	int v = 0;
	for ( int i = text.Num()-NUM_CON_TIMES; i < text.Num(); i++ ) {
		if ( i < 0 ) {
			continue;
		}
		time = times[i % NUM_CON_TIMES];
		if ( time == 0 ) {
			continue;
		}
		time = com_frameTime - time;
		if ( time > con_notifyTime.GetFloat() * 1000 ) {
			continue;
		}
		auto& text_p = text[i];
		
		for ( int x = 0; x < text_p.Length(); x++ ) {
			if ( ( text_p[x] ) == ' ' ) {
				continue;
			}
			if ( idStr::ColorIndex(text_p.colorCodes[x]) != currentColor ) {
				currentColor = idStr::ColorIndex(text_p.colorCodes[x]);
				SetColor( currentColor );
			}
			renderSystem->DrawSmallChar( (x+1)*SMALLCHAR_WIDTH, v, text_p[x], charSetMaterial() );
		}

		v += SMALLCHAR_HEIGHT;
	}

	SetColor( idStr::ColorIndex( C_COLOR_DEFAULT ) );
}

/*
================
DrawSolidConsole

Draws the console with the solid background
================
*/
void idConsoleLocal::DrawSolidConsole( float frac ) {
	int				x;
	float			y;
	int				row, rows;
	int				lines;
	int				currentColor;

	lines = idMath::FtoiRound( SCREEN_HEIGHT * frac );
	if ( lines <= 0 ) {
		return;
	}

	else if ( lines > SCREEN_HEIGHT ) {
		lines = SCREEN_HEIGHT;
	}

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if ( y < 1.0f ) {
		y = 0.0f;
	} else {
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, y, 0, 1.0f - displayFrac, 1, 1, consoleShader );
	}

	SetColor( idStr::ColorIndex( C_COLOR_DEFAULT ) );
	renderSystem->DrawStretchPic( 0, y, SCREEN_WIDTH, 0.25f * con_fontSize.GetInteger(), 0, 0, 0, 0, whiteShader );

	// draw the version number
	{
		// BluePill #4539 - show whether this is a 32-bit or 64-bit binary
		const idStr version = va("%s/%zu #%d", ENGINE_VERSION, sizeof(void*) * 8, RevisionTracker::Instance().GetHighestRevision());
		const int vlen = version.Length();

		for ( x = 0; x < vlen; x++ ) {
			renderSystem->DrawSmallChar( SCREEN_WIDTH - ( vlen - x ) * SMALLCHAR_WIDTH, 
				(lines-SMALLCHAR_HEIGHT), version[x], charSetMaterial() );
		}
	}


	// draw the text
	vislines = lines;
	rows = (lines - SMALLCHAR_HEIGHT) / SMALLCHAR_HEIGHT;		// rows of text to draw
	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if ( display != ( text.Num() - 1 ) ) {
		// draw arrows to show the buffer is backscrolled
		SetColor( idStr::ColorIndex( C_COLOR_DEFAULT ) );
		for ( x = 0; x < SCREEN_WIDTH / SMALLCHAR_WIDTH; x += 4 ) {
			renderSystem->DrawSmallChar( (x+1)*SMALLCHAR_WIDTH, idMath::FtoiRound( y ), '^', charSetMaterial() );
		}
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}
	
	row = (x == 0) ? (display -1) : display;

	currentColor = idStr::ColorIndex( C_COLOR_WHITE );
	SetColor( currentColor );

	for ( int i = 0; i < rows; i++, y -= SMALLCHAR_HEIGHT, row-- ) {
		if ( row < 0 || row >= text.Num() ) {
			break;
		}

		auto &text_p = text[row];

		for ( x = 0; x < text_p.Length(); x++ ) {
			if ( ( text_p[x] ) == ' ' ) {
				continue;
			}

			if ( idStr::ColorIndex(text_p.colorCodes[x]) != currentColor ) {
				currentColor = idStr::ColorIndex(text_p.colorCodes[x]);
				SetColor( currentColor );
			}
			renderSystem->DrawSmallChar( 0.5*SMALLCHAR_WIDTH + x*SMALLCHAR_WIDTH, idMath::FtoiRound( y ), text_p[x], charSetMaterial() );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	DrawInput();

	SetColor( idStr::ColorIndex( C_COLOR_DEFAULT ) );
}


/*
==============
Draw

ForceFullScreen is used by the editor
==============
*/
void idConsoleLocal::Draw( bool forceFullScreen ) {
#if 0
	//Is this even possible?
	if ( !charSetMaterial ) {
		return;
	}
#endif
	
	int y = 0; // Padding from the top of the screen for FPS display etc.

	if ( forceFullScreen ) {
		// if we are forced full screen because of a disconnect, 
		// we want the console closed when we go back to a session state
		Close();
		// we are however catching keyboard input
		keyCatching = true;
	}

	Scroll();

	UpdateDisplayFraction();

	if ( displayFrac ) {
		DrawSolidConsole( displayFrac );
	} else if ( forceFullScreen ) {
		DrawSolidConsole( 1.0f );
	} else if ( !con_noPrint.GetBool() ) {
		// draw the notify lines if the developer cvar is set (ie DEBUG build)
		DrawNotify();
	}

	if ( com_showFPS.GetBool() ) {
		y = SCR_DrawFPS( 4 ); // Initial padding from the top of the screen.
	}

	if ( com_showMemoryUsage.GetBool() ) {
		y = SCR_DrawMemoryUsage( y );
	}

	if ( com_showSoundDecoders.GetBool() ) {
		y = SCR_DrawSoundDecoders( y );
	}

}
