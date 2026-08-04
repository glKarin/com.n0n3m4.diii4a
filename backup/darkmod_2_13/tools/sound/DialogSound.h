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

#ifndef __DIALOGSOUND_H__
#define __DIALOGSOUND_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDialogSound dialog

class CDialogSound : public CDialog {
public:
							CDialogSound(CWnd* pParent = NULL);   // standard constructor\

	void					Set( const idDict *source );
	void					Get( idDict *dest );

	enum { NONE, SOUNDS, SOUNDPARENT, WAVES, WAVEDIR, INUSESOUNDS };

	//{{AFX_VIRTUAL(CDialogSound)
	virtual BOOL			OnInitDialog();
	virtual void			DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CDialogSound)
	afx_msg void			OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void			OnMove( int x, int y );
	afx_msg void			OnDestroy();
	afx_msg void			OnBtnSavemap();
	afx_msg void			OnBtnSwitchtogame();
	afx_msg void			OnBtnApply();
	afx_msg void			OnChangeEditVolume();
	afx_msg void			OnBtnRefresh();
	afx_msg void			OnBtnPlaysound();
	afx_msg void			OnDblclkTreeSounds(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void			OnSelchangedTreeSounds(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void			OnCheckPlay();
	afx_msg void			OnBtnEdit();
	afx_msg void			OnBtnDrop();
	afx_msg void			OnBtnGroup();
	afx_msg void			OnBtnSavemapas();
	afx_msg void			OnBtnYup();
	afx_msg void			OnBtnYdn();
	afx_msg void			OnBtnXdn();
	afx_msg void			OnBtnXup();
	afx_msg void			OnBtnZup();
	afx_msg void			OnBtnZdn();
	afx_msg void			OnBtnTrigger();
	afx_msg void			OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void			OnCheckGrouponly();
	afx_msg void			OnSelchangeComboGroups();
	afx_msg void			OnSelchangeComboSpeakers();
	afx_msg void			OnBtnDown();
	afx_msg void			OnBtnUp();
	afx_msg void			OnBtnRefreshspeakers();
	afx_msg void			OnBtnRefreshwave();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	//{{AFX_DATA(CDialogSound)
	enum					{ IDD = IDD_DIALOG_SOUND };
	CComboBox				comboSpeakers;
	CComboBox				comboGroups;
	CEdit					editVolume;
	CTreeCtrl				treeSounds;
	CString					strName;
	float					fVolume;
	float					fMax;
	float					fMin;
	CString					strShader;
	BOOL					bPlay;
	int						bTriggered;
	int						bOmni;
	CString					strGroup;
	BOOL					bGroupOnly;
	BOOL					bOcclusion;
	float					leadThrough;
	BOOL					plain;
	float					random;
	float					wait;
	float					shakes;
	BOOL					looping;
	BOOL					unclamped;
	//}}AFX_DATA

	CString					playSound;
	idHashTable<HTREEITEM>	quickTree;
	HTREEITEM				inUseTree;
private:
	void					AddSounds(bool rootItem);
	HTREEITEM				AddStrList(const char *root, const idStrList &list, int id);
	HTREEITEM				InsertTreeItem(const char *name, const char *fullName, HTREEITEM item);
	idStr					RebuildItemName(const char *root, HTREEITEM item);
	void					UpdateSelectedOrigin(float x, float y, float z);
	void					AddGroups();
	void					AddSpeakers();
	void					AddInUseSounds();
	void					ApplyChanges( bool volumeOnly = false , bool updateInUseTree = true );
	void					SetWaveSize( const char *p = NULL );
	void					SetVolume( float f );
	virtual BOOL			PreTranslateMessage(MSG* pMsg);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif /* !__DIALOGSOUND_H__ */
