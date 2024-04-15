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

shaderProgram_t	interactionShader;
shaderProgram_t	shadowShader;
shaderProgram_t	defaultShader;
shaderProgram_t	depthFillShader;
shaderProgram_t	depthFillClipShader; //k: z-fill clipped shader
shaderProgram_t cubemapShader; //k: skybox shader
shaderProgram_t reflectionCubemapShader; //k: reflection shader
shaderProgram_t	fogShader; //k: fog shader
shaderProgram_t	blendLightShader; //k: blend light shader
shaderProgram_t	interactionBlinnPhongShader; //k: BLINN-PHONG lighting model interaction shader
shaderProgram_t diffuseCubemapShader; //k: diffuse cubemap shader
shaderProgram_t texgenShader; //k: texgen shader
// new stage
shaderProgram_t heatHazeShader; //k: heatHaze shader
shaderProgram_t heatHazeWithMaskShader; //k: heatHaze with mask shader
shaderProgram_t heatHazeWithMaskAndVertexShader; //k: heatHaze with mask and vertex shader
shaderProgram_t colorProcessShader; //k: color process shader
#ifdef _SHADOW_MAPPING
shaderProgram_t depthShader_pointLight; //k: depth shader(point light)
shaderProgram_t	interactionShadowMappingShader_pointLight; //k: interaction with shadow mapping(point light)
shaderProgram_t	interactionShadowMappingBlinnPhongShader_pointLight; //k: interaction with shadow mapping(point light)

shaderProgram_t depthShader_parallelLight; //k: depth shader(parallel)
shaderProgram_t	interactionShadowMappingShader_parallelLight; //k: interaction with shadow mapping(parallel)
shaderProgram_t	interactionShadowMappingBlinnPhongShader_parallelLight; //k: interaction with shadow mapping(parallel)

shaderProgram_t depthShader_spotLight; //k: depth shader
shaderProgram_t	interactionShadowMappingShader_spotLight; //k: interaction with shadow mapping
shaderProgram_t	interactionShadowMappingBlinnPhongShader_spotLight; //k: interaction with shadow mapping

shaderProgram_t depthPerforatedShader; //k: depth perforated shader
#endif
#ifdef _TRANSLUCENT_STENCIL_SHADOW
shaderProgram_t	interactionTranslucentShader; //k: PHONG lighting model interaction shader(translucent stencil shadow)
shaderProgram_t	interactionBlinnPhongTranslucentShader; //k: BLINN-PHONG lighting model interaction shader(translucent stencil shadow)
#endif

int idGLSLShaderManager::Add(const shaderProgram_t *shader) {
	if(!shader)
		return -1;
	if(!shader->program)
	{
		common->Warning("idGLSLShaderManager::Add shader's program handle is 0");
		return -1;
	}
	const shaderProgram_t *c = Find(shader->name);
	if(c && c != shader)
	{
		common->Warning("idGLSLShaderManager::Add shader's name is dup '%s'", shader->name);
		return -1;
	}
	common->Printf("idGLSLShaderManager::Add shader program '%s'\n", shader->name);
	return shaders.Append(shader);
}

void idGLSLShaderManager::Clear(void) {
	shaders.Clear();
}

const shaderProgram_t * idGLSLShaderManager::Find(const char *name) const {
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(!idStr::Icmp(name, shader->name))
			return shader;
	}
	return NULL;
}

const shaderProgram_t * idGLSLShaderManager::Find(GLuint handle) const
{
	if(handle <= 0)
		return NULL;
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(handle == shader->program)
			return shader;
	}
	return NULL;
}

idGLSLShaderManager idGLSLShaderManager::_shaderManager;

idGLSLShaderManager *shaderManager = &idGLSLShaderManager::_shaderManager;

static bool shaderRequired = true;

struct GLSLShaderProp
{
	idStr name;
	shaderProgram_t *program;
	idStr default_vertex_shader_source;
	idStr default_fragment_shader_source;
	idStr macros;
#ifdef GL_ES_VERSION_3_0
	idStr es3_default_vertex_shader_source;
	idStr es3_default_fragment_shader_source;
	idStr es3_macros;
#endif
	idStr vertex_shader_source_file;
	idStr fragment_shader_source_file;

