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

// DlgPgPrimitive.cpp : implementation file
//

#include "stdafx.h"
#include "DlgPgPrimitive.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CTFileName _fnPrimitiveSettingsSavedAs;
BOOL _bForceRecreatePrimitive = FALSE;
/////////////////////////////////////////////////////////////////////////////
// CDlgPgPrimitive property page

IMPLEMENT_DYNCREATE(CDlgPgPrimitive, CPropertyPage)

CDlgPgPrimitive::CDlgPgPrimitive() : CPropertyPage(CDlgPgPrimitive::IDD)
{
	//{{AFX_DATA_INIT(CDlgPgPrimitive)
	m_fHeight = 0.0f;
	m_fLenght = 0.0f;
	m_fWidth = 0.0f;
	m_fEdit1 = 0.0f;
	m_fEdit2 = 0.0f;
	m_fEdit3 = 0.0f;
	m_fEdit4 = 0.0f;
	m_fEdit5 = 0.0f;
	m_bIfRoom = FALSE;
	m_bIfSpiral = FALSE;
	m_bIfOuter = FALSE;
	m_strDisplacePicture = _T("");
	m_bAutoCreateMipBrushes = FALSE;
	//}}AFX_DATA_INIT
  // set some initial colors;

  m_colLastSectorColor = 0x12345678;
  m_colLastPolygonColor = 0x12345678;
}

CDlgPgPrimitive::~CDlgPgPrimitive()
{
}

