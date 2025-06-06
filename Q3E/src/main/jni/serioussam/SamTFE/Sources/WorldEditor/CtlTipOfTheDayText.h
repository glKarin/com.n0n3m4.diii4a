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

#if !defined(AFX_CTLTIPOFTHEDAYTEXT_H__F7006AF5_44F1_11D4_93A2_004095812ACC__INCLUDED_)
#define AFX_CTLTIPOFTHEDAYTEXT_H__F7006AF5_44F1_11D4_93A2_004095812ACC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CtlTipOfTheDayText.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCtlTipOfTheDayText window

class CCtlTipOfTheDayText : public CStatic
{
// Construction
public:
	CCtlTipOfTheDayText();

// Attributes
public:
  CString m_strTipText;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCtlTipOfTheDayText)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCtlTipOfTheDayText();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCtlTipOfTheDayText)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CTLTIPOFTHEDAYTEXT_H__F7006AF5_44F1_11D4_93A2_004095812ACC__INCLUDED_)
