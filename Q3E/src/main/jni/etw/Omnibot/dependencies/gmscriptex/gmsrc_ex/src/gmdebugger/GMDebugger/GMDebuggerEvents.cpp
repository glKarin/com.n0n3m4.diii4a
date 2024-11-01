#include "GMDebuggerApp.h"
#include "gmDebugger.h"

extern wxSocketClient *g_socket;
extern wxCriticalSection g_SocketCritSect;

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(GMDebuggerFrame, wxFrame)
	EVT_MENU(ID_FILEOPEN,	GMDebuggerFrame::OnFileOpen)
	EVT_MENU(ID_FILESAVE,	GMDebuggerFrame::OnFileSave)
	EVT_MENU(ID_FILESAVEAS,	GMDebuggerFrame::OnFileSaveAs)
	EVT_MENU(ID_CLOSE,		GMDebuggerFrame::OnClose)
	
	EVT_MENU(ID_CONNECT,	GMDebuggerFrame::OnConnect)
	EVT_MENU(ID_ABOUT,		GMDebuggerFrame::OnAbout)

	EVT_SOCKET(SOCKET_ID,	GMDebuggerFrame::OnSocketEvent)

	/*EVT_LIST_ITEM_DESELECTED(LIST_CTRL, MyListCtrl::OnDeselected)
	EVT_LIST_KEY_DOWN(LIST_CTRL, MyListCtrl::OnListKeyDown)
	EVT_LIST_ITEM_ACTIVATED(LIST_CTRL, MyListCtrl::OnActivated)
	EVT_LIST_ITEM_FOCUSED(LIST_CTRL, MyListCtrl::OnFocused)*/

	EVT_MENU(ID_STEPINTO, GMDebuggerFrame::OnStepInto)
	EVT_MENU(ID_STEPOUT, GMDebuggerFrame::OnStepOut)
	EVT_MENU(ID_STEPOVER, GMDebuggerFrame::OnStepOver)
	EVT_MENU(ID_RESUMECURRENT, GMDebuggerFrame::OnRunCurrentThread)
	EVT_MENU(ID_BREAKCURRENT, GMDebuggerFrame::OnBreakCurrentThread)
	EVT_MENU(ID_TOGGLEBREAKPOINT, GMDebuggerFrame::OnToggleBreakPoint)
	EVT_MENU(ID_BREAKALL, GMDebuggerFrame::OnBreakAllThreads)
	EVT_MENU(ID_RESUMEALL, GMDebuggerFrame::OnResumeAllThreads)

	EVT_LIST_ITEM_SELECTED(ID_THREADLIST, GMDebuggerFrame::OnThreadSelected)
	EVT_LIST_ITEM_SELECTED(ID_CALLSTACK, GMDebuggerFrame::OnCallStackSelected)

	EVT_PG_ITEM_EXPANDED(ID_GLOBALSSVARIABLELIST, GMDebuggerFrame::OnGlobalListExpand)

	EVT_SCI_MARGINCLICK(ID_CODEVIEW, GMDebuggerFrame::OnCodeMarginClicked)
	
	EVT_IDLE(GMDebuggerFrame::OnIdle)
	//EVT_CLOSE(GMDebuggerFrame::OnCloseApp)

	EVT_TEXT_ENTER( ID_DEBUGTEXTIN, GMDebuggerFrame::OnDebugTextInput )
END_EVENT_TABLE()

BEGIN_EVENT_TABLE( AboutDialog, wxDialog )
	EVT_BUTTON( wxID_ANY, AboutDialog::OnAboutOk )
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////

wxString ThreadListCtrl::OnGetItemText(long item, long column) const
{
	assert(item < (long)GMDebuggerFrame::m_Instance->m_Threads.size());
	switch(column)
	{
	case 0:
		{
			return wxString::Format("%d",	GMDebuggerFrame::m_Instance->m_Threads[item].m_ThreadId);
		}
	case 1:
		{
			const char *pStateStr = 0;
			switch(GMDebuggerFrame::m_Instance->m_Threads[item].m_ThreadState)
			{
			case 0: pStateStr = "Running"; break;
			case 1: pStateStr = "Sleeping"; break;
			case 2: pStateStr = "Blocked"; break;
			case 3: pStateStr = "Killed"; break;
			case 4: pStateStr = "Debug"; break;
			default : 
				return wxString("");
			}
			return wxString(pStateStr);
		}
	}
	return wxString("");
}

