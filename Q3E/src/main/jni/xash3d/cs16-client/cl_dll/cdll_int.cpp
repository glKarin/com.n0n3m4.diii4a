/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
#include "pmtrace.h"

#include "pm_shared.h"

#include <string.h>
#include "interface.h" // not used here
#include "render_api.h"
#include "mobility_int.h"
#include "vgui_parser.h"

cl_enginefunc_t		gEngfuncs  = { };
render_api_t		gRenderAPI = { };
mobile_engfuncs_t	gMobileAPI = { };
CHud gHUD;
int g_iXash = 0; // indicates a buildnum
int g_iMobileAPIVersion = 0;

void InitInput (void);
void Game_HookEvents( void );
void IN_Commands( void );
void Input_Shutdown (void);

/*
========================== 
    Initialize

Called when the DLL is first loaded.
==========================
*/
int DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion )
{
	if (iVersion != CLDLL_INTERFACE_VERSION)
		return 0;

	gEngfuncs = *pEnginefuncs;

	sscanf( CVAR_GET_STRING( "host_ver" ), "%d", &g_iXash );

	Game_HookEvents();

	return 1;
}


/*
=============
HUD_Shutdown

=============
*/
void DLLEXPORT HUD_Shutdown( void )
{
	gHUD.Shutdown();
	Input_Shutdown();
	Localize_Free();
}


/*
================================
HUD_GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;

	switch ( hullnumber )
	{
	case 0:				// Normal player
		Vector(-16, -16, -36).CopyToArray(mins);
		Vector(16, 16, 36).CopyToArray(maxs);
		iret = 1;
		break;
	case 1:				// Crouched player
		Vector(-16, -16, -18).CopyToArray(mins);
		Vector(16, 16, 18).CopyToArray(maxs);
		iret = 1;
		break;
	case 2:				// Point based hull
		Vector(0, 0, 0).CopyToArray(mins);
		Vector(0, 0, 0).CopyToArray(maxs);
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
HUD_ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int	DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	// Parse stuff from args
	// int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

void DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove )
{
	PM_Init( ppmove );
}

char DLLEXPORT HUD_PlayerMoveTexture( char *name )
{
	return PM_FindTextureType( name );
}

void DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server )
{
	PM_Move( ppmove, server );
}

#ifdef _CS16CLIENT_ENABLE_GSRC_SUPPORT
/*
=================
HUD_GetRect

VGui stub
=================
*/
int *HUD_GetRect( void )
{
	static int extent[4];

	extent[0] = gEngfuncs.GetWindowCenterX() - ScreenWidth / 2;
	extent[1] = gEngfuncs.GetWindowCenterY() - ScreenHeight / 2;
	extent[2] = gEngfuncs.GetWindowCenterX() + ScreenWidth / 2;
	extent[3] = gEngfuncs.GetWindowCenterY() + ScreenHeight / 2;

	return extent;
}
#endif

/*
==========================
	HUD_VidInit

Called when the game initializes
and whenever the vid_mode is changed
so the HUD can reinitialize itself.
==========================
*/

bool isLoaded = false;

int DLLEXPORT HUD_VidInit( void )
{
	gHUD.VidInit();

	isLoaded = true;

	//VGui_Startup();

	return 1;
}

/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/

void DLLEXPORT HUD_Init( void )
{
	InitInput();
	gHUD.Init();
	//Scheme_Init();
}


/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/

int DLLEXPORT HUD_Redraw( float time, int intermission )
{
	gHUD.Redraw( time, intermission );

	return 1;
}


/*
==========================
	HUD_UpdateClientData

called every time shared client
dll/engine data gets changed,
and gives the cdll a chance
to modify the data.

returns 1 if anything has been changed, 0 otherwise.
==========================
*/

int DLLEXPORT HUD_UpdateClientData(client_data_t *pcldata, float flTime )
{
	IN_Commands();

	return gHUD.UpdateClientData(pcldata, flTime );
}

/*
==========================
	HUD_Reset

Called at start and end of demos to restore to "non"HUD state.
==========================
*/

void DLLEXPORT HUD_Reset( void )
{
	gHUD.VidInit();
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/

void DLLEXPORT HUD_Frame( double time )
{
#ifdef _CS16CLIENT_ENABLE_GSRC_SUPPORT
	gEngfuncs.VGui_ViewportPaintBackground(HUD_GetRect());
#endif

	GetClientVoice()->Frame( time );
}


/*
==========================
HUD_VoiceStatus

Called when a player starts or stops talking.
==========================
*/

void DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	// gHUD.m_Radio.Voice( entindex, bTalking );

	if ( entindex >= 0 && entindex < gEngfuncs.GetMaxClients() )
	{
		if ( bTalking )
		{
			g_PlayerExtraInfo[entindex].radarflashtime = gHUD.m_flTime;
			g_PlayerExtraInfo[entindex].radarflashes = 99999;
		}
		else
		{
			g_PlayerExtraInfo[entindex].radarflashtime = -1.0f;
			g_PlayerExtraInfo[entindex].radarflashes = 0;
		}
	}

	GetClientVoice()->UpdateSpeakerStatus( entindex, bTalking );
}

