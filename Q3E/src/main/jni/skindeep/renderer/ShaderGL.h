// ----------------------------------------------------------------
// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.
// 
// Released under the BSD License
// See LICENSE in root directory for full details.
// ----------------------------------------------------------------

#pragma once
#include "sys/platform.h"
#include "idlib/math/Math.h"
#include "idlib/Str.h"
#include "idlib/math/Matrix.h"
#include "idlib/math/Vector.h"
#include "SDL_opengl.h"
#include "qgl.h"

const int NUM_PROG_ENV = 32;
const int NUM_PROG_LOCAL = 8;

template <typename T>
void idUpdateUniform(GLint uniformLoc, const T& value) {}

template<>
ID_INLINE void idUpdateUniform(GLint uniformLoc, const float& value)
{
	qglUniform1f(uniformLoc, value);
}

template<>
ID_INLINE void idUpdateUniform(GLint uniformLoc, const unsigned int& value)
{
	qglUniform1ui(uniformLoc, value);
}

template<>
ID_INLINE void idUpdateUniform(GLint uniformLoc, const idVec4& value)
{
	qglUniform4fv(uniformLoc, 1, value.ToFloatPtr());
}

template<>
ID_INLINE void idUpdateUniform(GLint uniformLoc, const idMat4& value)
{
	qglUniformMatrix4fv(uniformLoc, 1, GL_FALSE, value.ToFloatPtr());
}

template <typename T>
class idShaderUniform
{
public:
	void Setup(GLuint program, const char* uniformName, const T& initialValue)
	{
		uniformLoc = qglGetUniformLocation(program, uniformName);
		data = initialValue;
		if (uniformLoc != -1)
		{
			idUpdateUniform(uniformLoc, data);
		}
	}

	void operator=(const T& value)
	{
		if (value != data)
		{
			data = value;
			if (uniformLoc != -1)
			{
				idUpdateUniform(uniformLoc, data);
			}
		}
	}
#if 0 //karin: debug uniform data
	const T & Data() const {
		return data;
	}
#endif
private:
	T data;
	GLint uniformLoc;
};

class idShaderGL
{
public:
	idShaderGL();
	~idShaderGL();
	bool LoadCombined(const idStr& prefix);
	bool Load(const idStr& vertName, const idStr& fragName);
	void Unload();
	// Set this as the active shader program
	void SetActive();
	// Sets a Matrix uniform
	void SetMatrixUniform(const char* name, const idMat4& matrix);
	// Sets an array of matrix uniforms
	void SetMatrixUniforms(const char* name, idMat4* matrices, unsigned count);
	// Sets 2D/3D/4D vectors
	void SetVector4Uniform(const char* name, const idVec4& vector);
	void SetVector3Uniform(const char* name, const idVec3& vector);
	void SetVector2Uniform(const char* name, const idVec2& vector);
	// Sets a float uniform
	void SetFloatUniform(const char* name, float value);
	// Sets an integer uniform
	void SetIntUniform(const char* name, int value);

	// These uniforms are available to all shader programs
	struct {
		// Matrices
		idShaderUniform<idMat4> projectionMatrix;
		idShaderUniform<idMat4> modelviewMatrix;
		idShaderUniform<idMat4> textureMatrix;

		// Program environment
		idShaderUniform<idVec4> programEnv[NUM_PROG_ENV];
		// Program local
		idShaderUniform<idVec4> programLocal[NUM_PROG_LOCAL];
		// Custom texture value (for post process)
		idShaderUniform<unsigned int> customPostValue;
	} uniforms;
#if 0 //karin: debug shader name
	const char * Name(void) const {
		return mProgramName.c_str();
	}
#endif
private:
	// Tries to compile the specified shader
	bool CompileShader(const idStr& fileName,
					   GLenum shaderType,
					   GLuint& outShader);
	
	// Tests whether shader compiled successfully
	bool IsCompiled(GLuint shader);
	// Tests whether vertex/fragment programs link
	bool IsValidProgram();
	// Sets up any global uniform/uniform buffer objects
	void SetupUniforms();
private:
	// Store the shader object IDs
	GLuint mVertexShader;
	GLuint mFragShader;
	GLuint mShaderProgram;
	idStr mProgramName;
};
