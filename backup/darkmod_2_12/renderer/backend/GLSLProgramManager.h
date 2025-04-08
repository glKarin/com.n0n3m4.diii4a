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

#ifndef __GLSL_PROGRAM_MANAGER_H__
#define __GLSL_PROGRAM_MANAGER_H__

class GLSLProgram;

class GLSLProgramManager {
public:
	using Generator = std::function<void( GLSLProgram* )>;

	GLSLProgramManager();
	~GLSLProgramManager();

	void Init();
	void Shutdown();

	// Load program by convention: assumes that the shader source files are named
	// <name>.fs, <name>.vs, <name>.gs and that the default attributes should be bound.
	GLSLProgram *Load( const idStr &name, const idHashMapDict &defines = {} );

	// Load program from individually specified source files. Default attributes will be bound.
	GLSLProgram *LoadFromFiles( const idStr &name, const idStr &vertexSource, const idHashMapDict &defines = {} );
	GLSLProgram *LoadFromFiles( const idStr &name, const idStr &vertexSource, const idStr &fragmentSource, const idHashMapDict &defines = {} );
	GLSLProgram *LoadFromFiles( const idStr &name, const idStr &vertexSource, const idStr &fragmentSource, const idStr &geometrySource, const idHashMapDict &defines = {} );

	// Register a GLSLProgram with a generating function.
	GLSLProgram *LoadFromGenerator( const char *name, const Generator &generator );

	GLSLProgram *Find( const char *name );

	void Reload( const char *name );
	void ReloadAllPrograms();

	// global built-in shaders
	GLSLProgram *softParticleShader;
	GLSLProgram *cubeMapShader;
	GLSLProgram *environment;
	GLSLProgram *bumpyEnvironment;
	GLSLProgram *fogShader;
	GLSLProgram *oldStageShader;
	GLSLProgram *blendShader;
	GLSLProgram *gaussianBlurShader;
	GLSLProgram *testImageCubeShader;

	GLuint uboHandle;

private:
	idList<GLSLProgram*> programs;
	int legacyTangentsCvarCallback;
};

extern GLSLProgramManager *programManager;

void R_ReloadGLSLPrograms_f( const idCmdArgs &args );

#endif
