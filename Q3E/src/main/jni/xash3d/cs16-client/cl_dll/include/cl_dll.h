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
//  cl_dll.h
//

// 4-23-98  JOHN
#pragma once
#ifndef CL_DLL_H
#define CL_DLL_H
//
//  This DLL is linked by the client when they first initialize.
// This DLL is responsible for the following tasks:
//		- Loading the HUD graphics upon initialization
//		- Drawing the HUD graphics every frame
//		- Handling the custum HUD-update packets
//
typedef unsigned char byte;
typedef unsigned short word;
typedef float vec_t;
typedef int (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);

#define MIN_XASH_VERSION 3137

#include <stdint.h>

#include "util_vector.h"

#include "../engine/cdll_int.h"
#include "../dlls/cdll_dll.h"

#include "exportdef.h"

#include "render_api.h"
#include "mobility_int.h"

extern "C"
{
int        DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion );
int        DLLEXPORT HUD_VidInit( void );
void       DLLEXPORT HUD_Init( void );
int        DLLEXPORT HUD_Redraw( float flTime, int intermission );
int        DLLEXPORT HUD_UpdateClientData( client_data_t *cdata, float flTime );
void       DLLEXPORT HUD_Reset( void );
void       DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server );
void       DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove );
char       DLLEXPORT HUD_PlayerMoveTexture( char *name );
int        DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
int        DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void       DLLEXPORT HUD_Frame( double time );
void       DLLEXPORT HUD_VoiceStatus( int entindex, qboolean bTalking );
void       DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf );
int        DLLEXPORT HUD_GetRenderInterface( int version, render_api_t *renderfuncs, render_interface_t *callback );
int        DLLEXPORT HUD_MobilityInterface( mobile_engfuncs_t *mobileapi );
void       DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed );
int        DLLEXPORT HUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname );
void       DLLEXPORT HUD_CreateEntities( void );
void       DLLEXPORT HUD_StudioEvent(const struct mstudioevent_s *event, cl_entity_s *entity );
void       DLLEXPORT HUD_TxferLocalOverrides( struct entity_state_s *state, const struct clientdata_s *client );
void       DLLEXPORT HUD_ProcessPlayerState( struct entity_state_s *dst, const struct entity_state_s *src );
void       DLLEXPORT HUD_TxferPredictionData( struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd, const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd );
void       DLLEXPORT HUD_TempEntUpdate( double frametime, double client_time, double cl_gravity, struct tempent_s **ppTempEntFree, struct tempent_s **ppTempEntActive, int ( *Callback_AddVisibleEntity )( struct cl_entity_s *pEntity ), void ( *Callback_TempEntPlaySound )( struct tempent_s *pTemp, float damp ) );
void       DLLEXPORT HUD_Shutdown( void );
int        DLLEXPORT HUD_Key_Event( int eventcode, int keynum, const char *pszCurrentBinding );
int        DLLEXPORT HUD_GetStudioModelInterface( int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio );
void       DLLEXPORT HUD_DrawNormalTriangles( void );
void       DLLEXPORT HUD_DrawTransparentTriangles( void );
void       DLLEXPORT CAM_Think( void );
int        DLLEXPORT CL_IsThirdPerson( void );
void       DLLEXPORT CL_CameraOffset( float *ofs );
void       DLLEXPORT CL_CreateMove( float frametime, struct usercmd_s *cmd, int active );
void       DLLEXPORT IN_ActivateMouse( void );
void       DLLEXPORT IN_DeactivateMouse( void );
void       DLLEXPORT IN_MouseEvent( int mstate );
void       DLLEXPORT IN_Accumulate( void );
void       DLLEXPORT IN_ClearStates( void );
void       DLLEXPORT V_CalcRefdef( struct ref_params_s *pparams );
void       DLLEXPORT Demo_ReadBuffer( int size, unsigned char *buffer );
struct cl_entity_s DLLEXPORT *HUD_GetUserEntity( int index );
struct kbutton_s   DLLEXPORT *KB_Find( const char *name );

void       DLLEXPORT IN_ClientMoveEvent( float forwardmove, float sidemove );
void       DLLEXPORT IN_ClientLookEvent( float relyaw, float relpitch );
}


extern cl_enginefunc_t gEngfuncs;
extern render_api_t gRenderAPI;
extern mobile_engfuncs_t gMobileAPI;
extern int g_iXash; // indicates buildnum
extern int g_iMobileAPIVersion; // indicates version. 0 if no mobile API
#endif // CL_DLL_H
