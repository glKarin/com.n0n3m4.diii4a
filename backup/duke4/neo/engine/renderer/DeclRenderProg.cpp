// DeclRenderProg.cpp
//


#pragma hdrstop

#include "RenderSystem_local.h"

void GL_SelectTextureNoClient(int unit);

int rvmDeclRenderProg::currentRenderProgram = -1;

/*
===================
rvmDeclRenderProg::Size
===================
*/
size_t rvmDeclRenderProg::Size(void) const {
	return sizeof(rvmDeclRenderProg);
}

/*
===================
rvmDeclRenderProg::SetDefaultText
===================
*/
bool rvmDeclRenderProg::SetDefaultText(void) {
	return false;
}

/*
===================
rvmDeclRenderProg::DefaultDefinition
===================
*/
const char* rvmDeclRenderProg::DefaultDefinition(void) const {
	return "";
}

/*
===================
rvmDeclRenderProg::ParseRenderParms
===================
*/
idStr rvmDeclRenderProg::ParseRenderParms(idStr& bracketText, const char* programMacro) {
#ifdef __ANDROID__ //karin: GLSL version is 320 es
	idStr uniforms = "#version 320 es\n precision highp float;\n";
#else
	idStr uniforms = "#version 140\n";
#endif
	uniforms += va("#define %s\n", programMacro);
	uniforms += globalText;
	uniforms += "\n";

	idLexer src;
	idToken	token, token2;

	src.LoadMemory(bracketText.c_str(), bracketText.Length(), GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");

	int backetId = 1;

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("{")) {
			backetId++;
		}

		if (!token.Icmp("}")) {
			backetId--;

			if (backetId == 0)
			{
				break;
			}
		}

		if (token == "$")
		{
			idToken tokenCase;
			src.ReadToken(&tokenCase);

			token = tokenCase;
			token.ToLower();
			rvmDeclRenderParam* parm = declManager->FindRenderParam(tokenCase.c_str());
			if (!parm)
			{
				src.Error("Failed to find render parm %s", token.c_str());
				return "";
			}

			if (renderParams.Find(parm) != nullptr) {
				continue;
			}

			// Make all params lower case.
			bracketText.Replace(tokenCase, token);

			idStr name = token;
			char buffer[2048];			
			strcpy(buffer, (char*)name.c_str());
			for (int i = 0; i < name.Length(); i++)
			{
				if (buffer[i] == '.')
				{
					buffer[i] = 0;
					break;
				}
			}

			if (parm->GetArraySize() == 1)
			{
				switch (parm->GetType())
				{
				case RENDERPARM_TYPE_IMAGE:
					uniforms += va("uniform sampler2D %s;\n", buffer);
					break;
				case RENDERPARM_TYPE_VEC4:
					uniforms += va("uniform vec4 %s;\n", buffer);
					break;
				case RENDERPARM_TYPE_FLOAT:
					uniforms += va("uniform float %s;\n", buffer);
					break;
				case RENDERPARM_TYPE_INT:
					uniforms += va("uniform int %s;\n", buffer);
					break;
				default:
					common->FatalError("Unknown renderparm type!");
					break;
				}
			}
			else
			{
				switch (parm->GetType())
				{
				case RENDERPARM_TYPE_IMAGE:
					uniforms += va("uniform sampler2D %s[%d];\n", buffer, parm->GetArraySize());
					break;
				case RENDERPARM_TYPE_VEC4:
					uniforms += va("uniform vec4 %s[%d];\n", buffer, parm->GetArraySize());
					break;
				case RENDERPARM_TYPE_FLOAT:
					uniforms += va("uniform float %s[%d];\n", buffer, parm->GetArraySize());
					break;
				case RENDERPARM_TYPE_INT:
					uniforms += va("uniform int %s[%d];\n", buffer, parm->GetArraySize());
					break;
				default:
					common->FatalError("Unknown renderparm type!");
					break;
				}
			}
			

			renderParams.AddUnique(parm);
		}
	}

	bracketText.Replace("$", "");

	uniforms += "\n";
	uniforms += tr.globalRenderInclude;
	uniforms += "\n";

	return uniforms;
}

