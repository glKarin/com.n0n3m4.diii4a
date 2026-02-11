// ----------------------------------------------------------------
// From Game Programming in C++ by Sanjay Madhav
// Copyright (C) 2017 Sanjay Madhav. All rights reserved.
// 
// Released under the BSD License
// See LICENSE in root directory for full details.
// ----------------------------------------------------------------

#include <fstream>
#include <sstream>
#include "ShaderGL.h"
#include "framework/Common.h"
#include "tr_local.h"

idShaderGL::idShaderGL()
	: mShaderProgram(0)
	, mVertexShader(0)
	, mFragShader(0)
{
	
}

idShaderGL::~idShaderGL()
{

}

bool idShaderGL::LoadCombined(const idStr& prefix)
{
	mProgramName = prefix;
	return Load(prefix + ".vert", prefix + ".frag");
}

bool idShaderGL::Load(const idStr& vertName, const idStr& fragName)
{
	// Compile vertex and pixel shaders
	if (!CompileShader(vertName,
					   GL_VERTEX_SHADER,
					   mVertexShader) ||
		!CompileShader(fragName,
					   GL_FRAGMENT_SHADER,
					   mFragShader))
	{
		return false;
	}
	
	// Now create a shader program that
	// links together the vertex/frag shaders
	mShaderProgram = qglCreateProgram();
	qglAttachShader(mShaderProgram, mVertexShader);
	qglAttachShader(mShaderProgram, mFragShader);
	qglLinkProgram(mShaderProgram);
	
	// Verify that the program linked successfully
	if (!IsValidProgram())
	{
		common->Error("%s/%s: failed to link\n", vertName.c_str(), fragName.c_str());
		return false;
	}
	
	common->Printf("%s/%s\n", vertName.c_str(), fragName.c_str());

#ifdef _DEBUG
	if (r_debugGLContext.GetBool() && glConfig.khrDebugAvailable)
	{
		qglObjectLabel(GL_SHADER, mVertexShader, vertName.Length(), vertName.c_str());
		qglObjectLabel(GL_SHADER, mFragShader, fragName.Length(), fragName.c_str());
	}
#endif

	SetActive();
	SetupUniforms();

	return true;
}

void idShaderGL::Unload()
{
	// Delete the program/shaders
	if (mShaderProgram != 0)
	{
		qglDeleteProgram(mShaderProgram);
		qglDeleteShader(mVertexShader);
		qglDeleteShader(mFragShader);
	}
}

void idShaderGL::SetActive()
{
	// Set this program as the active one
	qglUseProgram(mShaderProgram);
}

void idShaderGL::SetMatrixUniform(const char* name, const idMat4& matrix)
{
	// Find the uniform by this name
	GLuint loc = qglGetUniformLocation(mShaderProgram, name);
	// Send the matrix data to the uniform
	qglUniformMatrix4fv(loc, 1, GL_TRUE, matrix.ToFloatPtr());
}

void idShaderGL::SetMatrixUniforms(const char* name, idMat4* matrices, unsigned count)
{
	GLuint loc = qglGetUniformLocation(mShaderProgram, name);
	// Send the matrix data to the uniform
	qglUniformMatrix4fv(loc, count, GL_TRUE, matrices->ToFloatPtr());
}

void idShaderGL::SetVector4Uniform(const char* name, const idVec4& vector)
{
	GLuint loc = qglGetUniformLocation(mShaderProgram, name);
	// Send the vector data
	qglUniform4fv(loc, 1, vector.ToFloatPtr());
}

void idShaderGL::SetVector3Uniform(const char* name, const idVec3& vector)
{
	GLuint loc = qglGetUniformLocation(mShaderProgram, name);
	// Send the vector data
	qglUniform3fv(loc, 1, vector.ToFloatPtr());
}

void idShaderGL::SetVector2Uniform(const char* name, const idVec2& vector)
{
	GLuint loc = qglGetUniformLocation(mShaderProgram, name);
	// Send the vector data
	qglUniform2fv(loc, 1, vector.ToFloatPtr());
}

void idShaderGL::SetFloatUniform(const char* name, float value)
{
	GLuint loc = qglGetUniformLocation(mShaderProgram, name);
	// Send the float data
	qglUniform1f(loc, value);
}

void idShaderGL::SetIntUniform(const char* name, int value)
{
	GLuint loc = qglGetUniformLocation(mShaderProgram, name);
	// Send the float data
	qglUniform1i(loc, value);
}

