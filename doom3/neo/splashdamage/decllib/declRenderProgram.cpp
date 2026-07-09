// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "declRenderProgram.h"
#include "framework/DeclParseHelper.h"
#include "renderer/tr_local.h"

/*
===============================================================================

sdDeclRenderProgram

===============================================================================
*/

sdDeclRenderProgram::sdDeclRenderProgram()
	: flags(0),
	drawStateBits(0)
{
}

const char* sdDeclRenderProgram::DefaultDefinition( void ) const {
    return "{  }";
}

bool sdDeclRenderProgram::Parse( const char* text, const int textLength ) {
	idParser src;
	idToken	token;

	src.SetFlags(DECL_LEXER_FLAGS);
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );
	src.SkipUntilString("{");

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclRenderProgram::Parse: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "program" )) {
			if(!ParseShader(src)) {
				src.SkipUntilString("{");
				src.SkipBracedSection(false);
			}
			continue;
		}

		if( !token.Icmp( "state" )) {
			if(!ParseState(src)) {
				src.SkipBracedSection(false);
			}
			continue;
		}

		if( !token.Icmp( "hwSkinningVersion" )) {
			src.ReadToken(&token);
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "instanceVersion" )) {
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "alphaToCoverageVersion" )) {
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "notlitVersion" )) {
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "depthVersion" )) {
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "earlyCullVersion" )) {
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "coverageVersion" )) {
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "amblitVersion" )) {
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "ambientVersion" )) {
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "machineSpec" )) {
			src.ParseInt();
			continue;
		}

		if( !token.Icmp( "imposterBrightness" )) {
			src.ParseFloat();
			continue;
		}

		if( !token.Icmp( "fallBack" )) {
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "lodVersion" )) {
			src.ReadToken(&token);
			continue;
		}

		if( !token.Icmp( "interaction" )) {
			flags |= INTERACTION;
			continue;
		}

		if( !token.Icmp( "lowrangeuv" )) {
			flags |= LOWRANGEUV;
			continue;
		}

		src.Warning( "sdDeclRenderProgram::Parse: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}

	return true;
}

void sdDeclRenderProgram::FreeData() {
	Init();
}

sdRenderProgramShader::sdRenderProgramShader(void)
	: type(ST_INVALID),
	lang(SL_UNKNOWN)
{
}

void sdRenderProgramShader::Init(void)
{
	type = ST_INVALID;
	lang = SL_UNKNOWN;
	sourceRaw.Clear();
	placeholders.Clear();
	source.Clear();
	bindings.Clear();
	defines.Clear();
}

bool sdRenderProgramShader::IsValid(void) const
{
	return type != ST_INVALID && lang != SL_UNKNOWN && !sourceRaw.IsEmpty();
}

bool sdRenderProgramShader::Parse(idParser &src)
{
	idToken	token;

	if(!src.ReadToken(&token))
	{
		src.Warning( "sdDeclRenderProgram::ParseShader: unable parse shader language." );
		return false;
	}

	if(!token.Icmp("cg")) {
		lang = SL_CG;
	}
	else if(!token.Icmp("glsl")) {
		lang = SL_GLSL;
	}
	else if(!token.Icmp("arb")) {
		lang = SL_ARB;
	}
	else if(!token.Icmp("hlsl")) {
		lang = SL_HLSL;
	}
	else {
		src.Warning( "sdDeclRenderProgram::ParseShader: unknown shader language '%s'.", token.c_str() );
		return false;
	}

	while( 1 ) {
		if( !src.ReadToken( &token )) {
			src.Warning( "sdRenderProgramShader::Parse: unexpected end of file." );
			return false;
		}
		if (!token.Icmp("{")) {
			src.UnreadToken(&token);
			break;
		}

		if(!token.Icmp("userDecompress"));
		else
			src.Warning( "sdRenderProgramShader::Parse: unknown shader flag '%s'.", token.c_str() );
	}

	idStr text;
	src.ParseBracedSection(text, -1, true);
	text.StripTrailingWhitespace();
	text.StripLeadingOnce("{");
	text.StripTrailingOnce("}");
	text.StripTrailingWhitespace();
	text.ReplaceChar('"', ' ');
	text.StripTrailingWhitespace();
	if (!sdDeclTemplate::ExpandTemplate(sourceRaw, text.c_str(), text.Length()))
		sourceRaw = text;

	return IsValid();
}

