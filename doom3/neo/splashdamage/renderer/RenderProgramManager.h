//karin: SplashDamage materialStage shader manager
#ifndef _KARIN_RENDERPROGRAMMANAGER_H
#define _KARIN_RENDERPROGRAMMANAGER_H
#include "idlib/containers/List.h"

class sdRenderProgram;

class sdRenderProgramManager
{
public:
    									sdRenderProgramManager(void);
    									~sdRenderProgramManager();

    // parse
    const sdRenderProgram *				LoadProgram(const char *name);
	void								ReloadAll(void);
	void								CheckCVars(void);

    static void							LoadRenderProgram_f(const idCmdArgs &args);
    static void							ListRenderPrograms_f(const idCmdArgs &args);
    static void							ReloadAllRenderPrograms_f(const idCmdArgs &args);

private:
    const sdRenderProgram *				Alloc(const char *name);
    const sdRenderProgram *				Find(const char *name);

private:
    idList<sdRenderProgram *>			programs;
};

extern sdRenderProgramManager *renderProgramManager;

void R_CheckRenderProgramCVars(void);
#ifdef _MULTITHREAD
void RB_ReloadRenderPrograms(void);
#endif

#endif //_KARIN_RENDERPROGRAMMANAGER_H
