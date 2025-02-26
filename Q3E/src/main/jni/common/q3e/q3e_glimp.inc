/* GLimp functions */

#ifdef Q3E_GLIMP_INC
#error "q3e_glimp.inc included many times!"
#endif
#define Q3E_GLIMP_INC

#if !defined(Q3E_PRINTF)
#warning "Q3E_PRINTF defined for using print to stdout!"
#include <stdio.h>
#define Q3E_PRINTF printf
#endif

#if !defined(Q3E_ERRORF)
#warning "Q3E_ERRORF defined for using print to stderr and abort!"
#include <stdio.h>
#define Q3E_ERRORF printf
#define Q3E_ERRORF(fmt, ...) do { Q3E_PRINTF(fmt, __VA_ARGS__); abort(); } while(0);
#endif

#if !defined(Q3E_DEBUGF)
#warning "Q3E_DEBUGF defined for using print to stdout(no graphics)!"
#define Q3E_DEBUGF Q3E_PRINTF
#endif

#if !defined(Q3Ebool)
#warning "Q3Ebool defined for as bool type!"
#ifdef __cplusplus
#define Q3Ebool bool
#else
#define Q3Ebool _Bool
#endif
#endif

#if !defined(Q3E_TRUE)
#warning "Q3E_TRUE defined for as true value!"
#ifdef __cplusplus
#define Q3E_TRUE true
#else
#define Q3E_TRUE true
#endif
#endif

#if !defined(Q3E_FALSE)
#warning "Q3E_FALSE defined for as false value!"
#ifdef __cplusplus
#define Q3E_FALSE false
#else
#define Q3E_FALSE false
#endif
#endif

#if !defined(Q3E_USE_FUNCTION)
#warning "Q3E_USE_FUNCTION defined for use functions instead of macros!"
#endif

#if !defined(Q3E_GL_DEBUG_OUTPUT)
#warning "Q3E_GL_DEBUG_OUTPUT defined for enable OpenGL debug output extension!"
#endif

#if !defined(Q3E_POST_GL_INIT)
#warning "Q3E_POST_GL_INIT defined for post OpenGL init!"
#define Q3E_POST_GL_INIT
#endif

#if !defined(dp_strlcpy) // Quake1
#define dp_strlcpy strncpy
#endif

#include <stdlib.h>
#include <string.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#if !defined(EGL_CAST)
#define EGL_CAST(X,V) (X)(V)
#endif

#define GLFORMAT_RGB565 0x0565
#define GLFORMAT_RGBA4444 0x4444
#define GLFORMAT_RGBA5551 0x5551
#define GLFORMAT_RGBA8888 0x8888
#define GLFORMAT_RGBA1010102 0xaaa2

#define MAX_NUM_CONFIGS 1000

// screen info
extern int screen_width;
extern int screen_height;
extern int refresh_rate;

// OpenGL attributes
extern int gl_format;
extern int gl_depth_bits;
extern int gl_msaa;
static int gl_multiSamples = 0;
static int gl_renderApi = 0;
static int gl_debugOutput = 0;

static volatile ANativeWindow *win;
//volatile bool has_gl_context = Q3E_FALSE;

static EGLDisplay eglDisplay = EGL_NO_DISPLAY;
static EGLSurface eglSurface = EGL_NO_SURFACE;
static EGLContext eglContext = EGL_NO_CONTEXT;
static EGLConfig configs[1];
static EGLConfig eglConfig = 0;
static EGLint format = WINDOW_FORMAT_RGBA_8888; // AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;

#define Q3E_GL_CONFIG_ES_1_1() do { q3e_glConfig.major_version = 1; q3e_glConfig.minor_version = -1; } while(0);
#define Q3E_GL_CONFIG_ES_2_0() do { q3e_glConfig.major_version = 2; q3e_glConfig.minor_version = -1; } while(0);
#define Q3E_GL_CONFIG_ES_3_0() do { q3e_glConfig.major_version = 3; q3e_glConfig.minor_version = -1; } while(0);
#define Q3E_GL_CONFIG_ES_3_1() do { q3e_glConfig.major_version = 3; q3e_glConfig.minor_version = 1; } while(0);
#define Q3E_GL_CONFIG_ES_3_2() do { q3e_glConfig.major_version = 3; q3e_glConfig.minor_version = 2; } while(0);