	GLSLShaderProp()
			  : program(NULL)
	{}

	GLSLShaderProp(const char *name, shaderProgram_t *program, const idStr &vs, const idStr &fs, const idStr &macros
#ifdef GL_ES_VERSION_3_0
				   , const idStr &vs3, const idStr &fs3, const idStr &macros3
#endif
				   )
	: name(name),
	  program(program),
	  default_vertex_shader_source(vs),
	  default_fragment_shader_source(fs),
	  macros(macros)
#ifdef GL_ES_VERSION_3_0
	  , es3_default_vertex_shader_source(vs3),
	  es3_default_fragment_shader_source(fs3),
	  es3_macros(macros3)
#endif
	{
		vertex_shader_source_file = name;
		vertex_shader_source_file += ".vert";
		fragment_shader_source_file = name;
		fragment_shader_source_file += ".frag";
	}
};
static int RB_GLSL_LoadShaderProgram(
		const char *name,
		shaderProgram_t * const program,
		const char *default_vertex_shader_source,
		const char *default_fragment_shader_source,
		const char *vertex_shader_source_file,
		const char *fragment_shader_source_file,
		const char *macros
);

#define _GLPROGS "glslprogs" // "gl2progs"
static idCVar	harm_r_shaderProgramDir("harm_r_shaderProgramDir", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "[Harmattan]: Special external OpenGLES2 GLSL shader program directory path(default is empty, means using `" _GLPROGS "`).");

#ifdef GL_ES_VERSION_3_0
#define _GL3PROGS "glsl3progs"
static idCVar	harm_r_shaderProgramES3Dir("harm_r_shaderProgramES3Dir", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "[Harmattan]: Special external OpenGLES3 GLSL shader program directory path(default is empty, means using `" _GL3PROGS "`).");
#endif

