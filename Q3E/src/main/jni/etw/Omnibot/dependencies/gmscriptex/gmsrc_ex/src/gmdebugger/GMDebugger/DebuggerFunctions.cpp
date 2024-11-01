#include "gmDebugger.h"
#include "GMDebuggerApp.h"
#include <iostream>

void gmDebuggerBreak(gmDebuggerSession * a_session, int a_threadId, int a_sourceId, int a_lineNumber)
{
	gmMachineGetContext(a_session, a_threadId, 0);
}

void gmDebuggerRun(gmDebuggerSession * a_session, int a_threadId, int a_LineNo, const char *a_FuncName, const char *a_File)
{
	if(!a_File || !a_File[0])
		a_File = "<unknownfile>";
	if(!a_FuncName || !a_FuncName[0])
		a_FuncName = "<unknownfunc>";
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->LogText(wxString::Format("Thread %d started: %s::%s(%d).\r\n", 
		a_threadId, a_File, a_FuncName, a_LineNo));
	pApp->FindAddThread(a_threadId, 0, false);
}

void gmDebuggerStop(gmDebuggerSession * a_session, int a_threadId)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->LogText(wxString::Format("Thread %d stopped.\r\n", a_threadId));
	if(a_threadId == pApp->m_currentDebugThread) 
		pApp->ClearCurrentContext();
	pApp->RemoveThread(a_threadId);
}

void gmDebuggerSource(gmDebuggerSession * a_session, int a_sourceId, const char * a_sourceName, const char * a_source)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->SetSourceName(a_sourceName);
	pApp->SetSource(a_sourceId, a_source);
	
	if(pApp->m_lineNumberOnSourceRcv != -1)
	{
		pApp->SetLine(pApp->m_lineNumberOnSourceRcv);
		pApp->m_lineNumberOnSourceRcv = -1;
	}
}

void gmDebuggerException(gmDebuggerSession * a_session, int a_threadId)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	gmMachineGetContext(a_session, a_threadId, 0);
}

void gmDebuggerBeginContext(gmDebuggerSession * a_session, int a_threadId, int a_callFrame)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->m_currentDebugThread = a_threadId;
	pApp->FindAddThread(a_threadId, 4, true);
	pApp->ClearCallstack();
	pApp->BeginContext(0);
	pApp->m_currentCallFrame = a_callFrame;
}

void gmDebuggerContextCallFrame(gmDebuggerSession * a_session, int a_callFrame, const char * a_functionName, int a_sourceId, int a_lineNumber, const char * a_thisSymbol, const char * a_thisValue, int a_thisType, int a_thisId)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->AddToCallstack(a_callFrame, wxString::Format("%s (%d)", a_functionName, a_lineNumber));

	// Update the callstack.
	if(pApp->m_currentCallFrame == a_callFrame)
	{
		// add "this"
		pApp->ContextVariable(a_thisSymbol, a_thisValue, a_thisType, a_thisId);

		// do we have the source code?
		pApp->m_lineNumberOnSourceRcv = -1;
		if(!pApp->SetSource(a_sourceId, NULL))
		{
			// request source
			gmMachineGetSource(a_session, a_sourceId);
			pApp->m_lineNumberOnSourceRcv = a_lineNumber;
		}
		else
		{
			// update the position cursor.
			pApp->SetLine(a_lineNumber);
		}
	}
}

void gmDebuggerContextVariable(gmDebuggerSession * a_session, const char * a_varSymbol, const char * a_varValue, int a_varType, int a_varId)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->ContextVariable(a_varSymbol, a_varValue, a_varType, a_varId);
}

void gmDebuggerEndContext(gmDebuggerSession * a_session)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->EndContext();
}

void gmDebuggerBeginSourceInfo(gmDebuggerSession * a_session)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	// TODO:
}

void gmDebuggerSourceInfo(gmDebuggerSession * a_session, int a_sourceId, const char * a_sourceName)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	// TODO:
}

void gmDebuggerEndSourceInfo(gmDebuggerSession * a_session)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	// TODO:
}

void gmDebuggerBeginThreadInfo(gmDebuggerSession * a_session)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->StartUpdateThreadList();
}

void gmDebuggerThreadInfo(gmDebuggerSession * a_session, int a_threadId, int a_threadState)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->FindAddThread(a_threadId, a_threadState, false);
}

void gmDebuggerEndThreadInfo(gmDebuggerSession * a_session)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->EndUpdateThreadList();
}

void gmDebuggerError(gmDebuggerSession * a_session, const char * a_error)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->LogError(wxString::Format("%s%s", a_error, "\r\n"));
}

void gmDebuggerMessage(gmDebuggerSession * a_session, const char * a_message)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->LogText(wxString::Format("%s%s", a_message, "\r\n"));
}

void gmDebuggerAck(gmDebuggerSession * a_session, int a_response, int a_posNeg)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	if(a_response == pApp->m_responseId && a_posNeg)
	{
		if(pApp->m_BreakPoint.m_enabled)
		{
			pApp->GetCodeView()->MarkerAdd(pApp->m_BreakPoint.m_lineNumber-1, GMDebuggerFrame::MarkerArrow);
		}
		else
		{
			pApp->GetCodeView()->MarkerDelete(pApp->m_BreakPoint.m_lineNumber-1, GMDebuggerFrame::MarkerArrow);
		}
	}
}

void gmDebuggerQuit(gmDebuggerSession * a_session)
{
	//GMDebuggerNet::Disconnect();
}

//////////////////////////////////////////////////////////////////////////

void gmDebuggerBeginGlobals(gmDebuggerSession * a_session, int a_VarId)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->BeginGlobals(a_VarId);
}

void gmDebuggerGlobal(gmDebuggerSession * a_session, const char * a_varSymbol, const char * a_varValue, int a_varType, int a_VarId)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->AddVariableInfo(a_varSymbol, a_varValue, a_varType, a_VarId);
}

void gmDebuggerEndGlobals(gmDebuggerSession * a_session)
{
	GMDebuggerFrame *pApp = static_cast<GMDebuggerFrame*>(a_session->m_user);
	pApp->EndGlobals();
}