/*
==========================
HUD_DirectorEvent

Called when a director event message was received
==========================
*/

void DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf )
{
	 gHUD.m_Spectator.DirectorMessage( iSize, pbuf );
}

/*
==========================
HUD_GetRenderInterface

Called when Xash3D sends render api to us
==========================
*/

int DLLEXPORT HUD_GetRenderInterface( int version, render_api_t *renderfuncs, render_interface_t *callback )
{
	if( version != CL_RENDER_INTERFACE_VERSION )
		return false;

	gRenderAPI = *renderfuncs;

	// we didn't send callbacks to engine, because we don't use it
	// *callback = renderInterface;

	// we have here a Host_Error, so check Xash for version
	if( g_iXash < MIN_XASH_VERSION )
	{
		gRenderAPI.Host_Error("Xash3D version check failed!\nPlease update your Xash3D!\n");
	}

	return true;
}

/*
========================
HUD_MobilityInterface
========================
*/
int DLLEXPORT HUD_MobilityInterface( mobile_engfuncs_t *mobileapi )
{
	if( mobileapi->version != MOBILITY_API_VERSION )
	{
		gEngfuncs.Con_Printf("Client Error: Mobile API version mismatch. Got: %i, want: %i\n",
			mobileapi->version, MOBILITY_API_VERSION);

		gRenderAPI.Host_Error("Xash3D Android version check failed!\nPlease update your Xash3D Android!\n");
		return 1;
	}

	g_iMobileAPIVersion = MOBILITY_API_VERSION;
	gMobileAPI = *mobileapi;

#define TOUCH_ADDDEFAULT (*gMobileAPI.pfnTouchAddDefaultButton)

	gMobileAPI.pfnTouchResetDefaultButtons();
	unsigned char color[] = { 255, 255, 255, 150 };
	TOUCH_ADDDEFAULT( "move", "", "_move", 0.000000, 0.444444, 0.460000, 0.995556, color, 0, 0.673353, 0 );
	TOUCH_ADDDEFAULT( "look", "", "_look", 0.470000, 0.248889, 1.000000, 0.604444, color, 0, 0.377044, 0 );
	TOUCH_ADDDEFAULT( "joy", "touch/gfx/joy", "_joy", 0.290000, 0.187234, 0.410000, 0.400745, color, 0, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "dpad", "touch/gfx/dpad", "_dpad", 0.170000, 0.187234, 0.290000, 0.400745, color, 0, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "invprev", "touch/gfx/left", "invprev", 0.000000, 0.323404, 0.080000, 0.465745, color, 2, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "invnext", "touch/gfx/right", "invnext", 0.080000, 0.323404, 0.160000, 0.465745, color, 2, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "reload", "touch/gfx/reload", "+reload", 0.680000, 0.680851, 0.760000, 0.823192, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "use", "touch/gfx/use", "+use", 0.700000, 0.544681, 0.780000, 0.687022, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "spraypaint", "touch/gfx/spraypaint", "impulse 201", 0.700000, 0.817021, 0.780000, 0.959362, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "drop", "touch/gfx/drop", "drop", 0.780000, 0.868085, 0.860000, 1.010426, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "jump", "touch/gfx/jump", "+jump", 0.880000, 0.800000, 0.980000, 0.977926, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "attack", "touch/gfx/attack", "+attack", 0.760000, 0.612766, 0.910000, 0.879655, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "attack2", "touch/gfx/attack2", "+attack2", 0.900000, 0.578723, 1.000000, 0.756649, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "w5", "touch/gfx/w_c4", "slot5", 0.760000, 0.102128, 0.840000, 0.244469, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "w1", "touch/gfx/w_rifle", "slot1", 0.780000, 0.238298, 0.860000, 0.380639, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "w2", "touch/gfx/w_pistol", "slot2", 0.840000, 0.136170, 0.920000, 0.278511, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "w4", "touch/gfx/w_grenade", "slot4", 0.880000, 0.017021, 0.960000, 0.159362, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "w3", "touch/gfx/w_knife", "slot3", 0.920000, 0.136170, 1.000000, 0.278511, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "flight", "touch/gfx/flaghtlight", "impulse 100", 0.280000, 0.851064, 0.360000, 0.993405, color, 2, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "light", "touch/gfx/light", "toggle_light", 0.360000, 0.851064, 0.440000, 0.993405, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "buy", "touch/gfx/buy", "buy", 0.440000, 0.851064, 0.520000, 0.993405, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "score", "touch/gfx/score", "scoreboard", 0.520000, 0.851064, 0.600000, 0.993405, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "nightvision", "touch/gfx/nightvision", "nightvision;toggle_plusminus", 0.360000, 0.714894, 0.440000, 0.857235, color, 2, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "minus_nvg", "touch/gfx/minus", "nvgadjustdown", 0.340000, 0.629787, 0.400000, 0.736543, color, 2, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "plus_nvg", "touch/gfx/plus", "nvgadjustup", 0.400000, 0.629787, 0.460000, 0.736543, color, 2, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "numbers", "touch_default/show_weapons", "exec touch_default/numbers.cfg", 0.440000, 0.714894, 0.520000, 0.857235, color, 2, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "duck", "touch/gfx/duck", "+duck", 0.000000, 0.817021, 0.100000, 0.994947, color, 2, 1.000000, 512 );
	TOUCH_ADDDEFAULT( "duck_sw", "touch/gfx/duck", "crouchtoggle", 0.100000, 0.817021, 0.200000, 0.994947, color, 2, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "change_team", "touch/gfx/change_team", "chooseteam", 0.540000, 0.000000, 0.620000, 0.142341, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "exit", "touch/gfx/exit", "cancelselect", 0.460000, 0.000000, 0.540000, 0.142341, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "touch_edit", "touch/gfx/settings", "touch_enableedit", 0.380000, 0.000000, 0.460000, 0.142341, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "cmd", "touch/gfx/cmdmenu", "exec touch/cmd/cmd", 0.100000, 0.248889, 0.200000, 0.426815, color, 2, 1.000000, 1 );
	TOUCH_ADDDEFAULT( "radio", "touch/gfx/radio", "showvguimenu 38", 0.000000, 0.248889, 0.100000, 0.426815, color, 2, 1.000000, 0 );
	TOUCH_ADDDEFAULT( "walk", "touch/gfx/walk", "+speed", 0.000000, 0.640000, 0.100000, 0.817926, color, 2, 1.000000, 0 );

	return 0;
}

