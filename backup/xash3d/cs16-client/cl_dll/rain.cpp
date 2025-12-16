/***
*
*	Copyright (c) 2005, BUzer.
*	
*	Used with permission for Spirit of Half-Life 1.5
*
****/
/*
====== rain.cpp ========================================================
*/

#include <memory.h>
#include "hud.h"
#include "pm_math.h"
#include "cl_util.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "pm_defs.h"
#include "event_api.h"

#include "rain.h"
#include "r_efx.h"
#include "con_nprint.h"
#include "triangleapi.h"
#include "parsemsg.h"

#undef fabs
#include <new>

#include "com_model.h"

#define DRIPSPEED    900		// speed of raindrips (pixel per secs)
#define SNOWSPEED    200		// speed of snowflakes
#define SNOWFADEDIST 80

#define MAXDRIPS 2000	// max raindrops
#define MAXFX    3000	// max effects

#define DRIP_SPRITE_HALFHEIGHT 64
#define DRIP_SPRITE_HALFWIDTH  1
#define SNOW_SPRITE_HALFSIZE   3

// radius water rings
#define MAXRINGHALFSIZE	25

struct
{
	Vector2D wind;
	Vector2D rand;

	float    distFromPlayer;
	float    globalHeight;

	int      dripsPerSecond;
	int	     weatherMode;	// 0 - snow, 1 - rain
	int      weatherValue;

	float    curtime;    // current time
	float    oldtime;    // last time we have updated drips
	float    timedelta;  // difference between old time and current time
	float    nextspawntime;  // when the next drip should be spawned

	int dripcounter;
	int fxcounter;
	float heightFromPlayer;

	HSPRITE hsprRain;
	HSPRITE hsprSnow;
	HSPRITE hsprRipple;
} Rain;

bool Rain_Initialized = false;

enum
{
	NO_LANDING = 0,
	DEFAULT_LANDING,
	WATER_LANDING
};

struct cl_drip_t
{
	Vector		origin;
	float		birthTime;
	float		minHeight;	// minimal height to kill raindrop
	float		alpha;

	Vector2D    Delta; // side speed
	int         land;

	cl_drip_t*		p_Next;		// next drip in chain
	cl_drip_t*		p_Prev;		// previous drip in chain
} FirstChainDrip;

struct cl_rainfx_t
{
	Vector		origin;
	float		birthTime;
	float		life;
	float		alpha;

	int type;

	cl_rainfx_t*		p_Next;		// next fx in chain
	cl_rainfx_t*		p_Prev;		// previous fx in chain
} FirstChainFX;


#ifdef _DEBUG
cvar_t *debug_rain = NULL;
#endif


