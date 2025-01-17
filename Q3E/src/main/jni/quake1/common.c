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
// common.c -- misc functions used in client and server

#include <stdlib.h>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "quakedef.h"
#include "utf8lib.h"

cvar_t registered = {CF_CLIENT | CF_SERVER | CF_READONLY, "registered","0", "indicates if this is running registered quake (whether gfx/pop.lmp was found)"};
cvar_t cmdline = {CF_CLIENT | CF_SERVER | CF_READONLY, "cmdline","0", "contains commandline the engine was launched with"};

// FIXME: Find a better place for these.
cvar_t cl_playermodel = {CF_CLIENT | CF_SERVER | CF_USERINFO | CF_ARCHIVE, "playermodel", "", "current player model in Nexuiz/Xonotic"};
cvar_t cl_playerskin = {CF_CLIENT | CF_SERVER | CF_USERINFO | CF_ARCHIVE, "playerskin", "", "current player skin in Nexuiz/Xonotic"};

char com_token[MAX_INPUTLINE];
unsigned com_token_len;

//===========================================================================

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
}

unsigned char *SZ_GetSpace (sizebuf_t *buf, int length)
{
	unsigned char *data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Host_Error ("SZ_GetSpace: overflow without allowoverflow set");

		if (length > buf->maxsize)
			Host_Error ("SZ_GetSpace: %i is > full buffer size", length);

		buf->overflowed = true;
		Con_Print("SZ_GetSpace: overflow\n");
		SZ_Clear (buf);
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write (sizebuf_t *buf, const unsigned char *data, int length)
{
	memcpy (SZ_GetSpace(buf,length),data,length);
}

// LadyHavoc: thanks to Fuh for bringing the pure evil of SZ_Print to my
// attention, it has been eradicated from here, its only (former) use in
// all of darkplaces.

static const char *hexchar = "0123456789ABCDEF";
void Com_HexDumpToConsole(const unsigned char *data, int size)
{
	int i, j, n;
	char text[1024];
	char *cur, *flushpointer;
	const unsigned char *d;
	cur = text;
	flushpointer = text + 512;
	for (i = 0;i < size;)
	{
		n = 16;
		if (n > size - i)
			n = size - i;
		d = data + i;
		// print offset
		*cur++ = hexchar[(i >> 12) & 15];
		*cur++ = hexchar[(i >>  8) & 15];
		*cur++ = hexchar[(i >>  4) & 15];
		*cur++ = hexchar[(i >>  0) & 15];
		*cur++ = ':';
		// print hex
		for (j = 0;j < 16;j++)
		{
			if (j < n)
			{
				*cur++ = hexchar[(d[j] >> 4) & 15];
				*cur++ = hexchar[(d[j] >> 0) & 15];
			}
			else
			{
				*cur++ = ' ';
				*cur++ = ' ';
			}
			if ((j & 3) == 3)
				*cur++ = ' ';
		}
		// print text
		for (j = 0;j < 16;j++)
		{
			if (j < n)
			{
				// color change prefix character has to be treated specially
				if (d[j] == STRING_COLOR_TAG)
				{
					*cur++ = STRING_COLOR_TAG;
					*cur++ = STRING_COLOR_TAG;
				}
				else if (d[j] >= (unsigned char) ' ')
					*cur++ = d[j];
				else
					*cur++ = '.';
			}
			else
				*cur++ = ' ';
		}
		*cur++ = '\n';
		i += n;
		if (cur >= flushpointer || i >= size)
		{
			*cur++ = 0;
			Con_Print(text);
			cur = text;
		}
	}
}

void SZ_HexDumpToConsole(const sizebuf_t *buf)
{
	Com_HexDumpToConsole(buf->data, buf->cursize);
}


//============================================================================

/*
==============
COM_Wordwrap

Word wraps a string. The wordWidth function is guaranteed to be called exactly
once for each word in the string, so it may be stateful, no idea what that
would be good for any more. At the beginning of the string, it will be called
for the char 0 to initialize a clean state, and then once with the string " "
(a space) so the routine knows how long a space is.

In case no single character fits into the given width, the wordWidth function
must return the width of exactly one character.

Wrapped lines get the isContinuation flag set and are continuationWidth less wide.

The sum of the return values of the processLine function will be returned.
==============
*/
int COM_Wordwrap(const char *string, size_t length, float continuationWidth, float maxWidth, COM_WordWidthFunc_t wordWidth, void *passthroughCW, COM_LineProcessorFunc processLine, void *passthroughPL)
{
	// Logic is as follows:
	//
	// For each word or whitespace:
	//   Newline found? Output current line, advance to next line. This is not a continuation. Continue.
	//   Space found? Always add it to the current line, no matter if it fits.
	//   Word found? Check if current line + current word fits.
	//     If it fits, append it. Continue.
	//     If it doesn't fit, output current line, advance to next line. Append the word. This is a continuation. Continue.

	qbool isContinuation = false;
	float spaceWidth;
	const char *startOfLine = string;
	const char *cursor = string;
	const char *end = string + length;
	float spaceUsedInLine = 0;
	float spaceUsedForWord;
	int result = 0;
	size_t wordLen;
	size_t dummy;

	dummy = 0;
	wordWidth(passthroughCW, NULL, &dummy, -1);
	dummy = 1;
	spaceWidth = wordWidth(passthroughCW, " ", &dummy, -1);

	for(;;)
	{
		char ch = (cursor < end) ? *cursor : 0;
		switch(ch)
		{
			case 0: // end of string
				result += processLine(passthroughPL, startOfLine, cursor - startOfLine, spaceUsedInLine, isContinuation);
				goto out;
			case '\n': // end of line
				result += processLine(passthroughPL, startOfLine, cursor - startOfLine, spaceUsedInLine, isContinuation);
				isContinuation = false;
				++cursor;
				startOfLine = cursor;
				break;
			case ' ': // space
				++cursor;
				spaceUsedInLine += spaceWidth;
				break;
			default: // word
				wordLen = 1;
				while(cursor + wordLen < end)
				{
					switch(cursor[wordLen])
					{
						case 0:
						case '\n':
						case ' ':
							goto out_inner;
						default:
							++wordLen;
							break;
					}
				}
				out_inner:
				spaceUsedForWord = wordWidth(passthroughCW, cursor, &wordLen, maxWidth - continuationWidth); // this may have reduced wordLen when it won't fit - but this is GOOD. TODO fix words that do fit in a non-continuation line
				if(wordLen < 1) // cannot happen according to current spec of wordWidth
				{
					wordLen = 1;
					spaceUsedForWord = maxWidth + 1; // too high, forces it in a line of itself
				}
				if(spaceUsedInLine + spaceUsedForWord <= maxWidth || cursor == startOfLine)
				{
					// we can simply append it
					cursor += wordLen;
					spaceUsedInLine += spaceUsedForWord;
				}
				else
				{
					// output current line
					result += processLine(passthroughPL, startOfLine, cursor - startOfLine, spaceUsedInLine, isContinuation);
					isContinuation = true;
					startOfLine = cursor;
					cursor += wordLen;
					spaceUsedInLine = continuationWidth + spaceUsedForWord;
				}
		}
	}
	out:

	return result;

/*
	qbool isContinuation = false;
	float currentWordSpace = 0;
	const char *currentWord = 0;
	float minReserve = 0;

	float spaceUsedInLine = 0;
	const char *currentLine = 0;
	const char *currentLineEnd = 0;
	float currentLineFinalWhitespace = 0;
	const char *p;

	int result = 0;
	minReserve = charWidth(passthroughCW, 0);
	minReserve += charWidth(passthroughCW, ' ');

	if(maxWidth < continuationWidth + minReserve)
		maxWidth = continuationWidth + minReserve;

	charWidth(passthroughCW, 0);

	for(p = string; p < string + length; ++p)
	{
		char c = *p;
		float w = charWidth(passthroughCW, c);

		if(!currentWord)
		{
			currentWord = p;
			currentWordSpace = 0;
		}

		if(!currentLine)
		{
			currentLine = p;
			spaceUsedInLine = isContinuation ? continuationWidth : 0;
			currentLineEnd = 0;
		}

		if(c == ' ')
		{
			// 1. I can add the word AND a space - then just append it.
			if(spaceUsedInLine + currentWordSpace + w <= maxWidth)
			{
				currentLineEnd = p; // note: space not included here
				currentLineFinalWhitespace = w;
				spaceUsedInLine += currentWordSpace + w;
			}
			// 2. I can just add the word - then append it, output current line and go to next one.
			else if(spaceUsedInLine + currentWordSpace <= maxWidth)
			{
				result += processLine(passthroughPL, currentLine, p - currentLine, spaceUsedInLine + currentWordSpace, isContinuation);
				currentLine = 0;
				isContinuation = true;
			}
			// 3. Otherwise, output current line and go to next one, where I can add the word.
			else if(continuationWidth + currentWordSpace + w <= maxWidth)
			{
				if(currentLineEnd)
					result += processLine(passthroughPL, currentLine, currentLineEnd - currentLine, spaceUsedInLine - currentLineFinalWhitespace, isContinuation);
				currentLine = currentWord;
				spaceUsedInLine = continuationWidth + currentWordSpace + w;
				currentLineEnd = p;
				currentLineFinalWhitespace = w;
				isContinuation = true;
			}
			// 4. We can't even do that? Then output both current and next word as new lines.
			else
			{
				if(currentLineEnd)
				{
					result += processLine(passthroughPL, currentLine, currentLineEnd - currentLine, spaceUsedInLine - currentLineFinalWhitespace, isContinuation);
					isContinuation = true;
				}
				result += processLine(passthroughPL, currentWord, p - currentWord, currentWordSpace, isContinuation);
				currentLine = 0;
				isContinuation = true;
			}
			currentWord = 0;
		}
		else if(c == '\n')
		{
			// 1. I can add the word - then do it.
			if(spaceUsedInLine + currentWordSpace <= maxWidth)
			{
				result += processLine(passthroughPL, currentLine, p - currentLine, spaceUsedInLine + currentWordSpace, isContinuation);
			}
			// 2. Otherwise, output current line, next one and make tabula rasa.
			else
			{
				if(currentLineEnd)
				{
					processLine(passthroughPL, currentLine, currentLineEnd - currentLine, spaceUsedInLine - currentLineFinalWhitespace, isContinuation);
					isContinuation = true;
				}
				result += processLine(passthroughPL, currentWord, p - currentWord, currentWordSpace, isContinuation);
			}
			currentWord = 0;
			currentLine = 0;
			isContinuation = false;
		}
		else
		{
			currentWordSpace += w;
			if(
				spaceUsedInLine + currentWordSpace > maxWidth // can't join this line...
				&&
				continuationWidth + currentWordSpace > maxWidth // can't join any other line...
			)
			{
				// this word cannot join ANY line...
				// so output the current line...
				if(currentLineEnd)
				{
					result += processLine(passthroughPL, currentLine, currentLineEnd - currentLine, spaceUsedInLine - currentLineFinalWhitespace, isContinuation);
					isContinuation = true;
				}

				// then this word's beginning...
				if(isContinuation)
				{
					// it may not fit, but we know we have to split it into maxWidth - continuationWidth pieces
					float pieceWidth = maxWidth - continuationWidth;
					const char *pos = currentWord;
					currentWordSpace = 0;

					// reset the char width function to a state where no kerning occurs (start of word)
					charWidth(passthroughCW, ' ');
					while(pos <= p)
					{
						float w = charWidth(passthroughCW, *pos);
						if(currentWordSpace + w > pieceWidth) // this piece won't fit any more
						{
							// print everything until it
							result += processLine(passthroughPL, currentWord, pos - currentWord, currentWordSpace, true);
							// go to here
							currentWord = pos;
							currentWordSpace = 0;
						}
						currentWordSpace += w;
						++pos;
					}
					// now we have a currentWord that fits... set up its next line
					// currentWordSpace has been set
					// currentWord has been set
					spaceUsedInLine = continuationWidth;
					currentLine = currentWord;
					currentLineEnd = 0;
					isContinuation = true;
				}
				else
				{
					// we have a guarantee that it will fix (see if clause)
					result += processLine(passthroughPL, currentWord, p - currentWord, currentWordSpace - w, isContinuation);

					// and use the rest of this word as new start of a line
					currentWordSpace = w;
					currentWord = p;
					spaceUsedInLine = continuationWidth;
					currentLine = p;
					currentLineEnd = 0;
					isContinuation = true;
				}
			}
		}
	}

	if(!currentWord)
	{
		currentWord = p;
		currentWordSpace = 0;
	}

	if(currentLine) // Same procedure as \n
	{
		// Can I append the current word?
		if(spaceUsedInLine + currentWordSpace <= maxWidth)
			result += processLine(passthroughPL, currentLine, p - currentLine, spaceUsedInLine + currentWordSpace, isContinuation);
		else
		{
			if(currentLineEnd)
			{
				result += processLine(passthroughPL, currentLine, currentLineEnd - currentLine, spaceUsedInLine - currentLineFinalWhitespace, isContinuation);
				isContinuation = true;
			}
			result += processLine(passthroughPL, currentWord, p - currentWord, currentWordSpace, isContinuation);
		}
	}

	return result;
*/
}

/*
==============
COM_ParseToken_Simple

Parse a token out of a string
Writes the token and its strlen to the com_token and com_token_len globals.
==============
*/
qbool COM_ParseToken_Simple(const char **datapointer, qbool returnnewline, qbool parsebackslash, qbool parsecomments)
{
	int len;
	int c;
	const char *data = *datapointer;

	com_token_len = len = 0;
	com_token[0] = '\0';

	if (!data)
	{
		*datapointer = NULL;
		return false;
	}

// skip whitespace
skipwhite:
	// line endings:
	// UNIX: \n
	// Mac: \r
	// Windows: \r\n
	for (;ISWHITESPACE(*data) && ((*data != '\n' && *data != '\r') || !returnnewline);data++)
	{
		if (*data == 0)
		{
			// end of file
			*datapointer = NULL;
			return false;
		}
	}

	// handle Windows line ending
	if (data[0] == '\r' && data[1] == '\n')
		data++;

	if (parsecomments && data[0] == '/' && data[1] == '/')
	{
		// comment
		while (*data && *data != '\n' && *data != '\r')
			data++;
		goto skipwhite;
	}
	else if (parsecomments && data[0] == '/' && data[1] == '*')
	{
		// comment
		data++;
		while (*data && (data[0] != '*' || data[1] != '/'))
			data++;
		if (*data)
			data++;
		if (*data)
			data++;
		goto skipwhite;
	}
	else if (*data == '\"')
	{
		// quoted string
		for (data++;*data && *data != '\"';data++)
		{
			c = *data;
			if (*data == '\\' && parsebackslash)
			{
				data++;
				c = *data;
				if (c == 'n')
					c = '\n';
				else if (c == 't')
					c = '\t';
			}
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = c;
		}
		com_token[len] = '\0';
		com_token_len = len;
		if (*data == '\"')
			data++;
		*datapointer = data;
		return true;
	}
	else if (*data == '\r')
	{
		// translate Mac line ending to UNIX
		com_token[len++] = '\n';data++;
		com_token[len] = '\0';
		com_token_len = len;
		*datapointer = data;
		return true;
	}
	else if (*data == '\n')
	{
		// single character
		com_token[len++] = *data++;
		com_token[len] = '\0';
		com_token_len = len;
		*datapointer = data;
		return true;
	}
	else
	{
		// regular word
		for (;!ISWHITESPACE(*data);data++)
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = *data;
		com_token[len] = '\0';
		com_token_len = len;
		*datapointer = data;
		return true;
	}
}

/*
==============
COM_ParseToken_QuakeC

Parse a token out of a string
Writes the token and its strlen to the com_token and com_token_len globals.
==============
*/
qbool COM_ParseToken_QuakeC(const char **datapointer, qbool returnnewline)
{
	int len;
	int c;
	const char *data = *datapointer;

	com_token_len = len = 0;
	com_token[0] = '\0';

	if (!data)
	{
		*datapointer = NULL;
		return false;
	}

// skip whitespace
skipwhite:
	// line endings:
	// UNIX: \n
	// Mac: \r
	// Windows: \r\n
	for (;ISWHITESPACE(*data) && ((*data != '\n' && *data != '\r') || !returnnewline);data++)
	{
		if (*data == 0)
		{
			// end of file
			*datapointer = NULL;
			return false;
		}
	}

	// handle Windows line ending
	if (data[0] == '\r' && data[1] == '\n')
		data++;

	if (data[0] == '/' && data[1] == '/')
	{
		// comment
		while (*data && *data != '\n' && *data != '\r')
			data++;
		goto skipwhite;
	}
	else if (data[0] == '/' && data[1] == '*')
	{
		// comment
		data++;
		while (*data && (data[0] != '*' || data[1] != '/'))
			data++;
		if (*data)
			data++;
		if (*data)
			data++;
		goto skipwhite;
	}
	else if (*data == '\"' || *data == '\'')
	{
		// quoted string
		char quote = *data;
		for (data++;*data && *data != quote;data++)
		{
			c = *data;
			if (*data == '\\')
			{
				data++;
				c = *data;
				if (c == 'n')
					c = '\n';
				else if (c == 't')
					c = '\t';
			}
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = c;
		}
		com_token[len] = '\0';
		com_token_len = len;
		if (*data == quote)
			data++;
		*datapointer = data;
		return true;
	}
	else if (*data == '\r')
	{
		// translate Mac line ending to UNIX
		com_token[len++] = '\n';data++;
		com_token[len] = '\0';
		com_token_len = len;
		*datapointer = data;
		return true;
	}
	else if (*data == '\n' || *data == '{' || *data == '}' || *data == ')' || *data == '(' || *data == ']' || *data == '[' || *data == ':' || *data == ',' || *data == ';')
	{
		// single character
		com_token[len++] = *data++;
		com_token[len] = '\0';
		com_token_len = len;
		*datapointer = data;
		return true;
	}
	else
	{
		// regular word
		for (;!ISWHITESPACE(*data) && *data != '{' && *data != '}' && *data != ')' && *data != '(' && *data != ']' && *data != '[' && *data != ':' && *data != ',' && *data != ';';data++)
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = *data;
		com_token[len] = '\0';
		com_token_len = len;
		*datapointer = data;
		return true;
	}
}

/*
==============
COM_ParseToken_VM_Tokenize

Parse a token out of a string
Writes the token and its strlen to the com_token and com_token_len globals.
==============
*/
qbool COM_ParseToken_VM_Tokenize(const char **datapointer, qbool returnnewline)
{
	int len;
	int c;
	const char *data = *datapointer;

	com_token_len = len = 0;
	com_token[0] = '\0';

	if (!data)
	{
		*datapointer = NULL;
		return false;
	}

// skip whitespace
skipwhite:
	// line endings:
	// UNIX: \n
	// Mac: \r
	// Windows: \r\n
	for (;ISWHITESPACE(*data) && ((*data != '\n' && *data != '\r') || !returnnewline);data++)
	{
		if (*data == 0)
		{
			// end of file
			*datapointer = NULL;
			return false;
		}
	}

	// handle Windows line ending
	if (data[0] == '\r' && data[1] == '\n')
		data++;

	if (data[0] == '/' && data[1] == '/')
	{
		// comment
		while (*data && *data != '\n' && *data != '\r')
			data++;
		goto skipwhite;
	}
	else if (data[0] == '/' && data[1] == '*')
	{
		// comment
		data++;
		while (*data && (data[0] != '*' || data[1] != '/'))
			data++;
		if (*data)
			data++;
		if (*data)
			data++;
		goto skipwhite;
	}
	else if (*data == '\"' || *data == '\'')
	{
		char quote = *data;
		// quoted string
		for (data++;*data && *data != quote;data++)
		{
			c = *data;
			if (*data == '\\')
			{
				data++;
				c = *data;
				if (c == 'n')
					c = '\n';
				else if (c == 't')
					c = '\t';
			}
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = c;
		}
		com_token[len] = '\0';
		com_token_len = len;
		if (*data == quote)
			data++;
		*datapointer = data;
		return true;
	}
	else if (*data == '\r')
	{
		// translate Mac line ending to UNIX
		com_token[len++] = '\n';data++;
		com_token[len] = '\0';
		com_token_len = len;
		*datapointer = data;
		return true;
	}
	else if (*data == '\n' || *data == '{' || *data == '}' || *data == ')' || *data == '(' || *data == ']' || *data == '[' || *data == ':' || *data == ',' || *data == ';')
	{
		// single character
		com_token[len++] = *data++;
		com_token[len] = '\0';
		com_token_len = len;
		*datapointer = data;
		return true;
	}
	else
	{
		// regular word
		for (;!ISWHITESPACE(*data) && *data != '{' && *data != '}' && *data != ')' && *data != '(' && *data != ']' && *data != '[' && *data != ':' && *data != ',' && *data != ';';data++)
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = *data;
		com_token[len] = '\0';
		com_token_len = len;
		*datapointer = data;
		return true;
	}
}

/*
==============
COM_ParseToken_Console

Parse a token out of a string, behaving like the qwcl console
Writes the token and its strlen to the com_token and com_token_len globals.
==============
*/
qbool COM_ParseToken_Console(const char **datapointer)
{
	int len;
	const char *data = *datapointer;

	com_token_len = len = 0;
	com_token[0] = '\0';

	if (!data)
	{
		*datapointer = NULL;
		return false;
	}

// skip whitespace
skipwhite:
	for (;ISWHITESPACE(*data);data++)
	{
		if (*data == 0)
		{
			// end of file
			*datapointer = NULL;
			return false;
		}
	}

	if (*data == '/' && data[1] == '/')
	{
		// comment
		while (*data && *data != '\n' && *data != '\r')
			data++;
		goto skipwhite;
	}
	else if (*data == '\"')
	{
		// quoted string
		for (data++;*data && *data != '\"';data++)
		{
			// allow escaped " and \ case
			if (*data == '\\' && (data[1] == '\"' || data[1] == '\\'))
				data++;
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = *data;
		}
		com_token[len] = '\0';
		com_token_len = len;
		if (*data == '\"')
			data++;
		*datapointer = data;
	}
	else
	{
		// regular word
		for (;!ISWHITESPACE(*data);data++)
			if (len < (int)sizeof(com_token) - 1)
				com_token[len++] = *data;
		com_token[len] = '\0';
		com_token_len = len;
		*datapointer = data;
	}

	return true;
}

/*
===============
Com_CalcRoll

Used by view and sv_user
===============
*/
float Com_CalcRoll (const vec3_t angles, const vec3_t velocity, const vec_t angleval, const vec_t velocityval)
{
	vec3_t	forward, right, up;
	float	sign;
	float	side;

	AngleVectors (angles, forward, right, up);
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);

	if (side < velocityval)
		side = side * angleval / velocityval;
	else
		side = angleval;

	return side*sign;

}

