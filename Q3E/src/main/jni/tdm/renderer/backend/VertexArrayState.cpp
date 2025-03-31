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
#include "renderer/backend/VertexArrayState.h"

#include "renderer/backend/ImmediateRendering.h"
#include "renderer/backend/GLSLProgram.h"

VertexArrayState vaState;

const Attributes::Index VertexArrayState::allAttribsList[] = {
	Attributes::Position,
	Attributes::Normal,
	Attributes::Color,
	Attributes::TexCoord,
	Attributes::Tangent,
	Attributes::Bitangent,
};

void VertexArrayState::BindAttributesToProgram(GLSLProgram *program) {
	program->BindAttribLocation(Attributes::Position, "attr_Position");
	program->BindAttribLocation(Attributes::Normal, "attr_Normal");
	program->BindAttribLocation(Attributes::Color, "attr_Color");
	program->BindAttribLocation(Attributes::TexCoord, "attr_TexCoord");
	program->BindAttribLocation(Attributes::Tangent, "attr_Tangent");
	program->BindAttribLocation(Attributes::Bitangent, "attr_Bitangent");
}

VertexArrayState::VertexArrayState() {
	memset(formatAttribMasks, 0, sizeof(formatAttribMasks));

	formatAttribMasks[VF_REGULAR] |= (1 << Attributes::Position);
	formatAttribMasks[VF_REGULAR] |= (1 << Attributes::Normal);
	formatAttribMasks[VF_REGULAR] |= (1 << Attributes::Color);
	formatAttribMasks[VF_REGULAR] |= (1 << Attributes::TexCoord);
	formatAttribMasks[VF_REGULAR] |= (1 << Attributes::Tangent);
	formatAttribMasks[VF_REGULAR] |= (1 << Attributes::Bitangent);

	formatAttribMasks[VF_SHADOW] |= (1 << Attributes::Position);

	formatAttribMasks[VF_IMMEDIATE] |= (1 << Attributes::Position);
	formatAttribMasks[VF_IMMEDIATE] |= (1 << Attributes::Color);
	formatAttribMasks[VF_IMMEDIATE] |= (1 << Attributes::TexCoord);

	vao = 0;
	vbo = 0;
}

void VertexArrayState::UpdateVertexBuffer() {
	if (vbo == 0)
		return;		// API calls below generate errors otherwise

	if (vertexFormat == VF_REGULAR) {
		auto *null = (idDrawVert*)(size_t)0;
		qglVertexAttribPointer(Attributes::Position, 3, GL_FLOAT, false, sizeof(*null), null->xyz.ToFloatPtr());
		qglVertexAttribPointer(Attributes::Normal, 3, GL_FLOAT, false, sizeof(*null), null->normal.ToFloatPtr());
		qglVertexAttribPointer(Attributes::Color, 4, GL_UNSIGNED_BYTE, true, sizeof(*null), &null->color[0]);
		qglVertexAttribPointer(Attributes::TexCoord, 2, GL_FLOAT, false, sizeof(*null), null->st.ToFloatPtr());
		qglVertexAttribPointer(Attributes::Tangent, 3, GL_FLOAT, false, sizeof(*null), null->tangents[0].ToFloatPtr());
		qglVertexAttribPointer(Attributes::Bitangent, 3, GL_FLOAT, false, sizeof(*null), null->tangents[1].ToFloatPtr());
	} else if (vertexFormat == VF_SHADOW) {
		auto *null = (shadowCache_t*)(size_t)0;
		qglVertexAttribPointer(Attributes::Position, 4, GL_FLOAT, false, sizeof(*null), null->xyz.ToFloatPtr());
	} else if (vertexFormat == VF_IMMEDIATE) {
		auto *null = (ImmediateRendering::VertexData*)(size_t)0;
		qglVertexAttribPointer(Attributes::Position, 4, GL_FLOAT, GL_FALSE, sizeof(*null), null->vertex.ToFloatPtr());
		qglVertexAttribPointer(Attributes::Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(*null), &null->color[0]);
		qglVertexAttribPointer(Attributes::TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(*null), null->texCoord.ToFloatPtr());
	} else {
		assert(false);
	}
}

VertexArrayState::~VertexArrayState() = default;

void VertexArrayState::Init() {
	assert(vao == 0);
	qglGenVertexArrays(1, &vao);
	assert(vao != 0);

	qglBindVertexArray(vao);

	SetDefaultState();
}

void VertexArrayState::Shutdown() {
	assert(vao != 0);
	qglDeleteVertexArrays(1, &vao);
	vao = 0;

	qglBindVertexArray(0);
}

void VertexArrayState::SetDefaultState() {
	// reset to default state with currently bound VBOs
	memset(overriden, 0, sizeof(overriden));
	vbo = 0xDEADBEEF;				// ensure following Set sees change
	vertexFormat = VF_COUNT;		// ...
	BindVertexBufferAndSetVertexFormat(0, VF_REGULAR);
	for (Attributes::Index attrib : allAttribsList) {
		SetOverrideValuef(attrib);
	}
}

bool VertexArrayState::BindVertexBuffer_(unsigned vbo_) {
	if (vbo == vbo_)
		return false;

	vbo = vbo_;
	qglBindBuffer(GL_ARRAY_BUFFER, vbo);
	return true;
}
void VertexArrayState::BindVertexBuffer(unsigned vbo) {
	if (BindVertexBuffer_(vbo))
		UpdateVertexBuffer();
}

bool VertexArrayState::SetVertexFormat_(VertexFormat format) {
	if (vertexFormat == format)
		return false;

	vertexFormat = format;
	for (Attributes::Index attrib : allAttribsList) {
		ApplyAttribEnabled(attrib);
	}
	return true;
}
void VertexArrayState::SetVertexFormat(VertexFormat format) {
	if (SetVertexFormat_(format))
		UpdateVertexBuffer();
}

void VertexArrayState::BindVertexBufferAndSetVertexFormat(unsigned vbo, VertexFormat format) {
	bool changed = false;
	changed |= BindVertexBuffer_(vbo);
	changed |= SetVertexFormat_(format);
	if (changed)
		UpdateVertexBuffer();
}

void VertexArrayState::SetOverrideEnabled(Attributes::Index attrib, bool enabled) {
	if (overriden[attrib] == enabled)
		return;

	overriden[attrib] = enabled;
	ApplyAttribEnabled(attrib);
}

void VertexArrayState::SetOverrideValuef(Attributes::Index attrib, float x, float y, float z, float w) {
	qglVertexAttrib4f(attrib, x, y, z, w);
}

void VertexArrayState::ApplyAttribEnabled(Attributes::Index attrib) {
	int enabled = (formatAttribMasks[vertexFormat] >> attrib) & 1;
	if (overriden[attrib])
		enabled = false;

	if (enabled) {
		qglEnableVertexAttribArray(attrib);
	} else {
		qglDisableVertexAttribArray(attrib);
	}
}
