/***************************************************************
 * Name:      gmdebugger.cpp
 * Purpose:   Code::Blocks plugin
 * Author:    <>
 * Copyright: (c) 
 * License:   GPL
 **************************************************************/

#if defined(__GNUG__) && !defined(__APPLE__)
	#pragma implementation "gmdebugger.h"
#endif

#include "gmdebugger.h"
#include <licenses.h> // defines some common licenses (like the GPL)

// Implement the plugin's hooks
CB_IMPLEMENT_PLUGIN(GMDebugger);

BEGIN_EVENT_TABLE(GMDebugger, cbDebuggerPlugin)
	// add events here...
END_EVENT_TABLE()

GMDebugger::GMDebugger()
{
	//ctor
	m_PluginInfo.name = _T("GMDebugger");
	m_PluginInfo.title = _("");
	m_PluginInfo.version = _T("");
	m_PluginInfo.description = _("");
	m_PluginInfo.author = _T("");
	m_PluginInfo.authorEmail = _T("");
	m_PluginInfo.authorWebsite = _T("");
	m_PluginInfo.thanksTo = _("");
	m_PluginInfo.license = LICENSE_GPL;
	m_PluginInfo.hasConfigure = true;
}

GMDebugger::~GMDebugger()
{
	//dtor
}

void GMDebugger::OnAttach()
{
	// do whatever initialization you need for your plugin
	// NOTE: after this function, the inherited member variable
	// m_IsAttached will be TRUE...
	// You should check for it in other functions, because if it
	// is FALSE, it means that the application did *not* "load"
	// (see: does not need) this plugin...
}

void GMDebugger::OnRelease(bool appShutDown)
{
	// do de-initialization for your plugin
	// if appShutDown is false, the plugin is unloaded because Code::Blocks is being shut down,
	// which means you must not use any of the SDK Managers
	// NOTE: after this function, the inherited member variable
	// m_IsAttached will be FALSE...
}

int GMDebugger::Configure()
{
	//create and display the configuration dialog for your plugin
	NotImplemented(_T("GMDebugger::Configure()"));
	return -1;
}

void GMDebugger::BuildMenu(wxMenuBar* menuBar)
{
	//The application is offering its menubar for your plugin,
	//to add any menu items you want...
	//Append any items you need in the menu...
	//NOTE: Be careful in here... The application's menubar is at your disposal.
	NotImplemented(_T("GMDebugger::OfferMenuSpace()"));
}

void GMDebugger::BuildModuleMenu(const ModuleType type, wxMenu* menu, const wxString& arg)
{
	//Some library module is ready to display a pop-up menu.
	//Check the parameter "type" and see which module it is
	//and append any items you need in the menu...
	//TIP: for consistency, add a separator as the first item...
	NotImplemented(_T("GMDebugger::OfferModuleMenuSpace()"));
}

bool GMDebugger::BuildToolBar(wxToolBar* toolBar)
{
	//The application is offering its toolbar for your plugin,
	//to add any toolbar items you want...
	//Append any items you need on the toolbar...
	NotImplemented(_T("GMDebugger::BuildToolBar()"));
	// return true if you add toolbar items
	return false;
}

int GMDebugger::Debug()
{
	//actual debugging session starts here
	NotImplemented(_T("GMDebugger::Debug()"));
	return -1;
}
void GMDebugger::CmdContinue()
{
	//tell debugger to continue
	NotImplemented(_T("GMDebugger::CmdContinue()"));
}
void GMDebugger::CmdNext()
{
	//tell debugger to step one line of code
	NotImplemented(_T("GMDebugger::CmdNext()"));
}
void GMDebugger::CmdStep()
{
	//tell debugger to step one instruction (following inside functions, if needed)
	NotImplemented(_T("GMDebugger::CmdStep()"));
}
void GMDebugger::CmdStop()
{
	//tell debugger to end debugging session
	NotImplemented(_T("GMDebugger::CmdStop()"));
}
bool GMDebugger::IsRunning() const
{
	//return true if session is active
	//NotImplemented(_T("GMDebugger::IsRunning()"));
	return false;
}
int GMDebugger::GetExitCode() const
{
	//return last session exit code
	//NotImplemented(_T("GMDebugger::GetExitCode()"));
	return -1;
}
