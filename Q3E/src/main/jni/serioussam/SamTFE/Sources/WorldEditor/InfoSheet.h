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

// InfoSheet.h : header file
//
#ifndef INFOSHEET_H
#define INFOSHEET_H 1

/////////////////////////////////////////////////////////////////////////////
// CInfoSheet

class CInfoFrame;

enum ModeID {
  INFO_MODE_ALL = 0,
  INFO_MODE_GLOBAL,
  INFO_MODE_POSITION,
  INFO_MODE_PRIMITIVE,
  INFO_MODE_POLYGON,
  INFO_MODE_SECTOR,
  INFO_MODE_TERRAIN,
};

class CInfoSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CInfoSheet)

// Construction
public:
	CInfoSheet(CWnd* pWndParent);
	BOOL OnIdle(LONG lCount);
  // functions for dealing with pages
  void DeleteAllPages();
  void SetInfoModeGlobal(void);
  void SetInfoModePosition(void);
  void SetInfoModePrimitive(void);
  void SetInfoModePolygon(void);
  void SetInfoModeSector(void);
  void SetInfoModeTerrain(void);
  void SoftSetActivePage( INDEX iActivePage);
// Attributes
public:
	ModeID m_ModeID;
  CDlgPgGlobal m_PgGlobal;
  CDlgPgTerrain m_PgTerrain;
  CDlgPgPosition m_PgPosition;
  CDlgPgPrimitive m_PgPrimitive;
  CDlgPgRenderingStatistics m_PgRenderingStatistics;
  CDlgPgPolygon m_PgPolygon;
  CDlgPgShadow m_PgShadow;
  CDlgPgSector m_PgSector;
  CDlgPgTexture m_PgTexture;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInfoSheet)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CInfoSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CInfoSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // INFOSHEET_H