/*
=================================
WaterLandingEffect
=================================
*/
void LandingEffect( cl_drip_t *drip )
{
	if( drip->land == NO_LANDING )
		return;

	if (Rain.fxcounter >= MAXFX)
	{
		//gEngfuncs.Con_Printf( "Rain error: FX limit overflow!\n" );
		return;
	}

	cl_rainfx_t *newFX = new(std::nothrow) cl_rainfx_t;
	if( !newFX )
	{
		gEngfuncs.Con_Printf( "Rain error: failed to allocate FX object!\n");
		return;
	}

	newFX->alpha = gEngfuncs.pfnRandomFloat(0.6, 0.9);
	newFX->origin = drip->origin;
	newFX->origin.z = drip->minHeight; // correct position
	newFX->birthTime = Rain.curtime;
	newFX->life = gEngfuncs.pfnRandomFloat(0.7, 1);
	newFX->type = drip->land;

	// add to first place in chain
	newFX->p_Next = FirstChainFX.p_Next;
	newFX->p_Prev = &FirstChainFX;
	if (newFX->p_Next != NULL)
		newFX->p_Next->p_Prev = newFX;
	FirstChainFX.p_Next = newFX;

	Rain.fxcounter++;
}
/*
=================================
ProcessRain

Must think every frame.
=================================
*/
void ProcessRain( void )
{
	int speed = Rain.weatherMode ? SNOWSPEED : DRIPSPEED;

	Rain.oldtime = Rain.curtime; // save old time
	Rain.curtime = gEngfuncs.GetClientTime();
	Rain.timedelta = Rain.curtime - Rain.oldtime;

	if( gHUD.cl_weather->value > 3.0f )
		gEngfuncs.Cvar_Set( "cl_weather", "3" );

	Rain.weatherValue = gHUD.cl_weather->value;

	if( Rain.dripsPerSecond == 0 || !Rain.weatherValue )
		return; // disabled

	// first frame
	if( Rain.oldtime == 0 || ( Rain.dripsPerSecond == 0 && FirstChainDrip.p_Next == NULL ) )
	{
		// fix first frame bug with nextspawntime
		Rain.nextspawntime = Rain.curtime;
		return;
	}

	if( !Rain.timedelta )
		return; // not in pause

	int spawnDrips = (Rain.dripsPerSecond + (Rain.weatherValue - 1) * 150);
	double timeBetweenDrips = 1.0 / (double)(spawnDrips);

#ifdef _DEBUG
	// save debug info
	float debug_lifetime = 0;
	int debug_howmany = 0;
	int debug_attempted = 0;
	int debug_dropped = 0;
#endif

	for( cl_drip_t *curDrip = FirstChainDrip.p_Next, *nextDrip = NULL;
		 curDrip;
		 curDrip = nextDrip ) // go through list
	{
		nextDrip = curDrip->p_Next; // save pointer to next drip

		curDrip->origin.x += Rain.timedelta * curDrip->Delta.x;
		curDrip->origin.y += Rain.timedelta * curDrip->Delta.y;
		curDrip->origin.z -= Rain.timedelta * speed;

		// remove drip if its origin lower than minHeight
		if (curDrip->origin.z < curDrip->minHeight)
		{
			LandingEffect( curDrip );
#ifdef _DEBUG
			if( debug_rain->value )
			{
				debug_lifetime += ( Rain.curtime - curDrip->birthTime );
				debug_howmany++;
			}
#endif

			curDrip->p_Prev->p_Next = curDrip->p_Next; // link chain
			if( nextDrip != NULL )
				nextDrip->p_Prev = curDrip->p_Prev;
			delete curDrip;

			Rain.dripcounter--;
		}
	}

	int maxDelta = speed * Rain.timedelta; // maximum height randomize distance
	float falltime = (Rain.globalHeight + 4096) / speed;

	for( ; Rain.nextspawntime < Rain.curtime; Rain.nextspawntime += timeBetweenDrips )
	{
#ifdef _DEBUG
		if( debug_rain->value )
			debug_attempted++;
#endif
				
		if( Rain.dripcounter < spawnDrips ) // check for overflow
		{
			float deathHeight;
			Vector vecStart, vecEnd, vecStartStart;
			Vector2D Delta( Rain.wind.x + gEngfuncs.pfnRandomFloat( Rain.rand.x * -1, Rain.rand.x ),
							Rain.wind.y + gEngfuncs.pfnRandomFloat( Rain.rand.y * -1, Rain.rand.y ));
			pmtrace_t pmtrace, pmtrace2;

			vecStart.x = gEngfuncs.pfnRandomFloat( gHUD.m_vecOrigin.x - Rain.distFromPlayer, gHUD.m_vecOrigin.x + Rain.distFromPlayer );
			vecStart.y = gEngfuncs.pfnRandomFloat( gHUD.m_vecOrigin.y - Rain.distFromPlayer, gHUD.m_vecOrigin.y + Rain.distFromPlayer );
			vecStart.z = gHUD.m_vecOrigin.z + Rain.heightFromPlayer;

			// find a point at bottom of map
			vecEnd.x = falltime * Delta.x;
			vecEnd.y = falltime * Delta.y;
			vecEnd.z = -4096;

			if( gEngfuncs.PM_PointContents( vecStart, NULL ) == CONTENTS_SOLID )
			{
#ifdef _DEBUG
				if( debug_rain->value )
					debug_dropped++;
#endif
				continue; // drip cannot be placed
			}

			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecStart, vecEnd, PM_WORLD_ONLY, -1, &pmtrace );

			if( pmtrace.startsolid )
			{
#ifdef _DEBUG
				if( debug_rain->value )
					debug_dropped++;
#endif
				continue; // drip cannot be placed
			}

			vecStartStart = vecStart;
			vecStartStart.z = 999999;

			// second trace. Check that player have a real sky above him
			const char *s = gEngfuncs.pEventAPI->EV_TraceTexture( pmtrace.ent, vecStart, vecStartStart );
			if( !s || strcmp( s, "sky" ) )
			{
#ifdef _DEBUG
				if( debug_rain->value )
					debug_dropped++;
#endif
				continue;
			}
			
			// falling to water?
			int contents = gEngfuncs.PM_PointContents( pmtrace.endpos, NULL );
			if( contents == CONTENTS_WATER )
			{
				int waterEntity = gEngfuncs.PM_WaterEntity( pmtrace.endpos );
				if( waterEntity > 0 )
				{
					cl_entity_t *pwater = gEngfuncs.GetEntityByIndex( waterEntity );
					if( pwater && ( pwater->model != NULL ) )
					{
						deathHeight = pwater->curstate.maxs[2];
					}
					else
					{
						gEngfuncs.Con_Printf("Rain error: can't get water entity\n");
						continue;
					}
				}
				else
				{
					gEngfuncs.Con_Printf("Rain error: water is not func_water entity\n");
					continue;
				}
			}
			else
			{
				deathHeight = pmtrace.endpos[2];
			}

			// just in case..
			if (deathHeight > vecStart[2])
			{
				gEngfuncs.Con_Printf("Rain error: can't create drip in water\n");
				continue;
			}

			cl_drip_t *newClDrip = new(std::nothrow) cl_drip_t;
			if( !newClDrip )
			{
				Rain.dripsPerSecond = 0; // disable rain
				gEngfuncs.Con_Printf( "Rain error: failed to allocate object!\n");
				return;
			}
			
			vecStart[2] -= gEngfuncs.pfnRandomFloat( 0, maxDelta ); // randomize a bit
			
			newClDrip->alpha     = gEngfuncs.pfnRandomFloat( 0.12, 0.2 );
			newClDrip->origin    = vecStart;
			newClDrip->Delta     = Delta;
			newClDrip->birthTime = Rain.curtime; // store time when it was spawned
			newClDrip->minHeight = deathHeight;

			if( contents == CONTENTS_WATER )
			{
				newClDrip->land = WATER_LANDING;
			}
			else
			{
				newClDrip->land = NO_LANDING;
			}
			/*else if( pmtrace->fraction < 1.0f && pmtrace->plane.normal.z > 0.71 && !pmtrace->inopen)
			{
				newClDrip->land = DEFAULT_LANDING;
			}
			else
			{
				newClDrip->land = NO_LANDING;
			}*/

			// add to first place in chain
			newClDrip->p_Next = FirstChainDrip.p_Next;
			newClDrip->p_Prev = &FirstChainDrip;
			if (newClDrip->p_Next != NULL)
				newClDrip->p_Next->p_Prev = newClDrip;
			FirstChainDrip.p_Next = newClDrip;

			Rain.dripcounter++;
		}
		else
		{
			//gEngfuncs.Con_Printf( "Rain error: Drip limit overflow!\n" );
			return;
		}
	}

