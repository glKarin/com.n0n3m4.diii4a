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

#if !defined(AFX_DLGEDITTERRAINLAYER_H__985474D0_7967_4656_8ED1_DB317AE52FA3__INCLUDED_)
#define AFX_DLGEDITTERRAINLAYER_H__985474D0_7967_4656_8ED1_DB317AE52FA3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgEditTerrainLayer.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgEditTerrainLayer dialog

class CDlgEditTerrainLayer : public CDialog
{
// Construction
public:
	CDlgEditTerrainLayer(CWnd* pParent = NULL);   // standard constructor 

// Dialog Data
	//{{AFX_DATA(CDlgEditTerrainLayer)
	enum { IDD = IDD_EDIT_TERRAIN_LAYER };
	BOOL	m_bAutoGenerate;
	float	m_fAltitudeMax;
	float	m_fAltitudeMaxFade;
	float	m_fAltitudeMin;
	float	m_fAltitudeMinFade;
	float	m_fLayerCoverage;
	CString	m_strLayerName;
	float	m_fTextureOffsetX;
	float	m_fTextureOffsetY;
	float	m_fTextureRotationU;
	float	m_fTextureRotationV;
	float	m_fSlopeMax;
	float	m_fSlopeMaxFade;
	float	m_fSlopeMin;
	float	m_fSlopeMinFade;
	float	m_fTextureStretchX;
	float	m_fTextureStretchY;
	float	m_fCoverageFade;
	float	m_fAltitudeMaxNoise;
	BOOL	m_bApplyMaxAltitude;
	BOOL	m_bApplyMaxSlope;
	BOOL	m_bApplyMinAltitude;
	BOOL	m_bApplyMinSlope;
	float	m_fAltitudeMinNoise;
	float	m_fSlopeMaxNoise;
	float	m_fSlopeMinNoise;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgEditTerrainLayer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgEditTerrainLayer)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGEDITTERRAINLAYER_H__985474D0_7967_4656_8ED1_DB317AE52FA3__INCLUDED_)
