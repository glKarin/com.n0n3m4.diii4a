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
#pragma once

#include "MaterialEditor.h"

/**
* View in the Material Editor that functions as a Doom III
* console. It allows users to view console output as well as issue 
* console commands to the engine.
*/
class ConsoleView : public CFormView
{

public:
	enum{ IDD = IDD_CONSOLE_FORM };

	CEdit			editConsole;
	CEdit			editInput;
	
	idStr			consoleStr;
	idStrList		consoleHistory;
	idStr			currentCommand;
	int				currentHistoryPosition;
	bool			saveCurrentCommand;

public:
	virtual			~ConsoleView();

	//Public Operations
	void			AddText(const char *msg);
	void			SetConsoleText ( const idStr& text );
	void			ExecuteCommand ( const idStr& cmd = "" );
		
	
protected:
	ConsoleView();
	DECLARE_DYNCREATE(ConsoleView)

	//CFormView Overrides
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	virtual void	DoDataExchange(CDataExchange* pDX);
	virtual void	OnInitialUpdate();

	//Message Handlers
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()	

	//Protected Operations
	const char*		TranslateString(const char *buf);


};