#ifdef _DEBUG
	if( debug_rain->value ) // print debug info
	{
		con_nprint_t info =
		{
			1,
			0.5f,
			{1.0f, 0.6f, 0.0f }
		};
		gEngfuncs.Con_NXPrintf( &info, "Rain info: Drips exist: %i\n", Rain.dripcounter );

		info.index = 2;
		gEngfuncs.Con_NXPrintf( &info, "Rain info: FX's exist: %i\n", Rain.fxcounter );

		info.index = 3;
		gEngfuncs.Con_NXPrintf( &info, "Rain info: Attempted/Dropped: %i, %i\n", debug_attempted, debug_dropped);
		if( debug_howmany )
		{
			float ave = debug_lifetime / (float)debug_howmany;

			info.index = 4;
			gEngfuncs.Con_NXPrintf( &info, "Rain info: Average drip life time: %f\n", ave);
		}
	}
#endif
}

/*
=================================
ProcessFXObjects

Remove all fx objects with out time to live
Call every frame before ProcessRain
=================================
*/
void ProcessFXObjects( void )
{
	for( cl_rainfx_t *curFX = FirstChainFX.p_Next, *nextFX = NULL;
		 curFX;
		 curFX = nextFX )
	{
		nextFX = curFX->p_Next; // save pointer to next

		// delete current?
		if( curFX->birthTime + curFX->life < Rain.curtime )
		{
			curFX->p_Prev->p_Next = curFX->p_Next; // link chain
			if( nextFX )
				nextFX->p_Prev = curFX->p_Prev;

			delete curFX;
			Rain.fxcounter--;
		}
	}
}

