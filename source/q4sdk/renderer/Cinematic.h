// Copyright (C) 2004 Id Software, Inc.
//
#ifndef __CINEMATIC_H__
#define __CINEMATIC_H__

// RAVEN BEGIN
//nrausch: I made some semi-heavy changes to this entire file
//	- changed to idCinematic to use a private implementation which 
//	is determined & allocated when InitFromFile is called. A different
//	PIMPL is used depending on if the video file is a roq or a wmv.
//	This replaces the functionality that was in a few versions ago under the
//	"StandaloneCinematic" name.
// RAVEN END

/*
===============================================================================

	RoQ cinematic

	Multiple idCinematics can run simultaniously.
	A single idCinematic can be reused for multiple files if desired.

===============================================================================
*/

// cinematic states
typedef enum {
	FMV_IDLE,
	FMV_PLAY,			// play
	FMV_EOF,			// all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} cinStatus_t;

// a cinematic stream generates an image buffer, which the caller will upload to a texture
typedef struct {
	int					imageWidth, imageHeight;	// will be a power of 2
	const byte *		image;						// RGBA format, alpha will be 255
	int					status;
} cinData_t;

class idCinematic {
	idCinematic* PIMPL;
	
	// Store off the current mode - wmv or roq
	// If the cinematic is in the same mode if InitFromFile
	// is called again on it, this will prevent reallocation 
	// of the PIMPL
	int mode;
public:
	// initialize cinematic play back data
	static void			InitCinematic( void );

	// shutdown cinematic play back data
	static void			ShutdownCinematic( void );

	// allocates and returns a private subclass that implements the methods
	// This should be used instead of new
	static idCinematic	*Alloc();

						idCinematic();
	
	// frees all allocated memory
	virtual				~idCinematic();

	enum {
		SUPPORT_DRAW = 1,
		SUPPORT_IMAGEFORTIME = 2,
		SUPPORT_DEFAULT = SUPPORT_IMAGEFORTIME
	};
	
	// returns false if it failed to load
	// this interface can take either a wmv or roq file
	// wmv will imply movie audio, unless there is no audio encoded in the stream
	// right now there is no way to disable this.
	virtual bool		InitFromFile(const char *qpath, bool looping, int options = SUPPORT_DEFAULT);

	// returns the length of the animation in milliseconds
	virtual int			AnimationLength();

	// the pointers in cinData_t will remain valid until the next UpdateForTime() call
	// will do nothing if InitFromFile was not called with SUPPORT_IMAGEFORTIME
	virtual cinData_t	ImageForTime(int milliseconds);

	// closes the file and frees all allocated memory
	virtual void		Close();

	// closes the file and frees all allocated memory
	virtual void		ResetTime(int time);

	// draw the current animation frame to screen
	// will do nothing if InitFromFile was not called with SUPPORT_DRAW
	virtual void		Draw();
	
	// Set draw position & size
	// will do nothing if InitFromFile was not called with SUPPORT_DRAW
	virtual void		SetScreenRect(int left, int right, int bottom, int top);
	
	// Get draw position & size
	// will do nothing if InitFromFile was not called with SUPPORT_DRAW
	virtual void		GetScreenRect(int &left, int &right, int &bottom, int &top);
	
	// True if the video is playing
	// will do nothing if InitFromFile was not called with SUPPORT_DRAW
	virtual bool		IsPlaying();
};

/*
===============================================

	Sound meter.

===============================================
*/

class idSndWindow : public idCinematic {
public:
	
						idSndWindow() { showWaveform = false; }
						~idSndWindow() {}

	bool				InitFromFile( const char *qpath, bool looping, int options );
	cinData_t			ImageForTime( int milliseconds );
	int					AnimationLength();

private:
	bool				showWaveform;
};

#endif /* !__CINEMATIC_H__ */
