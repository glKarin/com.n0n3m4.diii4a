/*
Editable.cpp - generic item for editables
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

#include "Editable.h"

CMenuEditable::CMenuEditable() : BaseClass(), bUpdateImmediately( false ),
	m_szCvarName(), m_eType(),
	m_flValue(), m_flOriginalValue()
{
	m_szString[0] = m_szOriginalString[0] = 0;
}

void CMenuEditable::LinkCvar(const char *)
{
	assert(("Derivative class does not implement LinkCvar(const char*) method. You need to specify types."));
}

void CMenuEditable::LinkCvar(const char *name, cvarType_e type)
{
	m_szCvarName = name;
	m_eType = type;

	UpdateCvar( true );
}

void CMenuEditable::Reload()
{
	// editable already initialized, so update
	if( m_szCvarName )
		UpdateCvar( true );
}

void CMenuEditable::SetCvarValue( float value )
{
	m_flValue = value;

	if( onCvarChange ) onCvarChange( this );
	if( bUpdateImmediately ) WriteCvar();
}

void CMenuEditable::SetCvarString( const char *string )
{
	if( string != m_szString )
		Q_strncpy( m_szString, string, sizeof( m_szString ));

	if( onCvarChange ) onCvarChange( this );
	if( bUpdateImmediately ) WriteCvar();
}

void CMenuEditable::SetOriginalString( const char *psz )
{
	Q_strncpy( m_szOriginalString, psz, sizeof( m_szOriginalString ));
	SetCvarString( m_szOriginalString );
}

void CMenuEditable::SetOriginalValue( float val )
{
	m_flOriginalValue = val;
	SetCvarValue( m_flOriginalValue );
}

void CMenuEditable::UpdateCvar( bool haveUpdate )
{
	if( onCvarGet )
	{
		onCvarGet( this );
		haveUpdate = false; // FIXME: add return values to events
	}
	else if( m_szCvarName )
	{
		switch( m_eType )
		{
		case CVAR_STRING:
		{
			const char *str = EngFuncs::GetCvarString( m_szCvarName );
			if( haveUpdate || strcmp( m_szOriginalString, str ) )
			{
				SetOriginalString( str );
				haveUpdate = true;
			}
			break;
		}
		case CVAR_VALUE:
		{
			float val = EngFuncs::GetCvarFloat( m_szCvarName );
			if( haveUpdate || m_flOriginalValue != val )
			{
				SetOriginalValue( val );
				haveUpdate = true;
			}
			break;
		}
		}
	}

	if( haveUpdate )
		UpdateEditable();
}

void CMenuEditable::ResetCvar()
{
	switch( m_eType )
	{
	case CVAR_STRING: SetCvarString( m_szOriginalString ); break;
	case CVAR_VALUE: SetCvarValue( m_flOriginalValue ); break;
	}
}

void CMenuEditable::DiscardChanges()
{
	ResetCvar();
	WriteCvar();
}

void CMenuEditable::WriteCvar()
{
	if( onCvarWrite ) onCvarWrite( this );
	else if( m_szCvarName )
	{
		switch( m_eType )
		{
		case CVAR_STRING: EngFuncs::CvarSetString( m_szCvarName, m_szString ); break;
		case CVAR_VALUE: EngFuncs::CvarSetValue( m_szCvarName, m_flValue ); break;
		}
	}
}

