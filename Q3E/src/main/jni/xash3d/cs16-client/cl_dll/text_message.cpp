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
// text_message.cpp
//
// implementation of CHudTextMessage class
//
// this class routes messages through titles.txt for localisation
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "vgui_parser.h"
#include "ctype.h"
#include "draw_util.h"

int CHudTextMessage::Init(void)
{
	HOOK_MESSAGE( gHUD.m_TextMessage, TextMsg );

	gHUD.AddHudElem( this );
	m_iFlags = 0;

	return 1;
}

// Searches through the string for any msg names (indicated by a '#')
// any found are looked up in titles.txt and the new message substituted
// the new value is pushed into dst_buffer
char *CHudTextMessage::LocaliseTextString( const char *msg, char *dst_buffer, int buffer_size )
{
	int len = buffer_size;
	char *dst = dst_buffer;
	for ( char *src = (char*)msg; *src != 0 && buffer_size > 0; buffer_size-- )
	{
		if ( *src == '#' )
		{
			// cut msg name out of string
			static char word_buf[255];
			char *wdst = word_buf, *word_start = src;
			for ( ++src ; (*src >= 'A' && *src <= 'z') || (*src >= '0' && *src <= '9'); wdst++, src++ )
			{
				*wdst = *src;
			}
			*wdst = 0;

			// lookup msg name in titles.txt
			client_textmessage_t *clmsg = TextMessageGet( word_buf );
			if ( !clmsg || !(clmsg->pMessage) )
			{
				// look also in vgui2 translations
				const char *str = Localize( word_buf );
				if( str )
				{
					strncpy(dst, str, buffer_size);
					buffer_size = 0;
					continue;
				}
				else
				{
					src = word_start;
					*dst = *src;
					dst++, src++;
					continue;
				}
			}

			if(clmsg->pMessage[0] == '#')
			{
				strncpy(dst, Localize(clmsg->pMessage+1), buffer_size);
				buffer_size = 0;
			}
			else
			{
				// copy string into message over the msg name
				for ( char *wsrc = (char*)clmsg->pMessage; *wsrc != 0; wsrc++, dst++ )
				{
					*dst = *wsrc;
				}
				*dst = 0;
			}
		}
		else
		{
			*dst = *src;
			dst++, src++;
			*dst = 0;
		}
	}

	dst_buffer[len-1] = 0; // ensure null termination
	return dst_buffer;
}

// As above, but with a local static buffer
char *CHudTextMessage::BufferedLocaliseTextString( const char *msg )
{
	static char dst_buffer[1024];
	LocaliseTextString( msg, dst_buffer, 1024 );
	return dst_buffer;
}

// Simplified version of LocaliseTextString;  assumes string is only one word
char *CHudTextMessage::LookupString( char *msg, int *msg_dest )
{
	if ( !msg )
		return (char*)"";

	// '#' character indicates this is a reference to a string in titles.txt, and not the string itself
	if ( msg[0] == '#' ) 
	{
		// this is a message name, so look up the real message
		client_textmessage_t *clmsg = TextMessageGet( msg+1 );

		if ( !clmsg || !(clmsg->pMessage) )
			return (char*)msg; // lookup failed, so return the original string
				
		if ( msg_dest )
		{
			// check to see if titles.txt info overrides msg destination
			// if clmsg->effect is less than 0, then clmsg->effect holds -1 * message_destination
			if ( clmsg->effect < 0 )  // 
				*msg_dest = -clmsg->effect;
		}

		if( clmsg->pMessage[0] == '#')
			return (char *)Localize( clmsg->pMessage + 1);

		return (char*)clmsg->pMessage;
	}
	else
	{  // nothing special about this message, so just return the same string
		return (char*)msg;
	}
}

void StripEndNewlineFromString( char *str )
{
	int s = strlen( str ) - 1;
	if ( str[s] == '\n' || str[s] == '\r' )
		str[s] = 0;
}

