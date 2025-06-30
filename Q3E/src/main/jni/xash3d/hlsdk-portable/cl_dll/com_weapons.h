//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// com_weapons.h
// Shared weapons common function prototypes
#pragma once
#if !defined( COM_WEAPONSH )
#define COM_WEAPONSH

#include "hud_iface.h"

extern "C"
{
	void _DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed );
}

void			COM_Log( const char *pszFile, const char *fmt, ... );
int				CL_IsDead( void );

float			UTIL_SharedRandomFloat( unsigned int seed, float low, float high );
int				UTIL_SharedRandomLong( unsigned int seed, int low, int high );

int				HUD_GetWeaponAnim( void );
void			HUD_SendWeaponAnim( int iAnim, int body, int force );
void			HUD_PlaySound( const char *sound, float volume );
void			HUD_PlaybackEvent( int flags, const struct edict_s *pInvoker, unsigned short eventindex, float delay, const float *origin, const float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
void			HUD_SetMaxSpeed( const struct edict_s *ed, float speed );
int				stub_PrecacheModel( const char* s );
int				stub_PrecacheSound( const char* s );
unsigned short	stub_PrecacheEvent( int type, const char *s );
const char		*stub_NameForFunction( void *function );
void			stub_SetModel( struct edict_s *e, const char *m );

extern cvar_t *cl_lw;

extern int g_runfuncs;
extern vec3_t v_angles;
extern vec3_t v_client_aimangles;
extern float g_lastFOV;
extern struct local_state_s *g_finalstate;
#endif
