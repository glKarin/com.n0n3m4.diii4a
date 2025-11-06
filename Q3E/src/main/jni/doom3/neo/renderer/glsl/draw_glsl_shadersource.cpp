

static int RB_GLSL_ParseMacros(const char *macros, idStrList &ret)
{
    if(!macros || !macros[0])
        return 0;

    int start = 0;
    int index;
    int counter = 0;
    idStr str(macros);
    str.Strip(',');
    while((index = str.Find(',', start)) != -1)
    {
        if(index - start > 0)
        {
            idStr s = str.Mid(start, index - start);
            ret.AddUnique(s);
            counter++;
        }
        start = index + 1;
        if(index == str.Length() - 1)
            break;
    }
    if(start <= str.Length() - 1)
    {
        idStr s = str.Mid(start, str.Length() - start);
        ret.AddUnique(s);
        counter++;
    }

    return counter;
}

static int RB_GLSL_FindNextLinePositionOfVersion(const idStr &res)
{
    int index = res.Find("#version");
    if(index == -1)
    SHADER_ERROR("GLSL shader source can not find '#version'\n.");
    index = res.Find('\n', index);
    if(index == -1 || index + 1 == res.Length())
    SHADER_ERROR("GLSL shader source '#version' not completed\n.");
    return index + 1;
}

static idStr RB_GLSL_GetGLSLSourceVersion(const idStr &res)
{
    int start = res.Find("#version");
    if(start == -1)
    SHADER_ERROR("GLSL shader source can not find '#version'\n.");
    start += strlen("#version");
    int end = res.Find('\n', start);
    if(end == -1)
    SHADER_ERROR("GLSL shader source '#version' not completed\n.");
    idStr str = res.Mid(start, end - start);
    str.StripTrailingWhitespace();
    str.StripLeading(' ');
    //common->Printf("GLSL source version: %s\n", str.c_str());
    return str;
}

static void RB_GLSL_InsertGlobalDefines(idStr &res, const char *text)
{
    int index = RB_GLSL_FindNextLinePositionOfVersion(res);
    idStr str("\n");
    str.Append(text);
    str.Append("\n");

    res.Insert(str, index);
}

static idStr RB_GLSL_ExpandMacros(const char *source, const char *macros, int highp = 0)
{
    idStr res(source);

    if(highp > 0)
    {
        res.Replace("precision mediump float;", "precision highp float;");
        res.Replace("precision lowp float;", "precision highp float;");
        idStr samplerPrecision = "precision highp sampler2D;\n"
                                 "precision highp samplerCube;\n";
        if(USING_GLES3)
        {
            idStr ver = RB_GLSL_GetGLSLSourceVersion(res);
            if(ver.Cmp("120") > 0)
            {
                samplerPrecision.Append("precision highp sampler2DArrayShadow;\n"
                                        "precision highp sampler2DArray;\n"
                );
            }
        }
        RB_GLSL_InsertGlobalDefines(res, samplerPrecision.c_str());

        if(highp > 1)
        {
            res.Replace("mediump ", "highp ");
            res.Replace("lowp ", "highp ");
        }
    }

    if(!macros || !macros[0])
    {
        // printf("|%s|\n", res.c_str());
        return res;
    }

    idStrList list;
    int n = RB_GLSL_ParseMacros(macros, list);
    if(0 == n)
        return res;

    idStr m;
    for(int i = 0; i < list.Num(); i++)
    {
        m.Append("#define " + list[i] + "\n");
    }

    RB_GLSL_InsertGlobalDefines(res, m);

    // printf("%d|%s|\n%s\n", n, macros, res.c_str());
    return res;
}

static idStr RB_GLSL_GetExternalShaderSourcePath(void)
{
    idStr	fullPath;
#ifdef GL_ES_VERSION_3_0
    if(USING_GLES3)
    {
        fullPath = cvarSystem->GetCVarString("harm_r_shaderProgramES3Dir");
        if(fullPath.IsEmpty())
            fullPath = _GL3PROGS;
    }
    else
#endif
    {
        fullPath = cvarSystem->GetCVarString("harm_r_shaderProgramDir");
        if(fullPath.IsEmpty())
            fullPath = _GLPROGS;
    }
    return fullPath;
}

