#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../renderer/tr_local.h"

#if 0
#define NSS_DEBUG(x) x
#else
#define NSS_DEBUG(x)
#endif

template<class T>
void rvNewShaderStageParm__Init(rvNewShaderStage::rvNewShaderStageParm<T> *p) {
    p->index = -1;
    p->location = -1;
    p->numValue = 0;
}

extern idStr RB_GLSL_ConvertGL2ESVertexShader(const char *text, int version);
extern idStr RB_GLSL_ConvertGL2ESFragmentShader(const char *text, int version);

rvNewShaderStage::rvNewShaderStage()
        : shaderProgram(idGLSLShaderManager::INVALID_SHADER_HANDLE),
        numShaderParms(0),
        numShaderTextures(0)
{
    for(int i = 0; i < MAX_SHADER_PARMS; i++)
    {
        rvNewShaderStageParm__Init(shaderParms + i);
        memset(shaderParms[i].value, 0, sizeof(shaderParms[i].value));
    }
    for(int i = 0; i < MAX_SHADER_TEXTURES; i++)
    {
        rvNewShaderStageParm__Init(shaderTextures + i);
        shaderTextures[i].value = NULL;
    }
}

bool rvNewShaderStage::ParseShaderParm(idLexer &src, idMaterial *material)
{
    idToken				token;

    if (numShaderParms >= MAX_SHADER_PARMS) {
        common->Warning("shaderParm overflow\n");
        //SetMaterialFlag(MF_DEFAULTED);
        return false;
    }

    rvNewShaderStageParm<int[4]> *p = shaderParms + numShaderParms;
    p->index = numShaderParms;

    src.ReadTokenOnLine(&token);
    p->name = token;

    p->value[0] = material->ParseExpression(src);

    src.ReadTokenOnLine(&token);

    if (!token[0] || token.Icmp(",")) {
        p->value[1] =
        p->value[2] =
        p->value[3] = p->value[0];
        numShaderParms++;
        p->numValue = 1;
        return true;
    }

    p->value[1] = material->ParseExpression(src);

    src.ReadTokenOnLine(&token);

    if (!token[0] || token.Icmp(",")) {
        p->value[2] = material->GetExpressionConstant(0);
        p->value[3] = material->GetExpressionConstant(1);
        numShaderParms++;
        p->numValue = 2;
        return true;
    }

    p->value[2] = material->ParseExpression(src);

    src.ReadTokenOnLine(&token);

    if (!token[0] || token.Icmp(",")) {
        p->value[3] = material->GetExpressionConstant(1);
        numShaderParms++;
        p->numValue = 3;
        return true;
    }

    p->value[3] = material->ParseExpression(src);
    numShaderParms++;
    p->numValue = 4;
    return true;
}

bool rvNewShaderStage::ParseShaderTexture(idLexer &src, idMaterial *material)
{
    const char			*str;
    textureFilter_t		tf;
    textureRepeat_t		trp;
    textureDepth_t		td;
    cubeFiles_t			cubeMap;
    bool				allowPicmip;
    idToken				token;

    if (numShaderTextures >= MAX_SHADER_TEXTURES) {
        common->Warning("shaderTexture overflow\n");
        //SetMaterialFlag(MF_DEFAULTED);
        return false;
    }

    rvNewShaderStageParm<idImage *> *p = shaderTextures + numShaderTextures;

    tf = TF_DEFAULT;
    trp = TR_REPEAT;
    td = TD_DEFAULT;
    allowPicmip = true;
    cubeMap = CF_2D;

    src.ReadTokenOnLine(&token);
    p->name = token;

    // unit 1 is the normal map.. make sure it gets flagged as the proper depth
/*    if (unit == 1) {
        td = TD_BUMP;
    }*/

    while (1) {
        src.ReadTokenOnLine(&token);

        if (!token.Icmp("cubeMap")) {
            cubeMap = CF_NATIVE;
            continue;
        }

        if (!token.Icmp("cameraCubeMap")) {
            cubeMap = CF_CAMERA;
            continue;
        }

        if (!token.Icmp("nearest")) {
            tf = TF_NEAREST;
            continue;
        }

        if (!token.Icmp("linear")) {
            tf = TF_LINEAR;
            continue;
        }

        if (!token.Icmp("clamp")) {
            trp = TR_CLAMP;
            continue;
        }

        if (!token.Icmp("noclamp")) {
            trp = TR_REPEAT;
            continue;
        }

        if (!token.Icmp("zeroclamp")) {
            trp = TR_CLAMP_TO_ZERO;
            continue;
        }

        if (!token.Icmp("alphazeroclamp")) {
            trp = TR_CLAMP_TO_ZERO_ALPHA;
            continue;
        }

        if (!token.Icmp("forceHighQuality")) {
            td = TD_HIGH_QUALITY;
            continue;
        }

        if (!token.Icmp("uncompressed") || !token.Icmp("highquality")) {
            if (!globalImages->image_ignoreHighQuality.GetInteger()) {
                td = TD_HIGH_QUALITY;
            }

            continue;
        }

        if (!token.Icmp("nopicmip")) {
            allowPicmip = false;
            continue;
        }

        // assume anything else is the image name
        src.UnreadToken(&token);
        break;
    }

    str = R_ParsePastImageProgram(src);

    p->value = globalImages->ImageFromFile(str, tf, allowPicmip, trp, td, cubeMap);

    NSS_DEBUG(common->Printf("NSS shaderTexture: %d %s\n", numShaderTextures+1, p->value?p->value->imgName.c_str():"NULL"));
    if (!p->value) {
        p->value = globalImages->defaultImage;
    }
    p->numValue = 1;
    numShaderTextures++;
    return true;
}