//===========================================================================

/*
================
COM_Init
================
*/
void COM_Init_Commands (void)
{
	int i, j, n;
	char com_cmdline[MAX_INPUTLINE];

	Cvar_RegisterVariable (&registered);
	Cvar_RegisterVariable (&cmdline);
	Cvar_RegisterVariable(&cl_playermodel);
	Cvar_RegisterVirtual(&cl_playermodel, "_cl_playermodel");
	Cvar_RegisterVariable(&cl_playerskin);
	Cvar_RegisterVirtual(&cl_playerskin, "_cl_playerskin");

	// reconstitute the command line for the cmdline externally visible cvar
	n = 0;
	for (j = 0;(j < MAX_NUM_ARGVS) && (j < sys.argc);j++)
	{
		i = 0;
		if (strstr(sys.argv[j], " "))
		{
			// arg contains whitespace, store quotes around it
			// This condition checks whether we can allow to put
			// in two quote characters.
			if (n >= ((int)sizeof(com_cmdline) - 2))
				break;
			com_cmdline[n++] = '\"';
			// This condition checks whether we can allow one
			// more character and a quote character.
			while ((n < ((int)sizeof(com_cmdline) - 2)) && sys.argv[j][i])
				// FIXME: Doesn't quote special characters.
				com_cmdline[n++] = sys.argv[j][i++];
			com_cmdline[n++] = '\"';
		}
		else
		{
			// This condition checks whether we can allow one
			// more character.
			while ((n < ((int)sizeof(com_cmdline) - 1)) && sys.argv[j][i])
				com_cmdline[n++] = sys.argv[j][i++];
		}
		if (n < ((int)sizeof(com_cmdline) - 1))
			com_cmdline[n++] = ' ';
		else
			break;
	}
	com_cmdline[n] = 0;
	Cvar_SetQuick(&cmdline, com_cmdline);
}

