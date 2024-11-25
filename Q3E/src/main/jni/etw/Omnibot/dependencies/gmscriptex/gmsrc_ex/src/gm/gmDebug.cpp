/*
_____               __  ___          __            ____        _      __
/ ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
/ (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
\___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
/___/             /_/

See Copyright Notice in gmMachine.h

*/

#include "gmDebug.h"
#include "gmConfig.h"
#include "gmMachine.h"
#include "gmThread.h"

#if GMDEBUG_SUPPORT


#define ID_mrun GM_MAKE_ID32('m','r','u','n')
#define ID_msin GM_MAKE_ID32('m','s','i','n')
#define ID_msou GM_MAKE_ID32('m','s','o','u')
#define ID_msov GM_MAKE_ID32('m','s','o','v')
#define ID_mgct GM_MAKE_ID32('m','g','c','t')
#define ID_mgsr GM_MAKE_ID32('m','g','s','r')
#define ID_mgsi GM_MAKE_ID32('m','g','s','i')
#define ID_mgti GM_MAKE_ID32('m','g','t','i')
#define ID_mgvi GM_MAKE_ID32('m','g','v','i')
#define ID_msbp GM_MAKE_ID32('m','s','b','p')
#define ID_mbrk GM_MAKE_ID32('m','b','r','k')
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
#define ID_gbeg GM_MAKE_ID32('g','b','e','g') // begin globals
#define ID_glob GM_MAKE_ID32('g','l','o','b') // global
#define ID_gend GM_MAKE_ID32('g','e','n','d') // end globals
#define ID_srun GM_MAKE_ID32('s','r','u','n')
#define ID_mggi GM_MAKE_ID32('m','g','g','i')
#define ID_mbra GM_MAKE_ID32('m','b','r','a')
#define ID_mkil GM_MAKE_ID32('m','k','i','l')
#define ID_mkia GM_MAKE_ID32('m','k','i','a')
#define ID_retv GM_MAKE_ID32('r','e','t','v')

//
// functions to handle incoming commands from a debugger
//

void gmMachineRun(gmDebugSession * a_session, int a_threadId);
void gmMachineStepInto(gmDebugSession * a_session, int a_threadId);
void gmMachineStepOver(gmDebugSession * a_session, int a_threadId);
void gmMachineStepOut(gmDebugSession * a_session, int a_threadId);
void gmMachineGetContext(gmDebugSession * a_session, int a_threadId, int a_callframe);
void gmMachineGetSource(gmDebugSession * a_session, int a_sourceId);
void gmMachineGetSourceInfo(gmDebugSession * a_session);
void gmMachineGetThreadInfo(gmDebugSession * a_session);
void gmMachineGetVariableInfo(gmDebugSession * a_session, int a_variableId);
void gmMachineGetGlobalInfo(gmDebugSession * a_session, int a_tableId);
void gmMachineSetBreakPoint(gmDebugSession * a_session, int a_sourceId, int a_lineNumber, int a_threadId, int a_enabled);
void gmMachineBreak(gmDebugSession * a_session, int a_threadId);
void gmMachineBreakAll(gmDebugSession * a_session);
void gmMachineKill(gmDebugSession * a_session, int a_threadId);
void gmMachineKillAll(gmDebugSession * a_session);
void gmMachineQuit(gmDebugSession * a_session);

//
// functions to package outgoing messages to a debugger
//

void gmDebuggerBreak(gmDebugSession * a_session, int a_threadId, int a_sourceId, int a_lineNumber);
void gmDebuggerRun(gmDebugSession * a_session, int a_threadId, int a_LineNo, const char *a_FuncName, const char *a_File);
void gmDebuggerStop(gmDebugSession * a_session, int a_threadId);
void gmDebuggerSource(gmDebugSession * a_session, int a_sourceId, const char * a_sourceName, const char * a_source);
void gmDebuggerException(gmDebugSession * a_session, int a_threadId);
void gmDebuggerReturnVal(gmDebugSession * a_session, const char * a_varValue, const char * a_varType, int a_varId);

void gmDebuggerBeginContext(gmDebugSession * a_session, int a_threadId, int a_callFrame);
void gmDebuggerContextCallFrame(gmDebugSession * a_session, int a_callFrame, const char * a_functionName, int a_sourceId, int a_lineNumber, const char * a_thisSymbol, const char * a_thisValue, const char * a_thisType, int a_thisId);
void gmDebuggerContextVariable(gmDebugSession * a_session, const char * a_varSymbol, const char * a_varValue, const char * a_varType, int a_varId);
void gmDebuggerContextBreakpoint(gmDebugSession * a_session, int a_lineNum);
void gmDebuggerEndContext(gmDebugSession * a_session);

