/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __CINEMATIC_H__
#define __CINEMATIC_H__

/*
===============================================================================

	RoQ cinematic

	Multiple idCinematics can run simultaniously.
	A single idCinematic can be reused for multiple files if desired.

===============================================================================
*/

extern idCVar r_cinematic_legacyRoq;

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
	static idCinematic	*Alloc( const char *qpath );

	// frees all allocated memory
	virtual				~idCinematic();

	// returns false if it failed to load
	virtual bool		InitFromFile( const char *qpath, bool looping, bool withAudio = false );

	// returns path to video file played by this
	virtual const char *GetFilePath() const;

	// returns the length of the animation in milliseconds
	virtual int			AnimationLength();

	// the pointers in cinData_t will remain valid until the next UpdateForTime() call
	virtual cinData_t	ImageForTime( int milliseconds );

	//stgatilov #4534: allows to get sound from video file
	//sampleOffset and sampleSize specify beginning and length of interval (in samples)
	//each stereo sample consists of left speaker value and right speaker value (speakers are interleaved)
	//output buffer must be large enough to hold 2 * sampleSize float values
	//number of samples read is returned in sampleSize
	//note: this method can be called from multiple threads (e.g. sound thread) !
	virtual bool SoundForTimeInterval(int sampleOffset44k, int *sampleSize, float *output);
	//stgatilov #2454: returns actual sample offset synced to video
	virtual int GetRealSoundOffset(int sampleOffset44k) const;

	//stgatilov #4535: allows anyone to check at any moment if the video has ended
	//note that ImageForTime is called from backend, so it is hard to save result status from there
	virtual cinStatus_t GetStatus() const;


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

	virtual bool		InitFromFile( const char *qpath, bool looping, bool withAudio ) override;
	virtual cinData_t	ImageForTime( int milliseconds ) override;
	virtual int			AnimationLength() override;

private:
	bool				showWaveform;
};

#endif /* !__CINEMATIC_H__ */
