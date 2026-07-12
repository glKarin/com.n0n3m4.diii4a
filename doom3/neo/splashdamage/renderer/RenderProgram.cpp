#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../renderer/tr_local.h"
#include "RenderProgram.h"
#include "../decllib/declRenderProgram.h"
#include "../decllib/declRenderBinding.h"

#if 0
#define NSS_DEBUG(x) x
#else
#define NSS_DEBUG(x)
#endif

#define TEXEL_SIZE_SUFFIX "TexSize"
#define TEXEL_SIZE_NAME(x) va("%s" TEXEL_SIZE_SUFFIX, x)

static const char *Builtin_Vectors[] = {
	"u_glColorPointer", // using vertex color by glColorPointer
	"u_glColor4ub", // using uniform color by glColor
};

idCVar r_shaderQuality("r_shaderQuality", "3", CVAR_RENDERER | CVAR_INTEGER, "");
idCVar r_megaDrawMethod("r_megaDrawMethod", "0", CVAR_RENDERER | CVAR_BOOL, "");
idCVar r_normalizeNormalMaps("r_normalizeNormalMaps", "0", CVAR_RENDERER | CVAR_BOOL, "");
idCVar r_dxnNormalMaps("r_dxnNormalMaps", "0", CVAR_RENDERER | CVAR_BOOL, "");
idCVar r_32ByteVtx("r_32ByteVtx", "0", CVAR_RENDERER | CVAR_BOOL, "");
idCVar r_useDitherMask("r_useDitherMask", "0", CVAR_RENDERER | CVAR_BOOL, "");
idCVar r_shaderSkipSpecCubeMaps("r_shaderSkipSpecCubeMaps", "0", CVAR_RENDERER | CVAR_BOOL, "");
idCVar alphatest_kill("alphatest_kill", "1", CVAR_RENDERER | CVAR_BOOL, "");

idCVar r_useAlphaToCoverage("r_useAlphaToCoverage", "0", CVAR_BOOL | CVAR_RENDERER, "");
idCVar r_softParticles("r_softParticles", "0", CVAR_BOOL | CVAR_RENDERER, "");
idCVar image_diffusePicMip("image_diffusePicMip", "0", CVAR_BOOL | CVAR_RENDERER, "");
idCVar image_bumpPicMip("image_bumpPicMip", "0", CVAR_BOOL | CVAR_RENDERER, "");
idCVar image_specularPicMip("image_specularPicMip", "0", CVAR_BOOL | CVAR_RENDERER, "");

static idCVar harm_r_printShaderSource("harm_r_printShaderSource", "1", CVAR_BOOL | CVAR_RENDERER | CVAR_ARCHIVE, "print external converted shader source");

extern void RB_GLSL_ConvertGL2ESVertexShader(idStr &ret, const char *text, int version);
extern void RB_GLSL_ConvertGL2ESFragmentShader(idStr &ret, const char *text, int version);

sdRenderProgram::sdRenderProgram(void)
        : shaderProgram(idGLSLShaderManager::INVALID_SHADER_HANDLE),
        declRenderProgram(NULL),
		numTextureUnits(0)
{
}

bool sdRenderProgram::LoadProgram(const char *name) {
    shaderHandle_t handle;

    handle = shaderManager->GetHandle(name);
    if(SHADER_HANDLE_IS_VALID(handle))
    {
        shaderProgram = handle;
        return true;
    }

    const idDecl* decl = declManager->FindType(DECL_RENDERPROGRAM, name, false);
    if (!decl) {
        common->Warning("sdRenderProgram::LoadProgram: render program decl '%s' not found", name);
        return false;
    }

    return LoadProgram(static_cast<const sdDeclRenderProgram *>(decl));
}

