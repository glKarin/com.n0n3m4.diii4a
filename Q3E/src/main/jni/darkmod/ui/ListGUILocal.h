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

#ifndef __LISTGUILOCAL_H__
#define __LISTGUILOCAL_H__

/*
===============================================================================

	feed data to a listDef
	each item has an id and a display string

===============================================================================
*/

class idListGUILocal : protected idList<idStr>, public idListGUI {
public:
						idListGUILocal() { m_pGUI = NULL; m_water = 0; m_stateUpdates = true; }

	// idListGUI interface
	virtual void		Config( idUserInterface *pGUI, const char *name ) override { m_pGUI = pGUI; m_name = name; }
	virtual void		Add( int id, const idStr& s ) override;
						// use the element count as index for the ids
	virtual void		Push( const idStr& s ) override;
	virtual bool		Del( int id ) override;
	virtual void		Clear( void ) override;
	virtual int			Num( void ) override { return idList<idStr>::Num(); }
	virtual int			GetSelection( char *s, int size, int sel = 0 ) const override; // returns the id, not the list index (or -1)
	virtual void		SetSelection( int sel ) override;
	virtual int			GetNumSelections() override;
	virtual bool		IsConfigured( void ) const override;
	virtual void		SetStateChanges( bool enable ) override;
	virtual void		Shutdown( void ) override;

private:
	idUserInterface *	m_pGUI;
	idStr				m_name;
	int					m_water;
	idList<int>			m_ids;
	bool				m_stateUpdates;

	void				StateChanged();
};

#endif /* !__LISTGUILOCAL_H__ */