#define Q3E_GL_CONFIG_SET(x, v) do { q3e_glConfig.x = (v); } while(0);
#define Q3E_GL_CONFIG(x) q3e_glConfig.x

#define Q3E_GET_PROC_ADDRESS eglGetProcAddress

#ifdef Q3E_GL_DEBUG_OUTPUT
#define Q3E_DEBUG_OPENGL() GLimp_DebugOpenGL(Q3E_TRUE);
#define Q3E_NO_DEBUG_OPENGL() GLimp_DebugOpenGL(Q3E_FALSE);
#else
#define Q3E_DEBUG_OPENGL()
#define Q3E_NO_DEBUG_OPENGL()
#endif

#ifdef __cplusplus
#define Q3E_PTR_CAST(t, p) reinterpret_cast<t>(p)
#else
#define Q3E_PTR_CAST(t, p) (t)(p)
#endif

#define Q3E_NoDisplay() (eglDisplay == EGL_NO_DISPLAY)
#define Q3E_HasDisplay() (eglDisplay != EGL_NO_DISPLAY)
#define Q3E_HasWindow() (win)
#ifdef Q3E_USE_FUNCTION
#define Q3E_ActivateContext() { \
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext); \
}

#define Q3E_DeactivateContext() { \
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); \
}

#define Q3E_SwapBuffers() { \
    eglSwapBuffers(eglDisplay, eglSurface); \
}
#else
static void Q3E_ActivateContext(void) {
	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
}

