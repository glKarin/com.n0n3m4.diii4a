#ifndef _KARIN_GLSL_ARB_SHADER_H
#define _KARIN_GLSL_ARB_SHADER_H

class idARBProgram;
class idARBTokenList : public idStrList
{
public:
    idStr ToSource(void);

    int AddToken(const char *token) {
        if(token && token[0])
            return Append(token);
        else
            return Num() - 1;
    }
    void AddEOL(void) {
        AddToken("\n");
    }
    void AddEnding(void) {
        AddToken(";");
    }
    void RemoveLast(idStr *ret = NULL) {
        if(ret && Num() > 0)
            *ret = operator[](Num() - 1);
        RemoveIndex(Num() - 1);
    }
};

class idARBShader
{
public:
    explicit idARBShader(void);
    void Write(const char *path, const char *name);
    void Print(void);
    idStr ToSource(void);

    int AddToken(const char *token) {
        return tokenList.AddToken(token);
    }
    void AddEOL(void) {
        AddToken("\n");
    }
    void AddEnding(void) {
        AddToken(";");
    }
    void RemoveLast(idStr *ret = NULL) {
        return tokenList.RemoveLast(ret);
    }
    int CurrentIndex(void) {
        return tokenList.Num() - 1;
    }
    idStr & operator[](int i) {
        return tokenList[i];
    }
    bool HasToken(void) const {
        return tokenList.Num() > 0;
    }

    void AddUniform(const char *name);
    void AddIn(const char *name);
    void AddOut(const char *name);

private:
    int version;
    idARBTokenList tokenList;
    int type;

    idStrList uniformList;
    idStrList inList;
    idStrList outList;

    friend class idARBProgram;
};

class idARBProgram
{
public:
    explicit idARBProgram(int version);
    bool ParseFile(const char *file);
    bool Parse(const char *source, int length);
    void Write(const char *path, const char *name);
    void Print(void);

private:
    void ParseValue(void);
    void ADD(void);
    void SUB(void);
    void MUL(void);
    void ParseCMD(void);
    void Command(const char *cmd);
    void ParseComment(void);
    void ParseOption(void);
    void ParseType(void);
    void ParseUnknown(void);
    void ParseProgram(void);
    void ParseVertex(void);
	void ParseVec4(void);
    void ParseFragment(void);
    void ParseResult(void);
    void ParseSampler(void);
    void ParseState(void);
    void TEMP(void);
    void MOV(void);
    void PARAM(void);
    void RCP(void);
    void RSQ(void);
    void MAD(void);
    void ParseFloat(void);
    idStr ReadFloat(void);
    void TXP(void);
    void TEX(void);
    void END(void);
    void KIL(void);
    void DP3(void);
    void DP4(void);
    void MIN(void);
    void MAX(void);
    void LRP(void);
    void POW(void);
    void MAD_SAT(void);
    void ADD_SAT(void);
    void DP3_SAT(void);
    void DP4_SAT(void);
    void MUL_SAT(void);
    void OUTPUT(void);
    void ALIAS(void);
    void ATTRIB(void);
    void ADDRESS(void);
    void _SAT_Start(void);
    void _SAT_End(void);
    void Dot(const char *t);
    void Dot_SAT(const char *t);
    void ParseTexture(const char *cmd, const char *type = "");

    int AddToken(const char *token) {
        return Shader()->AddToken(token);
    }
    void AddEOL(void) {
        Shader()->AddEOL();
    }
    void AddEnding(void) {
        Shader()->AddEnding();
    }
    void RemoveLast(idStr *ret = NULL) {
        Shader()->RemoveLast(ret);
    }
    int CurrentIndex(void) {
        return Shader()->CurrentIndex();
    }
    idStr & operator[](int i) {
        return Shader()->operator[](i);
    }

    void AddUniform(const char *name, const char *type = "vec4");
    void AddIn(const char *name, const char *type = "vec4");
    void AddOut(const char *name, const char *type = "vec4");

    void HandleError(const char *error);
    void ExpectToken(const char *token);
    bool ExpectTokenString(const char *token);
    idStr TextureFunc(const char *type, const char *d);
    idARBShader *Shader(void);

private:
    int version;
    idARBShader vertexShader;
    idARBShader fragmentShader;
    idARBShader *shader;
    int type;

    idLexer parser;
};

idARBShader::idARBShader(void)
        : type(0)
{

}

void idARBShader::AddUniform(const char *name)
{
    uniformList.AddUnique(name);
}

void idARBShader::AddIn(const char *name)
{
    inList.AddUnique(name);
}

void idARBShader::AddOut(const char *name)
{
    outList.AddUnique(name);
}



idARBProgram::idARBProgram(int version)
: version(version),
    type(0),
  shader(NULL)
{
    vertexShader.type = 1;
    vertexShader.version = version;
    fragmentShader.type = 2;
    fragmentShader.version = version;
}

