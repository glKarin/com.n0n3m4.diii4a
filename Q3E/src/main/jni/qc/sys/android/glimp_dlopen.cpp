#ifndef ID_GL_HARDLINK

#include "../../idlib/precompiled.h"

#define QGLPROC(name, rettype, args) rettype (GL_APIENTRYP q##name) args;
#include "../../renderer/qgl_proc.h"
#undef QGLPROC

bool GLimp_dlopen() {
    return true;
}

void GLimp_dlclose() {}

void GLimp_LoadProc() {
#define QGLPROC(name, rettype, args) \
	q##name = (rettype(GL_APIENTRYP)args)GLimp_ExtensionPointer(#name); \
	if (!q##name) \
		common->FatalError("Unable to initialize OpenGL (%s)", #name); \
	else common->Printf("GetProc (%s) -> %p\n", #name, q##name);

#include "../../renderer/qgl_proc.h"
}

void GLimp_BindLogging(){}
void GLimp_BindNative(){}
#endif
