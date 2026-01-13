#define _OPENGLES3

#define QGLPROC(name, rettype, args) rettype q##name args {}

#include "../../renderer/qgl_proc.h"

// g++ -E gen_stub_gl.cpp > gen_stub_gl
