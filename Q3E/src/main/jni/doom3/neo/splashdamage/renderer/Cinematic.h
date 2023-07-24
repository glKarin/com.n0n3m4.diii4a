// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __CINEMATIC_H__
#define __CINEMATIC_H__

class sdDeclRenderBinding;

// a cinematic stream generates an image buffer, which the caller will upload to a texture
struct cinData_t {
	int					imageWidth, imageHeight;	// will be a power of 2
	const byte*			image[3];					// [0] when RGBA format, alpha will be 255, all when YUV format, no alpha
};

/*
===============================================

	Sound meter.

===============================================
*/
#if 0
class idSndWindow : public idCinematic {
public:
	
								idSndWindow() { showWaveform = false; }
								~idSndWindow() {}

	bool						InitFromFile( const char *qpath, bool looping );
	cinData_t					ImageForTime( int milliseconds );
	int							AnimationLength();
	const sdDeclRenderBinding*	CinematicRenderBinding() const;

private:
	bool						showWaveform;
};
#endif
#endif /* !__CINEMATIC_H__ */