void CDlgPgPrimitive::DoDataExchange(CDataExchange* pDX)
{
  if( theApp.m_bDisableDataExchange) return;

  CPropertyPage::DoDataExchange(pDX);

  // get active document
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  // if document doesn't yet exist, return
  if( pDoc == NULL)
  {
    return;
  }

  // if dialog is recieving data
  if( (pDX->m_bSaveAndValidate == FALSE) && (IsWindow(m_comboTopShape.m_hWnd)) )
  {
	  m_fWidth = theApp.m_vfpCurrent.vfp_fXMax-theApp.m_vfpCurrent.vfp_fXMin;
	  m_fLenght = theApp.m_vfpCurrent.vfp_fZMax-theApp.m_vfpCurrent.vfp_fZMin;
	  m_fHeight = theApp.m_vfpCurrent.vfp_fYMax-theApp.m_vfpCurrent.vfp_fYMin;
    m_SectorColor.SetColor( theApp.m_vfpCurrent.vfp_colSectorsColor);
    m_PolygonColor.SetColor( theApp.m_vfpCurrent.vfp_colPolygonsColor);
    m_bAutoCreateMipBrushes = theApp.m_vfpCurrent.vfp_bAutoCreateMipBrushes;

    GetDlgItem(IDC_DISPLACE_T)->ShowWindow( SW_HIDE);
    GetDlgItem(IDC_DISPLACE_BROWSE)->ShowWindow( SW_HIDE);
    GetDlgItem(IDC_DISPLACE_NONE)->ShowWindow( SW_HIDE);
    GetDlgItem(IDC_DISPLACE_FILE)->ShowWindow( SW_HIDE);
    GetDlgItem(IDC_AUTO_CREATE_MIP_BRUSHES)->ShowWindow( SW_HIDE);

    switch( theApp.m_vfpCurrent.vfp_ptPrimitiveType)
    {
    case PT_CONUS:
      {
        GetDlgItem(IDC_LENGHT)->EnableWindow( TRUE);
        GetDlgItem(IDC_LENGHT_T)->EnableWindow( TRUE);
        GetDlgItem(IDC_HEIGHT)->EnableWindow( TRUE);
        GetDlgItem(IDC_HEIGHT_T)->EnableWindow( TRUE);
        GetDlgItem(IDC_EDIT1)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT1_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT1_T)->SetWindowText( L"Shear x:");
        GetDlgItem(IDC_EDIT2)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT2_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT2_T)->SetWindowText( L"Shear y:");
        GetDlgItem(IDC_EDIT3)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT3_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT3_T)->SetWindowText( L"Base vtx:");
        GetDlgItem(IDC_EDIT4)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT4_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT4_T)->SetWindowText( L"Stretch x:");
        GetDlgItem(IDC_EDIT5)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT5_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT5_T)->SetWindowText( L"Stretch y:");
        GetDlgItem(IDC_IF_SPIRAL)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_IF_OUTER)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_TOP_SHAPE)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_BOTTOM_SHAPE)->ShowWindow( SW_HIDE);
        m_fEdit1 = theApp.m_vfpCurrent.vfp_fShearX;
        m_fEdit2 = theApp.m_vfpCurrent.vfp_fShearZ;
        m_fEdit3 = (float) theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Count();
        m_fEdit4 = theApp.m_vfpCurrent.vfp_fStretchX;
        m_fEdit5 = theApp.m_vfpCurrent.vfp_fStretchY;
	      m_bIfRoom = theApp.m_vfpCurrent.vfp_bClosed;
	      m_bIfOuter = theApp.m_vfpCurrent.vfp_bOuter;
        break;
      }
    case PT_TORUS:
      {
        GetDlgItem(IDC_LENGHT)->EnableWindow( TRUE);
        GetDlgItem(IDC_LENGHT_T)->EnableWindow( TRUE);
        GetDlgItem(IDC_HEIGHT)->EnableWindow( TRUE);
        GetDlgItem(IDC_HEIGHT_T)->EnableWindow( TRUE);
        GetDlgItem(IDC_EDIT1)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT1_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT1_T)->SetWindowText( L"Slices in 360:");
        GetDlgItem(IDC_EDIT2)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT2_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT2_T)->SetWindowText( L"No of slices:");
        GetDlgItem(IDC_EDIT3)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT3_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT3_T)->SetWindowText( L"Base vtx:");
        GetDlgItem(IDC_EDIT4)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT4_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT4_T)->SetWindowText( L"Radius:");
        GetDlgItem(IDC_EDIT5)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_EDIT5_T)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_TOP_SHAPE)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_BOTTOM_SHAPE)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_IF_SPIRAL)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_IF_OUTER)->ShowWindow( SW_SHOW);
        m_fEdit1 = (float)theApp.m_vfpCurrent.vfp_iSlicesIn360;
        m_fEdit2 = (float)theApp.m_vfpCurrent.vfp_iNoOfSlices;
        m_fEdit3 = (float)theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Count();
        m_fEdit4 = theApp.m_vfpCurrent.vfp_fRadius;
	      m_bIfRoom = theApp.m_vfpCurrent.vfp_bClosed;
	      m_bIfOuter = theApp.m_vfpCurrent.vfp_bOuter;
        break;
      }
    case PT_STAIRCASES:
      {
        GetDlgItem(IDC_HEIGHT)->EnableWindow( TRUE);
        GetDlgItem(IDC_HEIGHT_T)->EnableWindow( TRUE);
        GetDlgItem(IDC_EDIT2)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT2_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT2_T)->SetWindowText( L"No of stairs:");
        GetDlgItem(IDC_EDIT3)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_EDIT3_T)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_EDIT4)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_EDIT4_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT4_T)->SetWindowText( L"Top:");
        GetDlgItem(IDC_EDIT5)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_EDIT5_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT5_T)->SetWindowText( L"Bottom:");
        GetDlgItem(IDC_TOP_SHAPE)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_BOTTOM_SHAPE)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_IF_SPIRAL)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_IF_SPIRAL)->SetWindowText( L"Spiral:");

	      if( theApp.m_vfpCurrent.vfp_bLinearStaircases)
        {
          GetDlgItem(IDC_LENGHT)->EnableWindow( TRUE);
          GetDlgItem(IDC_LENGHT_T)->EnableWindow( TRUE);
          GetDlgItem(IDC_EDIT1)->ShowWindow( SW_HIDE);
          GetDlgItem(IDC_EDIT1_T)->ShowWindow( SW_HIDE);
          GetDlgItem(IDC_IF_OUTER)->ShowWindow( SW_HIDE);
        }
        else
        {
          GetDlgItem(IDC_LENGHT)->EnableWindow( FALSE);
          GetDlgItem(IDC_LENGHT_T)->EnableWindow( FALSE);
          GetDlgItem(IDC_EDIT1)->ShowWindow( SW_SHOW);
          GetDlgItem(IDC_EDIT1_T)->ShowWindow( SW_SHOW);
          GetDlgItem(IDC_EDIT1_T)->SetWindowText( L"Slices in 360:");
          GetDlgItem(IDC_EDIT3)->ShowWindow( SW_SHOW);
          GetDlgItem(IDC_EDIT3_T)->ShowWindow( SW_SHOW);
          GetDlgItem(IDC_EDIT3_T)->SetWindowText( L"Radius:");
          GetDlgItem(IDC_IF_OUTER)->ShowWindow( SW_SHOW);
        }
        m_fEdit1 = (float)theApp.m_vfpCurrent.vfp_iSlicesIn360;
        m_fEdit2 = (float)theApp.m_vfpCurrent.vfp_iNoOfSlices;
        m_fEdit3 = theApp.m_vfpCurrent.vfp_fRadius;
	      m_bIfRoom = theApp.m_vfpCurrent.vfp_bClosed;
	      m_bIfSpiral = !theApp.m_vfpCurrent.vfp_bLinearStaircases;
	      m_bIfOuter = theApp.m_vfpCurrent.vfp_bOuter;
        m_comboTopShape.SetCurSel( (int)theApp.m_vfpCurrent.vfp_iTopShape);
        m_comboBottomShape.SetCurSel( (int)theApp.m_vfpCurrent.vfp_iBottomShape);
        break;
      }
    case PT_SPHERE:
      {
        GetDlgItem(IDC_LENGHT)->EnableWindow( TRUE);
        GetDlgItem(IDC_LENGHT_T)->EnableWindow( TRUE);
        GetDlgItem(IDC_HEIGHT)->EnableWindow( TRUE);
        GetDlgItem(IDC_HEIGHT_T)->EnableWindow( TRUE);
        GetDlgItem(IDC_EDIT1)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT1_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT1_T)->SetWindowText( L"Meridians:");
        GetDlgItem(IDC_EDIT2)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT2_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT2_T)->SetWindowText( L"Parallels:");
        GetDlgItem(IDC_EDIT3)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_EDIT3_T)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_TOP_SHAPE)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_BOTTOM_SHAPE)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_IF_SPIRAL)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_IF_SPIRAL)->SetWindowText( L"Equal parallels:");
        GetDlgItem(IDC_IF_OUTER)->ShowWindow( SW_HIDE);
        m_fEdit1 = (float)theApp.m_vfpCurrent.vfp_iMeridians;
        m_fEdit2 = (float)theApp.m_vfpCurrent.vfp_iParalels;
        m_fEdit4 = theApp.m_fTerrainSwitchStart;
        m_fEdit5 = theApp.m_fTerrainSwitchStep;
        m_bIfRoom = theApp.m_vfpCurrent.vfp_bClosed;
        m_bIfSpiral = theApp.m_vfpCurrent.vfp_bLinearStaircases;
        break;
      }
    case PT_TERRAIN:
      {
        GetDlgItem(IDC_LENGHT)->EnableWindow( TRUE);
        GetDlgItem(IDC_LENGHT_T)->EnableWindow( TRUE);
        GetDlgItem(IDC_HEIGHT)->EnableWindow( TRUE);
        GetDlgItem(IDC_HEIGHT_T)->EnableWindow( TRUE);
        GetDlgItem(IDC_EDIT1)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT1_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT1_T)->SetWindowText( L"Slices per W:");
        GetDlgItem(IDC_EDIT2)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT2_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT2_T)->SetWindowText( L"Slices per H");
        GetDlgItem(IDC_EDIT4)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT4_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT4_T)->SetWindowText( L"Mip start");
        GetDlgItem(IDC_EDIT5)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT5_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT5_T)->SetWindowText( L"Mip step");
        GetDlgItem(IDC_TOP_SHAPE)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_BOTTOM_SHAPE)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_IF_SPIRAL)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_IF_OUTER)->ShowWindow( SW_HIDE);
        GetDlgItem(IDC_AUTO_CREATE_MIP_BRUSHES)->ShowWindow( SW_SHOW);
        m_fEdit1 = (float)theApp.m_vfpCurrent.vfp_iSlicesPerWidth;
        m_fEdit2 = (float)theApp.m_vfpCurrent.vfp_iSlicesPerHeight;
        m_fEdit3 = theApp.m_vfpCurrent.vfp_fAmplitude;
        m_fEdit4 = theApp.m_vfpCurrent.vfp_fMipStart;
        m_fEdit5 = theApp.m_vfpCurrent.vfp_fMipStep;
        m_bIfRoom = theApp.m_vfpCurrent.vfp_bClosed;
        GetDlgItem(IDC_DISPLACE_T)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_DISPLACE_BROWSE)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_DISPLACE_NONE)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_DISPLACE_FILE)->ShowWindow( SW_SHOW);

        GetDlgItem(IDC_EDIT3)->ShowWindow( SW_SHOW);
        GetDlgItem(IDC_EDIT3_T)->SetWindowText( L"Amplitude:");
        GetDlgItem(IDC_EDIT3_T)->ShowWindow( SW_SHOW);
        m_strDisplacePicture =
          theApp.m_vfpCurrent.vfp_fnDisplacement.FileName()+
          theApp.m_vfpCurrent.vfp_fnDisplacement.FileExt();
        if( m_strDisplacePicture == "")
        {
          GetDlgItem(IDC_EDIT3)->EnableWindow( FALSE);
          GetDlgItem(IDC_EDIT3_T)->EnableWindow( FALSE);
          GetDlgItem(IDC_DISPLACE_NONE)->EnableWindow( FALSE);
          m_strDisplacePicture = "<none>";
        }
        else
        {
          GetDlgItem(IDC_EDIT3)->EnableWindow( TRUE);
          GetDlgItem(IDC_EDIT3_T)->EnableWindow( TRUE);
          GetDlgItem(IDC_DISPLACE_NONE)->EnableWindow( TRUE);
        }
        break;
      }
    default:
      {
        ASSERTALWAYS( "Invalid primitive type found!");
      }
    }
  }

  //{{AFX_DATA_MAP(CDlgPgPrimitive)
	DDX_Control(pDX, IDC_PRIMITIVE_HISTORY, m_comboPrimitiveHistory);
	DDX_Control(pDX, IDC_TOP_SHAPE, m_comboTopShape);
	DDX_Control(pDX, IDC_BOTTOM_SHAPE, m_comboBottomShape);
	DDX_Control(pDX, ID_SECTOR_COLOR, m_SectorColor);
	DDX_Control(pDX, ID_POLYGON_COLOR, m_PolygonColor);
	DDX_Text(pDX, IDC_HEIGHT, m_fHeight);
	DDV_MinMaxFloat(pDX, m_fHeight, -256000.0f, 256000.0f);
	DDX_Text(pDX, IDC_LENGHT, m_fLenght);
	DDV_MinMaxFloat(pDX, m_fLenght, -256000.0f, 256000.0f);
	DDX_Text(pDX, IDC_WIDTH, m_fWidth);
	DDV_MinMaxFloat(pDX, m_fWidth, -256000.0f, 256000.0f);
	DDX_Text(pDX, IDC_EDIT1, m_fEdit1);
	DDX_Text(pDX, IDC_EDIT2, m_fEdit2);
	DDX_Text(pDX, IDC_EDIT3, m_fEdit3);
	DDX_Text(pDX, IDC_EDIT4, m_fEdit4);
	DDX_Text(pDX, IDC_EDIT5, m_fEdit5);
	DDX_Check(pDX, IDC_IF_ROOM, m_bIfRoom);
	DDX_Check(pDX, IDC_IF_SPIRAL, m_bIfSpiral);
	DDX_Check(pDX, IDC_IF_OUTER, m_bIfOuter);
	DDX_Text(pDX, IDC_DISPLACE_FILE, m_strDisplacePicture);
	DDX_Check(pDX, IDC_AUTO_CREATE_MIP_BRUSHES, m_bAutoCreateMipBrushes);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    CValuesForPrimitive vfpBeforeDDX = theApp.m_vfpCurrent;
    theApp.m_vfpCurrent.vfp_bDummy = FALSE;
	  theApp.m_vfpCurrent.vfp_bAutoCreateMipBrushes = m_bAutoCreateMipBrushes;
    theApp.m_vfpCurrent.vfp_colSectorsColor = m_SectorColor.GetColor();
    theApp.m_vfpCurrent.vfp_colPolygonsColor = m_PolygonColor.GetColor();
