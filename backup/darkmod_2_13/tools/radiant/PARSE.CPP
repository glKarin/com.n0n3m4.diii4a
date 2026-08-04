/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "qe3.h"

char	token[MAXTOKEN];
bool	unget;
const char	*script_p;
int		scriptline;

void	StartTokenParsing (const char *data)
{
	scriptline = 1;
	script_p = data;
	unget = false;
}

bool WINAPI GetToken (bool crossline)
{
	char    *token_p;

	if (unget)                         // is a token allready waiting?
	{
		unget = false;
		return true;
	}

//
// skip space
//
skipspace:
	while (*script_p <= 32)
	{
		if (!*script_p)
		{
			if (!crossline)
				common->Printf("Warning: Line %i is incomplete [01]\n",scriptline);
			return false;
		}
		if (*script_p++ == '\n')
		{
			if (!crossline)
				common->Printf("Warning: Line %i is incomplete [02]\n",scriptline);
			scriptline++;
		}
	}

	if (script_p[0] == '/' && script_p[1] == '/')	// comment field
	{
		if (!crossline)
			common->Printf("Warning: Line %i is incomplete [03]\n",scriptline);
		while (*script_p++ != '\n')
			if (!*script_p)
			{
				if (!crossline)
					common->Printf("Warning: Line %i is incomplete [04]\n",scriptline);
				return false;
			}
		goto skipspace;
	}

//
// copy token
//
	token_p = token;

	if (*script_p == '"')
	{
		script_p++;
    //if (*script_p == '"')   // handle double quotes i suspect they are put in by other editors cccasionally
    //  script_p++;
		while ( *script_p != '"' )
		{
			if (!*script_p)
				Error ("EOF inside quoted token");
			*token_p++ = *script_p++;
			if (token_p == &token[MAXTOKEN])
				Error ("Token too large on line %i",scriptline);
		}
		script_p++;
    //if (*script_p == '"')   // handle double quotes i suspect they are put in by other editors cccasionally
    //  script_p++;
	}
	else while ( *script_p > 32 )
	{
		*token_p++ = *script_p++;
		if (token_p == &token[MAXTOKEN])
			Error ("Token too large on line %i",scriptline);
	}

	*token_p = 0;
	
	return true;
}

void WINAPI UngetToken (void)
{
	unget = true;
}


/*
==============
TokenAvailable

Returns true if there is another token on the line
==============
*/
bool TokenAvailable (void)
{
	const char    *search_p;

	search_p = script_p;

	while ( *search_p <= 32)
	{
		if (*search_p == '\n')
			return false;
		if (*search_p == 0)
			return false;
		search_p++;
	}

	if (*search_p == ';')
		return false;

	return true;
}