bool sdRenderProgram::LoadProgram(const sdDeclRenderProgram *decl)
{
    shaderHandle_t handle;
    declRenderProgram = decl;

    if (!declRenderProgram->IsCompleted()) {
        common->Warning("sdRenderProgram::LoadProgram: render program decl '%s' not completed", decl->GetName());
        return false;
    }

#if 1
    if (declRenderProgram->GetVertexShader()->GetLang() != sdRenderProgramShader::SL_GLSL) {
        common->Warning("sdRenderProgram::LoadProgram: render program vertex shader '%s' language not GLSL: %d", decl->GetName(), declRenderProgram->GetVertexShader()->GetLang());
        return false;
    }

    if (declRenderProgram->GetFragmentShader()->GetLang() != sdRenderProgramShader::SL_GLSL) {
        common->Warning("sdRenderProgram::LoadProgram: render program fragment shader '%s' language not GLSL: %d", decl->GetName(), declRenderProgram->GetFragmentShader()->GetLang());
        return false;
    }
#endif

    GLSLShaderProp prop(decl->GetName(), this, &sdRenderProgram::LoadSourceCallback, &sdRenderProgram::BindingLocationCallback);
    prop.vertex_shader_source_file = decl->GetName();
    prop.vertex_shader_source_file.SetFileExtension(".vert");
    prop.fragment_shader_source_file = decl->GetName();
    prop.fragment_shader_source_file.SetFileExtension(".frag");
    handle = shaderManager->Load(prop);
    if(SHADER_HANDLE_IS_INVALID(handle))
    {
        common->Warning("Load GLSL shader program fail: %s.", decl->GetName());
        return false;
    }
    shaderProgram = handle;

    return true;
}

void sdRenderProgram::LoadSourceCallback(GLSLShaderProp *prop) {
    sdRenderProgram *self = (sdRenderProgram *)prop->data;
    prop->default_vertex_shader_source.Clear();
    prop->default_fragment_shader_source.Clear();
    self->LoadSource(prop->default_vertex_shader_source, prop->default_fragment_shader_source);
}

void sdRenderProgram::BindingLocationCallback(struct GLSLShaderProp *prop) {
    sdRenderProgram *self = (sdRenderProgram *)prop->data;
	GLuint currentProgram;
	qglGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)&currentProgram);
	qglUseProgram(prop->handle);
	self->GetLocations(prop->handle);
	qglUseProgram(currentProgram);
}

void sdRenderProgram::BindStageUniform(const materialStage_t *stage, const float *regs) const
{
    const stageVector_t *vec;
    const stageTextureMatrix_t *mat;
    const stageTexture_t *tex;
    GLint location;
    const sdDeclRenderBinding *binding;

    //Sys_Printf("BBB %d %d %d %d\n", shaderProgram, stage->numVectors, stage->numTextureMatrices, stage->numTextures);
	for(int j = 0; j < locations.Num(); j++)
	{
		binding = bindings[j];

		if(!binding) // external binding
		{
			continue;
		}
		location = locations[j];
		bool handled = false;

		if(binding->GetBindingType() == sdDeclRenderBinding::BT_VECTOR)
		{
			// setup vectors uniform
			for ( int i = 0; i < stage->numVectors; i++ ) {
				vec = &stage->vectors[i];
				if(binding != vec->renderBinding)
					continue;

				idVec4 vparm = binding->GetVec4();
				for (int d = 0; d < 4; d++) {
					int m = vec->registers[d];
					vparm[d] = regs[ m ];
				}

				qglUniform4fv(location, 1, vparm.ToFloatPtr());
				handled = true;
				//Sys_Printf("VVV %d %d %s %s\n", j,location, vec->renderBinding->GetName(), vparm.ToString());
				break;
			}
			if(handled)
				continue;

			// setup matrix uniform vec3 x 2
			for ( int i = 0; i < stage->numTextureMatrices; i++ ) {
				mat = &stage->textureMatrices[i];
				if(binding == mat->renderBinding_s)
				{
					idVec4 vparm = binding->GetVec4();
					for (int d = 0; d < 3; d++) {
						int m = mat->matrix[0][d];
						vparm[d] = regs[ m ];
					}

					qglUniform4fv(location, 1, vparm.ToFloatPtr());
					handled = true;
					//Sys_Printf("MMM111 %d %d %s %s\n", j,location, mat->renderBinding_s->GetName(), vparm.ToString());
					break;
				}
				else if(binding == mat->renderBinding_t)
				{
					idVec4 vparm = binding->GetVec4();
					for (int d = 0; d < 3; d++) {
						int m = mat->matrix[1][d];
						vparm[d] = regs[ m ];
					}

					qglUniform4fv(location, 1, vparm.ToFloatPtr());
					handled = true;
					//Sys_Printf("MMM222 %d %d %s %s\n", j,location, mat->renderBinding_t->GetName(), vparm.ToString());
					break;
				}
			}
			if(handled)
				continue;
			// binding default value
			qglUniform4fv(location, 1, binding->GetDefaultVector());
			//Sys_Printf("VVVddd %d %d %s %f %f %f %f\n", j,location, binding->GetName(), binding->GetDefaultVector()[0], binding->GetDefaultVector()[1], binding->GetDefaultVector()[2], binding->GetDefaultVector()[3]);
		}
		else if(binding->GetBindingType() == sdDeclRenderBinding::BT_TEXTURE)
		{
			// setup sampler uniform
			for ( int i = 0; i < stage->numTextures; i++ ) {
				tex = &stage->textures[i];
				if(binding != tex->renderBinding)
					continue;

				if (!tex->image)
					continue;

				//Sys_Printf("TTT %d %d %s %s\n", j,location, tex->renderBinding ? tex->renderBinding->GetName(): "<NULL>", tex->image->imgName.c_str());

				// uisng j as sampler handle
				qglUniform1i(location, textureUnits[j]);
				GL_SelectTexture( textureUnits[j] );
				tex->image->Bind();
				BindTexelSize(bindingNames[j], tex->image);
				handled = true;
				break;
			}
			if(handled)
				continue;
			// binding default value
			qglUniform1i(location, textureUnits[j]);
			GL_SelectTexture( textureUnits[j] );
			binding->GetDefaultImage()->Bind();
			BindTexelSize(bindingNames[j], binding->GetDefaultImage());
			//Sys_Printf("TTTddd %d %d %s %s\n", j,location, binding->GetName(), binding->GetDefaultImage()->imgName.c_str());
		}
	}
}

