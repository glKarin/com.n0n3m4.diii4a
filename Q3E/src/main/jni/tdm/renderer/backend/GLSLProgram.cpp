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
#include "renderer/backend/GLSLProgram.h"
#include "renderer/backend/GLSLUniforms.h"
#include <memory>

#include "renderer/backend/glsl.h"
#include "StdString.h"

idCVar r_debugGLSL("r_debugGLSL", "0", CVAR_BOOL|CVAR_ARCHIVE, "If enabled, checks and warns about additional potential sources of GLSL shader errors.");

GLSLProgram *GLSLProgram::currentProgram = nullptr;

GLSLProgram::GLSLProgram( const char *name, const Generator &generator ) : name( name ), program( 0 ), generator( generator ) {}

GLSLProgram::~GLSLProgram() {
	Destroy();
}

void GLSLProgram::Regenerate() {
	Init();
	generator( this );
}

void GLSLProgram::Init() {
	if ( program != 0 )
		Destroy();

	program = qglCreateProgram();
	if( program == 0 ) {
		common->Error( "Call to glCreateProgram failed for program %s", name.c_str() );
	}
}

void GLSLProgram::Destroy() {
	if( currentProgram == this ) {
		Deactivate();
	}

	for( auto &it : uniformGroups ) {
		delete it.group;
	}
	uniformGroups.clear();

	if( program != 0 ) {
		qglDeleteProgram( program );
		program = 0;
	}
}

void GLSLProgram::AttachVertexShader( const char *sourceFile, const idHashMapDict &defines ) {
	LoadAndAttachShader( GL_VERTEX_SHADER, sourceFile, defines );
}

void GLSLProgram::AttachGeometryShader( const char *sourceFile, const idHashMapDict &defines ) {
	LoadAndAttachShader( GL_GEOMETRY_SHADER_ARB, sourceFile, defines );
}

void GLSLProgram::AttachFragmentShader( const char *sourceFile, const idHashMapDict &defines ) {
	LoadAndAttachShader( GL_FRAGMENT_SHADER, sourceFile, defines );
}

void GLSLProgram::BindAttribLocation( unsigned location, const char *attribName ) {
	qglBindAttribLocation( program, location, attribName );
}

void GLSLProgram::BindUniformBlockLocation( unsigned location, const char *blockName ) {
	GLuint blockIndex = qglGetUniformBlockIndex( program, blockName );
	if ( blockIndex != GL_INVALID_INDEX ) {
		qglUniformBlockBinding( program, blockIndex, location );
	}
}

bool GLSLProgram::Link() {
	common->Printf( "Linking GLSL program %s ...\n", name.c_str() );
	qglLinkProgram( program );
	GL_SetDebugLabel( GL_PROGRAM, program, name );

	GLint result = false;
	qglGetProgramiv( program, GL_LINK_STATUS, &result );
	if( !result || r_glDebugContext.GetBool() ) {
		// display program info log, which may contain clues to the linking error
		GLint length;
		qglGetProgramiv( program, GL_INFO_LOG_LENGTH, &length );
		if ( !result || length > 0 ) {
			auto log = std::make_unique<char[]>( length + 1 );
			qglGetProgramInfoLog( program, length, &length, log.get() );
			log[length] = 0;
			if ( length > 1 ) {	// I get to here with length == 0 on AMD =)
				common->Warning( "Linking program %s %s:\n%s\n", name.c_str(), (result ? "info" : "failed"), log.get() );
			}
		}
	}

	SetDefaultUniformBlockBindings();
	return result;
}

void GLSLProgram::Activate() {
	if ( program == 0 ) {
		Regenerate();
	}

	if( currentProgram != this ) {
		qglUseProgram( program );
		currentProgram = this;
	}
}

void GLSLProgram::Deactivate() {
	qglUseProgram( 0 );
	currentProgram = nullptr;
}

int GLSLProgram::GetUniformLocation(const char *uniformName) const {
    const int location = qglGetUniformLocation( program, uniformName );
	if( location < 0 && r_debugGLSL.GetBool() ) {
		common->Warning( "In program %s: uniform %s is unknown or unused.", name.c_str(), uniformName );
	}
	return location;
}

