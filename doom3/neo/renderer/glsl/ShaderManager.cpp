
#include "../tr_local.h"

const shaderHandle_t idGLSLShaderManager::INVALID_SHADER_HANDLE = 0;

int idGLSLShaderManager::AddCustom(const GLSLShaderProp &prop) // return added shader's index
{
	shaderProgram_t *shader = prop.program;

	if(!shader) // custom shader must not null
		return -1;

	assert(shader->type == prop.type);
	if (shader->type < SHADER_CUSTOM)
	{
		common->FatalError("idGLSLShaderManager::Add custom shader %s type %d is build-in!", shader->name, shader->type);
		return -1;
	}
	if(shader->type >= shaders.Num())
	{
		common->Warning("idGLSLShaderManager::Add custom shader not alloc slot '%s'!", shader->name);
		return -1;
	}
	if (shaders[shader->type])
	{
		common->FatalError("idGLSLShaderManager::Add custom shader %s type %d slot is used!", shader->name, shader->type);
		return -1;
	}
	int index = FindIndex(shader->name);
	if(index >= 0)
	{
		common->Warning("idGLSLShaderManager::Add custom shader name is dup '%s'!", shader->name);
		return index; // -1
	}

	common->Printf("idGLSLShaderManager::Add custom shader program %d '%s' -> %d.\n", prop.type, shader->name, shader->program);
	shaders[shader->type] = shader;
	//shaderProps[shader->type] = prop;

	return shader->type;
}

int idGLSLShaderManager::AddBuiltin(const GLSLShaderProp &prop)
{
	shaderProgram_t *shader = prop.program;

	if(!shader) // build-in shader must not null
		return -1;

	assert(shader->type == prop.type);
	if (shader->type >= SHADER_CUSTOM)
	{
		common->FatalError("idGLSLShaderManager::Add built-in shader %s type %d is not build-in!", shader->name, shader->type);
		return -1;
	}
	if (shader->type < shaders.Num() && shaders[shader->type])
	{
		common->FatalError("idGLSLShaderManager::Add built-in shader %s type %d slot is used!", shader->name, shader->type);
		return -1;
	}
	int index = FindIndex(shader->name);
	if(index >= 0)
	{
		common->Warning("idGLSLShaderManager::Add built-in shader name is dup '%s'!", shader->name);
		return index; // -1
	}

	common->Printf("idGLSLShaderManager::Add built-in shader program %d '%s' -> %d.\n", prop.type, shader->name, shader->program);
	Resize(shader->type);
	shaders[shader->type] = shader;
	shaderProps[shader->type] = prop;
	shaderProps[shader->type].handle = GLSL_BUILTIN_SHADER_INDEX_TO_HANDLE(shader->type);

	return shader->type;
}

int idGLSLShaderManager::AddPlaceholder(const GLSLShaderProp &prop)
{
	if(prop.program) // custom shader must is null
		return -1;

	if (prop.type != SHADER_CUSTOM)
	{
		common->FatalError("idGLSLShaderManager::Add placeholder shader %s type %d is build-in!", prop.name.c_str(), prop.type);
		return -1;
	}
	int index = FindIndex(prop.name);
	if(index >= 0)
	{
		common->Warning("idGLSLShaderManager::Add placeholder shader name is dup '%s'!", prop.name.c_str());
		return index; // -1
	}

	int newType = shaders.Num();
	if (newType < SHADER_CUSTOM) // built-in shaders not loaded???
		newType = SHADER_CUSTOM;
	common->Printf("idGLSLShaderManager::Add placeholder shader program '%s': %d.\n", prop.name.c_str(), newType);
	Resize(newType);
	shaders[newType] = NULL;
	shaderProps[newType] = prop;
	shaderProps[newType].type = newType;
	shaderProps[newType].handle = GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(newType);

	return newType;
}

void idGLSLShaderManager::Resize(int type) {
	int size = type + 1;
	while (shaders.Num() < size) {
		shaders.Append(NULL);
		shaderProps.Append(GLSLShaderProp());
	}
}

