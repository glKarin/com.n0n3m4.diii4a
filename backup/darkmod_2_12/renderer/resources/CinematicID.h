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

#ifndef __CINEMATIC_ID_H__
#define __CINEMATIC_ID_H__

#include <jpeglib.h>

#include "renderer/tr_local.h"
#include "renderer/resources/Cinematic.h"

#define CIN_system	1
#define CIN_loop	2
#define	CIN_hold	4
#define CIN_silent	8
#define CIN_shader	16

class idCinematicLocal : public idCinematic {
public:
							idCinematicLocal();
	virtual					~idCinematicLocal() override;

	static void InitCinematic();
	static void ShutdownCinematic();

	virtual bool			InitFromFile( const char *qpath, bool looping, bool withAudio ) override;
	virtual cinData_t		ImageForTime( int milliseconds ) override;
	virtual int				AnimationLength() override;
	virtual void			Close() override;
	virtual void			ResetTime(int time) override;

private:
	size_t					mcomp[256];
	byte **					qStatus[2];
	idStr					fileName;
	int						CIN_WIDTH, CIN_HEIGHT;
	idFile *				iFile;
	cinStatus_t				status;
	int						tfps;
	int						RoQPlayed;
	int						ROQSize;
	unsigned int			RoQFrameSize;
	int						onQuad;
	int						numQuads;
	int						samplesPerLine;
	unsigned int			roq_id;
	int						screenDelta;
	byte *					buf;
	int						samplesPerPixel;				// defaults to 2
	unsigned int			xsize, ysize, maxsize, minsize;
	int						normalBuffer0;
	int						roq_flags;
	int						roqF0;
	int						roqF1;
	int						t[2];
	int						roqFPS;
	int						drawX, drawY;

	int						animationLength;
	int						startTime;
	float					frameRate;

	byte *					image;

	bool					looping;
	bool					dirty;
	bool					half;
	bool					smootheddouble;
	bool					inMemory;

	void					RoQ_init( void );
	void					blitVQQuad32fs( byte **status, unsigned char *data );
	void					RoQShutdown( void );
	void					RoQInterrupt(void);

	void					move8_32( byte *src, byte *dst, int spl );
	void					move4_32( byte *src, byte *dst, int spl );
	void					blit8_32( byte *src, byte *dst, int spl );
	void					blit4_32( byte *src, byte *dst, int spl );
	void					blit2_32( byte *src, byte *dst, int spl );

	unsigned short			yuv_to_rgb( int y, int u, int v );
	unsigned int			yuv_to_rgb24( int y, int u, int v );

	void					decodeCodeBook( byte *input, unsigned short roq_flags );
	void					recurseQuad( int startX, int startY, int quadSize, int xOff, int yOff );
	void					setupQuad( int xOff, int yOff );
	void					readQuadInfo( byte *qData );
	void					RoQPrepMcomp( int xoff, int yoff );
	void					RoQReset();
};

#endif
