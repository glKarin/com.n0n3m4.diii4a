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

#if !defined(AFX_DROPDOWN_H__8736EA9E_5675_44BB_AD38_E72FB7FF7C76__INCLUDED_)
#define AFX_DROPDOWN_H__8736EA9E_5675_44BB_AD38_E72FB7FF7C76__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DropDown.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDropDown window

class CDropDown : public CComboBox
{
// Construction
public:
	CDropDown();
	virtual ~CDropDown();

  void SetDataPtr(INDEX *pID);
  CTString m_strID; // ID of control (base texture)
  void RememberIDs();
  INDEX *m_pInt;  // pointing to index to change
  BOOL m_bSetID;  // is pointer pointion to ID or index in list


  
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDropDown)
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	//{{AFX_MSG(CDropDown)
	afx_msg void OnSelendok();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DROPDOWN_H__8736EA9E_5675_44BB_AD38_E72FB7FF7C76__INCLUDED_)
