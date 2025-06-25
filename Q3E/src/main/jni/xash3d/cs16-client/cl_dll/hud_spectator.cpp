//========= Copyright В© 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include "cl_entity.h"
#include "triangleapi.h"
#include "hltv.h"

#include "pm_shared.h"
#include "pm_defs.h"
#include "pmtrace.h"
#include "parsemsg.h"
#include "entity_types.h"

// these are included for the math functions
#include "com_model.h"
#include "demo_api.h"
#include "event_api.h"
#include "studio_util.h"
#include "screenfade.h"
#include "draw_util.h"

#ifdef _MSC_VER
#pragma warning(disable: 4244)
#endif

extern int		iJumpSpectator;
extern float	vJumpOrigin[3];
extern float	vJumpAngles[3];


extern void V_GetInEyePos(int entity, float * origin, float * angles );
extern void V_ResetChaseCam();
extern void V_GetChasePos(int target, float * cl_angles, float * origin, float * angles);
extern float * GetClientColor( int clientIndex );

extern vec3_t v_origin;		// last view origin
extern vec3_t v_angles;		// last view angle
extern vec3_t v_cl_angles;	// last client/mouse angle
extern vec3_t v_sim_org;	// last sim origin

void SpectatorMode(void)
{
	if ( gEngfuncs.Cmd_Argc() <= 1 )
	{
		gEngfuncs.Con_Printf( "usage:  spec_mode <Main Mode> [<Inset Mode>]\n" );
		return;
	}

	// SetModes() will decide if command is executed on server or local
	if ( gEngfuncs.Cmd_Argc() == 2 )
		gHUD.m_Spectator.SetModes( atoi( gEngfuncs.Cmd_Argv(1) ), -1 );
	else if ( gEngfuncs.Cmd_Argc() == 3 )
		gHUD.m_Spectator.SetModes( atoi( gEngfuncs.Cmd_Argv(1) ), atoi( gEngfuncs.Cmd_Argv(2) )  );
}

void SpectatorSpray(void)
{
	vec3_t forward;

	if ( !gEngfuncs.IsSpectateOnly() )
		return;

	AngleVectors(v_angles,forward,NULL,NULL);
	VectorScale(forward, 128, forward);
	VectorAdd(forward, v_origin, forward);
	pmtrace_t * trace = gEngfuncs.PM_TraceLine( v_origin, forward, PM_TRACELINE_PHYSENTSONLY, 2, -1 );
	if ( trace->fraction != 1.0 )
	{
		char string[128];
		sprintf(string, "drc_spray %.2f %.2f %.2f %i",
				trace->endpos[0], trace->endpos[1], trace->endpos[2], trace->ent );
		gEngfuncs.pfnServerCmd(string);
	}

}
void SpectatorHelp(void)
{
	char *text = CHudTextMessage::BufferedLocaliseTextString( "#Spec_Help_Text" );

	if ( text )
	{
		int len = DrawUtils::ConsoleStringLen( text );

		DrawUtils::DrawConsoleString( (ScreenWidth - len) / 2, ScreenHeight / 3, text );
	}
}

void SpectatorMenu( void )
{
	if ( gEngfuncs.Cmd_Argc() <= 1 )
	{
		gEngfuncs.Con_Printf( "usage:  spec_menu <0|1>\n" );
		return;
	}
	const char *name = "spec_menu_internal";
	char *arg = gEngfuncs.Cmd_Argv(1);

	if( arg[0] == 't' && arg[1] == '\0' )
		gEngfuncs.Cvar_SetValue( name, !gEngfuncs.pfnGetCvarFloat(name) );
	else
		gEngfuncs.Cvar_Set( name, gEngfuncs.Cmd_Argv(1) );
}

void SpecDrawNames( void )
{
	if ( gEngfuncs.Cmd_Argc() <= 1 )
	{
		gEngfuncs.Con_Printf( "usage:  spec_drawnames <0|1>\n" );
		return;
	}
	const char *name = "spec_drawnames_internal";
	char *arg = gEngfuncs.Cmd_Argv(1);

	if( arg[0] == 't' && arg[1] == '\0' )
		gEngfuncs.Cvar_SetValue( name, !gEngfuncs.pfnGetCvarFloat(name) );
	else
		gEngfuncs.Cvar_Set( name, gEngfuncs.Cmd_Argv(1) );}

void SpecDrawCone( void )
{
	if ( gEngfuncs.Cmd_Argc() <= 1 )
	{
		gEngfuncs.Con_Printf( "usage:  spec_drawcone <0|1>\n" );
		return;
	}

	const char *name = "spec_drawcone_internal";
	char *arg = gEngfuncs.Cmd_Argv(1);
	if( arg[0] == 't' && arg[1] == '\0' )
		gEngfuncs.Cvar_SetValue( name, !gEngfuncs.pfnGetCvarFloat(name) );
	else
		gEngfuncs.Cvar_Set( name, gEngfuncs.Cmd_Argv(1) );
}
void SpecDrawStatus( void )
{
	if ( gEngfuncs.Cmd_Argc() <= 1 )
	{
		gEngfuncs.Con_Printf( "usage:  spec_drawstatus <0|1>\n" );
		return;
	}

	const char *name = "spec_drawstatus_internal";
	char *arg = gEngfuncs.Cmd_Argv(1);
	if( arg[0] == 't' && arg[1] == '\0' )
		gEngfuncs.Cvar_SetValue( name, !gEngfuncs.pfnGetCvarFloat(name) );
	else
		gEngfuncs.Cvar_Set( name, gEngfuncs.Cmd_Argv(1) );
}

void SpecAutoDirector( void )
{
	if ( gEngfuncs.Cmd_Argc() <= 1 )
	{
		gEngfuncs.Con_Printf( "usage:  spec_autodirector <0|1>\n" );
		return;
	}

	const char *name = "spec_autodirector_internal";
	char *arg = gEngfuncs.Cmd_Argv(1);

	if( arg[0] == 't' && arg[1] == '\0' )
		gEngfuncs.Cvar_SetValue( name, !gEngfuncs.pfnGetCvarFloat(name) );
	else
		gEngfuncs.Cvar_Set( name, gEngfuncs.Cmd_Argv(1) );
}