static void Q3E_DeactivateContext(void) {
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

static void Q3E_SwapBuffers(void) {
    eglSwapBuffers(eglDisplay, eglSurface);
}
#endif

typedef struct Q3EGLConfig_s
{
	int red;
	int green;
	int blue;
	int alpha;
	int buffer;
	int depth;
	int stencil;
	int samples;
	int major_version;
	int minor_version;
	unsigned debug_output;

    unsigned sample_buffers;
	int egl_major_version;
	int egl_minor_version;
    unsigned stereo;
	int display_frequency;
	int window_width;
	int window_height;
	unsigned fullscreen;
	float pixel_aspect;
} Q3EGLConfig_t;

Q3EGLConfig_t q3e_glConfig;

typedef struct EGLConfigInfo_s
{
	EGLint red;
	EGLint green;
	EGLint blue;
	EGLint alpha;
	EGLint buffer;
	EGLint depth;
	EGLint stencil;
	EGLint samples;
	EGLint sample_buffers;
} EGLConfigInfo_t;

static void Q3E_HandleEGLError(const char *func, Q3Ebool exit)
{
	static const char *GLimp_StringErrors[] = {
			"EGL_SUCCESS",
			"EGL_NOT_INITIALIZED",
			"EGL_BAD_ACCESS",
			"EGL_BAD_ALLOC",
			"EGL_BAD_ATTRIBUTE",
			"EGL_BAD_CONFIG",
			"EGL_BAD_CONTEXT",
			"EGL_BAD_CURRENT_SURFACE",
			"EGL_BAD_DISPLAY",
			"EGL_BAD_MATCH",
			"EGL_BAD_NATIVE_PIXMAP",
			"EGL_BAD_NATIVE_WINDOW",
			"EGL_BAD_PARAMETER",
			"EGL_BAD_SURFACE",
			"EGL_CONTEXT_LOST",
	};
	EGLint err = eglGetError();
	if(err == EGL_SUCCESS)
		return;

	if(exit)
		Q3E_DEBUGF("[Harmattan]: EGL error %s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
	else
		Q3E_PRINTF("[Harmattan]: EGL error %s: 0x%04x: %s\n", func, err, GLimp_StringErrors[err - EGL_SUCCESS]);
}

static EGLConfigInfo_t Q3E_GetFormatConfig(int gl_format, int gl_depth_bits)
{
	int red_bits = 8;
	int green_bits = 8;
	int blue_bits = 8;
	int alpha_bits = 8;
	int buffer_bits = 32;
	int depth_bits = 24;
	int stencil_bits = 8;

	switch(gl_format)
	{
		case GLFORMAT_RGB565:
			red_bits = 5;
			green_bits = 6;
			blue_bits = 5;
			alpha_bits = 0;
			//depth_bits = 16;
			buffer_bits = 16;
			break;
		case GLFORMAT_RGBA4444:
			red_bits = 4;
			green_bits = 4;
			blue_bits = 4;
			alpha_bits = 4;
			//depth_bits = 16;
			buffer_bits = 16;
			break;
		case GLFORMAT_RGBA5551:
			red_bits = 5;
			green_bits = 5;
			blue_bits = 5;
			alpha_bits = 1;
			//depth_bits = 16;
			buffer_bits = 16;
			break;
		case GLFORMAT_RGBA1010102:
			red_bits = 10;
			green_bits = 10;
			blue_bits = 10;
			alpha_bits = 2;
			//depth_bits = 24;
			buffer_bits = 32;
			break;
		case GLFORMAT_RGBA8888:
		default:
			red_bits = 8;
			green_bits = 8;
			blue_bits = 8;
			alpha_bits = 8;
			//depth_bits = 24;
			buffer_bits = 32;
			break;
	}

	switch(gl_depth_bits)
	{
		case 16:
			depth_bits = 16;
			break;
		case 32:
			depth_bits = 32;
			break;
		case 24:
		default:
			depth_bits = 24;
			break;
	}

	EGLConfigInfo_t info;
	info.red = red_bits;
	info.green = green_bits;
	info.blue = blue_bits;
	info.alpha = alpha_bits;
	info.buffer = buffer_bits;
	info.depth = depth_bits;
	info.stencil = stencil_bits;
	return info;
}

static int Q3E_CompareEGLConfig(const void *left, const void *right)
{
	const EGLConfig lhs = *(EGLConfig *)left;
	const EGLConfig rhs = *(EGLConfig *)right;
	EGLConfigInfo_t info = Q3E_GetFormatConfig(gl_format, gl_depth_bits);
	int r = info.red;
	int g = info.green;
	int b = info.blue;
	int a = info.alpha;
	int d = info.depth;
	int s = info.samples;

	int lr, lg, lb, la, ld, ls;
	int rr, rg, rb, ra, rd, rs;
	int rat1, rat2;

	eglGetConfigAttrib(eglDisplay, lhs, EGL_RED_SIZE, &lr);
	eglGetConfigAttrib(eglDisplay, lhs, EGL_GREEN_SIZE, &lg);
	eglGetConfigAttrib(eglDisplay, lhs, EGL_BLUE_SIZE, &lb);
	eglGetConfigAttrib(eglDisplay, lhs, EGL_ALPHA_SIZE, &la);

	eglGetConfigAttrib(eglDisplay, rhs, EGL_RED_SIZE, &rr);
	eglGetConfigAttrib(eglDisplay, rhs, EGL_GREEN_SIZE, &rg);
	eglGetConfigAttrib(eglDisplay, rhs, EGL_BLUE_SIZE, &rb);
	eglGetConfigAttrib(eglDisplay, rhs, EGL_ALPHA_SIZE, &ra);

	rat1 = (abs(lr - r) + abs(lg - g) + abs(lb - b));//*1000000-(ld*10000+la*100+ls);
	rat2 = (abs(rr - r) + abs(rg - g) + abs(rb - b));//*1000000-(rd*10000+ra*100+rs);

	if(rat1 == rat2)
	{
        eglGetConfigAttrib(eglDisplay, lhs, EGL_DEPTH_SIZE, &ld);
        //eglGetConfigAttrib(eglDisplay, lhs, EGL_STENCIL_SIZE, &ls);

        eglGetConfigAttrib(eglDisplay, rhs, EGL_DEPTH_SIZE, &rd);
        //eglGetConfigAttrib(eglDisplay, rhs, EGL_STENCIL_SIZE, &rs);

        rat1 = abs(ld - d);
        rat2 = abs(rd - d);
	}

	if(rat1 == rat2)
	{
        eglGetConfigAttrib(eglDisplay, lhs, EGL_SAMPLES, &ls);

        eglGetConfigAttrib(eglDisplay, rhs, EGL_SAMPLES, &rs);

        rat1 = abs(ls - s);
        rat2 = abs(rs - s);
	}

	return rat1 - rat2;
}

static EGLConfig Q3E_ChooseEGLConfig(EGLConfig configs[], int size)
{
	qsort(configs, size, sizeof(EGLConfig), Q3E_CompareEGLConfig);
	return configs[0];
}

static EGLConfigInfo_t Q3E_GetConfigInfo(const EGLConfig eglConfig)
{
	EGLConfigInfo_t info;

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_RED_SIZE, &info.red);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_GREEN_SIZE, &info.green);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_BLUE_SIZE, &info.blue);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_ALPHA_SIZE, &info.alpha);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_DEPTH_SIZE, &info.depth);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_STENCIL_SIZE, &info.stencil);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_BUFFER_SIZE, &info.buffer);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLES, &info.samples);
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLE_BUFFERS, &info.sample_buffers);

	return info;
}

