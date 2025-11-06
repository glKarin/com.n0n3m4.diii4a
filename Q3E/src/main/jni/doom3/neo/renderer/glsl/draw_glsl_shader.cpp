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
GLint GL_GetAttribLocation(GLuint program, const char *name)
{
    GLint attribLocation = qglGetAttribLocation(program, name);
	Sys_Printf("GL_GetAttribLocation(%s) -> %d\n", name, attribLocation);
	return attribLocation;
}
GLint GL_GetUniformLocation(GLuint program, const char *name)
{
	GLint uniformLocation = qglGetUniformLocation(program, name);
	Sys_Printf("GL_GetUniformLocation(%s) -> %d\n", name, uniformLocation);
	return uniformLocation;
}
#endif

#ifdef GL_ES_VERSION_3_0
static idCVar harm_r_useGLSLShaderBinaryCache("harm_r_useGLSLShaderBinaryCache", "0", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE, "Use/cache GLSL shader compiled binary(OpenGL ES2.0 not support)");
#endif

#define _GLPROGS "glslprogs" // "gl2progs"
static idCVar	harm_r_shaderProgramDir("harm_r_shaderProgramDir", "", CVAR_RENDERER | CVAR_INIT, "Setup external OpenGLES2 GLSL shader program directory path(default is empty, means using `" _GLPROGS "`).");

#ifdef GL_ES_VERSION_3_0
#define _GLBINPROGS "glslbinprogs"
static idCVar	harm_r_shaderProgramBinaryDir("harm_r_shaderProgramBinaryDir", "", CVAR_RENDERER | CVAR_INIT, "Setup external OpenGLES2 GLSL shader program binary directory path(default is empty, means using `" _GLBINPROGS "`).");

#define _GL3PROGS "glsl3progs"
static idCVar	harm_r_shaderProgramES3Dir("harm_r_shaderProgramES3Dir", "", CVAR_RENDERER | CVAR_INIT, "Setup external OpenGLES3 GLSL shader program directory path(default is empty, means using `" _GL3PROGS "`).");

#define _GL3BINPROGS "glsl3binprogs"
static idCVar	harm_r_shaderProgramBinaryES3Dir("harm_r_shaderProgramBinaryES3Dir", "", CVAR_RENDERER | CVAR_INIT, "Setup external OpenGLES3 GLSL shader program binary directory path(default is empty, means using `" _GL3BINPROGS "`).");
#endif

static bool glslInitialized = false;
static bool reloadGLSLShaders = false;
static bool shaderRequired = true;
#define REQUIRE_SHADER shaderRequired = true;
#define UNNECESSARY_SHADER shaderRequired = false;

#ifdef _SHADOW_MAPPING
extern bool r_useDepthTexture;
extern bool r_useCubeDepthTexture;
extern bool r_usePackColorAsDepth;
#endif

#ifdef _MULTITHREAD
void RB_GLSL_HandleShaders(void)
{
    if(!multithreadActive)
        return;
    shaderManager->ActuallyLoad();
    if(reloadGLSLShaders)
    {
        shaderManager->ReloadShaders();
        reloadGLSLShaders = false;
    }
}
#endif

static void RB_GLSL_PrintShaderSource(const char *filename, const char *source);
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
static void RB_GLSL_DeleteShaderProgram(shaderProgram_t *shaderProgram, bool deleteProgram = true);

static bool RB_GLSL_LoadShaderProgramFromProp(const GLSLShaderProp *prop)
{
    if (!glConfig.isInitialized) {
        return false;
    }

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

#define GLSL_BUILTIN_SHADER_INDEX_TO_HANDLE(x) ( (x) + 1 )
#define GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(x) ( -( (x) + 1) )
#define GLSL_BUILTIN_SHADER_HANDLE_TO_INDEX(x) ( (x) - 1 )
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
	common->Printf("idGLSLShaderManager::Add shader program '%s' -> %d.\n", shader->name, shader->program);
	return shaders.Append(shader);
}

const shaderProgram_t * idGLSLShaderManager::Find(const char *name) const
{
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(!idStr::Icmp(name, shader->name))
        {
            common->Printf("GLSL shader manager::Find '%s' -> %d, type=%d %s.\n", shader->name, shader->program, shader->type, shader->type == SHADER_CUSTOM ? "custom" : "built-in");
            return shader;
        }
	}
	common->Printf("GLSL shader manager::Find '%s' -> NOT FOUND.\n", name);
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
			common->Printf("GLSL shader manager::FindIndex '%s' -> %d %s.\n", shader->name, shader->type, shader->type == SHADER_CUSTOM ? "custom" : "built-in");
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
		index = GLSL_BUILTIN_SHADER_HANDLE_TO_INDEX(handle);
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

    index = FindCustomIndex(name);
    if(index >= 0)
        return GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(index);

    index = FindIndex(name);
    if(index >= 0)
        return GLSL_BUILTIN_SHADER_INDEX_TO_HANDLE(index);

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