void SpecPip( void )
{
	if ( gEngfuncs.Cmd_Argc() <= 1 )
	{
		gEngfuncs.Con_Printf( "usage:  spec_pip <0|1>\n" );
		return;
	}

	const char *name = "spec_pip_internal";
	char *arg = gEngfuncs.Cmd_Argv(1);

	if( arg[0] == 't' && arg[1] == '\0' )
		gEngfuncs.Cvar_SetValue( name, !gEngfuncs.pfnGetCvarFloat(name) );
	else
		gEngfuncs.Cvar_Set( name, gEngfuncs.Cmd_Argv(1) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHudSpectator::Init()
{
	gHUD.AddHudElem(this);

	m_iFlags |= HUD_DRAW | HUD_THINK;
	m_flNextObserverInput = 0.0f;
	m_zoomDelta	= 0.0f;
	m_moveDelta = 0.0f;
	m_chatEnabled = (gHUD.m_SayText.m_HUD_saytext->value!=0);
	iJumpSpectator	= 0;

	memset( &m_OverviewData, 0, sizeof(m_OverviewData));
	memset( &m_OverviewEntities, 0, sizeof(m_OverviewEntities));
	m_lastPrimaryObject = m_lastSecondaryObject = 0;

	gEngfuncs.pfnAddCommand ("spec_mode", SpectatorMode );
	gEngfuncs.pfnAddCommand ("spec_decal", SpectatorSpray );
	gEngfuncs.pfnAddCommand ("spec_help", SpectatorHelp );
	gEngfuncs.pfnAddCommand ("spec_menu", SpectatorMenu );
	gEngfuncs.pfnAddCommand ("spec_drawnames", SpecDrawNames );
	gEngfuncs.pfnAddCommand ("spec_drawcone", SpecDrawCone );
	gEngfuncs.pfnAddCommand ("spec_drawstatus", SpecDrawStatus );
	gEngfuncs.pfnAddCommand ("spec_autodirector", SpecAutoDirector );
	gEngfuncs.pfnAddCommand ("spec_pip", SpecPip );

	m_drawnames		= gEngfuncs.pfnRegisterVariable("spec_drawnames_internal","1",0);
	m_drawcone		= gEngfuncs.pfnRegisterVariable("spec_drawcone_internal","1",0);
	m_drawstatus	= gEngfuncs.pfnRegisterVariable("spec_drawstatus_internal","1",0);
	m_autoDirector	= gEngfuncs.pfnRegisterVariable("spec_autodirector_internal","1",0);
	m_pip			= gEngfuncs.pfnRegisterVariable("spec_pip_internal","1",0);
	m_lastAutoDirector = 0.0f;
	
	if ( !m_drawnames || !m_drawcone || !m_drawstatus || !m_autoDirector || !m_pip )
	{
		gEngfuncs.Con_Printf("ERROR! Couldn't register all spectator variables.\n");
		return 0;
	}

	return 1;
}


//-----------------------------------------------------------------------------
// UTIL_StringToVector originally from ..\dlls\util.cpp, slightly changed
//-----------------------------------------------------------------------------

void UTIL_StringToVector( float * pVector, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	strncpy( tempString, pString, sizeof( tempString ) );
	tempString[ sizeof( tempString ) - 1 ] = '\0';
	pstr = pfront = tempString;
	
	for ( j = 0; j < 3; j++ )
	{
		pVector[j] = atof( pfront );
		
		while ( *pstr && *pstr != ' ' )
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	if (j < 2)
	{
		for (j = j+1;j < 3; j++)
			pVector[j] = 0;
	}
}

int UTIL_FindEntityInMap( const char * name, float * origin, float * angle)
{
	int				n,found = 0;
	char			keyname[256];
	char			token[1024];

	cl_entity_t *	pEnt = gEngfuncs.GetEntityByIndex( 0 );	// get world model

	if ( !pEnt ) return 0;

	if ( !pEnt->model )	return 0;

	char * data = pEnt->model->entities;

	while (data)
	{
		data = gEngfuncs.COM_ParseFile(data, token);
		
		if ( (token[0] == '}') ||  (token[0]==0) )
			break;

		if (!data)
		{
			gEngfuncs.Con_DPrintf("UTIL_FindEntityInMap: EOF without closing brace\n");
			return 0;
		}

		if (token[0] != '{')
		{
			gEngfuncs.Con_DPrintf("UTIL_FindEntityInMap: expected {\n");
			return 0;
		}

		// we parse the first { now parse entities properties
		
		while ( 1 )
		{
			// parse key
			data = gEngfuncs.COM_ParseFile(data, token);
			if (token[0] == '}')
				break; // finish parsing this entity

			if (!data)
			{
				gEngfuncs.Con_DPrintf("UTIL_FindEntityInMap: EOF without closing brace\n");
				return 0;
			}
			
			strncpy (keyname, token, sizeof(keyname));
			keyname[sizeof(keyname)-1]=0;

			// another hack to fix keynames with trailing spaces
			n = strlen(keyname);
			while (n && keyname[n-1] == ' ')
			{
				keyname[n-1] = 0;
				n--;
			}
			
			// parse value
			data = gEngfuncs.COM_ParseFile(data, token);
			if (!data)
			{
				gEngfuncs.Con_DPrintf("UTIL_FindEntityInMap: EOF without closing brace\n");
				return 0;
			};

			if (token[0] == '}')
			{
				gEngfuncs.Con_DPrintf("UTIL_FindEntityInMap: closing brace without data");
				return 0;
			}

			if (!strcmp(keyname,"classname"))
			{
				if (!strcmp(token, name ))
				{
					found = 1;	// thats our entity
				}
			};

			if( !strcmp( keyname, "angle" ) )
			{
				float y = atof( token );
				
				if (y >= 0)
				{
					angle[0] = 0.0f;
					angle[1] = y;
				}
				else if ((int)y == -1)
				{
					angle[0] = -90.0f;
					angle[1] =   0.0f;;
				}
				else
				{
					angle[0] = 90.0f;
					angle[1] =  0.0f;
				}

				angle[2] =  0.0f;
			}

			if( !strcmp( keyname, "angles" ) )
			{
				UTIL_StringToVector(angle, token);
			}
			
			if (!strcmp(keyname,"origin"))
			{
				UTIL_StringToVector(origin, token);

			};

		} // while (1)

		if (found)
			return 1;

	}

	return 0;	// we search all entities, but didn't found the correct

}

//-----------------------------------------------------------------------------
// SetSpectatorStartPosition(): 
// Get valid map position and 'beam' spectator to this position
//-----------------------------------------------------------------------------

void CHudSpectator::SetSpectatorStartPosition()
{
	// search for info_player start
	if ( UTIL_FindEntityInMap( "trigger_camera",  m_cameraOrigin, m_cameraAngles ) )
		iJumpSpectator = 1;

	else if ( UTIL_FindEntityInMap( "info_player_start",  m_cameraOrigin, m_cameraAngles ) )
		iJumpSpectator = 1;

	else if ( UTIL_FindEntityInMap( "info_player_deathmatch",  m_cameraOrigin, m_cameraAngles ) )
		iJumpSpectator = 1;

	else if ( UTIL_FindEntityInMap( "info_player_coop",  m_cameraOrigin, m_cameraAngles ) )
		iJumpSpectator = 1;
	else
	{
      static const Vector &nullvec = Vector (0.0, 0.0, 0.0);
		// jump to 0,0,0 if no better position was found
		VectorCopy(nullvec, m_cameraOrigin);
		VectorCopy(nullvec, m_cameraAngles);
	}
	
	VectorCopy(m_cameraOrigin, vJumpOrigin);
	VectorCopy(m_cameraAngles, vJumpAngles);

	iJumpSpectator = 1;	// jump anyway
}

//-----------------------------------------------------------------------------
// Purpose: Loads new icons
//-----------------------------------------------------------------------------
int CHudSpectator::VidInit()
{
	m_hsprPlayer		= SPR_Load("sprites/iplayer.spr");
	m_hsprPlayerBlue	= SPR_Load("sprites/iplayerblue.spr");
	m_hsprPlayerRed		= SPR_Load("sprites/iplayerred.spr");
	m_hsprPlayerDead	= SPR_Load("sprites/iplayerdead.spr");
	m_hsprPlayerVIP		= SPR_Load("sprites/iplayervip.spr");
	m_hsprPlayerC4		= SPR_Load("sprites/iplayerc4.spr");
	m_hsprUnkownMap		= SPR_Load("sprites/tile.spr");
	m_hsprBeam			= SPR_Load("sprites/laserbeam.spr");
	m_hsprCamera		= SPR_Load("sprites/camera.spr");
	m_hsprBomb			= SPR_Load("sprites/ic4.spr");
	m_hsprBackpack		= SPR_Load("sprites/ibackpack.spr");
	m_hsprHostage		= SPR_Load("sprites/ihostage.spr");
	
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flTime - 
//			intermission - 
//-----------------------------------------------------------------------------
int CHudSpectator::Draw(float flTime)
{
	int lx;

	char string[256];
	float * color;

	// draw only in spectator mode
	if ( !g_iUser1  )
		return 0;

	if ( m_lastAutoDirector != m_autoDirector->value )
	{
		m_lastAutoDirector = m_autoDirector->value;
		char cmd[64];
		snprintf(cmd, sizeof(cmd), "spec_set_ad %f", m_autoDirector->value);
		gEngfuncs.pfnClientCmd(cmd);
		if ( m_lastAutoDirector == 0.0 )
		{
			if ( g_iUser1 == OBS_CHASE_LOCKED )
			{
				SetModes(OBS_CHASE_FREE, INSET_OFF);
			}
		}
		else if ( g_iUser1 == OBS_CHASE_FREE )
		{
			SetModes(OBS_CHASE_LOCKED, INSET_OFF);
		}
	}

	// if user pressed zoom, aplly changes
	if ( (m_zoomDelta != 0.0f) && (	g_iUser1 == OBS_MAP_FREE ) )
	{
		m_mapZoom += m_zoomDelta;

		if ( m_mapZoom > 3.0f )
			m_mapZoom = 3.0f;

		if ( m_mapZoom < 0.5f )
			m_mapZoom = 0.5f;
	}

	// if user moves in map mode, change map origin
	if ( (m_moveDelta != 0.0f) && (g_iUser1 != OBS_ROAMING) )
	{
		vec3_t	right;
		AngleVectors(v_angles, NULL, right, NULL);
		VectorNormalize(right);
		VectorScale(right, m_moveDelta, right );

		VectorAdd( m_mapOrigin, right, m_mapOrigin )

	}
	
	// Only draw the icon names only if map mode is in Main Mode
	if ( g_iUser1 < OBS_MAP_FREE  )
		return 1;
	
	if ( !m_drawnames->value )
		return 1;
	
	// make sure we have player info
	//gViewPort->GetAllPlayersInfo();
	gHUD.m_Scoreboard.GetAllPlayersInfo();

	// loop through all the players and draw additional infos to their sprites on the map
	for (int i = 0; i < MAX_PLAYERS; i++)
	{

		if ( m_vPlayerPos[i][2]<0 )	// marked as invisible ?
			continue;

		// can player exist without name?
		if( !g_PlayerInfoList[i+1].name )
			continue;
		
		// check if name would be in inset window
		if ( m_pip->value != INSET_OFF )
		{
			if (	m_vPlayerPos[i][0] > XRES( m_OverviewData.insetWindowX ) &&
					m_vPlayerPos[i][1] > YRES( m_OverviewData.insetWindowY ) &&
					m_vPlayerPos[i][0] < XRES( m_OverviewData.insetWindowX + m_OverviewData.insetWindowWidth ) &&
					m_vPlayerPos[i][1] < YRES( m_OverviewData.insetWindowY + m_OverviewData.insetWindowHeight)
					) continue;
		}

		color = GetClientColor( i+1 ); // ???

		// draw the players name and health underneath
		sprintf(string, "%s", g_PlayerInfoList[i+1].name );
		
		lx = strlen(string)*3; // 3 is avg. character length :)

		DrawUtils::SetConsoleTextColor( color[0], color[1], color[2] );
		DrawUtils::DrawConsoleString( m_vPlayerPos[i][0]-lx,m_vPlayerPos[i][1], string);
		
	}


	// Only draw the overview if Map Mode is selected for this view
	if ( m_iDrawCycle == 0 &&  ( (g_iUser1 != OBS_MAP_FREE) && (g_iUser1 != OBS_MAP_CHASE) ) )
		return 1;

	if ( m_iDrawCycle == 1 && m_pip->value < INSET_MAP_FREE )
		return 1;

	return 1;
}


void CHudSpectator::DirectorMessage( int iSize, void *pbuf )
{
	float	value;
	char *	string;

	BufferReader reader( "DRCMsg", pbuf, iSize );

	int cmd = reader.ReadByte();

	switch ( cmd )	// director command byte
	{
	case DRC_CMD_START	:
		// now we have to do some things clientside, since the proxy doesn't know our mod
		g_iTeamNumber = 0;

		// fake a InitHUD & ResetHUD message
		gHUD.MsgFunc_InitHUD(NULL,0, NULL);
		gHUD.MsgFunc_ResetHUD(NULL, 0, NULL);

		break;

	case DRC_CMD_EVENT	:
		m_lastPrimaryObject		=	reader.ReadWord();
		m_lastSecondaryObject	=	reader.ReadWord();
		m_iObserverFlags		=	reader.ReadLong();

		if ( m_autoDirector->value )
		{
			if ( (g_iUser2 != m_lastPrimaryObject) || (g_iUser3 != m_lastSecondaryObject) )
				V_ResetChaseCam();

			g_iUser2 = m_lastPrimaryObject;
			g_iUser3 = m_lastSecondaryObject;
		}

		// gEngfuncs.Con_Printf("Director Camera: %i %i\n", firstObject, secondObject);
		break;

	case DRC_CMD_MODE  :
		if ( m_autoDirector->value )
		{
			SetModes( reader.ReadByte(), -1 );
		}
		break;

	case DRC_CMD_CAMERA	:
		if ( m_autoDirector->value )
		{
			vJumpOrigin[0] = reader.ReadCoord();	// position
			vJumpOrigin[1] = reader.ReadCoord();
			vJumpOrigin[2] = reader.ReadCoord();

			vJumpAngles[0] = reader.ReadCoord();	// view angle
			vJumpAngles[1] = reader.ReadCoord();
			vJumpAngles[2] = reader.ReadCoord();

			gEngfuncs.SetViewAngles( vJumpAngles );

			iJumpSpectator = 1;
		}
		break;

	case DRC_CMD_MESSAGE:
	{
		client_textmessage_t * msg = &m_HUDMessages[m_lastHudMessage];

		msg->effect = reader.ReadByte();		// effect

		int r, g, b;

		DrawUtils::UnpackRGB( r, g, b, reader.ReadLong() );		// color
		msg->r2 = msg->r1 = bound( 0, r, 255 );
		msg->g2 = msg->g1 = bound( 0, g, 255 );
		msg->b2 = msg->b1 = bound( 0, b, 255 );
		msg->a2 = msg->a1 = 0xFF;	// not transparent

		msg->x = reader.ReadFloat();	// x pos
		msg->y = reader.ReadFloat();	// y pos

		msg->fadein		= reader.ReadFloat();	// fadein
		msg->fadeout	= reader.ReadFloat();	// fadeout
		msg->holdtime	= reader.ReadFloat();	// holdtime
		msg->fxtime		= reader.ReadFloat();	// fxtime;

		strncpy( m_HUDMessageText[m_lastHudMessage], reader.ReadString(), 128 );
		m_HUDMessageText[m_lastHudMessage][127]=0;	// text

		msg->pMessage = m_HUDMessageText[m_lastHudMessage];
		msg->pName	  = "HUD_MESSAGE";

		gHUD.m_Message.MessageAdd( msg );

		m_lastHudMessage++;
		m_lastHudMessage %= MAX_SPEC_HUD_MESSAGES;

	}

		break;

	case DRC_CMD_SOUND :
		string = reader.ReadString();
		value =  reader.ReadFloat();

		// gEngfuncs.Con_Printf("DRC_CMD_FX_SOUND: %s %.2f\n", string, value );
		gEngfuncs.pEventAPI->EV_PlaySound(0, v_origin, CHAN_BODY, string, value, ATTN_NORM, 0, PITCH_NORM );

		break;

	case DRC_CMD_TIMESCALE	:
		value = reader.ReadFloat();
		break;



	case DRC_CMD_STATUS:
		reader.ReadLong(); // total number of spectator slots
		m_iSpectatorNumber = reader.ReadLong(); // total number of spectator
		reader.ReadWord(); // total number of relay proxies

		//gViewPort->UpdateSpectatorPanel();
		break;

	case DRC_CMD_BANNER:
		// gEngfuncs.Con_DPrintf("GUI: Banner %s\n",reader.ReadString() ); // name of banner tga eg gfx/temp/7454562234563475.tga
		//gViewPort->m_pSpectatorPanel->m_TopBanner->LoadImage( reader.ReadString() );
		//gViewPort->UpdateSpectatorPanel();
		break;

		/*case DRC_CMD_FADE:
							break;*/

	case DRC_CMD_STUFFTEXT:
		FilteredClientCmd( reader.ReadString() );
		break;
	default			:	gEngfuncs.Con_DPrintf("CHudSpectator::DirectorMessage: unknown command %i.\n", cmd );
	}
}

void CHudSpectator::FindNextPlayer(bool bReverse)
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.

	int		iStart;
	cl_entity_t * pEnt = NULL;

	// if we are NOT in HLTV mode, spectator targets are set on server
	if ( !gEngfuncs.IsSpectateOnly() )
	{
		char cmdstring[32];
		// forward command to server
		sprintf(cmdstring,"follownext %i",bReverse?1:0);
		gEngfuncs.pfnServerCmd(cmdstring);
		return;
	}
	
	if ( g_iUser2 )
		iStart = g_iUser2;
	else
		iStart = 1;

	g_iUser2 = 0;

	int	    iCurrent = iStart;

	int iDir = bReverse ? -1 : 1;

	// make sure we have player info
	//gViewPort->GetAllPlayersInfo();
	gHUD.m_Scoreboard.GetAllPlayersInfo();

	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if (iCurrent > MAX_PLAYERS)
			iCurrent = 1;
		if (iCurrent < 1)
			iCurrent = MAX_PLAYERS;

		pEnt = gEngfuncs.GetEntityByIndex( iCurrent );

		if ( !IsActivePlayer( pEnt ) )
			continue;

		// MOD AUTHORS: Add checks on target here.

		g_iUser2 = iCurrent;
		break;

	} while ( iCurrent != iStart );

	// Did we find a target?
	if ( !g_iUser2 )
	{
		gEngfuncs.Con_DPrintf( "No observer targets.\n" );
		// take save camera position
		VectorCopy(m_cameraOrigin, vJumpOrigin);
		VectorCopy(m_cameraAngles, vJumpAngles);
	}
	else
	{
		// use new entity position for roaming
		VectorCopy ( pEnt->origin, vJumpOrigin );
		VectorCopy ( pEnt->angles, vJumpAngles );
	}
	iJumpSpectator = 1;
}

void CHudSpectator::HandleButtonsDown( int ButtonPressed )
{
	double time = gEngfuncs.GetClientTime();

	int newMainMode		= g_iUser1;
	int newInsetMode	= m_pip->value;

	// gEngfuncs.Con_Printf(" HandleButtons:%i\n", ButtonPressed );
	//	if ( !gViewPort )

	//Not in intermission.
	if ( gHUD.m_iIntermission )
		return;

	if ( !g_iUser1 )
		return; // don't do anything if not in spectator mode

	// don't handle buttons during normal demo playback
	if ( gEngfuncs.pDemoAPI->IsPlayingback() && !gEngfuncs.IsSpectateOnly() )
		return;
	// Slow down mouse clicks.
	if ( m_flNextObserverInput > time )
		return;

	// enable spectator screen
	if ( ButtonPressed & IN_DUCK )
	{
		gHUD.m_SpectatorGui.UserCmd_ToggleSpectatorMenu();
	}

	//  'Use' changes inset window mode
	if ( ButtonPressed & IN_USE )
	{
		newInsetMode = ToggleInset(true);
	}

	// if not in HLTV mode, buttons are handled server side
	if ( gEngfuncs.IsSpectateOnly() )
	{
		// changing target or chase mode not in overviewmode without inset window

		// Jump changes main window modes
		if ( ButtonPressed & IN_JUMP )
		{
			if ( g_iUser1 == OBS_CHASE_LOCKED )
				newMainMode = OBS_CHASE_FREE;

			else if ( g_iUser1 == OBS_CHASE_FREE )
				newMainMode = OBS_IN_EYE;

			else if ( g_iUser1 == OBS_IN_EYE )
				newMainMode = OBS_ROAMING;

			else if ( g_iUser1 == OBS_ROAMING )
				newMainMode = OBS_MAP_FREE;

			else if ( g_iUser1 == OBS_MAP_FREE )
				newMainMode = OBS_MAP_CHASE;

			else
				newMainMode = OBS_CHASE_FREE;	// don't use OBS_CHASE_LOCKED anymore
		}

		// Attack moves to the next player
		if ( ButtonPressed & (IN_ATTACK | IN_ATTACK2) )
		{
			FindNextPlayer( (ButtonPressed & IN_ATTACK2) ? true:false );

			if ( g_iUser1 == OBS_ROAMING )
			{
				gEngfuncs.SetViewAngles( vJumpAngles );
				iJumpSpectator = 1;

			}
			// lease directed mode if player want to see another player
			m_autoDirector->value = 0.0f;
		}
	}

	SetModes(newMainMode, newInsetMode);

	if ( g_iUser1 == OBS_MAP_FREE )
	{
		if ( ButtonPressed & IN_FORWARD )
			m_zoomDelta =  0.01f;

		if ( ButtonPressed & IN_BACK )
			m_zoomDelta = -0.01f;
		
		if ( ButtonPressed & IN_MOVELEFT )
			m_moveDelta = -12.0f;

		if ( ButtonPressed & IN_MOVERIGHT )
			m_moveDelta =  12.0f;
	}

	m_flNextObserverInput = time + 0.2;
}

void CHudSpectator::HandleButtonsUp( int ButtonPressed )
{
	//if ( !gViewPort )
	return;

	//	if ( !gViewPort->m_pSpectatorPanel->isVisible() )
	//	return; // don't do anything if not in spectator mode

	if ( ButtonPressed & (IN_FORWARD | IN_BACK) )
		m_zoomDelta = 0.0f;
	
	if ( ButtonPressed & (IN_MOVELEFT | IN_MOVERIGHT) )
		m_moveDelta = 0.0f;
}

void CHudSpectator::SetModes(int iNewMainMode, int iNewInsetMode)
{
	// if value == -1 keep old value
	if ( iNewMainMode == -1 )
		iNewMainMode = g_iUser1;

	if ( iNewInsetMode == -1 )
		iNewInsetMode = m_pip->value;

	// inset mode is handled only clients side
	m_pip->value = iNewInsetMode;
	
	if ( iNewMainMode < OBS_CHASE_LOCKED || iNewMainMode > OBS_MAP_CHASE )
	{
		gEngfuncs.Con_Printf("Invalid spectator mode.\n");
		return;
	}
	
	// main modes ettings will override inset window settings
	if ( iNewMainMode != g_iUser1 )
	{
		// if we are NOT in HLTV mode, main spectator mode is set on server
		if ( !gEngfuncs.IsSpectateOnly() )
		{
			return;
		}

		if ( !g_iUser2 && (iNewMainMode != OBS_ROAMING ) )	// make sure we have a target
		{
			// choose last Director object if still available
			if ( IsActivePlayer( gEngfuncs.GetEntityByIndex( m_lastPrimaryObject ) ) )
			{
				g_iUser2 = m_lastPrimaryObject;
				g_iUser3 = m_lastSecondaryObject;
			}
			else
				FindNextPlayer(false); // find any target
		}

		switch ( iNewMainMode )
		{
		case OBS_CHASE_LOCKED:	g_iUser1 = OBS_CHASE_LOCKED;
			break;

		case OBS_CHASE_FREE :	g_iUser1 = OBS_CHASE_FREE;
			break;

		case OBS_ROAMING	:	// jump to current vJumpOrigin/angle
			g_iUser1 = OBS_ROAMING;
			if ( g_iUser2 )
			{
				V_GetChasePos( g_iUser2, v_cl_angles, vJumpOrigin, vJumpAngles );
				gEngfuncs.SetViewAngles( vJumpAngles );
				iJumpSpectator = 1;
			}
			break;

		case OBS_IN_EYE		:	g_iUser1 = OBS_IN_EYE;
			break;

		case OBS_MAP_FREE	:	g_iUser1 = OBS_MAP_FREE;
			// reset user values
			m_mapZoom = m_OverviewData.zoom;
			m_mapOrigin = m_OverviewData.origin;
			break;

		case OBS_MAP_CHASE	:	g_iUser1 = OBS_MAP_CHASE;
			// reset user values
			m_mapZoom = m_OverviewData.zoom;
			m_mapOrigin = m_OverviewData.origin;
			break;
		}

		char string[128];
		sprintf(string, "#Spec_Mode%d", g_iUser1 );
		sprintf(string, "%c%s", HUD_PRINTCENTER, CHudTextMessage::BufferedLocaliseTextString( string ));
		gHUD.m_TextMessage.MsgFunc_TextMsg(NULL, strlen(string)+1, string );
	}

	//gViewPort->UpdateSpectatorPanel();

}

bool CHudSpectator::IsActivePlayer(cl_entity_t * ent)
{
	return ( ent &&
			 ent->player &&
			 ent->curstate.solid != SOLID_NOT &&
			ent != gEngfuncs.GetLocalPlayer() &&
			g_PlayerInfoList[ent->index].name != NULL
			);
}


bool CHudSpectator::ParseOverviewFile( )
{
	char filename[255];
	char levelname[255];
	char token[1024];
	float height;

	char *pfile  = NULL;

	memset( &m_OverviewData, 0, sizeof(m_OverviewData));

	// fill in standrd values
	m_OverviewData.insetWindowX = 4;	// upper left corner
	m_OverviewData.insetWindowY = 4;
	m_OverviewData.insetWindowHeight = 180;
	m_OverviewData.insetWindowWidth = 240;
	m_OverviewData.origin[0] = 0.0f;
	m_OverviewData.origin[1] = 0.0f;
	m_OverviewData.origin[2] = 0.0f;
	m_OverviewData.zoom	= 1.0f;
	m_OverviewData.layers = 0;
	m_OverviewData.layersHeights[0] = 0.0f;
	strncpy( m_OverviewData.map, gEngfuncs.pfnGetLevelName(), sizeof(m_OverviewData.map) );

	if ( strlen( m_OverviewData.map ) == 0 )
		return false; // not active yet

	strncpy(levelname, m_OverviewData.map + 5, sizeof( levelname ));
	levelname[strlen(levelname)-4] = 0;
	
	snprintf(filename, sizeof( filename ), "overviews/%s.txt", levelname );

	pfile = (char *)gEngfuncs.COM_LoadFile( filename, 5, NULL);

	if (!pfile)
	{
		gEngfuncs.Con_Printf("Couldn't open file %s. Using default values for overiew mode.\n", filename );
		return false;
	}
	
	
	while (true)
	{
		pfile = gEngfuncs.COM_ParseFile(pfile, token);

		if (!pfile)
			break;

		if ( !stricmp( token, "global" ) )
		{
			// parse the global data
			pfile = gEngfuncs.COM_ParseFile(pfile, token);
			if ( stricmp( token, "{" ) )
			{
				gEngfuncs.Con_Printf("Error parsing overview file %s. (expected { )\n", filename );
				return false;
			}

			pfile = gEngfuncs.COM_ParseFile(pfile,token);

			while (stricmp( token, "}") )
			{
				if ( !stricmp( token, "zoom" ) )
				{
					pfile = gEngfuncs.COM_ParseFile(pfile,token);
					m_OverviewData.zoom = atof( token );
				}
				else if ( !stricmp( token, "origin" ) )
				{
					pfile = gEngfuncs.COM_ParseFile(pfile, token);
					m_OverviewData.origin[0] = atof( token );
					pfile = gEngfuncs.COM_ParseFile(pfile,token);
					m_OverviewData.origin[1] = atof( token );
					pfile = gEngfuncs.COM_ParseFile(pfile, token);
					m_OverviewData.origin[2] = atof( token );
				}
				else if ( !stricmp( token, "rotated" ) )
				{
					pfile = gEngfuncs.COM_ParseFile(pfile,token);
					m_OverviewData.rotated = atoi( token );
				}
				else if ( !stricmp( token, "inset" ) )
				{
					pfile = gEngfuncs.COM_ParseFile(pfile,token);
					m_OverviewData.insetWindowX = atof( token );
					pfile = gEngfuncs.COM_ParseFile(pfile,token);
					m_OverviewData.insetWindowY = atof( token );
					pfile = gEngfuncs.COM_ParseFile(pfile,token);
					m_OverviewData.insetWindowWidth = atof( token );
					pfile = gEngfuncs.COM_ParseFile(pfile,token);
					m_OverviewData.insetWindowHeight = atof( token );

				}
				else
				{
					gEngfuncs.Con_Printf("Error parsing overview file %s. (%s unknown)\n", filename, token );
					return false;
				}

				pfile = gEngfuncs.COM_ParseFile(pfile,token); // parse next token

			}
		}
		else if ( !stricmp( token, "layer" ) )
		{
			// parse a layer data

			if ( m_OverviewData.layers == OVERVIEW_MAX_LAYERS )
			{
				gEngfuncs.Con_Printf("Error parsing overview file %s. ( too many layers )\n", filename );
				return false;
			}

			pfile = gEngfuncs.COM_ParseFile(pfile,token);


			if ( stricmp( token, "{" ) )
			{
				gEngfuncs.Con_Printf("Error parsing overview file %s. (expected { )\n", filename );
				return false;
			}

			pfile = gEngfuncs.COM_ParseFile(pfile,token);

			while (stricmp( token, "}") )
			{
				if ( !stricmp( token, "image" ) )
				{
					pfile = gEngfuncs.COM_ParseFile(pfile,token);
					strncpy(m_OverviewData.layersImages[ m_OverviewData.layers ], token, 255);
					m_OverviewData.layersImages[ m_OverviewData.layers ][254] = 0;
					
				}
				else if ( !stricmp( token, "height" ) )
				{
					pfile = gEngfuncs.COM_ParseFile(pfile,token);
					height = atof(token);
					m_OverviewData.layersHeights[ m_OverviewData.layers ] = height;
				}
				else
				{
					gEngfuncs.Con_Printf("Error parsing overview file %s. (%s unknown)\n", filename, token );
					return false;
				}

				pfile = gEngfuncs.COM_ParseFile(pfile,token); // parse next token
			}

			m_OverviewData.layers++;

		}
	}

	gEngfuncs.COM_FreeFile( pfile );

	m_mapZoom = m_OverviewData.zoom;
	m_mapOrigin = m_OverviewData.origin;

	return true;

}

void CHudSpectator::LoadMapSprites()
{
	// right now only support for one map layer
	if (m_OverviewData.layers > 0 )
	{
		m_MapSprite = gEngfuncs.LoadMapSprite( m_OverviewData.layersImages[0] );
	}
	else
		m_MapSprite = NULL; // the standard "unknown map" sprite will be used instead
}

// 1 = s, 2 = t, 3 = 2048
static const int st_to_vec[6][3] =
{
{  3, -1,  2 },
{ -3,  1,  2 },
{  1,  3,  2 },
{ -1, -3,  2 },
{ -2, -1,  3 },  // 0 degrees yaw, look straight up
{  2, -1, -3 }   // look straight down
};

void MakeSkyVec( float s, float t, int axis )
{
	int	j, k, farclip;
	vec3_t	v, b;

	farclip = 4096;

	b[0] = s * (farclip >> 1);
	b[1] = t * (farclip >> 1);
	b[2] = (farclip >> 1);

	for( j = 0; j < 3; j++ )
	{
		k = st_to_vec[axis][j];
		v[j] = (k < 0) ? -b[-k-1] : b[k-1];
		// v[j] += RI.cullorigin[j];
	}

	// avoid bilerp seam
	s = (s + 1.0f) * 0.5f;
	t = (t + 1.0f) * 0.5f;

	if( s < 1.0f / 512.0f )
		s = 1.0f / 512.0f;
	else if( s > 511.0f / 512.0f )
		s = 511.0f / 512.0f;
	if( t < 1.0f / 512.0f )
		t = 1.0f / 512.0f;
	else if( t > 511.0f / 512.0f )
		t = 511.0f / 512.0f;

	t = 1.0f - t;

	gEngfuncs.pTriAPI->TexCoord2f( s, t );
	gEngfuncs.pTriAPI->Vertex3fv( v );
}

void CHudSpectator::DrawOverviewLayer()
{
	float screenaspect, xs, ys, xStep, yStep, x,y,z;
	int ix,iy,i,xTiles,yTiles,frame;
	model_t *   dummySprite = (struct model_s *)gEngfuncs.GetSpritePointer( m_hsprUnkownMap);

	if ( m_MapSprite )
	{
		i = m_MapSprite->numframes / (4*3);
		i = sqrt(i);
		xTiles = i*4;
		yTiles = i*3;
	}
	else
	{
		xTiles = 8;
		yTiles = 6;
	}

	screenaspect = ScreenWidth/ScreenHeight;


	xs = m_OverviewData.origin[0];
	ys = m_OverviewData.origin[1];
	z  = ( 90.0f - v_angles[0] ) / 90.0f;
	z *= m_OverviewData.layersHeights[0]; // gOverviewData.z_min - 32;

	// i = r_overviewTexture + ( layer*OVERVIEW_X_TILES*OVERVIEW_Y_TILES );
	/*gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Color4f( 0, 0, 0, 1.0f );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

	float		skyMins[2][6];
	float		skyMaxs[2][6];

	for( i = 0; i < 6; i++ )
	{
		// GL_Bind( XASH_TEXTURE0, tr.skyboxTextures[r_skyTexOrder[i]] );

		skyMins[0][i] = skyMins[1][i] = -1.0f;
		skyMaxs[0][i] = skyMaxs[1][i] = 1.0f;

		gEngfuncs.pTriAPI->Begin( TRI_QUADS );
		MakeSkyVec( skyMins[0][i], skyMins[1][i], i );
		MakeSkyVec( skyMins[0][i], skyMaxs[1][i], i );
		MakeSkyVec( skyMaxs[0][i], skyMaxs[1][i], i );
		MakeSkyVec( skyMaxs[0][i], skyMins[1][i], i );
		gEngfuncs.pTriAPI->End();
	}*/

	gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );	
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	frame = 0;	

	// rotated view ?
	if ( m_OverviewData.rotated )
	{
		xStep = (2*4096.0f / m_OverviewData.zoom ) / xTiles;
		yStep = -(2*4096.0f / (m_OverviewData.zoom* screenaspect) ) / yTiles;

		y = ys + (4096.0f / (m_OverviewData.zoom * screenaspect));

		for (iy = 0; iy < yTiles; iy++)
		{
			x = xs - (4096.0f / (m_OverviewData.zoom));

			for (ix = 0; ix < xTiles; ix++)
			{
				if ( m_MapSprite )
					gEngfuncs.pTriAPI->SpriteTexture( m_MapSprite, frame );
				else
					gEngfuncs.pTriAPI->SpriteTexture( dummySprite, 0 );

				gEngfuncs.pTriAPI->Begin( TRI_QUADS );
				gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
				gEngfuncs.pTriAPI->Vertex3f (x, y, z);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
				gEngfuncs.pTriAPI->Vertex3f (x+xStep ,y,  z);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
				gEngfuncs.pTriAPI->Vertex3f (x+xStep, y+yStep, z);

				gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
				gEngfuncs.pTriAPI->Vertex3f (x, y+yStep, z);
				gEngfuncs.pTriAPI->End();

				frame++;
				x+= xStep;
			}

			y+=yStep;
		}
	}
	else
	{
		xStep = -(2*4096.0f / m_OverviewData.zoom ) / xTiles;
		yStep = -(2*4096.0f / (m_OverviewData.zoom* screenaspect) ) / yTiles;


		x = xs + (4096.0f / (m_OverviewData.zoom * screenaspect ));

		
		
		for (ix = 0; ix < yTiles; ix++)
		{
			
			y = ys + (4096.0f / (m_OverviewData.zoom));

			for (iy = 0; iy < xTiles; iy++)
			{
				if ( m_MapSprite )
					gEngfuncs.pTriAPI->SpriteTexture( m_MapSprite, frame );
				else
					gEngfuncs.pTriAPI->SpriteTexture( dummySprite, 0 );

				gEngfuncs.pTriAPI->Begin( TRI_QUADS );
				gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
				gEngfuncs.pTriAPI->Vertex3f (x, y, z);

				gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
				gEngfuncs.pTriAPI->Vertex3f (x+xStep ,y,  z);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
				gEngfuncs.pTriAPI->Vertex3f (x+xStep, y+yStep, z);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
				gEngfuncs.pTriAPI->Vertex3f (x, y+yStep, z);
				gEngfuncs.pTriAPI->End();

				frame++;
				
				y+=yStep;
			}

			x+= xStep;
			
		}
	}
}

void CHudSpectator::DrawOverviewEntities()
{
	int				i,ir,ig,ib;
	struct model_s *hSpriteModel;
	vec3_t			origin, angles, point, forward, right, left, up, world, screen, offset;
	float			x,y,z, r,g,b, sizeScale = 4.0f;
	cl_entity_t *	ent;
	float rmatrix[3][4];	// transformation matrix
	
	float			zScale = (90.0f - v_angles[0] ) / 90.0f;


	z = m_OverviewData.layersHeights[0] * zScale;
	// get yellow/brown HUD color
	DrawUtils::UnpackRGB( ir, ig, ib, gHUD.m_iDefaultHUDColor );
	r = (float)ir/255.0f;
	g = (float)ig/255.0f;
	b = (float)ib/255.0f;
	
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );

	for (i=0; i < MAX_PLAYERS; i++ )
		m_vPlayerPos[i][2] = -1;	// mark as invisible

	// draw all players
	for (i=0 ; i < MAX_OVERVIEW_ENTITIES ; i++)
	{
		if ( !m_OverviewEntities[i].hSprite )
			continue;

		hSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_OverviewEntities[i].hSprite );
		ent = m_OverviewEntities[i].entity;
		
		gEngfuncs.pTriAPI->SpriteTexture( hSpriteModel, 0 );
		gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );

		// see R_DrawSpriteModel
		// draws players sprite

		AngleVectors(ent->angles, right, up, NULL );

		VectorCopy(ent->origin,origin);

		gEngfuncs.pTriAPI->Begin( TRI_QUADS );

		gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );

		gEngfuncs.pTriAPI->TexCoord2f (1, 0);
		VectorMA (origin,  16.0f * sizeScale, up, point);
		VectorMA (point,   16.0f * sizeScale, right, point);
		point[2] *= zScale;
		gEngfuncs.pTriAPI->Vertex3fv (point);

		gEngfuncs.pTriAPI->TexCoord2f (0, 0);

		VectorMA (origin,  16.0f * sizeScale, up, point);
		VectorMA (point,  -16.0f * sizeScale, right, point);
		point[2] *= zScale;
		gEngfuncs.pTriAPI->Vertex3fv (point);

		gEngfuncs.pTriAPI->TexCoord2f (0,1);
		VectorMA (origin, -16.0f * sizeScale, up, point);
		VectorMA (point,  -16.0f * sizeScale, right, point);
		point[2] *= zScale;
		gEngfuncs.pTriAPI->Vertex3fv (point);

		gEngfuncs.pTriAPI->TexCoord2f (1,1);
		VectorMA (origin, -16.0f * sizeScale, up, point);
		VectorMA (point,   16.0f * sizeScale, right, point);
		point[2] *= zScale;
		gEngfuncs.pTriAPI->Vertex3fv (point);

		gEngfuncs.pTriAPI->End ();


		if ( !ent->player)
			continue;
		// draw line under player icons
		origin[2] *= zScale;

		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );

		hSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_hsprBeam );
		gEngfuncs.pTriAPI->SpriteTexture( hSpriteModel, 0 );

		gEngfuncs.pTriAPI->Color4f(r, g, b, 0.3);

		gEngfuncs.pTriAPI->Begin ( TRI_QUADS );
		gEngfuncs.pTriAPI->TexCoord2f (1, 0);
		gEngfuncs.pTriAPI->Vertex3f (origin[0]+4, origin[1]+4, origin[2]-zScale);
		gEngfuncs.pTriAPI->TexCoord2f (0, 0);
		gEngfuncs.pTriAPI->Vertex3f (origin[0]-4, origin[1]-4, origin[2]-zScale);
		gEngfuncs.pTriAPI->TexCoord2f (0, 1);
		gEngfuncs.pTriAPI->Vertex3f (origin[0]-4, origin[1]-4,z);
		gEngfuncs.pTriAPI->TexCoord2f (1, 1);
		gEngfuncs.pTriAPI->Vertex3f (origin[0]+4, origin[1]+4,z);
		gEngfuncs.pTriAPI->End ();

		gEngfuncs.pTriAPI->Begin ( TRI_QUADS );
		gEngfuncs.pTriAPI->TexCoord2f (1, 0);
		gEngfuncs.pTriAPI->Vertex3f (origin[0]-4, origin[1]+4, origin[2]-zScale);
		gEngfuncs.pTriAPI->TexCoord2f (0, 0);
		gEngfuncs.pTriAPI->Vertex3f (origin[0]+4, origin[1]-4, origin[2]-zScale);
		gEngfuncs.pTriAPI->TexCoord2f (0, 1);
		gEngfuncs.pTriAPI->Vertex3f (origin[0]+4, origin[1]-4,z);
		gEngfuncs.pTriAPI->TexCoord2f (1, 1);
		gEngfuncs.pTriAPI->Vertex3f (origin[0]-4, origin[1]+4,z);
		gEngfuncs.pTriAPI->End ();

		// calculate screen position for name and infromation in hud::draw()
		if ( gEngfuncs.pTriAPI->WorldToScreen(origin,screen) )
			continue;	// object is behind viewer

		screen[0] = XPROJECT(screen[0]);
		screen[1] = YPROJECT(screen[1]);
		screen[2] = 0.0f;

		// calculate some offset under the icon
		origin[0]+=32.0f;
		origin[1]+=32.0f;

		gEngfuncs.pTriAPI->WorldToScreen(origin,offset);

		offset[0] = XPROJECT(offset[0]);
		offset[1] = YPROJECT(offset[1]);
		offset[2] = 0.0f;

		VectorSubtract(offset, screen, offset );

		int playerNum = ent->index - 1;

		m_vPlayerPos[playerNum][0] = screen[0];
		m_vPlayerPos[playerNum][1] = screen[1] + offset.Length();
		m_vPlayerPos[playerNum][2] = 1;	// mark player as visible
	}

	if ( !m_pip->value || !m_drawcone->value )
		return;

	// get current camera position and angle

	if ( m_pip->value == INSET_IN_EYE || g_iUser1 == OBS_IN_EYE )
	{
		V_GetInEyePos( g_iUser2, origin, angles );
	}
	else if ( m_pip->value == INSET_CHASE_FREE  || g_iUser1 == OBS_CHASE_FREE )
	{
		V_GetChasePos( g_iUser2, v_cl_angles, origin, angles );
	}
	else if ( g_iUser1 == OBS_ROAMING )
	{
		VectorCopy( v_sim_org, origin );
		VectorCopy( v_cl_angles, angles );
	}
	else
		V_GetChasePos( g_iUser2, NULL, origin, angles );

	
	// draw camera sprite

	x = origin[0];
	y = origin[1];
	z = origin[2];

	angles[0] = 0; // always show horizontal camera sprite

	hSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_hsprCamera );
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->SpriteTexture( hSpriteModel, 0 );
	
	
	gEngfuncs.pTriAPI->Color4f( r, g, b, 1.0 );

	AngleVectors(angles, forward, NULL, NULL );
	VectorScale (forward, 512.0f, forward);
	
	offset[0] =  0.0f;
	offset[1] = 45.0f;
	offset[2] =  0.0f;

	AngleMatrix(offset, rmatrix );
	VectorTransform(forward, rmatrix , right );

	offset[1]= -45.0f;
	AngleMatrix(offset, rmatrix );
	VectorTransform(forward, rmatrix , left );

	gEngfuncs.pTriAPI->Begin (TRI_TRIANGLES);
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f (x+right[0], y+right[1], (z+right[2]) * zScale);

	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f (x, y, z  * zScale);

	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f (x+left[0], y+left[1], (z+left[2]) * zScale);
	gEngfuncs.pTriAPI->End ();

}



