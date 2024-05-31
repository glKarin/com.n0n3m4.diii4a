#include "RenderThread.h"

#include "../../sys/sys_public.h"

#define RENDER_THREAD_STARTED() (Sys_ThreadIsRunning(&render_thread))

bool multithreadActive = false;

static idRenderThread renderThreadInstance;
idRenderThread *renderThread = &renderThreadInstance;

extern void GLimp_ActivateContext();
extern void GLimp_DeactivateContext();

static idCVar harm_r_multithread("harm_r_multithread", "0", CVAR_ARCHIVE | CVAR_INIT | CVAR_BOOL | CVAR_RENDERER, "Multithread backend");

static void * BackendThread(void *data)
{
    renderThreadInstance.render_thread_finished = false;
    Sys_Printf("[Harmattan]: Enter doom3 render thread -> %s\n", Sys_GetThreadName());
    GLimp_ActivateContext();
    while(true)
    {
        renderThreadInstance.BackendThreadTask();
        if(THREAD_CANCELED(renderThreadInstance.render_thread) || renderThreadInstance.backendThreadShutdown)
            break;
    }
    GLimp_DeactivateContext();
    Sys_Printf("[Harmattan]: Leave doom3 render thread -> %s\n", RENDER_THREAD_NAME);
    renderThreadInstance.render_thread_finished = true;
    Sys_TriggerEvent(TRIGGER_EVENT_RENDER_THREAD_FINISHED);
    return 0;
}

idRenderThread::idRenderThread()
: backendThreadShutdown(false),
  render_thread_finished(false),
  backendFinished(true),
  fdToRender(NULL),
  vertListToRender(0),
  pixelsCrop(NULL),
  pixels(NULL)
{
    memset(&render_thread, 0, sizeof(render_thread));
}

void idRenderThread::BackendThreadExecute(void)
{
    if(!multithreadActive)
        return;
    if (RENDER_THREAD_STARTED())
        return;
    GLimp_DeactivateContext();
    backendThreadShutdown = false;
    Sys_CreateThread(BackendThread, common, THREAD_HIGHEST, render_thread, RENDER_THREAD_NAME, g_threads, &g_thread_count);
    common->Printf("[Harmattan]: Render thread start -> %lu(%s)\n", render_thread.threadHandle, RENDER_THREAD_NAME);
}

void idRenderThread::BackendThreadShutdown(void)
{
    if(!multithreadActive)
        return;
    if (!RENDER_THREAD_STARTED())
        return;
    BackendThreadWait();
    backendThreadShutdown = true;
    Sys_TriggerEvent(TRIGGER_EVENT_RUN_BACKEND);
    Sys_DestroyThread(render_thread);
    while(!render_thread_finished)
        Sys_WaitForEvent(TRIGGER_EVENT_RENDER_THREAD_FINISHED);
    GLimp_ActivateContext();
    common->Printf("[Harmattan]: Render thread shutdown -> %s\n", RENDER_THREAD_NAME);
}

void idRenderThread::BackendThreadTask(void) // BackendThread ->
{
    // waiting start
    Sys_WaitForEvent(TRIGGER_EVENT_RUN_BACKEND);
    // Purge all images,  Load all images
    globalImages->HandlePendingImage();
    // Load custom GLSL shader
    shaderManager->ActuallyLoad();
    // debug tools
    RB_SetupRenderTools();
    // image process finished
    Sys_TriggerEvent(TRIGGER_EVENT_IMAGES_PROCESSES);

    int backendVertexCache = vertListToRender;
    vertexCache.BeginBackEnd(backendVertexCache);
    R_IssueRenderCommands(fdToRender);

    // Take screen shot
    if(pixels) // if block backend rendering, do not exit backend render function, because it will be swap buffers in GLSurfaceView
    {
        qglReadPixels( pixelsCrop->x, pixelsCrop->y, pixelsCrop->width, pixelsCrop->height, GL_RGBA, GL_UNSIGNED_BYTE, (void*)pixels );
        pixels = NULL;
        pixelsCrop = NULL;
    }

    vertexCache.EndBackEnd(backendVertexCache);
    backendFinished = true;
    Sys_TriggerEvent(TRIGGER_EVENT_BACKEND_FINISHED);
}


// waiting backend render finished
void idRenderThread::BackendThreadWait(void)
{
    while(/*multithreadActive &&*/ !backendFinished)
    {
        //usleep(1000 * 3);
        Sys_WaitForEvent(TRIGGER_EVENT_BACKEND_FINISHED);
        //usleep(500);
    }
}

bool Sys_InRenderThread(void)
{
    return Sys_InThread(&renderThreadInstance.render_thread);
}

void Sys_ShutdownRenderThread(void)
{
    if(multithreadActive)
    {
        Sys_TriggerEvent(TRIGGER_EVENT_RUN_BACKEND);
        renderThreadInstance.BackendThreadShutdown();
    }
}