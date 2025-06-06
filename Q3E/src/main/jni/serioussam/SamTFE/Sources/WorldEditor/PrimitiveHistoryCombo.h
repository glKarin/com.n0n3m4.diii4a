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

#if !defined(AFX_PRIMITIVEHISTORYCOMBO_H__6365B946_B440_11D2_84C0_004095812ACC__INCLUDED_)
#define AFX_PRIMITIVEHISTORYCOMBO_H__6365B946_B440_11D2_84C0_004095812ACC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrimitiveHistoryCombo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPrimitiveHistoryCombo window

class CPrimitiveHistoryCombo : public CComboBox
{  
// Construction
public:
	CPrimitiveHistoryCombo();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrimitiveHistoryCombo)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPrimitiveHistoryCombo();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPrimitiveHistoryCombo)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRIMITIVEHISTORYCOMBO_H__6365B946_B440_11D2_84C0_004095812ACC__INCLUDED_)
