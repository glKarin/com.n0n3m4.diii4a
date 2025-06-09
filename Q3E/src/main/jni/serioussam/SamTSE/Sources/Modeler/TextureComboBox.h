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

// TextureComboBox.h : header file
//
#ifndef TEXTURECOMBO_H
#define TEXTURECOMBO_H 1

/////////////////////////////////////////////////////////////////////////////
// CTextureComboBox window

class CTextureComboBox : public CComboBox
{
// Construction
public:
	BOOL OnIdle(LONG lCount);
  CTextureData *m_ptdSelectedTexture;
	CTextureComboBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextureComboBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTextureComboBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTextureComboBox)
	afx_msg void OnSelchange();
	afx_msg void OnDropdown();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // TEXTURECOMBO_H
