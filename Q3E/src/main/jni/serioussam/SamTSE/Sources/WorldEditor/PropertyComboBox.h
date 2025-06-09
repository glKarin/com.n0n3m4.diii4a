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

// PropertyComboBox.h : header file
//
#ifndef PROPERTYCOMBOBOX_H
#define PROPERTYCOMBOBOX_H 1

class CPropertyComboBar;
class CWorldEditorDoc;
/////////////////////////////////////////////////////////////////////////////
// CPropertyComboBox window

/*
 * Class used for creating property intersection of selected entities
 */
class CPropertyID
{
public:
  inline CPropertyID( CTString strName, CEntityProperty::PropertyType eptType,
                      CEntityProperty *penpProperty, CAnimData *padAnimData)
  {
    pid_strName = strName;
    pid_eptType = eptType;
    pid_penpProperty = penpProperty;
    if( eptType == CEntityProperty::EPT_ANIMATION) pid_padAnimData = padAnimData;
    else                                           pid_padAnimData = NULL;
    if( penpProperty != NULL) 
      pid_chrShortcutKey = penpProperty->ep_chShortcut;
    else
      pid_chrShortcutKey = 0;
  };
  // node for linking
  CListNode pid_lnNode;
  // descriptive name of this property
  CTString pid_strName;
  // shortcut key for this property
  char pid_chrShortcutKey;
  // name of anim data object (if any)
  CAnimData *pid_padAnimData;
  // property type
  CEntityProperty::PropertyType pid_eptType;
  // property ptr
  CEntityProperty *pid_penpProperty;
};

class CPropertyComboBox : public CComboBox
{
// Construction
public:
	CPropertyComboBox();
  BOOL OnIdle(LONG lCount);
  void DisableCombo();

// Attributes
public:
  // name of last edited property
  CTString m_strLastPropertyName;
  // ptr to parent dialog
  CPropertyComboBar *m_pDialog;
  // list head for holding intersected properties
  CListHead m_lhProperties;
  CUpdateableRT m_udComboEntries;
  INDEX m_iLastMode;
  CWorldEditorDoc *m_pLastDoc;

// Operations
public:
  // sets ptr to parent dialog
  void SetDialogPtr( CPropertyComboBar *pDialog);
  // adds intersecting properties of given entity into m_lhProperties list
  void JoinProperties( CEntity *penEntity, BOOL bIntersect);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropertyComboBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPropertyComboBox();
  void SelectProperty(void);

	// Generated message map functions
protected:
	//{{AFX_MSG(CPropertyComboBox)
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSelchange();
	afx_msg void OnDropdown();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // PROPERTYCOMBOBOX_H
