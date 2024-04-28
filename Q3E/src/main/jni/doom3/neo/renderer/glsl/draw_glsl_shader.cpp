#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../tr_local.h"

#define LOG_LEN 2048

#if !defined(_MSC_VER)
#define SHADER_ERROR(fmt, args...) { \
	if(shaderRequired) {               \
		common->Error(fmt, ##args); \
	} else {                           \
		common->Warning(fmt, ##args);   \
	}                                    \
}
#else
#define SHADER_ERROR(fmt, ...) { \
	if(shaderRequired) {               \
		common->Error(fmt, __VA_ARGS__); \
	} else {                           \
		common->Warning(fmt, __VA_ARGS__);   \
	}                                    \
}
#endif

//#define _DEBUG_VERT_SHADER_SOURCE
//#define _DEBUG_FRAG_SHADER_SOURCE

#if 1
#define GL_GetAttribLocation(program, name) qglGetAttribLocation(program, name)
#define GL_GetUniformLocation(program, name) qglGetUniformLocation(program, name)
#else
GLint GL_GetAttribLocation(GLint program, const char *name)
{
    GLint attribLocation = qglGetAttribLocation(program, name);
	Sys_Printf("GL_GetAttribLocation(%s) -> %d\n", name, attribLocation);
	return attribLocation;
}
GLint GL_GetUniformLocation(GLint program, const char *name)
{
	GLint uniformLocation = qglGetUniformLocation(program, name);
	Sys_Printf("GL_GetUniformLocation(%s) -> %d\n", name, uniformLocation);
	return uniformLocation;
}
#endif

static bool shaderRequired = true;
#define REQUIRE_SHADER shaderRequired = true;
#define UNNECESSARY_SHADER shaderRequired = false;

#ifdef _SHADOW_MAPPING
extern bool r_useDepthTexture;
extern bool r_useCubeDepthTexture;
extern bool r_usePackColorAsDepth;
#endif

static int RB_GLSL_LoadShaderProgram(
		const char *name,
        int type,
		shaderProgram_t * const program,
		const char *default_vertex_shader_source,
		const char *default_fragment_shader_source,
		const char *vertex_shader_source_file,
		const char *fragment_shader_source_file,
		const char *macros
);

static bool RB_GLSL_LoadShaderProgramFromProp(const GLSLShaderProp *prop)
{
	const char *vs;
	const char *fs;
	const char *macros;

	vs = prop->default_vertex_shader_source.c_str();
	fs = prop->default_fragment_shader_source.c_str();
	macros = prop->macros.c_str();

	if(RB_GLSL_LoadShaderProgram(
			prop->name.c_str(),
            prop->type,
			prop->program,
			vs,
			fs,
			prop->vertex_shader_source_file.c_str(),
			prop->fragment_shader_source_file.c_str(),
			macros
	) < 0)
		return false;
	return true;
}

#define GLSL_PROGRAM_PROC
#include "glsl_program.h"
#undef GLSL_PROGRAM_PROC

#define GLSL_INTERNAL_SHADER_INDEX_TO_HANDLE(x) ( (x) + 1 )
#define GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(x) ( -( (x) + 1) )
#define GLSL_INTERNAL_SHADER_HANDLE_TO_INDEX(x) ( (x) - 1 )
#define GLSL_CUSTOM_SHADER_HANDLE_TO_INDEX(x) ( -(x) - 1 )

const shaderHandle_t idGLSLShaderManager::INVALID_SHADER_HANDLE = 0;

int idGLSLShaderManager::Add(shaderProgram_t *shader)
{
	if(!shader)
		return -1;
	if(!shader->program)
	{
		common->Warning("idGLSLShaderManager::Add shader's program handle is 0!");
		return -1;
	}
	int index = FindIndex(shader->name);
	if(index >= 0)
	{
		common->Warning("idGLSLShaderManager::Add shader's name is dup '%s'!", shader->name);
		return index; // -1
	}
	common->Printf("idGLSLShaderManager::Add shader program '%s'.\n", shader->name);
	return shaders.Append(shader);
}

void idGLSLShaderManager::Clear(void)
{
	shaders.Clear();
}

const shaderProgram_t * idGLSLShaderManager::Find(const char *name) const
{
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(!idStr::Icmp(name, shader->name))
        {
            common->Printf("[Harmattan]: GLSL shader manager::Find '%s' -> %d %s.\n", shader->name, shader->type, shader->type == SHADER_CUSTOM ? "custom" : "internal");
            return shader;
        }
	}
	return NULL;
}

const shaderProgram_t * idGLSLShaderManager::Find(GLuint handle) const
{
	if(handle == 0)
		return NULL;
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(handle == shader->program)
			return shader;
	}
	return NULL;
}

int idGLSLShaderManager::FindIndex(const char *name) const
{
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(!idStr::Icmp(name, shader->name))
		{
			common->Printf("[Harmattan]: GLSL shader manager::FindIndex '%s' -> %d %s.\n", shader->name, shader->type, shader->type == SHADER_CUSTOM ? "custom" : "internal");
			return i;
		}
	}
	return -1;
}

int idGLSLShaderManager::FindIndex(GLuint handle) const
{
	if(handle == 0)
		return NULL;
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(handle == shader->program)
			return i;
	}
	return -1;
}

const shaderProgram_t * idGLSLShaderManager::Get(shaderHandle_t handle) const
{
	int index;

	if(handle > 0)
	{
		index = GLSL_INTERNAL_SHADER_HANDLE_TO_INDEX(handle);
		if(index < shaders.Num())
			return shaders[index];
	}
	else if(handle < 0)
	{
		index = GLSL_CUSTOM_SHADER_HANDLE_TO_INDEX(handle);
		if(index < customShaders.Num())
		{
			const shaderProgram_t *program = customShaders[index].program;
			if(program && program->program > 0)
				return program;
		}
	}
	return NULL;
}