const shaderProgram_t * idGLSLShaderManager::Find(const char *name) const
{
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(shader && !idStr::Icmp(name, shader->name))
        {
            common->Printf("idGLSLShaderManager::Find '%s' -> %d, type=%d %s.\n", shader->name, shader->program, shader->type, shader->type < SHADER_CUSTOM ? "built-in" : "custom");
            return shader;
        }
	}
	common->Printf("idGLSLShaderManager::Find '%s' -> NOT FOUND.\n", name);
	return NULL;
}

const shaderProgram_t * idGLSLShaderManager::Find(GLuint openGLHandle) const
{
	if(openGLHandle == 0)
		return NULL;
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(shader && openGLHandle == shader->program)
			return shader;
	}
	return NULL;
}

int idGLSLShaderManager::FindIndex(const char *name) const
{
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(shader && !idStr::Icmp(name, shader->name))
		{
			common->Printf("idGLSLShaderManager::FindIndex '%s' -> %d %s.\n", shader->name, shader->type, shader->type < SHADER_CUSTOM ? "built-in" : "custom");
			return i;
		}
	}
	return -1;
}

int idGLSLShaderManager::FindIndex(GLuint openGLHandle) const
{
	if(openGLHandle == 0)
		return -1;
	for (int i = 0; i < shaders.Num(); i++)
	{
		const shaderProgram_t *shader = shaders[i];
		if(shader && openGLHandle == shader->program)
			return i;
	}
	return -1;
}

const shaderProgram_t * idGLSLShaderManager::Get(shaderHandle_t handle) const
{
	int index;

	if(handle > 0)
		index = GLSL_BUILTIN_SHADER_HANDLE_TO_INDEX(handle);
	else if(handle < 0)
		index = GLSL_CUSTOM_SHADER_HANDLE_TO_INDEX(handle);
	else
		return NULL;
	if(index < shaders.Num())
		return shaders[index];
	return NULL;
}

shaderHandle_t idGLSLShaderManager::GetHandle(const char *name) const
{
    int index;

    index = FindIndex(name);
    if(index < 0)
		return INVALID_SHADER_HANDLE;

    return index < SHADER_CUSTOM ? GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(index) : GLSL_BUILTIN_SHADER_INDEX_TO_HANDLE(index);
}

const GLSLShaderProp * idGLSLShaderManager::FindProp(const char *name) const {
	for (int i = 0; i < shaderProps.Num(); i++)
	{
		const GLSLShaderProp *prop = &shaderProps[i];
		if(!idStr::Icmp(name, prop->name))
			return prop;
	}
	return NULL;
}

GLSLShaderProp * idGLSLShaderManager::FindCustom(const char *name, int *index)
{
	for(int i = SHADER_CUSTOM; i < shaderProps.Num(); i++)
	{
		GLSLShaderProp &p = shaderProps[i];
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
		common->Printf("idGLSLShaderManager::Load shader '%s' has loaded.\n", inProp.name.c_str());
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
				common->Printf("idGLSLShaderManagerr::Load custom shader '%s' has already loaded.\n", inProp.name.c_str());
				return GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(index);
			}
			else
			{
				common->Printf("idGLSLShaderManager::Load custom shader '%s' has already load failed.\n", inProp.name.c_str());
				return INVALID_SHADER_HANDLE;
			}
		}
		else
		{
			common->Printf("idGLSLShaderManager::Load custom shader '%s' wait loading.\n", inProp.name.c_str());
            return GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(index);
		}
	}

	// add to queue
	GLSLShaderProp p = inProp;
	p.type = SHADER_CUSTOM;
	p.program = NULL;
	index = AddPlaceholder(p);
	if (index < 0)
		return INVALID_SHADER_HANDLE;
	common->Printf("idGLSLShaderManager::Load shader push '%s' into queue.\n", p.name.c_str());
	prop = &shaderProps[index];
	if (prop->read_source)
		prop->read_source(prop);

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
	if (queueCurrentIndex < SHADER_CUSTOM) {
		common->FatalError("idGLSLShaderManager::ActuallyLoad: could not load built-in shaders: index = %d", queueCurrentIndex);
		return;
	}
	const unsigned int num = shaderProps.Num();

	if(
			// !queue.Num()
			index >= num
			)
		return;

	const bool B = RB_GLSL_IgnoreLoadError();

	while(
			index < num
			// queue.Num()
			)
	{
		/*
		index = queue[0];
		queue.RemoveIndex(0); // always remove it

		// it's not happened, using assert
		if(index >= shaderProps.Num())
		{
			common->Warning("idGLSLShaderManager::ActuallyLoad custom shader index '%d' over( >= %d ).", index, shaderProps.Num());
			continue;
		}*/

		GLSLShaderProp &prop = shaderProps[index];
		index++;

		if(FindIndex(prop.name.c_str()) >= 0)
		{
			common->Warning("idGLSLShaderManager::ActuallyLoad custom shader '%s' has loaded.", prop.name.c_str());
			continue;
		}
		if(prop.program)
		{
			common->Warning("idGLSLShaderManager::ActuallyLoad custom shader '%s' has handle.", prop.name.c_str());
			continue;
		}

		// create shader on heap
		common->Printf("idGLSLShaderManager::ActuallyLoad shader '%s'.\n", prop.name.c_str());
		shaderProgram_t *shader = (shaderProgram_t *)malloc(sizeof(*shader));
        R_InitShaderProgram(shader);
        idStr::Copynz(shader->name, prop.name.c_str(), sizeof(shader->name));
		shader->type = prop.type;
		prop.program = shader;
		if(RB_GLSL_LoadShaderProgramFromProp(&prop))
		{
			AddCustom(prop);
			if (prop.load_finish)
				prop.load_finish(&prop);
		}
		else
		{
			prop.program = NULL;
			common->Warning("idGLSLShaderManager::ActuallyLoad shader '%s' error!", prop.name.c_str());
		}
	}

	RB_GLSL_SetupLoadError(B);
	queueCurrentIndex = index;
}

