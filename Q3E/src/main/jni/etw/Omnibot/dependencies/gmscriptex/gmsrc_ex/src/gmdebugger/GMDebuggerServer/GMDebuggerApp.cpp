///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 16 2006)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#include "timer.h"
#include "gmMachine.h"
#include "gmThread.h"
#include "gmMathLib.h"
#include "gmStringLib.h"
#include "gmArrayLib.h"
#include "gmSystemLib.h"
#include "gmVector3Lib.h"
#include "gmDebug.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP

#include "GMDebuggerApp.h"

void gmeInit(gmMachine *a_machine);
void ScriptSysCallback_Print(gmMachine* a_machine, const char* a_string);
bool ScriptSysCallback_Machine(gmMachine* a_machine, gmMachineCommand a_command, const void* a_context);

CommandQueue		m_ExecQueue;
wxCriticalSection	m_ExecStringCritSection;

gmDebugSession		g_DebugSession;

struct GM_PacketInfo
{
	int		m_PacketId;
	int		m_PacketLength;
};

///////////////////////////////////////////////////////////////////////////

GMDebuggerFrame *GMDebuggerFrame::m_Instance = 0;
wxSocketServer		*g_SocketServer = 0;
wxSocketBase		*g_DebuggerSocket = 0;
wxCriticalSection	g_SocketCritSect;