/*
=================================
ResetRain

clear memory, delete all objects
=================================
*/
void ResetRain( void )
{
	// delete all drips
	for( cl_drip_t *curDrip = FirstChainDrip.p_Next; curDrip;
		 curDrip = FirstChainDrip.p_Next, Rain.dripcounter-- )
	{
		FirstChainDrip.p_Next = curDrip->p_Next;
		delete curDrip;
	}

	// delete all FX objects
	for( cl_rainfx_t *curFX = FirstChainFX.p_Next; curFX;
		 curFX = FirstChainFX.p_Next, Rain.fxcounter-- )
	{
		FirstChainFX.p_Next = curFX->p_Next;
		delete curFX;
	}

	InitRain();
	return;
}


int __MsgFunc_ReceiveW(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize);

	int iWeatherType = reader.ReadByte();

	if( iWeatherType == 0 )
	{
		ResetRain();
		return 1;
	}

	Rain.distFromPlayer = 500;
	Rain.dripsPerSecond = 500;
	Rain.wind.x = Rain.wind.y = 30;
	Rain.rand.x = Rain.rand.y = 0;
	Rain.weatherMode = iWeatherType - 1;
	Rain.globalHeight = 100;
	Rain.heightFromPlayer = 100;

	return 1;
}

/*
=================================
InitRain
initialze system
=================================
*/
void InitRain( void )
{
	memset( &Rain, 0, sizeof(Rain) );
	memset( &FirstChainDrip, 0, sizeof( cl_drip_t ));
	memset( &FirstChainFX, 0, sizeof( cl_rainfx_t ));

#ifdef _DEBUG
	if( !debug_rain )
		debug_rain = CVAR_CREATE( "Rain.debug", "0", 0 );
#endif

	Rain.hsprRain = SPR_Load("sprites/effects/rain.spr");
	Rain.hsprSnow = SPR_Load("sprites/effects/snowflake.spr");
	Rain.hsprRipple = SPR_Load("sprites/effects/ripple.spr");

	if( !Rain_Initialized )
	{
		HOOK_MESSAGE_FUNC( "ReceiveW", __MsgFunc_ReceiveW );

		Rain_Initialized = Rain.hsprRain && Rain.hsprSnow && Rain.hsprRipple;
	}
}


void SetPoint( float x, float y, float z, float (*matrix)[4])
{
	Vector point( x, y, z ), result;

	VectorTransform( point, matrix, result );

	gEngfuncs.pTriAPI->Vertex3fv( result );
}



