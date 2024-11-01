///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb  1 2007)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __noname__
#define __noname__

// Define WX_GCH in order to support precompiled headers with GCC compiler.
// You have to create the header "wx_pch.h" and include all files needed
// for compile your gui inside it.
// Then, compile it and place the file "wx_pch.h.gch" into the same
// directory that "wx_pch.h".
#ifdef WX_GCH
#include <wx_pch.h>
#else
#include <wx/wx.h>
#endif

#include <wx/menu.h>
#include <wx/wxscintilla.h>
#ifdef __VISUALC__
#include <wx/link_additions.h>
#endif //__VISUALC__
#include <wx/button.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class GMDebuggerFrame
///////////////////////////////////////////////////////////////////////////////
class GMDebuggerFrame : public wxFrame 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnDebugTextInput( wxCommandEvent& event ){ OnDebugTextInput( event ); }
		
	
	protected:
		enum
		{
			ID_DEBUGGERFRAME = 1000,
			ID_DEBUGGERMENUBAR,
			ID_FILEOPEN,
			ID_FILESAVE,
			ID_FILESAVEAS,
			ID_CLOSE,
			ID_RUNSCRIPT,
			ID_DEBUGGERSTATUSBAR,
			ID_CODEVIEW,
			ID_DEBUGTEXTOUT,
			ID_DEBUGTEXTIN,
		};
		
		wxMenuBar* DebuggerMenuBar;
		wxStatusBar* m_DebuggerStatusBar;
		wxScintilla* m_CodeView;
		wxTextCtrl* m_DebugTextOut;
		wxTextCtrl* m_DebugTextInput;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnDebugTextInput( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		GMDebuggerFrame( wxWindow* parent, int id = ID_DEBUGGERFRAME, wxString title = wxT("Game Monkey Debugger Server 1.0"), wxPoint pos = wxDefaultPosition, wxSize size = wxSize( 800,700 ), int style = wxCAPTION|wxCLOSE_BOX|wxMAXIMIZE|wxMAXIMIZE_BOX|wxMINIMIZE|wxMINIMIZE_BOX|wxSYSTEM_MENU|wxCLIP_CHILDREN|wxTAB_TRAVERSAL );
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class AboutDialog
///////////////////////////////////////////////////////////////////////////////
class AboutDialog : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnAboutOk( wxCommandEvent& event ){ OnAboutOk( event ); }
		
	
	protected:
		wxTextCtrl* m_textCtrl3;
		wxButton* m_AboutBtnOk;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnAboutOk( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		AboutDialog( wxWindow* parent, int id = wxID_ABOUT_DIALOG, wxString title = wxT("About"), wxPoint pos = wxDefaultPosition, wxSize size = wxSize( 400,200 ), int style = 0 );
	
};

#endif //__noname__