void gmDebuggerBeginSourceInfo(gmDebugSession * a_session);
void gmDebuggerSourceInfo(gmDebugSession * a_session, int a_sourceId, const char * a_sourceName);
void gmDebuggerEndSourceInfo(gmDebugSession * a_session);

void gmDebuggerBeginThreadInfo(gmDebugSession * a_session);
void gmDebuggerThreadInfo(gmDebugSession * a_session, int a_threadId, const char * a_threadState, int a_lineNum, const char * a_func, const char * a_file);
void gmDebuggerEndThreadInfo(gmDebugSession * a_session);

void gmDebuggerError(gmDebugSession * a_session, const char * a_error);
void gmDebuggerMessage(gmDebugSession * a_session, const char * a_message);
void gmDebuggerBreakpointSet(gmDebugSession * a_session, int a_sourceId, int a_lineNum, int a_enabled);
void gmDebuggerBreakpointClear(gmDebugSession * a_session);
void gmDebuggerQuit(gmDebugSession * a_session);

void gmDebuggerBeginGlobals(gmDebugSession * a_session, int a_VarId);
void gmDebuggerGlobal(gmDebugSession * a_session, const char * a_varSymbol, const char * a_varValue, const char * a_varType, int a_varId);
void gmDebuggerEndGlobals(gmDebugSession * a_session);

//
// debug machine callback
//

enum gmdThreadFlags
{
	TF_STEPOVER  = (1 << 0),
	TF_STEPINTO  = (1 << 1),
	TF_STEPOUT   = (1 << 2),
	TF_BREAK     = (1 << 3),
	TF_KILL      = (1 << 4),
};

// the following callbacks return true if the thread is to yield after completion of the callback.
static bool LineCallback(gmThread * a_thread)
{
	gmDebugSession * session = (gmDebugSession *) a_thread->GetMachine()->m_debugUser;
	GM_ASSERT(session);

	if(!(a_thread->m_debugFlags & TF_STEPOVER) ||
		(a_thread->m_debugUser != ((a_thread->GetFrame()) ? a_thread->GetFrame()->m_returnBase : 0)))
	{
		int * bp = session->FindBreakPoint((void *) a_thread->GetInstruction());
		if(bp == NULL)
			return false;

		if(*bp && *bp != a_thread->GetId())
			return false;
	}

	a_thread->m_debugFlags = TF_BREAK;
	const gmFunctionObject * fn = a_thread->GetFunctionObject();
	gmDebuggerBreak(session, a_thread->GetId(), fn->GetSourceId(), fn->GetLine(a_thread->GetInstruction()));
	return true;
}

static bool CallCallback(gmThread * a_thread)
{
	gmDebugSession * session = (gmDebugSession *) a_thread->GetMachine()->m_debugUser;
	GM_ASSERT(session);
	if(a_thread->m_debugFlags & TF_STEPINTO)
	{
		a_thread->m_debugFlags = TF_BREAK;
		const gmFunctionObject * fn = a_thread->GetFunctionObject();
		gmDebuggerBreak(session, a_thread->GetId(), fn->GetSourceId(), fn->GetLine(a_thread->GetInstruction()));
		return true;
	}
	return false;
}

static void CallEndCallback(gmThread * a_thread, const gmVariable & retVal)
{
	gmDebugSession * session = (gmDebugSession *) a_thread->GetMachine()->m_debugUser;
	GM_ASSERT(session);
	if(a_thread->m_debugFlags & TF_STEPOVER)
	{
		enum { BufferSize = 256 };
		char valueBuffer[BufferSize] = {};
		const char * retValue = retVal.AsString(a_thread->GetMachine(),valueBuffer,BufferSize);
		const char * retType = a_thread->GetMachine()->GetTypeName(retVal.m_type);
		const int varId = 
			retVal.IsReference() && !retVal.IsFunction() ? retVal.m_value.m_ref : 0;
		gmDebuggerReturnVal(session,retValue,retType,varId);
	}
}

