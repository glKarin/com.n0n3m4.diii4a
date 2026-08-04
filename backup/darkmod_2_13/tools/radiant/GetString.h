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

#if !defined(__GETSTRING_H__)
#define __GETSTRING_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// CGetString dialog

// NOTE: already included in qe3.h but won't compile without including it again !?
#include "../../sys/win32/rc/Radiant_resource.h"

class CGetString : public CDialog
{
public:
	CGetString(LPCSTR pPrompt, CString *pFeedback, CWnd* pParent = NULL);   // standard constructor
	virtual ~CGetString();
// Overrides

// Dialog Data

	enum { IDD = IDD_DIALOG_GETSTRING };
	
	CString	m_strEditBox;
	CString *m_pFeedback;
	LPCSTR	m_pPrompt;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};

LPCSTR GetString(LPCSTR psPrompt);
bool GetYesNo(const char *psQuery);
void ErrorBox(const char *sString);
void InfoBox(const char *sString);
void WarningBox(const char *sString);

#endif /* !__GETSTRING_H__ */