idGLSLShaderManager::~idGLSLShaderManager()
{
}

void idGLSLShaderManager::Shutdown(void)
{
    printf("idGLSLShaderManager destroying: %d shaders, %d customer shaders\n", shaders.Num(), shaderProps.Num());
    // stop load queue;
    queueCurrentIndex = shaderProps.Num();
    // delete shader programs
    for(int i = 0; i < shaders.Num(); i++)
    {
        shaderProgram_t *shader = shaders[i];
        if(shader) {
	        RB_GLSL_DeleteShaderProgram(shader, true);
        	if (shader->type >= SHADER_CUSTOM)
        		free(shader);
        }
    }
    shaders.Clear();
    // clear load queue
    queueCurrentIndex = SHADER_CUSTOM;
    shaderProps.Clear();
    printf("idGLSLShaderManager shutdown\n");
}

void idGLSLShaderManager::ReloadShader(int i)
{
	shaderProgram_t *shader = shaders[i];
	GLSLShaderProp *prop = &shaderProps[i];

	common->Printf("Reload GLSL shader '%s'......\n", prop->name.c_str());

	int type = prop->type;
	if(type < 0)
		return;
	if(type >= SHADER_BASE_BEGIN && type <= SHADER_BASE_END)
	{
		RB_GLSL_LoadNotAllowError();
	}
	else
	{
		RB_GLSL_IgnoreLoadError();
	}

	if (shader)
		RB_GLSL_DeleteShaderProgram(shader, false);
	if(type < SHADER_CUSTOM)
	{
		if(RB_GLSL_LoadShaderProgramFromProp(prop)) {
			shaders[i] = prop->program;
		}
		else {
			shaders[i] = NULL;
			common->Printf("Reload GLSL shader error %d -> %s!\n", i, prop->name.c_str());
		}
	}
	else if (prop->program)
	{
		if (prop->read_source)
			prop->read_source(prop);
		if(RB_GLSL_LoadShaderProgramFromProp(prop)) {
			shaders[i] = prop->program;
			if (prop->load_finish)
				prop->load_finish(prop);
		}
		else
		{
			shaders[i] = NULL;
			common->Printf("Reload custom GLSL shader error %d -> %s!\n", i, prop->name.c_str());
		}
	}
}

void idGLSLShaderManager::ReloadShaders(void)
{
	// idList<GLSLShaderProp> Props;
	// RB_GLSL_GetShaderSources(Props);
	shaderProgram_t *originShader = backEnd.glState.currentProgram;
	GL_UseProgram(NULL);

	int startMs = Sys_Milliseconds();
	common->Printf("----- Compiling GLSL shaders -----\n");

	for(int i = 0; i < shaderProps.Num(); i++)
	{
		ReloadShader(i);
	}

	int endMs = Sys_Milliseconds();
	common->Printf("----- Compile GLSL shaders finish(%d ms) -----\n", endMs - startMs);

	GL_UseProgram(originShader);
}