/*
============
va

varargs print into provided buffer, returns buffer (so that it can be called in-line, unlike dpsnprintf)
============
*/
char *va(char *buf, size_t buflen, const char *format, ...)
{
	va_list argptr;

	va_start (argptr, format);
	dpvsnprintf (buf, buflen, format,argptr);
	va_end (argptr);

	return buf;
}


//======================================

// snprintf and vsnprintf are NOT portable. Use their DP counterparts instead

#undef snprintf
#undef vsnprintf

#ifdef WIN32
# define snprintf _snprintf
# define vsnprintf _vsnprintf
#endif


int dpsnprintf (char *buffer, size_t buffersize, const char *format, ...)
{
	va_list args;
	int result;

	va_start (args, format);
	result = dpvsnprintf (buffer, buffersize, format, args);
	va_end (args);

	return result;
}


int dpvsnprintf (char *buffer, size_t buffersize, const char *format, va_list args)
{
	int result;

#if _MSC_VER >= 1400
	result = _vsnprintf_s (buffer, buffersize, _TRUNCATE, format, args);
#else
	result = vsnprintf (buffer, buffersize, format, args);
#endif
	if (result < 0 || (size_t)result >= buffersize)
	{
		// _vsnprintf_s returns -1 on error and on truncation (indistinguishable),
		// vsnprintf returns negative on error and the desired strlen on truncation.
		buffer[buffersize - 1] = '\0'; // should be unnecessary, but just in case
		// Basic stdout only: we could be inside Con_Printf, Sys_Printf calls dpvsnprintf,
		// Windows console doesn't support colours.
		if (result < 0)
			Sys_Print("dpvsnprintf: output error!\n", 27);
		else
		{
			char msg[MAX_INPUTLINE];
#if _MSC_VER >= 1400
			result = _snprintf_s(msg, sizeof(msg), _TRUNCATE, "dpvsnprintf: truncated to %lu bytes: \"%s\"\n", (unsigned long)buffersize - 1, buffer);
#else
			result = snprintf(msg, sizeof(msg), "dpvsnprintf: truncated to %lu bytes: \"%s\"\n", (unsigned long)buffersize - 1, buffer);
#endif
			if (result > 0)
			{
				msg[sizeof(msg) - 1] = '\n'; // may have been lost in truncation
				Sys_Print(msg, min((size_t)result, sizeof(msg) - 1));
			}
		}
		return -1;
	}

	return result;
}


