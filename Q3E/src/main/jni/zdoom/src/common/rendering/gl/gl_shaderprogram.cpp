/*
**  Postprocessing framework
**  Copyright (c) 2016-2020 Magnus Norddahl
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
*/

#include "gl_system.h"
#include "v_video.h"
#include "gl_interface.h"
#include "hw_cvars.h"
#include "gl_debug.h"
#include "gl_shaderprogram.h"
#include "hw_shaderpatcher.h"
#include "filesystem.h"
#include "printf.h"
#include "cmdlib.h"

namespace OpenGLRenderer
{
#ifdef _GLES //karin: control GLSL default precision
    CVAR(Int, gl_glsl_precision, 0, 0); // 0 = default(high), 1 = medium, 2 = low

FString GetGLSLPrecision()
{
	FString str = R"(

precision highp int;
precision highp float;
precision highp sampler2D;
precision highp sampler2DArray;
precision highp sampler2DMS;

)"
				  ;

	if (gl_glsl_precision == 2)
		str.Substitute("highp", "lowp");
	else if (gl_glsl_precision == 1)
		str.Substitute("highp", "mediump");

	return str;
}

CVAR(Bool, gl_glsl_dumpShader, false, 0);
void DumpGLSLShader(const char *name, const char *src)
{
	if(!gl_glsl_dumpShader)
		return;

	const char *ptr = strrchr(name, '/');
	FString path = "glsl/";
	if(ptr && ptr != name)
		path.AppendCStrPart(name, ptr - name);

	//Printf("Dump GLSL shader path: %s\n", path.GetChars());
	CreatePath(path.GetChars());

	path = "glsl/";
	path += name;
	std::unique_ptr<FileWriter> fw(FileWriter::Open(path.GetChars()));
	if (!fw)
	{
		Printf("Couldn't open file for write GLSL shader: %s -> %s\n", name, path.GetChars());
		return;
	}
	fw->Write(src, strlen(src));
	Printf("Dump GLSL shader: %s -> %s\n", name, path.GetChars());
}
#endif

bool IsShaderCacheActive();
TArray<uint8_t> LoadCachedProgramBinary(const FString &vertex, const FString &fragment, uint32_t &binaryFormat);
void SaveCachedProgramBinary(const FString &vertex, const FString &fragment, const TArray<uint8_t> &binary, uint32_t binaryFormat);

FShaderProgram::FShaderProgram()
{
	for (int i = 0; i < NumShaderTypes; i++)
		mShaders[i] = 0;
}

//==========================================================================
//
// Free shader program resources
//
//==========================================================================

FShaderProgram::~FShaderProgram()
{
	if (mProgram != 0)
		glDeleteProgram(mProgram);

	for (int i = 0; i < NumShaderTypes; i++)
	{
		if (mShaders[i] != 0)
			glDeleteShader(mShaders[i]);
	}
}

//==========================================================================
//
// Creates an OpenGL shader object for the specified type of shader
//
//==========================================================================

void FShaderProgram::CreateShader(ShaderType type)
{
	GLenum gltype = 0;
	switch (type)
	{
	default:
	case Vertex: gltype = GL_VERTEX_SHADER; break;
	case Fragment: gltype = GL_FRAGMENT_SHADER; break;
	}
	mShaders[type] = glCreateShader(gltype);
}

//==========================================================================
//
// Compiles a shader and attaches it the program object
//
//==========================================================================

void FShaderProgram::Compile(ShaderType type, const char *lumpName, const char *defines, int maxGlslVersion)
{
	int lump = fileSystem.CheckNumForFullName(lumpName);
	if (lump == -1) I_FatalError("Unable to load '%s'", lumpName);
	FString code = GetStringFromLump(lump);

	Compile(type, lumpName, code, defines, maxGlslVersion);
}

void FShaderProgram::Compile(ShaderType type, const char *name, const FString &code, const char *defines, int maxGlslVersion)
{
	mShaderNames[type] = name;
	mShaderSources[type] = PatchShader(type, code, defines, maxGlslVersion);
}

