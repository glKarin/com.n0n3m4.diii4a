/*
Editable.h - generic item for editables
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

#ifndef MENU_EDITABLE_H
#define MENU_EDITABLE_H

#include "BaseItem.h"

class CMenuEditable : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuEditable();
	void Reload() override;

	// Every derived class can define how it will work with cvars
	virtual void UpdateEditable() = 0;

	// Engine allow only string and value cvars
	enum cvarType_e
	{
		CVAR_STRING = 0,
		CVAR_VALUE
	};

	// setup editable
	void LinkCvar( const char *name, cvarType_e type );

	// Getters
	inline const char *CvarName()   const { return m_szCvarName; }
	inline float       CvarValue()  const { return m_flValue; }
	inline const char *CvarString() const { return m_szString; }
	inline cvarType_e  CvarType()   const { return m_eType; }

	// Set cvar value/string and emit an event(does not written to engine)
	void SetCvarValue( float value );
	void SetCvarString( const char *string );

	// Set last got engine cvar values
	void SetOriginalValue( float val );
	void SetOriginalString( const char *psz );

	// Reset editable to last got engine values
	void ResetCvar();

	// Send cvar value/string to engine
	void WriteCvar();

	// Discard any changes and immediately send them to engine
	void DiscardChanges();

	// Update cvar values from engine
	void UpdateCvar( bool forceUpdate );

	CEventCallback onCvarWrite;  // called on final writing of cvar value
	CEventCallback onCvarChange; // called on internal values changes
	CEventCallback onCvarGet;    // called on any cvar update

	// events library
	DECLARE_EVENT_TO_ITEM_METHOD( CMenuEditable, WriteCvar )
	DECLARE_EVENT_TO_ITEM_METHOD( CMenuEditable, DiscardChanges )
	DECLARE_EVENT_TO_ITEM_METHOD( CMenuEditable, ResetCvar )

	bool bUpdateImmediately;

protected:
	const char *m_szCvarName;
	cvarType_e  m_eType;

	char		m_szString[CS_SIZE], m_szOriginalString[CS_SIZE];
	float		m_flValue, m_flOriginalValue;

private:
	// A possible shortcut for derived class, that support only one cvar type
	// derived class can move it to public and implement
	virtual void LinkCvar( const char *name );
};

#endif // MENU_EDITABLE_H
