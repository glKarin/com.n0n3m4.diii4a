/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// console.c

#include "client.h"


int g_console_field_width = 78;

#ifdef __ANDROID__ //karin: limit console max height
extern float Android_GetConsoleMaxHeightFrac(float frac);
#endif

#define	NUM_CON_TIMES 4

#define		CON_TEXTSIZE	32768
typedef struct {
	qboolean	initialized;

	short	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vec4_t	color;
} console_t;

#define CONSOLE_ALL 0
#define CONSOLE_GENERAL 1
#define CONSOLE_KILLS 2
#define CONSOLE_HITS 3
#define CONSOLE_CHAT 4
#define CONSOLE_DEV 5

console_t consoles[6];

int currentConsoleNum = CONSOLE_ALL;
console_t *currentCon = &consoles[CONSOLE_ALL];

char *consoleNames[] = {
	"All",
	"General",
	"Kills",
	"Hits",
	"Chat",
	"Dev"
};
int numConsoles = 5;

cvar_t		*con_conspeed;
cvar_t		*con_notifytime;

#define	DEFAULT_CONSOLE_WIDTH	78

vec4_t	console_color = {1.0, 1.0, 1.0, 1.0};


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void) {
	// closing a full screen console restarts the demo loop
	if ( cls.state == CA_DISCONNECTED && cls.keyCatchers == KEYCATCH_CONSOLE ) {
		CL_StartDemoLoop();
		return;
	}

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify ();
	cls.keyCatchers ^= KEYCATCH_CONSOLE;
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void) {
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void) {
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_CROSSHAIR_PLAYER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_LAST_ATTACKER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void) {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		currentCon->text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}

	Con_Bottom();		// go to end
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int		l, x, i;
	short	*line;
	fileHandle_t	f;
	int		bufferlen;
	char	*buffer;
	char	filename[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".txt" );

	if (!COM_CompareExtension(filename, ".txt"))
	{
		Com_Printf("Con_Dump_f: Only the \".txt\" extension is supported by this command!\n");
		return;
	}

	f = FS_FOpenFileWrite( filename );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open %s.\n", filename);
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", filename );

	// skip empty lines
	for (l = consoles[CONSOLE_ALL].current - consoles[CONSOLE_ALL].totallines + 1 ; l <= consoles[CONSOLE_ALL].current ; l++)
	{
		line = consoles[CONSOLE_ALL].text + (l%consoles[CONSOLE_ALL].totallines)*consoles[CONSOLE_ALL].linewidth;
		for (x=0 ; x<consoles[CONSOLE_ALL].linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
				break;
		if (x != consoles[CONSOLE_ALL].linewidth)
			break;
	}

#ifdef _WIN32
	bufferlen = consoles[CONSOLE_ALL].linewidth + 3 * sizeof ( char );
#else
	bufferlen =  consoles[CONSOLE_ALL].linewidth + 2 * sizeof ( char );
#endif

	buffer = Hunk_AllocateTempMemory( bufferlen );

	// write the remaining lines
	buffer[bufferlen-1] = 0;
	for ( ; l <= consoles[CONSOLE_ALL].current ; l++)
	{
		line = consoles[CONSOLE_ALL].text + (l%consoles[CONSOLE_ALL].totallines)*consoles[CONSOLE_ALL].linewidth;
		for(i=0; i<consoles[CONSOLE_ALL].linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x=consoles[CONSOLE_ALL].linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
#ifdef _WIN32
		Q_strcat(buffer, bufferlen, "\r\n");
#else
		Q_strcat(buffer, bufferlen, "\n");
#endif
		FS_Write(buffer, strlen(buffer), f);
	}

	Hunk_FreeTempMemory( buffer );
	FS_FCloseFile( f );
}
						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int		i;
	
	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		currentCon->times[i] = 0;
	}
}

						

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (console_t *console)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	short	tbuf[CON_TEXTSIZE];

	width = (cls.glconfig.vidWidth * 0.8) / (SMALLCHAR_WIDTH - 1);

	if (width == console->linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = DEFAULT_CONSOLE_WIDTH;
		console->linewidth = width;
		console->totallines = CON_TEXTSIZE / console->linewidth;
		for(i=0; i<CON_TEXTSIZE; i++)
			console->text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}
	else
	{
		oldwidth = console->linewidth;
		console->linewidth = width;
		oldtotallines = console->totallines;
		console->totallines = CON_TEXTSIZE / console->linewidth;
		numlines = oldtotallines;

		if (console->totallines < numlines)
			numlines = console->totallines;

		numchars = oldwidth;
	
		if (console->linewidth < numchars)
			numchars = console->linewidth;

		Com_Memcpy (tbuf, console->text, CON_TEXTSIZE * sizeof(short));
		for(i=0; i<CON_TEXTSIZE; i++)

			console->text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';


		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				console->text[(console->totallines - 1 - i) * console->linewidth + j] =
						tbuf[((console->current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	console->current = console->totallines - 1;
	console->display = console->current;
}

void Con_PrevTab() {
	currentConsoleNum--;
	if (currentConsoleNum < 0)
		currentConsoleNum = numConsoles - 1;
	currentCon = &consoles[currentConsoleNum];
}

void Con_NextTab() {
	currentConsoleNum++;
	if (currentConsoleNum == numConsoles)
		currentConsoleNum = CONSOLE_ALL;
	currentCon = &consoles[currentConsoleNum];
}


/*
================
Con_Init
================
*/
void Con_Init (void) {
	int		i;

	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);
	con_conspeed = Cvar_Get ("scr_conspeed", "3", 0);

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_console_field_width;
	}
	CL_LoadConsoleHistory( );

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("messagemode3", Con_MessageMode3_f);
	Cmd_AddCommand ("messagemode4", Con_MessageMode4_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f);
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (console_t *console, qboolean skipnotify)
{
	int		i;

	// mark time for transparent overlay
	if (console->current >= 0)
	{
	if (skipnotify)
		  console->times[console->current % NUM_CON_TIMES] = 0;
	else
		  console->times[console->current % NUM_CON_TIMES] = cls.realtime;
	}

	console->x = 0;
	if (console->display == console->current)
		console->display++;
	console->current++;
	for(i=0; i<console->linewidth; i++)
		console->text[(console->current%console->totallines)*console->linewidth+i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
}




void writeTextToConsole(console_t *console, char *txt, qboolean skipnotify) {
	int		y;
	int		c, l;
	int		color;
	int prev;							// NERVE - SMF

	color = ColorIndex(COLOR_WHITE);

	while ( (c = *txt) != 0 ) {
		if ( Q_IsColorString( txt ) ) {
			color = ColorIndex( *(txt+1) );
			txt += 2;
			continue;
		}

		// count word length
		for (l=0 ; l< console->linewidth ; l++) {
			if ( txt[l] <= ' ') {
				break;
			}

		}

		// word wrap
		if (l != console->linewidth && (console->x + l >= console->linewidth) ) {
			Con_Linefeed(console, skipnotify);

		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed (console, skipnotify);
			break;
		case '\r':
			console->x = 0;
			break;
		default:	// display character and advance
			y = console->current % console->totallines;
			console->text[y*console->linewidth+console->x] = (color << 8) | c;
			console->x++;
			if (console->x >= console->linewidth) {
				Con_Linefeed(console, skipnotify);
				console->x = 0;
			}
			break;
		}
	}

	// mark time for transparent overlay
	if (console->current >= 0) {
		// NERVE - SMF
		if ( skipnotify ) {
			prev = console->current % NUM_CON_TIMES - 1;
			if ( prev < 0 )
				prev = NUM_CON_TIMES - 1;
			console->times[prev] = 0;
		}
		else
		// -NERVE - SMF
			console->times[console->current % NUM_CON_TIMES] = cls.realtime;
	}
}

void CL_DevConsolePrint(char *txt) {
	int i;
	qboolean skipnotify = qfalse;
	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}
	
	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}

	for (i = 0; i < numConsoles + 1; i++) {
		if (!consoles[i].initialized) {
			consoles[i].color[0] = 
			consoles[i].color[1] = 
			consoles[i].color[2] =
			consoles[i].color[3] = 1.0f;
			consoles[i].linewidth = -1;
			Con_CheckResize (&consoles[i]);
			consoles[i].initialized = qtrue;
		}
	}

	writeTextToConsole(&consoles[CONSOLE_DEV], txt, skipnotify);
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint( char *txt ) {
	int i;
	qboolean isKill = qfalse;
	qboolean isHit = qfalse;
	qboolean isChat = qfalse;

	qboolean skipnotify = qfalse; // NERVE - SMF
	
	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}
	
	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}

	for (i = 0; i < numConsoles; i++) {
		if (!consoles[i].initialized) {
			consoles[i].color[0] = 
			consoles[i].color[1] = 
			consoles[i].color[2] =
			consoles[i].color[3] = 1.0f;
			consoles[i].linewidth = -1;
			Con_CheckResize (&consoles[i]);
			consoles[i].initialized = qtrue;
		}
	}

	if (txt[0] == 17) {
		isKill = qtrue;
		txt++;
	} else if (txt[0] == 18) {
		isHit = qtrue;
		txt++;
	} else if (txt[0] == 19) {
		isChat = qtrue;
		txt++;
	}

	writeTextToConsole(&consoles[CONSOLE_ALL], txt, skipnotify);

	if (isKill) {
		writeTextToConsole(&consoles[CONSOLE_KILLS], txt, skipnotify);
		writeTextToConsole(&consoles[CONSOLE_HITS], txt, skipnotify);
	} else if (isHit) {
		writeTextToConsole(&consoles[CONSOLE_HITS], txt, skipnotify);
	} else if (isChat) {
		writeTextToConsole(&consoles[CONSOLE_CHAT], txt, skipnotify);
	} else {
		writeTextToConsole(&consoles[CONSOLE_GENERAL], txt, skipnotify);
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput (void) {
	int		y;

	if ( cls.state != CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = currentCon->vislines - ( SMALLCHAR_HEIGHT * 2 );

	re.SetColor( currentCon->color );

	SCR_DrawSmallChar( currentCon->xadjust + 1 * (SMALLCHAR_WIDTH - 1), y, ']' );

	Field_Draw( &g_consoleField, currentCon->xadjust + 2 * (SMALLCHAR_WIDTH - 1), y,
		SCREEN_WIDTH - 3 * (SMALLCHAR_WIDTH - 1), qtrue );
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	short	*text;
	int		i;
	int		time;
	int		skip;
	int		currentColor;

	currentColor = 7;
	re.SetColor( g_color_table[currentColor] );

	v = 0;
	for (i= currentCon->current-NUM_CON_TIMES+1 ; i<=currentCon->current ; i++)
	{
		if (i < 0)
			continue;
		time = currentCon->times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = currentCon->text + (i % currentCon->totallines)*currentCon->linewidth;

		if (cl.snap.ps.pm_type != PM_INTERMISSION && (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			continue;
		}

		for (x = 0 ; x < currentCon->linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}
			if ( ( (text[x]>>8)&7 ) != currentColor ) {
				currentColor = (text[x]>>8)&7;
				re.SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar( cl_conXOffset->integer + currentCon->xadjust + (x+1)*(SMALLCHAR_WIDTH - 1), v, text[x] & 0xff );
		}

		v += SMALLCHAR_HEIGHT;
	}

	re.SetColor( NULL );

	if (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
		return;
	}

	// draw the chat line
	if ( cls.keyCatchers & KEYCATCH_MESSAGE )
	{
		if (chat_team)
		{
			SCR_DrawBigString (8, v, "say_team:", 1.0f );
			skip = 10;
		}
		else
		{
			SCR_DrawBigString (8, v, "say:", 1.0f );
			skip = 5;
		}

		Field_BigDraw( &chatField, skip * BIGCHAR_WIDTH, v,
			SCREEN_WIDTH - ( skip + 1 ) * BIGCHAR_WIDTH, qtrue );

		v += BIGCHAR_HEIGHT;
	}

}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( float frac ) {
	int				i, x, y;
	int				rows;
	short			*text;
	int				row;
	int				lines;
	int				currentColor;
	vec4_t			color = {1, 0, 0, 1};
	vec4_t          bgColor = {0, 0, 0, 0.85f};
	vec4_t			darkTextColour = {0.25f, 0.25f, 0.25f, 1.0f};
	int totalOffset = 0;

	lines = cls.glconfig.vidHeight * frac;
	if (lines <= 0)
		return;

	if (lines > cls.glconfig.vidHeight )
		lines = cls.glconfig.vidHeight;

	// on wide screens, we will center the text
	currentCon->xadjust = 0;
	SCR_AdjustFrom640( &currentCon->xadjust, NULL, NULL, NULL );

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if ( y < 1 ) {
		y = 0;
	}
	else {
		SCR_FillRect(0, 0, SCREEN_WIDTH, y, bgColor);
	}

	int vertOffset;
	int horizOffset;
	horizOffset = 0;
	vertOffset = y;
	int tabWidth;
	int tabHeight;
	float old;

	for (i = 0; i < numConsoles; i++) {
		if (currentCon == &consoles[i]) {
			tabWidth = SCR_FontWidth(consoleNames[i], 0.24f) + 30;
			tabHeight = 22;
			color[3] = 1;
		} else {
			tabWidth = SCR_FontWidth(consoleNames[i], 0.18f) + 18;
			tabHeight = 18;
			color[3] = 0.2;
		}

		// tab background
		if (i)
			SCR_FillRect(horizOffset + 1, vertOffset, tabWidth - 1, tabHeight, bgColor);
		else
			SCR_FillRect(horizOffset, vertOffset, tabWidth, tabHeight, bgColor);

		if (currentCon != &consoles[i]) {
			old = color[3];
			color[3] = 1;

			// top border
			SCR_FillRect(horizOffset, vertOffset, tabWidth, 1, color);
			color[3] = old;
		}

		// bottom border
		SCR_FillRect(horizOffset, vertOffset + tabHeight - 1, tabWidth, 1, color);

		// left border
		if (i && currentCon == &consoles[i])
			SCR_FillRect(horizOffset, vertOffset, 1, tabHeight, color);

		// right border
		SCR_FillRect(horizOffset + tabWidth, vertOffset, 1, tabHeight, color);

		if (currentCon == &consoles[i]) {
			SCR_DrawFontText(horizOffset + 15, vertOffset + 14, 0.24f, g_color_table[7], consoleNames[i], ITEM_TEXTSTYLE_SHADOWED);
		} else {
			SCR_DrawFontText(horizOffset + 9, vertOffset + 12, 0.18f, darkTextColour, consoleNames[i], ITEM_TEXTSTYLE_SHADOWED);
		}

		horizOffset += tabWidth;
	}


	// The little bottom border
	color[3] = 1;
	totalOffset = horizOffset;
	SCR_FillRect(totalOffset, y, SCREEN_WIDTH - totalOffset, 1, color);


	// draw the version number

	re.SetColor( g_color_table[ColorIndex(COLOR_RED)] );

	i = strlen( SVN_VERSION );

	for (x=0 ; x<i ; x++) {

		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * (SMALLCHAR_WIDTH - 1), 

			(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)), SVN_VERSION[x] );

	}


	// draw the text
	currentCon->vislines = lines;
	rows = (lines-(SMALLCHAR_WIDTH - 1))/(SMALLCHAR_WIDTH - 1);		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if (currentCon->display != currentCon->current)
	{
	// draw arrows to show the buffer is backscrolled
		re.SetColor( g_color_table[ColorIndex(COLOR_RED)] );
		for (x=0 ; x<currentCon->linewidth ; x+=4)
			SCR_DrawSmallChar( currentCon->xadjust + (x+1)*(SMALLCHAR_WIDTH - 1), y, '^' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}
	
	row = currentCon->display;

	if ( currentCon->x == 0 ) {
		row--;
	}

	currentColor = 7;
	re.SetColor( g_color_table[currentColor] );

	for (i=0 ; i<rows ; i++, y -= SMALLCHAR_HEIGHT, row--)
	{
		if (row < 0)
			break;
		if (currentCon->current - row >= currentCon->totallines) {
			// past scrollback wrap point
			continue;	
		}

		text = currentCon->text + (row % currentCon->totallines)*currentCon->linewidth;

		for (x=0 ; x<currentCon->linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( ( (text[x]>>8)&7 ) != currentColor ) {
				currentColor = (text[x] >> 8) % 10;
				re.SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar(  currentCon->xadjust + (x+1)*(SMALLCHAR_WIDTH - 1), y, text[x] & 0xff );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();

	re.SetColor( NULL );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	int i;

	if (com_developer && com_developer->integer)
		numConsoles = 6;
	else
		numConsoles = 5;

	// check for console width changes from a vid mode change
	Con_CheckResize (currentCon);

	// if disconnected, render console full screen
	if ( cls.state == CA_DISCONNECTED ) {
		if ( !( cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( currentCon->displayFrac ) {
		for (i = 0; i < numConsoles; i++)
			consoles[i].displayFrac = currentCon->displayFrac;

		Con_DrawSolidConsole( currentCon->displayFrac );
	} else {
		// draw notify lines
		if ( cls.state == CA_ACTIVE ) {
			Con_DrawNotify ();
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole (void) {
	// decide on the destination height of the console
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
#ifdef __ANDROID__ //karin: limit console max height
		currentCon->finalFrac = Android_GetConsoleMaxHeightFrac(0.5f);		// half screen
#else
		currentCon->finalFrac = 0.5;		// half screen
#endif
	else
		currentCon->finalFrac = 0;				// none visible
	
	// scroll towards the destination height
	if (currentCon->finalFrac < currentCon->displayFrac)
	{
		currentCon->displayFrac -= con_conspeed->value*cls.realFrametime*0.001;
		if (currentCon->finalFrac > currentCon->displayFrac)
			currentCon->displayFrac = currentCon->finalFrac;

	}
	else if (currentCon->finalFrac > currentCon->displayFrac)
	{
		currentCon->displayFrac += con_conspeed->value*cls.realFrametime*0.001;
		if (currentCon->finalFrac < currentCon->displayFrac)
			currentCon->displayFrac = currentCon->finalFrac;
	}

}


void Con_PageUp( void ) {
	currentCon->display -= 2;
	if ( currentCon->current - currentCon->display >= currentCon->totallines ) {
		currentCon->display = currentCon->current - currentCon->totallines + 1;
	}
}

void Con_PageDown( void ) {
	currentCon->display += 2;
	if (currentCon->display > currentCon->current) {
		currentCon->display = currentCon->current;
	}
}

void Con_Top( void ) {
	currentCon->display = currentCon->totallines;
	if ( currentCon->current - currentCon->display >= currentCon->totallines ) {
		currentCon->display = currentCon->current - currentCon->totallines + 1;
	}
}

void Con_Bottom( void ) {
	currentCon->display = currentCon->current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify ();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	currentCon->finalFrac = 0;				// none visible
	currentCon->displayFrac = 0;
}