//======================================

size_t COM_ToLowerString(const char *in, char *out, size_t size_out)
{
	const char *out_start = out;

	if (size_out == 0)
		return 0;

	if(utf8_enable.integer)
	{
		*out = 0;
		while(*in && size_out > 1)
		{
			int n;
			Uchar ch = u8_getchar_utf8_enabled(in, &in);
			ch = u8_tolower(ch);
			n = u8_fromchar(ch, out, size_out);
			out += n; // before the break so the return is correct
			if(n <= 0)
				break;
			size_out -= n;
		}
		return out - out_start;
	}

	while (*in && size_out > 1)
	{
		if (*in >= 'A' && *in <= 'Z')
			*out++ = *in++ + 'a' - 'A';
		else
			*out++ = *in++;
		size_out--;
	}
	*out = '\0';
	return out - out_start;
}

size_t COM_ToUpperString(const char *in, char *out, size_t size_out)
{
	const char *out_start = out;

	if (size_out == 0)
		return 0;

	if(utf8_enable.integer)
	{
		*out = 0;
		while(*in && size_out > 1)
		{
			int n;
			Uchar ch = u8_getchar_utf8_enabled(in, &in);
			ch = u8_toupper(ch);
			n = u8_fromchar(ch, out, size_out);
			out += n; // before the break so the return is correct
			if(n <= 0)
				break;
			size_out -= n;
		}
		return out - out_start;
	}

	while (*in && size_out > 1)
	{
		if (*in >= 'a' && *in <= 'z')
			*out++ = *in++ + 'A' - 'a';
		else
			*out++ = *in++;
		size_out--;
	}
	*out = '\0';
	return out - out_start;
}

