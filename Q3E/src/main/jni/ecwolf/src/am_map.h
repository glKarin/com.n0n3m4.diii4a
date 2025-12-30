/*
** am_map.h
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#ifndef __AM_MAP_H__
#define __AM_MAP_H__

#include "gamemap.h"
#include "tarray.h"
#include "vectors.h"

enum
{
	AMA_Off,
	AMA_Overlay,
	AMA_Normal
};

extern unsigned automap;
extern bool am_cheat;
extern unsigned am_rotate;
extern bool am_overlaytextured;
extern bool am_drawtexturedwalls;
extern bool am_drawfloors;
extern unsigned am_overlay;
extern bool am_pause;
extern bool am_showratios;

void AM_ChangeResolution();
void AM_CheckKeys();
void AM_UpdateFlags();
void AM_Toggle();

void BasicOverhead();

struct AMVectorPoint;

class AutoMap
{
public:
	enum AMFlags
	{
		AMF_Rotate = 0x1,
		AMF_DrawTexturedWalls = 0x2,
		AMF_DrawFloor = 0x4,
		AMF_Overlay = 0x8,
		AMF_DispInfo = 0x10,
		AMF_DispRatios = 0x20,
		AMF_ShowThings = 0x40
	};

	struct Color
	{
		uint32 color;
		byte palcolor;

		Color &operator=(int rgb);
	};

	AutoMap(unsigned int flags=0);
	~AutoMap();

	void CalculateDimensions(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
	void Draw();
	fixed GetScale() const { return absscale; }
	fixed GetScreenScale() const;
	void SetFlags(unsigned int flags, bool set);
	void SetPanning(fixed x, fixed y, bool relative);
	void SetScale(fixed scale, bool relative);

protected:
	void ClipTile(TArray<FVector2> &points) const;
	void DrawActor(class AActor *actor, fixed x, fixed y, fixed scale);
	void DrawClippedLine(int x0, int y0, int x1, int y1, int palcolor, uint32 realcolor) const;
	void DrawStats() const;
	void DrawVector(const AMVectorPoint *points, unsigned int numPoints, fixed x, fixed y, fixed scale, angle_t angle, const Color &c) const;
	FVector2 GetClipIntersection(const FVector2 &p1, const FVector2 &p2, unsigned edge) const;
	bool TransformTile(MapSpot spot, fixed x, fixed y, TArray<FVector2> &points) const;

private:
	double rottable[2][2];

	bool fullRefresh;
	unsigned int amFlags;
	int amsizex, amsizey, amx, amy;
	fixed ampanx, ampany;
	fixed amsin, amcos;
	fixed absscale;
	angle_t amangle;
	unsigned short minmaxSel;

	Color ArrowColor;
	Color BackgroundColor;
	Color FloorColor;
	Color WallColor;
	Color DoorColor;
};

extern AutoMap AM_Main;

#endif
