/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

// LocalPlayersList.h : header file
//
#ifndef LOCALPLAYERSLIST_H
#define LOCALPLAYERSLIST_H 1

/////////////////////////////////////////////////////////////////////////////
// CLocalPlayersList window

class CLocalPlayersList : public CCheckListBox
{
// Construction
public:
	CLocalPlayersList();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLocalPlayersList)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLocalPlayersList();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLocalPlayersList)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // LOCALPLAYERSLIST_H