int COM_StringBeginsWith(const char *s, const char *match)
{
	for (;*s && *match;s++, match++)
		if (*s != *match)
			return false;
	return true;
}

int COM_ReadAndTokenizeLine(const char **text, char **argv, int maxargc, char *tokenbuf, int tokenbufsize, const char *commentprefix)
{
	int argc, commentprefixlength;
	char *tokenbufend;
	const char *l;
	argc = 0;
	tokenbufend = tokenbuf + tokenbufsize;
	l = *text;
	commentprefixlength = 0;
	if (commentprefix)
		commentprefixlength = (int)strlen(commentprefix);
	while (*l && *l != '\n' && *l != '\r')
	{
		if (!ISWHITESPACE(*l))
		{
			if (commentprefixlength && !strncmp(l, commentprefix, commentprefixlength))
			{
				while (*l && *l != '\n' && *l != '\r')
					l++;
				break;
			}
			if (argc >= maxargc)
				return -1;
			argv[argc++] = tokenbuf;
			if (*l == '"')
			{
				l++;
				while (*l && *l != '"')
				{
					if (tokenbuf >= tokenbufend)
						return -1;
					*tokenbuf++ = *l++;
				}
				if (*l == '"')
					l++;
			}
			else
			{
				while (!ISWHITESPACE(*l))
				{
					if (tokenbuf >= tokenbufend)
						return -1;
					*tokenbuf++ = *l++;
				}
			}
			if (tokenbuf >= tokenbufend)
				return -1;
			*tokenbuf++ = 0;
		}
		else
			l++;
	}
	// line endings:
	// UNIX: \n
	// Mac: \r
	// Windows: \r\n
	if (*l == '\r')
		l++;
	if (*l == '\n')
		l++;
	*text = l;
	return argc;
}