#ifdef Q3E_GL_DEBUG_OUTPUT
static void Q3E_OutputOpenGLCallback_f(GLenum source,
                                 GLenum type,
                                 GLuint id,
                                 GLenum severity,
                                 GLsizei length,
                                 const GLchar* message,
                                 const void* userParam)
{
    fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
             ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
             type, severity, message );
}

static void Q3E_DebugOpenGL(Q3Ebool on)
{
    if(on)
    {
        GLint flags = 0;
        qglGetIntegerv(GL_CONTEXT_FLAGS, &flags);

        printf("DDD %x %p %p %p\n", flags & GL_CONTEXT_FLAG_DEBUG_BIT, qglEnable, qglDebugMessageCallback, qglDisable);
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
#endif

static void Q3E_PrintGLConfig()
{
    if(q3e_glConfig.minor_version >= 0)
	    Q3E_PRINTF( "[Harmattan]: OpenGLES version: %d.%d\n", q3e_glConfig.major_version, q3e_glConfig.minor_version);
    else
	    Q3E_PRINTF( "[Harmattan]: OpenGLES version: %d\n", q3e_glConfig.major_version);

    Q3E_PRINTF( "[Harmattan]: EGL version: %d.%d\n", q3e_glConfig.egl_major_version, q3e_glConfig.egl_minor_version);

	Q3E_PRINTF( "[Harmattan]: EGL context: %d/%d/%d Color bits, %d Alpha bits.\n%d depth, %d stencil display.\nsamples %d, sample buffers %d.\n debug output: %d\n",
					q3e_glConfig.red, q3e_glConfig.green, q3e_glConfig.blue, q3e_glConfig.alpha,
					q3e_glConfig.depth, q3e_glConfig.stencil,
			        q3e_glConfig.samples, q3e_glConfig.sample_buffers,
			        q3e_glConfig.debug_output
	);
	Q3E_PRINTF( "[Harmattan]: EGL surface: stereo=%d, display frequency=%d\nscreen size=%dx%d, aspect=%f\nfullscreen=%d\n",
					q3e_glConfig.stereo, q3e_glConfig.display_frequency,
					q3e_glConfig.window_width, q3e_glConfig.window_height, q3e_glConfig.pixel_aspect,
					q3e_glConfig.fullscreen
	);
}

static void Q3E_PrintGLInfo(int version)
{
	const char * (* _qglGetString)(GLenum);
	_qglGetString = Q3E_PTR_CAST(const char * (*)(GLenum), eglGetProcAddress("glGetString"));

	const char *glstring;
	glstring = (const char *) _qglGetString(GL_RENDERER);
	Q3E_PRINTF("GL_RENDERER: %s\n", glstring);

	glstring = (const char *) _qglGetString(GL_VERSION);
	Q3E_PRINTF("GL_VERSION: %s\n", glstring);

	glstring = (const char *) _qglGetString(GL_EXTENSIONS);
	Q3E_PRINTF("GL_EXTENSIONS: %s\n", glstring);

#ifdef GL_SHADING_LANGUAGE_VERSION
	if(version > 1)
	{
        glstring = (const char *) _qglGetString(GL_SHADING_LANGUAGE_VERSION);
        Q3E_PRINTF("GL_SHADING_LANGUAGE_VERSION: %s\n", glstring);
	}
#endif
}

static EGLenum Q3E_GetEGLRenderType(int majorVersion)
{
    switch(majorVersion)
    {
        case 3: return EGL_OPENGL_ES3_BIT;
        case 2: return EGL_OPENGL_ES2_BIT;
        case 1: return EGL_OPENGL_ES_BIT;
        default: {
            Q3E_ERRORF("Unset OpenGLES version!");
            return 0;
        }
    }
}

static EGLBoolean Q3E_GetEGLDebugOutput(int majorVersion, int minorVersion)
{
    return majorVersion >= 3 && minorVersion >= 1;
}

static Q3Ebool Q3E_OpenDisplay(void)
{
	if(eglDisplay) {
		return Q3E_TRUE;
	}

	Q3E_PRINTF( "Setup EGL display connection\n" );

	if ( !( eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY) ) ) {
		Q3E_PRINTF( "Couldn't open the EGL display\n" );
		return Q3E_FALSE;
	}

	return Q3E_TRUE;
}

static bool GLES_Init_special(void)
{
	EGLint config_count = 0;
	EGLConfigInfo_t info = Q3E_GetFormatConfig(gl_format, gl_depth_bits);
	int stencil_bits = info.stencil;
	int depth_bits = info.depth;
	int red_bits = info.red;
	int green_bits = info.green;
	int blue_bits = info.blue;
	int alpha_bits = info.alpha;
	int buffer_bits = info.buffer;

	EGLint attrib[] = {
			EGL_BUFFER_SIZE, buffer_bits,
			EGL_ALPHA_SIZE, alpha_bits,
			EGL_RED_SIZE, red_bits,
			EGL_BLUE_SIZE, green_bits,
			EGL_GREEN_SIZE, blue_bits,
			EGL_DEPTH_SIZE, depth_bits,
			EGL_STENCIL_SIZE, stencil_bits,
			EGL_SAMPLE_BUFFERS, gl_multiSamples > 1 ? 1 : 0,
			EGL_SAMPLES, gl_multiSamples,
			EGL_RENDERABLE_TYPE, gl_renderApi,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_NONE,
	};

	Q3E_PRINTF( "[Harmattan]: Request special EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d sample buffers %d.\n",
					red_bits, green_bits,
					blue_bits, alpha_bits,
					depth_bits,
					stencil_bits
			, attrib[15], attrib[17]
	);

	int multisamples = gl_multiSamples;
	EGLConfig eglConfigs[MAX_NUM_CONFIGS];
	while(1)
	{
		if (!eglChooseConfig (eglDisplay, attrib, eglConfigs, MAX_NUM_CONFIGS, &config_count))
		{
			Q3E_HandleEGLError("eglChooseConfig", Q3E_FALSE);

			if(multisamples > 1) {
				multisamples = (multisamples <= 2) ? 0 : (multisamples/2);

				attrib[7 * 2 + 1] = multisamples > 1 ? 1 : 0;
				attrib[8 * 2 + 1] = multisamples;
				continue;
			}
			else
			{
				return Q3E_FALSE;
			}
		}
		else
		{
			Q3E_PRINTF( "[Harmattan]: Get EGL context num -> %d.\n", config_count);
			for(int i = 0; i < config_count; i++)
			{
				EGLConfigInfo_t cinfo = Q3E_GetConfigInfo(eglConfigs[i]);
				Q3E_PRINTF("\t%d EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d sample buffers %d.\n",
							   i + 1,
							   cinfo.red, cinfo.green,
							   cinfo.blue, cinfo.alpha,
							   cinfo.depth,
							   cinfo.stencil
						, cinfo.samples, cinfo.sample_buffers
				);
			}
		}
		break;
	}
	configs[0] = Q3E_ChooseEGLConfig(eglConfigs, config_count);

    EGLConfigInfo_t cinfo = Q3E_GetConfigInfo(configs[0]);
    Q3E_PRINTF("Choose EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d sample buffers %d.\n",
                   cinfo.red, cinfo.green,
                   cinfo.blue, cinfo.alpha,
                   cinfo.depth,
                   cinfo.stencil
            , cinfo.samples, cinfo.sample_buffers
    );

	return Q3E_TRUE;
}

static bool GLES_Init_prefer(void)
{
	EGLint config_count = 0;
	int colorbits = 24;
	int depthbits = 24;
	int stencilbits = 8;
	bool suc;

	for (int i = 0; i < 16; i++) {

		int multisamples = gl_multiSamples;
		suc = Q3E_FALSE;

		// 0 - default
		// 1 - minus colorbits
		// 2 - minus depthbits
		// 3 - minus stencil
		if ((i % 4) == 0 && i) {
			// one pass, reduce
			switch (i / 4) {
				case 2 :
					if (colorbits == 24)
						colorbits = 16;
					break;
				case 1 :
					if (depthbits == 24)
						depthbits = 16;
					else if (depthbits == 16)
						depthbits = 8;
				case 3 :
					if (stencilbits == 24)
						stencilbits = 16;
					else if (stencilbits == 16)
						stencilbits = 8;
			}
		}

		int tcolorbits = colorbits;
		int tdepthbits = depthbits;
		int tstencilbits = stencilbits;

		if ((i % 4) == 3) {
			// reduce colorbits
			if (tcolorbits == 24)
				tcolorbits = 16;
		}

		if ((i % 4) == 2) {
			// reduce depthbits
			if (tdepthbits == 24)
				tdepthbits = 16;
			else if (tdepthbits == 16)
				tdepthbits = 8;
		}

		if ((i % 4) == 1) {
			// reduce stencilbits
			if (tstencilbits == 24)
				tstencilbits = 16;
			else if (tstencilbits == 16)
				tstencilbits = 8;
			else
				tstencilbits = 0;
		}

		int channelcolorbits = 4;
		if (tcolorbits == 24)
			channelcolorbits = 8;

		int talphabits = channelcolorbits;

		EGLint attrib[] = {
				EGL_BUFFER_SIZE, channelcolorbits * 4,
				EGL_ALPHA_SIZE, talphabits,
				EGL_RED_SIZE, channelcolorbits,
				EGL_BLUE_SIZE, channelcolorbits,
				EGL_GREEN_SIZE, channelcolorbits,
				EGL_DEPTH_SIZE, tdepthbits,
				EGL_STENCIL_SIZE, tstencilbits,
				EGL_SAMPLE_BUFFERS, multisamples > 1 ? 1 : 0,
				EGL_SAMPLES, multisamples,
				EGL_RENDERABLE_TYPE, gl_renderApi,
				EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
				EGL_NONE,
		};

		Q3E_PRINTF( "[Harmattan]: Request EGL context: %d/%d/%d Color bits, %d Alpha bits, %d depth, %d stencil display. samples %d, sample buffers %d.\n",
						channelcolorbits, channelcolorbits,
						channelcolorbits, talphabits,
						tdepthbits,
						tstencilbits
				, attrib[15], attrib[17]
		);

		while(1)
		{
			if (!eglChooseConfig (eglDisplay, attrib, configs, 1, &config_count))
			{
				Q3E_HandleEGLError("eglChooseConfig", Q3E_FALSE);

				if(multisamples > 1) {
					multisamples = (multisamples <= 2) ? 0 : (multisamples/2);

					attrib[7 * 2 + 1] = multisamples > 1 ? 1 : 0;
					attrib[8 * 2 + 1] = multisamples;
					continue;
				}
				else
				{
					break;
				}
			}
			suc = Q3E_TRUE;
			break;
		}

		if(suc)
			break;
	}
	return suc;
}

Q3Ebool GLES_Init(void) {
	EGLint major, minor;

    if(gl_msaa < 0)
	    Q3E_PRINTF( "[Harmattan]: Q3E EGL initial config: game samples=%d\n", q3e_glConfig.samples);
    else
	    Q3E_PRINTF( "[Harmattan]: Q3E EGL initial config: system samples=%d, %d\n", gl_msaa, q3e_glConfig.samples);

	if (!eglInitialize(eglDisplay, &major, &minor))
	{
		Q3E_HandleEGLError("eglInitialize", Q3E_TRUE);
		return Q3E_FALSE;
	}

	q3e_glConfig.egl_major_version = major;
	q3e_glConfig.egl_minor_version = minor;

	Q3E_PRINTF("[Harmattan]: Initializing OpenGL display\n");

    gl_renderApi = Q3E_GetEGLRenderType(q3e_glConfig.major_version);
	gl_multiSamples = gl_msaa < 0 ? q3e_glConfig.samples : gl_msaa;
    gl_debugOutput = Q3E_GetEGLDebugOutput(q3e_glConfig.major_version, q3e_glConfig.minor_version) && q3e_glConfig.debug_output ? EGL_TRUE : EGL_FALSE;
    Q3E_PRINTF( "[Harmattan]: Q3E EGL target config: GLES api=%d, samples=%d, debug output: %d\n", gl_renderApi, gl_multiSamples, gl_debugOutput);

	if(!GLES_Init_special())
	{
		if(!GLES_Init_prefer())
		{
			Q3E_ERRORF("Initializing EGL error");
			return Q3E_FALSE;
		}
	}

	eglConfig = configs[0];

	// eglGetConfigAttrib(eglDisplay, eglConfig, EGL_SAMPLES, &gl_multiSamples);

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &format);
	ANativeWindow_setBuffersGeometry((ANativeWindow *)win, screen_width, screen_height, format);

	if ((eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType) win, NULL)) == EGL_NO_SURFACE)
	{
		Q3E_HandleEGLError("eglCreateWindowSurface", Q3E_TRUE);
		return Q3E_FALSE;
	}

    if(q3e_glConfig.minor_version >= 0)
    {
        EGLint ctxAttrib[] = {
                EGL_CONTEXT_MAJOR_VERSION_KHR, q3e_glConfig.major_version,
                EGL_CONTEXT_MINOR_VERSION_KHR, q3e_glConfig.minor_version,
//#ifdef Q3E_GL_DEBUG_OUTPUT
                EGL_CONTEXT_OPENGL_DEBUG, gl_debugOutput,
//#endif
                EGL_NONE
        };
        if(!gl_debugOutput)
            ctxAttrib[4] = EGL_NONE;
        eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttrib);
    }
    else
    {
        EGLint ctxAttrib[] = {
                EGL_CONTEXT_CLIENT_VERSION, q3e_glConfig.major_version,
//#ifdef Q3E_GL_DEBUG_OUTPUT
                EGL_CONTEXT_OPENGL_DEBUG, gl_debugOutput,
//#endif
                EGL_NONE
        };
        if(!gl_debugOutput)
            ctxAttrib[2] = EGL_NONE;
        eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttrib);
    }
    if (eglContext == EGL_NO_CONTEXT)
	{
		Q3E_HandleEGLError("eglCreateContext", Q3E_TRUE);
		return Q3E_FALSE;
	}

	if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
	{
		Q3E_HandleEGLError("eglMakeCurrent", Q3E_TRUE);
		return Q3E_FALSE;
	}

	EGLConfigInfo_t info = Q3E_GetConfigInfo(eglConfig);

	q3e_glConfig.red = info.red;
	q3e_glConfig.green = info.green;
	q3e_glConfig.blue = info.blue;
	q3e_glConfig.alpha = info.alpha;
	q3e_glConfig.depth = info.depth;
	q3e_glConfig.stencil = info.stencil;
	q3e_glConfig.buffer = info.red + info.green + info.blue;

	q3e_glConfig.fullscreen = 1;
	q3e_glConfig.stereo = Q3E_FALSE;
	q3e_glConfig.window_width = screen_width;
	q3e_glConfig.window_height = screen_height;
	q3e_glConfig.display_frequency = refresh_rate;
	q3e_glConfig.samples = info.samples > 1 ? info.samples : 0;
	q3e_glConfig.sample_buffers = info.sample_buffers;
	q3e_glConfig.pixel_aspect = 1.0f;

    Q3E_PrintGLConfig();
	Q3E_POST_GL_INIT;

	return Q3E_TRUE;
}

