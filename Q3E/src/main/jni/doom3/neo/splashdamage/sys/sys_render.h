// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SYS_RENDER__ )
#define __SYS_RENDER__

/*
==============================================================

	Render

==============================================================
*/

#if defined ( _WIN32 )
	typedef HDC		dcHandle_t;
	typedef HWND	wndHandle_t;
#else
	typedef void*	dcHandle_t;
	typedef void*	wndHandle_t;
#endif

struct multiSampleParms {
	int multi;
	int coverage;

	explicit multiSampleParms() : multi(0), coverage(0) {
	}

	explicit multiSampleParms( int ms ) : multi(ms), coverage(0) {
	}

	explicit multiSampleParms( int ms, int cv ) : multi(ms), coverage(cv) {
	}

	bool operator==( multiSampleParms const &other ) const {
		return multi == other.multi && coverage == other.coverage;
	}

	bool operator!=( multiSampleParms const &other ) const {
		return !(*this == other);
	}

};



struct glimpParms_t	{
	int			width;
	int			height;
	bool		fullScreen;
	bool		stereo;
	int			displayHz;
	multiSampleParms			multiSamples;
	float		pixelAspect;		// pixel width / height, should be 1.0 in most cases
	bool		fullscreenAvail;

	bool operator ==( const glimpParms_t& rhs ) const {
		return (
			width == rhs.width &&
			height == rhs.height &&
			fullScreen == rhs.fullScreen &&
			displayHz == rhs.displayHz &&
			multiSamples == rhs.multiSamples &&
			pixelAspect == rhs.pixelAspect &&
			fullscreenAvail == rhs.fullscreenAvail
		);
	}
};

class idRenderContextParms {
public:
	int			width;
	int			height;
	int			redBits;
	int			greenBits;
	int			blueBits;
	int			alphaBits;
	int			depthBits;
	int			stencilBits;
	multiSampleParms	multiSamples;
	bool		forceUnique;
	bool		offscreen;
	bool		doubleBuffer;
	bool		floatingPoint;
	bool		bindToTexture;
	bool		generateMipMaps;

	dcHandle_t	windowDC;			// if offscreen is true, this is ignored, otherwise it must be set to an existing window's device context.
	bool		isGameContext;		// true if this is the main game rendering context

public:
	idRenderContextParms( void )
	:	width( 512 ),
		height( 512 ),
		redBits( 8 ),
		greenBits( 8 ),
		blueBits( 8 ),
		alphaBits( 8 ),
		depthBits( 24 ),
		stencilBits( 8 ),
		multiSamples( 1 ),
		forceUnique( false ),
		offscreen( true ),
		doubleBuffer( false ),
		floatingPoint( false ),
		bindToTexture( false ),
		generateMipMaps( false ),
		windowDC( NULL ),
		isGameContext( false ) {
	}

	idRenderContextParms( int width, int height,
						  int redBits, int greenBits, int blueBits,
						  int alphaBits, int depthBits, int stencilBits,
						  int multiSamples, bool forceUnique,
						  bool offscreen, bool doubleBuffer, 
						  bool floatingPoint, bool bindToTexture, 
						  bool generateMipMaps, bool isGameContext ) 
	:	width( width ),
		height( height ),
		redBits( redBits ),
		greenBits( greenBits ),
		blueBits( blueBits ),
		alphaBits( alphaBits ),
		depthBits( depthBits ),
		stencilBits( stencilBits ),
		multiSamples( multiSamples ),
		forceUnique( forceUnique ),
		offscreen( offscreen ),
		doubleBuffer( doubleBuffer ),
		floatingPoint( floatingPoint ),
		bindToTexture( bindToTexture ),
		generateMipMaps( generateMipMaps ),
		windowDC( NULL ),
		isGameContext( isGameContext ) {
	}

	idRenderContextParms( int width, int height,
						  int redBits, int greenBits, int blueBits,
						  int alphaBits, int depthBits, int stencilBits,
						  multiSampleParms multiSamples, bool forceUnique,
						  bool offscreen, bool doubleBuffer, 
						  bool floatingPoint, bool bindToTexture, 
						  bool generateMipMaps, bool isGameContext ) 
	:	width( width ),
		height( height ),
		redBits( redBits ),
		greenBits( greenBits ),
		blueBits( blueBits ),
		alphaBits( alphaBits ),
		depthBits( depthBits ),
		stencilBits( stencilBits ),
		multiSamples( multiSamples ),
		forceUnique( forceUnique ),
		offscreen( offscreen ),
		doubleBuffer( doubleBuffer ),
		floatingPoint( floatingPoint ),
		bindToTexture( bindToTexture ),
		generateMipMaps( generateMipMaps ),
		windowDC( NULL ),
		isGameContext( isGameContext ) {
	}
};

struct rect_t {
	int x;
	int y;
	int w;
	int h;
};
struct monitorInfo_t {
	rect_t monitor;
	rect_t workArea;
	bool primary;
};