void CHudSpectator::DrawOverview()
{
	static bool glClearForce = false;
	static float old_glClearValue;

	// draw only in sepctator mode
	if ( !g_iUser1 || (m_iDrawCycle == 0 &&  ( (g_iUser1 != OBS_MAP_FREE) && (g_iUser1 != OBS_MAP_CHASE) )) || (m_iDrawCycle == 1 && m_pip->value < INSET_MAP_FREE) )
	{
		// fix non clearing background for overview
		if( glClearForce )
		{
			gEngfuncs.Cvar_SetValue("gl_clear", old_glClearValue );
			glClearForce = false;
		}
		return;
	}

	// fix non clearing background for overview
	if( !glClearForce )
	{
		old_glClearValue = CVAR_GET_FLOAT("gl_clear");
		gEngfuncs.Cvar_Set("gl_clear", "1");
		glClearForce = true;
	}

	DrawOverviewLayer();
	DrawOverviewEntities();
	CheckOverviewEntities();
}
void CHudSpectator::CheckOverviewEntities()
{
	double time = gEngfuncs.GetClientTime();

	// removes old entities from list
	for ( int i = 0; i< MAX_OVERVIEW_ENTITIES; i++ )
	{
		// remove entity from list if it is too old
		if ( m_OverviewEntities[i].killTime < time )
		{
			memset( &m_OverviewEntities[i], 0, sizeof (overviewEntity_t) );
		}
	}
}