shaderHandle_t idGLSLShaderManager::GetHandle(const char *name) const
{
    int index;

    index = FindIndex(name);
    if(index >= 0)
        return GLSL_INTERNAL_SHADER_INDEX_TO_HANDLE(index);

	index = FindCustomIndex(name);
	if(index >= 0)
		return GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(index);

    return INVALID_SHADER_HANDLE;
}

int idGLSLShaderManager::FindCustomIndex(const char *name) const
{
	for(int i = 0; i < customShaders.Num(); i++)
	{
		const GLSLShaderProp &p = customShaders[i];
		if(!idStr::Icmp(name, p.name.c_str()))
			return i;
	}
	return -1;
}

GLSLShaderProp * idGLSLShaderManager::FindCustom(const char *name)
{
	for(int i = 0; i < customShaders.Num(); i++)
	{
		GLSLShaderProp &p = customShaders[i];
		if(!idStr::Icmp(name, p.name.c_str()))
			return &p;
	}
	return NULL;
}

shaderHandle_t idGLSLShaderManager::Load(const GLSLShaderProp &inProp)
{
	int index;
	GLSLShaderProp *prop;

	// check has loaded
	index = FindIndex(inProp.name.c_str());
	if(index >= 0)
	{
		common->Printf("[Harmattan]: GLSL shader manager::Load shader '%s' has loaded.\n", inProp.name.c_str());
		return GLSL_INTERNAL_SHADER_INDEX_TO_HANDLE(index);
	}

	// check has custom loaded
	prop = FindCustom(inProp.name.c_str());
	if(prop)
	{
		if(prop->program && prop->program->program > 0)
		{
			common->Printf("[Harmattan]: GLSL shader manager::Load custom shader '%s' has already loaded.\n", inProp.name.c_str());
			return GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(index);
		}
		else
		{
			common->Printf("[Harmattan]: GLSL shader manager::Load custom shader '%s' has already load failed.\n", inProp.name.c_str());
			return INVALID_SHADER_HANDLE;
		}
	}

	// add to queue
	GLSLShaderProp p = inProp;
	p.type = SHADER_CUSTOM;
	p.program = NULL;
	index = customShaders.Append(p);
	// queue.Append(index);
	common->Printf("[Harmattan]: GLSL shader manager::Load shader push '%s' into queue.\n", p.name.c_str());

#ifdef _MULTITHREAD // in multi-threading, push on queue and load on backend
	if(!multithreadActive) {
#endif
		ActuallyLoad();
#ifdef _MULTITHREAD
	}
#endif

	return GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(index);
}

void idGLSLShaderManager::ActuallyLoad(void)
{
	unsigned int index = queueCurrentIndex;
	const int num = customShaders.Num();

	if(
			// !queue.Num()
			index >= num
			)
		return;

	const bool B = shaderRequired;
	UNNECESSARY_SHADER;

	while(
			index < num
			// queue.Num()
			)
	{
		/*
		index = queue[0];
		queue.RemoveIndex(0); // always remove it

		// it's not happened, using assert
		if(index >= customShaders.Num())
		{
			common->Warning("[Harmattan]: GLSL shader manager::ActuallyLoad custom shader index '%d' over( >= %d ).", index, customShaders.Num());
			continue;
		}*/

		GLSLShaderProp &prop = customShaders[index];
		index++;

		if(FindIndex(prop.name.c_str()) >= 0)
		{
			common->Warning("[Harmattan]: GLSL shader manager::ActuallyLoad custom shader '%s' has loaded.", prop.name.c_str());
			continue;
		}
		if(prop.program)
		{
			common->Warning("[Harmattan]: GLSL shader manager::ActuallyLoad custom shader '%s' has handle.", prop.name.c_str());
			continue;
		}

		// create shader on heap
		shaderProgram_t *shader = (shaderProgram_t *)malloc(sizeof(*shader));
		memset(shader, 0, sizeof(*shader));
		strncpy(shader->name, prop.name.c_str(), sizeof(shader->name));
		shader->type = SHADER_CUSTOM;
		prop.program = shader;

		common->Printf("[Harmattan]: GLSL shader manager::ActuallyLoad shader '%s'.\n", prop.name.c_str());
		if(RB_GLSL_LoadShaderProgramFromProp(&prop))
		{
			Add(shader);
		}
		else
		{
			memset(prop.program, 0, sizeof(*prop.program));
			common->Warning("[Harmattan]: GLSL shader manager::ActuallyLoad shader '%s' error!", prop.name.c_str());
		}
	}
	shaderRequired = B;
	queueCurrentIndex = index;
}

idGLSLShaderManager::~idGLSLShaderManager()
{
	printf("[Harmattan]: GLSL shader manager destroying......\n");
	//queue.Clear();
	queueCurrentIndex = customShaders.Num();
	for(int i = 0; i < customShaders.Num(); i++)
	{
		GLSLShaderProp &prop = customShaders[i];
		if(prop.program)
		{
			free(prop.program);
			prop.program = NULL;
		}
	}
	Clear();
}

idGLSLShaderManager idGLSLShaderManager::_shaderManager;

idGLSLShaderManager *shaderManager = &idGLSLShaderManager::_shaderManager;

#define _GLPROGS "glslprogs" // "gl2progs"
static idCVar	harm_r_shaderProgramDir("harm_r_shaderProgramDir", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "[Harmattan]: Special external OpenGLES2 GLSL shader program directory path(default is empty, means using `" _GLPROGS "`).");

