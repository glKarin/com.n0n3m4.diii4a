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

#ifndef __DIALOGGOTOLINE_H__
#define __DIALOGGOTOLINE_H__

// DialogGoToLine dialog

class DialogGoToLine : public CDialog {

	DECLARE_DYNAMIC(DialogGoToLine)

public:

						DialogGoToLine( CWnd* pParent = NULL );   // standard constructor
	virtual				~DialogGoToLine();

	enum				{ IDD = IDD_DIALOG_GOTOLINE };

	void				SetRange( int firstLine, int lastLine );
	int					GetLine( void ) const;

protected:
	virtual BOOL		OnInitDialog();
	virtual void		DoDataExchange( CDataExchange* pDX );    // DDX/DDV support
	afx_msg void		OnBnClickedOk();

	DECLARE_MESSAGE_MAP()

private:

	CEdit				numberEdit;
	int					firstLine;
	int					lastLine;
	int					line;
};

#endif /* !__DIALOGGOTOLINE_H__ */