void idARBProgram::Command(const char *cmd)
{
#define ARB_HANDLE_CMD(x) \
    if(!idStr::Icmp(cmd, #x)) { \
        x(); \
    }
    ARB_HANDLE_CMD(ADD)
    else ARB_HANDLE_CMD(SUB)
    else ARB_HANDLE_CMD(MUL)
    else ARB_HANDLE_CMD(MOV)
    else ARB_HANDLE_CMD(RCP)
    else ARB_HANDLE_CMD(RSQ)
    else ARB_HANDLE_CMD(MAD)
    else ARB_HANDLE_CMD(TXP)
    else ARB_HANDLE_CMD(TEX)
    else ARB_HANDLE_CMD(TEMP)
    else ARB_HANDLE_CMD(PARAM)
    else ARB_HANDLE_CMD(MAD_SAT)
    else ARB_HANDLE_CMD(END)
    else ARB_HANDLE_CMD(MIN)
    else ARB_HANDLE_CMD(MAX)
    else ARB_HANDLE_CMD(ADD_SAT)
    else ARB_HANDLE_CMD(DP3)
    else ARB_HANDLE_CMD(DP4)
    else ARB_HANDLE_CMD(LRP)
    else ARB_HANDLE_CMD(POW)
    else ARB_HANDLE_CMD(DP3_SAT)
    else ARB_HANDLE_CMD(DP4_SAT)
    else ARB_HANDLE_CMD(MUL_SAT)
    else ARB_HANDLE_CMD(OUTPUT)
    else ARB_HANDLE_CMD(ATTRIB)
    else ARB_HANDLE_CMD(ADDRESS)
    else
    {
        ParseUnknown();
    }
}

void idARBProgram::ParseCMD(void)
{
    idToken token;

    while(!parser.EndOfFile())
    {
        if(!parser.ReadToken(&token))
            break;
        //printf("xxx %s\n", token.c_str());
        parser.UnreadToken(&token);

        if(!token.Icmpn("#", 1))
        {
            ParseComment();
			if(!shader)
				continue;
        }
        else if(!token.Icmp("OPTION"))
        {
            ParseOption();
        }
        else if(!token.Icmp("!"))
        {
            ParseType();
        }
        else
        {
            Command(token.c_str());
        }

        AddEOL();
    }
}

// ADD T, A, B -> T = A + B
void idARBProgram::ADD(void)
{
    ExpectTokenString("ADD");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("+");
    ParseValue();
    AddEnding();
    parser.ExpectTokenString(";");
}

// SUB T, A, B -> T = A - B
void idARBProgram::SUB(void)
{
    ExpectTokenString("SUB");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("-");
    ParseValue();
    AddEnding();
    parser.ExpectTokenString(";");
}

// MUL T, A, B -> T = A * B
void idARBProgram::MUL(void)
{
    ExpectTokenString("MUL");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("*");
    ParseValue();
    AddEnding();
    parser.ExpectTokenString(";");
}

// MAD T, A, B, C -> T = A * B + C
void idARBProgram::MAD(void)
{
    ExpectTokenString("MAD");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("*");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("+");
    ParseValue();
    AddEnding();
    parser.ExpectTokenString(";");
}

// MAD_SAT T, A, B, C -> T = clamp(A * B + C, 0.0, 1.0)
void idARBProgram::MAD_SAT(void)
{
    ExpectTokenString("MAD_SAT");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    _SAT_Start();
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("*");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("+");
    ParseValue();
    _SAT_End();
    parser.ExpectTokenString(";");
}

// ADD_SAT T, A, B -> T = clamp(A + B, 0.0, 1.0)
void idARBProgram::ADD_SAT(void)
{
    ExpectTokenString("ADD_SAT");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    _SAT_Start();
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("+");
    ParseValue();
    _SAT_End();
    parser.ExpectTokenString(";");
}

// MUL_SAT T, A, B -> T = clamp(A * B, 0.0, 1.0)
void idARBProgram::MUL_SAT(void)
{
    ExpectTokenString("MUL_SAT");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    _SAT_Start();
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("*");
    ParseValue();
    _SAT_End();
    parser.ExpectTokenString(";");
}

// DP3_SAT T, A, B -> T = clamp(dot(A, B), 0.0, 1.0)
void idARBProgram::DP3_SAT(void)
{
	Dot_SAT("DP3_SAT");
}

// DP4_SAT T, A, B -> T = clamp(dot(A, B), 0.0, 1.0)
void idARBProgram::DP4_SAT(void)
{
	Dot_SAT("DP4_SAT");
}

void idARBProgram::Dot_SAT(const char *t)
{
    ExpectTokenString(t);

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    _SAT_Start();
    AddToken("dot");
    AddToken("(");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken(",");
    ParseValue();
    _SAT_End();
    parser.ExpectTokenString(";");
}

void idARBProgram::_SAT_Start(void)
{
    AddToken("clamp");
    AddToken("(");
}

void idARBProgram::_SAT_End(void)
{
    AddToken(",");
    AddToken("0.0");
    AddToken(",");
    AddToken("1.0");
    AddToken(")");
    AddEnding();
}

// TXP T, TexCoord, Sampler, 2D/CUBE -> T = textureProj(Sampler, TexCoord)
void idARBProgram::TXP(void)
{
    ParseTexture("TXP", "Proj");
}

// TEX T, TexCoord, Sampler, 2D/CUBE -> T = texture(Sampler, TexCoord)
void idARBProgram::TEX(void)
{
    ParseTexture("TEX");
}

// END
void idARBProgram::END(void)
{
    ExpectTokenString("END");
}

void idARBProgram::ParseTexture(const char *cmd, const char *type)
{
    ExpectTokenString(cmd);

    idStr token;
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    int i = AddToken("texture");
    AddToken("(");
    ParseValue();
    idStr st;
    RemoveLast(&st);
    parser.ExpectTokenString(",");
    ParseValue();
    AddToken(",");
    parser.ExpectTokenString(",");
    parser.ReadRestOfLine(token);
    token.StripTrailing(';');
    AddToken(st);
    AddToken(")");
    AddEnding();
    operator[](i) = TextureFunc(type, token);
}

void idARBProgram::ParseComment(void)
{
	idToken token;
    if(!parser.ReadToken(&token))
	{
		parser.Error("Expect '#'");
		return;
	}
	if(token.Icmpn("#", 1))
	{
		parser.Error("Expect '#', but found '%s'", token.c_str());
		return;
	}

	if(!shader)
	{
		parser.SkipRestOfLine();
		return;
	}

    AddToken("//");
    idStr comment;
    parser.ReadRestOfLine(comment);
    if(!comment.IsEmpty())
        AddToken(comment);
}

void idARBProgram::ParseOption(void)
{
    ExpectTokenString("OPTION");

	idStr str;
    parser.ReadRestOfLine(str);
	AddToken("//");
	AddToken(str);
}

void idARBProgram::ParseType(void)
{
    ExpectTokenString("!");
    parser.ExpectTokenString("!");

    idToken typeStr;
    parser.ExpectAnyToken(&typeStr);
    if(!typeStr.Icmpn("ARBvp", 5))
    {
        type = 1;
        shader = &vertexShader;
    }
    else if(!typeStr.Icmpn("ARBfp", 5))
    {
        type = 2;
        shader = &fragmentShader;
    }
    else
    {
        parser.Error("Unexpect ARB type '%s'", typeStr.c_str());
        return;
    }
	AddToken("//");
	AddToken(typeStr);
    float ver = parser.ParseFloat();
	AddToken(va("%f", ver));
}

void idARBProgram::ParseUnknown(void)
{
    idToken token;
    parser.ReadToken(&token);
    idStr str;
    parser.ReadRestOfLine(str);
    idStr line = token;
    line.Append(" ");
    line.Append(str);
    common->Warning("Unknown command line: '%s'", line.c_str());
    AddToken(line);
}


void idARBProgram::ADDRESS(void)
{
    idToken token;
    parser.ReadToken(&token);
    idStr str;
    parser.ReadRestOfLine(str);
    idStr line = token;
    line.Append(" ");
    line.Append(str);
    AddToken(line);
}

void idARBProgram::ParseVec4(void)
{
	parser.ExpectTokenString("{");
	idToken token;
	idStr name;
	name.Append("vec4");
	name.Append("(");
    while(parser.ReadTokenOnLine(&token))
    {
        if(!token.Cmp("}"))
            break;
        if(!token.Cmp(","))
            name.Append(",");
        else
        {
            parser.UnreadToken(&token);
            name.Append(ReadFloat());
        }
    }
	name.Append(")");
	AddToken(name);
}

void idARBProgram::ParseValue(void)
{
    idToken token;
    idStr str;
	bool neg = false;

	while(true)
	{
		parser.ReadTokenOnLine(&token);
		if(!token.Icmp("program"))
		{
			parser.UnreadToken(&token);
			ParseProgram();
			RemoveLast(&str);
		}
		else if(!token.Icmp("vertex"))
		{
			parser.UnreadToken(&token);
			ParseVertex();
			RemoveLast(&str);
		}
		else if(!token.Icmp("fragment"))
		{
			parser.UnreadToken(&token);
			ParseFragment();
			RemoveLast(&str);
		}
		else if(!token.Icmp("result"))
		{
			parser.UnreadToken(&token);
			ParseResult();
			RemoveLast(&str);
		}
		else if(!token.Icmp("state"))
		{
			parser.UnreadToken(&token);
			ParseState();
			RemoveLast(&str);
		}
		else if(!token.Icmp("texture"))
		{
			parser.UnreadToken(&token);
			ParseSampler();
			RemoveLast(&str);
		}
		else if(!token.Cmp("{"))
		{
			parser.UnreadToken(&token);
			ParseVec4();
			RemoveLast(&str);
		}
		else if(!token.Cmp("-"))
		{
			neg = true;
			continue;
		}
		else
		{
			str.Append(token);
			if(parser.ReadTokenOnLine(&token))
			{
				if(!token.Cmp("."))
				{
					str.Append(".");
					if(parser.ReadTokenOnLine(&token))
					{
						str.Append(token);
					}
					else
						parser.Error("Require xyzw components\n");
				}
				else
					parser.UnreadToken(&token);
			}
			else
				parser.UnreadToken(&token);
		}

		break;
	}

	if(neg)
		str.Insert("-", 0);
    AddToken(str);
}

bool idARBProgram::ParseFile(const char *file)
{
    void *buffer;
    int len;

    if((len = fileSystem->ReadFile(file, &buffer, NULL)) > 0)
    {
        bool res = Parse((const char *)buffer, len);
		fileSystem->FreeFile(buffer);
		return res;
    }
    else
    {
        common->Warning("ARB shader file '%s' read fail.", file);
        return false;
    }
}

bool idARBProgram::Parse(const char *source, int length)
{
    parser.SetFlags(LEXFL_ALLOWFLOATEXCEPTIONS | LEXFL_NOBASEINCLUDES);
	idStr text;
	text.Append(source, length);
	text.Replace("..", " \"..\" ");
    if(!parser.LoadMemory(text, text.Length(), "<implicit file>"))
    {
        common->Warning("ARB shader file parse fail.");
        return false;
    }

    try
    {
        ParseCMD();
    }
    catch (const idException &e)
    {
        HandleError(e.error);
        return false;
    }

    return true;
}

void idARBProgram::Write(const char *path, const char *name)
{
    if(vertexShader.HasToken())
        vertexShader.Write(path, name);
    if(fragmentShader.HasToken())
        fragmentShader.Write(path, name);
}

void idARBProgram::Print(void)
{
    if(vertexShader.HasToken())
        vertexShader.Print();
    if(fragmentShader.HasToken())
        fragmentShader.Print();
}

idStr idARBTokenList::ToSource(void)
{
    idStr source;
    for(int i = 0; i < Num(); i++)
    {
        const idStr &token = operator[](i);
        source.Append(token);
        if(token.Cmp("\n") && token.Cmp(";") && (i < Num() - 1 && operator[](i + 1).Cmp(";")))
            source.Append(" ");
    }
    return source;
}

idStr idARBShader::ToSource(void)
{
    idStr source;
    idARBTokenList list;

    list.AddToken("#version");
    list.AddToken(va("%d es", version));
    list.AddEOL();
    list.AddEOL();
    list.AddToken("precision");
    list.AddToken("mediump");
    list.AddToken("float");
    list.AddEnding();
    list.AddEOL();
    list.AddEOL();

    const char *attribute;
    const char *in;
    const char *out;

    if(version < 300)
    {
        attribute = "attribute";
        in = "varying";
        out = "varying";
    }
    else
    {
        attribute = "in";
        in = "in";
        out = "out";
    }

    for(int i = 0; i < uniformList.Num(); i++)
    {
        list.AddToken("uniform");
        list.AddToken(uniformList[i]);
        list.AddEnding();
        list.AddEOL();
    }
    if(uniformList.Num() > 0)
        list.AddEOL();

    for(int i = 0; i < inList.Num(); i++)
    {
        if(type == 1)
            list.AddToken(attribute);
        else
            list.AddToken(in);

        list.AddToken(inList[i]);
        list.AddEnding();
        list.AddEOL();
    }
    if(inList.Num() > 0)
        list.AddEOL();

    //if(type == 1)
    {
        for(int i = 0; i < outList.Num(); i++)
        {
            list.AddToken(out);
            list.AddToken(outList[i]);
            list.AddEnding();
            list.AddEOL();
        }
        if(outList.Num() > 0)
            list.AddEOL();
    }

    list.AddToken("void");
    list.AddToken("main");
    list.AddToken("(");
    list.AddToken("void");
    list.AddToken(")");
    list.AddToken("{");
    list.AddEOL();

    list.Append(tokenList);
    list.AddToken("}");
    list.AddEOL();

    return list.ToSource();
}

void idARBShader::Print(void)
{
    idStr source = ToSource();

    if(type == 1)
		common->Printf("====== Vertex shader: ======\n");
    else
		common->Printf("===== Fragment shader: =====\n");

	for(int i = 0; i < source.Length(); i += 1024)
	{
		idStr str = source.Mid(i, 1024);
		common->Printf("%s", str.c_str());
	}
	common->Printf("\n");
    common->Printf("================================\n");
}

void idARBShader::Write(const char *path, const char *name)
{
    idStr source = ToSource();

    //common->Printf("|%s|\n", source.c_str());

    idStr toPath;
    toPath.Append(path);
    toPath.AppendPath(name);
    if(type == 1)
        toPath.SetFileExtension(".vert");
    else
        toPath.SetFileExtension(".frag");
    common->Printf("Write GLSL shader to '%s'\n", toPath.c_str());
    fileSystem->WriteFile(toPath.c_str(), source.c_str(), source.Length());
}

void idARBProgram::HandleError(const char *error)
{
    common->Warning("Parse error: %s:%d\n", error, parser.GetLineNum());
}

// texture[0]
void idARBProgram::ParseSampler(void)
{
    ExpectToken("texture");

    idStr name;

    parser.ExpectTokenString("[");
    int index = parser.ParseInt();
    name.Append(va("u_fragmentMap%d", index));
    parser.ExpectTokenString("]");

    AddToken(name);
    AddUniform(name, "sampler2D");
}

// state.matrix.projection.row[0]
// state.matrix.modelview.row[0]
// state.matrix.mvp.row[0]
void idARBProgram::ParseState(void)
{
    ExpectToken("state");

    idToken token;
    parser.ExpectTokenString(".");

    parser.ExpectAnyToken(&token);
    idStr name;
	const char *t = "vec4";
    if(!token.Icmp("matrix"))
    {
		parser.ExpectTokenString(".");
		parser.ExpectAnyToken(&token);
		if(!token.Icmp("projection"))
		{
			if(ExpectTokenString("."))
			{
				parser.ExpectTokenString("row");
			}
			name.Append("u_projectionMatrix");
		}
		else if(!token.Icmp("modelview"))
		{
			if(ExpectTokenString("."))
			{
				parser.ExpectTokenString("row");
			}
			name.Append("u_modelViewMatrix");
		}
		else if(!token.Icmp("mvp"))
		{
			if(ExpectTokenString("."))
			{
				parser.ExpectTokenString("row");
			}
			name.Append("u_modelViewProjectionMatrix");
		}
		else
		{
			parser.Error("Unknown program '%s'", token.c_str());
			return;
		}
		t = "mat4";
    }
    else
    {
        parser.Error("Unknown program '%s'", token.c_str());
        return;
    }

    AddUniform(name, t);

	ExpectTokenString("[");
	int index = parser.ParseInt();
	parser.ExpectTokenString("]");
	name.Append(va("[%d]", index));

	if(ExpectTokenString("."))
	{
		if(parser.ReadTokenOnLine(&token))
			name.Append(va(".%s", token.c_str()));
		else
			parser.Error("Missing component '%s'", name.c_str());
	}
    AddToken(name);
}

// program.env[0]
// program.local[0]
void idARBProgram::ParseProgram(void)
{
    ExpectToken("program");

    idToken token;
    parser.ExpectTokenString(".");

    parser.ExpectAnyToken(&token);
    idStr name;
    if(!token.Icmp("env"))
    {
        name.Append("u_uniformParm");
    }
    else if(!token.Icmp("local"))
    {
        if(type == 1)
            name.Append("u_vertexParm");
        else
            name.Append("u_fragmentParm");
    }
    else
    {
        parser.Error("Unknown program '%s'", token.c_str());
        return;
    }
    ExpectTokenString("[");
    int index = parser.ParseInt();
    name.Append(va("%d", index));
    parser.ExpectTokenString("]");

    AddUniform(name);

    if(ExpectTokenString("."))
    {
        if(parser.ReadTokenOnLine(&token))
            name.Append(va(".%s", token.c_str()));
        else
            parser.Error("Missing component '%s'", name.c_str());
    }
    AddToken(name);
}

// vertex.texcoord[0]
// vertex.attrib[0]
void idARBProgram::ParseVertex(void)
{
    ExpectToken("vertex");

    idToken token;
    parser.ExpectTokenString(".");

    parser.ExpectAnyToken(&token);
	const char *dt = "vec4";
    idStr name;
    if(!token.Icmp("texcoord"))
    {
        name.Append("attr_TexCoord");
    }
    else if(!token.Icmp("position"))
    {
        name.Append("attr_Vertex");
    }
    else if(!token.Icmp("color"))
    {
        name.Append("attr_Color");
    }
    else if(!token.Icmp("normal"))
    {
        name.Append("attr_Normal");
		dt = "vec3";
    }
    else if(!token.Icmp("attrib"))
    {
        name.Append("attr_Attrib");
    }
    else
    {
        parser.Error("Unknown vertex '%s'", token.c_str());
        return;
    }
    if(ExpectTokenString("["))
    {
        int index = parser.ParseInt();
		if(!token.Icmp("attrib"))
		{
			if(index == 9)
			{
				name = "attr_Tangent";
				dt = "vec3";
			}
			else if(index == 10)
			{
				name = "attr_Bitangent";
				dt = "vec3";
			}
			else if(index == 8)
				name = "attr_TexCoord";
			else if(index == 11)
			{
				name = "attr_Normal";
				dt = "vec3";
			}
			else if(index == 12)
				name = "attr_Vertex";
			else if(index == 13)
				name = "attr_Color";
			else
				name.Append(va("%d", index));
		}
		else
			name.Append(va("%d", index));
        parser.ExpectTokenString("]");
    }

    AddIn(name, dt);

    if(ExpectTokenString("."))
    {
        if(parser.ReadTokenOnLine(&token))
            name.Append(va(".%s", token.c_str()));
        else
            parser.Error("Missing component '%s'", name.c_str());
    }
    AddToken(name);
}

// fragment.position
// fragment.texcoord[0]
void idARBProgram::ParseFragment(void)
{
    ExpectToken("fragment");

    idToken token;
    parser.ExpectTokenString(".");

    parser.ExpectAnyToken(&token);
    idStr name;
    if(!token.Icmp("position"))
    {
        name.Append("gl_FragCoord");
    }
    else if(!token.Icmp("texcoord"))
    {
        name.Append("var_TexCoord");
    }
    else if(!token.Icmp("color"))
    {
        name.Append("var_Color");
    }
    else
    {
        parser.Error("Unknown fragment '%s'", token.c_str());
        return;
    }
    if(ExpectTokenString("["))
    {
        int index = parser.ParseInt();
        name.Append(va("%d", index));
        parser.ExpectTokenString("]");
    }

    AddIn(name);

    if(ExpectTokenString("."))
    {
        if(parser.ReadTokenOnLine(&token))
            name.Append(va(".%s", token.c_str()));
        else
            parser.Error("Missing component '%s'", name.c_str());
    }
    AddToken(name);
}

// result.texcoord[0]
// result.color
void idARBProgram::ParseResult(void)
{
    ExpectToken("result");

    idToken token;
    parser.ExpectTokenString(".");

    parser.ExpectAnyToken(&token);
    idStr name;
    if(!token.Icmp("position"))
    {
        name.Append("gl_Position");
    }
    else if(!token.Icmp("depth"))
    {
        name.Append("gl_FragDepth");
    }
    else if(!token.Icmp("texcoord"))
    {
        name.Append("var_TexCoord");
    }
    else if(!token.Icmp("color"))
    {
        if(type == 1)
            name.Append("var_Color");
        else
            name.Append("gl_FragColor");
    }
    else
    {
        parser.Error("Unknown result '%s'", token.c_str());
        return;
    }
    if(ExpectTokenString("["))
    {
        int index = parser.ParseInt();
        name.Append(va("%d", index));
        parser.ExpectTokenString("]");
    }

    AddOut(name);

    if(ExpectTokenString("."))
    {
        if(parser.ReadTokenOnLine(&token))
            name.Append(va(".%s", token.c_str()));
        else
            parser.Error("Missing component '%s'", name.c_str());
    }
    AddToken(name);
}

// TEMP A, B, C -> vec4 A; vec4 B; vec4 C
void idARBProgram::TEMP(void)
{
    ExpectTokenString("TEMP");

    idToken token;

    while(parser.ReadTokenOnLine(&token))
    {
        if(!token.Cmp(","))
        {
            AddEOL();
            continue;
        }
        if(!token.Cmp(";"))
            return;
        AddToken("vec4");
        AddToken(token);
        AddEnding();
    }
}

// MOV T, A -> T = A
void idARBProgram::MOV(void)
{
    ExpectTokenString("MOV");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    ParseValue();
    AddEnding();
    parser.ExpectTokenString(";");
}

// RCP T, A -> T = 1.0 / A
void idARBProgram::RCP(void)
{
    ExpectTokenString("RCP");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    AddToken("1.0");
    AddToken("/");
    ParseValue();
    AddEnding();
    parser.ExpectTokenString(";");
}

// RSQ T, A -> T = 1.0 / sqrt(A)
void idARBProgram::RSQ(void)
{
    ExpectTokenString("RSQ");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    AddToken("1.0");
    AddToken("/");
    AddToken("sqrt");
    AddToken("(");
    ParseValue();
    AddToken(")");
    AddEnding();
    parser.ExpectTokenString(";");
}

// KIL -> discard
void idARBProgram::KIL(void)
{
    ExpectTokenString("KIL");

    AddToken("discard");
    AddEnding();
    parser.ExpectTokenString(";");
}

void idARBProgram::Dot(const char *t)
{
    ExpectTokenString(t);

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    AddToken("dot");
    AddToken("(");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken(",");
    ParseValue();
    AddToken(")");
    AddEnding();
    parser.ExpectTokenString(";");
}

// DP3 T, A, B -> T = dot(A, B)
void idARBProgram::DP3(void)
{
    Dot("DP3");
}

// DP4 T, A, B -> T = dot(A, B)
void idARBProgram::DP4(void)
{
    Dot("DP4");
}

// MIN T, A, B -> T = min(A, B)
void idARBProgram::MIN(void)
{
    ExpectTokenString("MIN");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    AddToken("min");
    AddToken("(");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken(",");
    ParseValue();
    AddToken(")");
    AddEnding();
    parser.ExpectTokenString(";");
}

// MAX T, A, B -> T = max(A, B)
void idARBProgram::MAX(void)
{
    ExpectTokenString("MAX");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    AddToken("max");
    AddToken("(");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken(",");
    ParseValue();
    AddToken(")");
    AddEnding();
    parser.ExpectTokenString(";");
}

// POW T, A, B -> T = pow(A, B)
void idARBProgram::POW(void)
{
    ExpectTokenString("POW");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    AddToken("pow");
    AddToken("(");
    ParseValue();
    parser.ExpectTokenString(",");
    AddToken(",");
    ParseValue();
    AddToken(")");
    AddEnding();
    parser.ExpectTokenString(";");
}

// OUTPUT T = A
void idARBProgram::OUTPUT(void)
{
    ExpectTokenString("OUTPUT");

    idToken token;
    parser.ExpectAnyToken(&token);
    AddOut(token);
    AddToken(token);
    parser.ExpectTokenString("=");
    AddToken("=");
	ParseValue();
    AddEnding();
    parser.ExpectTokenString(";");
}

// ATTRIB T = A
void idARBProgram::ATTRIB(void)
{
    ExpectTokenString("ATTRIB");

    idToken token;
    parser.ExpectAnyToken(&token);
	AddToken("vec4");
    AddToken(token);
    parser.ExpectTokenString("=");
    AddToken("=");
	ParseValue();
    AddEnding();
    parser.ExpectTokenString(";");
}

// LRP T, A, B, C -> T = mix( C, B, A )
void idARBProgram::LRP(void)
{
    ExpectTokenString("LRP");

    ParseValue();
    parser.ExpectTokenString(",");
    AddToken("=");
    AddToken("mix");
    AddToken("(");
    ParseValue();
	idStr a, b, c;
	RemoveLast(&a);
    parser.ExpectTokenString(",");
    ParseValue();
	RemoveLast(&b);
    parser.ExpectTokenString(",");
    ParseValue();
	RemoveLast(&c);
	AddToken(c);
    AddToken(",");
	AddToken(b);
    AddToken(",");
	AddToken(a);
    AddToken(")");
    AddEnding();
    parser.ExpectTokenString(";");
}

// PARAM A = { 0, 0, 0, 1 } -> vec4 A = vec4( 0.0, 0.0, 0.0, 1.0 )
// PARAM A[] = program.env[0..5] -> vec4 A[6] = vec4[6]( vec4(program.env[0]), ... )
void idARBProgram::PARAM(void)
{
    ExpectTokenString("PARAM");

    idToken token;
    parser.ExpectAnyToken(&token);
    AddToken("vec4");
    AddToken(token);
    idStr varName = token;
    parser.ExpectAnyToken(&token);
	if(!token.Cmp("[")) // is array
	{
		parser.ExpectAnyToken(&token);
		int length = 0;
		if(token.Cmp("]"))
		{
			parser.UnreadToken(&token);
			length = parser.ParseInt();
			parser.ExpectTokenString("]");
		}
		parser.ExpectTokenString("=");
		parser.ExpectTokenString("{");
		parser.ExpectTokenString("program");

		idToken token;
		parser.ExpectTokenString(".");

		parser.ExpectAnyToken(&token);
		const char *name;
		if(!token.Icmp("env"))
		{
			name = "u_uniformParm";
		}
		else if(!token.Icmp("local"))
		{
			if(type == 1)
				name = "u_vertexParm";
			else
				name = "u_fragmentParm";
		}
		else
		{
			parser.Error("Unknown array program '%s'", token.c_str());
			return;
		}
		parser.ExpectTokenString("[");
		int start = parser.ParseInt();
		parser.ReadToken(&token);
		int end = parser.ParseInt();
		parser.ExpectTokenString("]");
		parser.ExpectTokenString("}");
		parser.ExpectTokenString(";");
		if(length == 0)
			length = end - start + 1;
		AddToken("[");
		AddToken(va("%d", length));
		AddToken("]");
		if(version < 300)
		{
			AddEnding();
            AddEOL();
            for(int i = start; i <= end; i++)
            {
                AddToken(varName);
                AddToken("[");
                AddToken(va("%d", i - start));
                AddToken("]");
                AddToken("=");
                idStr str = va("%s%d", name, i);
                AddToken(str);
                AddUniform(str);
                AddEnding();
                AddEOL();
            }
		}
		else
		{
			AddToken("=");
			AddToken("vec4");
			AddToken("[");
			AddToken(va("%d", length));
			AddToken("]");
			AddToken("(");
			for(int i = start; i <= end; i++)
			{
				idStr str = va("%s%d", name, i);
				AddToken(str);
				AddUniform(str);
				if(i < end)
					AddToken(",");
			}
			AddToken(")");
		}
	}
	else
	{
		parser.UnreadToken(&token);
		parser.ExpectTokenString("=");
		AddToken("=");
		ParseValue();
		parser.ExpectTokenString(";");
	}
	AddEnding();
}

idStr idARBProgram::ReadFloat(void)
{
    float f = parser.ParseFloat();
    idStr str = va("%f", f);
    str.StripTrailing('0');
    int index = str.Find('.');
    if(index == 0)
        str.Insert('0', 0);
    else if(index == -1)
        str.Append(".0");
    else if(index == str.Length() - 1)
        str.Append("0");
	return str;
}

void idARBProgram::ParseFloat(void)
{
    AddToken(ReadFloat());
}

idARBShader * idARBProgram::Shader(void)
{
    if(!shader)
    {
        parser.Error("No shader");
        return NULL;
    }
    return shader;
}

void idARBProgram::ExpectToken(const char *token)
{
    idToken str;
    if(!parser.ReadToken(&str))
    {
        parser.Error("Read expect token '%s' fail", token);
        return;
    }
    if(str.Icmp(token))
    {
        parser.UnreadToken(&str);
        parser.Error("Expect token '%s', but '%s' found", token, str.c_str());
        return;
    }
}

bool idARBProgram::ExpectTokenString(const char *token)
{
    idToken str;
    if(!parser.ReadToken(&str))
    {
        return false;
    }
    if(str.Icmp(token))
    {
        parser.UnreadToken(&str);
        return false;
    }
    return true;
}

void idARBProgram::AddUniform(const char *name, const char *type)
{
    if(!idStr::Cmpn(name, "gl_", 3))
        return;
    idStr str(type);
    str.Append(" ");
    str.Append(name);
    Shader()->AddUniform(str);
}

void idARBProgram::AddIn(const char *name, const char *type)
{
    if(!idStr::Cmpn(name, "gl_", 3))
        return;
    idStr str(type);
    str.Append(" ");
    str.Append(name);
    Shader()->AddIn(str);
}

void idARBProgram::AddOut(const char *name, const char *type)
{
    if(!idStr::Cmpn(name, "gl_", 3))
        return;
    idStr str(type);
    str.Append(" ");
    str.Append(name);
    Shader()->AddOut(str);
}

idStr idARBProgram::TextureFunc(const char *td, const char *d)
{
    if(version < 300)
    {
        idStr str = d;
        if(idStr::CharIsNumeric(d[0]))
            str.ToUpper();
        else
        {
            str.ToLower();
            str[0] = idStr::ToUpper(str[0]);
        }
        if(!idStr::Icmp("proj", td))
            return va("texture%sProj", str.c_str());
        else
            return va("texture%s", str.c_str());
    }
    else
    {
        if(!idStr::Icmp("proj", td))
            return "textureProj";
        else
            return "texture";
    }
}

void GLSL_ArgCompletion_glprogs(const idCmdArgs &args, void(*callback)(const char *s))
{
	cmdSystem->ArgCompletion_FolderExtension(args, callback, "glprogs/", false, "vfp", "fp", "vp", "txt", NULL);
}

void GLSL_ConvertARBShader_f(const idCmdArgs &args)
{
    if (args.Argc() < 2) {
        common->Printf("Usage: %s <ARB shader source file> [<version=100,300> <save path>].\n", args.Argv(0));
        return;
    }

    idStr path = args.Argv(1);
    int version =
#ifdef GL_ES_VERSION_3_0
        USING_GLES3 ? 300 : 100
#else
        100
#endif
        ;
    if(args.Argc() > 2)
        version = atoi(args.Argv(2));
    idARBProgram arb(version);
    idStr savePath;
    if(args.Argc() > 3)
        savePath = args.Argv(3);
    else
        path.ExtractFilePath(savePath);

    idStr fileName;
    path.ExtractFileName(fileName);
    fileName.StripFileExtension();

    common->Printf("Convert ARB shader '%s' to GLSL shader '%s/%s'.\n", path.c_str(), savePath.c_str(), fileName.c_str());

    if(arb.ParseFile(path))
        arb.Write(savePath, fileName);
    else
	{
        common->Warning("Convert ARB shader to GLSL shader fail.");
        arb.Print();
	}
}


#endif


