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
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "triangleapi.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"

#include <assert.h>

#include "pm_shared.h"
#include "com_weapons.h"
#include "draw_util.h"

extern float g_flRoundTime;

char EV_HLDM_PlayTextureSound( int idx, pmtrace_t *ptr, const Vector &vecSrc, const Vector &vecEnd, int iBulletType, bool& isSky )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	const char *rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char *pTextureName;
	char texname[ 64 ];
	char szbuffer[ 64 ];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// FIXME check if playtexture sounds movevar is set
	//

	chTextureType = 0;
	isSky = false;

	// Player
	if ( entity >= 1 && entity <= gEngfuncs.GetMaxClients() )
	{
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	}
	else if ( entity == 0 )
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );

		if ( pTextureName )
		{
			strncpy( texname, pTextureName, sizeof( texname ) );
			texname[ sizeof( texname ) - 1 ] = 0;
			pTextureName = texname;

			if( !strcmp( pTextureName, "sky" ) )
			{
				isSky = true;
			}
			// strip leading '-0' or '+0~' or '{' or '!'
			else if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}
			else if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}

			// '}}'
			strncpy( szbuffer, pTextureName, sizeof(szbuffer) );
			szbuffer[ CBTEXTURENAMEMAX - 1 ] = 0;

			// get texture type
			chTextureType = PM_FindTextureType( szbuffer );
		}
	}

	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
	{
		fvol = 0.9;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	}
	case CHAR_TEX_METAL:
	{
		fvol = 0.9;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	}
	case CHAR_TEX_DIRT:
	{
		fvol = 0.9;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	}
	case CHAR_TEX_VENT:
	{
		fvol = 0.5;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	}
	case CHAR_TEX_GRATE:
	{
		fvol = 0.9;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	}
	case CHAR_TEX_TILE:
	{
		fvol = 0.8;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	}
	case CHAR_TEX_SLOSH:
	{
		fvol = 0.9;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	}
	case CHAR_TEX_SNOW:
	{
		fvol = 0.7;
		rgsz[0] = "debris/pl_snow1.wav";
		rgsz[1] = "debris/pl_snow2.wav";
		rgsz[2] = "debris/pl_snow3.wav";
		rgsz[3] = "debris/pl_snow4.wav";
		cnt = 4;
		break;
	}
	case CHAR_TEX_WOOD:
	{
		fvol = 0.9;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	}
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
	{
		fvol = 0.8;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	}
	case CHAR_TEX_FLESH:
	{
		fvol = 1.0;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[Com_RandomLong(0,cnt-1)], fvol, fattn, 0, 96 + Com_RandomLong(0,0xf) );

	return chTextureType;
}

char *EV_HLDM_DamageDecal( physent_t *pe )
{
	static char decalname[ 32 ];
	int idx;

	if ( pe->classnumber == 1 )
	{
		idx = Com_RandomLong( 0, 2 );
		sprintf( decalname, "{break%i", idx + 1 );
	}
	else if ( pe->rendermode != kRenderNormal )
	{
		sprintf( decalname, "{bproof1" );
	}
	else
	{
		idx = Com_RandomLong( 0, 4 );
		sprintf( decalname, "{shot%i", idx + 1 );
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName, char chTextureType )
{
	int iRand;
	physent_t *pe;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles( pTrace->endpos );


	iRand = Com_RandomLong(0,0x7FFF);
	if ( iRand < (0x7fff/2) )// not every bullet makes a sound.
	{
		if( chTextureType == CHAR_TEX_VENT || chTextureType == CHAR_TEX_METAL )
		{
			switch( iRand % 2 )
			{
			case 0: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_metal-1.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM); break;
			case 1: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_metal-2.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM); break;
			}
		}
		else
		{
			switch( iRand % 7)
			{
			case 0:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 1:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 2:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 3:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 4:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 5: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_conc-1.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM); break;
			case 6: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_conc-2.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM); break;
			}
		}

	}

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	// Only decal brush models such as the world etc.
	if (  decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot(
						gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ),
						gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );

		}
	}
}