//===============================================================
//
//	idRenderContext
//
//===============================================================

class idRenderContext {
public:
								idRenderContext( void ) {}
	virtual						~idRenderContext( void ) {}

								// create a suitable rendering context.
	virtual	bool				Create( const idRenderContextParms &parms ) = 0;

								// destroy this context.
	virtual	void				Destroy( void ) = 0;

								// make this rendering context the current context for this thread.
								// if dc != NULL, the current device context for this rendering context will be overridden
	virtual	bool				MakeCurrent( dcHandle_t handle = NULL ) = 0;

								// returns true if the context is valid.
	virtual	bool				IsValid( void ) const = 0;

								// sets any default states specific to a render context type.
	virtual	void				SetAdditionalDefaultState( void ) = 0;

	virtual	void				ShowContext( void ) const = 0;

								// returns the parms used to create the rendering context.
	const idRenderContextParms&	GetParms( void ) const { return parms; }

								// true if this render context is associated with a pbuffer.
	bool						IsOffscreen( void ) const { return parms.offscreen; }

protected:
	idRenderContextParms		parms;			// parameters used to create this context
};

//===============================================================
//
//	id3DContext
//
//===============================================================

typedef void (*GLExtension_t)( void );

#if defined( _WIN32 )
#undef IsMinimized
#endif // _WIN32

class id3DContext {
public:
	virtual							~id3DContext( ) { }

	// initializing the context will not automatically display a window
	virtual void					InitContext( const glimpParms_t& parms ) = 0;
	virtual void					Shutdown( void ) = 0;

	// allow for recreating a context at runtime
	virtual void					RecreateContext( const glimpParms_t& parms ) = 0;

	// OS specific, for switching back to the game window after
	// using a tool window
	virtual dcHandle_t				GetGameWindow( void ) = 0;

	// OS specific, for using as parent window, focus checks, etc
	virtual wndHandle_t				GetGameWindowHandle( void ) = 0;

	// returns the render context used for game rendering.
	virtual	idRenderContext*		GetGameRenderContext( void ) = 0;

	virtual bool					SetGameWindowParms( const glimpParms_t& parms ) = 0;

	// returns the current render context (which may be a tool context or the game context).
	virtual	idRenderContext*		GetCurrentRenderContext( void ) = 0;

	// these could change each frame if the window is being
	// drag-resized
	virtual const glimpParms_t&		GetGameWindowParms() const = 0;

	// any tool windows that are going to be rendered into
	// must set this pixelformat during their creation
	virtual void					SetPixelFormat( dcHandle_t windowDC ) = 0;

	virtual GLExtension_t			ExtensionPointer( const char *name ) = 0;

	virtual void					ShowGameWindow( void ) = 0;
	virtual void					HideGameWindow( void ) = 0;

	// convenience function for getting just the fullscreen flag
	virtual bool					IsFullscreen( void ) = 0;

	virtual bool					IsMinimized( void ) = 0;

	// adjust the game window glimpParms_t as a result of window size dragging
	virtual void					WindowSizeDragged( int width, int height ) = 0;

	// associates the rendering context with the device context.
	// NULL can be passed to disable the context for performance testing
	virtual bool					MakeCurrent( dcHandle_t windowDC ) = 0;

	virtual bool					ReleaseCurrent( dcHandle_t windowDC ) = 0;

	// disassociates the rendering context from the specified device context if 
	// the rendering context is currently associated with it.  Windows that call
	// MakeCurrent should call ReleaseContext before they are destroyed.
	virtual void					ReleaseContext( dcHandle_t windowDC ) = 0;

	// swapbuffers always swaps the window last passed to MakeCurrent
	virtual void					SwapBuffers( void ) = 0;

	// Sets the hardware gamma ramps for gamma and brightness adjustment.
	// These are now taken as 16 bit values, so we can take full advantage
	// of dacs with >8 bits of precision
	virtual void					SetGamma( unsigned short red[256], unsigned short green[256], unsigned short blue[256] ) = 0;

	virtual bool					IsDisplayModeAvailable( int width, int height ) = 0;
	virtual int						GetNumMSAAModes( void ) const = 0;
	virtual const char *			GetMSAAMode( int idx, int &val ) const = 0;
	virtual bool					IsMSAACountAvailable( int msaa ) const = 0;
	// returns the number of available monitors 
	virtual int						GetNumMonitors() const = 0;

	// returns the number of available monitors 
	virtual const monitorInfo_t&	GetMonitor( int index ) const = 0;

	// returns the system's primary monitor
	virtual const monitorInfo_t&	GetPrimaryMonitor() const = 0;

	// fit the dimensions to the primary monitor, maintaining the original aspect
	// ratio between width and height
	virtual void					ConstrainToPrimaryMonitor( int& width, int& height ) = 0;

	// find all monitors on the system
	virtual void					EnumerateMonitors() = 0;
};

extern id3DContext* sys3D;

#endif /* !__SYS_RENDER__ */