static int RB_GLSL_ReadExternalShaderSource(idStr &fullPath, idStr &buffer)
{
    idStr nameStr = fullPath;

    fullPath = RB_GLSL_GetExternalShaderSourcePath();

    fullPath.AppendPath(nameStr);

    char	*fileBuffer;

    // load the program even if we don't support it, so
    // fs_copyfiles can generate cross-platform data dumps
    fileSystem->ReadFile(fullPath.c_str(), (void **)&fileBuffer, NULL);

    if (!fileBuffer) {
        return 0;
    }

    // copy to stack memory and free
/*    buffer = (char *)_alloca(strlen(fileBuffer) + 1);
    strcpy(buffer, fileBuffer);*/

    int len = strlen(fileBuffer);
    buffer.Append(fileBuffer, strlen(fileBuffer));

    fileSystem->FreeFile(fileBuffer);

    return len;
}



void R_ExportGLSLShaderSource_f(const idCmdArgs &args)
{
    const char *vs;
    const char *fs;
    const char *macros;
    idList<GLSLShaderProp> Props;
    idStr path = NULL;
    idStrList target;

    RB_GLSL_GetShaderSources(Props);
    if(args.Argc() > 1)
        path = args.Argv(args.Argc() - 1);

    for(int i = 1; i < args.Argc() - 1; i++)
    {
        target.Append(args.Argv(i));
    }

    if(path.IsEmpty())
        path = RB_GLSL_GetExternalShaderSourcePath();

    if(!path.IsEmpty() && path[path.Length() - 1] != '/')
        path += "/";

    common->Printf("Save GLSL shader source to '%s'\n", path.c_str());

    for(int i = 0; i < Props.Num(); i++)
    {
        const GLSLShaderProp &prop = Props[i];
        if(target.Num() > 0 && target.FindIndex(prop.name) < 0)
            continue;

        vs = prop.default_vertex_shader_source.c_str();
        fs = prop.default_fragment_shader_source.c_str();
        macros = prop.macros.c_str();

        idStr vsSrc = RB_GLSL_ExpandMacros(vs, macros, harm_r_useHighPrecision.GetInteger());
        idStr p(path);
        p.Append(prop.vertex_shader_source_file);
        fileSystem->WriteFile(p.c_str(), vsSrc.c_str(), vsSrc.Length(), "fs_basepath");
        common->Printf("GLSL vertex shader: '%s'\n", p.c_str());

        idStr fsSrc = RB_GLSL_ExpandMacros(fs, macros, harm_r_useHighPrecision.GetInteger());
        p = path;
        p.Append(prop.fragment_shader_source_file);
        fileSystem->WriteFile(p.c_str(), fsSrc.c_str(), fsSrc.Length(), "fs_basepath");
        common->Printf("GLSL fragment shader: '%s'\n", p.c_str());
    }
}

static void R_PrintGLSLShaderSource(const idStr &source)
{
    int i = 0;
    while(i < source.Length())
    {
        idStr str = source.Mid(i, 1024);
        common->Printf("%s", str.c_str());
        i += str.Length();
    }
}

void R_PrintGLSLShaderSource_f(const idCmdArgs &args)
{
    const char *vs;
    const char *fs;
    const char *macros;
    idList<GLSLShaderProp> Props;
    idStrList target;

    RB_GLSL_GetShaderSources(Props);

    for(int i = 1; i < args.Argc(); i++)
    {
        target.Append(args.Argv(i));
    }

    for(int i = 0; i < Props.Num(); i++)
    {
        const GLSLShaderProp &prop = Props[i];
        if(target.Num() > 0 && target.FindIndex(prop.name) < 0)
            continue;

        vs = prop.default_vertex_shader_source.c_str();
        fs = prop.default_fragment_shader_source.c_str();
        macros = prop.macros.c_str();
        common->Printf("GLSL shader: %s\n\n", prop.name.c_str());

        idStr vsSrc = RB_GLSL_ExpandMacros(vs, macros, harm_r_useHighPrecision.GetInteger());
        common->Printf("  Vertex shader: \n");
        R_PrintGLSLShaderSource(vsSrc);
        common->Printf("\n");

        idStr fsSrc = RB_GLSL_ExpandMacros(fs, macros, harm_r_useHighPrecision.GetInteger());
        common->Printf("  Fragment shader: \n");
        R_PrintGLSLShaderSource(fsSrc);
        common->Printf("\n");
    }
}