Q3Ebool Q3E_InitGL(void)
{
	if (!Q3E_OpenDisplay()) {
		return Q3E_FALSE;
	}

	if (!GLES_Init()) {
		return Q3E_FALSE;
	}

	Q3E_PrintGLInfo(q3e_glConfig.major_version);
	//has_gl_context = Q3E_TRUE;

	return Q3E_TRUE;
}

Q3Ebool Q3E_RequireWindow(volatile ANativeWindow *w)
{
	if(!w)
		return Q3E_FALSE;

	win = w;
	ANativeWindow_acquire((ANativeWindow *)win);
	return Q3E_TRUE;
}

void Q3E_ReleaseWindow(volatile ANativeWindow *w)
{
	if(!win)
		return;

    ANativeWindow_release((ANativeWindow *)win);
    win = NULL;
}

void Q3E_SwapInterval(int interval)
{
    if(eglDisplay != EGL_NO_DISPLAY)
        eglSwapInterval(eglDisplay, interval);
}

Q3Ebool Q3E_QuerySurfaceSize(int *width, int *height)
{
    if(eglDisplay != EGL_NO_DISPLAY && eglSurface != EGL_NO_SURFACE)
    {
        eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, width);
        eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, height);
        return Q3E_TRUE;
    }
    return Q3E_FALSE;
}

