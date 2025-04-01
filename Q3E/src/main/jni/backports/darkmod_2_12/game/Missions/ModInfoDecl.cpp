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



#include "ModInfoDecl.h"

const char* const CModInfoDecl::TYPE_NAME = "tdm_missioninfo";

bool CModInfoDecl::Parse(idLexer& src)
{
	idToken		key;
	idToken		value;

	if (!src.ReadToken(&key))
	{
		src.Warning("Unclosed mod info declaration.");
		return false;
	}

	if (key.type != TT_PUNCTUATION || key.subtype != P_BRACEOPEN)
	{
		src.Warning("Cannot find opening brace in file %s:%d.", src.GetDisplayFileName(), src.GetLineNum());
		return false;
	}

	while (true)
	{
		// If there's an EOF, this is an error.
		if (!src.ReadToken(&key))
		{
			src.Warning("Unclosed mod info declaration.");
			return false;
		}

		// Quit upon encountering the closing brace.
		if (key.type == TT_PUNCTUATION && key.subtype == P_BRACECLOSE)
		{
			break;
		}
		else if (key.type == TT_STRING)
		{
			// Found a string, this must be a key
			if (!src.ReadToken(&value))
			{
				src.Warning("Unexpected EOF in key/value pair.");
				return false;
			}

			if (value.type == TT_STRING)
			{
				// Save the key:value pair.
				data.Set(key.c_str(), value.c_str());
			}
			else
			{
				src.Warning("Invalid value: %s", value.c_str());
				continue;
			}
		}
		else 
		{
			src.Warning("Unrecognized token: %s", key.c_str());
			continue;
		}
	}

	return true;
}

void CModInfoDecl::Update(const idStr& name)
{
	if ( name.IsEmpty() ) // grayman #4338
	{
		return;
	}

	_bodyText = TYPE_NAME;
	_bodyText += " " + name;
	_bodyText += "\n{\n";

	// Dump the keyvalues
	for (int i = 0; i < data.GetNumKeyVals(); ++i)
	{
		const idKeyValue* kv = data.GetKeyVal(i);

		_bodyText += "\t\"" + kv->GetKey() + "\"";
		_bodyText += "\t\"" + kv->GetValue() + "\"\n";
	}

	_bodyText += "}\n\n";
}

void CModInfoDecl::SaveToFile(idFile* file)
{
	file->Write(_bodyText.c_str(), _bodyText.Length());
}
