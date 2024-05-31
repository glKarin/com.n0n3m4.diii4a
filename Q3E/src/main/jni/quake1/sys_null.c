
#include "darkplaces.h"


// =======================================================================
// General routines
// =======================================================================

void Sys_SDL_Shutdown(void)
{
}

void Sys_SDL_Dialog(const char *title, const char *string)
{
}

char *Sys_SDL_GetClipboardData (void)
{
	return NULL;
}

void Sys_SDL_Init(void)
{
}

qbool sys_supportsdlgetticks = false;
unsigned int Sys_SDL_GetTicks (void)
{
	Sys_Error("Called Sys_SDL_GetTicks on non-SDL target");
	return 0;
}
void Sys_SDL_Delay (unsigned int milliseconds)
{
	Sys_Error("Called Sys_SDL_Delay on non-SDL target");
}

int main(int argc, char *argv[])
{
	return Sys_Main(argc, argv);
}
