#include "RenderThread.h"

#include "../../sys/sys_public.h"

#define RENDER_THREAD_STARTED() (Sys_ThreadIsRunning(&render_thread))

bool multithreadActive = false;

static idRenderThread renderThreadInstance;
idRenderThread *renderThread = &renderThreadInstance;

extern void GLimp_ActivateContext();
extern void GLimp_DeactivateContext();
extern void RB_GLSL_HandleShaders(void);

static idCVar harm_r_multithread("harm_r_multithread",
#ifdef __ANDROID__
                                 "0"
#else
        "1"
#endif
                                 , CVAR_ARCHIVE | CVAR_INIT | CVAR_BOOL | CVAR_RENDERER, "Multithread backend");

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
    return NULL;
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
//    imagesAlloc.Resize( 1024, 1024 );
//    imagesPurge.Resize( 1024, 1024 );
#ifdef _QUEUE_LIST_RECYCLE
	imagesAlloc.SetRecycle(true);
	imagesPurge.SetRecycle(true);
#endif
}

void idRenderThread::BackendThreadExecute( void )
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

void idRenderThread::BackendThreadShutdown( void )
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

// only render thread is running and main thread is waiting
ID_INLINE static void RB_OnlyRenderThreadRunningAndMainThreadWaiting(void)
{
    // Load custom GLSL shader or reload GLSL shaders
    RB_GLSL_HandleShaders();
    // debug tools
    RB_SetupRenderTools();
#ifdef _IMGUI
    // start imgui
    RB_ImGui_Start();
#endif
}

void idRenderThread::BackendThreadTask( void ) // BackendThread ->
{
    // waiting start
    Sys_WaitForEvent(TRIGGER_EVENT_RUN_BACKEND);

    // Purge all images,  Load all images
    this->HandlePendingImage();

    // main thread is waiting render thread, only render thread is running
    RB_OnlyRenderThreadRunningAndMainThreadWaiting();

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
void idRenderThread::BackendThreadWait( void )
{
    while(/*multithreadActive &&*/ !backendFinished)
    {
        //usleep(1000 * 3);
        Sys_WaitForEvent(TRIGGER_EVENT_BACKEND_FINISHED);
        //usleep(500);
    }
}

bool idRenderThread::IsActive( void ) const
{
    return multithreadActive && Sys_ThreadIsRunning(&render_thread);
}

#if 1
#define HARM_MT_IMAGES_DEBUG(...)
#else
#define HARM_MT_IMAGES_DEBUG(...) printf("[MT]: " __VA_ARGS__)
#endif
void idRenderThread::AddAllocList( idImage * image, bool checkForPrecompressed, bool fromBackEnd )
{
    // Front and backend threads can add images, protect this
    Sys_EnterCriticalSection( CRITICAL_SECTION_TWO );

    if(image)
    {
        HARM_MT_IMAGES_DEBUG("AddAllocList::before num = %d\n", imagesAlloc.Num());
        imagesAlloc.Append( ActuallyLoadImage_data_t( image, checkForPrecompressed, fromBackEnd ) );
        HARM_MT_IMAGES_DEBUG("AddAllocList::after num = %d\n", imagesAlloc.Num());
    }

    Sys_LeaveCriticalSection( CRITICAL_SECTION_TWO );
}

void idRenderThread::AddPurgeList( idImage * image )
{
    if(image)
    {
        HARM_MT_IMAGES_DEBUG("AddPurgeList::before num = %d\n", imagesPurge.Num());
        imagesPurge.Append( image );
        HARM_MT_IMAGES_DEBUG("AddPurgeList::after num = %d\n", imagesPurge.Num());
        image->purgePending = true;
    }
}

bool idRenderThread::GetNextAllocImage( ActuallyLoadImage_data_t &img )
{
    HARM_MT_IMAGES_DEBUG("GetNextAllocImage::not empty = %d\n", imagesAlloc.NotEmpty());
    if(imagesAlloc.NotEmpty())
    {
        HARM_MT_IMAGES_DEBUG("GetNextAllocImage::before remove num = %d\n", imagesAlloc.Num());
        const ActuallyLoadImage_data_t &ref = imagesAlloc.Get();
        img.image = ref.image;
        img.checkForPrecompressed = ref.checkForPrecompressed;
        img.fromBackEnd = ref.fromBackEnd;
        imagesAlloc.Remove();
        HARM_MT_IMAGES_DEBUG("GetNextAllocImage::after remove num = %d\n", imagesAlloc.Num());
        return true;
    }

    return false;
}

idImage * idRenderThread::GetNextPurgeImage( void )
{
    idImage * img = NULL;

    HARM_MT_IMAGES_DEBUG("GetNextPurgeImage::not empty = %d\n", imagesPurge.NotEmpty());
    if(imagesPurge.NotEmpty())
    {
        HARM_MT_IMAGES_DEBUG("GetNextPurgeImage::before remove num = %d\n", imagesPurge.Num());
        img = imagesPurge.Get();
        imagesPurge.Remove();
        img->purgePending = false;
        HARM_MT_IMAGES_DEBUG("GetNextPurgeImage::after remove num = %d\n", imagesPurge.Num());
    }

    return img;
}

void idRenderThread::HandlePendingImage( void )
{
    // Purge all images
    idImage * img;
    while( (img = GetNextPurgeImage()) != NULL )
    {
        img->PurgeImage();
    }

    // Load all images
    ActuallyLoadImage_data_t imgData;
    while(GetNextAllocImage(imgData))
    {
        imgData.image->ActuallyLoadImage( imgData.checkForPrecompressed, false ); // false
    }
}

void idRenderThread::ClearImages( void )
{
#if 0
    imagesAlloc.Clear();
    imagesPurge.Clear();
#else
    HARM_MT_IMAGES_DEBUG("ClearImages::alloc list not empty = %d\n", imagesAlloc.NotEmpty());
    while(imagesAlloc.NotEmpty())
    {
        HARM_MT_IMAGES_DEBUG("ClearImages::alloc list before remove num = %d\n", imagesAlloc.Num());
        imagesAlloc.Remove();
        HARM_MT_IMAGES_DEBUG("ClearImages::alloc list after remove num = %d\n", imagesAlloc.Num());
    }

    HARM_MT_IMAGES_DEBUG("ClearImages::purge list not empty = %d\n", imagesPurge.NotEmpty());
    while(imagesPurge.NotEmpty())
    {
        HARM_MT_IMAGES_DEBUG("ClearImages::purge list before remove num = %d\n", imagesPurge.Num());
        imagesPurge.Remove();
        HARM_MT_IMAGES_DEBUG("ClearImages::purge list after remove num = %d\n", imagesPurge.Num());
    }
#endif
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