void sdRenderProgram::BindMaterialUniform(const idMaterial *mat, const float *regs) const {
#if 0
	float parms[4];
	int i1 = mat->GetDeformRegister(1);
	int i2 = mat->GetDeformRegister(2);
	int i3 = mat->GetDeformRegister(3);

	parms[0] = regs[i1];
	parms[1] = regs[i2];
	parms[2] = 0.0f;
	parms[3] = 1.0f;
	BindVector("deformScroll", parms);

	parms[0] = regs[i3];
	parms[1] = 0.0f;
	parms[2] = 0.0f;
	parms[3] = 1.0f;
	BindVector("deformMagnitude", parms);
#endif
}

bool sdRenderProgram::Bind(void) const
{
    if(!IsValid())
        return false;

    const shaderProgram_t *shader = shaderManager->Get(shaderProgram);
    if(!shader)
        return false;
    GL_UseProgram((shaderProgram_t *)shader);

    return true;
}

bool sdRenderProgram::Bind(const materialStage_t *stage, const idMaterial *mat, const float *regs) const
{
	if(!Bind())
		return false;

    BindStageUniform(stage, regs);
	//BindMaterialUniform(mat, regs);

    return true;
}

int sdRenderProgram::SetupState(void) const {
	if(declRenderProgram->DrawStateBits())
	{
		int old = backEnd.glState.glStateBits;
		GL_State(declRenderProgram->DrawStateBits());
		return old;
	}
	return 0;
}

void sdRenderProgram::UnbindUniform(const materialStage_t *stage) const
{
    // binding sampler uniform to null
	for(int i = 0; i < numTextureUnits; i++)
	{
		GL_SelectTexture( i );
		globalImages->BindNull();
	}
}

void sdRenderProgram::Unbind(void) const
{
    GL_SelectTextureForce(0);
    GL_UseProgram(NULL);
}

void sdRenderProgram::Unbind(const materialStage_t *stage) const
{
    UnbindUniform(stage);
	Unbind();
}

