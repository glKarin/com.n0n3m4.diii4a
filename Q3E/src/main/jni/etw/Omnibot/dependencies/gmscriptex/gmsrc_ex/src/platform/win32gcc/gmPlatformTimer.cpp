#include <time.h>

clock_t		perfTimer;

void gmPlatformTimerStart()
{
	perfTimer = clock();
}

double gmPlatformTimerGetElapsedSecs()
{
	return static_cast<double>(clock() - perfTimer) / CLOCKS_PER_SEC;
}