GMDebuggerFrame::GMDebuggerFrame( wxWindow* parent, int id, wxString title, wxPoint pos, wxSize size, int style ) 
	: wxFrame( parent, id, title, pos, size, style )
{
	DebuggerMenuBar = new wxMenuBar( 0 );
	wxMenu* m_FileMenu;
	m_FileMenu = new wxMenu();
	wxMenuItem* FileOpen = new wxMenuItem( m_FileMenu, ID_FILEOPEN, wxString( wxT("Open") ) , wxEmptyString, wxITEM_NORMAL );
	m_FileMenu->Append( FileOpen );
	wxMenuItem* FileSave = new wxMenuItem( m_FileMenu, ID_FILESAVE, wxString( wxT("Save") ) , wxEmptyString, wxITEM_NORMAL );
	m_FileMenu->Append( FileSave );
	wxMenuItem* FileSaveAs = new wxMenuItem( m_FileMenu, ID_FILESAVEAS, wxString( wxT("Save As") ) , wxEmptyString, wxITEM_NORMAL );
	m_FileMenu->Append( FileSaveAs );
	wxMenuItem* Close = new wxMenuItem( m_FileMenu, ID_CLOSE, wxString( wxT("Close") ) , wxEmptyString, wxITEM_NORMAL );
	m_FileMenu->Append( Close );
	DebuggerMenuBar->Append( m_FileMenu, wxT("File") );
	wxMenu* m_EditMenu;
	m_EditMenu = new wxMenu();
	DebuggerMenuBar->Append( m_EditMenu, wxT("Edit") );
	wxMenu* m_DebugMenu;
	m_DebugMenu = new wxMenu();
	wxMenuItem* RunScript = new wxMenuItem( m_DebugMenu, ID_RUNSCRIPT, wxString( wxT("Run Script") ) , wxEmptyString, wxITEM_NORMAL );
	m_DebugMenu->Append( RunScript );
	wxMenuItem* StopScript = new wxMenuItem( m_DebugMenu, ID_STOPSCRIPT, wxString( wxT("Stop Script") ) , wxEmptyString, wxITEM_NORMAL );
	m_DebugMenu->Append( StopScript );
	DebuggerMenuBar->Append( m_DebugMenu, wxT("Debug") );
	wxMenu* m_HelpMenu;
	m_HelpMenu = new wxMenu();
	DebuggerMenuBar->Append( m_HelpMenu, wxT("Help") );
	m_HelpMenu->Append(ID_ABOUT, "About");
	this->SetMenuBar( DebuggerMenuBar );

	m_DebuggerStatusBar = this->CreateStatusBar( 3, wxST_SIZEGRIP, ID_DEBUGGERSTATUSBAR );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_CodeView = new wxScintilla( this, ID_CODEVIEW, wxDefaultPosition, wxSize( -1,-1 ), 0, wxEmptyString );
	m_CodeView->SetUseTabs( true );
	m_CodeView->SetTabWidth( 4 );
	m_CodeView->SetIndent( 4 );
	m_CodeView->SetTabIndents( false );
	m_CodeView->SetBackSpaceUnIndents( true );
	m_CodeView->SetViewEOL( false );
	m_CodeView->SetViewWhiteSpace( false );
	m_CodeView->SetMarginWidth( 2, 0 );
	m_CodeView->SetIndentationGuides( true );

	m_CodeView->SetMarginType( 2, wxSCI_MARGIN_SYMBOL );
	m_CodeView->SetMarginMask( 2, ~wxSCI_MASK_FOLDERS );
	m_CodeView->SetMarginWidth( 2, 16 );
	m_CodeView->SetMarginSensitive( 2, true );

	m_CodeView->SetMarginType( 1, wxSCI_MARGIN_SYMBOL );
	m_CodeView->SetMarginMask( 1, wxSCI_MASK_FOLDERS );
	m_CodeView->SetMarginWidth( 1, 16);
	m_CodeView->SetMarginSensitive( 1, true );

	m_CodeView->SetProperty( wxT("fold"), wxT("1") );
	m_CodeView->SetFoldFlags( wxSCI_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSCI_FOLDFLAG_LINEAFTER_CONTRACTED );
	
	m_CodeView->SetMarginType( 0, wxSCI_MARGIN_NUMBER );
	m_CodeView->SetMarginWidth( 0, m_CodeView->TextWidth( wxSCI_STYLE_LINENUMBER, wxT("9999") ) );
	
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDER, wxSCI_MARK_BOXPLUS );
	m_CodeView->MarkerSetBackground( wxSCI_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_CodeView->MarkerSetForeground( wxSCI_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDEROPEN, wxSCI_MARK_BOXMINUS );
	m_CodeView->MarkerSetBackground( wxSCI_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_CodeView->MarkerSetForeground( wxSCI_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDERSUB, wxSCI_MARK_EMPTY );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDEREND, wxSCI_MARK_BOXPLUS );
	m_CodeView->MarkerSetBackground( wxSCI_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_CodeView->MarkerSetForeground( wxSCI_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDEROPENMID, wxSCI_MARK_BOXMINUS );
	m_CodeView->MarkerSetBackground( wxSCI_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_CodeView->MarkerSetForeground( wxSCI_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDERMIDTAIL, wxSCI_MARK_EMPTY );
	m_CodeView->MarkerDefine( wxSCI_MARKNUM_FOLDERTAIL, wxSCI_MARK_EMPTY );

	/*m_CodeView->MarkerDefine( MarkerArrow, wxSCI_MARK_ARROW );
	m_CodeView->MarkerSetBackground( MarkerArrow, wxColour( wxT("BLACK") ) );
	m_CodeView->MarkerSetForeground( MarkerArrow, wxColour( wxT("BLACK") ) );

	m_CodeView->MarkerDefine( MarkerBreakPoint, wxSCI_MARK_CIRCLE );
	m_CodeView->MarkerSetBackground( MarkerBreakPoint, wxColour( wxT("RED") ) );
	m_CodeView->MarkerSetForeground( MarkerBreakPoint, wxColour( wxT("RED") ) );*/

	m_CodeView->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_CodeView->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );

	m_CodeView->SetMinSize( wxSize( 450,450 ) );
	
	//////////////////////////////////////////////////////////////////////////
	//m_CodeView->StyleClearAll();
	m_CodeView->SetLexer(wxSCI_LEX_CPP);
	m_CodeView->StyleSetForeground(wxSCI_C_COMMENT, wxColour(0, 150, 0));
	m_CodeView->StyleSetForeground(wxSCI_C_COMMENTLINE, wxColour(0, 150, 0));
	m_CodeView->StyleSetForeground(wxSCI_C_COMMENTDOC, wxColour(0, 150, 0));
	m_CodeView->StyleSetForeground(wxSCI_C_NUMBER, wxColour(150, 0, 150));
	m_CodeView->StyleSetForeground(wxSCI_C_PREPROCESSOR, wxColour(0, 150, 150));
	m_CodeView->StyleSetForeground(wxSCI_C_STRING, wxColour(150, 0, 150));
	m_CodeView->StyleSetForeground(wxSCI_C_STRINGEOL, wxColour(150, 0, 150));
	m_CodeView->StyleSetForeground(wxSCI_C_CHARACTER, wxColour(150, 0, 150));
	m_CodeView->StyleSetForeground(wxSCI_C_WORD, wxColour(0, 0, 255));
	m_CodeView->StyleSetBold(wxSCI_C_OPERATOR, true);
	
	wxString kw0 = "if else for foreach in and or while dowhile function return"
		" continue break null global local member table true false this";
	wxString kw1 = "debug typeId typeName typeRegisterOperator typeRegisterVariable"
		"sysCollectGarbage sysGetMemoryUsage sysSetDesiredMemoryUsageHard"
		"sysSetDesiredMemoryUsageSoft sysSetDesiredMemoryUsageAuto sysGetDesiredMemoryUsageHard"
		"sysGetDesiredMemoryUsageSoft sysTime doString globals threadTime"
		"threadId threadAllIds threadKill threadKillAll thread yield exit"
		"assert sleep signal block stateSet stateSetOnThread stateGet"
		"stateGetLast stateSetExitFunction tableCount tableDuplicate"
		"print format";
	m_CodeView->SetKeyWords(0, kw0);
	m_CodeView->SetKeyWords(1, kw1);
	//////////////////////////////////////////////////////////////////////////
	
	bSizer1->Add( m_CodeView, 0, wxALIGN_LEFT|wxALIGN_TOP|wxALL|wxEXPAND, 0 );

	m_DebugTextOut = new wxTextCtrl( this, ID_DEBUGTEXTOUT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH|wxTE_RICH2 );
	bSizer1->Add( m_DebugTextOut, 2, wxALIGN_LEFT|wxBOTTOM|wxEXPAND, 0 );

	m_DebugTextInput = new wxTextCtrl( this, ID_DEBUGTEXTIN, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_PROCESS_ENTER|wxTE_RICH|wxTE_RICH2 );
	bSizer1->Add( m_DebugTextInput, 1, wxALL|wxEXPAND, 0 );
	
	this->SetSizer( bSizer1 );
	this->Layout();

	m_DebugTextInput->Disable();

	m_AboutDlg = new AboutDialog(this);
	m_AboutDlg->Hide();

	m_Instance = this;
	m_DebuggerStatusBar->SetStatusText(_T("Idle."), 0);

	wxIPV4address addr;
	addr.Hostname("127.0.0.1");
	addr.Service(49001);
	g_SocketServer = new wxSocketServer(addr);
	g_SocketServer->SetFlags(wxSOCKET_NOWAIT);
	g_SocketServer->SetEventHandler(*this, SOCKET_SERVER);
	g_SocketServer->SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_LOST_FLAG);
	g_SocketServer->Notify(true);
	if(g_SocketServer->IsOk())
		m_DebuggerStatusBar->SetStatusText(_T("Listening for connections."), 0);
	else
		m_DebuggerStatusBar->SetStatusText(_T("Error initializing socket."), 0);

	m_NetThread = new GMDebuggerThread;

	if(m_NetThread->Create() != wxTHREAD_NO_ERROR ||
		m_NetThread->Run() != wxTHREAD_NO_ERROR)
	{
		LogError(wxT("Can't create network thread!\n"));
	}
}

