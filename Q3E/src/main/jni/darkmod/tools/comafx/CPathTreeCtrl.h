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

#ifndef __CPATHTREECTR_H__
#define __CPATHTREECTR_H__

/*
===============================================================================

	Tree Control for path names.

===============================================================================
*/

class idPathTreeStack {
public:
						idPathTreeStack( void ) { size = 0; }

	void				PushRoot( HTREEITEM root );
	void				Push( HTREEITEM item, const char *name );
	void				Pop( void ) { size--; }
	HTREEITEM			TopItem( void ) const { return stackItem[size-1]; }
	const char *		TopName( void ) const { return stackName[size-1]; }
	int					TopNameLength( void ) const { return stackName[size-1].Length(); }
	int					Num( void ) const { return size; }

private:
	int					size;
	HTREEITEM			stackItem[128];
	idStr				stackName[128];
};

ID_INLINE void idPathTreeStack::PushRoot( HTREEITEM root ) {
	assert( size == 0 );
	stackItem[size] = root;
	stackName[size] = "";
	size++;
}

ID_INLINE void idPathTreeStack::Push( HTREEITEM item, const char *name ) {
	assert( size < 127 );
	stackItem[size] = item;
	stackName[size] = stackName[size-1] + name + "/";
	size++;
}

typedef bool (*treeItemCompare_t)( void *data, HTREEITEM item, const char *name );


class CPathTreeCtrl : public CTreeCtrl {
public:
						CPathTreeCtrl();
						~CPathTreeCtrl();

	HTREEITEM			FindItem( const idStr &pathName );
	HTREEITEM			InsertPathIntoTree( const idStr &pathName, const int id );
	HTREEITEM			AddPathToTree( const idStr &pathName, const int id, idPathTreeStack &stack );
	int					SearchTree( treeItemCompare_t compare, void *data, CPathTreeCtrl &result );

protected:
	virtual void		PreSubclassWindow();
	virtual INT_PTR   OnToolHitTest( CPoint point, TOOLINFO * pTI ) const;
	afx_msg BOOL		OnToolTipText( UINT id, NMHDR * pNMHDR, LRESULT * pResult );

	DECLARE_MESSAGE_MAP()
};

#endif /* !__CPATHTREECTR_H__ */
