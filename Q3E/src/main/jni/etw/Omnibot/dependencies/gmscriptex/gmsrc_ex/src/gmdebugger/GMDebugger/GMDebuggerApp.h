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

class gmDebuggerSession;

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
			ID_AUTOPANEL = 1000,
			ID_AUTOSVARIABLELIST,
			ID_BREAKALL,
			ID_BREAKCURRENT,
			ID_CALLSTACK,
			ID_CLOSE,
			ID_CODEVIEW,
			ID_CONNECT,
			ID_DEBUGGERFRAME,
			ID_DEBUGGERMENUBAR,
			ID_DEBUGGERSTATUSBAR,
			ID_DEBUGGERTOOLBAR,
			ID_DEBUGTEXTIN,
			ID_DEBUGTEXTOUT,
			ID_DEFAULT,
			ID_FILEOPEN,
			ID_FILESAVE,
			ID_FILESAVEAS,
			ID_GLOBALS,
			ID_GLOBALSSVARIABLELIST,
			ID_RESUMEALL,
			ID_RESUMECURRENT,
			ID_STEPINTO,
			ID_STEPOUT,
			ID_STEPOVER,
			ID_STOPDEBUGGING,
			ID_THREADLIST,
			ID_TOGGLEBREAKPOINT,
			ID_ABOUT,

			// id for socket
			SOCKET_ID
		};
		
		

		wxMenuBar* DebuggerMenuBar;
		wxStatusBar* m_DebuggerStatusBar;
		wxToolBar* m_DebuggerToolbar;
		wxScintilla* m_CodeView;
		wxTextCtrl* m_DebugTextOut;
		wxTextCtrl* m_DebugTextInput;
		wxNotebook* m_VarNotebook;
		wxScrolledWindow* m_GlobalsPanelWindow;
		wxPropertyGrid* m_GlobalsPropGrid;
		wxScrolledWindow* m_AutoPanelWindow;
		wxPropertyGrid* m_AutosPropGrid;
		wxListCtrl* m_ThreadList;
		wxListCtrl* m_Callstack;
		wxPanel* m_panel3;
		AboutDialog *m_AboutDlg;
	
		// Virtual event handlers, overide them in your derived class
		virtual void OnDebugTextInput( wxCommandEvent& event );
	public:
		GMDebuggerFrame( wxWindow* parent, int id = -1, wxString title = wxT("Game Monkey Debugger Alpha 0.6"), wxPoint pos = wxDefaultPosition, wxSize size = wxSize( 800,700 ), int style = wxCAPTION|wxCLOSE_BOX|wxMAXIMIZE|wxMAXIMIZE_BOX|wxMINIMIZE|wxMINIMIZE_BOX|wxSYSTEM_MENU|wxCLIP_CHILDREN|wxTAB_TRAVERSAL );
	
		//////////////////////////////////////////////////////////////////////////
		struct gmdBreakPoint
		{
			int m_enabled;
			int m_allThreads;
			int m_responseId;
			int m_lineNumber;
			int m_threadId;
			int m_sourceId;
		};

		struct ThreadInfo
		{
			enum ThreadState
			{
				Running,
				Sleeping,
				Blocked,
				Killed,
				Exception,
			};
			int			m_ThreadId;
			ThreadState	m_ThreadState;

			// expiration time?
		};

		//////////////////////////////////////////////////////////////////////////

		enum Markers
		{
			MarkerArrow = 2,
			MarkerBreakPoint
		};

		gmDebuggerSession	*m_gmDebugger;
		gmdBreakPoint		m_BreakPoint;
		
		typedef std::vector<ThreadInfo> ThreadList;
		typedef std::map<unsigned int, wxString> SourceInfo;
		typedef std::list<wxString> CommandQueue;
		typedef std::map<int, wxPGId> ParentPropertyMap;

		ThreadList	m_Threads;
		SourceInfo	m_SourceInfo;
		ParentPropertyMap m_GlobalVariableInfoMap;
		ParentPropertyMap m_AutoVariableInfoMap;
		ParentPropertyMap &m_ActiveInfoMap;

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
		void OnAbout(wxCommandEvent& event);
		void OnConnect(wxCommandEvent& event);
		void OnSocketEvent(wxSocketEvent& event);
		
		void OnStepInto(wxCommandEvent& event);
		void OnStepOut(wxCommandEvent& event);
		void OnStepOver(wxCommandEvent& event);
		void OnRunCurrentThread(wxCommandEvent& event);
		void OnBreakCurrentThread(wxCommandEvent& event);
		void OnKillCurrentThread(wxCommandEvent& event);
		void OnToggleBreakPoint(wxCommandEvent& event);
		void OnBreakAllThreads(wxCommandEvent& event);
		void OnKillAllThreads(wxCommandEvent& event);
		void OnResumeAllThreads(wxCommandEvent& event);

		void OnGlobalListExpand(wxPropertyGridEvent &event);

		void OnCallStackSelected(wxListEvent& event);
		void OnThreadSelected(wxListEvent& event);

		void OnCodeMarginClicked(wxScintillaEvent &event);

		//////////////////////////////////////////////////////////////////////////
		static void ServerSendMessage(gmDebuggerSession * a_session, const void * a_command, int a_len);
		static const void *ServerPumpMessage(gmDebuggerSession * a_session, int &a_len);
		//////////////////////////////////////////////////////////////////////////
public:
		static GMDebuggerFrame *m_Instance;

		GMDebuggerThread *m_NetThread;

		int m_currentDebugThread;
		int m_currentCallFrame;
		int m_lineNumberOnSourceRcv;
		int m_sourceId;
		int m_currentPos;
		int m_responseId;
		
		void LogText(const wxString &aText);
		void LogError(const wxString &aText);

		void StartUpdateThreadList();
		void FindAddThread(int a_threadId, int a_state, bool a_select);
		void EndUpdateThreadList();
		void RemoveThread(int a_threadId);
		void SelectThread(int a_threadId);
		void ClearThreads();
		void ClearCurrentContext();
		bool SetSource(unsigned int a_sourceId, const char * a_source);
		void SetSourceName(const char * a_sourceName);
		void SetLine(int a_line);

		void AddToCallstack(long a_item, const wxString &a_string);
		
		void ClearCallstack();

		void BeginContext(int a_VarId);
		void ContextVariable(const wxString &a_name, const wxString &a_value, int a_varType, int a_VarId);
		void EndContext();

		wxPGId			m_ParentId;
		wxPropertyGrid	*m_ActivePropertyGrid;

		void BeginGlobals(int a_VarId);
		void AddVariableInfo(const wxString &a_thisSymbol, const wxString &a_thisValue, int a_valType, int a_VarId);
		void EndGlobals();

		void AddMessageToQueue(const wxString &_str);

		wxScintilla* GetCodeView() { return m_CodeView; }
};

class ThreadListCtrl : public wxListCtrl
{
public:
	ThreadListCtrl(wxWindow *parent,
		const wxWindowID id,
		const wxPoint& pos,
		const wxSize& size,
		long style)
		: wxListCtrl(parent, id, pos, size, style)
	{
	}

	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemColumnImage(long item, long column) const;
	//virtual wxListItemAttr *OnGetItemAttr(long item) const;
};


class GMDebuggerThread : public wxThread
{
public:
	GMDebuggerThread();

	// thread execution starts here
	virtual void *Entry();

	// called when the thread exits - whether it terminates normally or is
	// stopped with Delete() (but not when it is Kill()ed!)
	virtual void OnExit();

private:
	
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
