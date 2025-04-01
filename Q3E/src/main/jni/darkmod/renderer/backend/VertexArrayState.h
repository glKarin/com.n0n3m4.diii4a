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

#pragma once

class GLSLProgram;

namespace Attributes {
	// attribute indexes and GLSL names are the same for all shaders
	enum Index {
		Position  = 0,
		Normal	  = 2,
		Color	  = 3,
		TexCoord  = 8,
		Tangent	  = 9,
		Bitangent = 10,
		Count,
	};
};

enum VertexFormat {
	VF_REGULAR,		// idDrawVert
	VF_SHADOW,		// shadowCache_t
	VF_IMMEDIATE,	// ImmediateRendering::VertexData
	VF_COUNT,
};

/**
 * This is the only code which manages OpenGL's "Vertex Array Object", vertex attributes format, and vertex buffer object binds.
 * Also it supports overriding an attribute with a given constant value.
 * Vertex format, overrides, VBO binds are independent: you can change them in any order.
 */
class VertexArrayState {
public:
	VertexArrayState();
	~VertexArrayState();

	void Init();
	void Shutdown();

	// reset all settings to defaults
	void SetDefaultState();

	// bind specified VBO
	void BindVertexBuffer(unsigned vbo = 0);
	// set current vertex format
	void SetVertexFormat(VertexFormat format);

	// same as calling SetVertexFormat + BindVertexBuffer
	void BindVertexBufferAndSetVertexFormat(unsigned vbo, VertexFormat format);

	// override some attribute value with given value (thus don't read attribute from buffer)
	// note: overrides are global state, they persist after vertex format of vertex buffer bind is changed
	void SetOverrideEnabled(Attributes::Index attrib, bool enabled);
	void SetOverrideValuef(Attributes::Index attrib, float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 1.0f);

	// connect GLSL program to attribute indexes
	// (called automatically for all GLSL programs)
	void BindAttributesToProgram(GLSLProgram *program);

private:
	void UpdateVertexBuffer();
	void ApplyAttribEnabled(Attributes::Index attrib);
	bool BindVertexBuffer_(unsigned vbo);
	bool SetVertexFormat_(VertexFormat format);


	VertexFormat vertexFormat;
	bool overriden[Attributes::Count];

	unsigned vao;
	unsigned vbo;

	// const data
	int formatAttribMasks[VF_COUNT];
	static const Attributes::Index allAttribsList[];
};

extern VertexArrayState vaState;
