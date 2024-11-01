/*
_____               __  ___          __            ____        _      __
/ ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
/ (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
\___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
/___/             /_/

See Copyright Notice in gmMachine.h

*/

#include <string.h>
#include "gmDebugger.h"

//
// Please note that gmDebugger.c/.h are for implementing
// a debugger application and should not be included
// in an normal GM application build.
//

#ifndef GM_MAKE_ID32
#define GM_MAKE_ID32( a, b, c, d )  ( ((d)<<24) | ((c)<<16) | ((b)<<8) | (a))
#endif //GM_MAKE_ID32

#define ID_mrun GM_MAKE_ID32('m','r','u','n')
#define ID_msin GM_MAKE_ID32('m','s','i','n')
#define ID_msou GM_MAKE_ID32('m','s','o','u')
#define ID_msov GM_MAKE_ID32('m','s','o','v')
#define ID_mgct GM_MAKE_ID32('m','g','c','t')
#define ID_mgsr GM_MAKE_ID32('m','g','s','r')
#define ID_mgsi GM_MAKE_ID32('m','g','s','i')
#define ID_mgti GM_MAKE_ID32('m','g','t','i')
#define ID_mggi GM_MAKE_ID32('m','g','g','i')
#define ID_mgvi GM_MAKE_ID32('m','g','v','i')
#define ID_msbp GM_MAKE_ID32('m','s','b','p')
#define ID_mbrk GM_MAKE_ID32('m','b','r','k')
#define ID_mbra GM_MAKE_ID32('m','b','r','a')
#define ID_mkil GM_MAKE_ID32('m','k','i','l')
#define ID_mkia GM_MAKE_ID32('m','k','i','a')
#define ID_mend GM_MAKE_ID32('m','e','n','d')

#define ID_dbrk GM_MAKE_ID32('d','b','r','k')
#define ID_dexc GM_MAKE_ID32('d','e','x','c')
#define ID_drun GM_MAKE_ID32('d','r','u','n')
#define ID_dstp GM_MAKE_ID32('d','s','t','p')
#define ID_dsrc GM_MAKE_ID32('d','s','r','c')
#define ID_dctx GM_MAKE_ID32('d','c','t','x')
#define ID_call GM_MAKE_ID32('c','a','l','l')
#define ID_vari GM_MAKE_ID32('v','a','r','i')
#define ID_brkp GM_MAKE_ID32('b','r','k','p')
#define ID_done GM_MAKE_ID32('d','o','n','e')
#define ID_dsri GM_MAKE_ID32('d','s','r','i')
#define ID_srci GM_MAKE_ID32('s','r','c','i')
#define ID_done GM_MAKE_ID32('d','o','n','e')
#define ID_dthi GM_MAKE_ID32('d','t','h','i')
#define ID_thri GM_MAKE_ID32('t','h','r','i')
#define ID_done GM_MAKE_ID32('d','o','n','e')
#define ID_derr GM_MAKE_ID32('d','e','r','r')
#define ID_dmsg GM_MAKE_ID32('d','m','s','g')
#define ID_dend GM_MAKE_ID32('d','e','n','d')
#define ID_dbps GM_MAKE_ID32('d','b','p','s')
#define ID_dbpc GM_MAKE_ID32('d','b','p','c')

// my new fns
#define ID_gbeg GM_MAKE_ID32('g','b','e','g')
#define ID_glob GM_MAKE_ID32('g','l','o','b')
#define ID_gend GM_MAKE_ID32('g','e','n','d')
#define ID_srun GM_MAKE_ID32('s','r','u','n')
#define ID_retv GM_MAKE_ID32('r','e','t','v')
//
// Please note that gmDebugger.c/.h are for implementing
// a debugger application and should not be included
// in an normal GM application build.
//


gmDebuggerSession::gmDebuggerSession()
{
	m_outSize = 256;
	m_out = (void*) new char[m_outSize];
	m_outCursor = 0;
	m_in = NULL;
	m_inCursor = m_inSize = 0; //NULL;
}


gmDebuggerSession::~gmDebuggerSession()
{
	if(m_out)
	{
		delete [] (char*)m_out;
	}
}