static bool RetCallback(gmThread * a_thread)
{
	gmDebugSession * session = (gmDebugSession *) a_thread->GetMachine()->m_debugUser;
	GM_ASSERT(session);
	if(((a_thread->m_debugFlags & TF_STEPOUT) && (a_thread->m_debugUser == a_thread->GetIntBase())) ||
		((a_thread->m_debugFlags & TF_STEPOVER) && (a_thread->m_debugUser == a_thread->GetIntBase())))
	{
		a_thread->m_debugFlags = TF_BREAK;
		const gmFunctionObject * fn = a_thread->GetFunctionObject();
		gmDebuggerBreak(session, a_thread->GetId(), fn->GetSourceId(), fn->GetLine(a_thread->GetInstruction()));
		return true;
	}
	return false;
}

static bool IsBrokenCallback(gmThread * a_thread)
{
	return (a_thread->m_debugFlags & TF_BREAK) > 0;
}

static bool IsKilledCallback(gmThread * a_thread)
{
	return (a_thread->m_debugFlags & TF_KILL) > 0;
}

static gmMachineCallback s_prevMachineCallback = NULL;
static gmPrintCallback s_prevPrintCallback = NULL;
bool GM_CDECL gmdMachineCallback(gmMachine * a_machine, gmMachineCommand a_command, const void * a_context)
{
	gmDebugSession * session = (gmDebugSession *) a_machine->m_debugUser;
	const gmThread * thread = (const gmThread *) a_context;

	// chain callback
	if(s_prevMachineCallback) s_prevMachineCallback(a_machine, a_command, a_context);

	// do we have a debug session?
	if(session == NULL) return false;

	// command
	switch(a_command)
	{
	case MC_THREAD_EXCEPTION :
		{
			// send thread exception message
			gmDebuggerException(session, thread->GetId());
			a_machine->GetLog();
			bool first = true;
			const char * entry;
			while((entry = a_machine->GetLog().GetEntry(first)))
			{
				gmDebuggerError(session, entry);
			}
			return true;
		}
	case MC_THREAD_CREATE :
		{
			int Line = -1;
			const char *DebugName = 0;
			const char *Source = 0;
			const char *SourceFile = 0;
			const gmFunctionObject *pFn = thread->GetFunctionObject();
			if(pFn)
			{
				Line = pFn->GetLine(thread->GetInstruction());
				DebugName = pFn->GetDebugName();
				a_machine->GetSourceCode(pFn->GetSourceId(), Source, SourceFile);
			}

			gmDebuggerRun(session, thread->GetId(), Line, DebugName, SourceFile);
			break;
		}
	case MC_THREAD_DESTROY :
		{
			gmDebuggerStop(session, thread->GetId());
			break;
		}
	default : break;
	};

	return false;
}

void GM_CDECL gmdPrintCallback(gmMachine * a_machine, const char * a_string)
{
	gmDebugSession * session = (gmDebugSession *) a_machine->m_debugUser;

	// chain callback
	if(s_prevPrintCallback) s_prevPrintCallback(a_machine, a_string);

	gmDebuggerMessage(session, a_string);
}


//
// debug session
//

gmDebugSession::gmDebugSession() : 
m_breaks(32)
{
	m_machine = NULL;
}


gmDebugSession::~gmDebugSession()
{
	m_breaks.RemoveAndDeleteAll();
}


void gmDebugSession::UpdateDebugSession()
{
	for(;;)
	{
		int len;
		const void * msg = PumpDebuggerMessage(len);
		if(msg == NULL)
			break;

		m_in.Open(msg, len);

		// parse the message
		const char *ca;
		int id, pa, pb, pc, pd;
		Unpack(id);
		switch(id)
		{
		case ID_mrun :
			Unpack(id);
			gmMachineRun(this, id);
			break;
		case ID_msin :
			Unpack(id);
			gmMachineStepInto(this, id);
			break;
		case ID_msou :
			Unpack(id);
			gmMachineStepOut(this, id);
			break;
		case ID_msov :
			Unpack(id);
			gmMachineStepOver(this, id);
			break;
		case ID_mgct :
			Unpack(id).Unpack(pa);
			gmMachineGetContext(this, id, pa);
			break;
		case ID_mgsr :
			Unpack(id);
			gmMachineGetSource(this, id);
			break;
		case ID_mgsi :
			gmMachineGetSourceInfo(this);
			break;
		case ID_mgti :
			gmMachineGetThreadInfo(this);
			break;
		case ID_mgvi :
			Unpack(id);
			gmMachineGetVariableInfo(this, id);
			break;
		case ID_msbp :
			Unpack(pb).Unpack(pc).Unpack(id).Unpack(pd);
			gmMachineSetBreakPoint(this, pb, pc, id, pd);
			break;
		case ID_mbrk :
			Unpack(id);
			gmMachineBreak(this, id);
			break;
		case ID_mbra :
			gmMachineBreakAll(this);
			break;
		case ID_mkil:
			Unpack(id);
			gmMachineKill(this, id);
			break;
		case ID_mkia:
			gmMachineKillAll(this);
			break;
		case ID_mend :
			gmMachineQuit(this);
			break;
		case ID_mggi:
			Unpack(id);
			gmMachineGetGlobalInfo(this, id);
			break;
		case ID_srun:
			ca = 0;
			Unpack(ca);
			if(ca)
				this->GetMachine()->ExecuteString(ca);
			break;
		default:;
		}
	}

	gmMachineGetThreadInfo(this);
}


