
idCVar harm_r_debugOpenGL("harm_r_debugOpenGL", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INIT | CVAR_BOOL, "debug OpenGL");

static void GLimp_OutputOpenGLCallback_f(GLenum source,
                                 GLenum type,
                                 GLuint id,
                                 GLenum severity,
                                 GLsizei length,
                                 const GLchar* message,
                                 const void* userParam)
{
	const char* sourceStr = "Unknown";
	const char* typeStr = "Unknown";
	const char* severityStr = "Unknown";

	switch (severity)
	{
#define SVRCASE(X, STR)  case GL_DEBUG_SEVERITY_ ## X : severityStr = STR; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: return;
		SVRCASE(HIGH, "High")
		SVRCASE(MEDIUM, "Medium")
		SVRCASE(LOW, "Low")
#undef SVRCASE
	}

	switch (source)
	{
#define SRCCASE(X, N)  case GL_DEBUG_SOURCE_ ## X: sourceStr = N; break;
		SRCCASE(API, "API");
		SRCCASE(WINDOW_SYSTEM, "Window system");
		SRCCASE(SHADER_COMPILER, "Shader compiler");
		SRCCASE(THIRD_PARTY, "Third party");
		SRCCASE(APPLICATION, "Application");
		SRCCASE(OTHER, "Other");
#undef SRCCASE
	}

	switch(type)
	{
  #define TYPECASE(X, N)  case GL_DEBUG_TYPE_ ## X: typeStr = N; break;
		TYPECASE(ERROR, "Error");
		TYPECASE(DEPRECATED_BEHAVIOR, "Deprecated behavior");
		TYPECASE(UNDEFINED_BEHAVIOR, "Undefined behavior");
		TYPECASE(PORTABILITY, "Portability");
		TYPECASE(PERFORMANCE, "Performance");
		TYPECASE(MARKER, "Marker");
		TYPECASE(PUSH_GROUP, "Push group");
		TYPECASE(POP_GROUP, "Pop group");
		TYPECASE(OTHER, "Other");
#undef TYPECASE
	}

	common->Printf("[GLDBG]: %d source=%s, type=%s, severity=%s: %s\n", id, sourceStr, typeStr, severityStr, message);
}

static void GLimp_DebugOpenGL(bool on)
{
	if(!glConfig.debugOutput)
		return;
    if(!GL_DEBUG_MESSAGE_AVAILABLE())
        return;
	common->Printf("OpenGL debug: %s\n", on ? "enable" : "disable");
    if(on)
    {
        qglEnable( GL_DEBUG_OUTPUT );
        qglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        qglDebugMessageCallback( GLimp_OutputOpenGLCallback_f, 0 );
        qglDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
    }
    else
    {
        qglDisable( GL_DEBUG_OUTPUT );
        qglDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        qglDebugMessageCallback( NULL, 0 );
    }
}

void RB_DebugOpenGL(void)
{
	if(!glConfig.debugOutput)
	{
		common->Printf("GL_KHR_Debug not available.\n");
		return;
	}
	if(!harm_r_debugOpenGL.GetBool())
	{
		common->Printf("OpenGL debug context not be created.\n");
		return;
	}

	GLimp_DebugOpenGL(true);
}