const sdDeclRenderBinding * sdRenderProgramShader::GetBinding(const char *name) const {
	for(int i = 0; i < bindings.Num(); i++)
	{
		const sdDeclRenderBinding *binding = bindings[i];
		if(!binding)
			continue;
		if(!idStr::Icmp(binding->GetName(), name))
			return binding;
	}
	return NULL;
}

void sdRenderProgramShader::BuildSource(sdStringBuilder_Heap &buf, const char *program, const char *text, int length)
{
	idStr str;
	if(sdDeclTemplate::ExpandTemplate(str, text, length))
	{
		text = str.c_str();
		length = str.Length();
	}

	idStr path = program;
	path.Append("_shader");
	idLexer src;
	src.LoadMemory(text, length, path);
	src.SetFlags(LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_NODOLLARPRECOMPILE | LEXFL_NOFATALERRORS | LEXFL_NOERRORS);
	idToken token;

	int range_start = 0;
	int range_end = 0;
	while (1) {
		if(!src.ReadToken(&token))
			break;

		//if(token.linesCrossed && buf.Length() > 0)
		//buf.Append("\n");

		if(token == "$")
		{
			range_end = src.GetFileOffset() - token.Length();
			if(!src.ReadTokenOnLine(&token))
			{
				src.Warning("sdRenderProgramShader::ParsePost: missing placeholder name");
				continue;
			}
			if (!token.Icmp("include")) {
				idStr str;
				if(!src.ReadRestOfLine(str))
				{
					src.Warning("sdRenderProgramShader::ParsePost: missing include file name");
					continue;
				}
				if(range_start < range_end)
				{
					buf.Append(text + range_start, range_end - range_start);
					range_start = range_end;
				}
				HandleInclude(buf, program, str.c_str());
				range_start = src.GetFileOffset();
			} else  if (!token.Icmp("if") || !token.Icmp("ifdef") || !token.Icmp("ifndef") || !token.Icmp("elif") || !token.Icmp("define")) {
				if(range_start < range_end)
				{
					buf.Append(text + range_start, range_end - range_start);
					range_start = range_end;
				}
				buf.Append("#");
				//buf.Append(token.c_str());
				while(src.ReadTokenOnLine(&token))
				{
					//buf.Append(" ");
					// skip && ! || defined number
					if (token.type == TT_NAME && token.Icmp("defined")) {
						defines.AddUnique(token);
					}
					else if(token.type == TT_PUNCTUATION && token == "$") { // $ in precompile command
						src.ReadTokenOnLine(&token);
						placeholders.AddUnique(token);
					}
					//buf.Append(token.c_str());
					range_start = src.GetFileOffset();
				}
				buf.Append(text + range_end + 1, range_start - range_end - 1); // skip $
				//buf.Append("\n");
			} else if (!token.Icmp("else") || !token.Icmp("endif")) {
				if(range_start < range_end)
				{
					buf.Append(text + range_start, range_end - range_start);
					range_start = range_end;
				}
				range_start = src.GetFileOffset();
				buf.Append("#");
				buf.Append(text + range_end + 1, range_start - range_end - 1); // skip $
				//buf.Append("\n");
			} else {
				placeholders.AddUnique(token);
			}
		}
	}

	if(range_start < length)
		buf.Append(text + range_start, length - range_start);
}

void sdRenderProgramShader::PostParse(const sdDeclRenderProgram *program) {
	if(sourceRaw.IsEmpty())
		return;

	sdStringBuilder_Heap buf;
	BuildSource(buf, program->GetFileName(), sourceRaw.c_str(), sourceRaw.Length());
	source = buf.c_str();
	source.ReplaceChar('$', ' ');
	//printf("|%s|\n", source.c_str());

	const idDecl *decl;

	for(int i = 0; i < placeholders.Num(); i++) {
		decl = declManager->FindType(DECL_RENDERBINDING, placeholders[i], false);
		if( !decl ) {
			common->Warning( "sdRenderProgramShader::ParsePost: couldn't find binding '%s'.", placeholders[i].c_str() );
			bindings.Append(NULL);
		}
		else
			bindings.Append(static_cast<const sdDeclRenderBinding *>(decl));
	}

	bindings.Resize(bindings.Num());
	bindings.SetGranularity(1);
}