void EV_WallPuff_Wind( struct tempent_s *te, float frametime, float currenttime )
{
	static bool xWindDirection = true;
	static bool yWindDirection = true;
	static float xWindMagnitude;
	static float yWindMagnitude;

	if ( te->entity.curstate.frame > 7.0 )
	{
		te->entity.baseline.origin.x = 0.97 * te->entity.baseline.origin.x;
		te->entity.baseline.origin.y = 0.97 * te->entity.baseline.origin.y;
		te->entity.baseline.origin.z = 0.97 * te->entity.baseline.origin.z + 0.7;
		if ( te->entity.baseline.origin.z > 70.0 )
			te->entity.baseline.origin.z = 70.0;
	}

	if ( te->entity.curstate.frame > 6.0 )
	{
		xWindMagnitude += 0.075;
		if ( xWindMagnitude > 5.0 )
			xWindMagnitude = 5.0;

		yWindMagnitude += 0.075;
		if ( yWindMagnitude > 5.0 )
			yWindMagnitude = 5.0;

		te->entity.baseline.origin.x += xWindMagnitude * ( xWindDirection ? 1 : -1 );
		te->entity.baseline.origin.y += yWindMagnitude * ( yWindDirection ? 1 : -1 );

		if ( !Com_RandomLong(0, 10) && yWindMagnitude > 3.0 )
		{
			yWindMagnitude = 0;
			yWindDirection = !yWindDirection;
		}
		if ( !Com_RandomLong(0, 10) && xWindMagnitude > 3.0 )
		{
			xWindMagnitude = 0;
			xWindDirection = !xWindDirection;
		}
	}
}

void EV_SmokeRise( struct tempent_s *te, float frametime, float currenttime )
{
	if ( te->entity.curstate.frame > 7.0 )
	{
		te->entity.baseline.origin = 0.97f * te->entity.baseline.origin;
		te->entity.baseline.origin.z += 0.7f;

		if( te->entity.baseline.origin.z > 70.0f )
			te->entity.baseline.origin.z = 70.0f;
	}
}

void EV_HugWalls(TEMPENTITY *te, pmtrace_s *ptr)
{
	float len = te->entity.baseline.origin.Length();
	Vector norm;

	if( len == 0.0f )
		norm.x = norm.y = norm.z = 0.0f;
	else
		norm = te->entity.baseline.origin / len;

	Vector v2 = CrossProduct( ptr->plane.normal, CrossProduct( ptr->plane.normal, norm ) );

	len = min( len * 1.5, 3000.0f );

	te->entity.baseline.origin.x = v2.z * len;
	te->entity.baseline.origin.y = v2.y * len;
	te->entity.baseline.origin.z = v2.x * len;
}

struct
{
	int count;
	const char *files[4];
} g_SmokeSprites[] =
{
	{
		4,
		{
			"sprites/wall_puff1.spr",
			"sprites/wall_puff2.spr",
			"sprites/wall_puff3.spr",
			"sprites/wall_puff4.spr"
		}
	},
	{
		3,
		{
			"sprites/rifle_smoke1.spr",
			"sprites/rifle_smoke2.spr",
			"sprites/rifle_smoke3.spr"
		}
	},
	{
		2,
		{
			"sprites/pistol_smoke1.spr",
			"sprites/pistol_smoke2.spr"
		}
	},
	{
		4,
		{
			"sprites/black_smoke1.spr",
			"sprites/black_smoke2.spr",
			"sprites/black_smoke3.spr",
			"sprites/black_smoke4.spr"
		}
	}
};

