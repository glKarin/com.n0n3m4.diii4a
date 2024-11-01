#include "PrecompCommon.h"

#include "RecastInterfaces.h"

RecastBuildContext::RecastBuildContext() {
}

RecastBuildContext::~RecastBuildContext() {
}

// Virtual functions for custom implementations.
void RecastBuildContext::doResetLog()
{
	m_messageCount = 0;
	m_textPoolSize = 0;
}

void RecastBuildContext::doLog(const rcLogCategory category, const char* msg, const int len)
{
	if (!len) return;
	if (m_messageCount >= MAX_MESSAGES)
		return;
	char* dst = &m_textPool[m_textPoolSize];
	int n = TEXT_POOL_SIZE - m_textPoolSize;
	if (n < 2)
		return;
	// Store category
	*dst = (char)category;
	n--;
	// Store message
	const int count = rcMin(len+1, n);
	memcpy(dst+1, msg, count);
	dst[count+1] = '\0';
	m_textPoolSize += count+1;
	m_messages[m_messageCount++] = dst;
}

void RecastBuildContext::doResetTimers()
{
	for (int i = 0; i < RC_MAX_TIMERS; ++i)
		m_accTime[i] = -1;
}

void RecastBuildContext::doStartTimer(const rcTimerLabel label)
{
	m_timers[label].Reset();
}

void RecastBuildContext::doStopTimer(const rcTimerLabel label)
{
	const float deltaTime = (float)m_timers[label].GetElapsedSeconds();
	if (m_accTime[label] == -1)
		m_accTime[label] = deltaTime;
	else
		m_accTime[label] += deltaTime;
}

int RecastBuildContext::doGetAccumulatedTime(const rcTimerLabel label) const
{
	return (int)(m_accTime[label] * 1000.0f);
}

void RecastBuildContext::dumpLog(const char* format, ...)
{
	// Print header.
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	printf("\n");

	// Print messages
	const int TAB_STOPS[4] = { 28, 36, 44, 52 };
	for (int i = 0; i < m_messageCount; ++i)
	{
		const char* msg = m_messages[i]+1;
		int n = 0;
		while (*msg)
		{
			if (*msg == '\t')
			{
				int count = 1;
				for (int j = 0; j < 4; ++j)
				{
					if (n < TAB_STOPS[j])
					{
						count = TAB_STOPS[j] - n;
						break;
					}
				}
				while (--count)
				{
					putchar(' ');
					n++;
				}
			}
			else
			{
				putchar(*msg);
				n++;
			}
			msg++;
		}
		putchar('\n');
	}
}

int RecastBuildContext::getLogCount() const
{
	return m_messageCount;
}

const char* RecastBuildContext::getLogText(const int i) const
{
	return m_messages[i]+1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

FileIO::FileIO() :
m_mode(-1)
{
}

FileIO::~FileIO()
{
	m_file.Close();
}

bool FileIO::openForWrite(const char* path)
{
	m_mode = 1;
	return m_file.OpenForWrite( path, File::Text, false );
}

bool FileIO::openForRead(const char* path)
{
	m_mode = 2;
	return m_file.OpenForRead( path, File::Text );
}

bool FileIO::isWriting() const
{
	return m_mode == 1;
}

bool FileIO::isReading() const
{
	return m_mode == 2;
}

bool FileIO::write(const void* ptr, const size_t size)
{
	if( m_file.IsOpen() ) {
		m_file.Write( ptr, size, 1 );
		return true;
	}
	return false;
}

bool FileIO::read(void* ptr, const size_t size)
{
	if( m_file.IsOpen() ) {
		m_file.Read( ptr, size, 1 );
		return true;
	}
	return false;
}


