/*
radio.cpp -- Radio HUD implementation
Copyright (C) 2015-2016 a1batross
This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

In addition, as a special exception, the author gives permission to
link the code of this program with the Half-Life Game Engine ("HL
Engine") and Modified Game Libraries ("MODs") developed by Valve,
L.L.C ("Valve").  You must obey the GNU General Public License in all
respects for all of the code used other than the HL Engine and MODs
from Valve.  If you modify this file, you may extend this exception
to your version of the file, but you are not obligated to do so.  If
you do not wish to do so, delete this exception statement from your
version.
*/
#include "hud.h"
#include "parsemsg.h"
#include "cl_util.h"
#include "r_efx.h"
#include "event_api.h"
#include "com_model.h"
#include <string.h>

int CHudRadio::Init( )
{
	HOOK_MESSAGE( gHUD.m_Radio, SendAudio );
	HOOK_MESSAGE( gHUD.m_Radio, ReloadSound );
	HOOK_MESSAGE( gHUD.m_Radio, BotVoice );
	gHUD.AddHudElem( this );
	m_iFlags = 0;
	return 1;
}


void Broadcast( const char *msg, int pitch )
{
	if ( msg[0] == '%' && msg[1] == '!' )
		gEngfuncs.pfnPlaySoundVoiceByName( &msg[1], 1.0f, pitch );
	else
		gEngfuncs.pfnPlaySoundVoiceByName( msg, 1.0f, pitch );
}

int CHudRadio::MsgFunc_SendAudio( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int SenderID = reader.ReadByte( );
	char *sentence = reader.ReadString( );
	int pitch = reader.ReadShort( );

	Broadcast( sentence, pitch );

	if( SenderID <= MAX_PLAYERS )
	{
		g_PlayerExtraInfo[SenderID].radarflashes = 22;
		g_PlayerExtraInfo[SenderID].radarflashtime = gHUD.m_flTime;
		g_PlayerExtraInfo[SenderID].radarflashtimedelta = 0.5f;
	}
	return 1;
}

int CHudRadio::MsgFunc_ReloadSound( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int vol = reader.ReadByte( );
	if ( reader.ReadByte( ) )
	{
		gEngfuncs.pfnPlaySoundByName( "weapons/generic_reload.wav", vol / 255.0f );
	}
	else
	{
		gEngfuncs.pfnPlaySoundByName( "weapons/generic_shot_reload.wav", vol / 255.0f );
	}
	return 1;
}


static void VoiceIconCallback(struct tempent_s *ent, float frametime, float currenttime)
{
	int entIndex = ent->clientIndex;
	if( !g_PlayerExtraInfo[entIndex].talking )
	{
		g_PlayerExtraInfo[entIndex].talking = false;
		ent->die = 0.0f;
	}
}

void CHudRadio::Voice(int entindex, bool bTalking)
{
	extra_player_info_t *pplayer;
	TEMPENTITY *temp;
	int spr;

	if( entindex < 0 || entindex > MAX_PLAYERS - 1) // bomb can't talk!
		return;

	pplayer = g_PlayerExtraInfo + entindex;

	if( bTalking == pplayer->talking )
		return; // user is talking already

	if( !bTalking && pplayer->talking )
	{
		pplayer->talking = false;
		return; // stop talking
	}

	spr = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/voiceicon.spr" );
	if( !spr ) return;

	temp = gEngfuncs.pEfxAPI->R_DefaultSprite( vec3_origin, spr, 0 );
	if( !temp ) return;

	pplayer->talking = true; // sprite is created

	temp->flags = FTENT_SPRANIMATELOOP | FTENT_CLIENTCUSTOM | FTENT_PLYRATTACHMENT;
	temp->tentOffset.z = 40;
	temp->clientIndex = entindex;
	temp->callback = VoiceIconCallback;
	temp->entity.curstate.scale = 0.60f;
	temp->entity.curstate.rendermode = kRenderTransAdd;
	temp->die = gHUD.m_flTime + 60.0f; // 60 seconds must be enough?
}

int CHudRadio::MsgFunc_BotVoice( const char *pszName, int iSize, void *buf )
{
	BufferReader reader( pszName, buf, iSize );

	int enable   = reader.ReadByte();
	int entIndex = reader.ReadByte();

	HUD_VoiceStatus( entIndex, enable );

	// Voice( entIndex, enable );

	return 1;
}
