#ifndef _RENDERTHREAD_H
#define _RENDERTHREAD_H

#include "QueueList.h"

#define NUM_FRAME_DATA 2

#define RENDER_THREAD_NAME "render_thread"

extern bool multithreadActive;
extern bool multithreadEnable;

typedef struct ActuallyLoadImage_data_s
{
    idImage *image;
    bool checkForPrecompressed;
    bool fromBackEnd;
    ActuallyLoadImage_data_s( idImage *image, bool checkForPrecompressed, bool fromBackEnd )
            : image( image ),
              checkForPrecompressed( checkForPrecompressed ),
              fromBackEnd( fromBackEnd )
    {}
    ActuallyLoadImage_data_s()
            : image( NULL ),
              checkForPrecompressed( false ),
              fromBackEnd( false )
    {}
} ActuallyLoadImage_data_t;

class idRenderThread
{
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

    void                    BackendThreadExecute( void );
    void                    BackendThreadShutdown( void );
    void                    BackendThreadWait(void );
    void                    BackendThreadTask( void );
    void                    BackendThreadSingleTask( void );
    void                    BackendThreadToolsTask( void );
    bool                    IsActive( void ) const;

// images queue
    void				    AddAllocList( idImage * image, bool checkForPrecompressed, bool fromBackEnd );
    void				    AddPurgeList( idImage * image );

    bool				    GetNextAllocImage( ActuallyLoadImage_data_t &ret );
    idImage *			    GetNextPurgeImage();
    void                    HandlePendingImage( void );
    void                    ClearImages( void );
    void                    Request( bool on );
    void                    SyncState( void );

private:
    idQueueList<ActuallyLoadImage_data_t>	imagesAlloc; //List for the backend thread
    idQueueList<idImage *>	imagesPurge; //List for the backend thread
	int						requestState;

	enum {
		REQ_NONE,
		REQ_ENABLE,
		REQ_DISABLE,
	};

private:
    idRenderThread(const idRenderThread &);
    idRenderThread &        operator=(const idRenderThread &);
    void *                  operator new(size_t);
    void *                  operator new[](size_t);
    void                    operator delete(void *);
    void                    operator delete[](void *);
};
	void				R_EnableRenderThread_f(const idCmdArgs &args);

extern idRenderThread *renderThread;

// for other module exclude `renderer`
bool Sys_InRenderThread(void);
void Sys_ShutdownRenderThread(void);

#endif // _RENDERTHREAD_H
