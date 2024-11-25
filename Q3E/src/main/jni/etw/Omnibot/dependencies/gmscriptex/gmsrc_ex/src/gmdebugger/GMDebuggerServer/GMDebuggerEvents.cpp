
#include "gmMachine.h"
#include "gmThread.h"

#include "GMDebuggerApp.h"

extern wxSocketServer	*g_SocketServer;
extern wxSocketBase		*g_DebuggerSocket;
extern wxCriticalSection g_SocketCritSect;

bool		g_RunningThreads = true;
extern CommandQueue			m_ExecQueue;
extern wxCriticalSection	m_ExecStringCritSection;

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(GMDebuggerFrame, wxFrame)
	EVT_MENU(ID_FILEOPEN,	GMDebuggerFrame::OnFileOpen)
	EVT_MENU(ID_FILESAVE,	GMDebuggerFrame::OnFileSave)
	EVT_MENU(ID_FILESAVEAS,	GMDebuggerFrame::OnFileSaveAs)
	EVT_MENU(ID_CLOSE,		GMDebuggerFrame::OnClose)
	
	EVT_MENU(ID_ABOUT,		GMDebuggerFrame::OnAbout)

	EVT_SOCKET(SOCKET_SERVER,	GMDebuggerFrame::OnSocketEventServer)
	EVT_SOCKET(SOCKET_DEBUGGER,	GMDebuggerFrame::OnSocketEventDebugger)
	
	EVT_MENU(ID_RUNSCRIPT,	GMDebuggerFrame::OnRunScript)
	EVT_MENU(ID_STOPSCRIPT,	GMDebuggerFrame::OnStopScript)

	EVT_IDLE(GMDebuggerFrame::OnIdle)
	//EVT_CLOSE(GMDebuggerFrame::OnCloseApp)

	EVT_TEXT_ENTER( ID_DEBUGTEXTIN, GMDebuggerFrame::OnDebugTextInput )
END_EVENT_TABLE()

BEGIN_EVENT_TABLE( AboutDialog, wxDialog )
	EVT_BUTTON( wxID_ANY, AboutDialog::OnAboutOk )
END_EVENT_TABLE()

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

//////////////////////////////////////////////////////////////////////////

void GMDebuggerFrame::OnCloseApp(wxCloseEvent& event)
{

}

void GMDebuggerFrame::OnIdle(wxIdleEvent& event)
{
	m_CodeView->SetReadOnly(g_RunningThreads);
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

void GMDebuggerFrame::OnRunScript(wxCommandEvent& event)
{
	wxString s = m_CodeView->GetText();
	m_ExecStringCritSection.Enter();
	m_ExecQueue.push_back(s);
	m_ExecStringCritSection.Leave();
	LogText("Running Script");
}

void GMDebuggerFrame::OnStopScript(wxCommandEvent& event)
{
	m_NetThread->KillAllThreads();
}

void GMDebuggerFrame::OnAbout(wxCommandEvent& event)
{
	m_AboutDlg->Show();
}

void GMDebuggerFrame::OnSocketEventServer(wxSocketEvent& event)
{
	switch(event.GetSocketEvent())
	{
	case wxSOCKET_INPUT:
		break;
	case wxSOCKET_LOST:
		m_NetThread->DebuggerDisConnected();
		m_DebuggerStatusBar->SetStatusText(_T("Debugger Disconnected."), 0);
		g_DebuggerSocket = 0;
		break;
	case wxSOCKET_CONNECTION:
		if(!g_DebuggerSocket)
		{
			m_NetThread->DebuggerConnected();
			m_DebuggerStatusBar->SetStatusText(_T("Debugger Connected."), 0);
			g_DebuggerSocket = g_SocketServer->Accept();
			if(g_DebuggerSocket)
				g_DebuggerSocket->SetEventHandler(*this, SOCKET_DEBUGGER);
		}		
		break;
	default:
		break;
	}
}

void GMDebuggerFrame::OnSocketEventDebugger(wxSocketEvent& event)
{
	switch(event.GetSocketEvent())
	{
	case wxSOCKET_INPUT:
		break;
	case wxSOCKET_LOST:
		m_NetThread->DebuggerDisConnected();
		m_DebuggerStatusBar->SetStatusText(_T("Debugger Disconnected."), 0);
		g_DebuggerSocket = 0;
		break;
	case wxSOCKET_CONNECTION: 
		m_NetThread->DebuggerConnected();
		if(!g_DebuggerSocket)
		{
			m_DebuggerStatusBar->SetStatusText(_T("Debugger Connected."), 0);
			g_DebuggerSocket = g_SocketServer->Accept();
			if(g_DebuggerSocket)
				g_DebuggerSocket->SetEventHandler(*this, SOCKET_DEBUGGER);
		}		
		break;
	default:
		break;
	}
}

void GMDebuggerFrame::OnDebugTextInput( wxCommandEvent& event )
{
	wxString s = m_DebugTextInput->GetValue();
	m_DebugTextInput->Clear();

	m_ExecStringCritSection.Enter();
	m_ExecQueue.push_back(s);
	m_ExecStringCritSection.Leave();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void AboutDialog::OnAboutOk( wxCommandEvent& event )
{
	this->Hide();
}

//////////////////////////////////////////////////////////////////////////

void ScriptSysCallback_Print(gmMachine* a_machine, const char* a_string)
{
	if(a_string)
	{
		GMDebuggerFrame::m_Instance->LogText(a_string);
	}	
}

bool ScriptSysCallback_Machine(gmMachine* a_machine, gmMachineCommand a_command, const void* a_context)
{
	bool bCallFinalFunction = false;
	const gmThread *pThread = static_cast<const gmThread*>(a_context);

	switch(a_command)
	{
	case MC_THREAD_EXCEPTION:
		{
			bool bFirst = true;
			const char *pMessage = 0;
			while((pMessage = a_machine->GetLog().GetEntry(bFirst)))
			{
				GMDebuggerFrame::m_Instance->LogError(pMessage);
			}
			a_machine->GetLog().Reset();
			break;
		}
	case MC_THREAD_CREATE: 
		{
			GMDebuggerFrame::m_Instance->LogText(wxString::Format("Thread Created: %d%s", pThread->GetId(), "\r\n"));
			break;
		}
	case MC_THREAD_DESTROY: 		
		{
			GMDebuggerFrame::m_Instance->LogText(wxString::Format("Thread Destroyed: %d%s", pThread->GetId(), "\r\n"));
			break;
		}
	default:
		break;
	}
	return false;
}
