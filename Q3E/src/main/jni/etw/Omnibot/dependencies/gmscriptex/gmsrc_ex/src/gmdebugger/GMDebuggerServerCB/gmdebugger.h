/***************************************************************
 * Name:      gmdebugger.h
 * Purpose:   Code::Blocks plugin
 * Author:    <>
 * Copyright: (c) 
 * License:   GPL
 **************************************************************/

#ifndef GMDEBUGGER_H
#define GMDEBUGGER_H

#if defined(__GNUG__) && !defined(__APPLE__)
	#pragma interface "gmdebugger.h"
#endif
// For compilers that support precompilation, includes <wx/wx.h>
#include <wx/wxprec.h>

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include <cbplugin.h> // the base class we 're inheriting
#include <settings.h> // needed to use the Code::Blocks SDK

class GMDebugger : public cbDebuggerPlugin
{
	public:
		GMDebugger();
		~GMDebugger();
		int Configure();
		void BuildMenu(wxMenuBar* menuBar);
		void BuildModuleMenu(const ModuleType type, wxMenu* menu, const wxString& arg);
		bool BuildToolBar(wxToolBar* toolBar);
		int Debug();
		void CmdContinue();
		void CmdNext();
		void CmdStep();
		void CmdToggleBreakpoint();
		void CmdStop();
		bool IsRunning() const;
		int GetExitCode() const;
		void OnAttach(); // fires when the plugin is attached to the application
		void OnRelease(bool appShutDown); // fires when the plugin is released from the application
	protected:
	private:
		DECLARE_EVENT_TABLE()
};

// Declare the plugin's hooks
CB_DECLARE_PLUGIN();

#endif // GMDEBUGGER_H