void sdRenderProgramShader::HandleInclude(sdStringBuilder_Heap &buf, const char *program, const char *fileName) {
	idStr name = fileName;
	name.StripLeading("\"");
	name.StripTrailing("\"");
	name.StripLeading("<");
	name.StripTrailing(">");
	char *text = NULL;
	int length = 0;
	idStr path = name;

	//1. try raw include file path
	if ((length = fileSystem->ReadFile(path, (void **)&text, NULL)) <= 0)
	{
		//2. try current prog file path
		path = program;
		path.StripFilename();
		path.AppendPath(name);
		//Sys_Printf("XXX %s|%s|\n", program, path.c_str());
		length = fileSystem->ReadFile(path, (void **)&text, NULL);
		if ((length = fileSystem->ReadFile(name, (void **)&text, NULL)) <= 0)
		{
			//3. try to find in renderprogs/
			path = "renderprogs";
			path.AppendPath(name);
			//Sys_Printf("YYY %s|%s|\n", program, path.c_str());
			length = fileSystem->ReadFile(path, (void **)&text, NULL);
		}
	}

	if (!text || length <= 0)
	{
		common->Warning("sdRenderProgramShader::HandleInclude: Could not load include file: %s", name.c_str());
		return;
	}

	BuildSource(buf, path, text, length);

	Mem_Free(text);
}

void sdDeclRenderProgram::Init(void)
{
	flags = 0;
	vertex.Init();
	fragment.Init();
	drawStateBits = 0;
}

bool sdDeclRenderProgram::ParseShader(idParser &src)
{
	idToken	token;

	if(!src.ReadToken(&token))
	{
		src.Warning( "sdDeclRenderProgram::ParseShader: unable parse shader type." );
		return false;
	}

	sdRenderProgramShader::shaderType_t type;
	if(!token.Icmp("vertex")) {
		type = sdRenderProgramShader::ST_VERTEX;
	}
	else if(!token.Icmp("fragment")) {
		type = sdRenderProgramShader::ST_FRAGMENT;
	}
	else {
		src.Warning( "sdDeclRenderProgram::ParseShader: unknown shader type '%s'.", token.c_str() );
		return false;
	}

	sdRenderProgramShader *shader;
	if(type == sdRenderProgramShader::ST_VERTEX) {
		shader = &vertex;
	}
	else {
		shader = &fragment;
	}

	src.ReadToken(&token);
	bool isRef = false;
	if(!token.Icmp("reference")) {
		if( !src.ReadToken( &token )) {
			src.Warning( "sdDeclRenderProgram::ParseShader: expect reference shader program name." );
			return false;
		}
		const idDecl *decl = declManager->FindType(DECL_RENDERPROGRAM, token, false);
		if( !decl ) {
			src.Warning( "sdDeclRenderProgram::ParseShader: could't find reference shader program '%s'.", token.c_str() );
			return false;
		}
		const sdDeclRenderProgram *program = static_cast<const sdDeclRenderProgram *>(decl);
		if(type == sdRenderProgramShader::ST_VERTEX)
			*shader = program->vertex;
		else
			*shader = program->fragment;
		shader->type = type;
		isRef = true;
	}
	else
	{
		src.UnreadToken(&token);

		shader->type = type;
		if(!shader->Parse(src)) {
			shader->Init();
			return false;
		}
	}

	if (!isRef)
		shader->PostParse(this);

	return true;
}

void sdRenderProgramShader::ExportSource(const char *path, const char *filename, const char *name, bool raw) const {
	sdStringBuilder_Heap buf;

	idStr filePath = path;
	filePath.AppendPath(name);
	const char *typeName;
	if(type == sdRenderProgramShader::ST_VERTEX)
		typeName = "vert";
	else
		typeName = "frag";
	const char *langName;
	const char *comment;
	switch(lang)
	{
		case sdRenderProgramShader::SL_CG:
			langName = "cg";
			comment = "//";
			break;
		case sdRenderProgramShader::SL_GLSL:
			langName = "glsl";
			comment = "//";
			break;
		case sdRenderProgramShader::SL_HLSL:
			langName = "hlsl";
			comment = "//";
			break;
		case sdRenderProgramShader::SL_ARB:
		default:
			langName = "arb";
			comment = "#";
			break;
	}
	filePath.Append(".");
	filePath.Append(typeName);
	filePath.Append(".");
	filePath.Append(langName);

	buf.Append(comment);
	buf.Append(" File: ");
	buf.Append(filename);
	buf.Append("\n");
	buf.Append(comment);
	buf.Append(" ID: ");
	buf.Append(name);
	buf.Append("\n");
	buf.Append("\n");

	buf.Append(comment);
	buf.Append(va(" Macros %d\n", defines.Num()));
	for (int i = 0; i < defines.Num(); ++i) {
		buf.Append(comment);
		buf.Append(" ");
		if (raw)
			buf.Append("$");
		buf.Append(defines[i]);
		buf.Append("\n");
	}
	buf.Append("\n");

	buf.Append(comment);
	buf.Append(va(" Variables %d\n", placeholders.Num()));
	for (int i = 0; i < placeholders.Num(); ++i) {
		buf.Append(comment);
		buf.Append(" ");
		if (raw)
			buf.Append("$");
		buf.Append(placeholders[i]);
		buf.Append("\n");
	}
	buf.Append("\n");

	buf.Append(comment);
	buf.Append(va(" Source\n"));
	if (raw)
		buf.Append(sourceRaw);
	else
		buf.Append(source);
	buf.Append("\n");

	fileSystem->WriteFile(filePath.c_str(), buf.c_str(), buf.Length());

	common->Printf("Export %s::%s %s %s shader to %s\n", filename, name, langName, typeName, filePath.c_str());
}