#ifdef GL_ES_VERSION_3_0
#define _GL3PROGS "glsl3progs"
static idCVar	harm_r_shaderProgramES3Dir("harm_r_shaderProgramES3Dir", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "[Harmattan]: Special external OpenGLES3 GLSL shader program directory path(default is empty, means using `" _GL3PROGS "`).");
#endif

static void RB_GLSL_GetShaderSources(idList<GLSLShaderProp> &ret)
{
#include "glsl_shader.h"
#ifdef GL_ES_VERSION_3_0
#define GLSL_SHADER_SOURCE(name, type, program, vs, fs, macros, macros3) GLSLShaderProp(name, type, program, USING_GLES3 ? ES3_##vs : vs, USING_GLES3 ? ES3_##fs : fs, USING_GLES3 ? macros3 : macros)
#else
#define GLSL_SHADER_SOURCE(name, type, program, vs, fs, macros, unused) GLSLShaderProp(name, type, program, vs, fs, macros)
#endif

	ret.Clear();

	// base
	ret.Append(GLSL_SHADER_SOURCE("interaction", SHADER_INTERACTION, &interactionShader, INTERACTION_VERT, INTERACTION_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("shadow", SHADER_SHADOW, &shadowShader, SHADOW_VERT, SHADOW_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("default", SHADER_DEFAULT, &defaultShader, DEFAULT_VERT, DEFAULT_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("zfill", SHADER_ZFILL, &depthFillShader, ZFILL_VERT, ZFILL_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("zfillClip", SHADER_ZFILLCLIP, &depthFillClipShader, ZFILLCLIP_VERT, ZFILLCLIP_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("cubemap", SHADER_CUBEMAP, &cubemapShader, CUBEMAP_VERT, CUBEMAP_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("reflectionCubemap", SHADER_REFLECTIONCUBEMAP, &reflectionCubemapShader, REFLECTION_CUBEMAP_VERT, CUBEMAP_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("fog", SHADER_FOG, &fogShader, FOG_VERT, FOG_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("blendLight", SHADER_BLENDLIGHT, &blendLightShader, BLENDLIGHT_VERT, FOG_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphong", SHADER_INTERACTIONBLINNPHONG, &interactionBlinnPhongShader, INTERACTION_VERT, INTERACTION_FRAG, "BLINN_PHONG", "BLINN_PHONG"));
	ret.Append(GLSL_SHADER_SOURCE("diffuseCubemap", SHADER_DIFFUSECUBEMAP, &diffuseCubemapShader, DIFFUSE_CUBEMAP_VERT, CUBEMAP_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("texgen", SHADER_TEXGEN, &texgenShader, TEXGEN_VERT, TEXGEN_FRAG, "", ""));

    // newStage
	ret.Append(GLSL_SHADER_SOURCE("heatHaze", SHADER_HEATHAZE, &heatHazeShader, HEATHAZE_VERT, HEATHAZE_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("heatHazeWithMask", SHADER_HEATHAZEWITHMASK, &heatHazeWithMaskShader, HEATHAZEWITHMASK_VERT, HEATHAZEWITHMASK_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("heatHazeWithMaskAndVertex", SHADER_HEATHAZEWITHMASKANDVERTEX, &heatHazeWithMaskAndVertexShader, HEATHAZEWITHMASKANDVERTEX_VERT, HEATHAZEWITHMASKANDVERTEX_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("colorProcess", SHADER_COLORPROCESS, &colorProcessShader, COLORPROCESS_VERT, COLORPROCESS_FRAG, "", ""));

	// shadow mapping
#ifdef _SHADOW_MAPPING
#define GLSL2_SHADOW_MAPPING_2D_MACROS(type) r_useDepthTexture ? type ",_USING_DEPTH_TEXTURE" : (r_usePackColorAsDepth ? type ",_PACK_FLOAT" : type)
#define GLSL2_SHADOW_MAPPING_CUBE_MACROS(type) r_useCubeDepthTexture ? type ",_USING_DEPTH_TEXTURE" : (r_usePackColorAsDepth ? type ",_PACK_FLOAT" : type)
#define GLSL2_SHADOW_MAPPING_COLOR_MACROS(type) r_usePackColorAsDepth ? type ",_PACK_FLOAT" : type

	// shadow volume
	ret.Append(GLSL_SHADER_SOURCE("depth", SHADER_DEPTH, &depthShader, DEPTH_VERT, DEPTH_FRAG, "_USING_DEPTH_TEXTURE", ""));
	// perforated surface
	ret.Append(GLSL_SHADER_SOURCE("depthPerforated", SHADER_DEPTHPERFORATED, &depthPerforatedShader, DEPTH_PERFORATED_VERT, DEPTH_PERFORATED_FRAG, "_USING_DEPTH_TEXTURE", ""));
#ifdef GL_ES_VERSION_3_0
	if(!USING_GLES3 && (!r_useDepthTexture || !r_useCubeDepthTexture))
#else
	if(!r_useDepthTexture || !r_useCubeDepthTexture)
#endif
	{
		ret.Append(GLSL_SHADER_SOURCE("depthColor", SHADER_DEPTHCOLOR, &depthShader_color, DEPTH_VERT, DEPTH_FRAG, GLSL2_SHADOW_MAPPING_COLOR_MACROS(""), ""));
		ret.Append(GLSL_SHADER_SOURCE("depthPerforatedColor", SHADER_DEPTHPERFORATEDCOLOR, &depthPerforatedShader_color, DEPTH_PERFORATED_VERT, DEPTH_PERFORATED_FRAG, GLSL2_SHADOW_MAPPING_COLOR_MACROS(""), ""));
	}
	// point light
	ret.Append(GLSL_SHADER_SOURCE("interactionPointLightShadowMapping", SHADER_INTERACTIONPOINTLIGHT, &interactionShadowMappingShader_pointLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_CUBE_MACROS("_POINT_LIGHT"), "_POINT_LIGHT"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongPointLightShadowMapping", SHADER_INTERACTIONBLINNPHONGPOINTLIGHT, &interactionShadowMappingBlinnPhongShader_pointLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_CUBE_MACROS("_POINT_LIGHT,BLINN_PHONG"), "_POINT_LIGHT,BLINN_PHONG"));
	// parallel light
	ret.Append(GLSL_SHADER_SOURCE("interactionParallelLightShadowMapping", SHADER_INTERACTIONPARALLELLIGHT, &interactionShadowMappingShader_parallelLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_PARALLEL_LIGHT"), "_PARALLEL_LIGHT"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongParallelLightShadowMapping", SHADER_INTERACTIONBLINNPHONGPARALLELLIGHT, &interactionShadowMappingBlinnPhongShader_parallelLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_PARALLEL_LIGHT,BLINN_PHONG"), "_PARALLEL_LIGHT,BLINN_PHONG"));
	// spot light
	ret.Append(GLSL_SHADER_SOURCE("interactionSpotLightShadowMapping", SHADER_INTERACTIONSPOTLIGHT, &interactionShadowMappingShader_spotLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_SPOT_LIGHT"), "_SPOT_LIGHT"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongSpotLightShadowMapping", SHADER_INTERACTIONBLINNPHONGSPOTLIGHT, &interactionShadowMappingBlinnPhongShader_spotLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_SPOT_LIGHT,BLINN_PHONG"), "_SPOT_LIGHT,BLINN_PHONG"));
#endif

	// translucent stencil shadow
#ifdef _TRANSLUCENT_STENCIL_SHADOW
	ret.Append(GLSL_SHADER_SOURCE("interactionTranslucent", SHADER_INTERACTIONTRANSLUCENT, &interactionTranslucentShader, INTERACTION_TRANSLUCENT_VERT, INTERACTION_TRANSLUCENT_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongTranslucent", SHADER_INTERACTIONBLINNPHONGTRANSLUCENT, &interactionBlinnPhongTranslucentShader, INTERACTION_TRANSLUCENT_VERT, INTERACTION_TRANSLUCENT_FRAG, "BLINN_PHONG", "BLINN_PHONG"));
#endif
}

static bool RB_GLSL_CreateShaderProgram(shaderProgram_t *shaderProgram, const char *vert, const char *frag , const char *name);

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
	if(start < str.Length() - 1)
	{
		idStr s = str.Mid(start, str.Length() - start);
		ret.AddUnique(s);
		counter++;
	}

	return counter;
}

static idStr RB_GLSL_SetupPrecision(const char *source, const char *precision)
{
    if(!precision || !precision[0])
        return idStr(source);

    idStr res(source);
    res.Replace("precision", "// precision");

    int index = res.Find("#version");
    if(index == -1)
    SHADER_ERROR("[Harmattan]: GLSL shader source can not find '#version'\n.");

    index = res.Find('\n', index);
    if(index == -1 || index + 1 == res.Length())
    SHADER_ERROR("[Harmattan]: GLSL shader source '#version' not completed\n.");

    res.Insert(idStr("precision ") + precision + " float;\n", index + 1);

    //printf("%d|%s|\n%s\n", n, macros, res.c_str());
    return res;
}

static idStr RB_GLSL_ExpandMacros(const char *source, const char *macros)
{
	if(!macros || !macros[0])
		return idStr(source);

	idStrList list;
	int n = RB_GLSL_ParseMacros(macros, list);
	if(0 == n)
		return idStr(source);

	idStr res(source);
	int index = res.Find("#version");
	if(index == -1)
		SHADER_ERROR("[Harmattan]: GLSL shader source can not find '#version'\n.");

	index = res.Find('\n', index);
	if(index == -1 || index + 1 == res.Length())
		SHADER_ERROR("[Harmattan]: GLSL shader source '#version' not completed\n.");

	idStr m;
	m += "\n";
	for(int i = 0; i < list.Num(); i++)
	{
		m += "#define " + list[i] + "\n";
	}
	m += "\n";

	res.Insert(m.c_str(), index + 1);

	//printf("%d|%s|\n%s\n", n, macros, res.c_str());
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
/*
=================
RB_GLSL_LoadShader

loads GLSL vertex or fragment shaders
=================
*/
static void RB_GLSL_LoadShader(const char *name, shaderProgram_t *shaderProgram, GLenum type)
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

	fullPath.AppendPath(name);

	char	*fileBuffer;
	char	*buffer;

	if (!glConfig.isInitialized) {
		return;
	}

	// load the program even if we don't support it, so
	// fs_copyfiles can generate cross-platform data dumps
	fileSystem->ReadFile(fullPath.c_str(), (void **)&fileBuffer, NULL);

	common->Printf("Load GLSL shader file: %s -> %s\n", fullPath.c_str(), fileBuffer ? "success" : "fail");

	if (!fileBuffer) {
		return;
	}

	// copy to stack memory and free
	buffer = (char *)_alloca(strlen(fileBuffer) + 1);
	strcpy(buffer, fileBuffer);
	fileSystem->FreeFile(fileBuffer);

	switch (type) {
		case GL_VERTEX_SHADER:
			// create vertex shader
			shaderProgram->vertexShader = qglCreateShader(GL_VERTEX_SHADER);
			qglShaderSource(shaderProgram->vertexShader, 1, (const GLchar **)&buffer, 0);
			qglCompileShader(shaderProgram->vertexShader);
			break;
		case GL_FRAGMENT_SHADER:
			// create fragment shader
			shaderProgram->fragmentShader = qglCreateShader(GL_FRAGMENT_SHADER);
			qglShaderSource(shaderProgram->fragmentShader, 1, (const GLchar **)&buffer, 0);
			qglCompileShader(shaderProgram->fragmentShader);
			break;
		default:
			common->Printf("RB_GLSL_LoadShader: unexpected type\n");
			return;
	}
}

static void RB_GLSL_BindAttribLocations(GLuint program)
{
	qglBindAttribLocation(program, 8, "attr_TexCoord");
	qglBindAttribLocation(program, 9, "attr_Tangent");
	qglBindAttribLocation(program, 10, "attr_Bitangent");
	qglBindAttribLocation(program, 11, "attr_Normal");
	qglBindAttribLocation(program, 12, "attr_Vertex");
	qglBindAttribLocation(program, 13, "attr_Color");
}

/*
=================
RB_GLSL_LinkShader

links the GLSL vertex and fragment shaders together to form a GLSL program
=================
*/
static bool RB_GLSL_LinkShader(shaderProgram_t *shaderProgram, bool needsAttributes)
{
	char buf[BUFSIZ];
	int len;
	GLint status;
	GLint linked;

	if(!shaderProgram->vertexShader || !shaderProgram->fragmentShader)
		return false;

	shaderProgram->program = qglCreateProgram();

	qglAttachShader(shaderProgram->program, shaderProgram->vertexShader);
	qglAttachShader(shaderProgram->program, shaderProgram->fragmentShader);

	if (needsAttributes) {
		RB_GLSL_BindAttribLocations(shaderProgram->program);
	}

	qglLinkProgram(shaderProgram->program);

	qglGetProgramiv(shaderProgram->program, GL_LINK_STATUS, &linked);

	if (com_developer.GetBool()) {
		qglGetShaderInfoLog(shaderProgram->vertexShader, sizeof(buf), &len, buf);
		common->Printf("VS:\n%.*s\n", len, buf);
		qglGetShaderInfoLog(shaderProgram->fragmentShader, sizeof(buf), &len, buf);
		common->Printf("FS:\n%.*s\n", len, buf);
	}

	if (!linked) {
		common->Printf("RB_GLSL_LinkShader: program failed to link\n");
		qglGetProgramInfoLog(shaderProgram->program, sizeof(buf), NULL, buf);
		common->Printf("RB_GLSL_LinkShader:\n%.*s\n", len, buf);
		return false;
	}

	return true;
}

/*
=================
RB_GLSL_ValidateProgram

makes sure GLSL program is valid
=================
*/
static bool RB_GLSL_ValidateProgram(shaderProgram_t *shaderProgram)
{
	GLint validProgram;

	qglValidateProgram(shaderProgram->program);

	qglGetProgramiv(shaderProgram->program, GL_VALIDATE_STATUS, &validProgram);

	if (!validProgram) {
		common->Printf("RB_GLSL_ValidateProgram: program invalid\n");
		return false;
	}

	return true;
}

static void RB_GLSL_GetUniformLocations(shaderProgram_t *shader)
{
	int	i;
	char	buffer[32];

	GL_UseProgram(shader);

	// get uniform location
	shader->localLightOrigin = GL_GetUniformLocation(shader->program, "u_lightOrigin");
	shader->localViewOrigin = GL_GetUniformLocation(shader->program, "u_viewOrigin");
	shader->lightProjectionS = GL_GetUniformLocation(shader->program, "u_lightProjectionS");
	shader->lightProjectionT = GL_GetUniformLocation(shader->program, "u_lightProjectionT");
	shader->lightProjectionQ = GL_GetUniformLocation(shader->program, "u_lightProjectionQ");
	shader->lightFalloff = GL_GetUniformLocation(shader->program, "u_lightFalloff");
	shader->bumpMatrixS = GL_GetUniformLocation(shader->program, "u_bumpMatrixS");
	shader->bumpMatrixT = GL_GetUniformLocation(shader->program, "u_bumpMatrixT");
	shader->diffuseMatrixS = GL_GetUniformLocation(shader->program, "u_diffuseMatrixS");
	shader->diffuseMatrixT = GL_GetUniformLocation(shader->program, "u_diffuseMatrixT");
	shader->specularMatrixS = GL_GetUniformLocation(shader->program, "u_specularMatrixS");
	shader->specularMatrixT = GL_GetUniformLocation(shader->program, "u_specularMatrixT");
	shader->colorModulate = GL_GetUniformLocation(shader->program, "u_colorModulate");
	shader->colorAdd = GL_GetUniformLocation(shader->program, "u_colorAdd");
	shader->diffuseColor = GL_GetUniformLocation(shader->program, "u_diffuseColor");
	shader->specularColor = GL_GetUniformLocation(shader->program, "u_specularColor");
	shader->glColor = GL_GetUniformLocation(shader->program, "u_glColor");
	shader->alphaTest = GL_GetUniformLocation(shader->program, "u_alphaTest");
	shader->specularExponent = GL_GetUniformLocation(shader->program, "u_specularExponent");

	shader->eyeOrigin = GL_GetUniformLocation(shader->program, "u_eyeOrigin");
	shader->localEyeOrigin = GL_GetUniformLocation(shader->program, "u_localEyeOrigin");
	shader->nonPowerOfTwo = GL_GetUniformLocation(shader->program, "u_nonPowerOfTwo");
	shader->windowCoords = GL_GetUniformLocation(shader->program, "u_windowCoords");

	shader->modelViewProjectionMatrix = GL_GetUniformLocation(shader->program, "u_modelViewProjectionMatrix");

	shader->modelMatrix = GL_GetUniformLocation(shader->program, "u_modelMatrix");
	shader->textureMatrix = GL_GetUniformLocation(shader->program, "u_textureMatrix");
	//k: add modelView matrix uniform
	shader->modelViewMatrix = GL_GetUniformLocation(shader->program, "u_modelViewMatrix");
	shader->projectionMatrix = GL_GetUniformLocation(shader->program, "u_projectionMatrix");
	//k: add clip plane uniform
	shader->clipPlane = GL_GetUniformLocation(shader->program, "u_clipPlane");
	//k: add fog matrix uniform
	shader->fogMatrix = GL_GetUniformLocation(shader->program, "u_fogMatrix");
	//k: add fog color uniform
	shader->fogColor = GL_GetUniformLocation(shader->program, "u_fogColor");
	//k: add texgen S T Q uniform
	shader->texgenS = GL_GetUniformLocation(shader->program, "u_texgenS");
	shader->texgenT = GL_GetUniformLocation(shader->program, "u_texgenT");
	shader->texgenQ = GL_GetUniformLocation(shader->program, "u_texgenQ");

	for (i = 0; i < MAX_VERTEX_PARMS; i++) {
		idStr::snPrintf(buffer, sizeof(buffer), "u_vertexParm%d", i);
		shader->u_vertexParm[i] = GL_GetUniformLocation(shader->program, buffer);
	}

	for (i = 0; i < MAX_FRAGMENT_IMAGES; i++) {
		idStr::snPrintf(buffer, sizeof(buffer), "u_fragmentMap%d", i);
		shader->u_fragmentMap[i] = GL_GetUniformLocation(shader->program, buffer);
		if(shader->u_fragmentMap[i] != -1)
			qglUniform1i(shader->u_fragmentMap[i], i);
	}

	//k: add cubemap texture units
	for ( i = 0; i < MAX_FRAGMENT_IMAGES; i++ ) {
		idStr::snPrintf(buffer, sizeof(buffer), "u_fragmentCubeMap%d", i);
		shader->u_fragmentCubeMap[i] = GL_GetUniformLocation(shader->program, buffer);
		if(shader->u_fragmentCubeMap[i] != -1)
			qglUniform1i(shader->u_fragmentCubeMap[i], i);
	}

	for (i = 0; i < MAX_UNIFORM_PARMS; i++) {
		idStr::snPrintf(buffer, sizeof(buffer), "u_uniformParm%d", i);
		shader->u_uniformParm[i] = GL_GetUniformLocation(shader->program, buffer);
	}

#ifdef _SHADOW_MAPPING
	shader->shadowMVPMatrix = GL_GetUniformLocation(shader->program, "shadowMVPMatrix");
    shader->globalLightOrigin = GL_GetUniformLocation(shader->program, "globalLightOrigin");
    shader->bias = GL_GetUniformLocation(shader->program, "bias");
#endif

	// get attribute location
	shader->attr_TexCoord = GL_GetAttribLocation(shader->program, "attr_TexCoord");
	shader->attr_Tangent = GL_GetAttribLocation(shader->program, "attr_Tangent");
	shader->attr_Bitangent = GL_GetAttribLocation(shader->program, "attr_Bitangent");
	shader->attr_Normal = GL_GetAttribLocation(shader->program, "attr_Normal");
	shader->attr_Vertex = GL_GetAttribLocation(shader->program, "attr_Vertex");
	shader->attr_Color = GL_GetAttribLocation(shader->program, "attr_Color");

	GL_CheckErrors();

	GL_UseProgram(NULL);
}

static const GLSLShaderProp * RB_GLSL_FindShaderProp(const idList<GLSLShaderProp> &Props, int type)
{
	for(int i = 0; i <= SHADER_CUSTOM; i++)
	{
		const GLSLShaderProp &prop = Props[i];
		if(prop.type == type)
			return &prop;
	}
#ifdef _SHADOW_MAPPING
    if(type == SHADER_DEPTHCOLOR || type == SHADER_DEPTHPERFORATEDCOLOR)
    {
#ifdef GL_ES_VERSION_3_0
        if(!USING_GLES3 && (!r_useDepthTexture || !r_useCubeDepthTexture))
#else
        if(!r_useDepthTexture || !r_useCubeDepthTexture)
#endif
            common->Error("Shader prop '%d' not found!\n", type);
    }
    else
#endif
    common->Error("Shader prop '%d' not found!\n", type);
	return NULL;
}

static bool RB_GLSL_InitShaders(void)
{
	idList<GLSLShaderProp> Props;
	RB_GLSL_GetShaderSources(Props);

	// base shader
	REQUIRE_SHADER;
	for(int i = SHADER_BASE_BEGIN; i <= SHADER_BASE_END; i++)
	{
		const GLSLShaderProp *prop = RB_GLSL_FindShaderProp(Props, i);
        if(!prop)
            continue;
		if(!RB_GLSL_LoadShaderProgramFromProp(prop))
			return false;
		shaderManager->Add(prop->program);
	}

	// newStage shader
	UNNECESSARY_SHADER;
	for(int i = SHADER_NEW_STAGE_BEGIN; i <= SHADER_NEW_STAGE_END; i++)
	{
		const GLSLShaderProp *prop = RB_GLSL_FindShaderProp(Props, i);
        if(!prop)
            continue;
		if(!RB_GLSL_LoadShaderProgramFromProp(prop))
		{
			common->Printf("[Harmattan]: newStage %d not support!\n", i);
            continue;
		}
		shaderManager->Add(prop->program);
	}
	REQUIRE_SHADER;

#ifdef _SHADOW_MAPPING
	UNNECESSARY_SHADER;
	for(int i = SHADER_SHADOW_MAPPING_BEGIN; i <= SHADER_SHADOW_MAPPING_END; i++)
	{
		const GLSLShaderProp *prop = RB_GLSL_FindShaderProp(Props, i);
        if(!prop)
            continue;
		if(!RB_GLSL_LoadShaderProgramFromProp(prop))
		{
			common->Printf("[Harmattan]: not support shadow mapping!\n");
			if(r_useShadowMapping.GetBool())
			{
				r_useShadowMapping.SetBool(false);
			}
			r_useShadowMapping.SetReadonly();
			break;
		}
		shaderManager->Add(prop->program);
	}
	REQUIRE_SHADER;
#endif

#ifdef _TRANSLUCENT_STENCIL_SHADOW
	UNNECESSARY_SHADER;
	for(int i = SHADER_STENCIL_SHADOW_BEGIN; i <= SHADER_STENCIL_SHADOW_END; i++)
	{
		const GLSLShaderProp *prop = RB_GLSL_FindShaderProp(Props, i);
        if(!prop)
            continue;
		if(!RB_GLSL_LoadShaderProgramFromProp(prop))
		{
			common->Printf("[Harmattan]: translucent stencil shadow shader error!\n");
			if(harm_r_stencilShadowTranslucent.GetBool())
			{
				harm_r_stencilShadowTranslucent.SetBool(false);
			}
			harm_r_stencilShadowTranslucent.SetReadonly();
			break;
		}
		shaderManager->Add(prop->program);
	}
	REQUIRE_SHADER;
#endif

	return true;
}

void R_ReloadGLSLPrograms_f(const idCmdArgs &args)
{
	int		i;

	common->Printf("----- R_ReloadGLSLPrograms -----\n");

	if (!RB_GLSL_InitShaders()) {
		common->Printf("GLSL shaders failed to init.\n");
	}

	glConfig.allowGLSLPath = true;

	common->Printf("-------------------------------\n");
}



static void RB_GLSL_DeleteShaderProgram(shaderProgram_t *shaderProgram)
{
	if(shaderProgram->program)
	{
		if(qglIsProgram(shaderProgram->program))
			qglDeleteProgram(shaderProgram->program);
	}

	if(shaderProgram->vertexShader)
	{
		if(qglIsShader(shaderProgram->vertexShader))
			qglDeleteShader(shaderProgram->vertexShader);
	}

	if(shaderProgram->fragmentShader)
	{
		if(qglIsShader(shaderProgram->fragmentShader))
			qglDeleteShader(shaderProgram->fragmentShader);
	}
	memset(shaderProgram, 0, sizeof(shaderProgram_t));
}

static GLint RB_GLSL_CreateShader(GLenum type, const char *source)
{
	GLint shader = 0;
	GLint status;

	shader = qglCreateShader(type);
	if(shader == 0)
	{
		SHADER_ERROR("[Harmattan]: %s::glCreateShader(%s) error!\n", __func__, type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : "GL_FRAGMENT_SHADER");
		return 0;
	}

	qglShaderSource(shader, 1, (const GLchar **)&source, 0);
	qglCompileShader(shader);

	qglGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if(!status)
	{
		GLchar log[LOG_LEN];
		qglGetShaderInfoLog(shader, sizeof(GLchar) * LOG_LEN, NULL, log);
		SHADER_ERROR("[Harmattan]: %s::glCompileShader(%s) -> %s!\n", __func__, type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : "GL_FRAGMENT_SHADER", log);
		qglDeleteShader(shader);
		shader = 0;
	}

	return shader;
}

static GLint RB_GLSL_CreateProgram(GLint vertShader, GLint fragShader, bool needsAttributes = true)
{
	GLint program = 0;
	GLint result;

	program = qglCreateProgram();
	if(program == 0)
	{
		SHADER_ERROR("[Harmattan]: %s::glCreateProgram() error!\n", __func__);
		return 0;
	}

	qglAttachShader(program, vertShader);
	qglAttachShader(program, fragShader);

	if(needsAttributes)
	{
		RB_GLSL_BindAttribLocations(program);
	}

	qglLinkProgram(program);
	qglGetProgramiv(program, GL_LINK_STATUS, &result);
	if(!result)
	{
		GLchar log[LOG_LEN];
		qglGetProgramInfoLog(program, sizeof(GLchar) * LOG_LEN, NULL, log);
		SHADER_ERROR("[Harmattan]: %s::glLinkProgram() -> %s!\n", __func__, log);
		qglDeleteProgram(program);
		program = 0;
	}

	qglValidateProgram(program);
	qglGetProgramiv(program, GL_VALIDATE_STATUS, &result);
	if(!result)
	{
		GLchar log[LOG_LEN];
		qglGetProgramInfoLog(program, sizeof(GLchar) * LOG_LEN, NULL, log);
		SHADER_ERROR("[Harmattan]: %s::glValidateProgram() -> %s!\n", __func__, log);
#if 0
		qglDeleteProgram(program);
		program = 0;
#endif
	}

	return program;
}

bool RB_GLSL_CreateShaderProgram(shaderProgram_t *shaderProgram, const char *vert, const char *frag , const char *name, int type)
{
#ifdef _DEBUG_VERT_SHADER_SOURCE
	Sys_Printf("---------- Vertex shader source: ----------\n");
	Sys_Printf(vert);
	Sys_Printf("--------------------------------------------------\n");
#endif
#ifdef _DEBUG_FRAG_SHADER_SOURCE
	Sys_Printf("---------- Fragment shader source: ----------\n");
	Sys_Printf(frag);
	Sys_Printf("--------------------------------------------------\n");
#endif
	RB_GLSL_DeleteShaderProgram(shaderProgram);
	shaderProgram->vertexShader = RB_GLSL_CreateShader(GL_VERTEX_SHADER, vert);
	if(shaderProgram->vertexShader == 0)
		return false;

	shaderProgram->fragmentShader = RB_GLSL_CreateShader(GL_FRAGMENT_SHADER, frag);
	if(shaderProgram->fragmentShader == 0)
	{
		RB_GLSL_DeleteShaderProgram(shaderProgram);
		return false;
	}

	shaderProgram->program = RB_GLSL_CreateProgram(shaderProgram->vertexShader, shaderProgram->fragmentShader);
	if(shaderProgram->program == 0)
	{
		RB_GLSL_DeleteShaderProgram(shaderProgram);
		return false;
	}

	RB_GLSL_GetUniformLocations(shaderProgram);
	strncpy(shaderProgram->name, name, sizeof(shaderProgram->name));
    shaderProgram->type = type;

	return true;
}

int RB_GLSL_LoadShaderProgram(
		const char *name,
        int type,
		shaderProgram_t * const program,
		const char *default_vertex_shader_source,
		const char *default_fragment_shader_source,
		const char *vertex_shader_source_file,
		const char *fragment_shader_source_file,
		const char *macros
		)
{
	memset(program, 0, sizeof(shaderProgram_t));

	common->Printf("[Harmattan]: Load GLSL shader program: %s\n", name);

	common->Printf("[Harmattan]: 1. Load external shader source: Vertex(%s), Fragment(%s)\n", vertex_shader_source_file, fragment_shader_source_file);
	RB_GLSL_LoadShader(vertex_shader_source_file, program, GL_VERTEX_SHADER);
	RB_GLSL_LoadShader(fragment_shader_source_file, program, GL_FRAGMENT_SHADER);

	if (!RB_GLSL_LinkShader(program, true)/* && !RB_GLSL_ValidateProgram(program)*/) {
		common->Printf("[Harmattan]: 2. Load internal shader source\n");
		idStr vs = RB_GLSL_ExpandMacros(default_vertex_shader_source, macros);
		idStr fs = RB_GLSL_ExpandMacros(default_fragment_shader_source, macros);
		if(!RB_GLSL_CreateShaderProgram(program, vs.c_str(), fs.c_str(), name, type))
		{
			SHADER_ERROR("[Harmattan]: Load internal shader program fail!\n");
			return -1;
		}
		else
		{
			common->Printf("[Harmattan]: Load internal shader program success!\n\n");
			return 2;
		}
	} else {
		RB_GLSL_ValidateProgram(program);
		RB_GLSL_GetUniformLocations(program);
		common->Printf("[Harmattan]: Load external shader program success!\n\n");
		strncpy(program->name, name, sizeof(program->name));
        program->type = type;
		return 1;
	}
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

	common->Printf("[Harmattan]: Save GLSL shader source to '%s'\n", path.c_str());

	for(int i = 0; i < Props.Num(); i++)
	{
		const GLSLShaderProp &prop = Props[i];
		if(target.Num() > 0 && target.FindIndex(prop.name) < 0)
			continue;

		vs = prop.default_vertex_shader_source.c_str();
		fs = prop.default_fragment_shader_source.c_str();
		macros = prop.macros.c_str();

		idStr vsSrc = RB_GLSL_ExpandMacros(vs, macros);
		idStr p(path);
		p.Append(prop.vertex_shader_source_file);
		fileSystem->WriteFile(p.c_str(), vsSrc.c_str(), vsSrc.Length(), "fs_basepath");
		common->Printf("GLSL vertex shader: '%s'\n", p.c_str());

		idStr fsSrc = RB_GLSL_ExpandMacros(fs, macros);
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
	    common->Printf(str.c_str());
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

		idStr vsSrc = RB_GLSL_ExpandMacros(vs, macros);
		common->Printf("  Vertex shader: \n");
		R_PrintGLSLShaderSource(vsSrc);
		common->Printf("\n");

		idStr fsSrc = RB_GLSL_ExpandMacros(fs, macros);
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
 * 	ftransform() -> u_modelViewProjectionMatrix * attr_Vertex
 * 	gl_Vertex -> attr_Vertex
 * 	gl_MultiTexCoord0 -> attr_TexCoord
 * 	gl_Color -> (attr_Color / 255.0)
 * 	gl_Normal -> attr_Normal
 *
 * ES3.0
 * 	+ #version <version> es
 * 	+ precision mediump float;
 * 	+ in highp vec4 attr_Vertex;
 * 	+ in highp vec4 attr_TexCoord;
 * 	+ in lowp vec4 attr_Color;
 * 	+ in vec3 attr_Normal;
 *	attribute -> in
 *	varying -> out
 * 	ftransform() -> u_modelViewProjectionMatrix * attr_Vertex
 * 	gl_Vertex -> attr_Vertex
 * 	gl_MultiTexCoord0 -> attr_TexCoord
 * 	gl_Color -> (attr_Color / 255.0)
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
	source.Replace("gl_Color", "(attr_Color / 255.0)");
	source.Replace("gl_Normal", "attr_Normal");

	idStr ret;
	ret += "#version ";
	ret += ver;
	ret += "\n";
	ret += "//#pragma optimize(off)\n";
	ret += "\n";
	ret += "precision mediump float;\n";
	ret += "\n";

	ret += attribute + " highp vec4 attr_Vertex;\n";
	ret += attribute + " highp vec4 attr_TexCoord;\n";
	ret += attribute + " lowp vec4 attr_Color;\n";
	ret += attribute + " vec3 attr_Normal;\n";
	ret += "\n";

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
	ret += "precision mediump float;\n";
	ret += "\n";

	ret += out;

	ret += source;

	return ret;
}