void gmDebuggerSession::UpdateDebugSession()
{
	for(;;)
	{
		m_in = DebuggerPumpMessage(m_inSize);
		if(m_in == NULL) break;
		m_inCursor = 0;

		int cmdId = 0;
		Unpack(cmdId);
		switch(cmdId)
		{
		case ID_dbrk :
			{ 
				int thId = 0, sourceId = 0, lineNum = 0;
				Unpack(thId).Unpack(sourceId).Unpack(lineNum);
				gmDebuggerBreak(thId, sourceId, lineNum);
				break;
			}
		case ID_drun :
			{
				int thId = 0, lineNum = 0;
				const char * funcName = 0, *fileName = 0;
				Unpack(thId).Unpack(lineNum).Unpack(funcName).Unpack(fileName);
				gmDebuggerRun(thId,lineNum,funcName,fileName);
				break;
			}
		case ID_dstp :
			{
				int thId = 0;
				Unpack(thId);
				gmDebuggerStop(thId);
				break;
			}
		case ID_dsrc :
			{
				int srcId = 0;
				const char * srcName = 0, *src = 0;
				Unpack(srcId).Unpack(srcName).Unpack(src);
				gmDebuggerSource(srcId, srcName, src);
				break;
			}
		case ID_dexc :
			{
				int thId = 0;
				Unpack(thId);
				gmDebuggerException(thId);
				break;
			}
		case ID_dctx :
			{
				int thId = 0, callFrame = 0;
				Unpack(thId).Unpack(callFrame); // thread id, callframe
				gmDebuggerBeginContext(thId, callFrame);
				for(;;)
				{
					Unpack(cmdId);
					if(cmdId == ID_call)
					{
						int callFrame = 0, srcId = 0, lineNum = 0, thisId = 0;
						const char * funcName = 0, *thisSymbol = 0, *thisValue = 0, * thisType = 0;
						Unpack(callFrame)
							.Unpack(funcName)
							.Unpack(srcId)
							.Unpack(lineNum)
							.Unpack(thisSymbol)
							.Unpack(thisValue)
							.Unpack(thisType)
							.Unpack(thisId);
						gmDebuggerContextCallFrame(callFrame, funcName, srcId, lineNum, thisSymbol, thisValue, thisType, thisId);
					}
					else if(cmdId == ID_vari)
					{
						int thisId = 0;
						const char * thisSymbol = 0, *thisValue = 0, * thisType = 0;
						Unpack(thisSymbol)
							.Unpack(thisValue)
							.Unpack(thisType)
							.Unpack(thisId);
						gmDebuggerContextVariable(thisSymbol, thisValue, thisType, thisId);
					}
					else if(cmdId == ID_brkp)
					{
						int lineNum = 0;
						Unpack(lineNum);
						gmDebuggerContextBreakpoint(lineNum);
					}
					else if(cmdId == ID_done) break;
					else break;
				}
				gmDebuggerEndContext();
				break;
			}
		case ID_dsri :
			// todo
			break;
		case ID_dthi :
			{
				gmDebuggerBeginThreadInfo();
				for(;;)
				{
					Unpack(cmdId);
					if(cmdId == ID_thri)
					{
						int thId = 0, lineNum = 0;
						const char * thState = 0, * funcName = 0, * fileName = 0;
						Unpack(thId)
							.Unpack(thState)
							.Unpack(lineNum)
							.Unpack(funcName)
							.Unpack(fileName);
						gmDebuggerThreadInfo(thId, thState, lineNum, funcName, fileName);
					}
					else if(cmdId == ID_done) break;
					else break;
				}
				gmDebuggerEndThreadInfo();
				break;
			}
		case ID_derr :
			{
				const char * str = 0;
				Unpack(str);
				gmDebuggerError(str);
				break;
			}
		case ID_dmsg :
			{
				const char * str = 0;
				Unpack(str);
				gmDebuggerMessage(str);
				break;
			}
		case ID_dbps :
			{
				int sourceId = 0, lineNum = 0, enabled = 0;
				Unpack(sourceId).Unpack(lineNum).Unpack(enabled);
				gmDebuggerBreakPointSet(sourceId, lineNum, enabled);
				break;
			}
		case ID_dbpc :
			{
				gmDebuggerBreakClear();
				break;
			}
		case ID_dend :
			{
				gmDebuggerQuit();
				break;
			}
		case ID_gbeg :
			{
				int varId = 0;
				Unpack(varId);
				gmDebuggerBeginGlobals(varId);
				for(;;)
				{
					Unpack(cmdId);
					if(cmdId == ID_glob)
					{
						int varId = 0;
						const char * thisSymbol = 0, * thisValue = 0, * thisType = 0;
						Unpack(thisSymbol)
							.Unpack(thisValue)
							.Unpack(thisType)
							.Unpack(varId);
						gmDebuggerGlobal(thisSymbol, thisValue, thisType, varId);
					}
					else if(cmdId == ID_gend) break;
					else break;
				}
				gmDebuggerEndGlobals();
				break;
			}
		case ID_retv :
			{
				int varId = 0;
				const char * thisValue = 0, * thisType = 0;
				Unpack(thisValue).Unpack(thisType).Unpack(varId);
				gmDebuggerReturnValue(thisValue,thisType,varId);
				break;
			}
		default:
			gmDebuggerError( "Invalid Message Id\n" );
			break;
		}
	}
}

