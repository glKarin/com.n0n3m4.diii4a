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

// PropertyComboBar.h : header file
//
#ifndef PROPERTYCOMBOBAR_H
#define PROPERTYCOMBOBAR_H 1

/////////////////////////////////////////////////////////////////////////////
// CPropertyComboBar dialog

class CPropertyComboBar : public CDialogBar
{
// Construction
public:
  BOOL Create( CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle,
               UINT nID, BOOL = TRUE);
  BOOL OnIdle(LONG lCount);
  // show/hide controls depending on editing property type
  void ArrangeControls();
  CPropertyID *GetSelectedProperty();
  void SelectPreviousEmptyTarget(void);
  void SelectPreviousProperty(void);
  void SelectNextEmptyTarget(void);
  void SelectNextProperty(void);
  void CircleTargetProperties(INDEX iDirection, BOOL bOnlyEmptyTargets);
  void SetIntersectingFileName();
  void SelectAxisRadio(CWnd *pwndToSelect);
  void SetColorPropertyToEntities( COLOR colNewColor);
  void SetFirstValidEmptyTargetProperty(CEntity *penTarget);
  void ClearAllTargets(CEntity *penClicked);
  void SelectProperty(CEntityProperty *penpToMatch);
// Attributes
public:
  CSize m_Size;

  float m_fEditingFloat;
  float m_fEditingHeading;
  float m_fEditingPitch;
  float m_fEditingBanking;
  INDEX m_iEditingIndex;
  INDEX m_iEditStringMaxChars;
  float m_fEditingBBoxMin;
  float m_fEditingBBoxMax;
  INDEX m_iXYZAxis;
  CString m_strFloatRange;
  CString m_strIndexRange;
  CString m_strChooseColor;
  CString m_strFileName;
  CString m_strEditingString;
  CString m_strEntityClass;
  CString m_strEntityName;
  CString m_strEntityDescription;
  COLOR m_colLastColor;

  CPropertyComboBox m_PropertyComboBox;
  CCtrlEnumCombo m_EditEnumComboBox;
  CCtrlEditString m_EditStringCtrl;
  CCtrlEditFloat m_EditFloatCtrl;
  CCtrlEditFloat m_EditIndexCtrl;
  CCtrlAxisRadio m_XCtrlAxisRadio;
  CCtrlAxisRadio m_YCtrlAxisRadio;
  CCtrlAxisRadio m_ZCtrlAxisRadio;
  CCtrlEditFloat m_EditBBoxMinCtrl; 
  CCtrlEditFloat m_EditBBoxMaxCtrl;
  CCtrlEditBoolean m_EditBoolCtrl;
  CColoredButton m_EditColorCtrl;
  CCtrlBrowseFile m_BrowseFileCtrl;
  
  CCtrlEditFloat m_EditHeading;
  CCtrlEditFloat m_EditPitch;
  CCtrlEditFloat m_EditBanking;

  CCtrlEditBoolean m_EditEasySpawn;
  CCtrlEditBoolean m_EditNormalSpawn;
  CCtrlEditBoolean m_EditHardSpawn;
  CCtrlEditBoolean m_EditExtremeSpawn;
  CCtrlEditBoolean m_EditDifficulty_1;
  CCtrlEditBoolean m_EditDifficulty_2;
  CCtrlEditBoolean m_EditDifficulty_3;
  CCtrlEditBoolean m_EditDifficulty_4;
  CCtrlEditBoolean m_EditDifficulty_5;


  CCtrlEditBoolean m_EditSingleSpawn;
  CCtrlEditBoolean m_EditCooperativeSpawn;
  CCtrlEditBoolean m_EditDeathMatchSpawn;
  CCtrlEditBoolean m_EditGameMode_1;
  CCtrlEditBoolean m_EditGameMode_2;
  CCtrlEditBoolean m_EditGameMode_3;
  CCtrlEditBoolean m_EditGameMode_4;
  CCtrlEditBoolean m_EditGameMode_5;
  CCtrlEditBoolean m_EditGameMode_6;
  
  CCtrlEditFlags m_ctrlEditFlags;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropertyComboBar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

// Implementation
public:
  virtual CSize CalcDynamicLayout( int nLength, DWORD dwMode );
  void SetIntersectingEntityClassName(void);
  CEntity *GetSelectedEntityPtr(void);

	// Generated message map functions
	//{{AFX_MSG(CPropertyComboBar)
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
  afx_msg void OnNoFile();
	afx_msg void OnNoTarget();
	//}}AFX_MSG
  afx_msg void OnUpdateBrowseFile( CCmdUI* pCmdUI );
  afx_msg void OnUpdateNoFile( CCmdUI* pCmdUI );
  afx_msg void OnUpdateNoTarget( CCmdUI* pCmdUI );
  afx_msg void OnUpdateEditColor( CCmdUI* pCmdUI );
  afx_msg void OnUpdateEditFlags( CCmdUI* pCmdUI );
	DECLARE_MESSAGE_MAP()
};
#endif // PROPERTYCOMBOBAR_H
