
#define FENCE_SYNC_TIMEOUT (1000 * 1000)

idCVar harm_r_useFenceSync("harm_r_useFenceSync", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "use glClientWaitSync instead of glFinish(OpenGL ES3.0+)");
static idCVar r_syncEveryFrame( "r_syncEveryFrame", "1", CVAR_BOOL, "Don't let the GPU buffer execution past swapbuffers" );
#ifdef DEBUG_SYNC_MIN_INTERVAL
idCVar r_showSwapBuffers( "r_showSwapBuffers", "0", CVAR_RENDERER | CVAR_BOOL, "Show timings from GL_BlockingSwapBuffers" );
#endif

static int		swapIndex;		// 0 or 1 into renderSync
static GLsync	renderSync[2];

#if 1
#define GL_IsSync(x) ( x && qglIsSync(x) )
#define GL_DeleteSync(x) { qglDeleteSync(x); x = 0; }
#else
#define GL_IsSync(x) qglIsSync(x)
#define GL_DeleteSync(x) qglDeleteSync(x)
#endif

void RB_FenceSync(void)
{
/*	if(!glConfig.syncAvailable)
	{
		return;
	}
	if(!harm_r_useFenceSync.GetBool())
		return;
		*/

#ifdef DEBUG_SYNC_MIN_INTERVAL
    const int beforeFence = Sys_Milliseconds();
#endif

    swapIndex ^= 1;

    if( GL_IsSync( renderSync[swapIndex] ) )
    {
        GL_DeleteSync( renderSync[swapIndex] );
    }

#ifdef DEBUG_SYNC_MIN_INTERVAL
    // draw something tiny to ensure the sync is after the swap
    const int start = Sys_Milliseconds();
#endif
    renderSync[swapIndex] = qglFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
#ifdef DEBUG_SYNC_MIN_INTERVAL
    const int end = Sys_Milliseconds();
    if( r_showSwapBuffers.GetBool() && end - start > 1 )
    {
        common->Printf( "%i msec to start fence\n", end - start );
    }
#endif

    GLsync	syncToWaitOn;
    if( r_syncEveryFrame.GetBool() )
    {
        syncToWaitOn = renderSync[swapIndex];
    }
    else
    {
        syncToWaitOn = renderSync[!swapIndex];
    }

    if( GL_IsSync( syncToWaitOn ) )
    {
        for( GLenum r = GL_TIMEOUT_EXPIRED; r == GL_TIMEOUT_EXPIRED; )
        {
            r = qglClientWaitSync( syncToWaitOn, GL_SYNC_FLUSH_COMMANDS_BIT, FENCE_SYNC_TIMEOUT );
        }
    }

#ifdef DEBUG_SYNC_MIN_INTERVAL
    const int afterFence = Sys_Milliseconds();
    if( r_showSwapBuffers.GetBool() && afterFence - beforeFence > 0 )
    {
        common->Printf( "%i msec to wait on fence\n", afterFence - beforeFence );
    }
#endif
}

