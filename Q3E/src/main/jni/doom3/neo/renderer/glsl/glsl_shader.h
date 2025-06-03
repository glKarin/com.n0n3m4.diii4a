#ifndef _KARIN_GLSL_SHADER_H
#define _KARIN_GLSL_SHADER_H

// Unuse C++11 raw string literals for Traditional C++98

#define GLSL_SHADER // static

#ifdef COLOR_MODULATE_IS_NORMALIZED
#define BYTE_COLOR(x) #x
#else
#define BYTE_COLOR(x) "(" #x " / 255.0)"
#endif

// heatHaze using BFG version, otherelse using 2004 ARB translation
#define HEATHAZE_BFG 1

// PLACEHOLDER -> empty string
// ALIAS -> alias var
#define ES2_SHADER_SOURCE_PLACEHOLDER(x) \
	GLSL_SHADER const char x##_VERT[] = ""; \
	GLSL_SHADER const char x##_FRAG[] = "";

#define ES2_SHADER_SOURCE_ALIAS(x, a) \
	GLSL_SHADER const char * x##_VERT = a##_VERT; \
	GLSL_SHADER const char * x##_FRAG = a##_FRAG;

#include "glsl_shader_100.h"
#include "d3xp_glsl_shader_100.h"

#ifdef _POSTPROCESS
#include "postprocess_glsl_shader_100.h"
#endif

#ifdef _HUMANHEAD
#include "prey_glsl_shader_100.h"
#endif

#ifdef GL_ES_VERSION_3_0
#define ES3_SHADER_SOURCE_PLACEHOLDER(x) \
	GLSL_SHADER const char ES3_##x##_VERT[] = ""; \
	GLSL_SHADER const char ES3_##x##_FRAG[] = "";

#define ES_SHADER_SOURCE_PLACEHOLDER(x) \
	ES2_SHADER_SOURCE_PLACEHOLDER(x); \
	ES3_SHADER_SOURCE_PLACEHOLDER(x);

#define ES3_SHADER_SOURCE_ALIAS(x, a) \
	GLSL_SHADER const char * ES3_##x##_VERT = ES3_##a##_VERT; \
	GLSL_SHADER const char * ES3_##x##_FRAG = ES3_##a##_FRAG;

#define ES_SHADER_SOURCE_ALIAS(x, a) \
	ES2_SHADER_SOURCE_ALIAS(x, a); \
	ES3_SHADER_SOURCE_ALIAS(x, a);

#include "glsl_shader_300.h"
#include "d3xp_glsl_shader_300.h"

#ifdef _POSTPROCESS
#include "postprocess_glsl_shader_300.h"
#endif

#ifdef _HUMANHEAD
#include "prey_glsl_shader_300.h"
#endif

#else
#define ES_SHADER_SOURCE_PLACEHOLDER ES2_SHADER_SOURCE_PLACEHOLDER
#define ES_SHADER_SOURCE_ALIAS ES2_SHADER_SOURCE_ALIAS
#endif

// ES_SHADER_SOURCE_PLACEHOLDER(GLASSWARP)

//#ifdef _HUMANHEAD
//#endif

#endif