void sdRenderProgram::LoadSource(idStr &vsOut, idStr &fsOut) const
{
	if (harm_r_printShaderSource.GetBool())
		common->Printf("Convert GLSL shader %s:\n\n", declRenderProgram->GetName());
    LoadVertexSource(vsOut);
    LoadFragmentSource(fsOut);

	if (harm_r_printShaderSource.GetBool())
	{
		const int Length = 2048; // max is 4096 for idCommon::VPrintf
		common->Printf("Vertex shader:\n");
		for (int i = 0; i < vsOut.Length(); i += Length)
		{
			idStr str = vsOut.Mid(i, Length);
			common->Printf("%s", str.c_str());
		}
		common->Printf("\n\n");

		common->Printf("Fragment shader:\n");
		for (int i = 0; i < fsOut.Length(); i += Length)
		{
			idStr str = fsOut.Mid(i, Length);
			common->Printf("%s", str.c_str());
		}
		common->Printf("\n\n");
	}
}

void sdRenderProgram::LoadVertexSource(idStr &out) const {
    const sdRenderProgramShader *shader = declRenderProgram->GetVertexShader();

    sdStringBuilder_Heap buf;

    buf.Append("\n");
	InsertBuiltinMacros(buf);
    buf.Append("\n");

    InsertBindings(buf, shader);
    buf.Append("\n");

    buf.Append(shader->GetSource());

    const int Version = USING_GLES3 ? 300 : 100;
    RB_GLSL_ConvertGL2ESVertexShader(out, buf.c_str(), Version);
}

void sdRenderProgram::LoadFragmentSource(idStr &out) const {
    const sdRenderProgramShader *shader = declRenderProgram->GetFragmentShader();

    sdStringBuilder_Heap buf;

    buf.Append("\n");
	InsertBuiltinMacros(buf);
    buf.Append("\n");

    InsertBindings(buf, shader);
    buf.Append("\n");

    buf.Append(shader->GetSource());

    const int Version = USING_GLES3 ? 300 : 100;
    RB_GLSL_ConvertGL2ESFragmentShader(out, buf.c_str(), Version);
}

void sdRenderProgram::InsertBinding(sdStringBuilder_Heap &buf, const sdDeclRenderBinding *binding, const char *rawName) const {
    if ((!rawName || !rawName[0]) && !binding)
        rawName = binding->GetName();
    switch (binding->GetBindingType()) {
        case sdDeclRenderBinding::BT_ATTRIB:
            InsertAttribBinding(buf, binding, rawName);
            break;
        case sdDeclRenderBinding::BT_TEXTURE:
            InsertTextureBinding(buf, binding, rawName);
            break;
        case sdDeclRenderBinding::BT_VECTOR:
            InsertUniformBinding(buf, binding, rawName, "vec4");
            break;
        default:
            common->Warning("sdRenderProgram::InsertBinding: unknown render binding '%s' type: %d", binding->GetName(), binding->GetBindingType());
            break;
    }
}

void sdRenderProgram::InsertUniformBinding(sdStringBuilder_Heap &buf, const sdDeclRenderBinding *binding, const char *rawName, const char *type) const {
    buf.Append("uniform ");
    buf.Append(type);
    buf.Append(" ");
    buf.Append(rawName);
    buf.Append(";\n");
}

void sdRenderProgram::InsertMacro(sdStringBuilder_Heap &buf, const char *name, const char *value) const {
    buf.Append("#define ");
    buf.Append(name);
	if(value)
	{
		buf.Append(" ");
		buf.Append(value);
	}
    buf.Append("\n");
}