#define PRIMITIVE_CHANGED_DELTA 0.01f
    if( Abs(theApp.m_vfpCurrent.vfp_fXMax-theApp.m_vfpCurrent.vfp_fXMin-m_fWidth) > PRIMITIVE_CHANGED_DELTA)
    {
      theApp.m_vfpCurrent.vfp_fXMin = -m_fWidth/2;
	    theApp.m_vfpCurrent.vfp_fXMax =  m_fWidth/2;
    }
    if( Abs(theApp.m_vfpCurrent.vfp_fYMax-theApp.m_vfpCurrent.vfp_fYMin-m_fHeight) > PRIMITIVE_CHANGED_DELTA)
    {
	    theApp.m_vfpCurrent.vfp_fYMin =  0;
	    theApp.m_vfpCurrent.vfp_fYMax =  m_fHeight;
    }
    if( Abs(theApp.m_vfpCurrent.vfp_fZMax-theApp.m_vfpCurrent.vfp_fZMin-m_fLenght) > PRIMITIVE_CHANGED_DELTA)
    {
	    theApp.m_vfpCurrent.vfp_fZMin = -m_fLenght/2;
	    theApp.m_vfpCurrent.vfp_fZMax =  m_fLenght/2;
    }

    if( ((theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_CONUS) ||
         (theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_TORUS)) &&
         (theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Count() != m_fEdit3) &&
         ((INDEX)(m_fEdit3) > 0) )
    {
      theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Clear();
      theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.New( Abs((INDEX) m_fEdit3));
    }
	  theApp.m_vfpCurrent.vfp_fShearX = m_fEdit1;
	  theApp.m_vfpCurrent.vfp_iSlicesIn360 = (INDEX) m_fEdit1;
	  theApp.m_vfpCurrent.vfp_iMeridians = (INDEX) m_fEdit1;
	  theApp.m_vfpCurrent.vfp_iSlicesPerWidth = (INDEX) m_fEdit1;

    theApp.m_vfpCurrent.vfp_fShearZ = m_fEdit2;
    theApp.m_vfpCurrent.vfp_iNoOfSlices = (INDEX) m_fEdit2;
    theApp.m_vfpCurrent.vfp_iParalels = (INDEX) m_fEdit2;
    theApp.m_vfpCurrent.vfp_iSlicesPerHeight = (INDEX) m_fEdit2;

    if( theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_TORUS)
    {
      theApp.m_vfpCurrent.vfp_fRadius = m_fEdit4;
    }
    if( theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_STAIRCASES)
    {
      theApp.m_vfpCurrent.vfp_fRadius = m_fEdit3;
  	  theApp.m_vfpCurrent.vfp_bLinearStaircases = !m_bIfSpiral;
    }

    if( theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_TERRAIN)
    {
      theApp.m_vfpCurrent.vfp_fAmplitude = m_fEdit3;
      theApp.m_vfpCurrent.vfp_fMipStart = m_fEdit4;
      theApp.m_vfpCurrent.vfp_fMipStep = m_fEdit5;
    }
    else
    {
	    theApp.m_vfpCurrent.vfp_fStretchX = m_fEdit4;
	    theApp.m_vfpCurrent.vfp_fStretchY = m_fEdit5;
    }
    theApp.m_vfpCurrent.vfp_bClosed = m_bIfRoom;
	  theApp.m_vfpCurrent.vfp_bOuter = m_bIfOuter;
    theApp.m_vfpCurrent.vfp_iTopShape = m_comboTopShape.GetCurSel();
    theApp.m_vfpCurrent.vfp_iBottomShape = m_comboBottomShape.GetCurSel();
    // if anything changed
    if( !(theApp.m_vfpCurrent == vfpBeforeDDX) || _bForceRecreatePrimitive)
    {
      _bForceRecreatePrimitive = FALSE;
      // apply CSG change
      ApplySCGChange();
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgPgPrimitive, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgPgPrimitive)
	ON_CBN_SELCHANGE(IDC_BOTTOM_SHAPE, OnSelchangeBottomShape)
	ON_CBN_SELCHANGE(IDC_TOP_SHAPE, OnSelchangeTopShape)
	ON_BN_CLICKED(IDC_IF_ROOM, OnIfRoom)
	ON_BN_CLICKED(IDC_IF_SPIRAL, OnIfSpiral)
	ON_BN_CLICKED(IDC_IF_OUTER, OnIfOuter)
	ON_CBN_SELCHANGE(IDC_PRIMITIVE_HISTORY, OnSelchangePrimitiveHistory)
	ON_CBN_DROPDOWN(IDC_PRIMITIVE_HISTORY, OnDropdownPrimitiveHistory)
	ON_BN_CLICKED(IDC_DISPLACE_BROWSE, OnDisplaceBrowse)
	ON_BN_CLICKED(IDC_DISPLACE_NONE, OnDisplaceNone)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_LOAD_PRIMITIVE_SETTINGS, OnLoadPrimitiveSettings)
	ON_COMMAND(ID_SAVE_PRIMITIVE_SETTINGS, OnSavePrimitiveSettings)
	ON_COMMAND(ID_SAVE_AS_PRIMITIVE_SETTINGS, OnSaveAsPrimitiveSettings)
	ON_COMMAND(ID_RESET_PRIMITIVE, OnResetPrimitive)
	ON_CBN_DROPDOWN(IDC_TOP_SHAPE, OnDropdownTopShape)
	ON_CBN_DROPDOWN(IDC_BOTTOM_SHAPE, OnDropdownBottomShape)
	ON_BN_CLICKED(IDC_AUTO_CREATE_MIP_BRUSHES, OnAutoCreateMipBrushes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPgPrimitive message handlers

BOOL CDlgPgPrimitive::OnIdle(LONG lCount)
{
  COLOR colSectorColor = m_SectorColor.GetColor();
  if( colSectorColor != m_colLastSectorColor)
  {
    // view the color change
    UpdateData(TRUE);
    GetDlgItem( ID_SECTOR_COLOR)->Invalidate();
  }
  // set new sector color
  m_colLastSectorColor = colSectorColor;

  COLOR colPolygonColor = m_PolygonColor.GetColor();
  if( colPolygonColor != m_colLastPolygonColor)
  {
    // sector color is same as polygon color
    m_SectorColor.SetColor( m_PolygonColor.GetColor());
    // view the color change
    UpdateData(TRUE);
    GetDlgItem( ID_POLYGON_COLOR)->Invalidate();
    GetDlgItem( ID_SECTOR_COLOR)->Invalidate();
  }
  // set new Polygon color
  m_colLastPolygonColor = colPolygonColor;

  return TRUE;
}

void CDlgPgPrimitive::ApplySCGChange()
{
  // get active document
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  ASSERT( pDoc != NULL);

	// snap all values to grid
  pDoc->SnapPrimitiveValuesToGrid();
  // recreate primitive with new values
  pDoc->CreatePrimitive();
  // update all views
  pDoc->UpdateAllViews( NULL);
}

BOOL CDlgPgPrimitive::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN)
  {
    // move data from page to primitive
    UpdateData( TRUE);
    // place snapped values back to dialog
    UpdateData( FALSE);
    // the message is handled
    return TRUE;
  }
	return CPropertyPage::PreTranslateMessage(pMsg);
}