TEMPENTITY *EV_CS16Client_CreateSmoke( ESmoke type, Vector origin, Vector dir,
	int speed, float scale, int r, int g, int b , bool wind, Vector velocity, int framerate , int teflags )
{
	TEMPENTITY *te;
	const char *path;

	assert( type <= SMOKE_BLACK );

	int rand = Com_RandomLong( 0, g_SmokeSprites[type].count - 1 );
	path = g_SmokeSprites[type].files[rand];

	te = gEngfuncs.pEfxAPI->R_DefaultSprite( origin, gEngfuncs.pEventAPI->EV_FindModelIndex( path ), framerate );

	if( te )
	{
		if( wind )
			te->callback = EV_WallPuff_Wind;
		else
			te->callback = EV_SmokeRise;
		te->hitcallback = EV_HugWalls;
		te->flags |= teflags | FTENT_CLIENTCUSTOM;
		te->entity.curstate.rendermode = kRenderTransAdd;
		te->entity.curstate.rendercolor.r = r;
		te->entity.curstate.rendercolor.g = g;
		te->entity.curstate.rendercolor.b = b;
		te->entity.curstate.renderamt = Com_RandomLong( 100, 180 );
		te->entity.curstate.scale = scale;
		te->entity.baseline.origin = speed * dir;

		if( type != SMOKE_WALLPUFF && velocity.IsNull() )
		{
			velocity = g_vPlayerVelocity;
		}

		if( !velocity.IsNull() )
		{
			velocity.x *= 0.5;
			velocity.y *= 0.5;
			velocity.z *= 0.9;
			te->entity.baseline.origin = te->entity.baseline.origin + velocity;
		}
	}

	return te;
}

void EV_HLDM_DecalGunshot(pmtrace_t *pTrace, int iBulletType, float scale, int r, int g, int b, bool bCreateWallPuff, bool bCreateSparks, char cTextureType, bool isSky)
{
	physent_t *pe;

	if( isSky )
		return; // don't try to draw decals, spawn wall puff on skybox?

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if ( pe && pe->solid == SOLID_BSP )
	{
		EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ), cTextureType );

		// create sparks
		if( gHUD.cl_weapon_sparks->value && bCreateSparks )
		{
			Vector dir = pTrace->plane.normal;

			dir.x = dir.x * dir.x * gEngfuncs.pfnRandomFloat( 4.0f, 12.0f );
			dir.y = dir.y * dir.y * gEngfuncs.pfnRandomFloat( 4.0f, 12.0f );
			dir.z = dir.z * dir.z * gEngfuncs.pfnRandomFloat( 4.0f, 12.0f );

			gEngfuncs.pEfxAPI->R_StreakSplash( pTrace->endpos, dir, 4, Com_RandomLong( 5, 7 ), dir.z, -75.0f, 75.0f );
		}

		// create wallpuff
		if( gHUD.cl_weapon_wallpuff->value && bCreateWallPuff )
		{
			EV_CS16Client_CreateSmoke( SMOKE_WALLPUFF, pTrace->endpos, pTrace->plane.normal, 25, 0.5, r, g, b, true );
		}
	}
}

/*
============
EV_DescribeBulletTypeParameters

Sets iPenetrationPower and flPenetrationDistance.
If iBulletType is unknown, calls assert() and sets these two vars to 0
============
*/
void EV_DescribeBulletTypeParameters(int iBulletType, int &iPenetrationPower, float &flPenetrationDistance)
{
	switch (iBulletType)
	{
	case BULLET_PLAYER_9MM:
	{
		iPenetrationPower = 21;
		flPenetrationDistance = 800;
		break;
	}

	case BULLET_PLAYER_45ACP:
	{
		iPenetrationPower = 15;
		flPenetrationDistance = 500;
		break;
	}

	case BULLET_PLAYER_50AE:
	{
		iPenetrationPower = 30;
		flPenetrationDistance = 1000;
		break;
	}

	case BULLET_PLAYER_762MM:
	{
		iPenetrationPower = 39;
		flPenetrationDistance = 5000;
		break;
	}

	case BULLET_PLAYER_556MM:
	{
		iPenetrationPower = 35;
		flPenetrationDistance = 4000;
		break;
	}

	case BULLET_PLAYER_338MAG:
	{
		iPenetrationPower = 45;
		flPenetrationDistance = 8000;
		break;
	}

	case BULLET_PLAYER_57MM:
	{
		iPenetrationPower = 30;
		flPenetrationDistance = 2000;
		break;
	}

	case BULLET_PLAYER_357SIG:
	{
		iPenetrationPower = 25;
		flPenetrationDistance = 800;
		break;
	}

	default:
	{
		iPenetrationPower = 0;
		flPenetrationDistance = 0;
		break;
	}
	}
}