void sdRenderProgram::InsertBuiltinMacros(sdStringBuilder_Heap &buf) const {
	InsertMacro(buf, "_GLES", "1");
	//InsertMacro(buf, "_DEBUG", "0");
	InsertMacro(buf, "_HARM", "1");
#ifdef NORMALIZE_BYTE_COLOR
	InsertMacro(buf, "NORMALIZE_BYTE_COLOR", "1");
#endif

	const char *CvarMacros[] = {
		"r_shaderQuality",
		"r_megaDrawMethod",
		"r_normalizeNormalMaps",
		"r_dxnNormalMaps",
		"r_32ByteVtx",
		"r_useDitherMask",
		"r_shaderSkipSpecCubeMaps",
		"alphatest_kill",
	};
	idCVar *cvar;
	for(int i = 0; i < sizeof(CvarMacros) / sizeof(CvarMacros[0]); i++)
	{
		cvar = cvarSystem->Find(CvarMacros[i]);
		InsertMacro(buf, CvarMacros[i], cvar ? cvar->GetString() : "0");
	}

	//karin: glColorPointer/glColor
	InsertMacro(buf, "VERTEX_COLOR(x)", "( ( x ) * u_glColorPointer + u_glColor4ub )");
#ifdef NORMALIZE_BYTE_COLOR
	buf.Append("#ifdef NORMALIZE_BYTE_COLOR\n");
	InsertMacro(buf, "VERTEX_BYTE_COLOR(x)", "( x )");
	buf.Append("#else\n");
	InsertMacro(buf, "VERTEX_BYTE_COLOR(x)", "BYTE_COLOR( VERTEX_COLOR( x ) )");
	buf.Append("#endif\n");
#else
	InsertMacro(buf, "VERTEX_BYTE_COLOR(x)", "BYTE_COLOR( VERTEX_COLOR( x ) )");
#endif
}

void sdRenderProgram::InsertTextureBinding(sdStringBuilder_Heap &buf, const sdDeclRenderBinding *binding, const char *rawName) const {
    buf.Append("uniform ");
	if(binding)
	{
		switch (binding->GetCubeMap()) {
			case CF_CAMERA:
			case CF_NATIVE:
				buf.Append("samplerCube ");
				break;
			case CF_2D:
			default:
				buf.Append("sampler2D ");
				break;
		}
	}
	else
		buf.Append("sampler2D ");
    buf.Append(rawName);
    buf.Append(";\n");
	// add texture size to shader for OpenGLES2.0 texRECT
	InsertUniformBinding(buf, NULL, TEXEL_SIZE_NAME(rawName), "vec4");
}

void sdRenderProgram::InsertAttribBinding(sdStringBuilder_Heap &buf, const sdDeclRenderBinding *binding, const char *rawName) const {
    // hardcode fix
#if 1
    buf.Append("#define ");
    buf.Append(rawName);
    buf.Append(" ");
    if (!idStr::Icmp(rawName, "positionAttrib"))
        buf.Append("attr_Vertex");
    else if (!idStr::Icmp(rawName, "texCoordAttrib"))
        buf.Append("attr_TexCoord");
    else if (!idStr::Icmp(rawName, "colorAttrib"))
        buf.Append("attr_Color");
    else if (!idStr::Icmp(rawName, "normalAttrib"))
        buf.Append("attr_Normal");
    else if (!idStr::Icmp(rawName, "tangentAttrib"))
        buf.Append("attr_Tangent");
    else if (!idStr::Icmp(rawName, "signAttrib"))
        buf.Append("attr_Bitangent");
    buf.Append("\n");
#else
    buf.Append("attribute vec4 ");
    buf.Append(binding->GetName());
    buf.Append(";\n");
#endif
}

void sdRenderProgram::InsertBindings(sdStringBuilder_Heap &buf, const sdRenderProgramShader *shader) const {
    const sdDeclRenderBinding *binding;
	const char *name;
	idList<idStr> appendList;

    for (int i = 0; i < shader->NumBindings(); i++) {
        binding = shader->GetBinding(i);
    	name = shader->GetPlaceholder(i);
		if(binding)
			InsertBinding(buf, binding, name);
		else
			InsertBuiltinBinding(buf, name);
    	appendList.Append(name);
    }

    for (int i = 0; i < sizeof(Builtin_Vectors) / sizeof(Builtin_Vectors[0]); i++) {
		name = Builtin_Vectors[i];
		if (appendList.FindIndex(name) == -1)
			InsertBuiltinBinding(buf, name);
	}
}

