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

// CtlTipOfTheDayText.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "CtlTipOfTheDayText.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCtlTipOfTheDayText

CCtlTipOfTheDayText::CCtlTipOfTheDayText()
{
}

CCtlTipOfTheDayText::~CCtlTipOfTheDayText()
{
}


BEGIN_MESSAGE_MAP(CCtlTipOfTheDayText, CStatic)
	//{{AFX_MSG_MAP(CCtlTipOfTheDayText)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCtlTipOfTheDayText message handlers
void CCtlTipOfTheDayText::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
  // get rectangle of the text area inside dialog
  RECT rectWin;
  GetWindowRect(&rectWin);
  ScreenToClient(&rectWin);

  // calculate positions of the upper and left rectangle and separating line
  int iy, ix0, ix1;
  RECT rectUp;
  RECT rectDn;
  ix0 = rectUp.left = rectDn.left = rectWin.left;
  ix1 = rectUp.right = rectDn.right = rectWin.right;
  iy = rectUp.bottom = rectDn.top = int((rectWin.bottom+rectWin.top)*0.25f);
  rectUp.top = rectWin.top;
  rectDn.bottom = rectWin.bottom;
  InflateRect(&rectDn, -5,-10);
  InflateRect(&rectUp, -5,-10);
  OffsetRect(&rectDn, 0,5);
  OffsetRect(&rectUp, 0,5);

  // draw white rectangle with sunken edge
  CBrush brWhite;
  brWhite.CreateStockObject(WHITE_BRUSH);
  dc.FillRect(&rectWin, &brWhite);
  dc.DrawEdge(&rectWin, BDR_SUNKENOUTER, BF_RIGHT|BF_BOTTOM|BF_TOP);

  // draw separating line
  CBrush brGray;
  brGray.CreateStockObject(GRAY_BRUSH);
  dc.SelectObject(brGray);
  dc.MoveTo(ix0-1, iy);
  dc.LineTo(ix1, iy);

  // create two fonts, big and small
  LOGFONT lf;

  ::ZeroMemory (&lf, sizeof (lf));
  lf.lfHeight = 145;
  lf.lfWeight = FW_BOLD;
  lf.lfItalic = FALSE;
  wcscpy(lf.lfFaceName, L"Times New Roman");
  CFont fontBig;
  fontBig.CreatePointFontIndirect (&lf);

  ::ZeroMemory (&lf, sizeof (lf));
  lf.lfHeight = 100;
  lf.lfWeight = FW_NORMAL;
  lf.lfItalic = FALSE;
  wcscpy(lf.lfFaceName, L"Arial");
  CFont fontSmall;
  fontSmall.CreatePointFontIndirect (&lf);

  // print heading with big font
  dc.SelectObject(&fontBig);
  dc.DrawText("Did you know...", &rectUp, DT_VCENTER|DT_SINGLELINE);

  // print text with small font
  dc.SelectObject(&fontSmall);
  dc.DrawText(m_strTipText, &rectDn, DT_WORDBREAK);
}
