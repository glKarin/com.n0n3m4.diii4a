/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2000-2020 DarkPlaces contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// com_msg.c

#include "quakedef.h"

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

/* Casting to unsigned when shifting by 24 bits here is necessary to prevent UB
 * caused by shifting outside the range of int on platforms where int is 32 bits.
 */

float BuffBigFloat (const unsigned char *buffer)
{
	union
	{
		float f;
		unsigned int i;
	}
	u;
	u.i = ((unsigned)buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
	return u.f;
}

int BuffBigLong (const unsigned char *buffer)
{
	return ((unsigned)buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

short BuffBigShort (const unsigned char *buffer)
{
	return (buffer[0] << 8) | buffer[1];
}

float BuffLittleFloat (const unsigned char *buffer)
{
	union
	{
		float f;
		unsigned int i;
	}
	u;
	u.i = ((unsigned)buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
	return u.f;
}

int BuffLittleLong (const unsigned char *buffer)
{
	return ((unsigned)buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}

short BuffLittleShort (const unsigned char *buffer)
{
	return (buffer[1] << 8) | buffer[0];
}

void StoreBigLong (unsigned char *buffer, unsigned int i)
{
	buffer[0] = (i >> 24) & 0xFF;
	buffer[1] = (i >> 16) & 0xFF;
	buffer[2] = (i >>  8) & 0xFF;
	buffer[3] = i         & 0xFF;
}

void StoreBigShort (unsigned char *buffer, unsigned short i)
{
	buffer[0] = (i >>  8) & 0xFF;
	buffer[1] = i         & 0xFF;
}

void StoreLittleLong (unsigned char *buffer, unsigned int i)
{
	buffer[0] = i         & 0xFF;
	buffer[1] = (i >>  8) & 0xFF;
	buffer[2] = (i >> 16) & 0xFF;
	buffer[3] = (i >> 24) & 0xFF;
}

void StoreLittleShort (unsigned char *buffer, unsigned short i)
{
	buffer[0] = i         & 0xFF;
	buffer[1] = (i >>  8) & 0xFF;
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	unsigned char    *buf;

	buf = SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	unsigned char    *buf;

	buf = SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	unsigned char    *buf;

	buf = SZ_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	unsigned char    *buf;

	buf = SZ_GetSpace (sb, 4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = c>>24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float   f;
		int     l;
	} dat;


	dat.f = f;
	dat.l = LittleLong (dat.l);

	SZ_Write (sb, (unsigned char *)&dat.l, 4);
}

void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s || !*s)
		MSG_WriteChar (sb, 0);
	else
		SZ_Write (sb, (unsigned char *)s, (int)strlen(s)+1);
}

void MSG_WriteUnterminatedString (sizebuf_t *sb, const char *s)
{
	if (s && *s)
		SZ_Write (sb, (unsigned char *)s, (int)strlen(s));
}

void MSG_WriteCoord13i (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, Q_rint(f*8));
}

void MSG_WriteCoord16i (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, Q_rint(f));
}

void MSG_WriteCoord32f (sizebuf_t *sb, float f)
{
	MSG_WriteFloat (sb, f);
}

void MSG_WriteCoord (sizebuf_t *sb, float f, protocolversion_t protocol)
{
	if (protocol == PROTOCOL_QUAKE || protocol == PROTOCOL_QUAKEDP || protocol == PROTOCOL_NEHAHRAMOVIE || protocol == PROTOCOL_NEHAHRABJP || protocol == PROTOCOL_NEHAHRABJP2 || protocol == PROTOCOL_NEHAHRABJP3 || protocol == PROTOCOL_QUAKEWORLD)
		MSG_WriteCoord13i (sb, f);
	else if (protocol == PROTOCOL_DARKPLACES1)
		MSG_WriteCoord32f (sb, f);
	else if (protocol == PROTOCOL_DARKPLACES2 || protocol == PROTOCOL_DARKPLACES3 || protocol == PROTOCOL_DARKPLACES4)
		MSG_WriteCoord16i (sb, f);
	else
		MSG_WriteCoord32f (sb, f);
}

void MSG_WriteVector (sizebuf_t *sb, const vec3_t v, protocolversion_t protocol)
{
	MSG_WriteCoord (sb, v[0], protocol);
	MSG_WriteCoord (sb, v[1], protocol);
	MSG_WriteCoord (sb, v[2], protocol);
}

// LadyHavoc: round to nearest value, rather than rounding toward zero, fixes crosshair problem
void MSG_WriteAngle8i (sizebuf_t *sb, float f)
{
	MSG_WriteByte (sb, (int)Q_rint(f*(256.0/360.0)) & 255);
}

void MSG_WriteAngle16i (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, (int)Q_rint(f*(65536.0/360.0)) & 65535);
}

