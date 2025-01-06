//Copyright (C) 1999-2005 Id Software, Inc.
//
#include "ui_local.h"

/*
===============================================================================

CONNECTION SCREEN

===============================================================================
*/

qboolean	passwordNeeded = qtrue;
menufield_s passwordField;

static connstate_t	lastConnState;
static char			lastLoadingText[MAX_INFO_VALUE];

static void UI_ReadableSize ( char *buf, int bufsize, int value )
{
	if (value > 1024*1024*1024 ) { // gigs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d GB", 
			(value % (1024*1024*1024))*100 / (1024*1024*1024) );
	} else if (value > 1024*1024 ) { // megs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d MB", 
			(value % (1024*1024))*100 / (1024*1024) );
	} else if (value > 1024 ) { // kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	} else { // bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime ( char *buf, int bufsize, int time ) {
	time /= 1000;  // change to seconds

	if (time > 3600) { // in the hours range
		Com_sprintf( buf, bufsize, "%d hr %d min", time / 3600, (time % 3600) / 60 );
	} else if (time > 60) { // mins
		Com_sprintf( buf, bufsize, "%d min %d sec", time / 60, time % 60 );
	} else  { // secs
		Com_sprintf( buf, bufsize, "%d sec", time );
	}
}

static void UI_DisplayDownloadInfo( const char *downloadName ) {
	static char dlText[]	= "Downloading:";
	static char etaText[]	= "Estimated time left:";
	static char xferText[]	= "Transfer rate:";

	int downloadSize, downloadCount, downloadTime;
	char dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int xferRate;
	int width, leftWidth;
	int style = UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW;
	const char *s;

	downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );
	downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );

#if 0 // bk010104
	fprintf( stderr, "\n\n-----------------------------------------------\n");
	fprintf( stderr, "DB: downloadSize:  %16d\n", downloadSize );
	fprintf( stderr, "DB: downloadCount: %16d\n", downloadCount );
	fprintf( stderr, "DB: downloadTime:  %16d\n", downloadTime );  
  	fprintf( stderr, "DB: UI realtime:   %16d\n", uis.realtime );	// bk
	fprintf( stderr, "DB: UI frametime:  %16d\n", uis.frametime );	// bk
#endif

	leftWidth = width = UI_ProportionalStringWidth( dlText ) * UI_ProportionalSizeScale( style, 0 );
	width = UI_ProportionalStringWidth( etaText ) * UI_ProportionalSizeScale( style, 0 );
	if (width > leftWidth) leftWidth = width;
	width = UI_ProportionalStringWidth( xferText ) * UI_ProportionalSizeScale( style, 0 );
	if (width > leftWidth) leftWidth = width;
	leftWidth += 16;

	UI_DrawString( 8, 128, dlText, style, color_white );
	UI_DrawString( 8, 160, etaText, style, color_white );
	UI_DrawString( 8, 224, xferText, style, color_white );

	if (downloadSize > 0) {
		s = va( "%s (%d%%)", downloadName, downloadCount * 100 / downloadSize );
	} else {
		s = downloadName;
	}

	UI_DrawString( leftWidth, 128, s, style, color_white );

	UI_ReadableSize( dlSizeBuf,		sizeof dlSizeBuf,		downloadCount );
	UI_ReadableSize( totalSizeBuf,	sizeof totalSizeBuf,	downloadSize );

	if (downloadCount < 4096 || !downloadTime) {
		UI_DrawString( leftWidth, 160, "estimating", style, color_white );
		UI_DrawString( leftWidth, 192, 
			va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), style, color_white );
	} else {
	  // bk010108
	  //float elapsedTime = (float)(uis.realtime - downloadTime); // current - start (msecs)
	  //elapsedTime = elapsedTime * 0.001f; // in seconds
	  //if ( elapsedTime <= 0.0f ) elapsedTime == 0.0f;
	  if ( (uis.realtime - downloadTime) / 1000) {
			xferRate = downloadCount / ((uis.realtime - downloadTime) / 1000);
		  //xferRate = (int)( ((float)downloadCount) / elapsedTime);
		} else {
			xferRate = 0;
		}

	  //fprintf( stderr, "DB: elapsedTime:  %16.8f\n", elapsedTime );	// bk
	  //fprintf( stderr, "DB: xferRate:   %16d\n", xferRate );	// bk

		UI_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );

		// Extrapolate estimated completion time
		if (downloadSize && xferRate) {
			int n = downloadSize / xferRate; // estimated time for entire d/l in secs

			// We do it in K (/1024) because we'd overflow around 4MB
			n = (n - (((downloadCount/1024) * n) / (downloadSize/1024))) * 1000;
			
			UI_PrintTime ( dlTimeBuf, sizeof dlTimeBuf, n ); // bk010104
				//(n - (((downloadCount/1024) * n) / (downloadSize/1024))) * 1000);

			UI_DrawString( leftWidth, 160, 
				dlTimeBuf, style, color_white );
			UI_DrawString( leftWidth, 192, 
				va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), style, color_white );
		} else {
			UI_DrawString( leftWidth, 160, 
				"estimating", style, color_white );
			if (downloadSize) {
				UI_DrawString( leftWidth, 192, 
					va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), style, color_white );
			} else {
				UI_DrawString( leftWidth, 192, 
					va("(%s copied)", dlSizeBuf), style, color_white );
			}
		}

		if (xferRate) {
			UI_DrawString( leftWidth, 224, 
				va("%s/Sec", xferRateBuf), style, color_white );
		}
	}
}

