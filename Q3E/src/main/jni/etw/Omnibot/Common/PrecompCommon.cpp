#include "PrecompCommon.h"

// Global variables.
IEngineInterface *g_EngineFuncs = 0;

#ifdef BOOST_NO_EXCEPTIONS
void throw_exception(std::exception const & e)
{
	OBASSERT(0, e.what());
}
#endif
