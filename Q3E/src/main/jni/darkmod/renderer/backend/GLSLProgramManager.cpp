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
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/VertexArrayState.h"
#include "renderer/backend/GLSLUniforms.h"

GLSLProgramManager programManagerInstance;
GLSLProgramManager *programManager = &programManagerInstance;

namespace {
	void DefaultProgramInit( GLSLProgram *program, idHashMapDict defines, const char *vertexSource, const char *fragmentSource = nullptr, const char *geometrySource = nullptr ) {
		program->Init();
		if( vertexSource != nullptr ) {
			program->AttachVertexShader( vertexSource, defines );
		}
		if( fragmentSource != nullptr ) {
			program->AttachFragmentShader( fragmentSource, defines );
		}
		if( geometrySource != nullptr ) {
			program->AttachGeometryShader( geometrySource, defines );
		}
		vaState.BindAttributesToProgram( program );
		program->Link();
		program->Activate();
		int mv = program->GetUniformLocation( "u_modelViewMatrix" );
		if ( mv >= 0 )
			qglUniformMatrix4fv( mv, 1, false, mat4_identity.ToFloatPtr() );
		program->Deactivate();
	}
}


GLSLProgramManager::GLSLProgramManager() {
	// init all global program references to null
	Shutdown();
}

GLSLProgramManager::~GLSLProgramManager() {
	Shutdown();	
}

void GLSLProgramManager::Shutdown() {
	for( GLSLProgram *program : programs ) {
		delete program;
	}
	programs.ClearFree();

	renderToolsShader = nullptr;
}

GLSLProgram * GLSLProgramManager::Load( const idStr &name, const idHashMapDict &defines ) {
	Generator generator = [=]( GLSLProgram *program ) {
		if( fileSystem->FindFile( idStr(
#ifdef _GLES //karin: using glslprops/ for avoid override
                "glslprogs/"
#else
                "glprogs/"
#endif
        ) + name + ".gs" ) != FIND_NO ) {
			DefaultProgramInit( program, defines, name + ".vs", name + ".fs", name + ".gs" );
		} else {
			DefaultProgramInit( program, defines, name + ".vs", name + ".fs", nullptr );
		}
	};
	return LoadFromGenerator( name, generator );	
}

GLSLProgram * GLSLProgramManager::LoadFromFiles( const idStr &name, const idStr &vertexSource, const idHashMapDict &defines ) {
	Generator generator = [=]( GLSLProgram *program ) {
		DefaultProgramInit( program, defines, vertexSource );
	};
	return LoadFromGenerator( name, generator );
}

GLSLProgram * GLSLProgramManager::LoadFromFiles( const idStr &name, const idStr &vertexSource, const idStr &fragmentSource, const idHashMapDict &defines ) {
	Generator generator = [=]( GLSLProgram *program ) {
		DefaultProgramInit( program, defines, vertexSource, fragmentSource );
	};
	return LoadFromGenerator( name, generator );
}

GLSLProgram * GLSLProgramManager::LoadFromFiles( const idStr &name, const idStr &vertexSource, const idStr &fragmentSource, const idStr &geometrySource, const idHashMapDict &defines ) {
	Generator generator = [=]( GLSLProgram *program ) {
		DefaultProgramInit( program, defines, vertexSource, fragmentSource, geometrySource );
	};
	return LoadFromGenerator( name, generator );
}

GLSLProgram * GLSLProgramManager::LoadFromGenerator( const char *name, const Generator &generator ) {
	GLSLProgram *program = Find( name );
	if( program != nullptr ) {
		program->SetGenerator( generator );
		if( renderSystem->IsOpenGLRunning() ) {
			program->Destroy();
		}
		return program;
	} 

	program = new GLSLProgram( name, generator );
	programs.Append( program );
	return program;
}

GLSLProgram * GLSLProgramManager::Find( const char *name ) {
	for ( GLSLProgram *program : programs ) {
		if ( program->GetName() == name ) {
			return program;
		}
	}
	return nullptr;
}

void GLSLProgramManager::Reload( const char *name ) {
	GLSLProgram *program = Find( name );
	if( program != nullptr ) {
		program->Regenerate();
	}
}

void GLSLProgramManager::ReloadAllPrograms() {
	if ( uboHandle ) 
		qglDeleteBuffers( 1, &uboHandle );
	qglGenBuffers( 1, &uboHandle );
	qglBindBuffer( GL_UNIFORM_BUFFER, uboHandle );
	qglBindBufferBase( GL_UNIFORM_BUFFER, 0, uboHandle );
	for( GLSLProgram *program : programs ) {
		program->Regenerate();
	}
}

void R_ReloadGLSLPrograms_f( const idCmdArgs &args ) {
	common->Printf( "---------- R_ReloadGLSLPrograms_f -----------\n" );
	const char *programName = args.Argc() > 1 ? args.Argv( 1 ) : nullptr;
	if ( programName ) {
		programManager->Reload( programName );
	} else {
		programManager->ReloadAllPrograms();
	}
	common->Printf( "---------------------------------\n" );
}

// INITIALIZE BUILTIN PROGRAMS HERE

void GLSLProgramManager::Init() {
	renderToolsShader = LoadFromGenerator( "renderTools", []( GLSLProgram *program ) {
		DefaultProgramInit( program, {}, "renderTools.vs", "renderTools.fs" );
		program->Activate();
		GLSLUniform_sampler( program, "u_texture" ).Set( 0 );
		program->Validate();
	} );
}
