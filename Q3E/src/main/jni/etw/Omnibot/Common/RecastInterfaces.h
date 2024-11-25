#ifndef __RECAST_INTERFACES_H__
#define __RECAST_INTERFACES_H__

#include "DebugDraw.h"
#include "Recast.h"
#include "RecastDump.h"

// Recast build context.
class RecastBuildContext : public rcContext
{
public:
	RecastBuildContext();
	virtual ~RecastBuildContext();

	// Dumps the log to stdout.
	void dumpLog(const char* format, ...);
	// Returns number of log messages.
	int getLogCount() const;
	// Returns log message text.
	const char* getLogText(const int i) const;

protected:	
	// Virtual functions for custom implementations.
	virtual void doResetLog();
	virtual void doLog(const rcLogCategory /*category*/, const char* /*msg*/, const int /*len*/);
	virtual void doResetTimers();
	virtual void doStartTimer(const rcTimerLabel /*label*/);
	virtual void doStopTimer(const rcTimerLabel /*label*/);
	virtual int doGetAccumulatedTime(const rcTimerLabel /*label*/) const;

private:
	Timer				m_timers[RC_MAX_TIMERS];
	float				m_accTime[RC_MAX_TIMERS];

	static const int	MAX_MESSAGES = 1000;
	const char*			m_messages[MAX_MESSAGES];
	int					m_messageCount;
	static const int	TEXT_POOL_SIZE = 8000;
	char				m_textPool[TEXT_POOL_SIZE];
	int					m_textPoolSize;
};

// stdio file implementation.
class FileIO : public duFileIO
{
	File m_file;
	int m_mode;
public:
	FileIO();
	virtual ~FileIO();
	bool openForWrite(const char* path);
	bool openForRead(const char* path);
	virtual bool isWriting() const;
	virtual bool isReading() const;
	virtual bool write(const void* ptr, const size_t size);
	virtual bool read(void* ptr, const size_t size);
};

#endif