bool gmDebugSession::OpenDebugSession(gmMachine * a_machine)
{
	CloseDebugSession();
	m_machine = a_machine;
	m_machine->m_debugUser = this;
	m_machine->m_line = LineCallback;
	m_machine->m_call = CallCallback;
	m_machine->m_callEnd = CallEndCallback;
	m_machine->m_isBroken = IsBrokenCallback;
	m_machine->m_isKilled = IsKilledCallback;
	m_machine->m_return = RetCallback;	
	s_prevMachineCallback = a_machine->s_machineCallback;
	s_prevPrintCallback = a_machine->s_printCallback;

	a_machine->s_machineCallback = gmdMachineCallback;	
	a_machine->s_printCallback = gmdPrintCallback;
	return true;
}


static bool threadIterClose(gmThread * a_thread, void * a_context)
{
	a_thread->m_debugFlags = 0;
	a_thread->m_debugUser = 0;
	a_thread->m_debugState = 0;
	return true;
}


bool gmDebugSession::CloseDebugSession()
{
	if(m_machine && m_machine->m_debugUser == this)
	{
		gmDebuggerQuit(this);

		m_machine->m_debugUser = NULL;
		m_machine->s_machineCallback = s_prevMachineCallback;
		m_machine->s_printCallback = s_prevPrintCallback;

		m_machine->m_line = NULL;
		m_machine->m_call = NULL;
		m_machine->m_callEnd = NULL;
		m_machine->m_isBroken = NULL;
		m_machine->m_isKilled = NULL;
		m_machine->m_return = NULL;

		m_machine->KillExceptionThreads();
		m_machine->ForEachThread(threadIterClose, NULL);
		m_machine = NULL;

		m_breaks.RemoveAndDeleteAll();
		m_out.ResetAndFreeMemory();

		return true;
	}

	m_breaks.RemoveAndDeleteAll();
	m_out.ResetAndFreeMemory();

	return false;
}


gmDebugSession &gmDebugSession::Pack(int a_val)
{
	m_out << a_val;
	return *this;
}

#ifdef GM_PTR_SIZE_64 // Only needed if types gmptr != gmint
gmDebugSession &gmDebugSession::Pack(gmint64 a_val)
{
	m_out << a_val;
	return *this;
}
#endif // GM_PTR_SIZE_64

gmDebugSession &gmDebugSession::Pack(const char * a_val)
{
	if(a_val)
		m_out.Write(a_val, (unsigned int)strlen(a_val) + 1);
	else
		m_out.Write("", 1);
	return *this;
}


void gmDebugSession::Send()
{
	SendDebuggerMessage(m_out.GetData(), m_out.GetSize());
	m_out.Reset();
}


gmDebugSession &gmDebugSession::Unpack(int &a_val)
{
	if(m_in.Read(&a_val, 4) != 4) a_val = 0;
	return *this;
}

#ifdef GM_PTR_SIZE_64 // Only needed if types gmptr != gmint
gmDebugSession &gmDebugSession::Unpack(gmint64 &a_val)
{
	if(m_in.Read(&a_val, sizeof(gmint64)) != sizeof(gmint64)) a_val = 0;
	return *this;
}
#endif //GM_PTR_SIZE_64

gmDebugSession &gmDebugSession::Unpack(const char * &a_val)
{
	// this is dangerous!!!
	a_val = &m_in.GetData()[m_in.Tell()];
	int len = (int)strlen(a_val);
	m_in.Seek(m_in.Tell() + len + 1);
	return *this;
}