GLSLUniformGroup *&GLSLProgram::FindUniformGroup( const std::type_index &type ) {
	int n = (int)uniformGroups.size();
	for (int i = 0; i < n; i++)
		if (uniformGroups[i].type == type)
			return uniformGroups[i].group;
	uniformGroups.push_back(ActiveUniformGroup{type, nullptr});
	return uniformGroups[n].group;
}

bool GLSLProgram::Validate() {
	GLint result = GL_FALSE;
	qglValidateProgram( program );
	qglGetProgramiv( program, GL_VALIDATE_STATUS, &result );
	if( result != GL_TRUE ) {
		// display program info log, which may contain clues to the linking error
		GLint length;
		qglGetProgramiv( program, GL_INFO_LOG_LENGTH, &length );
		auto log = std::make_unique<char[]>( length + 1 );
		qglGetProgramInfoLog( program, length, &result, log.get() );
		log[length] = 0;
		if ( length > 1 ) {
			common->Warning( "Validation for program %s failed:\n%s\n", name.c_str(), log.get() );
		}
	}
	return result;
}

void GLSLProgram::LoadFromFiles( const char *vertexFile, const char *fragmentFile, const idHashMapDict &defines ) {
	AttachVertexShader( vertexFile, defines );
	AttachFragmentShader( fragmentFile, defines );
	Attributes::Default::Bind( this );
	Link();
}

void GLSLProgram::LoadAndAttachShader( GLint shaderType, const char *sourceFile, const idHashMapDict &defines ) {
	if( program == 0 ) {
		common->Error( "Tried to attach shader to an uninitialized program %s", name.c_str() );
	}

	GLuint shader = CompileShader( shaderType, sourceFile, defines );
	if( shader == 0) {
		common->Warning( "Failed to attach shader %s to program %s.\n", sourceFile, name.c_str() );
		return;
	}
	qglAttachShader( program, shader );
	GL_SetDebugLabel( GL_SHADER, shader, sourceFile );
	// won't actually be deleted until the program it's attached to is deleted
	qglDeleteShader( shader );
}

namespace {

	std::string ReadFile( const char *sourceFile ) {
		void *buf = nullptr;
		int len = fileSystem->ReadFile( idStr(
#ifdef __ANDROID__ //karin: using glslprops/ for avoid override
                "glslprogs/"
#else
                "glprogs/"
#endif
        ) + sourceFile, &buf );
		if( buf == nullptr ) {
			common->Warning( "Could not open shader file %s", sourceFile );
			return "";
		}
		std::string contents( static_cast< char* >( buf ), len );
		fileSystem->FreeFile( buf );

		return contents;
	}

	struct PragmaLine {
		size_t from = std::string::npos;	// [from, to) is where the whole line with
		size_t to = std::string::npos;		// #pragma is located, including EOL
		std::vector<std::string> tokens;	// set of all tokens after #pragma
	};
	/**
	 * Finds next #pragma directive in text, starting from specified position.
	 * Syntax example:
	 *   #pragma tdm_helpmeplease 1 2 3 "rty"
	 */
	PragmaLine FindNextPragmaInText(const std::string &text, size_t startFrom = 0) {
		PragmaLine result;

		size_t start = text.find("#pragma", startFrom);
		if (start == std::string::npos)
			return result;
		size_t pragmaEnd = start + 7;

		size_t end = text.find("\n", start);
		if (end == std::string::npos)
			end = text.size();
		else
			end++;

		while (start > 0 && text[start-1] != '\n')
			start--;

		std::string pragmaParams = text.substr(pragmaEnd, end - pragmaEnd);
		stdext::split(result.tokens, pragmaParams);
		result.from = start;
		result.to = end;
		return result;
	}