void sdRenderProgram::GetLocations(shaderHandle_t handle)
{
	shaderProgram = handle;
    const shaderProgram_t *shader = shaderManager->Get(shaderProgram);
    if(!shader) {
		common->Warning("sdRenderProgram::GetLocations: invalid program %d", shaderProgram);
        return;
	}

	bindings.Clear();
	bindingNames.Clear();
	nameHash.Clear();
	locations.Clear();
	textureUnits.Clear();

	GetShaderLocations(shader->program, declRenderProgram->GetVertexShader());
	GetShaderLocations(shader->program, declRenderProgram->GetFragmentShader());
	GetBuiltinLocations(shader->program);

	bindings.Resize(bindings.Num());
	bindings.SetGranularity(1);
	bindingNames.Resize(bindingNames.Num());
	bindingNames.SetGranularity(1);
	locations.Resize(locations.Num());
	locations.SetGranularity(1);
	nameHash.Resize(nameHash.Num());
	nameHash.SetGranularity(1);
	textureUnits.Resize(textureUnits.Num());
	textureUnits.SetGranularity(1);

	if (harm_r_printShaderSource.GetBool())
	{
		common->Printf("Shader %s: uniforms %d, texture units %d\n", declRenderProgram->GetName(), bindings.Num(), numTextureUnits);
		for(int i = 0; i < bindings.Num(); i++)
		{
			idStr str = textureUnits[i] >= 0 ? "sampler" : "variant";
			if(textureUnits[i] >= 0)
				str.Append(va("(%d)", textureUnits[i]));
			common->Printf("%2d: %s location %d, %s, %s\n", i, bindingNames[i].c_str(), locations[i], bindings[i] ? "binding" : "builtin", str.c_str());
		}
	}
}

void sdRenderProgram::GetShaderLocations(GLuint glHandle, const sdRenderProgramShader *shader)
{
    const sdDeclRenderBinding *binding;
    GLint location;
	const char *name;
	int index;
	numTextureUnits = 0;
	int unit;

    for (int i = 0; i < shader->NumBindings(); i++) {
		name = shader->GetPlaceholder(i);
		if(bindingNames.FindIndex(name) >= 0)
			continue;
        binding = shader->GetBinding(i);
		location = GetLocation(glHandle, binding, name);
        if(location < 0)
            continue;
		bindings.Append(binding);
		index = bindingNames.Append(name);
		locations.Append(location);
		nameHash.Append(idStr::Hash(name));
		unit = GetUniformType(glHandle, location, numTextureUnits);
		textureUnits.Append(unit);
		if(unit >= 0)
			qglUniform1i(location, unit);
		// add texture size to shader for OpenGLES2.0 texRECT
		if(binding && binding->GetBindingType() == sdDeclRenderBinding::BT_TEXTURE) {
			idStr texName = TEXEL_SIZE_NAME(name);
			location = GetLocation(glHandle, NULL, texName.c_str());
			if(location >= 0) {
				bindings.Append(NULL);
				index = bindingNames.Append(texName);
				locations.Append(location);
				nameHash.Append(idStr::Hash(texName));
				textureUnits.Append(-1);
			}
		}
    }
}

void sdRenderProgram::GetBuiltinLocations(GLuint glHandle)
{
    GLint location;
	const char *name;
	int index;
	numTextureUnits = 0;
	int unit;

    for (int i = 0; i < sizeof(Builtin_Vectors) / sizeof(Builtin_Vectors[0]); i++) {
		name = Builtin_Vectors[i];
		if(bindingNames.FindIndex(name) >= 0)
			continue;
		location = GetLocation(glHandle, NULL, name);
        if(location < 0)
            continue;
		bindings.Append(NULL);
		index = bindingNames.Append(name);
		locations.Append(location);
		nameHash.Append(idStr::Hash(name));
		unit = GetUniformType(glHandle, location, numTextureUnits);
		textureUnits.Append(unit);
    }
}