void sdDeclRenderProgram::ExportSource(const char *path, bool raw) const {
	if (vertex.IsValid())
		vertex.ExportSource(path, GetFileName(), GetName(), raw);
	if (fragment.IsValid())
		fragment.ExportSource(path, GetFileName(), GetName(), raw);
}

void sdDeclRenderProgram::ExportDeclRenderPrograms_f(const idCmdArgs &args)
{
	if (args.Argc() < 2) {
		common->Printf("Usage: %s <path> [<raw>]\n", args.Argv(0));
		return;
	}
	const char *outPath = args.Argv(1);
	bool raw = args.Argc() > 2;
	const idDecl *decl;
	const sdDeclRenderProgram *program;

	int numDecls = declManager->GetNumDecls(DECL_RENDERPROGRAM);
	common->Printf("Export %d render programs\n", numDecls);
	soundSystem->SetMute(true);

	for(int m = 0; m < numDecls; m++) {
		decl = declManager->DeclByIndex(DECL_RENDERPROGRAM, m, true);
		if (!decl)
			continue;
		common->Printf("%3d: export: %s::%s\n", m, decl->GetFileName(), decl->GetName());
		program = static_cast<const sdDeclRenderProgram *>(decl);
		program->ExportSource(outPath, raw);
	}

	soundSystem->SetMute(false);
}

bool sdRenderProgramShader::HasPostprocessTexture(void) const {
	const sdDeclRenderBinding *binding;

	for(int i = 0; i < bindings.Num(); i++) {
		binding = bindings[i];
		if(binding && binding->GetBindingType() == sdDeclRenderBinding::BT_TEXTURE && binding->GetDefaultImage() == globalImages->currentRenderImage)
			return true;
	}
	for(int i = 0; i < placeholders.Num(); i++) {
		if(!placeholders[i].Icmp("_currentRender"))
			return true;
	}
	return false;
}

bool sdDeclRenderProgram::HasPostprocess(void) const {
	return fragment.IsValid() && fragment.HasPostprocessTexture();
}