	/**
	 * Resolves include statements in GLSL source files.
	 * Note that the parsing is primitive and not context-sensitive. It will not respect multi-line comments
	 * or conditional preprocessor blocks, so keep includes simple in the source files!
	 * 
	 * Include directives should look like this:
	 * 
	 * #pragma tdm_include "somefile.glsl" // optional comment
	 */
	void ResolveIncludes( std::string &source, std::vector<std::string> &includedFiles ) {
		unsigned int currentFileNo = includedFiles.size() - 1;
		unsigned int totalIncludedLines = 0;

		size_t pos = 0;
		while (1) {
			auto pragma = FindNextPragmaInText( source, pos );
			if ( pragma.from == pragma.to )
				break;
			if ( pragma.tokens[0] != "tdm_include" ) {
				pos = pragma.to;
				continue;
			}

			std::string fileToInclude( pragma.tokens[1] );
			fileToInclude = fileToInclude.substr( 1, fileToInclude.size() - 2 );

			std::string replacement;
			if( std::find( includedFiles.begin(), includedFiles.end(), fileToInclude ) == includedFiles.end() ) {
				int nextFileNo = includedFiles.size();
				std::string includeContents = ReadFile( fileToInclude.c_str() );
				includedFiles.push_back( fileToInclude );
				ResolveIncludes( includeContents, includedFiles );

				// also add a #line instruction at beginning and end of include so that
				// compile errors are mapped to the correct file and line
				// unfortunately, #line does not take an actual filename, but only an integral reference to a file :(
				unsigned int currentLine = std::count( source.cbegin(), source.cbegin() + pragma.from, '\n' ) + 1 - totalIncludedLines;
				std::string includeBeginMarker = "#line 0 " + std::to_string( nextFileNo ) + '\n';
				std::string includeEndMarker = "\n#line " + std::to_string( currentLine ) + ' ' + std::to_string( currentFileNo );
				totalIncludedLines += std::count( includeContents.begin(), includeContents.end(), '\n' ) + 2;

				// replace include statement with content of included file
				replacement = includeBeginMarker + includeContents + includeEndMarker + "\n";
			} else {
				replacement = "// already included " + fileToInclude + "\n";
			}

			source.replace( source.begin() + pragma.from, source.begin() + pragma.to, replacement );
			pos = pragma.from;
		}
	}

	/**
	 * Resolves dynamic defines statements in GLSL source files.
	 * Note that the parsing is primitive and not context-sensitive. It will not respect multi-line comments
	 * or conditional preprocessor blocks!
	 * 
	 * Define directives should look like this:
	 * 
	 * #pragma tdm_define "DEF_NAME" // optional comment
	 * 
	 * If DEF_NAME is contained in defines, the line will be replaced by
	 * #define DEF_NAME <value>
	 * 
	 * Otherwise, it will be commented out.
	 */
	void ResolveDefines( std::string &source, const idHashMapDict &defines ) {
		size_t pos = 0;
		while (1) {
			auto pragma = FindNextPragmaInText( source, pos );
			if ( pragma.from == pragma.to )
				break;
			if ( pragma.tokens[0] != "tdm_define" ) {
				pos = pragma.to;
				continue;
			}

			std::string define( pragma.tokens[1] );
			assert( define.length() >= 2 && define.front() == '"' && define.back() == '"' );
			define = define.substr( 1, define.size() - 2 );

			std::string replacement;
			auto defIt = defines.Find( define.c_str() );
			if( defIt != nullptr ) {
				replacement = "#define " + define + " " + defIt->value.c_str() + "\n";
			} else {
				replacement = "// #undef " + define + "\n";
			}

			source.replace( source.begin() + pragma.from, source.begin() + pragma.to, replacement );
			pos = pragma.from;
		}
	}
}

GLuint GLSLProgram::CompileShader( GLint shaderType, const char *sourceFile, const idHashMapDict &defines ) {
	std::string source = ReadFile( sourceFile );
	if( source.empty() ) {
		return 0;
	}

	std::vector<std::string> sourceFiles { sourceFile };
	ResolveIncludes( source, sourceFiles );
	idHashMapDict definesPlus( defines );
	if ( shaderType == GL_VERTEX_SHADER )
		definesPlus.Set("VERTEX_SHADER", "1");
	if ( shaderType == GL_FRAGMENT_SHADER )
		definesPlus.Set("FRAGMENT_SHADER", "1");
	if ( shaderType == GL_GEOMETRY_SHADER )
		definesPlus.Set("GEOMETRY_SHADER", "1");
	ResolveDefines( source, definesPlus );
	//ResolveDefines( source, defines);

	GLuint shader = qglCreateShader( shaderType );
	GLint length = source.size();
	const char *sourcePtr = source.c_str();
	qglShaderSource( shader, 1, &sourcePtr, &length );
	qglCompileShader( shader );

	// check if compilation was successful
	GLint result;
	qglGetShaderiv( shader, GL_COMPILE_STATUS, &result );
	if( !result || r_glDebugContext.GetBool() ) {
		// display the shader info log, which contains compile errors
		int length;
		qglGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );
		if ( !result || length > 0 ) {
			auto log = std::make_unique<char[]>( length );
			qglGetShaderInfoLog( shader, length, &length, log.get() );
			log[length] = 0;
			if ( length > 1 ) {
				std::stringstream ss;
				ss << "Compiling shader file " << sourceFile << " " << (result ? "info" : "failed") << ":\n" << log.get() << "\n\n";
				// unfortunately, GLSL compilers don't reference any actual source files in their errors, but only
				// file index numbers. So we'll display a short legend which index corresponds to which file.
				ss << "File indexes:\n";
				for( size_t i = 0; i < sourceFiles.size(); ++i ) {
					ss << "  " << i << " - " << sourceFiles[i] << "\n";
				}
				common->Warning( "%s", ss.str().c_str() );
			}
		}

		if ( !result ) {
			qglDeleteShader( shader );
			return 0;
		}
	}

	return shader;
}

