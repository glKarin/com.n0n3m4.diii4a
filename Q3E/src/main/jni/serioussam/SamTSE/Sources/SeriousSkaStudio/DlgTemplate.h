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

#if !defined(AFX_DLG_TEMPLATE_INCLUDED_)
#define AFX_DLG_TEMPLATE_INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif

#include "SplitterFrame.h"

class CDlgTemplate : public CDialogBar
{
public:
  CDlgTemplate();
  virtual ~CDlgTemplate();
  virtual CSize CalcDynamicLayout(int nLength, DWORD nMode);
  virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
  virtual INDEX GetDockingSide();
  void EnableDockingSides(ULONG ulDockingSides);
  void DockCtrlBar();

  void SetSplitterControlID(INDEX iSplitterID);
  void AdjustSplitter();

  CSize m_Size;

  ULONG dlg_ulEnabledDockingSides;
protected:
  CSplitterFrame dlg_spSlitter;
  INDEX dlg_iSplitterID;
  BOOL dlg_bDockingEnabled;
	//{{AFX_MSG(CDlgTemplate)
  	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