int ThreadListCtrl::OnGetItemColumnImage(long item, long column) const
{
	if (!column)
		return 0;

	if (!(item %3) && column == 1)
		return 0;

	return -1;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void GMDebuggerFrame::LogText(const wxString &aText)
{
	m_DebugTextOut->AppendText(aText);
	m_DebugTextOut->ScrollLines(m_DebugTextOut->GetNumberOfLines());
	m_DebugTextOut->SetModified(true);
	m_DebugTextOut->Refresh();
}

void GMDebuggerFrame::LogError(const wxString &aText)
{
	m_DebugTextOut->AppendText(aText);
	m_DebugTextOut->ScrollLines(m_DebugTextOut->GetNumberOfLines());
	m_DebugTextOut->SetModified(true);
	m_DebugTextOut->Refresh();
}

void GMDebuggerFrame::FindAddThread(int a_threadId, int a_state, bool a_select)
{
	if(a_threadId <= 0) 
		return;

	const char *pStateStr = 0;
	ThreadInfo::ThreadState st;

	switch(a_state)
	{
	case 0 : st = ThreadInfo::Running; break;
	case 1 : st = ThreadInfo::Sleeping; break;
	case 2 : st = ThreadInfo::Blocked; break;
	case 3 : st = ThreadInfo::Killed; break;
	case 4 : st = ThreadInfo::Exception; break;
	default : 
		break;
	}
	
	for(unsigned int i = 0; i < m_Threads.size(); ++i)
	{
		if(m_Threads[i].m_ThreadId == a_threadId)
		{
			m_Threads[i].m_ThreadState = st;
			m_ThreadList->RefreshItem((long)i);
			return;
		}
	}

	ThreadInfo ti;
	ti.m_ThreadId = a_threadId;
	ti.m_ThreadState = st;
	m_Threads.push_back(ti);
	m_ThreadList->SetItemCount((long)m_Threads.size());
	m_ThreadList->RefreshItem((long)m_Threads.size()-1);

	SelectThread(a_threadId);
}

void GMDebuggerFrame::StartUpdateThreadList()
{	
	ClearThreads();
}

void GMDebuggerFrame::EndUpdateThreadList()
{
	m_ThreadList->Refresh();
}

void GMDebuggerFrame::RemoveThread(int a_threadId)
{
	long item = 0;
	ThreadList::iterator it = m_Threads.begin(), itEnd = m_Threads.end();
	for(; it != itEnd; ++it)
	{
		if((*it).m_ThreadId == a_threadId)
		{
			m_Threads.erase(it);
			m_ThreadList->SetItemCount((long)m_Threads.size());
			m_ThreadList->RefreshItems(item, (long)m_Threads.size()-1);
			return;
		}
		item++;
	}
}

void GMDebuggerFrame::SelectThread(int a_threadId)
{
	for(unsigned int i = 0; i < m_Threads.size(); ++i)
	{
		if(m_Threads[i].m_ThreadId == a_threadId)
		{
			long item = m_ThreadList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
			while( item != -1 )
			{
				m_ThreadList->SetItemState(item, ~wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

				item = m_ThreadList->GetNextItem(item, wxLIST_NEXT_ALL,
					wxLIST_STATE_SELECTED);
			}

			m_ThreadList->SetItemState(item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
		}
	}
}

void GMDebuggerFrame::ClearThreads()
{
	m_ThreadList->DeleteAllItems();
	m_Threads.clear();
}

void GMDebuggerFrame::ClearCurrentContext()
{	
	// clear selection
	m_currentDebugThread = 0;
	m_CodeView->ClearAll();
	m_Callstack->DeleteAllItems();
	m_AutosPropGrid->Clear();
	m_AutoVariableInfoMap.clear();
}

void GMDebuggerFrame::SetSourceName(const char * a_sourceName)
{
	m_DebuggerStatusBar->SetStatusText(a_sourceName, 1);
}

bool GMDebuggerFrame::SetSource(unsigned int a_sourceId, const char * a_source)
{
	if(a_sourceId == m_sourceId) 
		return true;

	// see if we already have this source.
	SourceInfo::iterator it = m_SourceInfo.find(a_sourceId);
	if(it != m_SourceInfo.end())
	{
		const wxString &st = (*it).second;
		m_CodeView->SetReadOnly(false);
		m_CodeView->SetText(st);
		m_CodeView->SetReadOnly(true);
		m_CodeView->Colourise(0, -1);
		m_sourceId = a_sourceId;
		return true;
	}
	
	// need to request it
	if(!a_source)
		return false;

	wxString st = a_source;
	m_SourceInfo.insert(std::make_pair(a_sourceId, st));
	m_CodeView->SetReadOnly(false);
	m_CodeView->SetText(st);
	m_CodeView->SetReadOnly(true);
	m_CodeView->Colourise(0, -1);
	m_sourceId = a_sourceId;

	return true;
}

void GMDebuggerFrame::SetLine(int a_line)
{
	if(m_currentPos >= 0)
	{
		m_CodeView->MarkerDelete(m_currentPos, MarkerArrow);	
	}

	m_currentPos = a_line-1;
	m_CodeView->MarkerAdd(m_currentPos, MarkerArrow);
	m_CodeView->ScrollToLine(a_line-1);
}

void GMDebuggerFrame::AddToCallstack(long a_item, const wxString &a_string)
{
	m_Callstack->InsertItem(a_item, a_string);
}

void GMDebuggerFrame::ClearCallstack()
{
	m_Callstack->DeleteAllItems();
}

void GMDebuggerFrame::BeginContext(int a_VarId)
{
	m_ActivePropertyGrid = m_AutosPropGrid;
	m_ActiveInfoMap = m_AutoVariableInfoMap;
	//////////////////////////////////////////////////////////////////////////
	ParentPropertyMap::iterator it = m_ActiveInfoMap.find(a_VarId);
	if(it != m_ActiveInfoMap.end())
		m_ParentId = it->second;
	else
		m_ParentId = wxPGId();

	if(m_ParentId.GetPropertyPtr())
	{
		wxPGPropertyWithChildren *pTmp = dynamic_cast<wxPGPropertyWithChildren*>(m_ParentId.GetPropertyPtr());
		if(pTmp)
			pTmp->Empty();	
	}

	if(a_VarId==0)
	{
		m_AutoVariableInfoMap.clear();
		m_ActivePropertyGrid->Clear();
		m_ParentId = wxPGId();
	}
	m_ActivePropertyGrid->Freeze();
	m_ActivePropertyGrid->Clear();
}

void GMDebuggerFrame::ContextVariable(const wxString &a_name, const wxString &a_value, int a_varType, int a_VarId)
{
	AddVariableInfo(a_name, a_value, a_varType, a_VarId);
}

void GMDebuggerFrame::EndContext()
{
	m_ActivePropertyGrid->Thaw();
}

void GMDebuggerFrame::BeginGlobals(int a_VarId)
{
	m_ActivePropertyGrid = m_GlobalsPropGrid;
	m_ActiveInfoMap = m_GlobalVariableInfoMap;
	//////////////////////////////////////////////////////////////////////////

	ParentPropertyMap::iterator it = m_ActiveInfoMap.find(a_VarId);
	if(it != m_ActiveInfoMap.end())
		m_ParentId = it->second;
	else
		m_ParentId = wxPGId();
	
	if(m_ParentId.GetPropertyPtr())
	{
		wxPGPropertyWithChildren *pTmp = dynamic_cast<wxPGPropertyWithChildren*>(m_ParentId.GetPropertyPtr());
		if(pTmp)
			pTmp->Empty();	
	}

	if(a_VarId==0)
	{
		m_GlobalVariableInfoMap.clear();
		m_ActivePropertyGrid->Clear();
		m_ParentId = wxPGId();
	}
	//m_ActivePropertyGrid->Freeze();
}

void GMDebuggerFrame::AddVariableInfo(const wxString &a_thisSymbol, const wxString &a_thisValue, int a_varType, int a_VarId)
{
	enum
	{
		GM_NULL = 0,
		GM_INT,
		GM_FLOAT,

		GM_VECTOR,
		GM_ENTITY,

		GM_STRING,
		GM_TABLE,
		GM_FUNCTION,
	};

	wxPGId id;
	switch(a_varType)
	{
	case GM_NULL:
	case GM_INT:
	case GM_FLOAT:
	case GM_VECTOR:
	case GM_ENTITY:
	case GM_STRING:
	case GM_FUNCTION:
		{
			if(m_ParentId.GetPropertyPtr())
				id = m_ActivePropertyGrid->AppendIn(m_ParentId, wxStringProperty(a_thisSymbol,a_thisSymbol,a_thisValue));
			else
				id = m_ActivePropertyGrid->Append(wxStringProperty(a_thisSymbol,a_thisSymbol,a_thisValue));
			m_ActivePropertyGrid->DisableProperty( id );
			break;
		}
	case GM_TABLE:
	default:
		{
			if(m_ParentId.GetPropertyPtr())
				id = m_ActivePropertyGrid->AppendIn(m_ParentId, wxParentProperty(a_thisSymbol,a_thisSymbol) );
			else
				id = m_ActivePropertyGrid->Append(wxParentProperty(a_thisSymbol,a_thisSymbol) );
			m_ActiveInfoMap[a_VarId] = id;
			m_ActivePropertyGrid->DisableProperty( id );

			if(a_VarId != 0)
				gmMachineGetGlobalsInfo(m_gmDebugger, a_VarId);
			break;
		}
	}
}

void GMDebuggerFrame::EndGlobals()
{
	//m_ActivePropertyGrid->Thaw();
}

//////////////////////////////////////////////////////////////////////////

void GMDebuggerFrame::OnCloseApp(wxCloseEvent& event)
{

}

void GMDebuggerFrame::OnIdle(wxIdleEvent& event)
{
	m_gmDebugger->Update();
}

void GMDebuggerFrame::OnClose(wxCommandEvent& event)
{
	if(m_NetThread && m_NetThread->IsRunning())
		m_NetThread->Delete();

	event;
	Close(true);
}

void GMDebuggerFrame::OnFileOpen(wxCommandEvent& event)
{
	wxFileDialog dlg(this, _T("Open a Game Monkey Script File"), 
		wxEmptyString, 
		wxEmptyString, 
		_T("Game Monkey Script Files (*.gm)|*.gm|Game Monkey Compiled Script Files (*.gmc)|*.gmc"));

	if (dlg.ShowModal() == wxID_OK && wxFileExists(dlg.GetPath()))
	{
		wxFile file(dlg.GetPath(), wxFile::read);
		if(file.IsOpened())
		{
			wxString contents;

			size_t numRead = 0;
			const int iBufferSize = 1024;
			char buffer[iBufferSize];			
			while(numRead = file.Read(buffer, iBufferSize))
				contents.Append(buffer, numRead);

			m_CodeView->SetText(contents);
		}		
	}
}

void GMDebuggerFrame::OnFileSave(wxCommandEvent& event)
{
}

void GMDebuggerFrame::OnFileSaveAs(wxCommandEvent& event)
{
}

void GMDebuggerFrame::OnAbout(wxCommandEvent& event)
{
	m_AboutDlg->Show();
}

void GMDebuggerFrame::OnConnect(wxCommandEvent& event)
{
	wxIPV4address addr;

	// Ask user for server address
	wxString hostname = wxGetTextFromUser(
		_("Enter the address of the Game Monkey Application:"),
		_("Connect ..."),
		_("localhost"));

	if(hostname.IsEmpty())
		return;

	addr.Hostname(hostname);
	addr.Service(49001);

	// Mini-tutorial for Connect() :-)
	// ---------------------------
	//
	// There are two ways to use Connect(): blocking and non-blocking,
	// depending on the value passed as the 'wait' (2nd) parameter.
	//
	// Connect(addr, true) will wait until the connection completes,
	// returning true on success and false on failure. This call blocks
	// the GUI (this might be changed in future releases to honour the
	// wxSOCKET_BLOCK flag).
	//
	// Connect(addr, false) will issue a nonblocking connection request
	// and return immediately. If the return value is true, then the
	// connection has been already successfully established. If it is
	// false, you must wait for the request to complete, either with
	// WaitOnConnect() or by watching wxSOCKET_CONNECTION / LOST
	// events (please read the documentation).
	//
	// WaitOnConnect() itself never blocks the GUI (this might change
	// in the future to honour the wxSOCKET_BLOCK flag). This call will
	// return false on timeout, or true if the connection request
	// completes, which in turn might mean:
	//
	//   a) That the connection was successfully established
	//   b) That the connection request failed (for example, because
	//      it was refused by the peer.
	//
	// Use IsConnected() to distinguish between these two.
	//
	// So, in a brief, you should do one of the following things:
	//
	// For blocking Connect:
	//
	//   bool success = client->Connect(addr, true);
	//
	// For nonblocking Connect:
	//
	//   client->Connect(addr, false);
	//
	//   bool waitmore = true;
	//   while (! client->WaitOnConnect(seconds, millis) && waitmore )
	//   {
	//     // possibly give some feedback to the user,
	//     // update waitmore if needed.
	//   }
	//   bool success = client->IsConnected();
	//
	// And that's all :-)

	LogText(_("Trying to connect (timeout = 10 sec) ...\n"));
	g_socket->Connect(addr, false);
	g_socket->WaitOnConnect(10);

	if (g_socket->IsConnected())
	{
		LogText(_("Succeeded ! Connection established\n"));
		m_CodeView->SetReadOnly(true);
		m_DebugTextInput->Enable();
	}
	else
	{
		g_socket->Close();
		LogText(_("Failed ! Unable to connect\n"));
		wxMessageBox(_("Can't connect to the specified host"), _("Alert !"));
	}
}

void GMDebuggerFrame::OnSocketEvent(wxSocketEvent& event)
{
	switch(event.GetSocketEvent())
	{
	case wxSOCKET_INPUT:
		break;
	case wxSOCKET_LOST:
		m_DebuggerStatusBar->SetStatusText(_T("Connection Lost..."), 0);
		break;
	case wxSOCKET_CONNECTION: 
		m_DebuggerStatusBar->SetStatusText(_T("Connected."), 0);
		gmMachineGetThreadInfo(m_gmDebugger);
		gmMachineGetGlobalsInfo(m_gmDebugger, 0);
		break;
	default:
		break;
	}
}

void GMDebuggerFrame::OnThreadSelected(wxListEvent& event)
{
	int iThreadId = m_Threads[event.m_itemIndex].m_ThreadId;
	gmMachineGetContext(m_gmDebugger, iThreadId, 0);
}

void GMDebuggerFrame::OnCallStackSelected(wxListEvent& event) 
{
	//if(CallStack->SelectedRows->Count > 0)
	//{
	//	DataGridViewRow ^row = CallStack->SelectedRows[0];

	//	// TODO: look for index of callstack selection
	//	int iSelected = -1;
	//	if(iSelected != -1 && m_currentDebugThread > 0)
	//	{
	//		gmMachineGetContext(m_session, m_currentDebugThread, iSelected);
	//	}
	//}
}

void GMDebuggerFrame::OnStepInto(wxCommandEvent& event) 
{
	if(m_currentDebugThread)
	{
		gmMachineStepInto(m_gmDebugger, m_currentDebugThread);
	}
}
void GMDebuggerFrame::OnStepOut(wxCommandEvent& event) 
{
	if(m_currentDebugThread)
	{
		gmMachineStepOut(m_gmDebugger, m_currentDebugThread);
	}
}
void GMDebuggerFrame::OnStepOver(wxCommandEvent& event) 
{
	if(m_currentDebugThread)
	{
		gmMachineStepOver(m_gmDebugger, m_currentDebugThread);
	}
}
void GMDebuggerFrame::OnRunCurrentThread(wxCommandEvent& event) 
{
	if(m_currentDebugThread)
	{
		gmMachineRun(m_gmDebugger, m_currentDebugThread);
	}
}
void GMDebuggerFrame::OnBreakCurrentThread(wxCommandEvent& event) 
{
	if(m_currentDebugThread != 0)
	{
		gmMachineBreak(m_gmDebugger, m_currentDebugThread);
	}
}
void GMDebuggerFrame::OnKillCurrentThread(wxCommandEvent& event)
{
	if(m_currentDebugThread != 0)
	{
		gmMachineKill(m_gmDebugger, m_currentDebugThread);
	}
}
void GMDebuggerFrame::OnToggleBreakPoint(wxCommandEvent& event) 
{
	// TODO:
}
void GMDebuggerFrame::OnBreakAllThreads(wxCommandEvent& event) 
{
	gmMachineBreakAll(m_gmDebugger);
}
void GMDebuggerFrame::OnKillAllThreads(wxCommandEvent& event) 
{
	gmMachineKillAll(m_gmDebugger);
}
void GMDebuggerFrame::OnResumeAllThreads(wxCommandEvent& event) 
{
	for(unsigned int i = 0; i < m_Threads.size(); ++i)
		gmMachineRun(m_gmDebugger, m_Threads[i].m_ThreadId);
	gmMachineGetThreadInfo(m_gmDebugger);
}
void GMDebuggerFrame::OnGlobalListExpand(wxPropertyGridEvent &event)
{
	ParentPropertyMap::iterator it = m_GlobalVariableInfoMap.begin(), itEnd = m_GlobalVariableInfoMap.end();
	for(;it != itEnd; ++it)
	{
		if(it->second == event.GetProperty())
		{
			gmMachineGetGlobalsInfo(m_gmDebugger, it->first);
			break;
		}
	}
}
void GMDebuggerFrame::OnCodeMarginClicked(wxScintillaEvent &event)
{
	if(event.GetMargin()==2)
	{
		int iLine = m_CodeView->LineFromPosition(event.GetPosition());

		bool enabled = true;
		if(m_CodeView->MarkerGet(iLine) & (1<<MarkerBreakPoint))
		{
			m_CodeView->MarkerDelete(iLine, MarkerBreakPoint);
			enabled = false;
		}
		else
		{
			enabled = true;
			m_CodeView->MarkerAdd(iLine, MarkerBreakPoint);
		}

		if(m_currentDebugThread)
		{
			m_BreakPoint.m_enabled = enabled;
			m_BreakPoint.m_allThreads = true;
			m_BreakPoint.m_responseId = ++m_responseId;
			m_BreakPoint.m_sourceId = m_sourceId;
			m_BreakPoint.m_lineNumber = iLine + 1;
			m_BreakPoint.m_threadId = 0;
			gmMachineSetBreakPoint(m_gmDebugger, m_responseId, m_sourceId, iLine + 1, 0, enabled ? 1 : 0);
		}
	}
}

void GMDebuggerFrame::OnDebugTextInput( wxCommandEvent& event )
{
	wxString strScript = m_DebugTextInput->GetValue();
	gmMachineRunScript(m_gmDebugger, strScript.c_str());
	m_DebugTextInput->Clear();
}

//////////////////////////////////////////////////////////////////////////

void GMDebuggerFrame::ServerSendMessage(gmDebuggerSession *a_session, const void *a_command, int a_len)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	g_SocketCritSect.Enter();
	if(g_socket->IsConnected())
	{
		GM_PacketInfo pi;
		pi.m_PacketId = 0xDEADB33F;
		pi.m_PacketLength = a_len;
		g_socket->Write(&pi, sizeof(pi));
		g_socket->Write(a_command, a_len);
	}
	g_SocketCritSect.Leave();
}

const void *GMDebuggerFrame::ServerPumpMessage(gmDebuggerSession *a_session, int &a_len)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	a_len = 0;

	if(!pApp->m_CommandQueue.empty())
	{
		pApp->m_LastCommand = pApp->m_CommandQueue.front();
		pApp->m_CommandQueue.pop_front();
		a_len = (int)pApp->m_LastCommand.Length();
		return pApp->m_LastCommand.ToAscii();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void AboutDialog::OnAboutOk( wxCommandEvent& event )
{
	this->Hide();
}