bool gmDebugSession::AddBreakPoint(const void * a_bp, int a_threadId, int a_lineNum)
{
	BreakPoint * bp = m_breaks.Find((void *const&)a_bp);
	if(bp) return false;
	bp = GM_NEW( BreakPoint() );
	bp->m_bp = a_bp;
	bp->m_threadId = a_threadId;
	bp->m_lineNum = a_lineNum;
	m_breaks.Insert(bp);
	return true;
}


int * gmDebugSession::FindBreakPoint(const void * a_bp)
{
	BreakPoint * bp = m_breaks.Find((void *const&)a_bp);
	if(bp)
	{
		return &bp->m_threadId;
	}
	return NULL;
}


bool gmDebugSession::RemoveBreakPoint(const void * a_bp)
{
	BreakPoint * bp = m_breaks.Find((void *const&)a_bp);
	if(bp)
	{
		m_breaks.Remove(bp);
		delete bp;
		return true;
	}
	return false;
}

int gmDebugSession::GetBreakPointsForThread(int a_threadId, int *a_bpline, const int maxlines) 
{
	int numBp = 0;
	BreakpointMap::Iterator it = m_breaks.First();
	while( it.IsValid() && numBp < maxlines ) {
		if ( (*it).m_threadId == a_threadId ) {
			a_bpline[numBp++] = (*it).m_lineNum;
		}
		it.Inc();
	}
	return numBp;
}

//
// implementation
//

void gmMachineRun(gmDebugSession * a_session, int a_threadId)
{
	gmThread * thread = a_session->GetMachine()->GetThread(a_threadId);
	if(thread)
	{
		thread->m_debugFlags = 0;
	}
}

void gmMachineStepInto(gmDebugSession * a_session, int a_threadId)
{
	gmThread * thread = a_session->GetMachine()->GetThread(a_threadId);
	if(thread)
	{
		thread->m_debugUser = (thread->GetFrame()) ? thread->GetFrame()->m_returnBase : 0;
		thread->m_debugFlags = TF_STEPINTO | TF_STEPOVER;
	}
}

void gmMachineStepOver(gmDebugSession * a_session, int a_threadId)
{
	gmThread * thread = a_session->GetMachine()->GetThread(a_threadId);
	if(thread)
	{
		thread->m_debugUser = (thread->GetFrame()) ? thread->GetFrame()->m_returnBase : 0;
		thread->m_debugFlags = TF_STEPOVER;
	}
}

void gmMachineStepOut(gmDebugSession * a_session, int a_threadId)
{
	gmThread * thread = a_session->GetMachine()->GetThread(a_threadId);
	if(thread)
	{
		thread->m_debugUser = (thread->GetFrame()) ? thread->GetFrame()->m_returnBase : 0;
		thread->m_debugFlags = TF_STEPOUT;
	}
}

void gmMachineGetContext(gmDebugSession * a_session, int a_threadId, int a_callframe)
{
	const int buffSize = 256;
	char buff[buffSize];  // buff is used for AsString

	gmThread * thread = a_session->GetMachine()->GetThread(a_threadId);
	if(thread)
	{
		// count the number of frames on the thread
		int numFrames = 0;
		const gmStackFrame * frame = thread->GetFrame();
		while(frame)
		{
			++numFrames;
			frame = frame->m_prev;
		}

		// if a valid frame was requested, fill out a context.
		if(a_callframe >= 0 && a_callframe <= numFrames)
		{
			gmDebuggerBeginContext(a_session, a_threadId, a_callframe);

			// pack frames
			frame = thread->GetFrame();
			numFrames = 0;

			gmVariable * base = thread->GetBase();
			const gmuint8 * ip = thread->GetInstruction();

			while(frame)
			{
				// get the function object
				gmVariable * fnVar = base - 1;
				if(fnVar->m_type == GM_FUNCTION)
				{
					gmFunctionObject * fn = (gmFunctionObject *) GM_MOBJECT(thread->GetMachine(), fnVar->m_value.m_ref);

					// this
					const char * thisType = thread->GetMachine()->GetTypeName(base[-2].m_type);
					base[-2].AsString(thread->GetMachine(), buff, buffSize);
					gmDebuggerContextCallFrame(
						a_session, 
						numFrames, 
						fn->GetDebugName(), 
						fn->GetSourceId(), 
						fn->GetLine(ip), 
						"this", 
						buff, 
						thisType, 
						(base[-2].IsReference()) ? base[-2].m_value.m_ref : 0);

					if(numFrames == a_callframe)
					{
						// this is the active frame, fill out the variables
						int i;
						for(i = 0; i < fn->GetNumParamsLocals(); ++i)
						{
							const char * typeName = thread->GetMachine()->GetTypeName(base[i].m_type);
							base[i].AsString(thread->GetMachine(), buff, buffSize);
							gmDebuggerContextVariable(a_session, fn->GetSymbol(i), buff, typeName, (base[i].IsReference()) ? base[i].m_value.m_ref : 0);
						}
					}
				}
				else
				{
					const char * typeName = thread->GetMachine()->GetTypeName(base[-2].m_type);
					base[-2].AsString(thread->GetMachine(), buff, buffSize);
					gmDebuggerContextCallFrame(a_session, numFrames, "unknown", 0, 0, "this", buff, typeName, (base[-2].IsReference()) ? base[-2].m_value.m_ref : 0);
				}

				// next call frame
				++numFrames;
				base = thread->GetBottom() + frame->m_returnBase;
				ip = frame->m_returnAddress;
				frame = frame->m_prev;
			}

			enum { MaxBpLineNum = 4096 };
			int bpLineNum[MaxBpLineNum] = {};
			const int numLines = a_session->GetBreakPointsForThread(a_threadId, bpLineNum, MaxBpLineNum);
			for(int i = 0; i < numLines; ++i)
			{
				gmDebuggerContextBreakpoint(a_session,bpLineNum[i]);
			}

			gmDebuggerEndContext(a_session);
		}
	}
}

