/*
Field.h - edit field
Copyright (C) 2010 Uncle Mike
Copyright (C) 2017 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef MENU_FIELD_H
#define MENU_FIELD_H

#include "Editable.h"

#define UI_MAX_FIELD_LINE		256

class CMenuField : public CMenuEditable
{
public:
	typedef CMenuEditable BaseClass;

	CMenuField();
	void Init( void ) override;
	void VidInit( void ) override;
	bool KeyDown( int key ) override;
	void Draw( void ) override;
	void Char( int key ) override;
	void UpdateEditable() override;

	bool KeyValueData(const char *key, const char *data) override;
	void LinkCvar(const char *name) override
	{
		CMenuEditable::LinkCvar( name, CVAR_STRING );
	}

	VGUI_DefaultCursor CursorAction() override
	{
		return dc_ibeam;
	}

	void Paste();
	void Clear();

	void SetBuffer( const char *buffer )
	{
		Q_strncpy( szBuffer, buffer, sizeof( szBuffer ));
		iCursor = strlen( szBuffer );
		iScroll = g_FontMgr->CutText( font, szBuffer, m_scChSize, iRealWidth, true );
		SetCvarString( szBuffer );
	}

	const char *GetBuffer()
	{
		return szBuffer;
	}

	bool bAllowColorstrings;
	bool bHideInput;
	bool bNumbersOnly;
	CImage szBackground;
	int    iMaxLength;		// can't be more than UI_MAX_FIELD_LINE

protected:
	void _Event( int ev ) override;

private:
	char	szBuffer[UI_MAX_FIELD_LINE];
	int		iCursor;
	int		iScroll;

	int		iRealWidth;

	bool	m_bOverrideOverstrike;
};

#endif // MENU_FIELD_H