bool CHudSpectator::AddOverviewEntity( int type, struct cl_entity_s *ent, const char *modelname)
{
	HSPRITE	hSprite = 0;
	double  duration = -1.0f;	// duration -1 means show it only this frame;

	if ( !ent )
		return false;

	if ( type == ET_PLAYER )
	{
		if ( ent->curstate.solid != SOLID_NOT)
		{
			switch ( g_PlayerExtraInfo[ent->index].teamnumber )
			{
			// blue and red teams are swapped in CS and TFC
			case TEAM_TERRORIST: hSprite = m_hsprPlayerRed; break;
			case TEAM_CT: hSprite = m_hsprPlayerBlue; break;
			default: hSprite = m_hsprPlayer; break;
			}

			if( g_PlayerExtraInfo[ent->index].has_c4 )
				hSprite = m_hsprPlayerC4;
			else if( g_PlayerExtraInfo[ent->index].vip )
				hSprite = m_hsprPlayerVIP;

			return AddOverviewEntityToList(hSprite, ent, gEngfuncs.GetClientTime() + duration );
		}
		else
			return false;	// it's an spectator
	}
	/*else if (type == ET_NORMAL)
	{
		return false;
	}
	else
	{
		else return false;
	}*/

	if( !stricmp( modelname, "models/w_c4.mdl" ))
		hSprite = m_hsprBomb;
	else if( !stricmp( modelname, "models/w_backpack.mdl" ))
		hSprite = m_hsprBackpack;
	else if( strstr( modelname, "models/hostage") || strstr( modelname, "models/scientist"))
		hSprite = m_hsprHostage;
	else return false;

	return AddOverviewEntityToList(hSprite, ent, gEngfuncs.GetClientTime() + duration );
}