bool rvNewShaderStage::ParseGLSLProgram(idLexer &src, idMaterial *material)
{
    idToken token;
    if (src.ReadTokenOnLine(&token)) {
        shaderProgram = idGLSLShaderManager::INVALID_SHADER_HANDLE;
        //if(shaderProgram == idGLSLShaderManager::INVALID_SHADER_HANDLE)
        {
            token.StripFileExtension();
            if(!LoadGLSLProgram(token.c_str()))
                shaderProgram = idGLSLShaderManager::INVALID_SHADER_HANDLE;
            NSS_DEBUG(common->Printf("NSS glslProgram: %s -> %d\n", material->GetName(), shaderProgram));
            return SHADER_HANDLE_IS_VALID(shaderProgram);
        }
    }
    return false;
}

void rvNewShaderStage::BindUniform(const shaderProgram_t *shader, const float *regs)
{
    //common->Printf("BBB %d\n", shaderProgram);
    // setting local parameters (specified in material definition)
    for ( int i = 0; i < numShaderParms; i++ ) {
        rvNewShaderStageParm<int[4]> *p = shaderParms + i;
        idVec4 vparm;
        for (int d = 0; d < 4; d++)
            vparm[d] = regs[ p->value[d] ];
        GLint location = GetLocation(shader, p);
        switch(p->numValue)
        {
            case 1:
                qglUniform1fv(location, 1, vparm.ToFloatPtr());
                break;
            case 2:
                qglUniform2fv(location, 1, vparm.ToFloatPtr());
                break;
            case 3:
                qglUniform3fv(location, 1, vparm.ToFloatPtr());
                break;
            case 4:
            default:
                qglUniform4fv(location, 1, vparm.ToFloatPtr());
                break;
        }
        //printf("UUU %d %d %s %s\n", i,location, p->name.c_str(), vparm.ToString(6));
    }

    // setting textures
    // note: the textures are also bound to TUs at this moment
    for ( int i = 0; i < numShaderTextures; i++ ) {
        if ( shaderTextures[i].value ) {
            rvNewShaderStageParm<idImage *> *p = shaderTextures + i;
            GLint location = GetLocation(shader, p);
            GL_SelectTexture( i );
            p->value->Bind();
            qglUniform1i(location, i);
            //printf("TTT %d %d %s %s\n", i,location, p->name.c_str(), p->value ? p->value->imgName.c_str() : "NULL");
        }
    }
}

bool rvNewShaderStage::Bind(const float *regs)
{
    if(!IsValid())
        return false;

    const shaderProgram_t *shader = shaderManager->Get(shaderProgram);
    if(!shader)
        return false;
    GL_UseProgram((shaderProgram_t *)shader);

    BindUniform(shader, regs);

    return true;
}

void rvNewShaderStage::UnbindUniform(void)
{
    for ( int i = 0; i < numShaderTextures; i++ ) {
        if ( shaderTextures[i].value ) {
            GL_SelectTexture( i );
            globalImages->BindNull();
        }
    }
}

void rvNewShaderStage::Unbind(void)
{
    UnbindUniform();
    GL_SelectTextureForce(0);
    GL_UseProgram(NULL);
}

bool rvNewShaderStage::LoadSource(const char *name, const char *extension, idStr &ret)
{
    idStr path = "glprogs/";
    path.Append(name);
    path.SetFileExtension(extension);

    void *buffer;
    fileSystem->ReadFile(path.c_str(), &buffer);
    if(!buffer)
        return false;
    ret = (char *) buffer;
    fileSystem->FreeFile(buffer);
    //printf("LLL %s|%s\n", path.c_str(), ret.c_str());
    return ret.Length() > 0;
}

bool rvNewShaderStage::LoadGLSLProgram(const char *name)
{
    const shaderProgram_t *shader;
    shaderHandle_t handle;

    handle = shaderManager->GetHandle(name);
    if(SHADER_HANDLE_IS_VALID(handle))
    {
        shaderProgram = handle;
        return true;
    }

    idStr vertexSource;
    if(!LoadSource(name, "glslvp", vertexSource))
    {
        common->Warning("Load GLSL vertex shader source fail: %s.", name);
        return false;
    }
    idStr fragmentSource;
    if(!LoadSource(name, "glslfp", fragmentSource))
    {
        common->Warning("Load GLSL fragment shader source fail: %s.", name);
        return false;
    }

    common->Printf("Convert GLSL shader %s:\n\n", name);
    const int Version = USING_GLES3 ? 300 : 100;
    GLSLShaderProp prop(name);
    vertexSource = RB_GLSL_ConvertGL2ESVertexShader(vertexSource.c_str(), Version);
    fragmentSource = RB_GLSL_ConvertGL2ESFragmentShader(fragmentSource.c_str(), Version);
    prop.default_vertex_shader_source = vertexSource;
    prop.default_fragment_shader_source = fragmentSource;
    common->Printf("Vertex shader:\n%s\n\n", vertexSource.c_str());
    common->Printf("Fragment shader:\n%s\n\n", fragmentSource.c_str());
    handle = shaderManager->Load(prop);
    if(SHADER_HANDLE_IS_INVALID(handle))
    {
        common->Warning("Load GLSL shader program fail: %s.", name);
        return false;
    }
    shaderProgram = handle;
    return true;
}

void rvNewShaderStage::ReloadImages(bool force)
{
	for (int j = 0 ; j < numShaderTextures ; j++) {
		if (shaderTextures[j].value) {
			shaderTextures[j].value->Reload(false, force);
		}
	}
}