void GMDebuggerFrame::AddMessageToQueue(const wxString &_str)
{
	m_QueueCritSection.Enter();
	m_CommandQueue.push_back(_str);
	m_QueueCritSection.Leave();
}

//////////////////////////////////////////////////////////////////////////

extern bool		g_RunningThreads;
CommandQueue	g_CommandQueue;
wxString		g_LastCommand;
wxString		g_RecieveBuffer;
void ClearConsole();

void PollDebugSocket()
{
	if(g_DebuggerSocket && g_DebuggerSocket->IsConnected())
	{
		const int iBufferSize = 8192;
		char buffer[iBufferSize];

		g_DebuggerSocket->Read(buffer, iBufferSize);
		if(g_DebuggerSocket->LastCount() <= 0)
			return;

		OutputDebugStr(wxString::Format("Recieved %d bytes", g_DebuggerSocket->LastCount()));

		g_RecieveBuffer.append(buffer, g_DebuggerSocket->LastCount());

		if(!g_RecieveBuffer.empty())
		{
			const GM_PacketInfo *pPacket = (const GM_PacketInfo*)g_RecieveBuffer.c_str();
			if(pPacket->m_PacketId == 0xDEADB33F)
			{
				const unsigned int iTotalMessageSize = pPacket->m_PacketLength+sizeof(GM_PacketInfo);
				if(g_RecieveBuffer.size() >= iTotalMessageSize)
				{
					//Utils::OutputDebug(Utils::VA("%d processed\n", iTotalMessageSize));
					wxString str = g_RecieveBuffer.substr(sizeof(GM_PacketInfo), pPacket->m_PacketLength);
					g_RecieveBuffer.erase(g_RecieveBuffer.begin(), g_RecieveBuffer.begin()+iTotalMessageSize);
					g_CommandQueue.push_back(str);
				}
			}
			else
			{
				GMDebuggerFrame::m_Instance->LogError("Invalid Message!\n");
				//Utils::OutputDebug("Invalid Message!\n");
			}
		}
	}
}