/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
void UI_DrawConnectScreen( qboolean overlay ) {
	char			*s;
	uiClientState_t	cstate;
	char			info[MAX_INFO_VALUE];
	int strWidth;
	
	UI_ScreenOffset();
	trap_Cvar_Set( "r_fx_blur", "0" );			//blur UI postFX
	
	// see what information we should display
	trap_GetClientState( &cstate );
	
	info[0] = '\0';
	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );

	if(!cl_sprun.integer && Q_stricmp (Info_ValueForKey( info, "mapname" ), "uimap_1") != 0 && uis.onmap){

	Menu_Cache();

	if ( !overlay ) {
		// draw the dialog background
		UI_SetColor( color_white );
		UI_DrawHandlePic( 0-uis.wideoffset, 0, SCREEN_WIDTH+uis.wideoffset*2, SCREEN_HEIGHT, uis.menuWallpapers );
		UI_DrawHandlePic( 0-uis.wideoffset, 0, SCREEN_WIDTH+uis.wideoffset*2, SCREEN_HEIGHT, trap_R_RegisterShaderNoMip( "menu/art/blacktrans" ) );
	}

	if( strlen(info) ) {
		UI_DrawString( 320, 16, va( "Loading %s", Info_ValueForKey( info, "mapname" ) ), UI_GIANTFONT|UI_CENTER|UI_DROPSHADOW, color_white );
	}

		if(cl_language.integer == 0){
		UI_DrawString( 320, 80, va("Connecting to %s", cstate.servername), UI_CENTER|UI_BIGFONT|UI_DROPSHADOW, menu_text_color );
		}
		if(cl_language.integer == 1){
		UI_DrawString( 320, 80, va("Подключение к %s", cstate.servername), UI_CENTER|UI_BIGFONT|UI_DROPSHADOW, menu_text_color );
		}
	//UI_DrawString( 320, 96, "Press Esc to disconnect", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );

	// display global MOTD at bottom
	UI_DrawString( SCREEN_WIDTH/2, SCREEN_HEIGHT-32, 
		Info_ValueForKey( cstate.updateInfoString, "motd" ), UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	
	// print any server info (server full, bad version, etc)
	if ( cstate.connState < CA_CONNECTED ) {
		UI_DrawString( 320, 192, cstate.messageString, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}
	if ( lastConnState > cstate.connState ) {
		lastLoadingText[0] = '\0';
	}
	lastConnState = cstate.connState;

	switch ( cstate.connState ) {
	case CA_CONNECTING:
	if(cl_language.integer == 0){
		s = va("Awaiting challenge...%i", cstate.connectPacketCount);
	}
	if(cl_language.integer == 1){
		s = va("Получение информации...%i", cstate.connectPacketCount);
	}
		break;
	case CA_CHALLENGING:
	if(cl_language.integer == 0){
		s = va("Awaiting connection...%i", cstate.connectPacketCount);
	}
	if(cl_language.integer == 1){
		s = va("Подключение к серверу...%i", cstate.connectPacketCount);
	}
		break;
	case CA_CONNECTED: {
		char downloadName[MAX_INFO_VALUE];

			trap_Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof(downloadName) );
			if (*downloadName) {
				UI_DisplayDownloadInfo( downloadName );
				return;
			}
		}
	if(cl_language.integer == 0){
		s = "Awaiting gamestate...";
	}
	if(cl_language.integer == 1){
		s = "Ожидание сервера...";
	}
		break;
	case CA_LOADING:
		return;
	case CA_PRIMED:
		return;
	default:
		return;
	}

	UI_DrawString( 320, 128, s, UI_CENTER|UI_BIGFONT|UI_DROPSHADOW, color_white );

	// password required / connection rejected information goes here
	}
	if(cl_sprun.integer || Q_stricmp (Info_ValueForKey( info, "mapname" ), "uimap_1") == 0 || !uis.onmap){

	Menu_Cache();

	if(cl_language.integer == 0){
	strWidth = strlen("Loading...") * 8;
	}
	if(cl_language.integer == 1){
	strWidth = strlen("Загрузка...") * 8;
	}

	UI_SetColor( color_white );
	UI_DrawHandlePic( 0-(uis.wideoffset+1), 0, SCREEN_WIDTH+(uis.wideoffset*2)+2, SCREEN_HEIGHT*777, trap_R_RegisterShaderNoMip( "gfx/colors/black" ) );
	if(cl_language.integer == 0){
	UI_DrawString( (SCREEN_WIDTH+uis.wideoffset - strWidth) - 16, SCREEN_HEIGHT - 32, "Loading...", UI_SMALLFONT, color_white );
	}
	if(cl_language.integer == 1){
	UI_DrawString( (SCREEN_WIDTH+uis.wideoffset - strWidth) - 16, SCREEN_HEIGHT - 32, "Загрузка...", UI_SMALLFONT, color_white );
	}
	if(Q_stricmp (Info_ValueForKey( info, "mapname" ), "uimap_1") != 0 && uis.onmap){
	UI_DrawHandlePic( (SCREEN_WIDTH+uis.wideoffset - strWidth) - 80, SCREEN_HEIGHT - 64, 64, 64, uis.menuLoadingIcon );
	}

	trap_GetClientState( &cstate );
	if ( cstate.connState < CA_CONNECTED ) {
		UI_DrawString( 320, 192, cstate.messageString, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}
	}
}


/*
===================
UI_KeyConnect
===================
*/
void UI_KeyConnect( int key ) {
	if ( key == K_ESCAPE ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
		return;
	}
}
