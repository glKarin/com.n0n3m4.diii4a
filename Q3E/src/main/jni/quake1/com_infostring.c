/*
Copyright (C) 2006-2021 DarkPlaces contributors

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

#include "darkplaces.h"

size_t InfoString_GetValue(const char *buffer, const char *key, char *value, size_t valuesize)
{
	unsigned pos = 0, j;
	size_t keylength;

	if (!key)
		key = "";
	keylength = strlen(key);
	if (valuesize < 1 || !value)
	{
		Con_Printf("InfoString_GetValue: no room in value\n");
		return 0;
	}
	value[0] = '\0';
	if (strchr(key, '\\'))
	{
		Con_Printf("InfoString_GetValue: key name \"%s\" contains \\ which is not possible in an infostring\n", key);
		return 0;
	}
	if (strchr(key, '\"'))
	{
		Con_Printf("InfoString_SetValue: key name \"%s\" contains \" which is not allowed in an infostring\n", key);
		return 0;
	}
	if (!key[0])
	{
		Con_Printf("InfoString_GetValue: can not look up a key with no name\n");
		return 0;
	}
	while (buffer[pos] == '\\')
	{
		if (!memcmp(buffer + pos+1, key, keylength) &&
				(buffer[pos+1 + keylength] == 0 ||
				 buffer[pos+1 + keylength] == '\\'))
		{
			pos += 1 + keylength;           // Skip \key
			if (buffer[pos] == '\\') pos++; // Skip \ before value.
			for (j = 0;buffer[pos+j] && buffer[pos+j] != '\\' && j < valuesize - 1;j++)
				value[j] = buffer[pos+j];
			value[j] = '\0';
			return j;
		}
		if (buffer[pos] == '\\') pos++; // Skip \ before value.
		for (pos++;buffer[pos] && buffer[pos] != '\\';pos++);
		if (buffer[pos] == '\\') pos++; // Skip \ before value.
		for (pos++;buffer[pos] && buffer[pos] != '\\';pos++);
	}
	// if we reach this point the key was not found
	return 0;
}

void InfoString_SetValue(char *buffer, size_t bufferlength, const char *key, const char *value)
{
	int pos = 0, pos2;
	size_t keylength;
	if (!key)
		key = "";
	if (!value)
		value = "";
	keylength = strlen(key);
	if (strchr(key, '\\') || strchr(value, '\\'))
	{
		Con_Printf("InfoString_SetValue: \"%s\" \"%s\" contains \\ which is not possible to store in an infostring\n", key, value);
		return;
	}
	if (strchr(key, '\"') || strchr(value, '\"'))
	{
		Con_Printf("InfoString_SetValue: \"%s\" \"%s\" contains \" which is not allowed in an infostring\n", key, value);
		return;
	}
	if (!key[0])
	{
		Con_Printf("InfoString_SetValue: can not set a key with no name\n");
		return;
	}
	while (buffer[pos] == '\\')
	{
		if (!memcmp(buffer + pos+1, key, keylength) &&
				(buffer[pos+1 + keylength] == 0 ||
				 buffer[pos+1 + keylength] == '\\'))
			break;
		if (buffer[pos] == '\\') pos++; // Skip \ before value.
		for (;buffer[pos] && buffer[pos] != '\\';pos++);
		if (buffer[pos] == '\\') pos++; // Skip \ before value.
		for (;buffer[pos] && buffer[pos] != '\\';pos++);
	}
	// if we found the key, find the end of it because we will be replacing it
	pos2 = pos;
	if (buffer[pos] == '\\')
	{
		pos2 += 1 + (int)keylength;  // Skip \key
		if (buffer[pos2] == '\\') pos2++; // Skip \ before value.
		for (;buffer[pos2] && buffer[pos2] != '\\';pos2++);
	}
	if (bufferlength <= pos + 1 + strlen(key) + 1 + strlen(value) + strlen(buffer + pos2))
	{
		Con_Printf("InfoString_SetValue: no room for \"%s\" \"%s\" in infostring\n", key, value);
		return;
	}
	if (value[0])
	{
		// set the key/value and append the remaining text
		char tempbuffer[MAX_INPUTLINE];
		dp_strlcpy(tempbuffer, buffer + pos2, sizeof(tempbuffer));
		dpsnprintf(buffer + pos, bufferlength - pos, "\\%s\\%s%s", key, value, tempbuffer);
	}
	else
	{
		// just remove the key from the text
		dp_strlcpy(buffer + pos, buffer + pos2, bufferlength - pos);
	}
}

void InfoString_Print(char *buffer)
{
	int i;
	char key[MAX_INPUTLINE];
	char value[MAX_INPUTLINE];
	while (*buffer)
	{
		if (*buffer != '\\')
		{
			Con_Printf("InfoString_Print: corrupt string\n");
			return;
		}
		for (buffer++, i = 0;*buffer && *buffer != '\\';buffer++)
			if (i < (int)sizeof(key)-1)
				key[i++] = *buffer;
		key[i] = 0;
		if (*buffer != '\\')
		{
			Con_Printf("InfoString_Print: corrupt string\n");
			return;
		}
		for (buffer++, i = 0;*buffer && *buffer != '\\';buffer++)
			if (i < (int)sizeof(value)-1)
				value[i++] = *buffer;
		value[i] = 0;
		// empty value is an error case
		Con_Printf("%20s %s\n", key, value[0] ? value : "NO VALUE");
	}
}
