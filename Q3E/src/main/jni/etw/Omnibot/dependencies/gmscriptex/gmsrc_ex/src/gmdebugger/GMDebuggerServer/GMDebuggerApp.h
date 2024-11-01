///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 16 2006)
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

#include <map>
#include <vector>
#include <list>

#ifdef __WXMSW__
	#ifdef _MSC_VER
		#ifdef _DEBUG
			#ifdef UNICODE  // __WXMSW__, _MSC_VER, _DEBUG, UNICODE
				#pragma comment( lib, "wxmsw28ud_propgrid.lib" )
				#pragma comment( lib, "wxmsw28ud_scintilla.lib" )
			#else  // __WXMSW__, _MSC_VER, _DEBUG
				#pragma comment( lib, "wxmsw28d_propgrid.lib" )
				#pragma comment( lib, "wxscintillad.lib" )
			#endif
		#else
			#ifdef UNICODE  // __WXMSW__, _MSC_VER, UNICODE
				#pragma comment( lib, "wxmsw28u_propgrid.lib" )
				#pragma comment( lib, "wxmsw28u_scintilla.lib" )
			#else // __WXMSW__, _MSC_VER
				#pragma comment( lib, "wxmsw28_propgrid.lib" )
				#pragma comment( lib, "wxscintilla.lib" )
			#endif
		#endif
	#endif
#endif

#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/notebook.h> 
#include <wx/panel.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/wxscintilla.h>

#include <wx/socket.h>
#include <wx/url.h>
#include <wx/wfstream.h>

class GMDebuggerThread;
class AboutDialog;

///////////////////////////////////////////////////////////////////////////

typedef std::list<wxString> CommandQueue;

/**
 * Class GMDebuggerFrame
 */
class GMDebuggerFrame : public wxFrame
{
	private:
	
	protected:
		friend class ThreadListCtrl;
		
		enum
		{
			ID_DEBUGGERFRAME = 1000,
			ID_DEBUGGERMENUBAR,
			ID_DEBUGGERSTATUSBAR,
			ID_DEBUGGERTOOLBAR,
			ID_DEBUGTEXTIN,
			ID_DEBUGTEXTOUT,
			ID_DEFAULT,
			ID_FILEOPEN,
			ID_FILESAVE,
			ID_FILESAVEAS,
			ID_CLOSE,
			ID_RUNSCRIPT,
			ID_STOPSCRIPT,
			ID_CODEVIEW,
			ID_ABOUT,

			// id for socket
			SOCKET_SERVER,
			SOCKET_DEBUGGER,
		};
		
		wxMenuBar* DebuggerMenuBar;
		wxStatusBar* m_DebuggerStatusBar;
		wxScintilla* m_CodeView;
		wxTextCtrl* m_DebugTextOut;
		wxTextCtrl* m_DebugTextInput;
		AboutDialog *m_AboutDlg;
	
		// Virtual event handlers, overide them in your derived class
		virtual void OnDebugTextInput( wxCommandEvent& event );
	public:
		GMDebuggerFrame( wxWindow* parent, int id = -1, wxString title = wxT("Game Monkey Debugger Server Alpha 1.0"), wxPoint pos = wxDefaultPosition, wxSize size = wxSize( 800,700 ), int style = wxCAPTION|wxCLOSE_BOX|wxMAXIMIZE|wxMAXIMIZE_BOX|wxMINIMIZE|wxMINIMIZE_BOX|wxSYSTEM_MENU|wxCLIP_CHILDREN|wxTAB_TRAVERSAL );
	
		//////////////////////////////////////////////////////////////////////////

		CommandQueue		m_CommandQueue;
		wxString			m_LastCommand;
		wxCriticalSection	m_QueueCritSection;

		//////////////////////////////////////////////////////////////////////////

		DECLARE_EVENT_TABLE()

		void OnCloseApp(wxCloseEvent& event);
		void OnIdle(wxIdleEvent& event);
		void OnFileOpen(wxCommandEvent& event);
		void OnFileSave(wxCommandEvent& event);
		void OnFileSaveAs(wxCommandEvent& event);
		void OnClose(wxCommandEvent& event);
		void OnRunScript(wxCommandEvent& event);
		void OnStopScript(wxCommandEvent& event);
		void OnAbout(wxCommandEvent& event);
		void OnSocketEventServer(wxSocketEvent& event);
		void OnSocketEventDebugger(wxSocketEvent& event);
		
		//////////////////////////////////////////////////////////////////////////
		//static void ServerSendMessage(gmDebuggerSession * a_session, const void * a_command, int a_len);
		//static const void *ServerPumpMessage(gmDebuggerSession * a_session, int &a_len);
		//////////////////////////////////////////////////////////////////////////
public:
		static GMDebuggerFrame *m_Instance;

		GMDebuggerThread *m_NetThread;
		
		void LogText(const wxString &aText);
		void LogError(const wxString &aText);

		void AddMessageToQueue(const wxString &_str);

		wxScintilla* GetCodeView() { return m_CodeView; }
};

class gmMachine;
class GMDebuggerThread : public wxThread
{
public:
	GMDebuggerThread();

	// thread execution starts here
	virtual void *Entry();

	// called when the thread exits - whether it terminates normally or is
	// stopped with Delete() (but not when it is Kill()ed!)
	virtual void OnExit();

	void KillAllThreads();

	void DebuggerConnected();
	void DebuggerDisConnected();
private:
	gmMachine *m_Machine;
	
	wxCriticalSection	m_MachineCritSection;
};

///////////////////////////////////////////////////////////////////////////////
/// Class AboutDialog
///////////////////////////////////////////////////////////////////////////////
class AboutDialog : public wxDialog 
{
	DECLARE_EVENT_TABLE()
private:


protected:
	enum
	{
		ID_DEFAULT = wxID_ANY, // Default
		ID_ABOUT_DIALOG = 2000,
	};

	wxTextCtrl* m_textCtrl3;
	wxButton* m_AboutBtnOk;

	// Virtual event handlers, overide them in your derived class
	virtual void OnAboutOk( wxCommandEvent& event );


public:
	AboutDialog( wxWindow* parent, int id = ID_ABOUT_DIALOG, wxString title = wxT("About"), wxPoint pos = wxDefaultPosition, wxSize size = wxSize( 400,200 ), int style = 0 );

};
#endif //__noname__
