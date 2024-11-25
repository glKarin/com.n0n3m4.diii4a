///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug 16 2006)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP

#include "GMDebuggerApp.h"
#include "gmDebugger.h"

#include "WxToolButton1_XPM.xpm"
#include "WxToolButton2_XPM.xpm"
#include "WxToolButton3_XPM.xpm"
#include "WxToolButton4_XPM.xpm"
#include "WxToolButton5_XPM.xpm"
#include "WxToolButton6_XPM.xpm"
#include "WxToolButton7_XPM.xpm"
#include "WxToolButton8_XPM.xpm"
#include "WxToolButton9_XPM.xpm"

///////////////////////////////////////////////////////////////////////////

GMDebuggerFrame *GMDebuggerFrame::m_Instance = 0;
wxSocketClient		*g_socket = 0;
wxCriticalSection	g_SocketCritSect;

GMDebuggerFrame::GMDebuggerFrame( wxWindow* parent, int id, wxString title, wxPoint pos, wxSize size, int style ) 
	: wxFrame( parent, id, title, pos, size, style )
	, m_ActiveInfoMap(m_GlobalVariableInfoMap)
{
	DebuggerMenuBar = new wxMenuBar( 0 );
	wxMenu* m_FileMenu;
	m_FileMenu = new wxMenu();
	wxMenuItem* FileOpen = new wxMenuItem( m_FileMenu, ID_FILEOPEN, wxString( wxT("Open") ) , wxT(""), wxITEM_NORMAL );
	m_FileMenu->Append( FileOpen );
	wxMenuItem* FileSave = new wxMenuItem( m_FileMenu, ID_FILESAVE, wxString( wxT("Save") ) , wxT(""), wxITEM_NORMAL );
	m_FileMenu->Append( FileSave );
	wxMenuItem* FileSaveAs = new wxMenuItem( m_FileMenu, ID_FILESAVEAS, wxString( wxT("Save As") ) , wxT(""), wxITEM_NORMAL );
	m_FileMenu->Append( FileSaveAs );
	wxMenuItem* Connect = new wxMenuItem( m_FileMenu, ID_CONNECT, wxString( wxT("Connect") ) , wxT(""), wxITEM_NORMAL );
	m_FileMenu->Append( Connect );
	wxMenuItem* Close = new wxMenuItem( m_FileMenu, ID_CLOSE, wxString( wxT("Close") ) , wxT(""), wxITEM_NORMAL );
	m_FileMenu->Append( Close );
	DebuggerMenuBar->Append( m_FileMenu, wxT("File") );
	wxMenu* m_EditMenu;
	m_EditMenu = new wxMenu();
	DebuggerMenuBar->Append( m_EditMenu, wxT("Edit") );
	wxMenu* m_DebugMenu;
	m_DebugMenu = new wxMenu();
	DebuggerMenuBar->Append( m_DebugMenu, wxT("Debug") );
	wxMenu* m_HelpMenu;
	m_HelpMenu = new wxMenu();
	DebuggerMenuBar->Append( m_HelpMenu, wxT("Help") );
	m_HelpMenu->Append(ID_ABOUT, "About");
	this->SetMenuBar( DebuggerMenuBar );

	m_DebuggerStatusBar = this->CreateStatusBar( 3, wxST_SIZEGRIP, ID_DEBUGGERSTATUSBAR );
	m_DebuggerToolbar = this->CreateToolBar( wxTB_FLAT|wxTB_HORIZONTAL, ID_DEBUGGERTOOLBAR ); 
	m_DebuggerToolbar->AddTool( ID_STEPINTO, wxT("tool"), wxBitmap( WxToolButton1_XPM ), wxNullBitmap, wxITEM_NORMAL, wxT(""), wxT("") );
	m_DebuggerToolbar->AddTool( ID_STEPOVER, wxT("tool"), wxBitmap( WxToolButton2_XPM ), wxNullBitmap, wxITEM_NORMAL, wxT(""), wxT("") );
	m_DebuggerToolbar->AddTool( ID_STEPOUT, wxT("tool"), wxBitmap( WxToolButton3_XPM ), wxNullBitmap, wxITEM_NORMAL, wxT(""), wxT("") );
	m_DebuggerToolbar->AddTool( ID_RESUMECURRENT, wxT("tool"), wxBitmap( WxToolButton4_XPM ), wxNullBitmap, wxITEM_NORMAL, wxT(""), wxT("") );
	m_DebuggerToolbar->AddTool( ID_STOPDEBUGGING, wxT("tool"), wxBitmap( WxToolButton5_XPM ), wxNullBitmap, wxITEM_NORMAL, wxT(""), wxT("") );
	m_DebuggerToolbar->AddTool( ID_TOGGLEBREAKPOINT, wxT("tool"), wxBitmap( WxToolButton6_XPM ), wxNullBitmap, wxITEM_NORMAL, wxT(""), wxT("") );
	m_DebuggerToolbar->AddTool( ID_BREAKALL, wxT("tool"), wxBitmap( WxToolButton7_XPM ), wxNullBitmap, wxITEM_NORMAL, wxT(""), wxT("") );
	m_DebuggerToolbar->AddTool( ID_RESUMEALL, wxT("tool"), wxBitmap( WxToolButton8_XPM ), wxNullBitmap, wxITEM_NORMAL, wxT(""), wxT("") );
	m_DebuggerToolbar->AddTool( ID_BREAKCURRENT, wxT("tool"), wxBitmap( WxToolButton9_XPM ), wxNullBitmap, wxITEM_NORMAL, wxT(""), wxT("") );
	m_DebuggerToolbar->Realize();

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	bSizer4->SetMinSize(wxSize( 425,500 )); 
	m_CodeView = new wxScintilla( this, ID_CODEVIEW, wxDefaultPosition, wxSize( -1,-1 ), 0, wxEmptyString );
	m_CodeView->SetUseTabs( true );
	m_CodeView->SetTabWidth( 4 );
	m_CodeView->SetIndent( 4 );
	m_CodeView->SetTabIndents( false );
	m_CodeView->SetBackSpaceUnIndents( true );
	m_CodeView->SetViewEOL( false );
	m_CodeView->SetViewWhiteSpace( false );
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

	m_CodeView->MarkerDefine( MarkerArrow, wxSCI_MARK_ARROW );
	m_CodeView->MarkerSetBackground( MarkerArrow, wxColour( wxT("BLACK") ) );
	m_CodeView->MarkerSetForeground( MarkerArrow, wxColour( wxT("BLACK") ) );

	m_CodeView->MarkerDefine( MarkerBreakPoint, wxSCI_MARK_CIRCLE );
	m_CodeView->MarkerSetBackground( MarkerBreakPoint, wxColour( wxT("RED") ) );
	m_CodeView->MarkerSetForeground( MarkerBreakPoint, wxColour( wxT("RED") ) );

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
	
	bSizer4->Add( m_CodeView, 0, wxALIGN_LEFT|wxALIGN_TOP|wxALL|wxEXPAND, 0 );

	m_DebugTextOut = new wxTextCtrl( this, ID_DEBUGTEXTOUT, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH|wxTE_RICH2 );
	bSizer4->Add( m_DebugTextOut, 2, wxALIGN_LEFT|wxBOTTOM|wxEXPAND, 0 );

	m_DebugTextInput = new wxTextCtrl( this, ID_DEBUGTEXTIN, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_RICH|wxTE_RICH2 );
	bSizer4->Add( m_DebugTextInput, 1, wxALL|wxEXPAND, 0 );
	
	bSizer1->Add( bSizer4, 0, wxEXPAND, 0 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_VarNotebook = new wxNotebook( this, ID_DEFAULT, wxDefaultPosition, wxDefaultSize, wxNB_FLAT );
	m_VarNotebook->SetMinSize( wxSize( -1,270 ) );

	m_GlobalsPanelWindow = new wxScrolledWindow( m_VarNotebook, ID_GLOBALS, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_GlobalsPanelWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );
	
	m_GlobalsPropGrid = new wxPropertyGrid(m_GlobalsPanelWindow, ID_GLOBALSSVARIABLELIST, wxDefaultPosition, wxDefaultSize, wxPG_AUTO_SORT|wxPG_DEFAULT_STYLE|wxPG_SPLITTER_AUTO_CENTER|wxPG_STATIC_SPLITTER|wxPG_TOOLTIPS);
	bSizer31->Add( m_GlobalsPropGrid, 1, wxEXPAND, 0 );
	
	m_GlobalsPanelWindow->SetSizer( bSizer31 );
	m_GlobalsPanelWindow->Layout();
	m_VarNotebook->AddPage( m_GlobalsPanelWindow, wxT("Globals"), true );
	m_AutoPanelWindow = new wxScrolledWindow( m_VarNotebook, ID_AUTOPANEL, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_AutoPanelWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_AutosPropGrid = new wxPropertyGrid(m_AutoPanelWindow, ID_AUTOSVARIABLELIST, wxDefaultPosition, wxDefaultSize, wxPG_AUTO_SORT|wxPG_DEFAULT_STYLE|wxPG_SPLITTER_AUTO_CENTER|wxPG_STATIC_SPLITTER|wxPG_TOOLTIPS);
	bSizer3->Add( m_AutosPropGrid, 1, wxEXPAND, 0 );

	m_AutoPanelWindow->SetSizer( bSizer3 );
	m_AutoPanelWindow->Layout();
	m_VarNotebook->AddPage( m_AutoPanelWindow, wxT("Auto"), false );
	bSizer2->Add( m_VarNotebook, 0, wxEXPAND|wxTOP, 0 );

	m_ThreadList = new ThreadListCtrl( this, ID_THREADLIST, wxDefaultPosition, wxSize( 225,260 ), wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VIRTUAL );

	m_ThreadList->InsertColumn(0, _T("Thread Id"));
	m_ThreadList->InsertColumn(1, _T("Status"));
	m_ThreadList->SetColumnWidth(0, 120);
	m_ThreadList->SetColumnWidth(1, 120);

	bSizer2->Add( m_ThreadList, 0, wxEXPAND, 0 );

	m_Callstack = new wxListCtrl( this, ID_CALLSTACK, wxDefaultPosition, wxSize( 225,100 ), wxLC_REPORT|wxLC_SINGLE_SEL );
	m_Callstack->InsertColumn(0, _T("Callstack"));
	m_Callstack->SetColumnWidth(0, 220);
	bSizer2->Add( m_Callstack, 0, wxEXPAND, 0 );

	m_panel3 = new wxPanel( this, ID_DEFAULT, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	bSizer2->Add( m_panel3, 1, wxEXPAND, 0 );

	bSizer1->Add( bSizer2, 1, wxEXPAND, 0 );

	this->SetSizer( bSizer1 );
	this->Layout();

	m_DebugTextInput->Disable();

	m_AboutDlg = new AboutDialog(this);
	m_AboutDlg->Hide();

	//////////////////////////////////////////////////////////////////////////

	m_Instance = this;

	//////////////////////////////////////////////////////////////////////////
	// Set up the gm debugger
	m_gmDebugger = new gmDebuggerSession;
	m_gmDebugger->m_pumpMessage = ServerPumpMessage;
	m_gmDebugger->m_sendMessage = ServerSendMessage;
	m_gmDebugger->m_user = this;

	m_sourceId = 0;
	m_currentDebugThread = 0;
	m_currentCallFrame = -1;
	m_lineNumberOnSourceRcv = -1;
	m_currentPos = -1;
	m_responseId = 0;

	m_DebuggerStatusBar->SetStatusText(_T("Idle."), 0);
	//////////////////////////////////////////////////////////////////////////

	g_socket = new wxSocketClient();
	g_socket->SetFlags(wxSOCKET_NOWAIT);
	g_socket->SetEventHandler(*this, SOCKET_ID);
	g_socket->SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_LOST_FLAG);
	g_socket->Notify(true);

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

GMDebuggerThread::GMDebuggerThread()
{
}

void GMDebuggerThread::OnExit()
{
}

void *GMDebuggerThread::Entry()
{
	int iMessageSize;

	wxString recieveBuffer;
	recieveBuffer.reserve(1048576);

	const int iMaxBufferSize = 1048576;
	char *buffer = new char[iMaxBufferSize];

	while(!TestDestroy())
	{
		iMessageSize = 0;

		g_SocketCritSect.Enter();
		if(g_socket->IsConnected())
		{
			g_socket->Read(buffer, iMaxBufferSize);
			iMessageSize = g_socket->LastCount();
		}
		g_SocketCritSect.Leave();

		// Add to the recieve buffer for processing.
		if(iMessageSize > 0)
		{
			OutputDebugStr(wxString::Format("%d recieved\n", iMessageSize));
			recieveBuffer.Append(buffer, iMessageSize);
		}
		
		if(!recieveBuffer.IsEmpty())
		{
			const GM_PacketInfo *pPacket = (const GM_PacketInfo*)recieveBuffer.GetData();
			if(pPacket->m_PacketId == 0xDEADB33F)
			{
				const unsigned int iTotalMessageSize = pPacket->m_PacketLength+sizeof(GM_PacketInfo);
				if(recieveBuffer.size() >= iTotalMessageSize)
				{
					OutputDebugStr(wxString::Format("%d processed\n", iTotalMessageSize));
					wxString str = recieveBuffer.substr(sizeof(GM_PacketInfo), pPacket->m_PacketLength);
					recieveBuffer.erase(recieveBuffer.begin(), recieveBuffer.begin()+iTotalMessageSize);
					GMDebuggerFrame::m_Instance->AddMessageToQueue(str);
				}
			}
			else
			{
				GMDebuggerFrame::m_Instance->LogError("Invalid Message!\n");
			}
		}
		wxThread::Sleep(10);
	}

	delete [] buffer;

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