void FShaderProgram::CompileShader(ShaderType type)
{
	CreateShader(type);

	const auto &handle = mShaders[type];

	FGLDebug::LabelObject(GL_SHADER, handle, mShaderNames[type].GetChars());

	const FString &patchedCode = mShaderSources[type];
	int lengths[1] = { (int)patchedCode.Len() };
	const char *sources[1] = { patchedCode.GetChars() };
#ifdef _GLES //karin: print glsl shader name for debug
    Printf("FShaderProgram::CompileShader: %s (%s)\n", mShaderNames[type].GetChars(), type == Vertex ? "Vertex" : "Fragment");
	DumpGLSLShader(mShaderNames[type].GetChars(), sources[0]);
#endif
	glShaderSource(handle, 1, sources, lengths);

	glCompileShader(handle);

	GLint status = 0;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		I_FatalError("Compile Shader '%s':\n%s\n", mShaderNames[type].GetChars(), GetShaderInfoLog(handle).GetChars());
	}
	else
	{
		if (mProgram == 0)
			mProgram = glCreateProgram();
		glAttachShader(mProgram, handle);
	}
}

//==========================================================================
//
// Links a program with the compiled shaders
//
//==========================================================================

void FShaderProgram::Link(const char *name)
{
	FGLDebug::LabelObject(GL_PROGRAM, mProgram, name);

	uint32_t binaryFormat = 0;
	TArray<uint8_t> binary;
	if (IsShaderCacheActive())
		binary = LoadCachedProgramBinary(mShaderSources[Vertex], mShaderSources[Fragment], binaryFormat);

	bool loadedFromBinary = false;
	if (binary.Size() > 0 && glProgramBinary)
	{
		if (mProgram == 0)
			mProgram = glCreateProgram();
		glProgramBinary(mProgram, binaryFormat, binary.Data(), binary.Size());
		GLint status = 0;
		glGetProgramiv(mProgram, GL_LINK_STATUS, &status);
		loadedFromBinary = (status == GL_TRUE);
	}

	if (!loadedFromBinary)
	{
		CompileShader(Vertex);
		CompileShader(Fragment);

		glLinkProgram(mProgram);

		GLint status = 0;
		glGetProgramiv(mProgram, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			I_FatalError("Link Shader '%s':\n%s\n", name, GetProgramInfoLog(mProgram).GetChars());
		}
		else if (glProgramBinary && IsShaderCacheActive())
		{
			int binaryLength = 0;
			glGetProgramiv(mProgram, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
			binary.Resize(binaryLength);
			glGetProgramBinary(mProgram, binary.Size(), &binaryLength, &binaryFormat, binary.Data());
			binary.Resize(binaryLength);
			SaveCachedProgramBinary(mShaderSources[Vertex], mShaderSources[Fragment], binary, binaryFormat);
		}
	}

	// This is only for old OpenGL which didn't allow to set the binding from within the shader.
	if (screen->glslversion < 4.20)
	{
		glUseProgram(mProgram);
		for (auto &uni : samplerstobind)
		{
			auto index = glGetUniformLocation(mProgram, uni.first.GetChars());
			if (index >= 0)
			{
				glUniform1i(index, uni.second);
			}
		}
	}
	samplerstobind.Clear();
	samplerstobind.ShrinkToFit();
}

//==========================================================================
//
// Set uniform buffer location (only useful for GL 3.3)
//
//==========================================================================

void FShaderProgram::SetUniformBufferLocation(int index, const char *name)
{
	if (screen->glslversion < 4.20)
	{
		GLuint uniformBlockIndex = glGetUniformBlockIndex(mProgram, name);
		if (uniformBlockIndex != GL_INVALID_INDEX)
			glUniformBlockBinding(mProgram, uniformBlockIndex, index);
	}
}

//==========================================================================
//
// Makes the shader the active program
//
//==========================================================================

void FShaderProgram::Bind()
{
	glUseProgram(mProgram);
}

//==========================================================================
//
// Returns the shader info log (warnings and compile errors)
//
//==========================================================================

FString FShaderProgram::GetShaderInfoLog(GLuint handle)
{
	static char buffer[10000];
	GLsizei length = 0;
	buffer[0] = 0;
	glGetShaderInfoLog(handle, 10000, &length, buffer);
	return FString(buffer);
}

//==========================================================================
//
// Returns the program info log (warnings and compile errors)
//
//==========================================================================

FString FShaderProgram::GetProgramInfoLog(GLuint handle)
{
	static char buffer[10000];
	GLsizei length = 0;
	buffer[0] = 0;
	glGetProgramInfoLog(handle, 10000, &length, buffer);
	return FString(buffer);
}

//==========================================================================
//
// Patches a shader to be compatible with the version of OpenGL in use
//
//==========================================================================

FString FShaderProgram::PatchShader(ShaderType type, const FString &code, const char *defines, int maxGlslVersion)
{
	FString patchedCode;

	// If we have 4.2, always use it because it adds important new syntax.
	if (maxGlslVersion < 420 && gl.glslversion >= 4.2f) maxGlslVersion = 420;
	int shaderVersion = min((int)round(gl.glslversion * 10) * 10, maxGlslVersion);
#ifdef _GLES //karin: using #version 320 es on OpenGLES
    patchedCode.AppendFormat("#version 300 es\n");
#else
	patchedCode.AppendFormat("#version %d\n", shaderVersion);
#endif

	// TODO: Find some way to add extension requirements to the patching
	//
	// #extension GL_ARB_uniform_buffer_object : require
	// #extension GL_ARB_shader_storage_buffer_object : require

	if (defines)
		patchedCode << defines;

	// these settings are actually pointless but there seem to be some old ATI drivers that fail to compile the shader without setting the precision here.
#ifdef _GLES //karin: control GLSL default precision
    patchedCode << GetGLSLPrecision();
    patchedCode << R"(

#define _GLES //karin: GLES macro only for OpenGL, not Vulkan

)";
#else
	patchedCode << "precision highp int;\n";
	patchedCode << "precision highp float;\n";
#endif

	patchedCode << "#line 1\n";
	patchedCode << RemoveLayoutLocationDecl(code, type == Vertex ? "out" : "in");

	if (maxGlslVersion < 420)
	{
		// Here we must strip out all layout(binding) declarations for sampler uniforms and store them in 'samplerstobind' which can then be processed by the link function.
		patchedCode = RemoveSamplerBindings(patchedCode, samplerstobind);
	}

	return patchedCode;
}

