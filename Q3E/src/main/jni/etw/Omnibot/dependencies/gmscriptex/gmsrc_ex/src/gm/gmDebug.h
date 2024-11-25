/*
_____               __  ___          __            ____        _      __
/ ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
/ (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
\___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
/___/             /_/

See Copyright Notice in gmMachine.h

*/

#ifndef _GMDEBUG_H_
#define _GMDEBUG_H_

#include "gmConfig.h"
#include "gmStreamBuffer.h"
#include "gmHash.h"

class gmMachine;
class gmDebugSession;

// bind debug lib
void gmBindDebugLib(gmMachine * a_machine);

#if GMDEBUG_SUPPORT

/// \class gmDebugSession
class gmDebugSession
{
public:

	gmDebugSession();
	virtual ~gmDebugSession();

	/// \brief UpdateDebugSession() must be called to pump messages
	void UpdateDebugSession();

	/// \brief Open() will start debugging on a_machine
	bool OpenDebugSession(gmMachine * a_machine);

	/// \brief Close() will stop debugging
	bool CloseDebugSession();

	/// \brief GetMachine()
	inline gmMachine * GetMachine() const { return m_machine; }

	// callbacks used to hook up comms
	virtual void SendDebuggerMessage(const void * a_command, int a_len) = 0;
	virtual const void * PumpDebuggerMessage(int &a_len) = 0;

	// send message helpers
	gmDebugSession &Pack(int a_val);
#ifdef GM_PTR_SIZE_64 // Only needed if gmptr != gmint
	gmDebugSession &Pack(gmint64 a_val);
#endif // GM_PTR_SIZE_64
	gmDebugSession &Pack(const char * a_val);
	void Send();

	// rcv message helpers
	gmDebugSession &Unpack(int &a_val);
#ifdef GM_PTR_SIZE_64
	gmDebugSession &Unpack(gmint64 &a_val);
#endif // GM_PTR_SIZE_64
	gmDebugSession &Unpack(const char * &a_val);

	// helpers
	bool AddBreakPoint(const void * a_bp, int a_threadId, int a_lineNum);
	int * FindBreakPoint(const void * a_bp); // return thread id
	bool RemoveBreakPoint(const void * a_bp);
	int GetBreakPointsForThread(int a_threadId, int *a_bpline, const int maxlines) ;

private:

	class BreakPoint : public gmHashNode<void *, BreakPoint>
	{
	public:
		inline const void * GetKey() const { return m_bp; }
		const void * m_bp;
		int m_threadId;
		int m_lineNum;
	};

	gmMachine * m_machine;

	typedef gmHash<void *, BreakPoint> BreakpointMap;
	BreakpointMap m_breaks;
	gmStreamBufferDynamic m_out;
	gmStreamBufferStatic m_in;
};

#endif

#endif // _GMDEBUG_H_
