#ifndef _KARIN_SHADER_MANAGER_H
#define _KARIN_SHADER_MANAGER_H

/**
 * > 0: internal shader(index = handle - 1)
 * < 0: custom shader(index = -handle - 1)
 * = 0: invalid
 */
typedef int shaderHandle_t;
#define SHADER_HANDLE_IS_VALID(x) ( (x) != idGLSLShaderManager::INVALID_SHADER_HANDLE )
#define SHADER_HANDLE_IS_INVALID(x) ( (x) == idGLSLShaderManager::INVALID_SHADER_HANDLE )
#define SHADER_HANDLE_INVALID ( idGLSLShaderManager::INVALID_SHADER_HANDLE )
#define SHADER_HANDLE_IS_BUILTIN(x) ( (x) > idGLSLShaderManager::INVALID_SHADER_HANDLE )
#define SHADER_HANDLE_IS_CUSTOM(x) ( (x) < idGLSLShaderManager::INVALID_SHADER_HANDLE )

#define SHADER_MAX_CUSTOM 64 // 32

#define GLSL_BUILTIN_SHADER_INDEX_TO_HANDLE(x) ( (x) + 1 )
#define GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(x) ( -( (x) + 1) )
#define GLSL_BUILTIN_SHADER_HANDLE_TO_INDEX(x) ( (x) - 1 )
#define GLSL_CUSTOM_SHADER_HANDLE_TO_INDEX(x) ( -(x) - 1 )

#define GLSL_SHADER_HANDLE_TO_INDEX(x) ( (x) > 0 ? GLSL_BUILTIN_SHADER_HANDLE_TO_INDEX(x) : ( (x) < 0 ? GLSL_CUSTOM_SHADER_HANDLE_TO_INDEX(x) : -1 ) )
#define GLSL_SHADER_INDEX_TO_HANDLE(x) ( (x) >= SHADER_CUSTOM ? GLSL_CUSTOM_SHADER_INDEX_TO_HANDLE(x) : ( (x) >= 0 ? GLSL_BUILTIN_SHADER_INDEX_TO_HANDLE(x) : INVALID_SHADER_HANDLE ) )

struct GLSLShaderProp
{
	typedef void (*readSource_f)(GLSLShaderProp *prop); // when start load on main thread
	typedef void (*loadFinish_f)(GLSLShaderProp *prop); // when load successful on render thread

	idStr name;
	shaderProgram_t *program;
	shaderHandle_t handle;
	idStr default_vertex_shader_source;
	idStr default_fragment_shader_source;
	idStr macros;
	idStr vertex_shader_source_file;
	idStr fragment_shader_source_file;
    int type; // glsl_program_t
	void *data;
	readSource_f read_source;
	loadFinish_f load_finish;

	GLSLShaderProp()
			: program(NULL),
			handle(0),
            type(-1),
	data(NULL),
			read_source(NULL),
			load_finish(NULL)
	{}

	GLSLShaderProp(const char *name)
			: name(name),
			  program(NULL),
			handle(0),
				type(-1),
	data(NULL),
			  read_source(NULL),
			  load_finish(NULL)
	{
		vertex_shader_source_file = name;
		vertex_shader_source_file += ".vert";
		fragment_shader_source_file = name;
		fragment_shader_source_file += ".frag";
	}

	GLSLShaderProp(const char *name, void *data, readSource_f readSource, loadFinish_f loadFinish)
			: name(name),
			  program(NULL),
			handle(0),
				type(-1),
				data(data),
			  read_source(readSource),
			  load_finish(loadFinish)
	{
	}

	GLSLShaderProp(const char *name, int type, shaderProgram_t *program, const idStr &vs, const idStr &fs, const idStr &macros)
			: name(name),
              type(type),
			  program(program),
			handle(0),
			  default_vertex_shader_source(vs),
			  default_fragment_shader_source(fs),
				macros(macros),
	data(NULL),
			  read_source(NULL),
			  load_finish(NULL)
	{
		vertex_shader_source_file = name;
		vertex_shader_source_file += ".vert";
		fragment_shader_source_file = name;
		fragment_shader_source_file += ".frag";
	}
};

class idGLSLShaderManager
{
public:
	~idGLSLShaderManager();
	int AddBuiltin(const GLSLShaderProp &prop); // return added shader's index
    void Shutdown(void);
	const shaderProgram_t * Find(const char *name) const;
	const shaderProgram_t * Find(GLuint openGLHandle) const; // handle is OpenGL shader program's handle
	shaderHandle_t Load(const GLSLShaderProp &prop); // frontend: if in multi-threading, only add on queue, because current thread has not OpenGL context; else if not in multi-threading, actual load directly. however always return a shader program handle, if has loaded, return OpenGL program handle(> 0), else return -(shaderProps::index + 1), error return 0.
	void ActuallyLoad(void); // backend: if in multi-threading, load actually from queue with OpenGL context
	const shaderProgram_t * Get(shaderHandle_t handle) const;
	shaderHandle_t GetHandle(const char *name) const;
	const GLSLShaderProp * FindProp(const char *name) const;
	void ReloadShaders(void);
	void ReloadShaders(const idStrList &names);
	void Print(void);

	static idGLSLShaderManager _shaderManager;
	static const shaderHandle_t INVALID_SHADER_HANDLE;
	static void R_ExportGLSLShaderSource_f(const idCmdArgs &args);
	static void R_PrintGLSLShaderSource_f(const idCmdArgs &args);
	static void ArgCompletion_Shaders(const idCmdArgs &args, void(*callback)(const char *s));

private:
	int AddPlaceholder(const GLSLShaderProp &prop); // return added shader's index
	int AddCustom(const GLSLShaderProp &prop); // return added shader's index
	int FindIndex(const char *name) const; // return raw index
	int FindIndex(GLuint openGLHandle) const; // return raw index
    GLSLShaderProp * FindCustom(const char *name, int *index = NULL);
	void Resize(int type);
	void ReloadShader(int index);

private:
	idList<shaderProgram_t *> shaders; // available shaders, include internal shaders and loaded custom shaders
	idList<GLSLShaderProp> shaderProps; // custom shaders load list. GLSLShaderProp::program == NULL: loading not start; GLSLShaderProp::program->program > 0: load success; GLSLShaderProp::program->program == 0: load failed
	// idList<unsigned int> queue; // custom shaders load queue: index to shaderProps
	unsigned int queueCurrentIndex; // current loaded index in shaderProps

private:
	idGLSLShaderManager(void);
};
extern idGLSLShaderManager *shaderManager;

#endif // _KARIN_SHADER_MANAGER_H
