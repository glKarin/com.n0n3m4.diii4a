#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

#define LOG_LEN 2048

#define SHADER_ERROR(fmt, args...) { \
	if(shaderRequired) {               \
		common->Error(fmt, ##args); \
	} else {                           \
		common->Warning(fmt, ##args);   \
	}                                    \
}

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
shaderProgram_t cubemapShader; //k: skybox shader
shaderProgram_t reflectionCubemapShader; //k: reflection shader
shaderProgram_t	depthFillClipShader; //k: z-fill clipped shader
shaderProgram_t	fogShader; //k: fog shader
shaderProgram_t	blendLightShader; //k: blend light shader
shaderProgram_t	interactionBlinnPhongShader; //k: BLINN-PHONG lighting model interaction shader
shaderProgram_t diffuseCubemapShader; //k: diffuse cubemap shader
shaderProgram_t texgenShader; //k: texgen shader
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
#endif
#ifdef _TRANSLUCENT_STENCIL_SHADOW
shaderProgram_t	interactionTranslucentShader; //k: PHONG lighting model interaction shader(translucent stencil shadow)
shaderProgram_t	interactionTranslucentBlinnPhongShader; //k: BLINN-PHONG lighting model interaction shader(translucent stencil shadow)
#endif

static bool shaderRequired = true;

struct GLSLShaderProp
{
	const char *name;
	shaderProgram_t * const program;
	const char *default_vertex_shader_source;
	const char *default_fragment_shader_source;
	const char *vertex_shader_source_file;
	const char *fragment_shader_source_file;
	const char *macros;
};
static int R_LoadGLSLShaderProgram(
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

static bool R_CreateShaderProgram(shaderProgram_t *shaderProgram, const char *vert, const char *frag , const char *name);

static int R_GLSL_ParseMacros(const char *macros, idStrList &ret)
{
	if(!macros || !macros[0])
		return 0;

	int start = 0;
	int index;
	int counter = 0;
	idStr str(macros);
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

static idStr R_GLSL_ExpandMacros(const char *source, const char *macros)
{
	if(!macros || !macros[0])
		return idStr(source);

	idStrList list;
	int n = R_GLSL_ParseMacros(macros, list);
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

/*
=================
R_LoadGLSLShader

loads GLSL vertex or fragment shaders
=================
*/
static void R_LoadGLSLShader(const char *name, shaderProgram_t *shaderProgram, GLenum type)
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
			common->Printf("R_LoadGLSLShader: unexpected type\n");
			return;
	}
}

/*
=================
R_LinkGLSLShader

links the GLSL vertex and fragment shaders together to form a GLSL program
=================
*/
static bool R_LinkGLSLShader(shaderProgram_t *shaderProgram, bool needsAttributes)
{
	char buf[BUFSIZ];
	int len;
	GLint status;
	GLint linked;

	shaderProgram->program = qglCreateProgram();

	qglAttachShader(shaderProgram->program, shaderProgram->vertexShader);
	qglAttachShader(shaderProgram->program, shaderProgram->fragmentShader);

	if (needsAttributes) {
		qglBindAttribLocation(shaderProgram->program, 8, "attr_TexCoord");
		qglBindAttribLocation(shaderProgram->program, 9, "attr_Tangent");
		qglBindAttribLocation(shaderProgram->program, 10, "attr_Bitangent");
		qglBindAttribLocation(shaderProgram->program, 11, "attr_Normal");
		qglBindAttribLocation(shaderProgram->program, 12, "attr_Vertex");
		qglBindAttribLocation(shaderProgram->program, 13, "attr_Color");
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
		common->Printf("R_LinkGLSLShader: program failed to link\n");
		qglGetProgramInfoLog(shaderProgram->program, sizeof(buf), NULL, buf);
		common->Printf("R_LinkGLSLShader:\n%.*s\n", len, buf);
		return false;
	}

	return true;
}

/*
=================
R_ValidateGLSLProgram

makes sure GLSL program is valid
=================
*/
static bool R_ValidateGLSLProgram(shaderProgram_t *shaderProgram)
{
	GLint validProgram;

	qglValidateProgram(shaderProgram->program);

	qglGetProgramiv(shaderProgram->program, GL_VALIDATE_STATUS, &validProgram);

	if (!validProgram) {
		common->Printf("R_ValidateGLSLProgram: program invalid\n");
		return false;
	}

	return true;
}


static void RB_GLSL_GetUniformLocations(shaderProgram_t *shader)
{
	int	i;
	char	buffer[32];

	GL_UseProgram(shader);

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

	shader->attr_TexCoord = GL_GetAttribLocation(shader->program, "attr_TexCoord");
	shader->attr_Tangent = GL_GetAttribLocation(shader->program, "attr_Tangent");
	shader->attr_Bitangent = GL_GetAttribLocation(shader->program, "attr_Bitangent");
	shader->attr_Normal = GL_GetAttribLocation(shader->program, "attr_Normal");
	shader->attr_Vertex = GL_GetAttribLocation(shader->program, "attr_Vertex");
	shader->attr_Color = GL_GetAttribLocation(shader->program, "attr_Color");

	for (i = 0; i < MAX_VERTEX_PARMS; i++) {
		idStr::snPrintf(buffer, sizeof(buffer), "u_vertexParm%d", i);
		shader->u_vertexParm[i] = GL_GetAttribLocation(shader->program, buffer);
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

	GL_CheckErrors();

	GL_UseProgram(NULL);
}

static bool RB_GLSL_InitShaders(void)
{
#include "glsl_shader.h"

#ifdef GL_ES_VERSION_3_0
	if(USING_GLES3)
	{
		shaderRequired = true;
		const GLSLShaderProp Props[] = {
				{
					"interaction",
					&interactionShader,
					ES3_INTERACTION_VERT, ES3_INTERACTION_FRAG,
					"interaction.vert", "interaction.frag",
					NULL
					},

				{
					"shadow",
				  &shadowShader,
				  ES3_SHADOW_VERT, ES3_SHADOW_FRAG,
				  "shadow.vert", "shadow.frag",
				  NULL
				  },
				{
					"default",
					&defaultShader,
					ES3_DEFAULT_VERT, ES3_DEFAULT_FRAG,
					"default.vert", "default.frag",
					NULL
					},

				{
					"zfill",
					&depthFillShader,
					ES3_ZFILL_VERT, ES3_ZFILL_FRAG,
					"zfill.vert", "zfill.frag",
                    NULL
					},
				{
					"zfillClip",
					&depthFillClipShader,
					ES3_ZFILLCLIP_VERT, ES3_ZFILLCLIP_FRAG,
					"zfillClip.vert", "zfillClip.frag",
					NULL
					},

				{
					"cubemap",
					&cubemapShader,
					ES3_CUBEMAP_VERT, ES3_CUBEMAP_FRAG,
					"cubemap.vert", "cubemap.frag",
					NULL
				},
				{
					"reflectionCubemap",
					&reflectionCubemapShader,
					ES3_REFLECTION_CUBEMAP_VERT, ES3_CUBEMAP_FRAG,
					"reflectionCubemap.vert", "reflectionCubemap.frag",
					NULL
					},
				{
					"fog",
					&fogShader,
					ES3_FOG_VERT, ES3_FOG_FRAG,
					"fog.vert", "fog.frag",
					NULL
					},
				{
					"blendLight",
					&blendLightShader,
					ES3_BLENDLIGHT_VERT, ES3_FOG_FRAG,
					"blendLight.vert", "blendLight.frag",
					NULL
					},

				{
					"interaction_blinn_phong",
					&interactionBlinnPhongShader,
					ES3_INTERACTION_VERT, ES3_INTERACTION_FRAG,
					"interaction_blinnphong.vert", "interaction_blinnphong.frag",
					"BLINN_PHONG"
					},

				{
					"diffuseCubemap",
					&diffuseCubemapShader,
					ES3_DIFFUSE_CUBEMAP_VERT, ES3_CUBEMAP_FRAG,
					"diffuseCubemap.vert", "diffuseCubemap.frag",
					NULL
					},
				{
					"texgen",
					&texgenShader,
					ES3_TEXGEN_VERT, ES3_TEXGEN_FRAG,
					"texgen.vert", "texgen.frag",
					NULL
				},
		};

		for(int i = 0; i < sizeof(Props) / sizeof(Props[0]); i++)
		{
			const GLSLShaderProp *prop = Props + i;
			if(R_LoadGLSLShaderProgram(
					prop->name,
					prop->program,
					prop->default_vertex_shader_source,
					prop->default_fragment_shader_source,
					prop->vertex_shader_source_file,
					prop->fragment_shader_source_file,
					prop->macros
			) < 0)
				return false;
		}

#ifdef _SHADOW_MAPPING
		shaderRequired = false;
		const GLSLShaderProp Props_shadowMapping[] = {
				{
					"depth_point_light",
					&depthShader_pointLight,
					ES3_DEPTH_VERT, ES3_DEPTH_FRAG,
					"depth_point_light.vert", "depth_point_light.frag",
					"_POINT_LIGHT"
					},
				{
					"interaction_point_light_shadow_mapping",
				  &interactionShadowMappingShader_pointLight,
				  ES3_INTERACTION_SHADOW_MAPPING_VERT, ES3_INTERACTION_SHADOW_MAPPING_FRAG,
				  "interaction_point_light_shadow_mapping.vert", "interaction_point_light_shadow_mapping.frag",
				  "_POINT_LIGHT"
				  },
				{
					"interaction_blinnphong_point_light_shadow_mapping",
				  &interactionShadowMappingBlinnPhongShader_pointLight,
				  ES3_INTERACTION_SHADOW_MAPPING_VERT, ES3_INTERACTION_SHADOW_MAPPING_FRAG,
				  "interaction_blinnphong_point_light_shadow_mapping.vert", "interaction_blinnphong_point_light_shadow_mapping.frag",
				  "_POINT_LIGHT,BLINN_PHONG"
				  },

				{
					"depth_parallel_light",
				  &depthShader_parallelLight,
				  ES3_DEPTH_VERT, ES3_DEPTH_FRAG,
				  "depth.vert", "depth.frag",
				  "_PARALLEL_LIGHT"
				  },
				{
					"interaction_parallel_light_shadow_mapping",
					&interactionShadowMappingShader_parallelLight,
					ES3_INTERACTION_SHADOW_MAPPING_VERT, ES3_INTERACTION_SHADOW_MAPPING_FRAG,
					"interaction_shadow_mapping.vert", "interaction_shadow_mapping.frag",
					"_PARALLEL_LIGHT"
					},
				{
					"interaction_blinnphong_parallel_light_shadow_mapping",
				  &interactionShadowMappingBlinnPhongShader_parallelLight,
				  ES3_INTERACTION_SHADOW_MAPPING_VERT, ES3_INTERACTION_SHADOW_MAPPING_FRAG,
				  "interaction_blinnphong_shadow_mapping.vert", "interaction_blinnphong_shadow_mapping.frag",
				  "_PARALLEL_LIGHT,BLINN_PHONG"
				  },

				{
					"depth_spot_light",
					&depthShader_spotLight,
					ES3_DEPTH_VERT, ES3_DEPTH_FRAG,
					"depth.vert", "depth.frag",
					"_SPOT_LIGHT"
					},
				{
					"interaction_spot_light_shadow_mapping",
					&interactionShadowMappingShader_spotLight,
					ES3_INTERACTION_SHADOW_MAPPING_VERT, ES3_INTERACTION_SHADOW_MAPPING_FRAG,
					"interaction_shadow_mapping.vert", "interaction_shadow_mapping.frag",
					"_SPOT_LIGHT"
					},
				{
					"interaction_blinnphong_spot_light_shadow_mapping",
					&interactionShadowMappingBlinnPhongShader_spotLight,
					ES3_INTERACTION_SHADOW_MAPPING_VERT, ES3_INTERACTION_SHADOW_MAPPING_FRAG,
					"interaction_blinnphong_spot_light_shadow_mapping.vert", "interaction_blinnphong_shadow_mapping.frag",
					"_SPOT_LIGHT,BLINN_PHONG"
					},
		};

		for(int i = 0; i < sizeof(Props_shadowMapping) / sizeof(Props_shadowMapping[0]); i++)
		{
			const GLSLShaderProp *prop = Props_shadowMapping + i;
			if(R_LoadGLSLShaderProgram(
					prop->name,
					prop->program,
					prop->default_vertex_shader_source,
					prop->default_fragment_shader_source,
					prop->vertex_shader_source_file,
					prop->fragment_shader_source_file,
					prop->macros
			) < 0)
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
		const GLSLShaderProp Props_translucentStencilShadow[] = {
				{
					"interaction_translucent",
					&interactionTranslucentShader,
					ES3_INTERACTION_TRANSLUCENT_VERT, ES3_INTERACTION_TRANSLUCENT_FRAG,
					"interaction_translucent.vert", "interaction_translucent.frag",
					NULL
					},
				{
					"interaction_translucent_blinn_phong",
					&interactionTranslucentBlinnPhongShader,
					ES3_INTERACTION_TRANSLUCENT_VERT, ES3_INTERACTION_TRANSLUCENT_FRAG,
					"interaction_translucent_blinnphong.vert", "interaction_translucent_blinnphong.frag",
					"BLINN_PHONG"
					},
		};

		for(int i = 0; i < sizeof(Props_translucentStencilShadow) / sizeof(Props_translucentStencilShadow[0]); i++)
		{
			const GLSLShaderProp *prop = Props_translucentStencilShadow + i;
			if(R_LoadGLSLShaderProgram(
					prop->name,
					prop->program,
					prop->default_vertex_shader_source,
					prop->default_fragment_shader_source,
					prop->vertex_shader_source_file,
					prop->fragment_shader_source_file,
					prop->macros
			) < 0)
			{
				common->Printf("[Harmattan]: translucent stencil shadow shader error!\n");
				if(harm_r_translucentStencilShadow.GetBool())
				{
					harm_r_translucentStencilShadow.SetBool(false);
				}
				harm_r_translucentStencilShadow.SetReadonly();
				break;
			}
		}
		shaderRequired = true;
#endif
	}
	else
#endif
	{
		shaderRequired = true;
		const GLSLShaderProp Props[] = {
				{
					"interaction",
					&interactionShader,
					INTERACTION_VERT, INTERACTION_FRAG,
					"interaction.vert", "interaction.frag",
					NULL
					},

				{
					"shadow",
					&shadowShader,
					SHADOW_VERT, SHADOW_FRAG,
					"shadow.vert", "shadow.frag",
					NULL
					},
				{
					"default",
					&defaultShader,
					DEFAULT_VERT, DEFAULT_FRAG,
					"default.vert", "default.frag",
					NULL
					},

				{
					"zfill",
					&depthFillShader,
					ZFILL_VERT, ZFILL_FRAG,
					"zfill.vert", "zfill.frag",
					NULL
					},
				{
					"zfillClip",
					&depthFillClipShader,
					ZFILLCLIP_VERT, ZFILLCLIP_FRAG,
					"zfillClip.vert", "zfillClip.frag",
					NULL
					},

				{
					"cubemap",
					&cubemapShader,
					CUBEMAP_VERT, CUBEMAP_FRAG,
					"cubemap.vert", "cubemap.frag",
					NULL
					},
				{
					"reflectionCubemap",
					&reflectionCubemapShader,
					REFLECTION_CUBEMAP_VERT, CUBEMAP_FRAG,
					"reflectionCubemap.vert", "reflectionCubemap.frag",
					NULL
					},
				{
					"fog",
					&fogShader,
					FOG_VERT, FOG_FRAG,
					"fog.vert", "fog.frag",
					NULL
					},
				{
					"blendLight",
					&blendLightShader,
					BLENDLIGHT_VERT, FOG_FRAG,
					"blendLight.vert", "blendLight.frag",
					NULL
					},

				{
					"interaction_blinn_phong",
					&interactionBlinnPhongShader,
					INTERACTION_VERT, INTERACTION_FRAG,
					"interaction_blinnphong.vert", "interaction_blinnphong.frag",
					"BLINN_PHONG"
					},

				{
					"diffuseCubemap",
					&diffuseCubemapShader,
					DIFFUSE_CUBEMAP_VERT, CUBEMAP_FRAG,
					"diffuseCubemap.vert", "diffuseCubemap.frag",
					NULL
					},
				{
					"texgen",
					&texgenShader,
					TEXGEN_VERT, TEXGEN_FRAG,
					"texgen.vert", "texgen.frag",
					NULL
					},
		};

		for(int i = 0; i < sizeof(Props) / sizeof(Props[0]); i++)
		{
			const GLSLShaderProp *prop = Props + i;
			if(R_LoadGLSLShaderProgram(
					prop->name,
					prop->program,
					prop->default_vertex_shader_source,
					prop->default_fragment_shader_source,
					prop->vertex_shader_source_file,
					prop->fragment_shader_source_file,
					prop->macros
			) < 0)
				return false;
		}

#ifdef _SHADOW_MAPPING
#if 0
#define POINT_LIGHT_EXTRA_MACROS ",_HARM_DEPTH_PACK_TO_VEC4"
#else
#define POINT_LIGHT_EXTRA_MACROS
#endif
		extern bool r_useDepthTexture;
		extern bool r_useCubeDepthTexture;
		const char *USING_DEPTH_TEXTURE = r_useDepthTexture ? ",_USING_DEPTH_TEXTURE" : (harm_r_shadowMapDepthBuffer.GetInteger() == 3 ? ",_PACK_FLOAT" : "");
		const char *USING_DEPTH_CUBEMAP_TEXTURE = r_useCubeDepthTexture ? ",_USING_DEPTH_TEXTURE" : (harm_r_shadowMapDepthBuffer.GetInteger() == 3 ? ",_PACK_FLOAT" : "");
		const idStr macros[] = {
			idStr("_POINT_LIGHT" POINT_LIGHT_EXTRA_MACROS) + USING_DEPTH_CUBEMAP_TEXTURE,
			idStr("_POINT_LIGHT" POINT_LIGHT_EXTRA_MACROS) + USING_DEPTH_CUBEMAP_TEXTURE,
			idStr("_POINT_LIGHT,BLINN_PHONG" POINT_LIGHT_EXTRA_MACROS) + USING_DEPTH_CUBEMAP_TEXTURE,

			idStr("_PARALLEL_LIGHT") + USING_DEPTH_TEXTURE,
			idStr("_PARALLEL_LIGHT") + USING_DEPTH_TEXTURE,
			idStr("_PARALLEL_LIGHT,BLINN_PHONG") + USING_DEPTH_TEXTURE,

			idStr("_SPOT_LIGHT") + USING_DEPTH_TEXTURE,
			idStr("_SPOT_LIGHT") + USING_DEPTH_TEXTURE,
			idStr("_SPOT_LIGHT,BLINN_PHONG") + USING_DEPTH_TEXTURE,
		};

		shaderRequired = false;
		const GLSLShaderProp Props_shadowMapping[] = {
				{
					"depth_point_light",
					&depthShader_pointLight,
					DEPTH_VERT, DEPTH_FRAG,
					"depth_point_light.vert", "depth_point_light.frag",
					macros[0]
				},
				{
					"interaction_point_light_shadow_mapping",
				  &interactionShadowMappingShader_pointLight,
				  INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG,
				  "interaction_point_light_shadow_mapping.vert", "interaction_point_light_shadow_mapping.frag",
				  macros[1]
				},
				{
					"interaction_blinnphong_point_light_shadow_mapping",
				  &interactionShadowMappingBlinnPhongShader_pointLight,
				  INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG,
				  "interaction_blinnphong_point_light_shadow_mapping.vert", "interaction_blinnphong_point_light_shadow_mapping.frag",
				  macros[2]
				},

				{
					"depth_parallel_light",
					&depthShader_parallelLight,
					DEPTH_VERT, DEPTH_FRAG,
					"depth.vert", "depth.frag",
					macros[3]
				},
				{
					"interaction_parallel_light_shadow_mapping",
					&interactionShadowMappingShader_parallelLight,
					INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG,
					"interaction_shadow_mapping.vert", "interaction_shadow_mapping.frag",
					macros[4]
				},
				{
					"interaction_blinnphong_parallel_light_shadow_mapping",
					&interactionShadowMappingBlinnPhongShader_parallelLight,
					INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG,
					"interaction_blinnphong_shadow_mapping.vert", "interaction_blinnphong_shadow_mapping.frag",
					macros[5]
				},

				{
					"depth_spot_light",
					&depthShader_spotLight,
					DEPTH_VERT, DEPTH_FRAG,
					"depth.vert", "depth.frag",
					macros[6]
				},
				{
					"interaction_spot_light_shadow_mapping",
					&interactionShadowMappingShader_spotLight,
					INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG,
					"interaction_shadow_mapping.vert", "interaction_shadow_mapping.frag",
					macros[7]
				},
				{
					"interaction_blinnphong_spot_light_shadow_mapping",
					&interactionShadowMappingBlinnPhongShader_spotLight,
					INTERACTION_SHADOW_MAPPING_VERT, INTERACTION_SHADOW_MAPPING_FRAG,
					"interaction_blinnphong_shadow_mapping.vert", "interaction_blinnphong_shadow_mapping.frag",
					macros[8]
				},
		};

		for(int i = 0; i < sizeof(Props_shadowMapping) / sizeof(Props_shadowMapping[0]); i++)
		{
			const GLSLShaderProp *prop = Props_shadowMapping + i;
			if(R_LoadGLSLShaderProgram(
					prop->name,
					prop->program,
					prop->default_vertex_shader_source,
					prop->default_fragment_shader_source,
					prop->vertex_shader_source_file,
					prop->fragment_shader_source_file,
					prop->macros
			) < 0)
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
		const GLSLShaderProp Props_translucentStencilShadow[] = {
				{
					"interaction_translucent",
					&interactionTranslucentShader,
					INTERACTION_TRANSLUCENT_VERT, INTERACTION_TRANSLUCENT_FRAG,
					"interaction_translucent.vert", "interaction_translucent.frag",
					NULL
					},
				{
					"interaction_translucent_blinn_phong",
					&interactionTranslucentBlinnPhongShader,
					INTERACTION_TRANSLUCENT_VERT,
					INTERACTION_TRANSLUCENT_FRAG,
					"interaction_translucent_blinnphong.vert",
					"interaction_translucent_blinnphong.frag",
					"BLINN_PHONG"
					},
		};

		for(int i = 0; i < sizeof(Props_translucentStencilShadow) / sizeof(Props_translucentStencilShadow[0]); i++)
		{
			const GLSLShaderProp *prop = Props_translucentStencilShadow + i;
			if(R_LoadGLSLShaderProgram(
					prop->name,
					prop->program,
					prop->default_vertex_shader_source,
					prop->default_fragment_shader_source,
					prop->vertex_shader_source_file,
					prop->fragment_shader_source_file,
					prop->macros
			) < 0)
			{
				common->Printf("[Harmattan]: translucent stencil shadow shader error!\n");
				if(harm_r_translucentStencilShadow.GetBool())
				{
					harm_r_translucentStencilShadow.SetBool(false);
				}
				harm_r_translucentStencilShadow.SetReadonly();
				break;
			}
		}
		shaderRequired = true;
#endif
	}

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



static void R_DeleteShaderProgram(shaderProgram_t *shaderProgram)
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

static GLint R_CreateShader(GLenum type, const char *source)
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

static GLint R_CreateProgram(GLint vertShader, GLint fragShader, bool needsAttributes = true)
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
		qglBindAttribLocation(program, 8, "attr_TexCoord");
		qglBindAttribLocation(program, 9, "attr_Tangent");
		qglBindAttribLocation(program, 10, "attr_Bitangent");
		qglBindAttribLocation(program, 11, "attr_Normal");
		qglBindAttribLocation(program, 12, "attr_Vertex");
		qglBindAttribLocation(program, 13, "attr_Color");
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
		qglDeleteProgram(program);
		program = 0;
	}

	return program;
}

bool R_CreateShaderProgram(shaderProgram_t *shaderProgram, const char *vert, const char *frag , const char *name)
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
	R_DeleteShaderProgram(shaderProgram);
	shaderProgram->vertexShader = R_CreateShader(GL_VERTEX_SHADER, vert);
	if(shaderProgram->vertexShader == 0)
		return false;

	shaderProgram->fragmentShader = R_CreateShader(GL_FRAGMENT_SHADER, frag);
	if(shaderProgram->fragmentShader == 0)
	{
		R_DeleteShaderProgram(shaderProgram);
		return false;
	}

	shaderProgram->program = R_CreateProgram(shaderProgram->vertexShader, shaderProgram->fragmentShader);
	if(shaderProgram->program == 0)
	{
		R_DeleteShaderProgram(shaderProgram);
		return false;
	}

	RB_GLSL_GetUniformLocations(shaderProgram);
#ifdef _HARM_SHADER_NAME
	strncpy(shaderProgram->name, name, sizeof(shaderProgram->name));
#else
	(void)(name);
#endif

	return true;
}

int R_LoadGLSLShaderProgram(
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
	R_LoadGLSLShader(vertex_shader_source_file, program, GL_VERTEX_SHADER);
	R_LoadGLSLShader(fragment_shader_source_file, program, GL_FRAGMENT_SHADER);

	if (!R_LinkGLSLShader(program, true) && !R_ValidateGLSLProgram(program)) {
		common->Printf("[Harmattan]: 2. Load internal shader source\n");
		idStr vs = R_GLSL_ExpandMacros(default_vertex_shader_source, macros);
		idStr fs = R_GLSL_ExpandMacros(default_fragment_shader_source, macros);
		if(!R_CreateShaderProgram(program, vs.c_str(), fs.c_str(), name))
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
		RB_GLSL_GetUniformLocations(program);
		common->Printf("[Harmattan]: Load external shader program success!\n\n");
#ifdef _HARM_SHADER_NAME
		strncpy(program->name, name, sizeof(program->name));
#endif
		return 1;
	}
}