// Convert OpenGL2.0(GLSL 120) shader to GLES2.0(GLSL 100es) or GLES3.x(GLSL 3xx es) for Quake 4 GLSL progs
/**
 * Vertex shader
 * ES2.0
 * 	+ #version 100
 * 	+ precision mediump float;
 * 	+ attribute highp vec4 attr_Vertex;
 * 	+ attribute highp vec4 attr_TexCoord;
 * 	+ attribute lowp vec4 attr_Color;
 * 	+ attribute vec3 attr_Normal;
 * 	+ uniform vec4 u_glColor;
 * 	+ uniform mat4 u_modelViewProjectionMatrix;
 * 	ftransform() -> u_modelViewProjectionMatrix * attr_Vertex
 * 	gl_Vertex -> attr_Vertex
 * 	gl_MultiTexCoord0 -> attr_TexCoord
 * 	gl_Color -> u_glColor // * (attr_Color / 255.0)
 * 	gl_Normal -> attr_Normal
 *
 * ES3.0
 * 	+ #version <version> es
 * 	+ precision mediump float;
 * 	+ in highp vec4 attr_Vertex;
 * 	+ in highp vec4 attr_TexCoord;
 * 	+ in lowp vec4 attr_Color;
 * 	+ in vec3 attr_Normal;
 * 	+ uniform vec4 u_glColor;
 * 	+ uniform mat4 u_modelViewProjectionMatrix;
 *	attribute -> in
 *	varying -> out
 * 	ftransform() -> u_modelViewProjectionMatrix * attr_Vertex
 * 	gl_Vertex -> attr_Vertex
 * 	gl_MultiTexCoord0 -> attr_TexCoord
 * 	gl_Color -> u_glColor // * (attr_Color / 255.0)
 * 	gl_Normal -> attr_Normal
 */
idStr RB_GLSL_ConvertGL2ESVertexShader(const char *text, int version)
{
    idStr source = text;

    idStr ver;
    idStr attribute;
    if(version == 100)
    {
        ver = "100";
        attribute = "attribute";
    }
    else
    {
        ver += version;
        ver += " es";
        attribute = "in";
        source.Replace("varying", "out");
    }

    source.Replace("ftransform()", "u_modelViewProjectionMatrix * attr_Vertex");
    source.Replace("gl_Vertex", "attr_Vertex");
    source.Replace("gl_MultiTexCoord0", "attr_TexCoord");
    //source.Replace("gl_Color", "(attr_Color / 255.0)");
    //source.Replace("gl_Color", "(u_glColor * attr_Color / 255.0)");
    source.Replace("gl_Color", "u_glColor");
    source.Replace("gl_Normal", "attr_Normal");

    idStr ret;
    ret += "#version ";
    ret += ver;
    ret += "\n";
    ret += "//#pragma optimize(off)\n";
    ret += "\n";
    ret += "precision highp float;\n";
    ret += "\n";

    ret += attribute + " highp vec4 attr_Vertex;\n";
    ret += attribute + " highp vec4 attr_TexCoord;\n";
    ret += attribute + " lowp vec4 attr_Color;\n";
    ret += attribute + " vec3 attr_Normal;\n";
    ret += "\n";

    ret += "uniform lowp vec4 u_glColor;\n";
    ret += "uniform highp mat4 u_modelViewProjectionMatrix;\n";
    ret += "\n";

    ret += source;

    return ret;
}