GLSLShaderProp * idGLSLShaderManager::FindCustom(const char *name, int *index)
{
	for(int i = 0; i < customShaders.Num(); i++)
	{
		GLSLShaderProp &p = customShaders[i];
		if(!idStr::Icmp(name, p.name.c_str()))
        {
            if(index)
                *index = i;
            return &p;
        }
	}
    if(index)
        *index = -1;
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
		common->Printf("GLSL shader manager::Load shader '%s' has loaded.\n", inProp.name.c_str());
		return GLSL_BUILTIN_SHADER_INDEX_TO_HANDLE(index);
	}

	// check has custom loaded
	prop = FindCustom(inProp.name.c_str(), &index);
	if(prop)
	{
		if(prop->program)
		{
            if(prop->program->program > 0)
            {
                common->Printf("GLSL shader manager::Load custom shader '%s' has already loaded.\n", inProp.name.c_str());
                return GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(index);
            }
            else
            {
                common->Printf("GLSL shader manager::Load custom shader '%s' has already load failed.\n", inProp.name.c_str());
                return INVALID_SHADER_HANDLE;
            }
		}
		else
		{
			common->Printf("GLSL shader manager::Load custom shader '%s' wait loading.\n", inProp.name.c_str());
            return GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(index);
		}
	}

	// add to queue
	GLSLShaderProp p = inProp;
	p.type = SHADER_CUSTOM;
	p.program = NULL;
	index = customShaders.Append(p);
	// queue.Append(index);
	common->Printf("GLSL shader manager::Load shader push '%s' into queue.\n", p.name.c_str());

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
	const unsigned int num = customShaders.Num();

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
			common->Warning("GLSL shader manager::ActuallyLoad custom shader index '%d' over( >= %d ).", index, customShaders.Num());
			continue;
		}*/

		GLSLShaderProp &prop = customShaders[index];
		index++;

		if(FindIndex(prop.name.c_str()) >= 0)
		{
			common->Warning("GLSL shader manager::ActuallyLoad custom shader '%s' has loaded.", prop.name.c_str());
			continue;
		}
		if(prop.program)
		{
			common->Warning("GLSL shader manager::ActuallyLoad custom shader '%s' has handle.", prop.name.c_str());
			continue;
		}

		// create shader on heap
		shaderProgram_t *shader = (shaderProgram_t *)malloc(sizeof(*shader));
		memset(shader, 0, sizeof(*shader));
        idStr::Copynz(shader->name, prop.name.c_str(), sizeof(shader->name));
		shader->type = SHADER_CUSTOM;
		prop.program = shader;

		common->Printf("GLSL shader manager::ActuallyLoad shader '%s'.\n", prop.name.c_str());
		if(RB_GLSL_LoadShaderProgramFromProp(&prop))
		{
			Add(shader);
		}
		else
		{
			memset(prop.program, 0, sizeof(*prop.program));
			common->Warning("GLSL shader manager::ActuallyLoad shader '%s' error!", prop.name.c_str());
		}
	}
	shaderRequired = B;
	queueCurrentIndex = index;
}

idGLSLShaderManager::~idGLSLShaderManager()
{
}

void idGLSLShaderManager::Shutdown(void)
{
    printf("GLSL shader manager destroying: %d shaders, %d customer shaders\n", shaders.Num(), customShaders.Num());
    // stop load queue;
    queueCurrentIndex = customShaders.Num();
    // delete shader programs
    for(int i = 0; i < shaders.Num(); i++)
    {
        shaderProgram_t *shader = shaders[i];
        if(shader)
        {
            RB_GLSL_DeleteShaderProgram(shader, true);
        }
    }
    shaders.Clear();
    // clear load queue
    for(int i = 0; i < customShaders.Num(); i++)
    {
        GLSLShaderProp &prop = customShaders[i];
        if(prop.program)
        {
            free(prop.program);
            prop.program = NULL;
        }
    }
    queueCurrentIndex = 0;
    customShaders.Clear();
    printf("GLSL shader manager shutdown\n");
}

idGLSLShaderManager idGLSLShaderManager::_shaderManager;