/////////////////////////////////////////////////////////////////////////////

void FPresentShaderBase::Init(const char * vtx_shader_name, const char * program_name)
{
	FString prolog = Uniforms.CreateDeclaration("Uniforms", PresentUniforms::Desc());

	mShader.reset(new FShaderProgram());
	mShader->Compile(FShaderProgram::Vertex, "shaders/pp/screenquad.vp", prolog.GetChars(), 330);
	mShader->Compile(FShaderProgram::Fragment, vtx_shader_name, prolog.GetChars(), 330);
	mShader->Link(program_name);
	mShader->SetUniformBufferLocation(Uniforms.BindingPoint(), "Uniforms");
	Uniforms.Init();
}

void FPresentShader::Bind()
{
	if (!mShader)
	{
		Init("shaders/pp/present.fp", "shaders/pp/present");
	}
	mShader->Bind();
}

/////////////////////////////////////////////////////////////////////////////

void FPresent3DCheckerShader::Bind()
{
	if (!mShader)
	{
		Init("shaders/pp/present_checker3d.fp", "shaders/pp/presentChecker3d");
	}
	mShader->Bind();
}

void FPresent3DColumnShader::Bind()
{
	if (!mShader)
	{
		Init("shaders/pp/present_column3d.fp", "shaders/pp/presentColumn3d");
	}
	mShader->Bind();
}

void FPresent3DRowShader::Bind()
{
	if (!mShader)
	{
		Init("shaders/pp/present_row3d.fp", "shaders/pp/presentRow3d");
	}
	mShader->Bind();
}

/////////////////////////////////////////////////////////////////////////////

void FShadowMapShader::Bind()
{
	if (!mShader)
	{
		FString prolog = Uniforms.CreateDeclaration("Uniforms", ShadowMapUniforms::Desc());

		mShader.reset(new FShaderProgram());
		mShader->Compile(FShaderProgram::Vertex, "shaders/pp/screenquad.vp", "", 430);
		mShader->Compile(FShaderProgram::Fragment, "shaders/pp/shadowmap.fp", prolog.GetChars(), 430);
		mShader->Link("shaders/glsl/shadowmap");
		mShader->SetUniformBufferLocation(Uniforms.BindingPoint(), "Uniforms");
		Uniforms.Init();
	}
	mShader->Bind();
}

}