/*
================
EV_HLDM_FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets(int idx,
						 float *forward, float *right, float *up,
						 int cShots,
						 float *vecSrc, float *vecDirShooting, float *vecSpread,
						 float flDistance, int iBulletType, int iPenetration)
{
	int i;
	pmtrace_t tr;
	int iShot;
	int iPenetrationPower;
	float flPenetrationDistance;
	bool isSky;

	EV_DescribeBulletTypeParameters( iBulletType, iPenetrationPower, flPenetrationDistance );

	for ( iShot = 1; iShot <= cShots; iShot++ )
	{
		Vector vecShotSrc = vecSrc;
		int iShotPenetration = iPenetration;
		Vector vecDir, vecEnd;

		if ( iBulletType == BULLET_PLAYER_BUCKSHOT )
		{
			//We randomize for the Shotgun.
			float x, y, z;
			do {
				x = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				y = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				z = x*x+y*y;
			} while (z > 1);

			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + x * vecSpread[0] * right[ i ] + y * vecSpread[1] * up [ i ];
				vecEnd[i] = vecShotSrc[ i ] + flDistance * vecDir[ i ];
			}
		}
		else //But other guns already have their spread randomized in the synched spread.
		{
			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + vecSpread[0] * right[ i ] + vecSpread[1] * up [ i ];
				vecEnd[i] = vecShotSrc[ i ] + flDistance * vecDir[ i ];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );

		while (iShotPenetration != 0)
		{
			if( gEngfuncs.PM_PointContents( vecShotSrc, NULL ) == CONTENTS_SOLID )
				break;

			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

			gEngfuncs.pEventAPI->EV_PlayerTrace( vecShotSrc, vecEnd, 0, -1, &tr );

			float flCurrentDistance = tr.fraction * flDistance;

			if( flCurrentDistance == 0.0f )
			{
				break;
			}

			if ( flCurrentDistance > flPenetrationDistance )
				iShotPenetration = 0;
			else iShotPenetration--;

			char cTextureType = EV_HLDM_PlayTextureSound(idx, &tr, vecShotSrc, vecEnd, iBulletType, isSky );
			bool bSparks = true;
			int r_smoke, g_smoke, b_smoke;
			r_smoke = g_smoke = b_smoke = 40;

			switch (cTextureType)
			{
			case CHAR_TEX_METAL:
				iPenetrationPower *= 0.15;
				break;
			case CHAR_TEX_CONCRETE:
				r_smoke = g_smoke = b_smoke = 65;
				iPenetrationPower *= 0.25;
				break;
			case CHAR_TEX_VENT:
			case CHAR_TEX_GRATE:
				iPenetrationPower *= 0.5;
				break;
			case CHAR_TEX_TILE:
				iPenetrationPower *= 0.65;
				break;
			case CHAR_TEX_COMPUTER:
				iPenetrationPower *= 0.4;
				break;
			case CHAR_TEX_WOOD:
				bSparks = false;
				r_smoke = 75;
				g_smoke = 42;
				b_smoke = 15;
				break;
			}

			// do damage, paint decals
			EV_HLDM_DecalGunshot( &tr, iBulletType, 0, r_smoke, g_smoke, b_smoke, true, bSparks, cTextureType, isSky );

			if(/* iBulletType == BULLET_PLAYER_BUCKSHOT ||*/ iShotPenetration <= 0 )
			{
				break;
			}

			flDistance = (flDistance - flCurrentDistance) * 0.5;
			VectorMA( tr.endpos, iPenetration, vecDir, vecShotSrc );
			VectorMA( vecShotSrc, flDistance, vecDir, vecEnd );

			// trace back, so we will have a decal on the other side of solid area
			pmtrace_t trOriginal;

			if( gEngfuncs.PM_PointContents( vecShotSrc, NULL ) != CONTENTS_SOLID )
			{
				gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
				gEngfuncs.pEventAPI->EV_PlayerTrace(vecShotSrc, vecSrc, 0, -1, &trOriginal);
				if( !trOriginal.startsolid )
					EV_HLDM_DecalGunshot( &trOriginal, iBulletType, 0, r_smoke, g_smoke, b_smoke, true, bSparks, cTextureType, isSky );
			}
		}
		gEngfuncs.pEventAPI->EV_PopPMStates();
	}

}