/*
===================
rvmDeclRenderProg::CreateVertexShader
===================
*/
void rvmDeclRenderProg::CreateVertexShader(idStr& bracketText) {
	vertexShader = ParseRenderParms(bracketText, "ID_VERTEX_SHADER");

	vertexShader += "void main(void)\n";
	vertexShader += bracketText;
}

/*
===================
rvmDeclRenderProg::CreatePixelShader
===================
*/
void rvmDeclRenderProg::CreatePixelShader(idStr& bracketText) {
	pixelShader = ParseRenderParms(bracketText, "ID_PIXEL_SHADER");
	pixelShader += "void main(void)\n";
	pixelShader += bracketText;
}
/*
===================
rvmDeclRenderProg::LoadGLSLShader
===================
*/
int rvmDeclRenderProg::LoadGLSLShader(GLenum target, idStr& programGLSL) {
	const GLuint shader = glCreateShader(target);
	if (shader) {
		const char* source[1] = { programGLSL.c_str() };

		glShaderSource(shader, 1, source, NULL);
		printf("------------------ %s\n%s\n-------------------\n", GetName(), source[0]);
		glCompileShader(shader);

		int infologLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
		if (infologLength > 1) {
			idTempArray<char> infoLog(infologLength);
			int charsWritten = 0;
			glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog.Ptr());

			// catch the strings the ATI and Intel drivers output on success
			if (strstr(infoLog.Ptr(), "successfully compiled to run on hardware") != NULL ||
				strstr(infoLog.Ptr(), "No errors.") != NULL) {
				//common->Printf( "%s program %s from %s compiled to run on hardware\n", typeName, GetName(), GetFileName() );
			}
			else {
				common->Printf("While compiling %s program %s\n", (target == GL_FRAGMENT_SHADER) ? "fragment" : "vertex", GetName());

				const char separator = '\n';
				idList<idStr> lines;
				lines.Clear();
				idStr source(programGLSL);
				lines.Append(source);
				for (int index = 0, ofs = lines[index].Find(separator); ofs != -1; index++, ofs = lines[index].Find(separator)) {
					lines.Append(lines[index].c_str() + ofs + 1);
					lines[index].CapLength(ofs);
				}

				common->Printf("-----------------\n");
				for (int i = 0; i < lines.Num(); i++) {
					common->Printf("%3d: %s\n", i + 1, lines[i].c_str());
				}
				common->Printf("-----------------\n");

				common->Printf("%s\n", infoLog.Ptr());
			}
		}

		GLint compiled = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (compiled == GL_FALSE) {
			glDeleteShader(shader);
			return -1;
		}
	}

	return shader;
}

/*
================================================================================================
idRenderProgManager::LoadGLSLProgram
================================================================================================
*/
void rvmDeclRenderProg::LoadGLSLProgram(void) {
	GLuint vertexProgID = vertexShaderHandle;
	GLuint fragmentProgID = pixelShaderHandle;

	program = glCreateProgram();
	if (program) {
		glAttachShader(program, vertexProgID);
		glAttachShader(program, fragmentProgID);		

		glBindAttribLocation(program, PC_ATTRIB_INDEX_VERTEX, "attr_Position");
		glBindAttribLocation(program, PC_ATTRIB_INDEX_ST, "attr_TexCoord0");
		glBindAttribLocation(program, PC_ATTRIB_INDEX_TANGENT, "attr_Tangent");
		glBindAttribLocation(program, PC_ATTRIB_INDEX_BINORMAL, "attr_Bitangent");
		glBindAttribLocation(program, PC_ATTRIB_INDEX_NORMAL, "attr_Normal");
		glBindAttribLocation(program, PC_ATTRIB_INDEX_COLOR, "attr_Color");
		glBindAttribLocation(program, PC_ATTRIB_INDEX_COLOR2, "attr_Color2");

#if !defined(__ANDROID__) //karin: GLES not support glBindFragDataLocation
		glBindFragDataLocation(program, 0, "finalpixel_color");
		glBindFragDataLocation(program, 1, "finalpixel_color2");
		glBindFragDataLocation(program, 2, "finalpixel_color3");
		glBindFragDataLocation(program, 3, "finalpixel_color4");
		glBindFragDataLocation(program, 4, "finalpixel_color5");
		glBindFragDataLocation(program, 5, "finalpixel_color6");
#endif

		glLinkProgram(program);

		int infologLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength);
		if (infologLength > 1) {
			char* infoLog = (char*)malloc(infologLength);
			int charsWritten = 0;
			glGetProgramInfoLog(program, infologLength, &charsWritten, infoLog);

			// catch the strings the ATI and Intel drivers output on success
			if (strstr(infoLog, "Vertex shader(s) linked, fragment shader(s) linked.") != NULL || strstr(infoLog, "No errors.") != NULL) {
				//common->Printf( "render prog %s from %s linked\n", GetName(), GetFileName() );
			}
			else {
				common->FatalError("WHILE LINKING %s\n", infoLog);
			}

			free(infoLog);
		}
	}

	int linked = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE) {
		glDeleteProgram(program);
		idLib::Error("While linking GLSL program %s there was a internal error\n", GetName());
		return;
	}

	// get the uniform buffer binding for skinning joint matrices
	GLint blockIndex = glGetUniformBlockIndex(program, "matrices_ubo");
	if (blockIndex != -1) {
		glUniformBlockBinding(program, blockIndex, 0);
	}

	int textureUnit = 0;
	glUseProgram(program);

