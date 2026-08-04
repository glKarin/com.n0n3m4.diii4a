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

#include "precompiled.h"
#pragma hdrstop



// TearoffContainerWindow.cpp : implementation file
//

#include "TabsDlg.h"
#include "TearoffContainerWindow.h"


// CTearoffContainerWindow

IMPLEMENT_DYNAMIC(CTearoffContainerWindow, CWnd)
CTearoffContainerWindow::CTearoffContainerWindow()
{
	m_DragPreviewActive = false;
	m_ContainedDialog = NULL;
	m_DockManager = NULL;
}

CTearoffContainerWindow::~CTearoffContainerWindow()
{
}


BEGIN_MESSAGE_MAP(CTearoffContainerWindow, CWnd)
	ON_WM_NCLBUTTONDBLCLK()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

// CTearoffContainerWindow message handlers


void CTearoffContainerWindow::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
	if ( nHitTest == HTCAPTION )
	{
		m_DockManager->DockWindow ( m_DialogID , true );
	}

	CWnd::OnNcLButtonDblClk(nHitTest, point);
}


void CTearoffContainerWindow::SetDialog ( CWnd* dlg , int ID )
{
	m_DialogID = ID;
	m_ContainedDialog = dlg;
	
	CRect rect;
	CPoint point (-10 , -10);
	m_ContainedDialog->GetWindowRect ( rect );
	
	rect.OffsetRect(point);	//move the window slightly so you can tell it's been popped up

	//stupid hack to get the window resize itself properly
	rect.DeflateRect(0,0,0,1);
	MoveWindow(rect);	
	rect.InflateRect(0,0,0,1);
	MoveWindow(rect);	
}

void CTearoffContainerWindow::SetDockManager ( CTabsDlg* dlg )
{
	m_DockManager = dlg;
}
void CTearoffContainerWindow::OnClose()
{
	if ( m_DockManager ) 
	{
		//send it back to the docking window (for now at least)
		m_DockManager->DockWindow ( m_DialogID , true );
	}
}


BOOL CTearoffContainerWindow:: PreTranslateMessage( MSG* pMsg ) 
{
	if ( pMsg->message == WM_NCLBUTTONUP )
	{
/*		CRect rect;
		GetWindowRect ( rect );

		rect.DeflateRect( 0,0,0,rect.Height() - GetSystemMetrics(SM_CYSMSIZE));
		if ( m_DockManager->RectWithinDockManager ( rect ))
		{
			m_DockManager->DockDialog ( m_DialogID , true );
		}
*/
	}

	return CWnd::PreTranslateMessage(pMsg);
}
void CTearoffContainerWindow::OnSize(UINT nType, int cx, int cy)
{
	if ( m_ContainedDialog ) 
	{
		m_ContainedDialog->MoveWindow ( 0,0,cx,cy);		
	}
	
	CWnd::OnSize(nType, cx, cy);
}

void CTearoffContainerWindow::OnDestroy()
{
	CWnd::OnDestroy();

	// TODO: Add your message handler code here
}

void CTearoffContainerWindow::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	if ( m_ContainedDialog ) 
	{
		m_ContainedDialog->SetFocus();
	}
	// TODO: Add your message handler code here
}