void SendDebuggerMessage(gmDebugSession * a_session, const void * a_command, int a_len)
{
	if(g_DebuggerSocket && g_DebuggerSocket->IsConnected())
	{
		GM_PacketInfo pi;
		pi.m_PacketId = 0xDEADB33F;
		pi.m_PacketLength = a_len;
		g_DebuggerSocket->Write((const char *)&pi, sizeof(pi));
		g_DebuggerSocket->Write((const char *)a_command, a_len);
		OutputDebugStr(wxString::Format("Wrote %d bytes to socket", sizeof(pi) + a_len).c_str());
	}
}

const void *PumpDebuggerMessage(gmDebugSession * a_session, int &a_len)
{
	a_len = 0;
	if(!g_CommandQueue.empty())
	{
		g_LastCommand = g_CommandQueue.front();
		g_CommandQueue.pop_front();
		a_len = g_LastCommand.length();
		return g_LastCommand.c_str();
	}
	return 0;
}

bool gmThreadKillCallback(gmThread * a_thread, void *a_context)
{
	a_thread->GetMachine()->KillThread(a_thread->GetId());
	return true;
}

//////////////////////////////////////////////////////////////////////////

GMDebuggerThread::GMDebuggerThread() : m_Machine(0)
{
}

void GMDebuggerThread::KillAllThreads()
{
	m_MachineCritSection.Enter();
	m_Machine->ForEachThread(gmThreadKillCallback, 0);
	ClearConsole();
	m_MachineCritSection.Leave();
}

void GMDebuggerThread::DebuggerConnected()
{
	/*if(m_Machine)
	{
		g_DebugSession.m_sendMessage = SendDebuggerMessage;
		g_DebugSession.m_pumpMessage = PumpDebuggerMessage;
		g_DebugSession.Open(m_Machine);
	}*/	
}

void GMDebuggerThread::DebuggerDisConnected()
{
	g_DebugSession.Close();
}

void GMDebuggerThread::OnExit()
{
	
}

void *GMDebuggerThread::Entry()
{
	//////////////////////////////////////////////////////////////////////////
	// Initialize the scripting thread

	m_Machine = new gmMachine;
	m_Machine->SetDebugMode(true);
	gmMachine::s_machineCallback = ScriptSysCallback_Machine;
	gmMachine::s_printCallback = ScriptSysCallback_Print;
	
	//
	// bind the default libs
	//

	gmBindMathLib(m_Machine);
	gmBindStringLib(m_Machine);
	gmBindArrayLib(m_Machine);
	gmBindSystemLib(m_Machine);
	BindVector3Stack(m_Machine);

	gmBindDebugLib(m_Machine);
	gmeInit(m_Machine);

	g_DebugSession.m_sendMessage = SendDebuggerMessage;
	g_DebugSession.m_pumpMessage = PumpDebuggerMessage;
	g_DebugSession.Open(m_Machine);

	//////////////////////////////////////////////////////////////////////////
	gmTimer t;
	while(!TestDestroy())
	{
		//////////////////////////////////////////////////////////////////////////		
		// Run any waiting strings
		wxString exec;
		m_ExecStringCritSection.Enter();
		if(!m_ExecQueue.empty())
		{
			exec = m_ExecQueue.front();
			m_ExecQueue.pop_front();
		}		
		m_ExecStringCritSection.Leave();

		m_MachineCritSection.Enter();
		if(!exec.IsEmpty())
			m_Machine->ExecuteString(exec.c_str());		
		//////////////////////////////////////////////////////////////////////////
		// Step the script machine
		double dt = t.GetElapsedSeconds() * 1000.0;
		m_Machine->Execute((int)(dt));
		t.Reset();
		g_RunningThreads = m_Machine->GetNumThreads() > 0;
		//////////////////////////////////////////////////////////////////////////
		// Poll Debug Socket
		m_MachineCritSection.Leave();
		
		PollDebugSocket();
		g_DebugSession.Update();

		wxThread::Sleep(10);
	}

	delete m_Machine;
	m_Machine = 0;

	return NULL;
}

AboutDialog::AboutDialog( wxWindow* parent, int id, wxString title, wxPoint pos, wxSize size, int style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_textCtrl3 = new wxTextCtrl( this, wxID_ANY, wxT("Game Monkey Script Remote Debugger\nby Jeremy Swigart\n\nQuestions/Comments/Bugs, report to drevil@omni-bot.com\n\nhttp://www.omni-bot.com\n"), wxDefaultPosition, wxDefaultSize, wxTE_AUTO_URL|wxTE_CENTRE|wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH|wxTE_RICH2 );
	bSizer6->Add( m_textCtrl3, 2, wxALIGN_CENTER|wxALL|wxEXPAND, 5 );

	m_AboutBtnOk = new wxButton( this, wxID_ANY, wxT("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_AboutBtnOk, 0, wxALIGN_CENTER|wxALL, 5 );

	this->SetSizer( bSizer6 );
	this->Layout();
}
