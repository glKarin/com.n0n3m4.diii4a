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

// CDlgExportForSkinning.cpp : implementation file
//

#include "stdafx.h"
#include "DlgExportForSkinning.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgExportForSkinning dialog

CDlgExportForSkinning::CDlgExportForSkinning(CTFileName fnExportFile, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgExportForSkinning::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgExportForSkinning)
	m_bColoredSurfaces = FALSE;
	m_bSurfaceNumbers = FALSE;
	m_bWireFrame = FALSE;
	m_strExportedFileName = _T("");
	m_strSurfaceListFile = _T("");
	//}}AFX_DATA_INIT

  m_bWireFrame =                    GetFlagFromProfile(   "Export mapping wire frame", TRUE);
  m_bColoredSurfaces =              GetFlagFromProfile(   "Export mapping colored surfaces", TRUE);
  m_bSurfaceNumbers =               GetFlagFromProfile(   "Export surface numbers", TRUE);
	m_strExportedFileName = fnExportFile;
	m_strSurfaceListFile = fnExportFile.FileDir()+fnExportFile.FileName()+".txt";
	
  COLOR colPaper = GetColorFromProfile( "Paper color", C_WHITE);
  m_ctrlPaperColor.SetColor( colPaper);
	COLOR colWire = GetColorFromProfile( "Wire color", C_BLACK);
  m_ctrlWireColor.SetColor( colWire);

  m_strExportedFileName = fnExportFile;
}


void CDlgExportForSkinning::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
  
  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE )
  {
  }
	//{{AFX_DATA_MAP(CDlgExportForSkinning)
	DDX_Control(pDX, IDC_EXPORT_WIRE_COLOR, m_ctrlWireColor);
	DDX_Control(pDX, IDC_EXPORT_PAPER_COLOR, m_ctrlPaperColor);
	DDX_Control(pDX, IDC_EXPORT_PICTURE_SIZE, m_ctrlExportPictureSize);
	DDX_Check(pDX, IDC_COLORED_SURFACES, m_bColoredSurfaces);
	DDX_Check(pDX, IDC_COLORED_SURFACES_WITH_NUMBERS, m_bSurfaceNumbers);
	DDX_Check(pDX, IDC_WIRE_FRAME, m_bWireFrame);
	DDX_Text(pDX, IDC_EXPORT_FILE_NAME_T, m_strExportedFileName);
	DDX_Text(pDX, IDC_SURFACE_LIST_T, m_strSurfaceListFile);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE )
  {
    INDEX iSelected = m_ctrlExportPictureSize.GetCurSel();
    m_iTextureWidth = m_ctrlExportPictureSize.GetItemData( iSelected);
    SetIndexToProfile(  "Export mapping width", m_iTextureWidth);
    SetFlagToProfile(   "Export mapping wire frame", m_bWireFrame);
    SetFlagToProfile(   "Export mapping colored surfaces", m_bColoredSurfaces);
    SetFlagToProfile(   "Export surface numbers", m_bSurfaceNumbers);
  
    COLOR colPaper = m_ctrlPaperColor.GetColor();
    SetColorToProfile( "Paper color", colPaper);
    COLOR colWire = m_ctrlWireColor.GetColor();
	  SetColorToProfile( "Wire color", colWire);
  }
}


BEGIN_MESSAGE_MAP(CDlgExportForSkinning, CDialog)
	//{{AFX_MSG_MAP(CDlgExportForSkinning)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgExportForSkinning message handlers

BOOL CDlgExportForSkinning::OnInitDialog() 
{
	CDialog::OnInitDialog();
  
  m_iTextureWidth = GetIndexFromProfile(  "Export mapping width", 256);
  CModelerDoc* pDoc = theApp.GetDocument();
  FLOAT fWHRatio = FLOAT(pDoc->m_emEditModel.edm_md.md_Width)/pDoc->m_emEditModel.edm_md.md_Height;
  CTString strTemp;
  INDEX iWidth = 8;
  INDEX iToSelect = 0;
  while( iWidth <= 2048)
  {
    strTemp.PrintF( "%d x %d", iWidth, INDEX(iWidth/fWHRatio));
    INDEX iAddedAs = m_ctrlExportPictureSize.AddString( CString(strTemp));
    m_ctrlExportPictureSize.SetItemData( iAddedAs, iWidth);
    if( iWidth == m_iTextureWidth)
    {
      iToSelect = iAddedAs;
    }
    iWidth*=2;
  }
  // select last used export picture size
  m_ctrlExportPictureSize.SetCurSel( iToSelect);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