BOOL CDlgPgPrimitive::OnInitDialog()
{
  _fnPrimitiveSettingsSavedAs = CTString("");
  CPropertyPage::OnInitDialog();

  // set last colors as curent ones so OnIdle would not copy polygon color over sector
  m_colLastPolygonColor = m_PolygonColor.GetColor();
  m_colLastSectorColor = m_SectorColor.GetColor();

  if( !IsWindow( m_comboPrimitiveHistory.m_hWnd)) return TRUE;

  INDEX iCt = 1;
  // add descriptions of primitives from history list into combo
  FOREACHINLIST( CPrimitiveInHistoryBuffer, pihb_lnNode, theApp.m_lhPrimitiveHistory, itPrim)
  {
    CTString strNo;
    strNo.PrintF("%d) ", iCt);
    CTString strDescription;
    switch( itPrim->pihb_vfpPrimitive.vfp_ptPrimitiveType)
    {
    case PT_CONUS:
      {
        strDescription.PrintF("%d vtx, Size(%g,%g,%g) Shear(%g,%g) Stretch(%g,%g)",
          itPrim->pihb_vfpPrimitive.vfp_avVerticesOnBaseOfPrimitive.Count(),
          itPrim->pihb_vfpPrimitive.vfp_fXMax-itPrim->pihb_vfpPrimitive.vfp_fXMin,
          itPrim->pihb_vfpPrimitive.vfp_fZMax-itPrim->pihb_vfpPrimitive.vfp_fZMin,
          itPrim->pihb_vfpPrimitive.vfp_fYMax-itPrim->pihb_vfpPrimitive.vfp_fYMin,
          itPrim->pihb_vfpPrimitive.vfp_fShearX, itPrim->pihb_vfpPrimitive.vfp_fShearZ,
          itPrim->pihb_vfpPrimitive.vfp_fStretchX, itPrim->pihb_vfpPrimitive.vfp_fStretchY);
        break;
      }
    case PT_TORUS:
      {
        strDescription.PrintF("%d vtx, Size(%g,%g) Radius(%g) Slices(%d,%d)",
          itPrim->pihb_vfpPrimitive.vfp_avVerticesOnBaseOfPrimitive.Count(),
          itPrim->pihb_vfpPrimitive.vfp_fXMax-itPrim->pihb_vfpPrimitive.vfp_fXMin,
          itPrim->pihb_vfpPrimitive.vfp_fZMax-itPrim->pihb_vfpPrimitive.vfp_fZMin,
          itPrim->pihb_vfpPrimitive.vfp_fRadius,
          itPrim->pihb_vfpPrimitive.vfp_iSlicesIn360,
          itPrim->pihb_vfpPrimitive.vfp_iNoOfSlices);
        break;
      }
    case PT_STAIRCASES:
      {
        if( itPrim->pihb_vfpPrimitive.vfp_bLinearStaircases)
        {
          strDescription.PrintF("%d stairs, Size(%g,%g,%g)",
            itPrim->pihb_vfpPrimitive.vfp_iNoOfSlices,
            itPrim->pihb_vfpPrimitive.vfp_fXMax-itPrim->pihb_vfpPrimitive.vfp_fXMin,
            itPrim->pihb_vfpPrimitive.vfp_fZMax-itPrim->pihb_vfpPrimitive.vfp_fZMin,
            itPrim->pihb_vfpPrimitive.vfp_fYMax-itPrim->pihb_vfpPrimitive.vfp_fYMin);
        }
        else
        {
          strDescription.PrintF("Radius(%g), %d Slices %d Stairs Size(%g,%g)",
            itPrim->pihb_vfpPrimitive.vfp_fRadius,
            itPrim->pihb_vfpPrimitive.vfp_iSlicesIn360,
            itPrim->pihb_vfpPrimitive.vfp_iNoOfSlices,
            itPrim->pihb_vfpPrimitive.vfp_fXMax-itPrim->pihb_vfpPrimitive.vfp_fXMin,
            itPrim->pihb_vfpPrimitive.vfp_fYMax-itPrim->pihb_vfpPrimitive.vfp_fYMin);
        }
        break;
      }
    case PT_SPHERE:
      {
        strDescription.PrintF("Size(%g,%g,%g) %d meridians %d parallels",
          itPrim->pihb_vfpPrimitive.vfp_fXMax-itPrim->pihb_vfpPrimitive.vfp_fXMin,
          itPrim->pihb_vfpPrimitive.vfp_fZMax-itPrim->pihb_vfpPrimitive.vfp_fZMin,
          itPrim->pihb_vfpPrimitive.vfp_fYMax-itPrim->pihb_vfpPrimitive.vfp_fYMin,
          itPrim->pihb_vfpPrimitive.vfp_iMeridians,
          itPrim->pihb_vfpPrimitive.vfp_iParalels);
        break;
      }
    case PT_TERRAIN:
      {
        strDescription.PrintF("Size(%g,%g,%g) Slices(%d,%d)",
          itPrim->pihb_vfpPrimitive.vfp_fXMax-itPrim->pihb_vfpPrimitive.vfp_fXMin,
          itPrim->pihb_vfpPrimitive.vfp_fZMax-itPrim->pihb_vfpPrimitive.vfp_fZMin,
          itPrim->pihb_vfpPrimitive.vfp_fYMax-itPrim->pihb_vfpPrimitive.vfp_fYMin,
          itPrim->pihb_vfpPrimitive.vfp_iSlicesPerWidth,
          itPrim->pihb_vfpPrimitive.vfp_iSlicesPerHeight);
         if( itPrim->pihb_vfpPrimitive.vfp_fnDisplacement != "")
         {
           strDescription.PrintF( "%s, Displ.: \"%s\", Amp. %g", strDescription,
             (CTString&)(itPrim->pihb_vfpPrimitive.vfp_fnDisplacement.FileName()+
             itPrim->pihb_vfpPrimitive.vfp_fnDisplacement.FileExt()),
             itPrim->pihb_vfpPrimitive.vfp_fAmplitude);
         }
         else
         {
           strDescription.PrintF( "%s, No displacement picture", strDescription);
         }
        break;
      }
    default:
      {
        ASSERTALWAYS( "Invalid primitive type found!");
      }
    }
    CTString strPosition;
    strPosition.PrintF(", Pos(%g,%g,%g), Ang(%g,%g,%g)",
          itPrim->pihb_vfpPrimitive.vfp_plPrimitive.pl_PositionVector(1),
          itPrim->pihb_vfpPrimitive.vfp_plPrimitive.pl_PositionVector(2),
          itPrim->pihb_vfpPrimitive.vfp_plPrimitive.pl_PositionVector(3),
          DegAngle( itPrim->pihb_vfpPrimitive.vfp_plPrimitive.pl_OrientationAngle(1)),
          DegAngle( itPrim->pihb_vfpPrimitive.vfp_plPrimitive.pl_OrientationAngle(2)),
          DegAngle( itPrim->pihb_vfpPrimitive.vfp_plPrimitive.pl_OrientationAngle(3)));

    CTString strTriangularisation;
    switch( itPrim->pihb_vfpPrimitive.vfp_ttTriangularisationType)
    {
    case TT_NONE: strTriangularisation = ""; break;
    case TT_CENTER_VERTEX: strTriangularisation = ", Triang:Center"; break;
    case TT_FROM_VTX00: strTriangularisation = ", Triang:Vtx0"; break;
    case TT_FROM_VTX01: strTriangularisation = ", Triang:Vtx1"; break;
    case TT_FROM_VTX02: strTriangularisation = ", Triang:Vtx2"; break;
    case TT_FROM_VTX03: strTriangularisation = ", Triang:Vtx3"; break;
    case TT_FROM_VTX04: strTriangularisation = ", Triang:Vtx4"; break;
    case TT_FROM_VTX05: strTriangularisation = ", Triang:Vtx5"; break;
    case TT_FROM_VTX06: strTriangularisation = ", Triang:Vtx6"; break;
    case TT_FROM_VTX07: strTriangularisation = ", Triang:Vtx7"; break;
    case TT_FROM_VTX08: strTriangularisation = ", Triang:Vtx8"; break;
    case TT_FROM_VTX09: strTriangularisation = ", Triang:Vtx9"; break;
    case TT_FROM_VTX10: strTriangularisation = ", Triang:Vtx10"; break;
    case TT_FROM_VTX11: strTriangularisation = ", Triang:Vtx11"; break;
    case TT_FROM_VTX12: strTriangularisation = ", Triang:Vtx12"; break;
    case TT_FROM_VTX13: strTriangularisation = ", Triang:Vtx13"; break;
    case TT_FROM_VTX14: strTriangularisation = ", Triang:Vtx14"; break;
    case TT_FROM_VTX15: strTriangularisation = ", Triang:Vtx15"; break;
    }

    INDEX iAddedAs = m_comboPrimitiveHistory.AddString(CString(
      strNo+strDescription+strPosition+strTriangularisation));
    m_comboPrimitiveHistory.SetItemData( iAddedAs, (DWORD_PTR) &itPrim->pihb_vfpPrimitive);
    iCt++;
  }
	return TRUE;
}

