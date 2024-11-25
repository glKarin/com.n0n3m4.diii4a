/*
    _____               __  ___          __            ____        _      __
   / ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
  / (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
  \___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
                                               /___/             /_/
                                             
  See Copyright Notice in gmMachine.h

*/

#ifndef _GMDEBUGGER_H_
#define _GMDEBUGGER_H_

//
// Please note that gmDebugger.c/.h are for implementing
// a debugger application and should not be included
// in an normal GM application build.
//

class gmDebuggerSession;

/// \class gmDebuggerSession
class gmDebuggerSession
{
protected:

  gmDebuggerSession();
  virtual ~gmDebuggerSession();

  /// \brief Update() must be called to pump messages
  void UpdateDebugSession();

  /// \brief OpenDebugSession() will start debugging
  bool OpenDebugSession();
  
  /// \brief CloseDebugSession() will stop debugging
  bool CloseDebugSession();

  virtual void DebuggerSendMessage(const void * a_command, int a_len) = 0;
  virtual const void * DebuggerPumpMessage(int &a_len) = 0;

  // send message helpers
  gmDebuggerSession &Pack(int a_val);
  gmDebuggerSession &Pack(const char * a_val);
  void Send();

  // rcv message helpers
  gmDebuggerSession &Unpack(int &a_val);
  gmDebuggerSession &Unpack(const char * &a_val);

protected:

	//
	// the debugger must implement the following functions
	//

	virtual void gmDebuggerBreak(int a_threadId, int a_sourceId, int a_lineNumber) = 0;
	virtual void gmDebuggerRun(int a_threadId, int a_lineNum, const char *a_func, const char *a_file) = 0;
	virtual void gmDebuggerStop(int a_threadId) = 0;
	virtual void gmDebuggerSource(int a_sourceId, const char * a_sourceName, const char * a_source) = 0;
	virtual void gmDebuggerException(int a_threadId) = 0;

	virtual void gmDebuggerBeginContext(int a_threadId, int a_callFrame) = 0;
	virtual void gmDebuggerContextCallFrame(int a_callFrame, const char * a_functionName, int a_sourceId, int a_lineNumber, const char * a_thisSymbol, const char * a_thisValue, const char * a_thisType, int a_thisId) = 0;
	virtual void gmDebuggerContextVariable(const char * a_varSymbol, const char * a_varValue, const char * a_varType, int a_varId) = 0;
	virtual void gmDebuggerContextBreakpoint(int a_lineNum) = 0;
	virtual void gmDebuggerEndContext() = 0;

	virtual void gmDebuggerBeginThreadInfo() = 0;
	virtual void gmDebuggerThreadInfo(int a_threadId, const char * a_threadState, int a_lineNum, const char * a_func, const char * a_file) = 0;
	virtual void gmDebuggerEndThreadInfo() = 0;

	virtual void gmDebuggerError(const char * a_error) = 0;
	virtual void gmDebuggerMessage(const char * a_message) = 0;
	virtual void gmDebuggerBreakPointSet(int a_sourceId, int a_lineNum, int a_enabled) = 0;
	virtual void gmDebuggerBreakClear() = 0;

	virtual void gmDebuggerBeginGlobals(int a_VarId) = 0;
	virtual void gmDebuggerGlobal(const char * a_varSymbol, const char * a_varValue, const char * a_varType, int a_varId) = 0;
	virtual void gmDebuggerEndGlobals() = 0;

	virtual void gmDebuggerReturnValue(const char * a_retVal, const char * a_retType, int a_retVarId) = 0;

	virtual void gmDebuggerQuit() = 0;

	//
	// the debugger can use the following functions to send messages to the machine
	//

	void gmMachineRun(int a_threadId);
	void gmMachineStepInto(int a_threadId);
	void gmMachineStepOver(int a_threadId);
	void gmMachineStepOut(int a_threadId);
	void gmMachineGetContext(int a_threadId, int a_callframe);
	void gmMachineGetSource(int a_sourceId);
	void gmMachineGetSourceInfo();
	void gmMachineGetThreadInfo();
	void gmMachineGetGlobalsInfo(int a_tableRef);
	void gmMachineGetVariableInfo(int a_variableId);
	void gmMachineSetBreakPoint(int a_sourceId, int a_lineNumber, int a_threadId, int a_enabled);
	void gmMachineBreak(int a_threadId);
	void gmMachineBreakAll();
	void gmMachineKill(int a_threadId);
	void gmMachineKillAll();
	void gmMachineQuit();
	void gmMachineRunScript(const char *_script);
private:

  void * m_out;
  int m_outCursor, m_outSize;
  void Need(int a_bytes);

  const void * m_in;
  int m_inCursor, m_inSize;
};


#endif