void Q3E_RestoreGL(void)
{
	if(eglDisplay == EGL_NO_DISPLAY)
		return;

	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &format);

    ANativeWindow_setBuffersGeometry((ANativeWindow *)win, screen_width, screen_height, format);

	if ((eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType) win, NULL)) == EGL_NO_SURFACE)
	{
		Q3E_HandleEGLError("eglCreateWindowSurface", Q3E_TRUE);
		return;
	}

	if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext))
	{
		Q3E_HandleEGLError("eglMakeCurrent", Q3E_TRUE);
		return;
	}
	//has_gl_context = Q3E_TRUE;
	Q3E_PRINTF("[Harmattan]: EGL surface created and using EGL context.\n");
}

void Q3E_DestroyGL(Q3Ebool full)
{
	//has_gl_context = Q3E_FALSE;
	if(!win)
		return;

	if(eglDisplay != EGL_NO_DISPLAY)
		Q3E_DeactivateContext();
	if(eglSurface != EGL_NO_SURFACE)
	{
		eglDestroySurface(eglDisplay, eglSurface);
		eglSurface = EGL_NO_SURFACE;
	}

	if(full)
	{
        ANativeWindow_release((ANativeWindow *)win);
        win = NULL;
	    Q3E_DEBUGF("[Harmattan]: ANativeWindow released.\n");
	}
	Q3E_DEBUGF("[Harmattan]: EGL surface destroyed and no EGL context.\n");
}