static std::string readFile( const std::string& fileName )
{
	std::string prefix = "glsl/";
	std::string fullPath = prefix + fileName;
	void* glslBuffer = NULL;
	int glslBufferLength = fileSystem->ReadFile(fullPath.c_str(), &glslBuffer);
	std::string contents;

	if (glslBufferLength > 0)
	{
		std::istringstream shaderSteam((char*)glslBuffer);
		contents.reserve(1024);
		std::string line;
		while (!shaderSteam.eof())
		{
			std::getline(shaderSteam, line);
			if (line.find("#include") != std::string::npos)
			{
				size_t startFile = line.find_first_of('"') + 1;
				size_t endFile = line.find_last_of('"');
				std::string includeFile = line.substr(startFile, endFile - startFile);
				contents += readFile(includeFile);
			}
			else
			{
				contents += line;
				contents += '\n';
			}
		}
	}

	fileSystem->FreeFile(glslBuffer);

	return contents;
}

bool idShaderGL::CompileShader(const idStr& fileName,
				   GLenum shaderType,
				   GLuint& outShader)
{
	// Open file
	std::string contents = readFile( fileName.c_str() );
	if (!contents.empty())
	{
		const char* contentsChar = contents.c_str();
		
		// Create a shader of the specified type
		outShader = qglCreateShader(shaderType);
		// Set the source characters and try to compile
		qglShaderSource(outShader, 1, &(contentsChar), nullptr);
		qglCompileShader(outShader);
		
		if (!IsCompiled(outShader))
		{
			common->Error("%s: failed to compile shader\n", fileName.c_str());
			return false;
		}
	}
	else
	{
		common->Error("%s: file not found\n", fileName.c_str());
		return false;
	}
	
	return true;
}

bool idShaderGL::IsCompiled(GLuint shader)
{
	GLint status;
	// Query the compile status
	qglGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	
	if (status != GL_TRUE)
	{
		char buffer[512];
		memset(buffer, 0, 512);
		qglGetShaderInfoLog(shader, 511, nullptr, buffer);
		common->Warning("GLSL Compile Failed:\n%s", buffer);
		return false;
	}
	
	return true;
}

bool idShaderGL::IsValidProgram()
{
	
	GLint status;
	// Query the link status
	qglGetProgramiv(mShaderProgram, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		char buffer[512];
		memset(buffer, 0, 512);
		qglGetProgramInfoLog(mShaderProgram, 511, nullptr, buffer);
		common->Warning("GLSL Link Status:\n%s", buffer);
		return false;
	}
	
	return true;
}

void idShaderGL::SetupUniforms()
{
	uniforms.projectionMatrix.Setup(mShaderProgram, "uProjectionMatrix", mat4_identity);
	uniforms.modelviewMatrix.Setup(mShaderProgram, "uModelViewMatrix", mat4_identity);
	uniforms.textureMatrix.Setup(mShaderProgram, "uTextureMatrix", mat4_identity);

	for (int i = 0; i < NUM_PROG_ENV; i++)
	{
		uniforms.programEnv[i].Setup(mShaderProgram, va("uProgramEnv[%d]", i), vec4_zero);
	}

	for (int i = 0; i < NUM_PROG_LOCAL; i++)
	{
		uniforms.programLocal[i].Setup(mShaderProgram, va("uProgramLocal[%d]", i), vec4_zero);
	}

	uniforms.customPostValue.Setup(mShaderProgram, "uCustomPostValue", 0);
	// Don't need to do the below anymore since using 4.2 extensions
	// Setup uniform buffers
// 	GLuint matrices = qglGetUniformBlockIndex(mShaderProgram, "Matrices");
// 	if (matrices != GL_INVALID_INDEX)
// 	{
// 		qglUniformBlockBinding(mShaderProgram, matrices, 0);
// 	}
// 
// 	GLuint programUniforms = qglGetUniformBlockIndex(mShaderProgram, "ProgramUniforms");
// 	if (programUniforms != GL_INVALID_INDEX)
// 	{
// 		qglUniformBlockBinding(mShaderProgram, programUniforms, 1);
// 	}

	// Setup the texture sampler uniforms
// 	const int MAX_SAMPLERS = 16;
// 	for (int i = 0; i < MAX_SAMPLERS; i++)
// 	{
// 		GLint sampler = qglGetUniformLocation(mShaderProgram, va("uTexture%d", i));
// 		if (sampler != -1)
// 		{
// 			qglUniform1i(sampler, i);
// 		}
// 	}
}