void CDlgPgPrimitive::OnAutoCreateMipBrushes()
{
  // move data from page to primitive
  UpdateData( TRUE);
  // place snapped values back to dialog
  UpdateData( FALSE);
}

void CDlgPgPrimitive::OnSelchangeBottomShape()
{
  // move data from page to primitive
  UpdateData( TRUE);
  // place snapped values back to dialog
  UpdateData( FALSE);
}

void CDlgPgPrimitive::OnSelchangeTopShape()
{
  // move data from page to primitive
  UpdateData( TRUE);
  // place snapped values back to dialog
  UpdateData( FALSE);
}


void CDlgPgPrimitive::OnIfRoom()
{
  // move data from page to primitive
  UpdateData( TRUE);
  // place snapped values back to dialog
  UpdateData( FALSE);
}

void CDlgPgPrimitive::OnIfSpiral()
{
  // move data from page to primitive
  UpdateData( TRUE);
  // place snapped values back to dialog
  UpdateData( FALSE);
}

void CDlgPgPrimitive::OnIfOuter()
{
  // move data from page to primitive
  UpdateData( TRUE);
  // place snapped values back to dialog
  UpdateData( FALSE);
}

void CDlgPgPrimitive::OnSelchangePrimitiveHistory()
{
  INDEX iSelected = m_comboPrimitiveHistory.GetCurSel();
  if( iSelected == CB_ERR) return;
  INDEX iCurrent = 0;
  // write history primitives list
  FOREACHINLIST( CPrimitiveInHistoryBuffer, pihb_lnNode, theApp.m_lhPrimitiveHistory, itPrim)
  {
    if( iCurrent == iSelected)
    {
      theApp.m_vfpCurrent = itPrim->pihb_vfpPrimitive;
      break;
    }
    iCurrent++;
  }

  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  ASSERT( pDoc != NULL);
  pDoc->ApplyCurrentPrimitiveSettings();
  UpdateData( FALSE);
}