/*
============
COM_StringLengthNoColors

calculates the visible width of a color coded string.

*valid is filled with TRUE if the string is a valid colored string (that is, if
it does not end with an unfinished color code). If it gets filled with FALSE, a
fix would be adding a STRING_COLOR_TAG at the end of the string.

valid can be set to NULL if the caller doesn't care.

For size_s, specify the maximum number of characters from s to use, or 0 to use
all characters until the zero terminator.
============
*/
size_t
COM_StringLengthNoColors(const char *s, size_t size_s, qbool *valid)
{
	const char *end = size_s ? (s + size_s) : NULL;
	size_t len = 0;
	for(;;)
	{
		switch((s == end) ? 0 : *s)
		{
			case 0:
				if(valid)
					*valid = true;
				return len;
			case STRING_COLOR_TAG:
				++s;
				switch((s == end) ? 0 : *s)
				{
					case STRING_COLOR_RGB_TAG_CHAR:
						if (s+1 != end && isxdigit(s[1]) &&
							s+2 != end && isxdigit(s[2]) &&
							s+3 != end && isxdigit(s[3]) )
						{
							s+=3;
							break;
						}
						++len; // STRING_COLOR_TAG
						++len; // STRING_COLOR_RGB_TAG_CHAR
						break;
					case 0: // ends with unfinished color code!
						++len;
						if(valid)
							*valid = false;
						return len;
					case STRING_COLOR_TAG: // escaped ^
						++len;
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9': // color code
						break;
					default: // not a color code
						++len; // STRING_COLOR_TAG
						++len; // the character
						break;
				}
				break;
			default:
				++len;
				break;
		}
		++s;
	}
	// never get here
}