void GLSLProgram::SetDefaultUniformBlockBindings() {
	BindUniformBlockLocation( 0, "block" );
	BindUniformBlockLocation( 0, "ViewParamsBlock" );
	BindUniformBlockLocation( 1, "PerDrawCallParamsBlock" );
}


/// UNIT TESTS FOR SHADER INCLUDES AND DEFINES

#include "../tests/testing.h"

namespace {
	const std::string BASIC_SHADER =
#ifdef __ANDROID__ //karin: OpenGLES3.2 on Android for geometry shader
        "#version 320 es\n"
#else
        "#version 150\n"
#endif
		"void main() {}";
	const std::string SHARED_COMMON =
		"uniform vec4 someParam;\n"
		"\n"
		"vec4 doSomething {\n"
		"  return someParam * 2;\n"
		"}\n";
	const std::string INCLUDE_SHADER =
#ifdef __ANDROID__ //karin: OpenGLES3.2 on Android for geometry shader
        "#version 320 es\n"
#else
        "#version 140\n"
#endif
		"#pragma tdm_include \"tests/shared_common.glsl\"\r\n"
		"void main() {}\n";

	const std::string NESTED_INCLUDE =
		"#pragma tdm_include \"tests/shared_common.glsl\"\n"
		"float myFunc() {\n"
		"  return 0.3;\n"
		"}";

	const std::string ADVANCED_INCLUDES =
#ifdef __ANDROID__ //karin: OpenGLES3.2 on Android for geometry shader
        "#version 320 es\n"
#else
        "#version 330\n"
#endif
		"\n"
		" #pragma tdm_include \"tests/nested_include.glsl\"\n"
		"#pragma  tdm_include \"tests/shared_common.glsl\"  // ignore this comment\n"
		"#pragma tdm_include \"tests/advanced_includes.glsl\"\n"
		"void main() {\n"
		"  float myVar = myFunc();\n"
		"}\n"
		"#pragma tdm_include \"tests/basic_shader.glsl\"\n";

	const std::string EXPANDED_INCLUDE_SHADER =
#ifdef __ANDROID__ //karin: OpenGLES3.2 on Android for geometry shader
        "#version 320 es\n"
#else
        "#version 140\n"
#endif
		"#line 0 1\n"
		"uniform vec4 someParam;\n"
		"\n"
		"vec4 doSomething {\n"
		"  return someParam * 2;\n"
		"}\n"
		"\n#line 2 0\n"
		"void main() {}\n";
	const std::string EXPANDED_ADVANCED_INCLUDES =
#ifdef __ANDROID__ //karin: OpenGLES3.2 on Android for geometry shader
		"#version 320 es\n"
#else
        "#version 330\n"
#endif
		"\n"
		"#line 0 1\n"
		"#line 0 2\n"
		"uniform vec4 someParam;\n"
		"\n"
		"vec4 doSomething {\n"
		"  return someParam * 2;\n"
		"}\n"
		"\n#line 1 1\n"
		"float myFunc() {\n"
		"  return 0.3;\n"
		"}"
		"\n#line 3 0\n"
		"// already included tests/shared_common.glsl\n"
		"// already included tests/advanced_includes.glsl\n"
		"void main() {\n"
		"  float myVar = myFunc();\n"
		"}\n"
		"#line 0 3\n"
#ifdef __ANDROID__ //karin: OpenGLES3.2 on Android for geometry shader
        "#version 320 es\n"
#else
        "#version 150\n"
#endif
		"void main() {}"
		"\n#line 9 0\n";