/*
=================================
DrawRain

draw raindrips and snowflakes
=================================
*/
void DrawRain( void )
{
	if (FirstChainDrip.p_Next == NULL)
		return; // no drips to draw

	cl_entity_t *player = gEngfuncs.GetLocalPlayer();

	if( Rain.weatherMode == 0 ) // draw rain
	{
		const model_s *pTexture = gEngfuncs.GetSpritePointer( Rain.hsprRain );
		if( !pTexture )
			return;

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pTexture, 0 );
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );

		for( cl_drip_t *Drip = FirstChainDrip.p_Next; Drip; Drip = Drip->p_Next )
		{
			Vector2D toPlayer, shift(Drip->Delta * DRIP_SPRITE_HALFHEIGHT / DRIPSPEED);
			toPlayer.x = (player->origin.x - Drip->origin.x) * DRIP_SPRITE_HALFWIDTH;
			toPlayer.y = (player->origin.y - Drip->origin.y) * DRIP_SPRITE_HALFWIDTH;
			toPlayer = toPlayer.Normalize();

			// --- draw triangle --------------------------
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, Drip->alpha );
			gEngfuncs.pTriAPI->Begin( TRI_TRIANGLES );

				gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
				gEngfuncs.pTriAPI->Vertex3f( Drip->origin.x-toPlayer.y - shift.x,
						Drip->origin.y + toPlayer.x - shift.y,
						Drip->origin.z + DRIP_SPRITE_HALFHEIGHT );

				gEngfuncs.pTriAPI->TexCoord2f( 0.5, 1 );
				gEngfuncs.pTriAPI->Vertex3f( Drip->origin.x + shift.x,
						Drip->origin.y + shift.y,
						Drip->origin.z - DRIP_SPRITE_HALFHEIGHT );

				gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
				gEngfuncs.pTriAPI->Vertex3f( Drip->origin.x+toPlayer.y - shift.x,
						Drip->origin.y - toPlayer.x - shift.y,
						Drip->origin.z + DRIP_SPRITE_HALFHEIGHT);

			gEngfuncs.pTriAPI->End();
			// --- draw triangle end ----------------------
		}
	}

	else	// draw snow
	{
		const model_s *pTexture = gEngfuncs.GetSpritePointer( Rain.hsprSnow );
		if( !pTexture )
			return;

		float visibleHeight = Rain.globalHeight - SNOWFADEDIST;
		vec3_t normal;
		float  matrix[3][4];

		gEngfuncs.GetViewAngles( normal );
		AngleMatrix (normal, matrix);	// calc view matrix

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pTexture, 0 );
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );


		for( cl_drip_t *Drip = FirstChainDrip.p_Next; Drip; Drip = Drip->p_Next )
		{
			matrix[0][3] = Drip->origin.x; // write origin to matrix
			matrix[1][3] = Drip->origin.y;
			matrix[2][3] = Drip->origin.z;

			// apply start fading effect
			float alpha = (Drip->origin.z <= visibleHeight) ?
							  Drip->alpha :
							  (((gHUD.m_vecOrigin.z + Rain.heightFromPlayer) - Drip->origin.z) / (float)SNOWFADEDIST) * Drip->alpha;

			// --- draw quad --------------------------
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, alpha );
			gEngfuncs.pTriAPI->Begin( TRI_QUADS );

				gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
				SetPoint(0, SNOW_SPRITE_HALFSIZE, SNOW_SPRITE_HALFSIZE, matrix);

				gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
				SetPoint(0, SNOW_SPRITE_HALFSIZE, -SNOW_SPRITE_HALFSIZE, matrix);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
				SetPoint(0, -SNOW_SPRITE_HALFSIZE, -SNOW_SPRITE_HALFSIZE, matrix);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
				SetPoint(0, -SNOW_SPRITE_HALFSIZE, SNOW_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->End();
			// --- draw quad end ----------------------
		}
	}
}

/*
=================================
DrawFXObjects
=================================
*/
void DrawFXObjects( void )
{

	if( !FirstChainFX.p_Next )
		return;

	const model_s *pTexture = gEngfuncs.GetSpritePointer( Rain.hsprRipple );
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pTexture, 0 );
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );

	// go through objects list
	for( cl_rainfx_t *curFX = FirstChainFX.p_Next; curFX; curFX = curFX->p_Next )
	{
		switch( curFX->type )
		{
		case WATER_LANDING:
		{
			// fadeout
			float alpha = ((curFX->birthTime + curFX->life - Rain.curtime) / curFX->life) * curFX->alpha;
			float size = (Rain.curtime - curFX->birthTime) * MAXRINGHALFSIZE;

			// --- draw quad --------------------------
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, alpha );
			gEngfuncs.pTriAPI->Begin( TRI_QUADS );

				gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
				gEngfuncs.pTriAPI->Vertex3f(curFX->origin.x - size, curFX->origin.y - size, curFX->origin.z);

				gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
				gEngfuncs.pTriAPI->Vertex3f(curFX->origin.x - size, curFX->origin.y + size, curFX->origin.z);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
				gEngfuncs.pTriAPI->Vertex3f(curFX->origin.x + size, curFX->origin.y + size, curFX->origin.z);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
				gEngfuncs.pTriAPI->Vertex3f(curFX->origin.x + size, curFX->origin.y - size, curFX->origin.z);

			gEngfuncs.pTriAPI->End();
			// --- draw quad end ----------------------
		}
		}
	}
}
