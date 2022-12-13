// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __CINEMATIC_H__
#define __CINEMATIC_H__

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
public:
	// initialize cinematic play back data
	static void			InitCinematic( void );

	// shutdown cinematic play back data
	static void			ShutdownCinematic( void );

	// allocates and returns a private subclass that implements the methods
	// This should be used instead of new
	static idCinematic	*Alloc();

	// frees all allocated memory
	virtual				~idCinematic();

	// returns false if it failed to load
	virtual bool		InitFromFile( const char *qpath, bool looping );

	// returns the length of the animation in milliseconds
	virtual int			AnimationLength();

	// the pointers in cinData_t will remain valid until the next UpdateForTime() call
	virtual cinData_t	ImageForTime( int milliseconds );

	// closes the file and frees all allocated memory
	virtual void		Close();

	// closes the file and frees all allocated memory
	virtual void		ResetTime(int time);
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

	bool				InitFromFile( const char *qpath, bool looping );
	cinData_t			ImageForTime( int milliseconds );
	int					AnimationLength();

private:
	bool				showWaveform;
};

// HUMANHEAD pdm: profiler
class hhProfilerGraph : public idCinematic {
public:
	
						hhProfilerGraph() {}
						~hhProfilerGraph() {}

	bool				InitFromFile( const char *qpath, bool looping );
	cinData_t			ImageForTime( int milliseconds );
	int					AnimationLength();

private:
};
// HUMANHEAD END

#endif /* !__CINEMATIC_H__ */