/*
===============
sdDeclRenderProgram::NameToSrcBlendMode
===============
*/
int sdDeclRenderProgram::NameToSrcBlendMode(const idStr &name)
{
	if (!name.Icmp("GL_ONE")) {
		return GLS_SRCBLEND_ONE;
	} else if (!name.Icmp("GL_ZERO")) {
		return GLS_SRCBLEND_ZERO;
	} else if (!name.Icmp("GL_DST_COLOR")) {
		return GLS_SRCBLEND_DST_COLOR;
	} else if (!name.Icmp("GL_ONE_MINUS_DST_COLOR")) {
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	} else if (!name.Icmp("GL_SRC_ALPHA")) {
		return GLS_SRCBLEND_SRC_ALPHA;
	} else if (!name.Icmp("GL_ONE_MINUS_SRC_ALPHA")) {
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	} else if (!name.Icmp("GL_DST_ALPHA")) {
		return GLS_SRCBLEND_DST_ALPHA;
	} else if (!name.Icmp("GL_ONE_MINUS_DST_ALPHA")) {
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	} else if (!name.Icmp("GL_SRC_ALPHA_SATURATE")) {
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	common->Warning("unknown src blend mode '%s' in renderProg '%s' at '%s'", name.c_str(), GetName(), GetFileName());

	return GLS_SRCBLEND_ONE;
}

/*
===============
sdDeclRenderProgram::NameToDstBlendMode
===============
*/
int sdDeclRenderProgram::NameToDstBlendMode(const idStr &name)
{
	if (!name.Icmp("GL_ONE")) {
		return GLS_DSTBLEND_ONE;
	} else if (!name.Icmp("GL_ZERO")) {
		return GLS_DSTBLEND_ZERO;
	} else if (!name.Icmp("GL_SRC_ALPHA")) {
		return GLS_DSTBLEND_SRC_ALPHA;
	} else if (!name.Icmp("GL_ONE_MINUS_SRC_ALPHA")) {
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if (!name.Icmp("GL_DST_ALPHA")) {
		return GLS_DSTBLEND_DST_ALPHA;
	} else if (!name.Icmp("GL_ONE_MINUS_DST_ALPHA")) {
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	} else if (!name.Icmp("GL_SRC_COLOR")) {
		return GLS_DSTBLEND_SRC_COLOR;
	} else if (!name.Icmp("GL_ONE_MINUS_SRC_COLOR")) {
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	common->Warning("unknown dst blend mode '%s' in renderProg '%s' at '%s'", name.c_str(), GetName(), GetFileName());

	return GLS_DSTBLEND_ONE;
}

void sdDeclRenderProgram::ParseBlend(idParser &src)
{
	idToken token;
	int		srcBlend, dstBlend;

	if (!src.ReadToken(&token)) {
		return;
	}

	// blending combinations
	if (!token.Icmp("blend")) {
		drawStateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		return;
	}

	if (!token.Icmp("add")) {
		drawStateBits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
		return;
	}

	if (!token.Icmp("filter") || !token.Icmp("modulate")) {
		drawStateBits = GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		return;
	}

	if (!token.Icmp("none")) {
		// none is used when defining an alpha mask that doesn't draw
		drawStateBits = GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE;
		return;
	}

	srcBlend = NameToSrcBlendMode(token);

	if (!src.ExpectTokenString(",")) {
		src.UnreadToken(&token);
		src.SkipRestOfLine();
		return;
	}

	if (!src.ReadToken(&token)) {
		return;
	}

	dstBlend = NameToDstBlendMode(token);

	drawStateBits = srcBlend | dstBlend;
}

void sdDeclRenderProgram::ParseDepthFunc(idParser &src)
{
	idToken t;
	src.ReadToken(&t);
	if(!idStr::Icmp(t, "equal"))
		drawStateBits |= GLS_DEPTHFUNC_EQUAL;
	else if(!idStr::Icmp(t, "always"))
		drawStateBits |= GLS_DEPTHFUNC_ALWAYS;
	else if(!idStr::Icmp(t, "lequal"))
		drawStateBits |= GLS_DEPTHFUNC_LESS;
	else if(!idStr::Icmp(t, "less"))
		drawStateBits |= GLS_DEPTHFUNC_LESS;
	else
		common->Warning("unknown depth func '%s' in renderProg '%s' at '%s'", t.c_str(), GetName(), GetFileName());
}

bool sdDeclRenderProgram::ParseState(idParser &src)
{
	idToken	token;

	while(1)
	{
		if(!src.ReadToken(&token))
		{
			src.Warning( "sdDeclRenderProgram::ParseState: unable parse state." );
			return false;
		}

		if(!token.Icmp("force"))
			continue;

		if(!token.Cmp("{"))
			break;
	}

	while (1) {
		if( !src.ReadToken( &token )) {
			src.Error( "sdDeclRenderProgram::ParseState: unexpected end of file." );
			break;
		}

		if (!token.Icmp("}")) {
			break;
		}

		if( !token.Icmp( "blend" )) {
			ParseBlend(src);
			continue;
		}

		if( !token.Icmp( "depthFunc" )) {
			ParseDepthFunc(src);
			continue;
		}

		if (!token.Icmp("maskAlpha")) {
			drawStateBits |= GLS_ALPHAMASK;
			continue;
		}

		if (!token.Icmp("maskDepth")) {
			drawStateBits |= GLS_DEPTHMASK;
			continue;
		}

		src.Warning( "sdDeclRenderProgram::ParseState: unexpected token '%s'.", token.c_str() );
		src.SkipBracedSection(false);
		break;
	}

	return true;
}

bool sdDeclRenderProgram::HasDefine(const char *macro) const
{
	return (vertex.IsValid() && vertex.HasDefine(macro)) || (fragment.IsValid() && fragment.HasDefine(macro));
}