void gmMachineGetSource(gmDebugSession * a_session, int a_sourceId)
{
	const char * source;
	const char * filename;
	if(a_session->GetMachine()->GetSourceCode(a_sourceId, source, filename))
	{
		gmDebuggerSource(a_session, a_sourceId, filename, source);
	}
}

void gmMachineGetSourceInfo(gmDebugSession * a_session)
{
	// todo
}


static bool threadIter(gmThread * a_thread, void * a_context)
{
	if ( a_thread->m_debugState != a_thread->GetState() ) {
		a_thread->m_debugState = a_thread->GetState();

		gmDebugSession * session = (gmDebugSession *) a_context;
		const char * state = "exception";
		if(a_thread->m_debugFlags)
			state = "debug";
		else if(a_thread->GetState() == gmThread::EXCEPTION)
			state = "exception";
		else if(a_thread->GetState() == gmThread::RUNNING)
			state = "running";
		else if(a_thread->GetState() == gmThread::BLOCKED)
			state = "blocked";
		else if(a_thread->GetState() == gmThread::SLEEPING)
			state = "sleeping";

		int lineNum = -1;
		const char *funcName = 0;
		const char *source = 0;
		const char *sourceFile = 0;
		const gmFunctionObject *fn = a_thread->GetFunctionObject();
		if(fn)
		{
			lineNum = fn->GetLine(a_thread->GetInstruction());
			funcName = fn->GetDebugName();
			a_thread->GetMachine()->GetSourceCode(fn->GetSourceId(), source, sourceFile);
		}

		gmDebuggerThreadInfo(session, a_thread->GetId(), state, lineNum, funcName, sourceFile);
	}
	return true;
}


void gmMachineGetThreadInfo(gmDebugSession * a_session)
{
	gmDebuggerBeginThreadInfo(a_session);
	a_session->GetMachine()->ForEachThread(threadIter, a_session);
	gmDebuggerEndThreadInfo(a_session);
}

void gmMachineGetVariableInfo(gmDebugSession * a_session, int a_variableId)
{
	// todo:
}

namespace DebugChild
{
	gmDebugSession *Session = 0;
	void _gmChildInfoCallback(const char * a_symbol, const char * a_value, const char * a_valuetype, gmptr a_varId)
	{
		gmDebuggerGlobal(Session, a_symbol, a_value, a_valuetype, a_varId);
	}
};

void gmMachineGetGlobalInfo(gmDebugSession * a_session, int a_varId)
{
	gmObject *pObj = a_session->GetMachine()->GetGlobals();
	if(a_varId != 0) {
		pObj = a_session->GetMachine()->GetGC()->CheckReference(a_varId);
	}

	if(!pObj)
		return;

	gmDebuggerBeginGlobals(a_session, a_varId);

	gmVariable v(pObj->GetType(), pObj->GetRef());
	DebugChild::Session = a_session;
	v.DebugInfo(a_session->GetMachine(), DebugChild::_gmChildInfoCallback);

	// Send type table if we're sending the globals
	/*if(a_varId==0)
	{
		int iType = 1;
		gmTableObject *pTbl = 0;
		while(pTbl = a_session->GetMachine()->GetTypeTable(iType))
		{
			gmVariable typeVar(pTbl);
			typeVar.DebugInfo(a_session->GetMachine(), DebugChild::_gmChildInfoCallback);
			++iType;
		}	
	}*/
	gmDebuggerEndGlobals(a_session);
}