extern "C" void DLLEXPORT HUD_ChatInputPosition( int *x, int *y )
{
}

extern "C" int DLLEXPORT HUD_GetPlayerTeam(int iplayer)
{
	if ( iplayer <= MAX_PLAYERS )
		return g_PlayerExtraInfo[iplayer].teamnumber;
	return 0;
}

#include "APIProxy.h"

cldll_func_dst_t *g_pcldstAddrs;

extern "C" void DLLEXPORT F(void *pv)
{
	cldll_func_t *pcldll_func = (cldll_func_t *)pv;

	// Hack!
	g_pcldstAddrs = ((cldll_func_dst_t *)pcldll_func->pHudVidInitFunc);

	cldll_func_t cldll_func =
	{
	Initialize,
	HUD_Init,
	HUD_VidInit,
	HUD_Redraw,
	HUD_UpdateClientData,
	HUD_Reset,
	HUD_PlayerMove,
	HUD_PlayerMoveInit,
	HUD_PlayerMoveTexture,
	IN_ActivateMouse,
	IN_DeactivateMouse,
	IN_MouseEvent,
	IN_ClearStates,
	IN_Accumulate,
	CL_CreateMove,
	CL_IsThirdPerson,
	CL_CameraOffset,
	KB_Find,
	CAM_Think,
	V_CalcRefdef,
	HUD_AddEntity,
	HUD_CreateEntities,
	HUD_DrawNormalTriangles,
	HUD_DrawTransparentTriangles,
	HUD_StudioEvent,
	HUD_PostRunCmd,
	HUD_Shutdown,
	HUD_TxferLocalOverrides,
	HUD_ProcessPlayerState,
	HUD_TxferPredictionData,
	Demo_ReadBuffer,
	HUD_ConnectionlessPacket,
	HUD_GetHullBounds,
	HUD_Frame,
	HUD_Key_Event,
	HUD_TempEntUpdate,
	HUD_GetUserEntity,
	HUD_VoiceStatus,
	HUD_DirectorMessage,
	HUD_GetStudioModelInterface,
	HUD_ChatInputPosition,
	HUD_GetPlayerTeam,
	NULL
	};

	*pcldll_func = cldll_func;
}

#include "cl_dll/IGameClientExports.h"

//-----------------------------------------------------------------------------
// Purpose: Exports functions that are used by the gameUI for UI dialogs
//-----------------------------------------------------------------------------
class CClientExports : public IGameClientExports
{
public:
	// returns the name of the server the user is connected to, if any
	virtual const char *GetServerHostName()
	{
		return gHUD.m_szServerName;
	}

	// ingame voice manipulation
	virtual bool IsPlayerGameVoiceMuted( int playerIndex )
	{
		if ( GetClientVoice() )
			return GetClientVoice()->IsPlayerBlocked( playerIndex );

		return false;
	}

	virtual void MutePlayerGameVoice( int playerIndex )
	{
		if ( GetClientVoice() )
		{
			GetClientVoice()->SetPlayerBlockedState( playerIndex, true );
		}
	}

	virtual void UnmutePlayerGameVoice( int playerIndex )
	{
		if ( GetClientVoice() )
		{
			GetClientVoice()->SetPlayerBlockedState( playerIndex, false );
		}
	}
};

EXPOSE_SINGLE_INTERFACE(CClientExports, IGameClientExports, GAMECLIENTEXPORTS_INTERFACE_VERSION)