/**
 * Fragment shader
 * ES2.0
 * 	+ #version 100
 * 	+ precision mediump float;
 *
 * ES3.0
 * 	+ #version <version> es
 * 	+ precision mediump float;
 *	varying -> in
 * 	+ out vec4 _gl_FragColor;
 * 	gl_FragColor -> _gl_FragColor
 * 	texture2D -> texture
 * 	textureCube -> texture
 * 	texture2DProj -> textureProj
 * 	textureCubeProj -> textureProj
 */
idStr RB_GLSL_ConvertGL2ESFragmentShader(const char *text, int version)
{
    idStr source = text;

    idStr ver;
    idStr out;
    if(version == 100)
    {
        ver = "100";
    }
    else
    {
        ver += version;
        ver += " es";
        source.Replace("varying", "in");
        out = "out vec4 _gl_FragColor;\n";
        source.Replace("gl_FragColor", "_gl_FragColor");
        source.Replace("texture2D", "texture");
        source.Replace("texture2DProj", "textureProj");
        source.Replace("textureCube", "texture");
        source.Replace("textureCubeProj", "textureProj");
    }


    idStr ret;
    ret += "#version ";
    ret += ver;
    ret += "\n";
    ret += "//#pragma optimize(off)\n";
    ret += "\n";
    ret += "precision highp float;\n";
    ret += "\n";

    ret += out;

    ret += source;

    return ret;
}

void RB_GLSL_PrintShaderSource(const char *filename, const char *source)
{
    idStr str(source);
    int line = 1;
    int index;
    int start = 0;

    common->Printf("---------- GLSL shader: %s ----------\n", filename ? filename : "<implicit file>");
    while((index = str.Find('\n', start)) != -1)
    {
        idStr sub = str.Mid(start, index - start);
        common->Printf("%4d: %s\n", line, sub.c_str());
        start = index + 1;
        line++;
    }
    if(start < str.Length() - 1)
    {
        idStr sub = str.Right(str.Length() - start);
        common->Printf("%4d: %s\n", line, sub.c_str());
    }
    common->Printf("--------------------------------------------------\n");
}

static void RB_GLSL_ExportDevGLSLShaderSource(const char *source, const char *name, const char *dir)
{
    if(!source || !source[0])
        return;

    idStr path = dir;

    if(!path.IsEmpty() && path[path.Length() - 1] != '/')
        path += "/";

    common->Printf("Save base GLSL shader source '%s' to '%s'\n", name, path.c_str());

    idStr p(path);
    p.Append(name);
    fileSystem->WriteFile(p.c_str(), source, strlen(source), "fs_basepath");
}

