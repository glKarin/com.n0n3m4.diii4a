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

#if !defined(AFX_DLGCLIENT_H__820F46A0_EEF3_408E_9EB4_BB12E4A657FB__INCLUDED_)
#define AFX_DLGCLIENT_H__820F46A0_EEF3_408E_9EB4_BB12E4A657FB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define FIRSTSHADEID 2020
#define LASTSHADEID  2300
#define CTRLHEIGTH  19
#define CTRLCOUNT   (iCustomControlID-FIRSTSHADEID+1)
#define YPOS  CTRLCOUNT*CTRLHEIGTH/2+YSPC+15
#define YSPC  (2*CTRLCOUNT)

#include "TextBox.h"
#include "ColoredButton.h"
#include "DropDown.h"
#include "CheckBox.h"

// classes for controls in shader dialog
class CTextureControl
{
public:
  CTextureControl::CTextureControl(){}
  CTextureControl::~CTextureControl(){}
  void AddControl(CTString strLabelText, INDEX *piItemID);
  CStatic txc_Label;
  CDropDown txc_Combo;
};
class CTexCoordControl
{
public:
  CTexCoordControl::CTexCoordControl(){}
  CTexCoordControl::~CTexCoordControl(){}
  void AddControl(CTString strLabelText, INDEX *piItemID);
  CStatic txcc_Label;
  CDropDown txcc_Combo;
};
class CColorControl
{
public:
  CColorControl::CColorControl(){}
  CColorControl::~CColorControl(){}
  void AddControl(CTString strLabelText, COLOR *pcolColor);
  CStatic cc_Label;
  CColoredButton cc_Button;
};
class CFloatControl
{
public:
  CFloatControl::CFloatControl(){}
  CFloatControl::~CFloatControl(){}
  void AddControl(CTString strLabelText, FLOAT *pFloat);
  CStatic fc_Label;
  CTextBox fc_TextBox;
};
class CFlagControl
{
public:
  CFlagControl::CFlagControl(){}
  CFlagControl::~CFlagControl(){}
  void AddControl(CTString strLabelText, INDEX iFlagIndex, ULONG ulFlags);
  CCheckBox fc_CheckBox;
};

    
class CDlgClient : public CDialog
{
// Construction
public:
	CDlgClient(CWnd* pParent = NULL);   // standard constructor

// Implementation
public:

	// Generated message map functions
	//{{AFX_MSG(CDlgClient)
	afx_msg void OnCbCompresion();
	afx_msg void OnCbSecperframe();
	afx_msg void OnSelendokCbParentbone();
	afx_msg void OnSelendokCbParentmodel();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSelendokCbShader();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBtConvert();
	afx_msg void OnBtReloadTexture();
	afx_msg void OnBtRecreateTexture();
	afx_msg void OnBtBrowseTexture();
	afx_msg void OnBtResetColision();
	afx_msg void OnBtResetOffset();
	afx_msg void OnBtCalcAllframesBbox();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGCLIENT_H__820F46A0_EEF3_408E_9EB4_BB12E4A657FB__INCLUDED_)
