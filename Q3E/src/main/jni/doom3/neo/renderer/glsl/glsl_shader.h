#ifndef _KARIN_GLSL_SHADER_H
#define _KARIN_GLSL_SHADER_H

// Unuse C++11 raw string literals for Traditional C++98

#define GLSL_SHADER // static

#include "glsl_shader_100.h"

#ifdef GL_ES_VERSION_3_0
#include "glsl_shader_300.h"
#endif

#endif