static void R_GetShaderSources(idList<GLSLShaderProp> &ret)
{
#include "glsl_shader.h"
#ifdef GL_ES_VERSION_3_0
#define GLSL_SHADER_SOURCE(name, program, vs, fs, macros, macros3) GLSLShaderProp(name, program, vs, fs, macros, ES3_##vs, ES3_##fs, macros3)
#else
#define GLSL_SHADER_SOURCE(name, program, vs, fs, macros, unused) GLSLShaderProp(name, program, vs, fs, macros)
#endif
#define GLSL2_SHADOW_MAPPING_2D_MACROS(type) r_useDepthTexture ? type ",_USING_DEPTH_TEXTURE" : (packFloat ? type ",_PACK_FLOAT" : type)
#define GLSL2_SHADOW_MAPPING_CUBE_MACROS(type) r_useCubeDepthTexture ? type ",_USING_DEPTH_TEXTURE" : (packFloat ? type ",_PACK_FLOAT" : type)
#ifdef _SHADOW_MAPPING
	extern bool r_useDepthTexture;
	extern bool r_useCubeDepthTexture;
	const bool packFloat = harm_r_shadowMapDepthBuffer.GetInteger() == 3;
#endif

	ret.Clear();

	// base
	ret.Append(GLSL_SHADER_SOURCE("interaction", &interactionShader, INTERACTION_VERT, INTERACTION_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("shadow", &shadowShader, SHADOW_VERT, SHADOW_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("default", &defaultShader, DEFAULT_VERT, DEFAULT_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("zfill", &depthFillShader, ZFILL_VERT, ZFILL_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("zfillClip", &depthFillClipShader, ZFILLCLIP_VERT, ZFILLCLIP_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("cubemap", &cubemapShader, CUBEMAP_VERT, CUBEMAP_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("reflectionCubemap", &reflectionCubemapShader, REFLECTION_CUBEMAP_VERT, CUBEMAP_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("fog", &fogShader, FOG_VERT, FOG_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("blendLight", &blendLightShader, BLENDLIGHT_VERT, FOG_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphong", &interactionBlinnPhongShader, INTERACTION_VERT, INTERACTION_FRAG, "BLINN_PHONG", "BLINN_PHONG"));
	ret.Append(GLSL_SHADER_SOURCE("diffuseCubemap", &diffuseCubemapShader, DIFFUSE_CUBEMAP_VERT, CUBEMAP_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("texgen", &texgenShader, TEXGEN_VERT, TEXGEN_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("heatHaze", &heatHazeShader, HEATHAZE_VERT, HEATHAZE_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("heatHazeWithMask", &heatHazeWithMaskShader, HEATHAZEWITHMASK_VERT, HEATHAZEWITHMASK_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("heatHazeWithMaskAndVertex", &heatHazeWithMaskAndVertexShader, HEATHAZEWITHMASKANDVERTEX_VERT, HEATHAZEWITHMASKANDVERTEX_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("colorProcess", &colorProcessShader, COLORPROCESS_VERT, COLORPROCESS_FRAG, "", ""));
	// shadow mapping
#ifdef _SHADOW_MAPPING
	// point light
	ret.Append(GLSL_SHADER_SOURCE("depthPointLight", &depthShader_pointLight, DEPTH_VERT, DEPTH_FRAG, GLSL2_SHADOW_MAPPING_CUBE_MACROS("_POINT_LIGHT"), "_POINT_LIGHT"));
	ret.Append(GLSL_SHADER_SOURCE("interactionPointLightShadowMapping", &interactionShadowMappingShader_pointLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_CUBE_MACROS("_POINT_LIGHT"), "_POINT_LIGHT"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongPointLightShadowMapping", &interactionShadowMappingBlinnPhongShader_pointLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_CUBE_MACROS("_POINT_LIGHT,BLINN_PHONG"), "_POINT_LIGHT,BLINN_PHONG"));
	// parallel light
	ret.Append(GLSL_SHADER_SOURCE("depthParallelLight", &depthShader_parallelLight, DEPTH_VERT, DEPTH_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_PARALLEL_LIGHT"), "_PARALLEL_LIGHT"));
	ret.Append(GLSL_SHADER_SOURCE("interactionParallelLightShadowMapping", &interactionShadowMappingShader_parallelLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_PARALLEL_LIGHT"), "_PARALLEL_LIGHT"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongParallelLightShadowMapping", &interactionShadowMappingBlinnPhongShader_parallelLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_PARALLEL_LIGHT,BLINN_PHONG"), "_PARALLEL_LIGHT,BLINN_PHONG"));
	// spot light
	ret.Append(GLSL_SHADER_SOURCE("depthSpotLight", &depthShader_spotLight, DEPTH_VERT, DEPTH_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_SPOT_LIGHT"), "_SPOT_LIGHT"));
	ret.Append(GLSL_SHADER_SOURCE("interactionSpotLightShadowMapping", &interactionShadowMappingShader_spotLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_SPOT_LIGHT"), "_SPOT_LIGHT"));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongSpotLightShadowMapping", &interactionShadowMappingBlinnPhongShader_spotLight, INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG, GLSL2_SHADOW_MAPPING_2D_MACROS("_SPOT_LIGHT,BLINN_PHONG"), "_SPOT_LIGHT,BLINN_PHONG"));
	// perforated surface
	ret.Append(GLSL_SHADER_SOURCE("depthPerforated", &depthPerforatedShader, DEPTH_PERFORATED_VERT, DEPTH_PERFORATED_FRAG, "", ""));
#endif
	// translucent stencil shadow
#ifdef _TRANSLUCENT_STENCIL_SHADOW
	ret.Append(GLSL_SHADER_SOURCE("interactionTranslucent", &interactionTranslucentShader, INTERACTION_TRANSLUCENT_VERT, INTERACTION_TRANSLUCENT_FRAG, "", ""));
	ret.Append(GLSL_SHADER_SOURCE("interactionBlinnphongTranslucent", &interactionBlinnPhongTranslucentShader, INTERACTION_TRANSLUCENT_VERT, INTERACTION_TRANSLUCENT_FRAG, "BLINN_PHONG", "BLINN_PHONG"));
#endif
}

static bool RB_GLSL_LoadShaderProgram(const GLSLShaderProp *prop)
{
	const char *vs;
	const char *fs;
	const char *macros;
#ifdef GL_ES_VERSION_3_0
	if(USING_GLES3)
	{
		vs = prop->es3_default_vertex_shader_source.c_str();
		fs = prop->es3_default_fragment_shader_source.c_str();
		macros = prop->es3_macros.c_str();
	}
	else
	{
#endif
		vs = prop->default_vertex_shader_source.c_str();
		fs = prop->default_fragment_shader_source.c_str();
		macros = prop->macros.c_str();
#ifdef GL_ES_VERSION_3_0
	}
#endif
	if(RB_GLSL_LoadShaderProgram(
			prop->name.c_str(),
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

static bool RB_GLSL_InitShaders(void)
{
	idList<GLSLShaderProp> Props;
	R_GetShaderSources(Props);

	shaderRequired = true;

	// base shader
	for(int i = SHADER_BASE_BEGIN; i <= SHADER_BASE_END; i++)
	{
		const GLSLShaderProp &prop = Props[i];
		if(!RB_GLSL_LoadShaderProgram(&prop))
			return false;
	}

	// newStage shader
	shaderRequired = false;
	for(int i = SHADER_NEW_STAGE_BEGIN; i <= SHADER_NEW_STAGE_END; i++)
	{
		const GLSLShaderProp &prop = Props[i];
		if(!RB_GLSL_LoadShaderProgram(&prop))
		{
			common->Printf("[Harmattan]: newStage %d not support!\n", i);
		}
	}


#ifdef _SHADOW_MAPPING
	shaderRequired = false;
	for(int i = SHADER_SHADOW_MAPPING_BEGIN; i <= SHADER_SHADOW_MAPPING_END; i++)
	{
		const GLSLShaderProp &prop = Props[i];
		if(!RB_GLSL_LoadShaderProgram(&prop))
		{
			common->Printf("[Harmattan]: not support shadow mapping!\n");
			if(r_useShadowMapping.GetBool())
			{
				r_useShadowMapping.SetBool(false);
			}
			r_useShadowMapping.SetReadonly();
			break;
		}
	}
	shaderRequired = true;
#endif

#ifdef _TRANSLUCENT_STENCIL_SHADOW
	shaderRequired = false;
	for(int i = SHADER_STENCIL_SHADOW_BEGIN; i <= SHADER_STENCIL_SHADOW_END; i++)
	{
		const GLSLShaderProp &prop = Props[i];
		if(!RB_GLSL_LoadShaderProgram(&prop))
		{
			common->Printf("[Harmattan]: translucent stencil shadow shader error!\n");
			if(harm_r_stencilShadowTranslucent.GetBool())
			{
				harm_r_stencilShadowTranslucent.SetBool(false);
			}
			harm_r_stencilShadowTranslucent.SetReadonly();
			break;
		}
	}
	shaderRequired = true;
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

bool RB_GLSL_CreateShaderProgram(shaderProgram_t *shaderProgram, const char *vert, const char *frag , const char *name)
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

	shaderManager->Add(shaderProgram);

	return true;
}

int RB_GLSL_LoadShaderProgram(
		const char *name,
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
		if(!RB_GLSL_CreateShaderProgram(program, vs.c_str(), fs.c_str(), name))
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
		shaderManager->Add(program);
		return 1;
	}
}

void R_SaveGLSLShaderSource_f(const idCmdArgs &args)
{
	const char *vs;
	const char *fs;
	const char *macros;
	idList<GLSLShaderProp> Props;
	idStr path = NULL;
	idStrList target;

	R_GetShaderSources(Props);
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

	for(int i = 0; i < SHADER_TOTAL; i++)
	{
		const GLSLShaderProp &prop = Props[i];
		if(target.Num() > 0 && target.FindIndex(prop.name) < 0)
			continue;

#ifdef GL_ES_VERSION_3_0
		if(USING_GLES3)
		{
			vs = prop.es3_default_vertex_shader_source.c_str();
			fs = prop.es3_default_fragment_shader_source.c_str();
			macros = prop.es3_macros.c_str();
		}
		else
		{
#endif
			vs = prop.default_vertex_shader_source.c_str();
			fs = prop.default_fragment_shader_source.c_str();
			macros = prop.macros.c_str();
#ifdef GL_ES_VERSION_3_0
		}
#endif

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