#ifdef __ANDROID__ //karin: debug
	for(int i = 1; i <= 6; i++)
	{
		idStr name("finalpixel_color");
		if(i > 1)
			name += i;
		GLint dataLocation = glGetFragDataLocation(program, name.c_str());
		printf("ColorAttachment %d %s %d\n", i, name.c_str(), dataLocation);
	}
#endif

	// store the uniform locations after we have linked the GLSL program
	uniformLocations.Clear();
	uniformLocationUpdateId.Clear();
	for (int i = 0; i < renderParams.Num(); i++) {
		const char* parmName = renderParams[i]->GetName();
		GLint loc = glGetUniformLocation(program, parmName);
		if (loc != -1) {
			glslUniformLocation_t uniformLocation;
			uniformLocation.parmIndex = i;
			uniformLocation.uniformIndex = loc;

			if (renderParams[i]->GetType() == RENDERPARM_TYPE_IMAGE)
			{
				glUniform1i(loc, textureUnit);
				uniformLocation.textureUnit = textureUnit;
				textureUnit++;
			}

			uniformLocations.Append(uniformLocation);
			uniformLocationUpdateId.Append(0);
		}
	}

	glUseProgram(0);
}

/*
===================
rvmDeclRenderProg::Bind
===================
*/
void rvmDeclRenderProg::Bind(void) {	
	tmu = 0;

	if (currentRenderProgram != program)
	{
		glUseProgram(program);
//#define _GLDBG 1
#ifdef __ANDROID__ //karin: auto enable vertex and texcoord attrib array
#if _GLDBG
		GLint fb;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fb);
		printf("BINDPROGRAM %s (%d)\n", GetName(), fb);
#endif
		glEnableVertexAttribArray(PC_ATTRIB_INDEX_VERTEX);
		glEnableVertexAttribArray(PC_ATTRIB_INDEX_ST);
#endif
		currentRenderProgram = program;
	}

	for (int i = 0; i < uniformLocations.Num(); i++) {
		const glslUniformLocation_t& uniformLocation = uniformLocations[i];
		rvmDeclRenderParam* parm = renderParams[uniformLocations[i].parmIndex];

		if (uniformLocationUpdateId[i] == parm->GetUpdateID())
		{
			if (parm->GetType() == RENDERPARM_TYPE_IMAGE)
			{
				tmu++;
			}	
			continue;
		}

		switch (parm->GetType())
		{
			case RENDERPARM_TYPE_IMAGE:
				if (uniformLocation.textureUnit != -1)
				{
#if _GLDBG
					printf("UUU sampler| %d = %d -> %s: %d %s\n", i, uniformLocation.uniformIndex, parm->GetName(), uniformLocation.textureUnit, parm->GetImage()->GetName());
#endif
					GL_SelectTextureNoClient(uniformLocation.textureUnit);
					parm->GetImage()->Bind();
					tmu++;
				}
				break;

			case RENDERPARM_TYPE_VEC4:
#if _GLDBG
				{
					idStr str;
					for (int m = 0; m < parm->GetArraySize(); m++) {
						str += m;
						str += ": ";
						str += parm->GetVectorValue(m).ToString(6);
						str += " | ";
					}
					printf("UUU vec4| %d = %d -> %s: %d = %s\n", i, uniformLocation.uniformIndex, parm->GetName(),parm->GetArraySize(), str.c_str());
				}
#endif
				glUniform4fv(uniformLocation.uniformIndex, parm->GetArraySize(), parm->GetVectorValuePtr());
				break;

			case RENDERPARM_TYPE_FLOAT:
#if _GLDBG
				printf("UUU float| %d = %d -> %s: %.6f\n", i, uniformLocation.uniformIndex, parm->GetName(), parm->GetFloatValue());
#endif
				glUniform1f(uniformLocation.uniformIndex, parm->GetFloatValue());
				break;

			case RENDERPARM_TYPE_INT:
#if _GLDBG
				printf("UUU int| %d = %d -> %s: %d\n", i, uniformLocation.uniformIndex, parm->GetName(), parm->GetIntValue());
#endif
				glUniform1i(uniformLocation.uniformIndex, parm->GetIntValue());
				break;
		}
		
		uniformLocationUpdateId[i] = parm->GetUpdateID();
	}
}
/*
===================
rvmDeclRenderProg::BindNull
===================
*/
void rvmDeclRenderProg::BindNull(void) {
	if (currentRenderProgram != 0)
	{
#ifdef __ANDROID__ //karin: auto disable vertex and texcoord attrib array
		glDisableVertexAttribArray(PC_ATTRIB_INDEX_VERTEX);
		glDisableVertexAttribArray(PC_ATTRIB_INDEX_ST);
#endif
		glUseProgram(0);
		currentRenderProgram = 0;
	}

	for (int i = 0; i < uniformLocations.Num(); i++) {
		const glslUniformLocation_t& uniformLocation = uniformLocations[i];
		rvmDeclRenderParam* parm = renderParams[uniformLocations[i].parmIndex];

		if (parm->GetType() == RENDERPARM_TYPE_IMAGE) {
			uniformLocationUpdateId[i] = 0;
		}
	}

	if (tmu > 1) {
		while (tmu > 1)
		{
			GL_SelectTextureNoClient(tmu);
			globalImages->BindNull();
			tmu--;
		}
	}
}