/*
============
COM_StringDecolorize

removes color codes from a string.

If escape_carets is true, the resulting string will be safe for printing. If
escape_carets is false, the function will just strip color codes (for logging
for example).
Returns the number of bytes written to the *out buffer excluding the \0 terminator.

If the output buffer size did not suffice for converting, the function returns 0.
Generally, if escape_carets is false, the output buffer needs
strlen(str)+1 bytes, and if escape_carets is true, it can need strlen(str)*1.5+2
bytes. In any case, the function makes sure that the resulting string is
zero terminated.

For size_in, specify the maximum number of characters from in to use, or 0 to use
all characters until the zero terminator.
============
*/
size_t COM_StringDecolorize(const char *in, size_t size_in, char *out, size_t size_out, qbool escape_carets)
{
#define APPEND(ch) do { if(--size_out) { *out++ = (ch); } else { *out++ = 0; return 0; } } while(0)

	const char *out_start = out;
	const char *end = size_in ? (in + size_in) : NULL;

	if(size_out < 1)
		return 0;
	for(;;)
	{
		switch((in == end) ? 0 : *in)
		{
			case 0:
				*out = '\0';
				return out - out_start;
			case STRING_COLOR_TAG:
				++in;
				switch((in == end) ? 0 : *in)
				{
					case STRING_COLOR_RGB_TAG_CHAR:
						if (in+1 != end && isxdigit(in[1]) &&
							in+2 != end && isxdigit(in[2]) &&
							in+3 != end && isxdigit(in[3]) )
						{
							in+=3;
							break;
						}
						APPEND(STRING_COLOR_TAG);
						if(escape_carets)
							APPEND(STRING_COLOR_TAG);
						APPEND(STRING_COLOR_RGB_TAG_CHAR);
						break;
					case 0: // ends with unfinished color code!
						APPEND(STRING_COLOR_TAG);
						// finish the code by appending another caret when escaping
						if(escape_carets)
							APPEND(STRING_COLOR_TAG);
						*out = '\0';
						return out - out_start;
					case STRING_COLOR_TAG: // escaped ^
						APPEND(STRING_COLOR_TAG);
						// append a ^ twice when escaping
						if(escape_carets)
							APPEND(STRING_COLOR_TAG);
						break;
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9': // color code
						break;
					default: // not a color code
						APPEND(STRING_COLOR_TAG);
						APPEND(*in);
						break;
				}
				break;
			default:
				APPEND(*in);
				break;
		}
		++in;
	}
	// never get here
#undef APPEND
}


/*
============
String Copying

The glibc implementation of memccpy is several times faster than old functions that
copy one byte at a time (even at -O3) and its advantage increases with string length.
============
*/
#ifdef WIN32
	// memccpy() is standard in POSIX.1-2001, POSIX.1-2008, SVr4, 4.3BSD, C23.
	// Microsoft supports it, but apparently complains if we use it.
	#undef memccpy
	#define memccpy _memccpy
#endif

/** Chain-copies a string with truncation and efficiency (compared to strlcat()).
 * The destination ends at an absolute pointer instead of a relative offset
 * and a pointer to the \0 terminator is returned on success.
 * Truncates, warns, and returns the end pointer on overflow or unterminated source.
 * Guarantees \0 termination.    end = dst + sizeof(src[])
 */