void MSG_WriteAngle32f (sizebuf_t *sb, float f)
{
	MSG_WriteFloat (sb, f);
}

void MSG_WriteAngle (sizebuf_t *sb, float f, protocolversion_t protocol)
{
	if (protocol == PROTOCOL_QUAKE || protocol == PROTOCOL_QUAKEDP || protocol == PROTOCOL_NEHAHRAMOVIE || protocol == PROTOCOL_NEHAHRABJP || protocol == PROTOCOL_NEHAHRABJP2 || protocol == PROTOCOL_NEHAHRABJP3 || protocol == PROTOCOL_DARKPLACES1 || protocol == PROTOCOL_DARKPLACES2 || protocol == PROTOCOL_DARKPLACES3 || protocol == PROTOCOL_DARKPLACES4 || protocol == PROTOCOL_QUAKEWORLD)
		MSG_WriteAngle8i (sb, f);
	else
		MSG_WriteAngle16i (sb, f);
}

//
// reading functions
//

void MSG_InitReadBuffer (sizebuf_t *buf, unsigned char *data, int size)
{
	memset(buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = buf->cursize = size;
	MSG_BeginReading(buf);
}

void MSG_BeginReading(sizebuf_t *sb)
{
	sb->readcount = 0;
	sb->badread = false;
}

int MSG_ReadLittleShort(sizebuf_t *sb)
{
	if (sb->readcount+2 > sb->cursize)
	{
		sb->badread = true;
		return -1;
	}
	sb->readcount += 2;
	return (short)(sb->data[sb->readcount-2] | (sb->data[sb->readcount-1]<<8));
}

int MSG_ReadBigShort (sizebuf_t *sb)
{
	if (sb->readcount+2 > sb->cursize)
	{
		sb->badread = true;
		return -1;
	}
	sb->readcount += 2;
	return (short)((sb->data[sb->readcount-2]<<8) + sb->data[sb->readcount-1]);
}

int MSG_ReadLittleLong (sizebuf_t *sb)
{
	if (sb->readcount+4 > sb->cursize)
	{
		sb->badread = true;
		return -1;
	}
	sb->readcount += 4;
	return sb->data[sb->readcount-4] | (sb->data[sb->readcount-3]<<8) | (sb->data[sb->readcount-2]<<16) | ((unsigned)sb->data[sb->readcount-1]<<24);
}

int MSG_ReadBigLong (sizebuf_t *sb)
{
	if (sb->readcount+4 > sb->cursize)
	{
		sb->badread = true;
		return -1;
	}
	sb->readcount += 4;
	return ((unsigned)sb->data[sb->readcount-4]<<24) + (sb->data[sb->readcount-3]<<16) + (sb->data[sb->readcount-2]<<8) + sb->data[sb->readcount-1];
}

float MSG_ReadLittleFloat (sizebuf_t *sb)
{
	union
	{
		float f;
		int l;
	} dat;
	if (sb->readcount+4 > sb->cursize)
	{
		sb->badread = true;
		return -1;
	}
	sb->readcount += 4;
	dat.l = sb->data[sb->readcount-4] | (sb->data[sb->readcount-3]<<8) | (sb->data[sb->readcount-2]<<16) | ((unsigned)sb->data[sb->readcount-1]<<24);
	return dat.f;
}

float MSG_ReadBigFloat (sizebuf_t *sb)
{
	union
	{
		float f;
		int l;
	} dat;
	if (sb->readcount+4 > sb->cursize)
	{
		sb->badread = true;
		return -1;
	}
	sb->readcount += 4;
	dat.l = ((unsigned)sb->data[sb->readcount-4]<<24) | (sb->data[sb->readcount-3]<<16) | (sb->data[sb->readcount-2]<<8) | sb->data[sb->readcount-1];
	return dat.f;
}

char *MSG_ReadString (sizebuf_t *sb, char *string, size_t maxstring)
{
	size_t l = 0;

	// read string into sbfer, but only store as many characters as will fit
	// if dest buffer is full sb->readcount will still be advanced to end of message string
	while ((string[l] = MSG_ReadByte_opt(sb)) != '\0')
		if (l < maxstring)
			++l;
	return string;
}
size_t MSG_ReadString_len (sizebuf_t *sb, char *string, size_t maxstring)
{
	size_t l = 0;

	// read string into sbfer, but only store as many characters as will fit
	// if dest buffer is full sb->readcount will still be advanced to end of message string
	while ((string[l] = MSG_ReadByte_opt(sb)) != '\0')
		if (l < maxstring)
			++l;
	return l;
}

size_t MSG_ReadBytes (sizebuf_t *sb, size_t numbytes, unsigned char *out)
{
	size_t l = 0;

	// when numbytes have been read sb->readcount won't be advanced any further
	while (l < numbytes && !sb->badread)
		out[l++] = MSG_ReadByte_opt(sb);
	return l;
}

float MSG_ReadCoord13i (sizebuf_t *sb)
{
	return MSG_ReadLittleShort(sb) * (1.0/8.0);
}

float MSG_ReadCoord16i (sizebuf_t *sb)
{
	return (signed short) MSG_ReadLittleShort(sb);
}

float MSG_ReadCoord32f (sizebuf_t *sb)
{
	return MSG_ReadLittleFloat(sb);
}

float MSG_ReadCoord (sizebuf_t *sb, protocolversion_t protocol)
{
	if (protocol == PROTOCOL_QUAKE || protocol == PROTOCOL_QUAKEDP || protocol == PROTOCOL_NEHAHRAMOVIE || protocol == PROTOCOL_NEHAHRABJP || protocol == PROTOCOL_NEHAHRABJP2 || protocol == PROTOCOL_NEHAHRABJP3 || protocol == PROTOCOL_QUAKEWORLD)
		return MSG_ReadCoord13i(sb);
	else if (protocol == PROTOCOL_DARKPLACES1)
		return MSG_ReadCoord32f(sb);
	else if (protocol == PROTOCOL_DARKPLACES2 || protocol == PROTOCOL_DARKPLACES3 || protocol == PROTOCOL_DARKPLACES4)
		return MSG_ReadCoord16i(sb);
	else
		return MSG_ReadCoord32f(sb);
}

void MSG_ReadVector (sizebuf_t *sb, vec3_t v, protocolversion_t protocol)
{
	v[0] = MSG_ReadCoord(sb, protocol);
	v[1] = MSG_ReadCoord(sb, protocol);
	v[2] = MSG_ReadCoord(sb, protocol);
}

// LadyHavoc: round to nearest value, rather than rounding toward zero, fixes crosshair problem
float MSG_ReadAngle8i (sizebuf_t *sb)
{
	return (signed char) MSG_ReadByte (sb) * (360.0/256.0);
}

float MSG_ReadAngle16i (sizebuf_t *sb)
{
	return (signed short)MSG_ReadShort (sb) * (360.0/65536.0);
}

float MSG_ReadAngle32f (sizebuf_t *sb)
{
	return MSG_ReadFloat (sb);
}

float MSG_ReadAngle (sizebuf_t *sb, protocolversion_t protocol)
{
	if (protocol == PROTOCOL_QUAKE || protocol == PROTOCOL_QUAKEDP || protocol == PROTOCOL_NEHAHRAMOVIE || protocol == PROTOCOL_NEHAHRABJP || protocol == PROTOCOL_NEHAHRABJP2 || protocol == PROTOCOL_NEHAHRABJP3 || protocol == PROTOCOL_DARKPLACES1 || protocol == PROTOCOL_DARKPLACES2 || protocol == PROTOCOL_DARKPLACES3 || protocol == PROTOCOL_DARKPLACES4 || protocol == PROTOCOL_QUAKEWORLD)
		return MSG_ReadAngle8i (sb);
	else
		return MSG_ReadAngle16i (sb);
}