idGLSLShaderManager *shaderManager = &idGLSLShaderManager::_shaderManager;

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
	ret.Append(GLSL_SHADER_SOURCE("interaction", SHADER_INTERACTION, &interactionShader, INTERACTION_VERT, INTERACTION_FRAG, "PHONG", "PHONG"));
	ret.Append(GLSL_SHADER_SOURCE("shadow", SHADER_SHADOW, &shadowShader, SHADOW_VERT, SHADOW_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("default", SHADER_DEFAULT, &defaultShader, DEFAULT_VERT, DEFAULT_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("zfill", SHADER_ZFILL, &depthFillShader, ZFILL_VERT, ZFILL_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("zfillClip", SHADER_ZFILLCLIP, &depthFillClipShader, ZFILLCLIP_VERT, ZFILLCLIP_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("cubemap", SHADER_CUBEMAP, &cubemapShader, CUBEMAP_VERT, CUBEMAP_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("environment", SHADER_ENVIRONMENT, &environmentShader, ENVIRONMENT_VERT, ENVIRONMENT_FRAG, "", ""));
    ret.Append(GLSL_SHADER_SOURCE("bumpyEnvironment", SHADER_BUMPY_ENVIRONMENT, &bumpyEnvironmentShader, BUMPY_ENVIRONMENT_VERT, BUMPY_ENVIRONMENT_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("fog", SHADER_FOG, &fogShader, FOG_VERT, FOG_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("blendLight", SHADER_BLENDLIGHT, &blendLightShader, BLENDLIGHT_VERT, FOG_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("interactionPBR", SHADER_INTERACTION_PBR, &interactionPBRShader, INTERACTION_VERT, INTERACTION_FRAG, "_PBR", "_PBR"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphong", SHADER_INTERACTION_BLINNPHONG, &interactionBlinnPhongShader, INTERACTION_VERT, INTERACTION_FRAG, "BLINN_PHONG", "BLINN_PHONG"));
    ret.Append(GLSL_SHADER_SOURCE("ambientLighting", SHADER_AMBIENT_LIGHTING, &ambientLightingShader, INTERACTION_VERT, INTERACTION_FRAG, "_AMBIENT", "_AMBIENT"));
#ifdef _GLOBAL_ILLUMINATION
    ret.Append(GLSL_SHADER_SOURCE("globalIllumination", SHADER_GLOBAL_ILLUMINATION, &globalIlluminationShader, GLOBAL_ILLUMINATION_VERT, GLOBAL_ILLUMINATION_FRAG, "_BFG", "_BFG"));
#endif
	ret.Append(GLSL_SHADER_SOURCE("diffuseCubemap", SHADER_DIFFUSECUBEMAP, &diffuseCubemapShader, DIFFUSE_CUBEMAP_VERT, CUBEMAP_FRAG, "", ""));
	// ret.Append(GLSL_SHADER_SOURCE("glasswarp", SHADER_GLASSWARP, &glasswarpShader, GLASSWARP_VERT, GLASSWARP_FRAG, "", ""));
#ifdef GL_ES_VERSION_3_0
    if(USING_GLES3 || glConfig.depthTextureAvailable)
#else
    if(glConfig.depthTextureAvailable)
#endif
    ret.Append(GLSL_SHADER_SOURCE("depthToColor", SHADER_DEPTH_TO_COLOR, &depthToColorShader, SIMPLE_VERTEX_TEXCOORD_VERT, DEPTH_TO_COLOR_FRAG, "", ""));
#ifdef GL_ES_VERSION_3_0
    if(USING_GLES3)
    ret.Append(GLSL_SHADER_SOURCE("stencilToColor", SHADER_STENCIL_TO_COLOR, &stencilToColorShader, SIMPLE_VERTEX_TEXCOORD_VERT, STENCIL_TO_COLOR_FRAG, "", ""));
#endif
	ret.Append(GLSL_SHADER_SOURCE("texgen", SHADER_TEXGEN, &texgenShader, TEXGEN_VERT, TEXGEN_FRAG, "", ""));

    // newStage
	ret.Append(GLSL_SHADER_SOURCE("heatHaze", SHADER_HEATHAZE, &heatHazeShader, HEATHAZE_VERT, HEATHAZE_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("heatHazeWithMask", SHADER_HEATHAZE_WITH_MASK, &heatHazeWithMaskShader, HEATHAZEWITHMASK_VERT, HEATHAZEWITHMASK_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("heatHazeWithMaskAndVertex", SHADER_HEATHAZE_WITH_MASK_AND_VERTEX, &heatHazeWithMaskAndVertexShader, HEATHAZEWITHMASKANDVERTEX_VERT, HEATHAZEWITHMASKANDVERTEX_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("colorProcess", SHADER_COLORPROCESS, &colorProcessShader, COLORPROCESS_VERT, COLORPROCESS_FRAG, "", ""));
	// D3XP
	ret.Append(GLSL_SHADER_SOURCE("enviroSuit", SHADER_ENVIROSUIT, &enviroSuitShader, ENVIROSUIT_VERT, ENVIROSUIT_FRAG, "", ""));
#ifdef _HUMANHEAD
    ret.Append(GLSL_SHADER_SOURCE("screeneffect", SHADER_SCREENEFFECT, &screeneffectShader, SCREENEFFECT_VERT, SCREENEFFECT_FRAG, "", ""));
    ret.Append(GLSL_SHADER_SOURCE("radialblur", SHADER_RADIALBLUR, &radialblurShader, RADIALBLUR_VERT, RADIALBLUR_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("liquid", SHADER_LIQUID, &liquidShader, LIQUID_VERT, LIQUID_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("membrane", SHADER_MEMBRANE, &membraneShader, MEMBRANE_VERT, MEMBRANE_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("screenprocess", SHADER_SCREENPROCESS, &screenprocessShader, SCREENPROCESS_VERT, SCREENPROCESS_FRAG, "", ""));
#endif
    ret.Append(GLSL_SHADER_SOURCE("megaTexture", SHADER_MEGATEXTURE, &megaTextureShader, MEGATEXTURE_VERT, MEGATEXTURE_FRAG, "", ""));

	// shadow mapping
#ifdef _SHADOW_MAPPING
#define GLSL2_SHADOW_MAPPING_2D_MACROS(type) r_useDepthTexture ? type ",_USING_DEPTH_TEXTURE" : (r_usePackColorAsDepth ? type ",_PACK_FLOAT" : type)
#define GLSL2_SHADOW_MAPPING_CUBE_MACROS(type) r_useCubeDepthTexture ? type ",_USING_DEPTH_TEXTURE" : (r_usePackColorAsDepth ? type ",_PACK_FLOAT" : type)
#define GLSL2_SHADOW_MAPPING_COLOR_MACROS(type) r_usePackColorAsDepth ? type ",_PACK_FLOAT" : type

	// shadow volume
	ret.Append(GLSL_SHADER_SOURCE("depth", SHADER_DEPTH, &depthShader, DEPTH_VERT, DEPTH_FRAG, "_USING_DEPTH_TEXTURE", ""));
	// perforated surface
	ret.Append(GLSL_SHADER_SOURCE("depthPerforated", SHADER_DEPTH_PERFORATED, &depthPerforatedShader, DEPTH_PERFORATED_VERT, DEPTH_PERFORATED_FRAG, "_USING_DEPTH_TEXTURE", ""));
#ifdef GL_ES_VERSION_3_0
	if(!USING_GLES3 && (!r_useDepthTexture || !r_useCubeDepthTexture))
#else
	if(!r_useDepthTexture || !r_useCubeDepthTexture)
#endif
	{
		ret.Append(GLSL_SHADER_SOURCE("depthColor", SHADER_DEPTH_COLOR, &depthShader_color, DEPTH_VERT, DEPTH_FRAG, GLSL2_SHADOW_MAPPING_COLOR_MACROS(""), ""));
		ret.Append(GLSL_SHADER_SOURCE("depthPerforatedColor", SHADER_DEPTH_PERFORATED_COLOR, &depthPerforatedShader_color, DEPTH_PERFORATED_VERT, DEPTH_PERFORATED_FRAG, GLSL2_SHADOW_MAPPING_COLOR_MACROS(""), ""));
	}
	// point light
	ret.Append(GLSL_SHADER_SOURCE("interactionPointLightShadowMapping", SHADER_INTERACTION_POINT_LIGHT, &interactionShadowMappingShader_pointLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_CUBE_MACROS("_POINT_LIGHT,PHONG"), "_POINT_LIGHT,PHONG"));
	ret.Append(GLSL_SHADER_SOURCE("interactionPBRPointLightShadowMapping", SHADER_INTERACTION_PBR_POINT_LIGHT, &interactionShadowMappingPBRShader_pointLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_CUBE_MACROS("_POINT_LIGHT,_PBR"), "_POINT_LIGHT,_PBR"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongPointLightShadowMapping", SHADER_INTERACTION_BLINNPHONG_POINT_LIGHT, &interactionShadowMappingBlinnPhongShader_pointLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_CUBE_MACROS("_POINT_LIGHT,BLINN_PHONG"), "_POINT_LIGHT,BLINN_PHONG"));
    ret.Append(GLSL_SHADER_SOURCE("ambientLightingPointLightShadowMapping", SHADER_AMBIENT_LIGHTING_POINT_LIGHT, &ambientLightingShadowMappingShader_pointLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_CUBE_MACROS("_POINT_LIGHT,_AMBIENT"), "_POINT_LIGHT,_AMBIENT"));
	// parallel light
	ret.Append(GLSL_SHADER_SOURCE("interactionParallelLightShadowMapping", SHADER_INTERACTION_PARALLEL_LIGHT, &interactionShadowMappingShader_parallelLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_PARALLEL_LIGHT,PARALLEL_LIGHT_CASCADE_FRUSTUM,PHONG"), "_PARALLEL_LIGHT,PARALLEL_LIGHT_CASCADE_FRUSTUM,PHONG"));
	ret.Append(GLSL_SHADER_SOURCE("interactionPBRParallelLightShadowMapping", SHADER_INTERACTION_PBR_PARALLEL_LIGHT, &interactionShadowMappingPBRShader_parallelLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_PARALLEL_LIGHT,PARALLEL_LIGHT_CASCADE_FRUSTUM,_PBR"), "_PARALLEL_LIGHT,PARALLEL_LIGHT_CASCADE_FRUSTUM,_PBR"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongParallelLightShadowMapping", SHADER_INTERACTION_BLINNPHONG_PARALLEL_LIGHT, &interactionShadowMappingBlinnPhongShader_parallelLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_PARALLEL_LIGHT,PARALLEL_LIGHT_CASCADE_FRUSTUM,BLINN_PHONG"), "_PARALLEL_LIGHT,PARALLEL_LIGHT_CASCADE_FRUSTUM,BLINN_PHONG"));
    ret.Append(GLSL_SHADER_SOURCE("ambientLightingParallelLightShadowMapping", SHADER_AMBIENT_LIGHTING_PARALLEL_LIGHT, &ambientLightingShadowMappingShader_parallelLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_PARALLEL_LIGHT,PARALLEL_LIGHT_CASCADE_FRUSTUM,_AMBIENT"), "_PARALLEL_LIGHT,PARALLEL_LIGHT_CASCADE_FRUSTUM,_AMBIENT"));
	// spot light
	ret.Append(GLSL_SHADER_SOURCE("interactionSpotLightShadowMapping", SHADER_INTERACTION_SPOT_LIGHT, &interactionShadowMappingShader_spotLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_SPOT_LIGHT,PHONG"), "_SPOT_LIGHT,PHONG"));
	ret.Append(GLSL_SHADER_SOURCE("interactionPBRSpotLightShadowMapping", SHADER_INTERACTION_PBR_SPOT_LIGHT, &interactionShadowMappingPBRShader_spotLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_SPOT_LIGHT,_PBR"), "_SPOT_LIGHT,_PBR"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongSpotLightShadowMapping", SHADER_INTERACTION_BLINNPHONG_SPOT_LIGHT, &interactionShadowMappingBlinnPhongShader_spotLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_SPOT_LIGHT,BLINN_PHONG"), "_SPOT_LIGHT,BLINN_PHONG"));
    ret.Append(GLSL_SHADER_SOURCE("ambientLightingSpotLightShadowMapping", SHADER_AMBIENT_LIGHTING_SPOT_LIGHT, &ambientLightingShadowMappingShader_spotLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_SPOT_LIGHT,_AMBIENT"), "_SPOT_LIGHT,_AMBIENT"));
#endif

	// translucent stencil shadow
#ifdef _STENCIL_SHADOW_IMPROVE
	ret.Append(GLSL_SHADER_SOURCE("interactionTranslucent", SHADER_INTERACTION_TRANSLUCENT, &interactionTranslucentShader, INTERACTION_STENCIL_SHADOW_VERT, INTERACTION_STENCIL_SHADOW_FRAG, "_TRANSLUCENT,PHONG", "_TRANSLUCENT,PHONG"));
	ret.Append(GLSL_SHADER_SOURCE("interactionPBRTranslucent", SHADER_INTERACTION_PBR_TRANSLUCENT, &interactionPBRTranslucentShader, INTERACTION_STENCIL_SHADOW_VERT, INTERACTION_STENCIL_SHADOW_FRAG, "_TRANSLUCENT,_PBR", "_TRANSLUCENT,_PBR"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongTranslucent", SHADER_INTERACTION_BLINNPHONG_TRANSLUCENT, &interactionBlinnPhongTranslucentShader, INTERACTION_STENCIL_SHADOW_VERT, INTERACTION_STENCIL_SHADOW_FRAG, "_TRANSLUCENT,BLINN_PHONG", "_TRANSLUCENT,BLINN_PHONG"));
    ret.Append(GLSL_SHADER_SOURCE("ambientLightingTranslucent", SHADER_AMBIENT_LIGHTING_TRANSLUCENT, &ambientLightingTranslucentShader, INTERACTION_STENCIL_SHADOW_VERT, INTERACTION_STENCIL_SHADOW_FRAG, "_TRANSLUCENT,_AMBIENT", "_TRANSLUCENT,_AMBIENT"));

	// soft stencil shadow
#ifdef _SOFT_STENCIL_SHADOW
	ret.Append(GLSL_SHADER_SOURCE("interactionSoft", SHADER_INTERACTION_SOFT, &interactionSoftShader, INTERACTION_STENCIL_SHADOW_VERT, INTERACTION_STENCIL_SHADOW_FRAG, "_SOFT,PHONG", "_SOFT,PHONG"));
	ret.Append(GLSL_SHADER_SOURCE("interactionPBRSoft", SHADER_INTERACTION_PBR_SOFT, &interactionPBRSoftShader, INTERACTION_STENCIL_SHADOW_VERT, INTERACTION_STENCIL_SHADOW_FRAG, "_SOFT,_PBR", "_SOFT,_PBR"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongSoft", SHADER_INTERACTION_BLINNPHONG_SOFT, &interactionBlinnPhongSoftShader, INTERACTION_STENCIL_SHADOW_VERT, INTERACTION_STENCIL_SHADOW_FRAG, "_SOFT,BLINN_PHONG", "_SOFT,BLINN_PHONG"));
    ret.Append(GLSL_SHADER_SOURCE("ambientLightingSoft", SHADER_AMBIENT_LIGHTING_SOFT, &ambientLightingSoftShader, INTERACTION_STENCIL_SHADOW_VERT, INTERACTION_STENCIL_SHADOW_FRAG, "_SOFT,_AMBIENT", "_SOFT,_AMBIENT"));
#endif
#endif

#ifdef _POSTPROCESS
    ret.Append(GLSL_SHADER_SOURCE("retro_2bit", SHADER_RETRO_2BIT, &retro2BitShader, RETRO_POSTPROCESS_2D_VERT, RETRO_2BIT_FRAG, "", ""));
    ret.Append(GLSL_SHADER_SOURCE("retro_c64", SHADER_RETRO_C64, &retroC64Shader, RETRO_POSTPROCESS_2D_VERT, RETRO_C64_FRAG, "", ""));
    ret.Append(GLSL_SHADER_SOURCE("retro_cpc", SHADER_RETRO_CPC, &retroCPCShader, RETRO_POSTPROCESS_2D_VERT, RETRO_CPC_FRAG, "", ""));
    ret.Append(GLSL_SHADER_SOURCE("retro_genesis", SHADER_RETRO_GENESIS, &retroGenesisShader, RETRO_POSTPROCESS_2D_VERT, RETRO_GENESIS_FRAG, "", ""));
    ret.Append(GLSL_SHADER_SOURCE("retro_ps1", SHADER_RETRO_PSX, &retroPS1Shader, RETRO_POSTPROCESS_2D_VERT, RETRO_PS1_FRAG, "", ""));
#endif
}

#include "draw_glsl_shadersource.cpp"

/*
=================
RB_GLSL_LoadShader

loads GLSL vertex or fragment shaders
=================
*/
static void RB_GLSL_LoadShader(const char *buffer, shaderProgram_t *shaderProgram, GLenum type, const char *fullPath)
{
	if (!buffer) {
		return;
	}

	GLuint shader = 0;

	switch (type) {
		case GL_VERTEX_SHADER:
			// create vertex shader
			shaderProgram->vertexShader = qglCreateShader(GL_VERTEX_SHADER);
			qglShaderSource(shaderProgram->vertexShader, 1, (const GLchar **)&buffer, 0);
			qglCompileShader(shaderProgram->vertexShader);
			shader = shaderProgram->vertexShader;
			break;
		case GL_FRAGMENT_SHADER:
			// create fragment shader
			shaderProgram->fragmentShader = qglCreateShader(GL_FRAGMENT_SHADER);
			qglShaderSource(shaderProgram->fragmentShader, 1, (const GLchar **)&buffer, 0);
			qglCompileShader(shaderProgram->fragmentShader);
			shader = shaderProgram->fragmentShader;
			break;
		default:
			common->Printf("RB_GLSL_LoadShader: unexpected type\n");
			return;
	}

	GLint status;
	qglGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if(!status)
	{
		RB_GLSL_PrintShaderSource(fullPath, buffer);
		GLchar log[LOG_LEN];
		qglGetShaderInfoLog(shader, sizeof(GLchar) * LOG_LEN, NULL, log);
		common->Warning("%s::glCompileShader(%s) -> \n%s", __func__, type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : "GL_FRAGMENT_SHADER", log);
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
	GLint linked;

	if(!shaderProgram->vertexShader || !shaderProgram->fragmentShader)
		return false;

    if(!qglIsProgram(shaderProgram->program))
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
        GLchar log[LOG_LEN];
        qglGetProgramInfoLog(shaderProgram->program, sizeof(GLchar) * LOG_LEN, NULL, log);
        common->Warning("%s::glValidateProgram() -> %s", __func__, log);
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
#if 1 // defined(_RAVEN) || defined(_HUMANHEAD) //karin: fragment shader parms
	for (i = 0; i < MAX_FRAGMENT_PARMS; i++) {
		idStr::snPrintf(buffer, sizeof(buffer), "u_fragmentParm%d", i);
		shader->u_fragmentParm[i] = GL_GetUniformLocation(shader->program, buffer);
	}
#endif

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

    for (i = 0; i < MAX_MEGATEXTURE_PARMS; i++) {
        idStr::snPrintf(buffer, sizeof(buffer), "u_megaTextureLevel%d", i);
        shader->u_megaTextureLevel[i] = GL_GetUniformLocation(shader->program, buffer);
    }

#ifdef _SHADOW_MAPPING
	shader->shadowMVPMatrix = GL_GetUniformLocation(shader->program, "shadowMVPMatrix");
    shader->globalLightOrigin = GL_GetUniformLocation(shader->program, "globalLightOrigin");
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
	for(int i = 0; i < Props.Num()/*<= SHADER_CUSTOM*/; i++)
	{
		const GLSLShaderProp &prop = Props[i];
		if(prop.type == type)
			return &prop;
	}
	
    if(type == SHADER_DEPTH_TO_COLOR)
    {
#ifdef GL_ES_VERSION_3_0
        if(USING_GLES3 || glConfig.depthTextureAvailable)
#else
        if(glConfig.depthTextureAvailable)
#endif
            common->Error("Shader prop '%d' not found!\n", type);
    }
#ifdef GL_ES_VERSION_3_0
    else if(type == SHADER_STENCIL_TO_COLOR)
    {
        if(USING_GLES3)
            common->Error("Shader prop '%d' not found!\n", type);
    }
#endif
#ifdef _SHADOW_MAPPING
    else if(type == SHADER_DEPTH_COLOR || type == SHADER_DEPTH_PERFORATED_COLOR)
    {
#ifdef GL_ES_VERSION_3_0
        if(!USING_GLES3 && (!r_useDepthTexture || !r_useCubeDepthTexture))
#else
        if(!r_useDepthTexture || !r_useCubeDepthTexture)
#endif
            common->Error("Shader prop '%d' not found!\n", type);
    }
#endif
    else
    common->Error("Shader prop '%d' not found!\n", type);
	return NULL;
}

static bool RB_GLSL_InitShaders(void)
{
    int startMs = Sys_Milliseconds();
    common->Printf("----- Compiling GLSL shaders -----\n");

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
			common->Printf("newStage %d not support!\n", i);
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
			common->Printf("Not support shadow mapping!\n");
			if(r_useShadowMapping.GetBool())
			{
				r_useShadowMapping.SetBool(false);
			}
			CVAR_READONLY(r_useShadowMapping);
			break;
		}
		shaderManager->Add(prop->program);
	}
	REQUIRE_SHADER;
#endif

#ifdef _STENCIL_SHADOW_IMPROVE
	UNNECESSARY_SHADER;
	for(int i = SHADER_STENCIL_SHADOW_BEGIN; i <= SHADER_STENCIL_SHADOW_END; i++)
	{
		const GLSLShaderProp *prop = RB_GLSL_FindShaderProp(Props, i);
        if(!prop)
            continue;
		if(!RB_GLSL_LoadShaderProgramFromProp(prop))
		{
			common->Printf("Translucent stencil shadow shader error!\n");
			if(harm_r_stencilShadowTranslucent.GetBool())
			{
				harm_r_stencilShadowTranslucent.SetBool(false);
			}
			CVAR_READONLY(harm_r_stencilShadowTranslucent);
            if(harm_r_stencilShadowSoft.GetBool())
            {
                harm_r_stencilShadowSoft.SetBool(false);
            }
			CVAR_READONLY(harm_r_stencilShadowSoft);
			break;
		}
		shaderManager->Add(prop->program);
	}
	REQUIRE_SHADER;
#endif

#ifdef _POSTPROCESS
    UNNECESSARY_SHADER;
    for(int i = SHADER_POSTPROCESS_BEGIN; i <= SHADER_POSTPROCESS_END; i++)
    {
        const GLSLShaderProp *prop = RB_GLSL_FindShaderProp(Props, i);
        if(!prop)
            continue;
        if(!RB_GLSL_LoadShaderProgramFromProp(prop))
        {
            common->Printf("Not support postprocess!\n");
            if(r_renderMode.GetInteger())
            {
                r_renderMode.SetInteger(0);
            }
            CVAR_READONLY(r_renderMode);
            break;
        }
        shaderManager->Add(prop->program);
    }
    REQUIRE_SHADER;
#endif

    UNNECESSARY_SHADER;
    for(int i = SHADER_DEBUG_BEGIN; i <= SHADER_DEBUG_END; i++)
    {
        const GLSLShaderProp *prop = RB_GLSL_FindShaderProp(Props, i);
        if(!prop)
            continue;
        if(!RB_GLSL_LoadShaderProgramFromProp(prop))
        {
            common->Printf("Not support debug in GLSL shader!\n");
            break;
        }
        shaderManager->Add(prop->program);
    }
    REQUIRE_SHADER;

    int endMs = Sys_Milliseconds();
    common->Printf("----- Compile GLSL shaders finish(%d ms) -----\n", endMs - startMs);

	return true;
}

void R_ReloadGLSLPrograms_f(const idCmdArgs &args)
{
	common->Printf("----- R_ReloadGLSLPrograms -----\n");

    if(!glslInitialized)
    {
        if (!RB_GLSL_InitShaders()) {
            common->Printf("GLSL shaders failed to init.\n");
        }
        // else
            glslInitialized = true;

	    glConfig.allowGLSLPath = true;
    }
    else
    {
#ifdef _MULTITHREAD
        if(multithreadActive)
        {
            reloadGLSLShaders = true;
            common->Printf("Reload GLSL shader will run on next renderer thread!\n");
        }
        else
#endif
        shaderManager->ReloadShaders();
    }

	common->Printf("-------------------------------\n");
}

void R_GLSL_Shutdown(void)
{
    common->Printf("----- R_GLSL_Shutdown -----\n");
    shaderManager->Shutdown();
    glslInitialized = false;
    common->Printf("-------------------------------\n");
}



// new
void RB_GLSL_DeleteShaderProgram(shaderProgram_t *shaderProgram, bool deleteProgram)
{
    const bool isProgram = shaderProgram->program && qglIsProgram(shaderProgram->program);
    const bool isVertexShader = shaderProgram->vertexShader && qglIsShader(shaderProgram->vertexShader);
    const bool isFragmentShader = shaderProgram->fragmentShader && qglIsShader(shaderProgram->fragmentShader);
    GLuint program = shaderProgram->program;

	if(isProgram)
	{
        if(deleteProgram)
            common->Printf("Delete GLSL shader '%s': %d\n", shaderProgram->name, shaderProgram->program);
        else
            common->Printf("Purge GLSL shader '%s': %d\n", shaderProgram->name, shaderProgram->program);

	    if(isVertexShader)
	        qglDetachShader(shaderProgram->program, shaderProgram->vertexShader);
	    if(isFragmentShader)
	        qglDetachShader(shaderProgram->program, shaderProgram->fragmentShader);
	    if(deleteProgram)
		    qglDeleteProgram(shaderProgram->program);
	}

	if(isVertexShader)
	{
		qglDeleteShader(shaderProgram->vertexShader);
	}

	if(isFragmentShader)
	{
		qglDeleteShader(shaderProgram->fragmentShader);
	}

    // keep name
    idStr name = shaderProgram->name;
	memset(shaderProgram, 0, sizeof(shaderProgram_t));
    idStr::Copynz(shaderProgram->name, name.c_str(), sizeof(shaderProgram->name));

	if(!deleteProgram)
	    shaderProgram->program = program;
}

static GLuint RB_GLSL_CreateShader(GLenum type, const char *source, const char *name)
{
	GLuint shader = 0;
	GLint status;

	shader = qglCreateShader(type);
	if(shader == 0)
	{
		SHADER_ERROR("%s::glCreateShader(%s) error!\n", __func__, type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : "GL_FRAGMENT_SHADER");
		return 0;
	}

	qglShaderSource(shader, 1, (const GLchar **)&source, 0);
	qglCompileShader(shader);

	qglGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if(!status)
	{
		idStr innerName("<builtin>/");
		innerName.Append(name);
		innerName.Append(".");
		innerName.Append(type == GL_VERTEX_SHADER ? "vert" : "frag");
		RB_GLSL_PrintShaderSource(innerName.c_str(), source);
		GLchar log[LOG_LEN];
		qglGetShaderInfoLog(shader, sizeof(GLchar) * LOG_LEN, NULL, log);
		SHADER_ERROR("%s::glCompileShader(%s) -> \n%s\n", __func__, type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : "GL_FRAGMENT_SHADER", log);
		qglDeleteShader(shader);
		shader = 0;
	}

	return shader;
}

static GLuint RB_GLSL_CreateProgram(GLuint &program, GLuint vertShader, GLuint fragShader, bool needsAttributes = true)
{
	GLint result;

    if(!qglIsProgram(program))
	program = qglCreateProgram();
	if(program == 0)
	{
		SHADER_ERROR("%s::glCreateProgram() error!\n", __func__);
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
		SHADER_ERROR("%s::glLinkProgram() -> %s!\n", __func__, log);
		qglDeleteProgram(program);
		program = 0;
	}

	qglValidateProgram(program);
	qglGetProgramiv(program, GL_VALIDATE_STATUS, &result);
	if(!result)
	{
		GLchar log[LOG_LEN];
		qglGetProgramInfoLog(program, sizeof(GLchar) * LOG_LEN, NULL, log);
        common->Warning("%s::glValidateProgram() -> %s!", __func__, log);
//		qglDeleteProgram(program);
//		program = 0;
	}

	return program;
}

bool RB_GLSL_CreateShaderProgram(shaderProgram_t *shaderProgram, const char *vert, const char *frag, const char *name, int type)
{
#ifdef _DEBUG_VERT_SHADER_SOURCE
	{
		idStr shaderType(name);
		shaderType.Append(".vert");
		RB_GLSL_PrintShaderSource(shaderType.c_str(), vert);
	}
#endif
#ifdef _DEBUG_FRAG_SHADER_SOURCE
	{
		idStr shaderType(name);
		shaderType.Append(".frag");
		RB_GLSL_PrintShaderSource(shaderType.c_str(), frag);
	}
#endif
	RB_GLSL_DeleteShaderProgram(shaderProgram);
	shaderProgram->vertexShader = RB_GLSL_CreateShader(GL_VERTEX_SHADER, vert, name);
	if(shaderProgram->vertexShader == 0)
		return false;

	shaderProgram->fragmentShader = RB_GLSL_CreateShader(GL_FRAGMENT_SHADER, frag, name);
	if(shaderProgram->fragmentShader == 0)
	{
		RB_GLSL_DeleteShaderProgram(shaderProgram);
		return false;
	}

	shaderProgram->program = RB_GLSL_CreateProgram(shaderProgram->program, shaderProgram->vertexShader, shaderProgram->fragmentShader);
	if(shaderProgram->program == 0)
	{
		RB_GLSL_DeleteShaderProgram(shaderProgram);
		return false;
	}

	RB_GLSL_GetUniformLocations(shaderProgram);
    idStr::Copynz(shaderProgram->name, name, sizeof(shaderProgram->name));
    shaderProgram->type = type;

	return true;
}

#include "draw_glsl_shaderbinary.cpp"

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
    enum {
        GLSL_LOAD_EXTERNAL_SOURCE = 1,
        GLSL_LOAD_BUILT_IN_SOURCE = 2,
        GLSL_LOAD_EXTERNAL_BINARY = 3,
        GLSL_LOAD_BUILT_IN_BINARY = 4,
    };
	// memset(program, 0, sizeof(shaderProgram_t));

	common->Printf("\nLoad GLSL shader program: %s -> %d\n", name, type);

	common->Printf(" 1. Load external shader source: Vertex(%s), Fragment(%s)\n", vertex_shader_source_file, fragment_shader_source_file);

    // Load external GLSL shader source
    idStr vertexShaderStr;
    idStr vertexFullPath = vertex_shader_source_file;
    int vertexFileBuffer = RB_GLSL_ReadExternalShaderSource(vertexFullPath, vertexShaderStr);
    common->Printf("    Load GLSL shader file: %s -> %s\n", vertexFullPath.c_str(), vertexFileBuffer ? "success" : "fail");
    idStr fragmentShaderStr;
    idStr fragmentFullPath = fragment_shader_source_file;
    int fragmentFileBuffer = RB_GLSL_ReadExternalShaderSource(fragmentFullPath, fragmentShaderStr);
    common->Printf("    Load GLSL shader file: %s -> %s\n", fragmentFullPath.c_str(), fragmentFileBuffer ? "success" : "fail");

    if(vertexFileBuffer && fragmentFileBuffer)
    {
#ifdef GL_ES_VERSION_3_0
        // Create external GLSL shader with binary
        if(USING_GLES3 && harm_r_useGLSLShaderBinaryCache.GetBool())
        {
            common->Printf("    Load external shader binary: Vertex(%s), Fragment(%s)\n", vertex_shader_source_file, fragment_shader_source_file);
            if(RB_GLSL_LoadShaderBinaryCache(program, name, vertexShaderStr, fragmentShaderStr, type, true))
            {
                common->Printf("    Load external shader binary success!\n");
                return GLSL_LOAD_EXTERNAL_BINARY;
            }
        }
#endif
        // Create external GLSL shader with source
        RB_GLSL_LoadShader(vertexShaderStr.c_str(), program, GL_VERTEX_SHADER, vertexFullPath.c_str());
        RB_GLSL_LoadShader(fragmentShaderStr.c_str(), program, GL_FRAGMENT_SHADER, fragmentFullPath.c_str());

        if (RB_GLSL_LinkShader(program, true)/* && RB_GLSL_ValidateProgram(program)*/) {
            RB_GLSL_ValidateProgram(program);
            RB_GLSL_GetUniformLocations(program);
            common->Printf("    Load external shader program success!\n");
            idStr::Copynz(program->name, name, sizeof(program->name));
            program->type = type;

#ifdef GL_ES_VERSION_3_0
            // Cache external GLSL shader binary
            if(USING_GLES3 && harm_r_useGLSLShaderBinaryCache.GetBool())
            {
                RB_GLSL_CacheShaderBinary(program->program, name, vertexShaderStr, fragmentShaderStr, type, true);
            }
#endif
            return GLSL_LOAD_EXTERNAL_SOURCE;
        }
    }

    // Load built-in GLSL shader source
    common->Printf(" 2. Load built-in shader source\n");
    if(harm_r_useHighPrecision.GetBool())
        common->Printf("    '%s' use high precision float\n", name);
    idStr vs = RB_GLSL_ExpandMacros(default_vertex_shader_source, macros, harm_r_useHighPrecision.GetInteger());
    idStr fs = RB_GLSL_ExpandMacros(default_fragment_shader_source, macros, harm_r_useHighPrecision.GetInteger());

#ifdef GL_ES_VERSION_3_0
    // Create built-in GLSL shader with binary
    if(USING_GLES3 && harm_r_useGLSLShaderBinaryCache.GetBool())
    {
        common->Printf("    Load built-in shader binary\n", vertex_shader_source_file, fragment_shader_source_file);
        if(RB_GLSL_LoadShaderBinaryCache(program, name, vs, fs, type, false))
        {
            common->Printf("    Load built-in shader binary success!\n");
            return GLSL_LOAD_BUILT_IN_BINARY;
        }
    }
#endif

    // Create built-in GLSL shader with source
    if(RB_GLSL_CreateShaderProgram(program, vs.c_str(), fs.c_str(), name, type))
    {
        common->Printf("    Load built-in shader program success!\n");

#ifdef GL_ES_VERSION_3_0
        // Cache external GLSL shader binary
        if(USING_GLES3 && harm_r_useGLSLShaderBinaryCache.GetBool())
        {
            RB_GLSL_CacheShaderBinary(program->program, name, vs, fs, type, false);
        }
#endif

        return GLSL_LOAD_BUILT_IN_SOURCE;
    }
    else
    {
        SHADER_ERROR("    Load built-in shader program fail!\n");
        return -1;
    }
}

void idGLSLShaderManager::ReloadShaders(void)
{
	idList<GLSLShaderProp> Props;
	RB_GLSL_GetShaderSources(Props);
	shaderProgram_t *originShader = backEnd.glState.currentProgram;
	GL_UseProgram(NULL);

    int startMs = Sys_Milliseconds();
    common->Printf("----- Compiling GLSL shaders -----\n");

    for(int i = 0; i < shaders.Num(); i++)
    {
        shaderProgram_t *shader = shaders[i];
        common->Printf("Reload GLSL shader %d -> %s......\n", i, shader->name);

        int type = shader->type;
        if(type >= SHADER_BASE_BEGIN && type <= SHADER_BASE_END)
        {
            REQUIRE_SHADER;
        }
        else
        {
	        UNNECESSARY_SHADER;
        }
        RB_GLSL_DeleteShaderProgram(shader, false);
	    if(type < SHADER_CUSTOM)
	    {
            const GLSLShaderProp *prop = RB_GLSL_FindShaderProp(Props, type);
            if(prop)
            {
                if(!RB_GLSL_LoadShaderProgramFromProp(prop))
                {
                    common->Printf("Reload GLSL shader error %d -> %s!\n", i, prop->name.c_str());
                    continue;
                }
            }
	    }
        else
        {
            for(int m = 0; m < customShaders.Num(); m++)
            {
		        GLSLShaderProp &prop = customShaders[m];
                if(shader == prop.program)
                {
                    if(!RB_GLSL_LoadShaderProgramFromProp(&prop))
                    {
                        common->Printf("Reload custom GLSL shader error %d(%d) -> %s!\n", i, m, prop.name.c_str());
                        continue;
                    }
                    break;
                }
            }
        }
    }

    int endMs = Sys_Milliseconds();
    common->Printf("----- Compile GLSL shaders finish(%d ms) -----\n", endMs - startMs);

	GL_UseProgram(originShader);
}

void GLSL_AddCommand(void)
{
	cmdSystem->AddCommand("exportGLSLShaderSource", R_ExportGLSLShaderSource_f, CMD_FL_RENDERER, "export built-in GLSL shader source to game data directory\nUsage: COMMAND [name1 name2 ...] [save_path]");
    cmdSystem->AddCommand("cleanExternalGLSLShaderSource", R_CleanGLSLShaderSource_f, CMD_FL_RENDERER, "remove external GLSL shader source directory");
	cmdSystem->AddCommand("printGLSLShaderSource", R_PrintGLSLShaderSource_f, CMD_FL_RENDERER, "print built-in GLSL shader source\nUsage: COMMAND [name1 name2 ...]");
	cmdSystem->AddCommand("exportDevShaderSource", R_ExportDevShaderSource_f, CMD_FL_RENDERER, "export built-in original C-String GLSL shader source for developer");
#ifdef GL_ES_VERSION_3_0
    cmdSystem->AddCommand("cleanGLSLShaderBinary", R_CleanGLSLShaderBinary_f, CMD_FL_RENDERER, "remove GLSL shader binary directory");
#endif
    cmdSystem->AddCommand("convertARB", GLSL_ConvertARBShader_f, CMD_FL_RENDERER, "convert ARB shader to GLSL shader", GLSL_ArgCompletion_glprogs);
}
