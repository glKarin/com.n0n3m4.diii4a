/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop


#include "renderer/backend/ImmediateRendering.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/VertexArrayState.h"
#include "renderer/tr_local.h"


idCVar r_immediateRenderingEmulate(
	"r_immediateRenderingEmulate", "1", CVAR_BOOL | CVAR_RENDERER,
	"Enable emulation of deprecated immediate-mode OpenGL rendering? "
	"It is used mainly in debug tools. "
	"Note that this cannot be disabled in GL Core profile. "
);
idCVar r_immediateRenderingChunk(
	"r_immediateRenderingChunk", "32", CVAR_INTEGER | CVAR_RENDERER,
	"Flush accumulated draws prematurely if VBO size exceeds this number of kilobytes. "
	"If set to 0, then flushes after every glEnd. "
	"Only matters when r_immediateRenderingEmulate is ON. ",
	0, 1000000
);

struct ImmediateRenderingGlobals {
	idList<ImmediateRendering::VertexData> vertexBuffers[2];
	idList<ImmediateRendering::DrawCall> drawBuffers;
	bool redirectToGL = false;
};
//OpenGL can be only called from one thread anyway
//so it is perfectly OK to have global buffers
static ImmediateRenderingGlobals globals;


ImmediateRendering::ImmediateRendering() {
	if (r_glCoreProfile.GetInteger() > 0 && r_immediateRenderingEmulate.GetBool() == false)
		r_immediateRenderingEmulate.SetBool(true);
	globals.redirectToGL = (r_immediateRenderingEmulate.GetBool() == false);
	if (globals.redirectToGL)
		return;

	vertexList.Swap(globals.vertexBuffers[0]);
	tempList.Swap(globals.vertexBuffers[1]);
	drawList.Swap(globals.drawBuffers);
}

ImmediateRendering::~ImmediateRendering() {
	if (globals.redirectToGL)
		return;

	FlushInternal();

	vertexList.SetNum(0, false);
	tempList.SetNum(0, false);
	drawList.SetNum(0, false);
	vertexList.Swap(globals.vertexBuffers[0]);
	tempList.Swap(globals.vertexBuffers[1]);
	drawList.Swap(globals.drawBuffers);
}

void ImmediateRendering::FlushInternal() {
	if (globals.redirectToGL)
		return;

	//must be outside glBegin/glEnd
	assert(viBeginCurrent < 0);

	if (vertexList.Num() > 0) {
		GLuint vbo = 0;
		qglGenBuffers(1, &vbo);

		vaState.BindVertexBufferAndSetVertexFormat(vbo, VF_IMMEDIATE);
		qglBufferData(GL_ARRAY_BUFFER, vertexList.Num() * sizeof(VertexData), vertexList.Ptr(), GL_STREAM_DRAW);

		for (int i = 0; i < drawList.Num(); i++) {
			auto draw = drawList[i];
			if (draw.setupFunc)
				(*draw.setupFunc)(draw.setupContext);
			qglDrawArrays(draw.mode, draw.viBegin, draw.viEnd - draw.viBegin);
		}

		vaState.BindVertexBuffer();
		qglDeleteBuffers(1, &vbo);
	}

	vertexList.SetNum(0, false);
	drawList.SetNum(0, false);
}

void ImmediateRendering::Flush() {
	FlushInternal();
	state_setupFunc = nullptr;
	state_setupContext = nullptr;
}

void ImmediateRendering::DrawSetupRaw(DrawSetupFunc func, void *context) {
	if (globals.redirectToGL)
		return (*func)(context);

	assert(viBeginCurrent < 0);		//outside of glBegin/glEnd
	state_setupFunc = func;
	state_setupContext = context;
}

void ImmediateRendering::glBegin(GLenum mode) {
	if (globals.redirectToGL)
		return qglBegin(mode);

	//check that glBegin was NOT opened yet
	assert(viBeginCurrent < 0);

	state_currentMode = mode;
	viBeginCurrent = vertexList.Num();
}

