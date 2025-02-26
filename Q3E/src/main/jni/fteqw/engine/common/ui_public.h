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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


//these structures are shared with the exe.

#define UIMAX_SCOREBOARDNAME 16
#define UIMAX_INFO_STRING EXTENDED_INFO_STRING

typedef struct {
	int userid;
	char name[UIMAX_SCOREBOARDNAME];	//for faster reading.
	float starttime;
	int frags;
	int ping;
	int pl;

	int topcolour;
	int bottomcolour;

	char userinfo[UIMAX_INFO_STRING];	//should this size be enforced?
									//you can get all sorts of stuff like names.
} vmuiclientinfo_t;

//useful for it's width/height. The others are a little pointless to be honest.
typedef struct {
	unsigned int width;
	unsigned int height;
	unsigned int bpp;
	unsigned int refreshrate;	//quakeworld normally only draws 30 frames per second dontcha know?
	qboolean fullscreen;	//oposite of windowed.
	char renderername[256];		//Human readable

	int vidbugs;	//flags for the buggy implementations of opengl or whatever.
} vidinfo_t;

//is there any point to these?
enum {
	VB_NOSCALE			= 1<<0,	//software rendering, incapable of scaling.
	VB_NOCOLOUR			= 1<<1,	//software rendering that doesn't allow belnding colours. (8 bit paletted)
	VB_NOCOLOURINTERP		= 1<<2,	//software rendering that supports a blending of colours, but not per vertex.

	VB_NOINTERPOLATEALPHA	= 1<<3,	//riva128
	VB_NOMODULATEALPHA	= 1<<4,	//ragepro
	VB_NOSRCTIMESDST		= 1<<5,	//permedia
};

typedef enum {
	SID_Q2STATUSBAR		= -4,
	SID_Q2LAYOUT		= -3,
	SID_CENTERPRINTTEXT	= -2,
	SID_SERVERNAME		= -1,
	//q2's config strings come here.
} stringid_e;

typedef enum {
	Q3CA_UNINITIALIZED,
	Q3CA_DISCONNECTED, 	// not talking to a server
	Q3CA_AUTHORIZING,		// not used any more, was checking cd key 
	Q3CA_CONNECTING,		// sending request packets to the server
	Q3CA_CHALLENGING,		// sending challenge packets to the server
	Q3CA_CONNECTED,		// netchan_t established, getting gamestate
	Q3CA_LOADING,			// only during cgame initialization, never during main loop
	Q3CA_PRIMED,			// got gamestate, waiting for first frame
	Q3CA_ACTIVE,			// game views should be displayed
	Q3CA_CINEMATIC		// playing a cinematic or a static pic, not connected to a server
} q3connstate_t;
typedef struct {
	q3connstate_t		connState;
	int				connectPacketCount;
	int				clientNum;
	char			servername[MAX_STRING_CHARS];
	char			updateInfoString[MAX_STRING_CHARS];
	char			messageString[MAX_STRING_CHARS];
} uiClientState_t;

typedef enum {
	UI_GETAPIVERSION = 0,	// system reserved

	UI_INIT,
//	void	UI_Init( void );

	UI_SHUTDOWN,
//	void	UI_Shutdown( void );

	UI_KEY_EVENT,
//	void	UI_KeyEvent( int key );

	UI_MOUSE_EVENT,
//	void	UI_MouseEvent( int dx, int dy );

	UI_REFRESH,
//	void	UI_Refresh( int time );

	UI_IS_FULLSCREEN,
//	qboolean UI_IsFullscreen( void );

	UI_SET_ACTIVE_MENU,
//	void	UI_SetActiveMenu( uiMenuCommand_t menu );

	UI_CONSOLE_COMMAND,
//	qboolean UI_ConsoleCommand( int realTime );

	UI_DRAW_CONNECT_SCREEN,
//	void	UI_DrawConnectScreen( qboolean overlay );
	UI_HASUNIQUECDKEY,
// if !overlay, the background will be drawn, otherwise it will be
// overlayed over whatever the cgame has drawn.
// a GetClientState syscall will be made to get the current strings

} uiExport_t;

