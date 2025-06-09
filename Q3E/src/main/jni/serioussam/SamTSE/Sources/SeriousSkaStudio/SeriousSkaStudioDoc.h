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

// SeriousSkaStudioDoc.h : interface of the CSeriousSkaStudioDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERIOUSSKASTUDIODOC_H__ADB1B81A_7B4C_4335_81F4_E9D1E51B7C68__INCLUDED_)
#define AFX_SERIOUSSKASTUDIODOC_H__ADB1B81A_7B4C_4335_81F4_E9D1E51B7C68__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Engine/Base/Serial.h>

class CSeriousSkaStudioDoc : public CDocument
{
protected: // create from serialization only
	CSeriousSkaStudioDoc();
	DECLARE_DYNCREATE(CSeriousSkaStudioDoc)

// Attributes
public:
  CModelInstance  *m_ModelInstance;
  BOOL  m_bModelInstanceChanged;

  FLOAT m_fSpeedZ;// speed Z
  FLOAT m_fLoopSecends; // how long to loop moving by z
  FLOAT fCustomMeshLodDist;
  FLOAT fCustomSkeletonLodDist;
  BOOL  bAutoMiping;    
  BOOL  bShowGround;
  BOOL  bShowLights;
  BOOL  bAnimLoop;  // will played anims be looping
  BOOL  bShowColisionBox;
  BOOL bShowAllFramesBBox;
  
  COLOR m_colAmbient; // Ambient color
  COLOR m_colLight;   // Light color
  FLOAT3D m_vLightDir;// Light direction

  CTimerValue m_tvStart;      // each view has its own timing so it can be pause
  CTimerValue m_tvPauseStart; // time when pause accured
  CTimerValue m_tvPauseTime;  // how much time was this paused
  BOOL  m_bViewPaused;        // is this view paused

// Operations
public:
	void OnIdle(void);
  void SetTimerForDocument();
  // set flag that this document has changed and need to be saved
  void MarkAsChanged();
  INDEX BeforeDocumentClose();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSeriousSkaStudioDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
  virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSeriousSkaStudioDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CSeriousSkaStudioDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERIOUSSKASTUDIODOC_H__ADB1B81A_7B4C_4335_81F4_E9D1E51B7C68__INCLUDED_)