char *dp_stpecpy(char *dst, char *end, const char *src)
{
	char *p = (char *)memccpy(dst, src, '\0', end - dst);

	if (p)
		return p - 1;
	end[-1] = '\0';
	Con_Printf(CON_WARN "%s: src string unterminated or truncated to %lu bytes: \"%s\"\n", __func__, (unsigned long)(dst == end ? 0 : (end - dst) - 1), dst);
	return end;
}

/** Copies a measured byte sequence (unterminated string) to a null-terminated string.
 * Returns a pointer to the \0 terminator. Guarantees \0 termination.
 * Compared to ustr2stp(): truncates and warns on overflow.
 */
char *dp_ustr2stp(char *dst, size_t dsize, const char *src, size_t slen)
{
	if (slen >= dsize)
	{
		slen = dsize - 1;
		Con_Printf(CON_WARN "%s: src string truncated to %lu bytes: \"%.*s\"\n", __func__, (unsigned long)slen, (int)slen, src);
	}
	memcpy(dst, src, slen);
	dst[slen] = '\0';
	return &dst[slen];
}

/** Copies a string, like strlcpy() but with a better return: the number of bytes copied
 * excluding the \0 terminator. Truncates and warns on overflow or unterminated source,
 * whereas strlcpy() truncates silently and overreads (possibly segfaulting).
 * Guarantees \0 termination.
 * See also: dp_stpecpy() and dp_ustr2stp().
 */
size_t dp__strlcpy(char *dst, const char *src, size_t dsize, const char *func, unsigned line)
{
	char *p = (char *)memccpy(dst, src, '\0', dsize);

	if (p)
		return (p - 1) - dst;
	dst[dsize - 1] = '\0';
	Con_Printf(CON_WARN "%s:%u: src string unterminated or truncated to %lu bytes: \"%s\"\n", func, line, (unsigned long)dsize - 1, dst);
	return dsize - 1;
}

/** Catenates a string, like strlcat() but with a better return: the number of bytes copied
 * excluding the \0 terminator. Truncates and warns on overflow or unterminated source,
 * whereas strlcat() truncates silently and overreads (possibly segfaulting).
 * Guarantees \0 termination.
 * Inefficient like any strcat(), please use memcpy(), dp_stpecpy() or dp_strlcpy() instead.
 */
size_t dp__strlcat(char *dst, const char *src, size_t dsize, const char *func, unsigned line)
{
	size_t offset;
	char *p = (char *)memchr(dst, '\0', dsize);

	if (!p)
		p = dst;
	offset = p - dst;
	return dp__strlcpy(p, src, dsize - offset, func, line) + offset;
}



void FindFraction(double val, int *num, int *denom, int denomMax)
{
	int i;
	double bestdiff;
	// initialize
	bestdiff = fabs(val);
	*num = 0;
	*denom = 1;

	for(i = 1; i <= denomMax; ++i)
	{
		int inum = (int) floor(0.5 + val * i);
		double diff = fabs(val - inum / (double)i);
		if(diff < bestdiff)
		{
			bestdiff = diff;
			*num = inum;
			*denom = i;
		}
	}
}

// decodes an XPM from C syntax
char **XPM_DecodeString(const char *in)
{
	static char *tokens[257];
	static char lines[257][512];
	size_t line = 0;

	// skip until "{" token
	while(COM_ParseToken_QuakeC(&in, false) && strcmp(com_token, "{"));

	// now, read in succession: string, comma-or-}
	while(COM_ParseToken_QuakeC(&in, false))
	{
		tokens[line] = lines[line];
		dp_strlcpy(lines[line++], com_token, sizeof(lines[0]));
		if(!COM_ParseToken_QuakeC(&in, false))
			return NULL;
		if(!strcmp(com_token, "}"))
			break;
		if(strcmp(com_token, ","))
			return NULL;
		if(line >= sizeof(tokens) / sizeof(tokens[0]))
			return NULL;
	}

	return tokens;
}

static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void base64_3to4(const unsigned char *in, unsigned char *out, int bytes)
{
	unsigned char i0 = (bytes > 0) ? in[0] : 0;
	unsigned char i1 = (bytes > 1) ? in[1] : 0;
	unsigned char i2 = (bytes > 2) ? in[2] : 0;
	unsigned char o0 = base64[i0 >> 2];
	unsigned char o1 = base64[((i0 << 4) | (i1 >> 4)) & 077];
	unsigned char o2 = base64[((i1 << 2) | (i2 >> 6)) & 077];
	unsigned char o3 = base64[i2 & 077];
	out[0] = (bytes > 0) ? o0 : '?';
	out[1] = (bytes > 0) ? o1 : '?';
	out[2] = (bytes > 1) ? o2 : '=';
	out[3] = (bytes > 2) ? o3 : '=';
}

size_t base64_encode(unsigned char *buf, size_t buflen, size_t outbuflen)
{
	size_t blocks, i;
	// expand the out-buffer
	blocks = (buflen + 2) / 3;
	if(blocks*4 > outbuflen)
		return 0;
	for(i = blocks; i > 0; )
	{
		--i;
		base64_3to4(buf + 3*i, buf + 4*i, (int)(buflen - 3*i));
	}
	return blocks * 4;
}