typedef enum {
	UI_ERROR,
	UI_PRINT,
	UI_MILLISECONDS,
	UI_CVAR_SET,
	UI_CVAR_VARIABLEVALUE,
	UI_CVAR_VARIABLESTRINGBUFFER,
	UI_CVAR_SETVALUE,
	UI_CVAR_RESET,
	UI_CVAR_CREATE,
	UI_CVAR_INFOSTRINGBUFFER,
	UI_ARGC,//10
	UI_ARGV,
	UI_CMD_EXECUTETEXT,
	UI_FS_FOPENFILE,
	UI_FS_READ,
	UI_FS_WRITE,
	UI_FS_FCLOSEFILE,
	UI_FS_GETFILELIST,
	UI_R_REGISTERMODEL,
	UI_R_REGISTERSKIN,
	UI_R_REGISTERSHADERNOMIP,//20
	UI_R_CLEARSCENE,
	UI_R_ADDREFENTITYTOSCENE,
	UI_R_ADDPOLYTOSCENE,
	UI_R_ADDLIGHTTOSCENE,
	UI_R_RENDERSCENE,
	UI_R_SETCOLOR,
	UI_R_DRAWSTRETCHPIC,
	UI_UPDATESCREEN,
	UI_CM_LERPTAG,
	UI_CM_LOADMODEL,//30
	UI_S_REGISTERSOUND,
	UI_S_STARTLOCALSOUND,
	UI_KEY_KEYNUMTOSTRINGBUF,
	UI_KEY_GETBINDINGBUF,
	UI_KEY_SETBINDING,
	UI_KEY_ISDOWN,
	UI_KEY_GETOVERSTRIKEMODE,
	UI_KEY_SETOVERSTRIKEMODE,
	UI_KEY_CLEARSTATES,
	UI_KEY_GETCATCHER,//40
	UI_KEY_SETCATCHER,
	UI_GETCLIPBOARDDATA,
	UI_GETGLCONFIG,
	UI_GETCLIENTSTATE,
	UI_GETCONFIGSTRING,
	UI_LAN_GETPINGQUEUECOUNT,
	UI_LAN_CLEARPING,
	UI_LAN_GETPING,
	UI_LAN_GETPINGINFO,
	UI_CVAR_REGISTER,//50
	UI_CVAR_UPDATE,
	UI_MEMORY_REMAINING,
	UI_GET_CDKEY,
	UI_SET_CDKEY,
	UI_R_REGISTERFONT,
	UI_R_MODELBOUNDS,
	UI_PC_ADD_GLOBAL_DEFINE,
	UI_PC_LOAD_SOURCE,
	UI_PC_FREE_SOURCE,
	UI_PC_READ_TOKEN,//60
	UI_PC_SOURCE_FILE_AND_LINE,
	UI_S_STOPBACKGROUNDTRACK,
	UI_S_STARTBACKGROUNDTRACK,
	UI_REAL_TIME,
	UI_LAN_GETSERVERCOUNT,
	UI_LAN_GETSERVERADDRESSSTRING,
	UI_LAN_GETSERVERINFO,
	UI_LAN_MARKSERVERVISIBLE,
	UI_LAN_UPDATEVISIBLEPINGS,
	UI_LAN_RESETPINGS,//70
	UI_LAN_LOADCACHEDSERVERS,
	UI_LAN_SAVECACHEDSERVERS,
	UI_LAN_ADDSERVER,
	UI_LAN_REMOVESERVER,
	UI_CIN_PLAYCINEMATIC,
	UI_CIN_STOPCINEMATIC,
	UI_CIN_RUNCINEMATIC,
	UI_CIN_DRAWCINEMATIC,
	UI_CIN_SETEXTENTS,
	UI_R_REMAP_SHADER,//80
	UI_VERIFY_CDKEY,
	UI_LAN_SERVERSTATUS,
	UI_LAN_GETSERVERPING,
	UI_LAN_SERVERISVISIBLE,
	UI_LAN_COMPARESERVERS,
	// 1.32
	UI_FS_SEEK,
	UI_SET_PBCLSTATUS,//87

	UI_MEMSET = 100,
	UI_MEMCPY,
	UI_STRNCPY,
	UI_SIN,
	UI_COS,
	UI_ATAN2,
	UI_SQRT,
	UI_FLOOR,
	UI_CEIL,
/*
	UI_CACHE_PIC		= 500,
	UI_PICFROMWAD		= 501,
	UI_GETPLAYERINFO	= 502,
	UI_GETSTAT			= 503,
	UI_GETVIDINFO		= 504,
	UI_GET_STRING		= 510,
*/
} uiImport_t;