void Q3E_ShutdownGL(void)
{
	//has_gl_context = false;
	if(eglDisplay != EGL_NO_DISPLAY)
	    Q3E_DeactivateContext();
	if(eglContext != EGL_NO_CONTEXT)
	{
		eglDestroyContext(eglDisplay, eglContext);
		eglContext = EGL_NO_CONTEXT;
	}
	if(eglSurface != EGL_NO_SURFACE)
	{
		eglDestroySurface(eglDisplay, eglSurface);
		eglSurface = EGL_NO_SURFACE;
	}
	if(eglDisplay != EGL_NO_DISPLAY)
	{
		eglTerminate(eglDisplay);
		eglDisplay = EGL_NO_DISPLAY;
	}

	Q3E_DEBUGF("[Harmattan]: EGL destroyed.\n");
}

void Q3E_SetupMultiSamples(int multisamples)
{
	gl_multiSamples = gl_msaa < 0 ? multisamples : gl_msaa;
}

Q3Ebool Q3E_GL_ExtensionSupported(const char *name)
{
	const char * (* _qglGetString)(GLenum);
	_qglGetString = Q3E_PTR_CAST(const char * (*)(GLenum), eglGetProcAddress("glGetString"));

    const char *exts = (const char *)_qglGetString (GL_EXTENSIONS);
    if(!exts)
        return Q3E_FALSE;

    size_t size = strlen(exts) + 1 + 1;
    char *new_exts = (char *)malloc(size);
    dp_strlcpy(new_exts, exts, size - 2);
    new_exts[size - 2] = ' ';
    new_exts[size - 1] = '\0';

    size_t name_size = strlen(name) + 1 + 1;
    char *new_name = (char *)malloc(name_size);
    dp_strlcpy(new_name, name, name_size - 2);
    new_name[name_size - 2] = ' ';
    new_name[name_size - 1] = '\0';

    Q3Ebool has = strstr(new_exts, new_name) != NULL;
	Q3E_PRINTF("[Harmattan]: OpenGL extension '%s' -> %s\n", name, has ? "support" : "missing");
    free(new_exts);
    free(new_name);
	return has;
}