void R_ExportDevShaderSource_f(const idCmdArgs &args)
{
#undef _KARIN_GLSL_SHADER_H
#include "glsl_shader.h"

#ifdef GL_ES_VERSION_3_0
#define EXPORT_SHADER_SOURCE(source, name, type) \
	{ \
	if(gl2) \
		RB_GLSL_ExportDevGLSLShaderSource(source, name "." type, SHADER_ES_PATH); \
	else \
		RB_GLSL_ExportDevGLSLShaderSource(ES3_##source, name "." type, SHADER_ES_PATH); \
	}
#else
#define EXPORT_SHADER_SOURCE(source, name, type) RB_GLSL_ExportBaseGLSLShaderSource(source, name "." type, SHADER_ES_PATH);
#endif
#define EXPORT_SHADER_PAIR_SOURCE(source, name) \
            EXPORT_SHADER_SOURCE(source##_VERT, name, "vert") \
            EXPORT_SHADER_SOURCE(source##_FRAG, name, "frag")

#define EXPORT_BASE_SHADER() \
	EXPORT_SHADER_PAIR_SOURCE(INTERACTION, "interaction"); \
	EXPORT_SHADER_PAIR_SOURCE(SHADOW, "shadow"); \
	EXPORT_SHADER_PAIR_SOURCE(DEFAULT, "default"); \
	EXPORT_SHADER_PAIR_SOURCE(ZFILL, "zfill"); \
	EXPORT_SHADER_PAIR_SOURCE(ZFILLCLIP, "zfillClip"); \
	EXPORT_SHADER_PAIR_SOURCE(CUBEMAP, "cubemap"); \
	EXPORT_SHADER_PAIR_SOURCE(ENVIRONMENT, "environment"); \
	EXPORT_SHADER_PAIR_SOURCE(BUMPY_ENVIRONMENT, "bumpyEnvironment"); \
	EXPORT_SHADER_PAIR_SOURCE(FOG, "fog"); \
	EXPORT_SHADER_SOURCE(BLENDLIGHT_VERT, "blendLight", "vert"); \
	EXPORT_SHADER_SOURCE(DIFFUSE_CUBEMAP_VERT, "diffuseCubemap", "vert"); \
	EXPORT_SHADER_PAIR_SOURCE(TEXGEN, "texgen"); \
	EXPORT_SHADER_PAIR_SOURCE(HEATHAZE, "heatHaze"); \
	EXPORT_SHADER_PAIR_SOURCE(HEATHAZEWITHMASK, "heatHazeWithMask"); \
	EXPORT_SHADER_PAIR_SOURCE(HEATHAZEWITHMASKANDVERTEX, "heatHazeWithMaskAndVertex"); \
	EXPORT_SHADER_PAIR_SOURCE(COLORPROCESS, "colorProcess"); \
    EXPORT_SHADER_PAIR_SOURCE(MEGATEXTURE, "megaTexture");

#define EXPORT_DEBUG_SHADER() \
	EXPORT_SHADER_SOURCE(SIMPLE_VERTEX_TEXCOORD_VERT, "simpleVertexTexcoord", "vert"); \
	EXPORT_SHADER_SOURCE(DEPTH_TO_COLOR_FRAG, "depthToColor", "frag"); \
	EXPORT_SHADER_SOURCE(STENCIL_TO_COLOR_FRAG, "stencilToColor", "frag");

#ifdef _GLOBAL_ILLUMINATION
#define EXPORT_GI_SHADER() \
    EXPORT_SHADER_PAIR_SOURCE(GLOBAL_ILLUMINATION, "globalIllumination");
#endif

#define EXPORT_D3XP_SHADER() \
    EXPORT_SHADER_PAIR_SOURCE(ENVIROSUIT, "enviroSuit");

#ifdef _HUMANHEAD
    #define EXPORT_PREY_SHADER() \
    EXPORT_SHADER_PAIR_SOURCE(SCREENEFFECT, "screeneffect"); \
    EXPORT_SHADER_PAIR_SOURCE(RADIALBLUR, "radialblur"); \
    EXPORT_SHADER_PAIR_SOURCE(LIQUID, "liquid"); \
    EXPORT_SHADER_PAIR_SOURCE(SCREENPROCESS, "screenprocess");
#endif

#ifdef _SHADOW_MAPPING
#define EXPORT_SHADOW_MAPPING_SHADER() \
	EXPORT_SHADER_PAIR_SOURCE(DEPTH, "depthShadowMapping"); \
	EXPORT_SHADER_PAIR_SOURCE(DEPTH_PERFORATED, "depthPerforated"); \
	EXPORT_SHADER_PAIR_SOURCE(INTERACTION_SHADOW_MAPPING, "interactionShadowMapping");
#endif

#ifdef _STENCIL_SHADOW_IMPROVE
#define EXPORT_STENCIL_SHADOW_SHADER() \
	EXPORT_SHADER_PAIR_SOURCE(INTERACTION_STENCIL_SHADOW, "interactionStencilShadow");
#endif

#ifdef _POSTPROCESS
#define EXPORT_POSTPROCESS_SHADER() \
    EXPORT_SHADER_SOURCE(RETRO_POSTPROCESS_2D_VERT, "retro_postprocess_2d", "vert"); \
	EXPORT_SHADER_SOURCE(RETRO_2BIT_FRAG, "retro_2bit", "frag"); \
	EXPORT_SHADER_SOURCE(RETRO_C64_FRAG, "retro_c64", "frag"); \
	EXPORT_SHADER_SOURCE(RETRO_CPC_FRAG, "retro_cpc", "frag"); \
	EXPORT_SHADER_SOURCE(RETRO_GENESIS_FRAG, "retro_genesis", "frag"); \
	EXPORT_SHADER_SOURCE(RETRO_PS1_FRAG, "retro_ps1", "frag");
#endif

#define SHADER_ES_PATH glprogs.c_str()
    bool gl2 = true;
    idStr glprogs;

    glprogs = "dev/glslprogs";
    EXPORT_BASE_SHADER()
    EXPORT_D3XP_SHADER()
    EXPORT_DEBUG_SHADER()
#ifdef _GLOBAL_ILLUMINATION
    EXPORT_GI_SHADER()
#endif
#ifdef _SHADOW_MAPPING
    EXPORT_SHADOW_MAPPING_SHADER()
#endif
#ifdef _STENCIL_SHADOW_IMPROVE
    EXPORT_STENCIL_SHADOW_SHADER()
#endif
#ifdef _POSTPROCESS
    EXPORT_POSTPROCESS_SHADER()
#endif
#ifdef _HUMANHEAD
    EXPORT_PREY_SHADER()
#endif

#ifdef GL_ES_VERSION_3_0
    gl2 = false;
    glprogs = "dev/glsl3progs";

    EXPORT_BASE_SHADER()
    EXPORT_D3XP_SHADER()
    EXPORT_DEBUG_SHADER()
#ifdef _GLOBAL_ILLUMINATION
    EXPORT_GI_SHADER()
#endif
#ifdef _SHADOW_MAPPING
    EXPORT_SHADOW_MAPPING_SHADER()
#endif
#ifdef _STENCIL_SHADOW_IMPROVE
    EXPORT_STENCIL_SHADOW_SHADER()
#endif
#ifdef _POSTPROCESS
    EXPORT_POSTPROCESS_SHADER()
#endif
#ifdef _HUMANHEAD
    EXPORT_PREY_SHADER()
#endif

#endif
}

bool RB_GLSL_FindGLSLShaderSource(const char *name, int type, idStr *source, idStr *realPath)
{
    idStr path;
    void *data = NULL;
    int length = 0;

    idStrList exts;
    if(type == 2) // fragment
    {
        exts.Append(".frag");
        exts.Append(".fp");
    }
    else // == 1 vertex
    {
        exts.Append(".vert");
        exts.Append(".vp");
    }

#if 0
    // 1. find in glslprogs or glsl3progs
    idStr glesDir = RB_GLSL_GetExternalShaderSourcePath();
    path = glesDir;
    path.AppendPath(name);
    if((length = fileSystem->ReadFile(path.c_str(), &data, NULL)) <= 0)
    {
        path.StripFileExtension();
        for(int i = 0; i < exts.Num(); i++)
        {
            path.SetFileExtension(exts[i]);
            if((length = fileSystem->ReadFile(path.c_str(), &data, NULL)) > 0)
            {
                break;
            }
        }
    }
#endif

    // 2. find in glprogs
    if(length <= 0)
    {
        path = "glprogs";
        path.AppendPath(name);
        if((length = fileSystem->ReadFile(path.c_str(), &data, NULL)) <= 0)
        {
            path.StripFileExtension();
            for(int i = 0; i < exts.Num(); i++)
            {
                path.SetFileExtension(exts[i]);
                if((length = fileSystem->ReadFile(path.c_str(), &data, NULL)) > 0)
                {
                    break;
                }
            }
        }
    }

    if(length > 0)
    {
        if(realPath)
            *realPath = path;
        if(source)
        {
            idStr str;
            str.Append((char *)data, length);
            *source = str;
        }

        fileSystem->FreeFile(data);
        return true;
    }
    else
    {
        return false;
    }
}

#include "glsl_arb_shader.cpp"
