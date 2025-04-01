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
#pragma once

// CTearoffContainerWindow

class CTabsDlg;
class CTearoffContainerWindow : public CWnd
{
	DECLARE_DYNAMIC(CTearoffContainerWindow)

public:
	CTearoffContainerWindow();
	virtual ~CTearoffContainerWindow();

	CWnd* m_ContainedDialog;		//dialog that is being docked/undocked
	int m_DialogID;					//identifier for this dialog
	CTabsDlg* m_DockManager;		//the dialog that contains m_ContainedDialog  when docked

protected:
	DECLARE_MESSAGE_MAP()
	bool m_DragPreviewActive;	
public:
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	void SetDialog ( CWnd* dlg , int ID );
	void SetDockManager ( CTabsDlg* dlg );
	afx_msg void OnClose();
	BOOL PreTranslateMessage( MSG* pMsg );
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
};