GLint sdRenderProgram::GetUniformType(GLuint glHandle, GLint location, GLint &unit)
{
	GLint count = 0;
	qglGetProgramiv(glHandle, GL_ACTIVE_UNIFORMS, &count);

	GLchar name[128];
	GLsizei length;
	GLint size;
	GLenum type;
	GLint loc;
	for(int i = 0; i < count; i++)
	{
		qglGetActiveUniform(glHandle, i, sizeof(name), &length, &size, &type, name);
		loc = qglGetUniformLocation(glHandle, name);
		if(location == loc)
		{
			switch(type)
			{
				case GL_FLOAT:
				case GL_FLOAT_VEC2:
				case GL_FLOAT_VEC3:
				case GL_FLOAT_VEC4:
					return -1;
				case GL_INT:
				case GL_INT_VEC2:
				case GL_INT_VEC3:
				case GL_INT_VEC4:
					return -2;
				case GL_UNSIGNED_INT:
				case GL_UNSIGNED_INT_VEC2:
				case GL_UNSIGNED_INT_VEC3:
				case GL_UNSIGNED_INT_VEC4:
					return -3;
				case GL_BOOL:
				case GL_BOOL_VEC2:
				case GL_BOOL_VEC3:
				case GL_BOOL_VEC4:
					return -4;
				case GL_FLOAT_MAT2:
				case GL_FLOAT_MAT3:
				case GL_FLOAT_MAT4:
				case GL_FLOAT_MAT2x3:
				case GL_FLOAT_MAT2x4:
				case GL_FLOAT_MAT3x2:
				case GL_FLOAT_MAT3x4:
				case GL_FLOAT_MAT4x2:
				case GL_FLOAT_MAT4x3:
					return -5;
				case GL_SAMPLER_2D:
				case GL_SAMPLER_3D:
				case GL_SAMPLER_CUBE:
				case GL_SAMPLER_2D_SHADOW:
				case GL_SAMPLER_2D_ARRAY:
				case GL_SAMPLER_2D_ARRAY_SHADOW:
				case GL_SAMPLER_CUBE_SHADOW:
				case GL_INT_SAMPLER_2D:
				case GL_INT_SAMPLER_3D:
				case GL_INT_SAMPLER_CUBE:
				case GL_INT_SAMPLER_2D_ARRAY:
				case GL_UNSIGNED_INT_SAMPLER_2D:
				case GL_UNSIGNED_INT_SAMPLER_3D:
				case GL_UNSIGNED_INT_SAMPLER_CUBE:
				case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
					return unit++;
				default:
					return -6;
			}
		}
	}
	return -7;
}

int sdRenderProgram::GetLocation(GLuint glHandle, const sdDeclRenderBinding *binding, const char *rawName) const {
	GLint location;
	if(binding)
	{
		if (!rawName || !rawName[0])
			rawName = binding->GetName();
		switch (binding->GetBindingType()) {
			case sdDeclRenderBinding::BT_ATTRIB:
				location = qglGetAttribLocation(glHandle, rawName);
				break;
			case sdDeclRenderBinding::BT_TEXTURE:
			case sdDeclRenderBinding::BT_VECTOR:
				location = qglGetUniformLocation(glHandle, rawName);
				break;
			default:
				common->Warning("sdRenderProgram::GetLocation: unknown render binding %s type: %d", binding->GetName(), binding->GetBindingType());
				location = -1;
				break;
		}
	}
	else // maybe built-in
	{
		location = qglGetUniformLocation(glHandle, rawName);
	}

	return location;
}

GLint sdRenderProgram::GetBindingLocation(const sdDeclRenderBinding *binding) const {
    if (!binding)
        return -1;
    int index = bindings.FindIndex(binding);
    if (index < 0)
        return -1;
    return locations[index];
}

void sdRenderProgram::InsertBuiltinBinding(sdStringBuilder_Heap &buf, const char *rawName) const {
	const char *Builtin_Variables[] = {
		"currentRenderTexelSize",
		"u_glColorPointer", // using vertex color by glColorPointer
		"u_glColor4ub", // using uniform color by glColor
	};
	for (int i = 0; i < sizeof(Builtin_Variables) / sizeof(Builtin_Variables[0]); i++) {
		if(!idStr::Icmp(rawName, Builtin_Variables[i])) {
			InsertUniformBinding(buf, NULL, rawName, "vec4");
			return;
		}
	}

	const char *BuiltinMat4_Variables[] = {
		"u_projectionMatrix",
		"u_modelViewMatrix",
		"u_modelMatrix",
		"transposedModelMatrix",
		"transposedModelViewMatrix",
		"transposedProjectionMatrix",
	};
	for (int i = 0; i < sizeof(BuiltinMat4_Variables) / sizeof(BuiltinMat4_Variables[0]); i++) {
		if(!idStr::Icmp(rawName, BuiltinMat4_Variables[i])) {
			InsertUniformBinding(buf, NULL, rawName, "mat4");
			return;
		}
	}

	int index = idStr::FindText(rawName, TEXEL_SIZE_SUFFIX);
	if(index == -1 || index != idStr::Length(rawName) - idStr::Length(TEXEL_SIZE_SUFFIX))
		common->Warning("sdRenderProgram::InsertBuiltinBinding: unknown render built-in binding '%s'", rawName);
}

