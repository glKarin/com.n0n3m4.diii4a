#ifndef _RENDERTHREAD_H
#define _RENDERTHREAD_H

#define NUM_FRAME_DATA 2

#define RENDER_THREAD_NAME "render_thread"

extern bool multithreadActive;

class idRenderThread {
public:
    idRenderThread();
    volatile bool           backendThreadShutdown;
    xthreadInfo				render_thread;
    volatile bool           render_thread_finished;
    volatile bool           backendFinished;

    volatile frameData_t	*fdToRender;
    volatile int			vertListToRender;
// These are set if the backend should save pixels
    volatile renderCrop_t	*pixelsCrop;
    volatile byte           *pixels;

    void                    BackendThreadExecute(void);
    void                    BackendThreadShutdown(void);
    void                    BackendThreadWait(void);
    void                    BackendThreadTask(void);

private:
    idRenderThread(const idRenderThread &);
    idRenderThread &        operator=(const idRenderThread &);
    void *                  operator new(size_t);
    void *                  operator new[](size_t);
    void                    operator delete(void *);
    void                    operator delete[](void *);
};

extern idRenderThread *renderThread;

// for other module exclude `renderer`
bool Sys_InRenderThread(void);
void Sys_ShutdownRenderThread(void);

#endif // _RENDERTHREAD_H