void idGLSLShaderManager::ReloadShaders(const idStrList &names)
{
	if(names.Num() == 0)
		return;

	// idList<GLSLShaderProp> Props;
	// RB_GLSL_GetShaderSources(Props);
	shaderProgram_t *originShader = backEnd.glState.currentProgram;
	GL_UseProgram(NULL);

	int startMs = Sys_Milliseconds();
	common->Printf("----- Reload GLSL shaders -----\n");

	for(int i = 0; i < shaderProps.Num(); i++)
	{
		if (names.FindIndex(shaderProps[i].name) == -1)
			continue;

		ReloadShader(i);
	}

	int endMs = Sys_Milliseconds();
	common->Printf("----- Reload GLSL shaders finish(%d ms) -----\n", endMs - startMs);

	GL_UseProgram(originShader);
}

void idGLSLShaderManager::Print(void)
{
	common->Printf("----- %d GLSL shaders -----\n", shaders.Num());

	if (shaders.Num() != shaderProps.Num()) {
		common->FatalError("idGLSLShaderManager: shaders size(%d) != properties size(%d)", shaders.Num(), shaderProps.Num());
		return;
	}

	for(int i = 0; i < shaders.Num(); i++)
	{
		shaderProgram_t *shader = shaders[i];
		GLSLShaderProp *prop = &shaderProps[i];

		if (shader) {
			if (idStr::Cmp(shader->name, prop->name)) {
				common->FatalError("idGLSLShaderManager: %d shader name(%s) != property name(%s)", i, shader->name, prop->name.c_str());
				return;
			}
			if (shader->type != prop->type) {
				common->FatalError("idGLSLShaderManager: %d shader type(%d) != property type(%d)", i, shader->type, prop->type);
				return;
			}
			if (shader != prop->program) {
				common->FatalError("idGLSLShaderManager: %d shader program(%p) != property program(%p)", i, shader, prop->program);
				return;
			}
			if (shader->type != i) {
				common->FatalError("idGLSLShaderManager: %d shader type(%d) != shader index(%d)", i, shader->type, i);
				return;
			}
		}
		if (prop->type != -1) {
			if (prop->type != i) {
				common->FatalError("idGLSLShaderManager: %d property type(%d) != property index(%d)", i, prop->type, i);
				return;
			}
			if (prop->handle != GLSL_SHADER_INDEX_TO_HANDLE(i)) {
				common->FatalError("idGLSLShaderManager: %d property handle(%d) != property index(%d)", i, prop->handle, GLSL_SHADER_HANDLE_TO_INDEX(i));
				return;
			}
			if (i != GLSL_SHADER_HANDLE_TO_INDEX(prop->handle)) {
				common->FatalError("idGLSLShaderManager: %d property index(%d) != property handle(%d)", i, i, GLSL_SHADER_INDEX_TO_HANDLE(prop->handle));
				return;
			}
		}

		if (prop->type != -1) {
			common->Printf("[%2d] %s: type=%d(%s), handle=%d, OpenGL handle=%d\n", i,
				prop->name.c_str(), prop->type, shader ? shader->type >= SHADER_CUSTOM ? "custom" : "built-in" : "unload", prop->handle,
				shader ? shader->program : -1
				);
		}
		else
			common->Printf("[%2d] \n", i);
	}
}

idGLSLShaderManager::idGLSLShaderManager(void)
: queueCurrentIndex(SHADER_CUSTOM) {
}


static void GLSL_ListShaders_f(const idCmdArgs &args)
{
	shaderManager->Print();
}



idGLSLShaderManager idGLSLShaderManager::_shaderManager;

idGLSLShaderManager *shaderManager = &idGLSLShaderManager::_shaderManager;