void CHudSpectator::DeathMessage(int victim)
{
	// find out where the victim is
	cl_entity_t *pl = gEngfuncs.GetEntityByIndex(victim);

	if (pl && pl->player)
		AddOverviewEntityToList(m_hsprPlayerDead, pl, gEngfuncs.GetClientTime() + 2.0f );
}

bool CHudSpectator::AddOverviewEntityToList(HSPRITE sprite, cl_entity_t *ent, double killTime)
{
	for ( int i = 0; i< MAX_OVERVIEW_ENTITIES; i++ )
	{
		// find empty entity slot
		if ( m_OverviewEntities[i].entity == NULL)
		{
			m_OverviewEntities[i].entity = ent;
			m_OverviewEntities[i].hSprite = sprite;
			m_OverviewEntities[i].killTime = killTime;
			return true;
		}
	}

	return false;	// maximum overview entities reached
}
void CHudSpectator::CheckSettings()
{
	// disallow same inset mode as main mode:

	m_pip->value = (int)m_pip->value;
	
	if ( ( g_iUser1 < OBS_MAP_FREE ) && ( m_pip->value == INSET_CHASE_FREE || m_pip->value == INSET_IN_EYE ) )
	{
		// otherwise both would show in World picures
		m_pip->value = INSET_MAP_FREE;
	}

	if ( ( g_iUser1 >= OBS_MAP_FREE ) && ( m_pip->value >= INSET_MAP_FREE ) )
	{
		// both would show map views
		m_pip->value = INSET_CHASE_FREE;
	}

	// disble in intermission screen
	if ( gHUD.m_iIntermission )
		m_pip->value = INSET_OFF;

	// check chat mode
	if ( m_chatEnabled != (gHUD.m_SayText.m_HUD_saytext->value!=0) )
	{
		// hud_saytext changed
		m_chatEnabled = (gHUD.m_SayText.m_HUD_saytext->value!=0);

		if ( gEngfuncs.IsSpectateOnly() )
		{
			// tell proxy our new chat mode
			char chatcmd[32];
			sprintf(chatcmd, "ignoremsg %i", m_chatEnabled?0:1 );
			gEngfuncs.pfnServerCmd(chatcmd);
		}
	}

	// if we are a real player on server don't allow inset window
	// in First Person mode since this is our restricted forcecamera mode 2
	// team number 3 = SPECTATOR see player.h

	if ( ( (g_iTeamNumber == 1) || (g_iTeamNumber == 2)) && (g_iUser1 == OBS_IN_EYE) )
		m_pip->value = INSET_OFF;

	// draw small border around inset view, adjust upper black bar
	//	gViewPort->m_pSpectatorPanel->EnableInsetView( m_pip->value != INSET_OFF );
}

