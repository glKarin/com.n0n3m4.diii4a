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
//
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS	64
#define MAX_LOADING_ITEM_ICONS		256

static int			loadingPlayerIconCount;
static int			loadingItemIconCount;
static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t	loadingItemIcons[MAX_LOADING_ITEM_ICONS];


/*
===================
CG_DrawLoadingIcons
===================
*/
static void CG_DrawLoadingIcons( void ) {
	int		n;
	int		x, y;

	for( n = 0; n < loadingPlayerIconCount; n++ ) {
		x = 8 + n * 36 - cl_screenoffset.value;
		y = 440-32;
		CG_DrawPic( x, y, 32, 32, loadingPlayerIcons[n] );
	}

	for( n = 0; n < loadingItemIconCount; n++ ) {
		y = 480-20;
		x = 8 + n * 18- cl_screenoffset.value;
		CG_DrawPic( x, y, 16, 16, loadingItemIcons[n] );
	}
}


/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );

	trap_UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem( int itemNum ) {
	gitem_t		*item;

	item = &bg_itemlist[itemNum];
	
	if ( item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS ) {
		loadingItemIcons[loadingItemIconCount++] = trap_R_RegisterShaderNoMip( item->icon );
	}
	if(cl_language.integer == 0){
	CG_LoadingString( item->pickup_name );
	}
	if(cl_language.integer == 1){
	CG_LoadingString( item->pickup_nameru );
	}
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char		*info;
	char			*skin;
	char			personality[MAX_QPATH];
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];

	info = CG_ConfigString( CS_PLAYERS + clientNum );

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		skin = strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", model, skin );
		
		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/characters/%s/icon_%s.tga", model, skin );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", DEFAULT_MODEL, "default" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}

	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
	Q_CleanStr( personality );

	CG_LoadingString( personality );
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation( void ) {
	const char	*s;
	const char	*info;
	const char	*sysInfo;
	int			y;
	int			value;
	qhandle_t	levelshot;
	qhandle_t	detail;
	char		buf[1024];

	info = CG_ConfigString( CS_SERVERINFO );
	sysInfo = CG_ConfigString( CS_SYSTEMINFO );

	s = Info_ValueForKey( info, "mapname" );
    if(cgs.gametype == GT_SINGLE || Q_stricmp (s, "uimap_1") == 0){
		return;
    }
	levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga", s ) );
	if ( !levelshot ) {
		levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
	}
	trap_R_SetColor( NULL );

	// blend a detail texture over it
	detail	= trap_R_RegisterShaderNoMip( "menu/animbg" );
	CG_DrawPic( -1 - cl_screenoffset.value, 0, 642 + cl_screenoffset.value*2, 480, detail );
	CG_DrawPic( -1 - cl_screenoffset.value, 0, 642 + cl_screenoffset.value*2, 480, trap_R_RegisterShaderNoMip( "menu/art/blacktrans" ) );
	CG_DrawPic( 30, 180-58, 280, 200, levelshot );

	// draw the icons of things as they are loaded
	CG_DrawLoadingIcons();

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if ( cg.infoScreenText[0] ) {
		if(cl_language.integer == 0){
		CG_DrawBigString( 320, 128, va("Loading... %s", cg.infoScreenText), 1.0F );
		}
		if(cl_language.integer == 1){
		CG_DrawBigString( 320, 128, va("Загрузка... %s", cg.infoScreenText), 1.0F );
		}
	} else {
		if(cl_language.integer == 0){
		CG_DrawBigString( 320, 128, "Connecting...", 1.0F );
		}
		if(cl_language.integer == 1){
		CG_DrawBigString( 320, 128, "Подключение...", 1.0F );	
		}
	}

	// draw info string information

	y = 220-32;

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer( "sv_running", buf, sizeof( buf ) );
	if ( !atoi( buf ) ) {
		// server hostname
		Q_strncpyz(buf, Info_ValueForKey( info, "sv_hostname" ), 1024);
		Q_CleanStr(buf);
		CG_DrawBigString( 320, y, buf, 1.0F );
		y += PROP_HEIGHT;

		// pure server
		s = Info_ValueForKey( sysInfo, "sv_pure" );
		if ( s[0] == '1' ) {
			if(cl_language.integer == 0){
			CG_DrawBigString( 320, y, "Pure Server", 1.0F );
			}
			if(cl_language.integer == 1){
			CG_DrawBigString( 320, y, "Чистый Сервер", 1.0F );
			}
			y += PROP_HEIGHT;
		}

		// server-specific message of the day
		s = CG_ConfigString( CS_MOTD );
		if ( s[0] ) {
			CG_DrawBigString( 320, y, s, 1.0F );
			y += PROP_HEIGHT;
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	s = CG_ConfigString( CS_MESSAGE );
	if ( s[0] ) {
		CG_DrawBigString( 320, y, s, 1.0F );
		y += PROP_HEIGHT;
	}
	
	// QSandbox by Noire.dev
		CG_DrawBigString( 320, y, "^2QSandbox by Noire.dev", 1.0F );
		y += PROP_HEIGHT;

	// cheats warning
	s = Info_ValueForKey( sysInfo, "sv_cheats" );
	if ( s[0] == '1' ) {
		if(cl_language.integer == 0){
		CG_DrawBigString( 320, y, "CHEATS ARE ENABLED", 1.0F );
		}
		if(cl_language.integer == 1){
		CG_DrawBigString( 320, y, "ЧИТЫ ВКЛЮЧЕНЫ", 1.0F );
		}
		y += PROP_HEIGHT;
	}

if(cl_language.integer == 0){
	// game type
	switch ( cgs.gametype ) {
	case GT_SANDBOX:
		s = "Sandbox";
		break;
	case GT_FFA:
		s = "Free For All";
		break;
	case GT_SINGLE:
		s = "Single Player";
		break;
	case GT_TOURNAMENT:
		s = "Tournament";
		break;
	case GT_TEAM:
		s = "Team Deathmatch";
		break;
	case GT_CTF:
		s = "Capture The Flag";
		break;
	case GT_1FCTF:
		s = "One Flag CTF";
		break;
	case GT_OBELISK:
		s = "Overload";
		break;
	case GT_HARVESTER:
		s = "Harvester";
		break;
	case GT_ELIMINATION:
		s = "Elimination";
		break;
	case GT_CTF_ELIMINATION:
		s = " CTF Elimination";
		break;
	case GT_LMS:
		s = "Last Man Standing";
		break;
	case GT_DOUBLE_D:
		s = "Double Domination";
		break;
        case GT_DOMINATION:
		s = "Domination";
		break;
	default:
		s = "Unknown Gametype";
		break;
	}
}
if(cl_language.integer == 1){
	// game type
	switch ( cgs.gametype ) {
	case GT_SANDBOX:
		s = "Песочница";
		break;
	case GT_FFA:
		s = "Все против всех";
		break;
	case GT_SINGLE:
		s = "Одиночная Игра";
		break;
	case GT_TOURNAMENT:
		s = "Турнир";
		break;
	case GT_TEAM:
		s = "Командный бой";
		break;
	case GT_CTF:
		s = "Захват флага";
		break;
	case GT_1FCTF:
		s = "Один флаг";
		break;
	case GT_OBELISK:
		s = "Атака базы";
		break;
	case GT_HARVESTER:
		s = "Жнец";
		break;
	case GT_ELIMINATION:
		s = "Устранение";
		break;
	case GT_CTF_ELIMINATION:
		s = "Устранение: Захват флага";
		break;
	case GT_LMS:
		s = "Последний оставшийся";
		break;
	case GT_DOUBLE_D:
		s = "Двойное доминирование";
		break;
        case GT_DOMINATION:
		s = "Доминирование";
		break;
	default:
		s = "Неизвесный режим";
		break;
	}
}
	CG_DrawBigString( 320, y, s, 1.0F );
	y += PROP_HEIGHT;
	value = atoi( Info_ValueForKey( info, "timelimit" ) );
	if ( value ) {
		if(cl_language.integer == 0){
		CG_DrawBigString( 320, y, va( "Time limit %i", value ), 1.0F );
		}
		if(cl_language.integer == 1){
		CG_DrawBigString( 320, y, va( "Лимит времени %i", value ), 1.0F );
		}
		y += PROP_HEIGHT;
	}

	if (cgs.gametype < GT_CTF || cgs.ffa_gt>0) {
		value = atoi( Info_ValueForKey( info, "fraglimit" ) );
		if ( value ) {
			if(cl_language.integer == 0){
			CG_DrawBigString( 320, y, va( "Frag limit %i", value ), 1.0F );
			}
			if(cl_language.integer == 1){
			CG_DrawBigString( 320, y, va( "Лимит фрагов %i", value ), 1.0F );
			}
			y += PROP_HEIGHT;
		}
	}

	if (cgs.gametype >= GT_CTF && cgs.ffa_gt == 0) {
		value = atoi( Info_ValueForKey( info, "capturelimit" ) );
		if ( value ) {
			if(cl_language.integer == 0){
			CG_DrawBigString( 320, y, va( "Сapture limit %i", value ), 1.0F );
			}
			if(cl_language.integer == 1){
			CG_DrawBigString( 320, y, va( "Лимит захвата %i", value ), 1.0F );
			}
			y += PROP_HEIGHT;
		}
	}
}