void CDlgPgPrimitive::OnDropdownPrimitiveHistory()
{
  CRect rectCombo;
  m_comboPrimitiveHistory.GetWindowRect( &rectCombo);
  PIX pixScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
  m_comboPrimitiveHistory.SetDroppedWidth( pixScreenWidth-rectCombo.left);
}

void CDlgPgPrimitive::OnDisplaceBrowse()
{
  CTFileName fnDisplace =  _EngineGUI.FileRequester(
    "Choose picture for displacement", FILTER_TGA FILTER_PCX FILTER_ALL FILTER_END,
    "Displacement pictures directory", "Textures\\",
    theApp.m_vfpCurrent.vfp_fnDisplacement.FileName()+
    theApp.m_vfpCurrent.vfp_fnDisplacement.FileExt());
  if( fnDisplace == "") return;
  theApp.m_vfpCurrent.vfp_fnDisplacement = fnDisplace;
  _bForceRecreatePrimitive = TRUE;
  // move data from page to primitive
  UpdateData( TRUE);
  // place snapped values back to dialog
  UpdateData( FALSE);
}

void CDlgPgPrimitive::OnDisplaceNone()
{
  theApp.m_vfpCurrent.vfp_fnDisplacement = CTString( "");
  _bForceRecreatePrimitive = TRUE;
  // move data from page to primitive
  UpdateData( TRUE);
  // place snapped values back to dialog
  UpdateData( FALSE);
}

