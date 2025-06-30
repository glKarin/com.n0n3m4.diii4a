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

// ModelerDoc.h : interface of the CModelerDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CModelerDoc : public CDocument
{
protected: // create from serialization only
	CModelerDoc();
	DECLARE_DYNCREATE(CModelerDoc)

// Attributes
public:
  CSoundObject m_soSoundObject;
  BOOL CreateModelFromScriptFile( CTFileName fnScrFileName, char *strError);
  CTString GetModelDirectory( void);
  CTString GetModelName( void);
  void SelectMipModel( INDEX iMipToSelect);
  void ClearAttachments( void);
  void SetupAttachments( void);
  CAttachmentModelObject *GetAttachmentModelObject( INDEX iAttachment);
	void OnIdle(void);
	CEditModel m_emEditModel;

  // overridden from mfc to discard rendering precalculations
  void SetModifiedFlag( BOOL bModified = TRUE );

  void ClearSurfaceSelection(void);
  void SelectAllSurfaces(void);
  INDEX GetCountOfSelectedSurfaces(void);
  INDEX GetOnlySelectedSurface(void);
  void SelectSurface(INDEX iSurface, BOOL bClearRest);
  void SelectPreviousSurface(void);
  void SelectNextSurface(void);
  void ToggleSurfaceSelection( INDEX iSurfaceToToggle);
  void SpreadSurfaceSelection( void);

// Operations
public:
  // Clipboard variables used for storing copy/paste data
  FLOAT3D m_f3ClipboardCenter;
  FLOAT m_fClipboardZoom;
  FLOAT3D m_f3ClipboardHPB;
  INDEX m_iCurrentMip;

  // ARGH!! To be able to see right open doc messages
  BOOL m_bDocLoadedOk;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModelerDoc)
	public:
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CModelerDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
public:
	//{{AFX_MSG(CModelerDoc)
	afx_msg void OnFileAddTexture();
	afx_msg void OnUpdateFileAddTexture(CCmdUI* pCmdUI);
	afx_msg void OnNextSurface();
	afx_msg void OnPrevSurface();
	afx_msg void OnLinkSurfaces();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