	std::string LoadSource( const std::string &sourceFile ) {
		std::vector<std::string> includedFiles { sourceFile };
		std::string source = ReadFile( sourceFile.c_str() );
		ResolveIncludes( source, includedFiles );
		return source;
	}

	TEST_CASE("Shader include handling") {
		INFO( "Preparing test shaders" );
#ifdef __ANDROID__ //karin: using glslprops/ for avoid override
#define TEST_GLPROGS_DIR "glslprogs"
#else
#define TEST_GLPROGS_DIR "glprogs"
#endif
		fileSystem->WriteFile( TEST_GLPROGS_DIR "/tests/basic_shader.glsl", BASIC_SHADER.c_str(), BASIC_SHADER.size(), "fs_savepath", "" );
		fileSystem->WriteFile( TEST_GLPROGS_DIR "/tests/shared_common.glsl", SHARED_COMMON.c_str(), SHARED_COMMON.size(), "fs_savepath", "" );
		fileSystem->WriteFile( TEST_GLPROGS_DIR "/tests/include_shader.glsl", INCLUDE_SHADER.c_str(), INCLUDE_SHADER.size(), "fs_savepath", "" );
		fileSystem->WriteFile( TEST_GLPROGS_DIR "/tests/nested_include.glsl", NESTED_INCLUDE.c_str(), NESTED_INCLUDE.size(), "fs_savepath", "" );
		fileSystem->WriteFile( TEST_GLPROGS_DIR "/tests/advanced_includes.glsl", ADVANCED_INCLUDES.c_str(), ADVANCED_INCLUDES.size(), "fs_savepath", "" );

		SUBCASE( "Basic shader without includes remains unaltered" ) {
			REQUIRE( LoadSource( "tests/basic_shader.glsl" ) == BASIC_SHADER );
		}

		SUBCASE( "Simple include works" ) {
			REQUIRE( LoadSource( "tests/include_shader.glsl" ) == EXPANDED_INCLUDE_SHADER );
		}

		SUBCASE( "Multiple and nested includes" ) {
			REQUIRE( LoadSource( "tests/advanced_includes.glsl" ) == EXPANDED_ADVANCED_INCLUDES );
		}

		INFO( "Cleaning up" );
		fileSystem->RemoveFile( "tests/basic_shader.glsl", "" );
		fileSystem->RemoveFile( "tests/shared_common.glsl", "" );
		fileSystem->RemoveFile( "tests/include_shader.glsl", "" );
		fileSystem->RemoveFile( "tests/nested_include.glsl", "" );
		fileSystem->RemoveFile( "tests/advanced_includes.glsl", "" );
	}

	TEST_CASE("Shader defines handling") {
		const std::string shaderWithDynamicDefines =
#ifdef __ANDROID__ //karin: OpenGLES3.2 on Android for geometry shader
			"#version 320 es\n"
#else
            "#version 140\n"
#endif
			"#pragma tdm_define \"FIRST_DEFINE\"\n"
			"\n"
			"  #pragma   tdm_define   \"SECOND_DEFINE\"\n"
			"void main() {\n"
			"#ifdef FIRST_DEFINE\n"
			"  return;\n"
			"#endif\n"
			"}\n" ;

		const std::string expectedResult =
#ifdef __ANDROID__ //karin: OpenGLES3.2 on Android for geometry shader
			"#version 320 es\n"
#else
            "#version 140\n"
#endif
			"#define FIRST_DEFINE 1\n"
			"\n"
			"// #undef SECOND_DEFINE\n"
			"void main() {\n"
			"#ifdef FIRST_DEFINE\n"
			"  return;\n"
			"#endif\n"
			"}\n" ;

		std::string source = shaderWithDynamicDefines;
		idHashMapDict defines;
		defines.Set( "FIRST_DEFINE", "1" );
		ResolveDefines( source, defines );
		REQUIRE( source == expectedResult );
	}
}