GLint sdRenderProgram::GetUniformLocation(const char *name) const {
	int index = FindIndex(name);
	if(index < 0)
		return -1;
	return locations[index];
}

int sdRenderProgram::FindIndex(const char *name) const {
	if(name[0] == '$')
		name++;
#if 1
	int key = idStr::Hash(name);
	int index = nameHash.FindIndex(key);
#else
	int index = bindingNames.FindIndex(name);
#endif
	return index;
}

void sdRenderProgram::BindVector(const char *name, const float v4[]) const
{
	GLint location = GetUniformLocation(name);
	if(location < 0)
		return;

	qglUniform4fv(location, 1, v4);
}

void sdRenderProgram::BindVector(const char *name, const idVec4 &v4) const
{
	GLint location = GetUniformLocation(name);
	if(location < 0)
		return;

	qglUniform4fv(location, 1, v4.ToFloatPtr());
}

void sdRenderProgram::BindVector(const char *name, const idVec3 &v3) const
{
	GLint location = GetUniformLocation(name);
	if(location < 0)
		return;

	qglUniform4f(location, v3[0], v3[1], v3[2], 1.0f);
	//qglUniform3fv(location, 1, v3.ToFloatPtr());
}

void sdRenderProgram::BindVector(const char *name, const idVec2 &v2) const
{
	GLint location = GetUniformLocation(name);
	if(location < 0)
		return;

	qglUniform4f(location, v2[0], v2[1], 1.0f, 1.0f);
	//qglUniform2fv(location, 1, v2.ToFloatPtr());
}

void sdRenderProgram::BindVector(const char *name, float f) const
{
	GLint location = GetUniformLocation(name);
	if(location < 0)
		return;

	qglUniform4f(location, f, f, f, f);
}

void sdRenderProgram::BindVector(const char *name, float x, float y, float z, float w) const
{
	GLint location = GetUniformLocation(name);
	if(location < 0)
		return;

	qglUniform4f(location, x, y, z, w);
}

void sdRenderProgram::BindMat4(const char *name, const float mat4[]) const
{
	GLint location = GetUniformLocation(name);
	if(location < 0)
		return;

	qglUniformMatrix4fv(location, 1, false, mat4);
}

void sdRenderProgram::BindMat4(const char *name, const idMat4 &mat4) const
{
	GLint location = GetUniformLocation(name);
	if(location < 0)
		return;

	qglUniformMatrix4fv(location, 1, false, mat4.ToFloatPtr());
}

void sdRenderProgram::BindImage(const char *name, idImage *image) const
{
	int index = FindIndex(name);
	if(index < 0)
		return;

	GLint location = locations[index];
	if(location < 0 || textureUnits[index] < 0)
		return;

    const sdDeclRenderBinding *binding = bindings[index];
	// setup sampler uniform
	qglUniform1i(location, textureUnits[index]);
	GL_SelectTexture( textureUnits[index] );
	if(image)
	{
		image->Bind();
		BindTexelSize(name, image);
	}
	else if(binding && binding->GetBindingType() == sdDeclRenderBinding::BT_TEXTURE)
	{
		// binding default value
		binding->GetDefaultImage()->Bind();
		BindTexelSize(name, binding->GetDefaultImage());
	}
	else
	{
		globalImages->BindNull();
		BindTexelSize(name, NULL);
	}
}

void sdRenderProgram::BindTexelSize(const char *name, const idImage *img) const {
	float texelSize[4];
	if (img) {
		texelSize[0] = (float)img->uploadWidth;
		texelSize[1] = (float)img->uploadHeight;
	}
	else
	{
		texelSize[0] = 0.0f;
		texelSize[1] = 0.0f;
	}
	texelSize[2] = 0.0f;
	texelSize[3] = 1.0f;
	BindVector(TEXEL_SIZE_NAME(name), texelSize);
}
