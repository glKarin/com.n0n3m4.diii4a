#ifndef __GM_DEBUGWINDOW_H__
#define __GM_DEBUGWINDOW_H__

#ifdef ENABLE_DEBUG_WINDOW

class gmMachine;
void gmBindDebugWindowLibrary(gmMachine *_machine);
void gmBindDebugWindowLibraryDocs(gmMachine *_m, gmBind2::TableConstructor &_tc);

#endif

#endif