void gmMachineSetBreakPoint(gmDebugSession * a_session, int a_sourceId, int a_lineNumber, int a_threadId, int a_enabled)
{
	// get break point
	const void * bp = (const void *) a_session->GetMachine()->GetInstructionAtBreakPoint(a_sourceId, a_lineNumber);
	if(bp)
	{
		// get to next instruction
		bp = (const void *) (((const char *) bp) + 4);

		int * id = a_session->FindBreakPoint(bp);
		if(id)
		{
			if(!a_enabled)
			{
				a_session->RemoveBreakPoint(bp);
				gmDebuggerBreakpointSet(a_session, a_sourceId, a_lineNumber, 0);
			}
		}
		else
		{
			if(a_session->AddBreakPoint(bp, a_threadId, a_lineNumber))
			{
				gmDebuggerBreakpointSet(a_session, a_sourceId, a_lineNumber, 1);
			}
		}
	}		
}

void gmMachineBreak(gmDebugSession * a_session, int a_threadId)
{
	gmThread * thread = a_session->GetMachine()->GetThread(a_threadId);
	if(thread)
	{
		thread->m_debugUser = (thread->GetFrame()) ? thread->GetFrame()->m_returnBase : 0;
		thread->m_debugFlags = TF_STEPINTO | TF_STEPOVER;
	}
}

bool gmfDebugBreakAll(gmThread * a_thread, void * a_context)
{
	a_thread->m_debugUser = (a_thread->GetFrame()) ? a_thread->GetFrame()->m_returnBase : 0;
	a_thread->m_debugFlags = TF_STEPINTO | TF_STEPOVER;
	return true;
}
void gmMachineBreakAll(gmDebugSession * a_session)
{
	a_session->GetMachine()->ForEachThread(gmfDebugBreakAll,0);
}

void gmMachineKill(gmDebugSession * a_session, int a_threadId)
{
	gmThread * thread = a_session->GetMachine()->GetThread(a_threadId);
	if(thread)
	{
		thread->m_debugFlags = TF_KILL;
	}
}

bool gmfDebugKillAll(gmThread * a_thread, void * a_context)
{
	a_thread->m_debugFlags = TF_KILL;
	return true;
}
void gmMachineKillAll(gmDebugSession * a_session)
{
	a_session->GetMachine()->ForEachThread(gmfDebugKillAll,0);
}
void gmMachineQuit(gmDebugSession * a_session)
{
	a_session->CloseDebugSession();
}