// converts all '\r' characters to '\n', so that the engine can deal with the properly
// returns a pointer to str
char* ConvertCRtoNL( char *str )
{
	for ( char *ch = str; *ch != 0; ch++ )
		if ( *ch == '\r' )
			*ch = '\n';
	return str;
}

// Message handler for text messages
// displays a string, looking them up from the titles.txt file, which can be localised
// parameters:
//   byte:   message direction  ( HUD_PRINTCONSOLE, HUD_PRINTNOTIFY, HUD_PRINTCENTER, HUD_PRINTTALK )
//   string: message
// optional parameters:
//   string: message parameter 1
//   string: message parameter 2
//   string: message parameter 3
//   string: message parameter 4
// any string that starts with the character '#' is a message name, and is used to look up the real message in titles.txt
// the next (optional) one to four strings are parameters for that string (which can also be message names if they begin with '#')
#define MAX_TEXTMSG_STRING 512
int CHudTextMessage::MsgFunc_TextMsg( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int msg_dest = reader.ReadByte();
	int clientIdx = -1;

	static char szBuf[6][MAX_TEXTMSG_STRING];
	char *msg_text = LookupString( reader.ReadString(), &msg_dest );
	msg_text = strncpy( szBuf[0], msg_text, MAX_TEXTMSG_STRING );
	szBuf[0][MAX_TEXTMSG_STRING - 1] = 0;

	// keep reading strings and using C format strings for substituting the strings into the localised text string
	for( int i = 1; i <= 4; i++ )
	{
		char *str = LookupString( reader.ReadString() );
		strncpy( szBuf[i], str, MAX_TEXTMSG_STRING );
		szBuf[i][MAX_TEXTMSG_STRING-1] = 0;

		// these strings are meant for subsitution into the main strings, so cull the automatic end newlines
		StripEndNewlineFromString( szBuf[i] );
	}

	char *psz = szBuf[5];

	// Remove numbers after %s.
	// VALVEWHY?
	if( strlen(msg_text) >= 3 )
	{
		for( size_t i = 0; i < strlen(msg_text) - 2; i++)
		{
			if( msg_text[i] == '%' && msg_text[i + 1] == 's' && isdigit(msg_text[i + 2]))
			{
				char *first = &msg_text[i + 2];
				char *second = &msg_text[i + 3];

				size_t len = strlen( second );

				memmove( first, second, strlen( second ));
				first[len] = '\0'; // one character has been removed and string moved, set null terminator
			}
		}
	}


	switch ( msg_dest )
	{
	case HUD_PRINTCENTER:
	{
		snprintf( psz, MAX_TEXTMSG_STRING, msg_text, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );

		ConvertCRtoNL( psz );

		int len = DrawUtils::ConsoleStringLen( psz );

		DrawUtils::DrawConsoleString( (ScreenWidth - len) / 2, ScreenHeight / 3, psz );

		CenterPrint( psz );
		break;
	}
	case HUD_PRINTNOTIFY:
		psz[0] = 1;  // mark this message to go into the notify buffer
		snprintf( psz+1, MAX_TEXTMSG_STRING - 1, msg_text, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		ConsolePrint( ConvertCRtoNL( psz ) );
		break;

	case HUD_PRINTTALK:
		psz[0] = 2; // mark, so SayTextPrint will color it
		snprintf( psz+1, MAX_TEXTMSG_STRING-1, msg_text, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		gHUD.m_SayText.SayTextPrint( ConvertCRtoNL( psz ), 128 );
		break;

	case HUD_PRINTCONSOLE:
		snprintf( psz, MAX_TEXTMSG_STRING, msg_text, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		ConsolePrint( ConvertCRtoNL( psz ) );
		break;

	case HUD_PRINTRADIO:
		psz[0] = 2;
		snprintf( psz + 1, MAX_TEXTMSG_STRING-1, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );

		clientIdx = atoi( szBuf[0] );
		gHUD.m_SayText.SayTextPrint( ConvertCRtoNL( psz ), 128, clientIdx );
		break;
	}

	return 1;
}