void ImmediateRendering::glEnd() {
	if (globals.redirectToGL)
		return qglEnd();

	//check that glBegin was opened
	assert(viBeginCurrent >= 0);

	//preprocess vertices if the draw type is missing in Core profile
	auto CopyToTemp = [&]() {
		int cnt = vertexList.Num() - viBeginCurrent;
		tempList.SetNum(cnt, false);
		memcpy(tempList.Ptr(), vertexList.Ptr() + viBeginCurrent, cnt * sizeof(vertexList[0]));
	};
	if (state_currentMode == GL_QUADS) {
		CopyToTemp();
		int n = tempList.Num() / 4;
		vertexList.SetNum(viBeginCurrent + 6 * n, false);
		state_currentMode = GL_TRIANGLES;
		for (int i = 0; i < n; i++) {
			vertexList[viBeginCurrent + 6 * i + 0] = tempList[4 * i + 0];
			vertexList[viBeginCurrent + 6 * i + 1] = tempList[4 * i + 1];
			vertexList[viBeginCurrent + 6 * i + 2] = tempList[4 * i + 2];
			vertexList[viBeginCurrent + 6 * i + 3] = tempList[4 * i + 0];
			vertexList[viBeginCurrent + 6 * i + 4] = tempList[4 * i + 2];
			vertexList[viBeginCurrent + 6 * i + 5] = tempList[4 * i + 3];
		}
	}
	if (state_currentMode == GL_POLYGON) {
		CopyToTemp();
		int n = idMath::Imax(tempList.Num() - 2, 0);
		vertexList.SetNum(viBeginCurrent + 3 * n, false);
		state_currentMode = GL_TRIANGLES;
		for (int i = 0; i < n; i++) {
			vertexList[viBeginCurrent + 3 * i + 0] = tempList[0];
			vertexList[viBeginCurrent + 3 * i + 1] = tempList[i + 1];
			vertexList[viBeginCurrent + 3 * i + 2] = tempList[i + 2];
		}
	}
	tempList.SetNum(0, false);

	DrawCall dc = {
		state_currentMode,
		viBeginCurrent, vertexList.Num(),
		state_setupFunc, state_setupContext
	};
	drawList.AddGrow(dc);
	viBeginCurrent = -1;

	int vboCurrentSize = vertexList.Num() * sizeof(VertexData);
	if (vboCurrentSize >= r_immediateRenderingChunk.GetInteger() * 1024)
		FlushInternal();
}

void ImmediateRendering::glVertex4f(float x, float y, float z, float w) {
	if (globals.redirectToGL)
		return qglVertex4f(x, y, z, w);

	state_vertex.vertex.Set(x, y, z, w);
	vertexList.AddGrow(state_vertex);
}

void ImmediateRendering::glColor4f(float r, float g, float b, float a) {
	if (globals.redirectToGL)
		return qglColor4f(r, g, b, a);

#ifdef __SSE2__
	__m128 vec = _mm_setr_ps(r, g, b, a);
	vec = _mm_add_ps(_mm_mul_ps(vec, _mm_set1_ps(255.0f)), _mm_set1_ps(0.5f));
	__m128i icol = _mm_cvttps_epi32(vec);
	icol = _mm_packs_epi32(icol, icol);
	icol = _mm_packus_epi16(icol, icol);
	int wcol = _mm_cvtsi128_si32(icol);
	*(int*)(state_vertex.color) = wcol;
#else
	state_vertex.color[0] = (byte) idMath::Round(r * 255.0f);
	state_vertex.color[1] = (byte) idMath::Round(g * 255.0f);
	state_vertex.color[2] = (byte) idMath::Round(b * 255.0f);
	state_vertex.color[3] = (byte) idMath::Round(a * 255.0f);
#endif
}

void ImmediateRendering::glColor4ub(byte r, byte g, byte b, byte a) {
	if (globals.redirectToGL)
		return qglColor4ub(r, g, b, a);

	state_vertex.color[0] = r;
	state_vertex.color[1] = g;
	state_vertex.color[2] = b;
	state_vertex.color[3] = a;
}

void ImmediateRendering::glTexCoord4f(float s, float t, float r, float q) {
	if (globals.redirectToGL)
		return qglTexCoord4f(s, t, r, q);

	state_vertex.texCoord.Set(s, t);
}