void idGLSLShaderManager::R_ExportGLSLShaderSource_f(const idCmdArgs &args)
{
    const char *vs;
    const char *fs;
    const char *macros;
    idStr path;
    idStrList target;

#if 0
    if(args.Argc() > 1)
        path = args.Argv(args.Argc() - 1);
#endif

    for(int i = 1; i < args.Argc()/* - 1*/; i++)
    {
        target.Append(args.Argv(i));
    }

    if(path.IsEmpty())
        path = RB_GLSL_GetExternalShaderSourcePath();

    if(!path.IsEmpty() && path[path.Length() - 1] != '/')
        path += "/";

    common->Printf("Save GLSL shader source to '%s'\n", path.c_str());

    for(int i = 0; i < shaderManager->shaderProps.Num(); i++)
    {
        const GLSLShaderProp &prop = shaderManager->shaderProps[i];
		if(prop.type < 0)
			continue;
		if(!shaderManager->shaders[i])
			continue;
        if(target.Num() > 0 && target.FindIndex(prop.name) < 0)
            continue;

        vs = prop.default_vertex_shader_source.c_str();
        fs = prop.default_fragment_shader_source.c_str();
        macros = prop.macros.c_str();

        idStr vsSrc;
        RB_GLSL_ExpandMacros(vsSrc, vs, macros, harm_r_useHighPrecision.GetInteger());
        idStr p(path);
        p.Append(prop.vertex_shader_source_file);
        fileSystem->WriteFile(p.c_str(), vsSrc.c_str(), vsSrc.Length(), "fs_basepath");
        common->Printf("GLSL vertex shader: '%s'\n", p.c_str());

        idStr fsSrc;
        RB_GLSL_ExpandMacros(fsSrc, fs, macros, harm_r_useHighPrecision.GetInteger());
        p = path;
        p.Append(prop.fragment_shader_source_file);
        fileSystem->WriteFile(p.c_str(), fsSrc.c_str(), fsSrc.Length(), "fs_basepath");
        common->Printf("GLSL fragment shader: '%s'\n", p.c_str());
    }
}

void idGLSLShaderManager::ArgCompletion_Shaders(const idCmdArgs &args, void(*callback)(const char *s))
{
	for(int i = 0; i < shaderManager->shaderProps.Num(); i++)
	{
		callback(va("%s %s", args.Argv(0), shaderManager->shaderProps[i].name.c_str()));
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

void idGLSLShaderManager::R_PrintGLSLShaderSource_f(const idCmdArgs &args)
{
    const char *vs;
    const char *fs;
    const char *macros;
    idList<GLSLShaderProp> Props;
    idStrList target;

    for(int i = 1; i < args.Argc(); i++)
    {
        target.Append(args.Argv(i));
    }

    for(int i = 0; i < shaderManager->shaderProps.Num(); i++)
    {
        const GLSLShaderProp &prop = shaderManager->shaderProps[i];
		if(prop.type < 0)
			continue;
		if(!shaderManager->shaders[i])
			continue;
        if(target.Num() > 0 && target.FindIndex(prop.name) < 0)
            continue;

        vs = prop.default_vertex_shader_source.c_str();
        fs = prop.default_fragment_shader_source.c_str();
        macros = prop.macros.c_str();
        common->Printf("GLSL shader: %s\n\n", prop.name.c_str());

        idStr vsSrc;
        RB_GLSL_ExpandMacros(vsSrc, vs, macros, harm_r_useHighPrecision.GetInteger());
        common->Printf("  Vertex shader: \n");
        R_PrintGLSLShaderSource(vsSrc);
        common->Printf("\n");

        idStr fsSrc;
        RB_GLSL_ExpandMacros(fsSrc, fs, macros, harm_r_useHighPrecision.GetInteger());
        common->Printf("  Fragment shader: \n");
        R_PrintGLSLShaderSource(fsSrc);
        common->Printf("\n");
    }
}

void R_ReloadShader_f(const idCmdArgs &args)
{
	if (args.Argc() < 2)
	{
		common->Printf("Usage: %s <shaders>...", args.Argv(0));
		return;
	}
	common->Printf("Reload \n");

	if(!glslInitialized)
		return;

	reloadShaderNames.Clear();
	for (int i = 1; i < args.Argc(); i++)
		reloadShaderNames.AddUnique(args.Argv(i));

#ifdef _MULTITHREAD
	if(multithreadActive)
		common->Printf("Reload GLSL shader will run on next renderer thread!\n");
	else
#endif
	{
		shaderManager->ReloadShaders(reloadShaderNames);
		reloadShaderNames.Clear();
	}
}