/*
===================
rvmDeclRenderProg::Parse
===================
*/
bool rvmDeclRenderProg::Parse(const char* text, const int textLength) {
	idLexer src;
	idToken	token, token2;

	tmu = 0;

	declText = text;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");

	// Check to see if we are inheritting from another shader.
	src.ReadToken(&token);
	if (token == "inherit")
	{
		src.ReadToken(&token);

		rvmDeclRenderProg* parent = tr.FindRenderProgram(token);
		
		while (1) {
			if (!src.ReadToken(&token)) {
				break;
			}

			if (!token.Icmp("}")) {
				break;
			}
			
			if (token == "define")
			{
				src.ReadToken(&token);
				globalText += va("#define %s\n", token.c_str());
			}
			else
			{
				common->FatalError("Invalid token in inheritted shader!");
			}
		}

		src.FreeSource();

		// Now we load the inheritted shader with our new variables.
		src.LoadMemory(parent->declText, parent->declText.Length(), GetFileName(), GetLineNum());
		src.SetFlags(DECL_LEXER_FLAGS);
		src.SkipUntilString("{");
	}
	else
	{
		src.UnreadToken(&token);
	}

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if (token == "vertex")
		{
			idStr bracketSection;
			src.ParseBracedSection(bracketSection);
			CreateVertexShader(bracketSection);

			vertexShaderHandle = LoadGLSLShader(GL_VERTEX_SHADER, vertexShader);
		}
		else if (token == "pixel")
		{
			idStr bracketSection;
			src.ParseBracedSection(bracketSection);
			CreatePixelShader(bracketSection);

			pixelShaderHandle = LoadGLSLShader(GL_FRAGMENT_SHADER, pixelShader);
		}
		else
		{
			src.Error("Unknown or unexpected token %s\n", token.c_str());
		}
	}

	LoadGLSLProgram();
	return true;
}

/*
===================
rvmDeclRenderProg::FreeData
===================
*/
void rvmDeclRenderProg::FreeData(void) {

}