void CDlgPgPrimitive::OnContextMenu(CWnd* pWnd, CPoint point)
{
  CMenu menu;
  if( menu.LoadMenu(IDR_PRIMITIVE_SETTINGS))
  {
		CMenu* pPopup = menu.GetSubMenu(0);
    pPopup->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								 point.x, point.y, this);
  }
}

void CDlgPgPrimitive::OnLoadPrimitiveSettings()
{
  // call file requester for list containing worlds to convert
  _fnPrimitiveSettingsSavedAs = _EngineGUI.FileRequester( "Choose file to load primitive settings",
    "Primitive settings (*.prm)\0*.prm\0" FILTER_ALL FILTER_END, "Primitive settings", "", "", NULL, TRUE);
  if( _fnPrimitiveSettingsSavedAs == "") return;

  // read settings to current primitive
  CTFileStream strmFile;
  try
  {
    strmFile.Open_t( _fnPrimitiveSettingsSavedAs);
    theApp.m_vfpCurrent.Read_t( strmFile);
  }
  catch (const char *strError)
  {
    WarningMessage( strError);
    return;
  }
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  ASSERT( pDoc != NULL);
  pDoc->ApplyCurrentPrimitiveSettings();
  UpdateData( FALSE);
}

void CDlgPgPrimitive::OnSavePrimitiveSettings()
{
  if( _fnPrimitiveSettingsSavedAs == "")
    OnSaveAsPrimitiveSettings();

  // save current primitive
  CTFileStream strmFile;
  try
  {
    strmFile.Create_t( _fnPrimitiveSettingsSavedAs);
    theApp.m_vfpCurrent.Write_t( strmFile);
  }
  catch (const char *strError)
  {
    WarningMessage( strError);
  }
}

void CDlgPgPrimitive::OnSaveAsPrimitiveSettings()
{
  // call file requester for list containing worlds to convert
  _fnPrimitiveSettingsSavedAs = _EngineGUI.FileRequester( "Choose file to load primitive settings",
    "Primitive settings (*.prm)\0*.prm\0" FILTER_ALL FILTER_END, "Primitive settings", "", "", NULL, FALSE);
  if( _fnPrimitiveSettingsSavedAs == "") return;
  OnSavePrimitiveSettings();
}

void CDlgPgPrimitive::OnResetPrimitive()
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  if( pDoc == NULL) return;
  pDoc->ResetPrimitive();
}

void CDlgPgPrimitive::OnDropdownTopShape()
{
  m_comboTopShape.SetDroppedWidth(256);
}

void CDlgPgPrimitive::OnDropdownBottomShape()
{
  m_comboBottomShape.SetDroppedWidth(256);
}
