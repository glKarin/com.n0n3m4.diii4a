//
// Created by n950 on 2024/11/16.
//
#include <SDL.h>

#include "q3e/q3e_android.h"

void ShutdownGame() {}
SDL_bool com_fullyInitialized;

#define Q3E_GAME_NAME "Test_SDL2"
#define Q3E_IS_INITIALIZED (com_fullyInitialized)
#define Q3E_PRINTF printf
#define Q3E_SHUTDOWN_GAME ShutdownGame()
#define Q3Ebool SDL_bool
#define Q3E_TRUE SDL_TRUE
#define Q3E_FALSE SDL_FALSE
#define Q3E_REQUIRE_THREAD
#define Q3E_INIT_WINDOW GLimp_AndroidOpenWindow
#define Q3E_QUIT_WINDOW GLimp_AndroidQuit
#define Q3E_CHANGE_WINDOW GLimp_AndroidInit

void GLimp_AndroidOpenWindow(volatile ANativeWindow *win){}
void GLimp_AndroidInit(volatile ANativeWindow *win){}
void GLimp_AndroidQuit(void){}

void Q3E_MotionEvent(float dx, float dy){}
void Q3E_KeyEvent(int state,int key,int character){}

#include "q3e/q3e_android.inc"

#include <time.h>
#include <stdio.h>
#include <GLES/gl.h>
#include <android/log.h>
extern SDL_mutex *Android_ActivityMutex;

#define LOGI(fmt, args...) {  __android_log_print(ANDROID_LOG_INFO, "XXX", fmt, ##args); }
#define LOGE(fmt, args...) {  __android_log_print(ANDROID_LOG_ERROR, "XXX", fmt, ##args); }

static SDL_Window *window;
static SDL_GLContext context;

static void CreateGLContext()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_STEREO, 0);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

    // Get GLES2 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    int glMajorVersion = 1;
    int glMinorVersion = 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, glMajorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, glMinorVersion);

    window = SDL_CreateWindow("ENGINE_VERSION",
                                          SDL_WINDOWPOS_UNDEFINED_DISPLAY(0),
                                          SDL_WINDOWPOS_UNDEFINED_DISPLAY(0),
                                          512, 512, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);

    LOGE("www %p", window);

    context = SDL_GL_CreateContext(window);
    LOGE("ccc %p", context);

    int r, g, b, a, d, s;
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &r);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &g);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &b);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &a);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &d);
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &s);

    LOGE("Got %d stencil bits, %d depth bits, color bits: r%d g%d b%d a%d\n", s, d, r, g, b, a);
}

static void ClearGLError()
{
    while(glGetError() != GL_NO_ERROR);
}

static void CheckGLError()
{
    int i = 0;
    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
    {
        i++;

        LOGE("GL_ERROR %x", err);
    }
}
#define CHECK_GL_ERROR(x) do { ClearGLError(); x; CheckGLError(); } while(0);

static void InitGL()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glMatrixMode(GL_PROJECTION);
    glOrthof(-1, 1, -1, 1, -1, 1);
    //LOGI("InitGL");

    setState(STATE_GAME);
}

static void FinishRender()
{
    glFlush();

    SDL_GL_SwapWindow(window);
}

static void Render()
{
    GLfloat vs[] = {
            0, 0,
            0, 1,
            1, 0,
            1, 1
    };
    CHECK_GL_ERROR(
            {
                InitGL();
                glVertexPointer(2, GL_FLOAT, 0, vs);
                glColor4f(1.0, 1.0, 0, 1.0);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
    )
}

static void handle_event(const SDL_Event *e)
{
    switch (e->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            LOGI("Key %s: %d %d", e->type == SDL_KEYDOWN ? "down" : "up", e->key.keysym, e->key.state);
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        LOGI("Mouse %s: %d %d (%d x %d)", e->type == SDL_MOUSEBUTTONDOWN ? "down" : "up", e->button.button, e->button.state, e->button.x, e->button.y);
            break;
        case SDL_MOUSEMOTION:
        LOGI("Mouse motion: %d (%d x %d) (%d x %d)", e->motion.state, e->motion.xrel, e->motion.yrel, e->motion.x, e->motion.y);
            break;
        default:
            LOGI("Other Event: %d", e->type);
            break;
    }

}

int SDL_main()
{
    CreateGLContext();

    InitGL();

    int i = 0;
    const char * a = "0";
    SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, a);
    while(1) {

        //SDL_PumpEvents();
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            handle_event(&e);
        }

        //__android_log_print(ANDROID_LOG_ERROR, "XX", "%d: %zd\n", i++, time(NULL));

        Render();

        FinishRender();

        SDL_Delay(1000);
    }
}

#undef main
int main(int argc, char **argv)
{
    CreateGLContext();

    InitGL();

    int i = 0;
    const char * a = "0";
    SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, a);
    while(1) {

        //SDL_PumpEvents();
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            LOGE("EEE %x", e.type);
        }

        //__android_log_print(ANDROID_LOG_ERROR, "XX", "%d: %zd\n", i++, time(NULL));

        Render();

        FinishRender();

        SDL_Delay(2000);
    }
}