void gmDebuggerBreak(gmDebugSession * a_session, int a_threadId, int a_sourceId, int a_lineNumber) {
	a_session->Pack(ID_dbrk).Pack(a_threadId).Pack(a_sourceId).Pack(a_lineNumber).Send();
}
void gmDebuggerException(gmDebugSession * a_session, int a_threadId) {
	a_session->Pack(ID_dexc).Pack(a_threadId).Send();
}
void gmDebuggerReturnVal(gmDebugSession * a_session, const char * a_varValue, const char * a_varType, int a_varId) {
	a_session->Pack(ID_retv).Pack(a_varValue).Pack(a_varType).Pack(a_varId).Send();
}
void gmDebuggerRun(gmDebugSession * a_session, int a_threadId, int a_LineNo, const char *a_FuncName, const char *a_File) {
	a_session->Pack(ID_drun).Pack(a_threadId).Pack(a_LineNo).Pack(a_FuncName).Pack(a_File).Send();
}
void gmDebuggerStop(gmDebugSession * a_session, int a_threadId) {
	a_session->Pack(ID_dstp).Pack(a_threadId).Send();
}
void gmDebuggerSource(gmDebugSession * a_session, int a_sourceId, const char * a_sourceName, const char * a_source) {
	a_session->Pack(ID_dsrc).Pack(a_sourceId).Pack(a_sourceName).Pack(a_source).Send();
}
void gmDebuggerBeginContext(gmDebugSession * a_session, int a_threadId, int a_callFrame) {
	a_session->Pack(ID_dctx).Pack(a_threadId).Pack(a_callFrame);
}
void gmDebuggerContextCallFrame(gmDebugSession * a_session, int a_callFrame, const char * a_functionName, int a_sourceId, int a_lineNumber, const char * a_thisSymbol, const char * a_thisValue, const char * a_thisType, int a_thisId) {
	a_session->Pack(ID_call).Pack(a_callFrame).Pack(a_functionName).Pack(a_sourceId).Pack(a_lineNumber).Pack(a_thisSymbol).Pack(a_thisValue).Pack(a_thisType).Pack(a_thisId);
}
void gmDebuggerContextVariable(gmDebugSession * a_session, const char * a_varSymbol, const char * a_varValue, const char * a_varType, int a_varId) {
	a_session->Pack(ID_vari).Pack(a_varSymbol).Pack(a_varValue).Pack(a_varType).Pack(a_varId);
}
void gmDebuggerContextBreakpoint(gmDebugSession * a_session, int a_lineNum) {
	a_session->Pack(ID_brkp).Pack(a_lineNum);
}
void gmDebuggerEndContext(gmDebugSession * a_session) {
	a_session->Pack(ID_done).Send();
}
void gmDebuggerBeginSourceInfo(gmDebugSession * a_session) {
	a_session->Pack(ID_dsri);
}
void gmDebuggerSourceInfo(gmDebugSession * a_session, int a_sourceId, const char * a_sourceName) {
	a_session->Pack(ID_srci).Pack(a_sourceId).Pack(a_sourceName);
}
void gmDebuggerEndSourceInfo(gmDebugSession * a_session) {
	a_session->Pack(ID_done).Send();
}
void gmDebuggerBeginThreadInfo(gmDebugSession * a_session) {
	a_session->Pack(ID_dthi);
}
void gmDebuggerThreadInfo(gmDebugSession * a_session, int a_threadId, const char * a_threadState, int a_lineNum, const char * a_func, const char * a_file) {
	a_session->Pack(ID_thri).Pack(a_threadId).Pack(a_threadState).Pack(a_lineNum).Pack(a_func).Pack(a_file);
}
void gmDebuggerEndThreadInfo(gmDebugSession * a_session) {
	a_session->Pack(ID_done).Send();
}
void gmDebuggerError(gmDebugSession * a_session, const char * a_error) {
	a_session->Pack(ID_derr).Pack(a_error).Send();
}
void gmDebuggerMessage(gmDebugSession * a_session, const char * a_message) {
	a_session->Pack(ID_dmsg).Pack(a_message).Send();
}
void gmDebuggerBreakpointSet(gmDebugSession * a_session, int a_sourceId, int a_lineNum, int a_enabled) {
	a_session->Pack(ID_dbps).Pack(a_sourceId).Pack(a_lineNum).Pack(a_enabled).Send();
}
void gmDebuggerBreakpointClear(gmDebugSession * a_session) {
	a_session->Pack(ID_dbpc).Send();
}
void gmDebuggerQuit(gmDebugSession * a_session) {
	a_session->Pack(ID_dend).Send();
}
void gmDebuggerBeginGlobals(gmDebugSession * a_session, int a_VarId) {
	a_session->Pack(ID_gbeg).Pack(a_VarId);
}
void gmDebuggerGlobal(gmDebugSession * a_session, const char * a_varSymbol, const char * a_varValue, const char * a_varType, int a_varId) {
	a_session->Pack(ID_glob).Pack(a_varSymbol).Pack(a_varValue).Pack(a_varType).Pack(a_varId);
}
void gmDebuggerEndGlobals(gmDebugSession * a_session) {
	a_session->Pack(ID_gend).Send();
}



//
// lib binding
//


int GM_CDECL gmdDebug(gmThread * a_thread)
{
	// if the machine has a debug session, attach a debug hook to the thread
	if(a_thread->GetMachine()->m_debugUser && a_thread->GetMachine()->GetDebugMode())
	{
		a_thread->m_debugUser = (a_thread->GetFrame()) ? a_thread->GetFrame()->m_returnBase : 0;
		a_thread->m_debugFlags = TF_STEPINTO | TF_STEPOVER;
	}
	return GM_OK;
}



static gmFunctionEntry s_debugLib[] = 
{ 
	/*gm
	\lib gm
	\brief functions in the gm lib are all global scope
	*/

	/*gm
	\function debug
	\brief debug will cause a the debugger to break at this point while running.
	*/

	{"debug", gmdDebug},
};



void gmBindDebugLib(gmMachine * a_machine)
{
	a_machine->RegisterLibrary(s_debugLib, sizeof(s_debugLib) / sizeof(s_debugLib[0]));
}

#endif