bool gmDebuggerSession::OpenDebugSession()
{
	m_outCursor = 0;
	return true;
}

bool gmDebuggerSession::CloseDebugSession()
{
	return true;
}

gmDebuggerSession &gmDebuggerSession::Pack(int a_val)
{
	Need(4);
	memcpy((char *) m_out + m_outCursor, &a_val, 4);
	m_outCursor += 4;
	return *this;
}

gmDebuggerSession &gmDebuggerSession::Pack(const char * a_val)
{
	if(a_val)
	{
		int len = (int)strlen(a_val) + 1;
		Need(len);
		memcpy((char *) m_out + m_outCursor, a_val, len);
		m_outCursor += len;
	}
	else
	{
		Need(1);
		memcpy((char *) m_out + m_outCursor, "", 1);
		m_outCursor += 1;
	}
	return *this;
}

void gmDebuggerSession::Send()
{
	DebuggerSendMessage(m_out, m_outCursor);
	m_outCursor = 0;
}

gmDebuggerSession &gmDebuggerSession::Unpack(int &a_val)
{
	if(m_inCursor + 4 <= m_inSize)
	{
		memcpy(&a_val, (const char *) m_in + m_inCursor, 4);
		m_inCursor += 4;
	}
	else
	{
		a_val = 0;
	}
	return *this;
}

gmDebuggerSession &gmDebuggerSession::Unpack(const char * &a_val)
{
	a_val = (const char *) m_in + m_inCursor;
	m_inCursor += (int)strlen(a_val) + 1;
	return *this;
}

void gmDebuggerSession::Need(int a_bytes)
{
	if((m_outCursor + a_bytes) >= m_outSize)
	{
		int newSize = m_outSize + a_bytes + 256;
		void * buffer = (void*)new char[newSize];
		memcpy(buffer, m_out, m_outCursor);
		delete [] (char*)m_out;
		m_out = buffer;
		m_outSize = newSize;
	}
}

void gmDebuggerSession::gmMachineRun(int a_threadId)
{
	Pack(ID_mrun).Pack(a_threadId).Send();
}

void gmDebuggerSession::gmMachineStepInto(int a_threadId)
{
	Pack(ID_msin).Pack(a_threadId).Send();
}

void gmDebuggerSession::gmMachineStepOver(int a_threadId)
{
	Pack(ID_msov).Pack(a_threadId).Send();
}

void gmDebuggerSession::gmMachineStepOut(int a_threadId)
{
	Pack(ID_msou).Pack(a_threadId).Send();
}

void gmDebuggerSession::gmMachineGetContext(int a_threadId, int a_callframe)
{
	Pack(ID_mgct).Pack(a_threadId).Pack(a_callframe).Send();
}

void gmDebuggerSession::gmMachineGetSource(int a_sourceId)
{
	Pack(ID_mgsr).Pack(a_sourceId).Send();
}

void gmDebuggerSession::gmMachineGetSourceInfo()
{
	Pack(ID_mgsi).Send();
}

void gmDebuggerSession::gmMachineGetThreadInfo()
{
	Pack(ID_mgti).Send();
}

void gmDebuggerSession::gmMachineGetGlobalsInfo(int a_tableRef)
{
	Pack(ID_mggi).Pack(a_tableRef).Send();
}

void gmDebuggerSession::gmMachineGetVariableInfo(int a_variableId)
{
	Pack(ID_mgvi).Pack(a_variableId).Send();
}

void gmDebuggerSession::gmMachineSetBreakPoint(int a_sourceId, int a_lineNumber, int a_threadId, int a_enabled)
{
	Pack(ID_msbp).Pack(a_sourceId).Pack(a_lineNumber).Pack(a_threadId).Pack(a_enabled).Send();
}

void gmDebuggerSession::gmMachineBreak(int a_threadId)
{
	Pack(ID_mbrk).Pack(a_threadId).Send();
}

void gmDebuggerSession::gmMachineBreakAll()
{
	Pack(ID_mbra).Send();
}

void gmDebuggerSession::gmMachineKill(int a_threadId)
{
	Pack(ID_mkil).Pack(a_threadId).Send();
}

void gmDebuggerSession::gmMachineKillAll()
{
	Pack(ID_mkia).Send();
}

void gmDebuggerSession::gmMachineQuit()
{
	Pack(ID_mend).Send();
}

void gmDebuggerSession::gmMachineRunScript(const char *_script)
{
	Pack(ID_srun).Pack(_script).Send();
}