void EV_CS16Client_KillEveryRound( TEMPENTITY *te, float frametime, float current_time )
{
	if( !g_flRoundTime && current_time > g_flRoundTime )
	{
		// Mark it die on next TempEntUpdate
		te->die = 0.0f;
		// Set null renderamt, so it will be invisible now
		// Also it will die immediately, if FTEMP_FADEOUT was set
		te->entity.curstate.renderamt = 0;
	}
}

void RemoveBody( TEMPENTITY *te, float frametime, float current_time )
{
	if ( current_time >= gEngfuncs.pfnGetCvarFloat("cl_corpsestay") + te->entity.curstate.fuser2 + 5.0f )
		te->entity.origin.z = te->entity.origin.z - 5.0f * frametime;
}

void HitBody( TEMPENTITY *ent, pmtrace_s *ptr )
{
	if ( ptr->plane.normal.z > 0.0f )
    	ent->flags |= FTENT_BODYGRAVITY;
}

TEMPENTITY *g_DeadPlayerModels[64];

void CreateCorpse(Vector vOrigin, Vector vAngles, const char *pModel, float flAnimTime, int iSequence, int iBody)
{
	int modelIdx = gEngfuncs.pEventAPI->EV_FindModelIndex(pModel);
	Vector null(0, 0, 0);
	TEMPENTITY *model = gEngfuncs.pEfxAPI->R_TempModel( vOrigin, null, vAngles, 100.0f, modelIdx, 0 );

	if( model )
	{
		model->flags = (FTENT_CLIENTCUSTOM|FTENT_COLLIDEALL|FTENT_SPRANIMATE|FTENT_FADEOUT|FTENT_COLLIDEWORLD|FTENT_BODYTRACE);
		model->frameMax = 255.0f;
		model->entity.curstate.framerate = 1.0f;
		model->entity.curstate.animtime = flAnimTime;
		model->entity.curstate.frame = 0.0f;
		model->entity.curstate.fuser1 = gHUD.m_flTime + 1.0f;
		model->entity.curstate.sequence = iSequence;
		model->entity.curstate.body = iBody;
		model->entity.baseline.renderamt = 255;
		model->entity.curstate.renderamt = 255;
		model->entity.curstate.fuser2 = gHUD.m_flTime + gEngfuncs.pfnGetCvarFloat("cl_corpsestay");
		model->callback = RemoveBody;
		model->hitcallback = HitBody;
		model->bounceFactor = 0.0f;
		model->die = gEngfuncs.GetClientTime() + gEngfuncs.pfnGetCvarFloat("cl_corpsestay") + 9.0f;
		//model->entity.curstate.renderfx = kRenderFxDeadPlayer;
		//model->entity.curstate.fuser4 = gHUD.m_flTime;

		for ( int i = 0; i < 64; i++ )
		{
			if ( !g_DeadPlayerModels[i] )
			{
				g_DeadPlayerModels[i] = model;
				break;
			}
		}
	}
}
