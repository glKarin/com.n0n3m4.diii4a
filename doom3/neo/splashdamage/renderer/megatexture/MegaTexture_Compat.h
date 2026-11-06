//
// Only for compat with jmarshall23's idMegaTexture of DarklightNG
//

#ifndef _SD_MEGATEXTURE_COMPAT_H
#define _SD_MEGATEXTURE_COMPAT_H

#define qglCompressedTexSubImage2DARB qglCompressedTexSubImage2D
#define qglCompressedTexSubImage2DARB qglCompressedTexSubImage2D

#define R_SetGLSLProgramEnvParameter(shaderType, index, value) GL_Uniform4fv(SHADER_PARMS_ADDR(u_vertexParm, index), value);

#include "idlib/threading/Lock.h"

#endif //_SD_MEGATEXTURE_COMPAT_H