int CHudSpectator::ToggleInset(bool allowOff)
{
	int newInsetMode = (int)m_pip->value + 1;

	if ( g_iUser1 < OBS_MAP_FREE )
	{
		if ( newInsetMode > INSET_MAP_CHASE )
		{
			if (allowOff)
				newInsetMode = INSET_OFF;
			else
				newInsetMode = INSET_MAP_FREE;
		}

		if ( newInsetMode == INSET_CHASE_FREE )
			newInsetMode = INSET_MAP_FREE;
	}
	else
	{
		if ( newInsetMode > INSET_IN_EYE )
		{
			if (allowOff)
				newInsetMode = INSET_OFF;
			else
				newInsetMode = INSET_CHASE_FREE;
		}
	}

	return newInsetMode;
}
void CHudSpectator::Reset()
{
	// Reset HUD
	if ( strcmp( m_OverviewData.map, gEngfuncs.pfnGetLevelName() ) )
	{
		// update level overview if level changed
		ParseOverviewFile();
		LoadMapSprites();
	}

	memset( &m_OverviewEntities, 0, sizeof(m_OverviewEntities));

	SetSpectatorStartPosition();
}

void CHudSpectator::InitHUDData()
{
	m_lastPrimaryObject = m_lastSecondaryObject = 0;
	m_flNextObserverInput = 0.0f;
	m_lastHudMessage = 0;
	m_iSpectatorNumber = 0;
	iJumpSpectator	= 0;
	g_iUser1 = g_iUser2 = 0;

	memset( &m_OverviewData, 0, sizeof(m_OverviewData));
	memset( &m_OverviewEntities, 0, sizeof(m_OverviewEntities));

	if ( gEngfuncs.IsSpectateOnly() || gEngfuncs.pDemoAPI->IsPlayingback() )
		m_autoDirector->value = 1.0f;
	else
		m_autoDirector->value = 0.0f;

	Reset();

	SetModes( OBS_CHASE_FREE, INSET_OFF );

	g_iUser2 = 0; // fake not target until first camera command

	// reset HUD FOV
	gHUD.m_iFOV =  CVAR_GET_FLOAT("default_fov");
}

int CHudSpectator::MsgFunc_Spectator(const char *pszName, int iSize, void *buf)
{
	return 1;
}

int CHudSpectator::MsgFunc_ADStop(const char *pszName, int iSize, void *buf)
{
	m_autoDirector->value = 0;
	return 1;
}
