/*
** am_map.cpp
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

#include "wl_def.h"
#include "am_map.h"
#include "colormatcher.h"
#include "id_ca.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "g_mapinfo.h"
#include "gamemap.h"
#include "r_data/colormaps.h"
#include "r_sprites.h"
#include "v_font.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_main.h"
#include "wl_play.h"

AutoMap::Color &AutoMap::Color::operator=(int rgb)
{
	color = rgb;
	palcolor = ColorMatcher.Pick(RPART(rgb), GPART(rgb), BPART(rgb));
	return *this;
}

AutoMap AM_Main;
AutoMap AM_Overlay;

enum
{
	AMO_Off,
	AMO_On,
	AMO_Both
};
enum
{
	AMR_Off,
	AMR_On,
	AMR_Overlay
};

unsigned automap = 0;
bool am_cheat = false;
unsigned am_rotate = 0;
bool am_overlaytextured = false;
bool am_drawtexturedwalls = true;
bool am_drawfloors = false;
unsigned am_overlay = 0;
bool am_pause = true;
bool am_showratios = false;
bool am_needsrecalc = false;

void AM_ChangeResolution()
{
	if(StatusBar == NULL)
	{
		am_needsrecalc = true;
		return;
	}

	unsigned int y = statusbary1;
	unsigned int height = statusbary2 - statusbary1;

	AM_Main.CalculateDimensions(0, y, screenWidth, height);
	AM_Overlay.CalculateDimensions(viewscreenx, viewscreeny, viewwidth, viewheight);
}

void AM_CheckKeys()
{
	if(control[ConsolePlayer].ambuttonstate[bt_zoomin])
	{
		AM_Overlay.SetScale(FRACUNIT*135/128, true);
		AM_Main.SetScale(FRACUNIT*135/128, true);
	}
	if(control[ConsolePlayer].ambuttonstate[bt_zoomout])
	{
		AM_Overlay.SetScale(FRACUNIT*122/128, true);
		AM_Main.SetScale(FRACUNIT*122/128, true);
	}

	if(am_pause)
	{
		const fixed PAN_AMOUNT = FixedDiv(FRACUNIT*10, AM_Main.GetScreenScale());
		const fixed PAN_ANALOG_MULTIPLIER = PAN_AMOUNT/100;
		fixed panx = 0, pany = 0;

		if(control[ConsolePlayer].ambuttonstate[bt_panleft])
			panx += PAN_AMOUNT;
		if(control[ConsolePlayer].ambuttonstate[bt_panright])
			panx -= PAN_AMOUNT;
		if(control[ConsolePlayer].ambuttonstate[bt_panup])
			pany += PAN_AMOUNT;
		if(control[ConsolePlayer].ambuttonstate[bt_pandown])
			pany -= PAN_AMOUNT;

		if(control[ConsolePlayer].controlpanx != 0)
			panx += control[ConsolePlayer].controlpanx * (PAN_ANALOG_MULTIPLIER * (panxadjustment+1));

		if(control[ConsolePlayer].controlpany != 0)
			pany += control[ConsolePlayer].controlpany * (PAN_ANALOG_MULTIPLIER * (panxadjustment+1));

		AM_Main.SetPanning(panx, pany, true);
	}
}

void AM_UpdateFlags()
{
	// Release pause if we appear to have unset am_pause
	if(!am_pause && (Paused&2)) Paused &= ~2;

	unsigned int flags = 0;
	unsigned int ovFlags = AutoMap::AMF_Overlay;

	if(am_rotate == AMR_On) flags |= AutoMap::AMF_Rotate;

	// Overlay only flags
	ovFlags |= flags;
	if(am_rotate == AMR_Overlay) ovFlags |= AutoMap::AMF_Rotate;
	if(am_overlaytextured) ovFlags |= AutoMap::AMF_DrawTexturedWalls;

	// Full only flags
	if(am_showratios) flags |= AutoMap::AMF_DispRatios;
	if(am_drawfloors) flags |= AutoMap::AMF_DrawFloor;
	if(am_drawtexturedwalls) flags |= AutoMap::AMF_DrawTexturedWalls;

	AM_Main.SetFlags(~flags, false);
	AM_Overlay.SetFlags(~ovFlags, false);
	AM_Main.SetFlags(flags|AutoMap::AMF_DispInfo|AutoMap::AMF_ShowThings, true);
	AM_Overlay.SetFlags(ovFlags, true);
}

void AM_Toggle()
{
	++automap;
	if(automap == AMA_Overlay && am_overlay == AMO_Off)
		++automap;
	else if(automap > AMA_Normal || (automap == AMA_Normal && am_overlay == AMO_On))
	{
		automap = AMA_Off;
		AM_Main.SetPanning(0, 0, false);
		DrawPlayScreen();
	}

	if(am_pause)
	{
		if(automap == AMA_Normal)
			Paused |= 2;
		else
			Paused &= ~2;
	}
}

// Culling table
static const struct
{
	const struct
	{
		const unsigned short Min, Max;
	} X, Y;
} AM_MinMax[4] = {
	{ {3, 1}, {0, 2} },
	{ {2, 0}, {3, 1} },
	{ {1, 3}, {2, 0} },
	{ {0, 2}, {1, 3} }
};

// #FF9200
struct AMVectorPoint
{
	fixed X, Y;
};
static const AMVectorPoint AM_Arrow[] =
{
	{0, -FRACUNIT/2},
	{FRACUNIT/2, 0},
	{FRACUNIT/4, 0},
	{FRACUNIT/4, FRACUNIT*7/16},
	{-FRACUNIT/4, FRACUNIT*7/16},
	{-FRACUNIT/4, 0},
	{-FRACUNIT/2, 0},
	{0, -FRACUNIT/2},
};

AutoMap::AutoMap(unsigned int flags) :
	fullRefresh(true), amFlags(flags),
	ampanx(0), ampany(0)
{
	amangle = 0;
	minmaxSel = 0;
	amsin = 0;
	amcos = FRACUNIT;
	absscale = FRACUNIT/4;
	rottable[0][0] = 1.0;
	rottable[0][1] = 0.0;
	rottable[1][0] = 1.0;
	rottable[1][1] = 1.0;
}

AutoMap::~AutoMap()
{
}

void AutoMap::CalculateDimensions(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	fullRefresh = true;
	amsizex = width;
	amsizey = height;
	amx = x;
	amy = y;

	// Since the simple polygon fill function seems to be off by one in the y
	// direction, lets shift this up!
	--amy;

	// TODO: Find a better spot for this
	ArrowColor = gameinfo.automap.YourColor;
	BackgroundColor = gameinfo.automap.Background;
	FloorColor = gameinfo.automap.FloorColor;
	WallColor = gameinfo.automap.WallColor;
	DoorColor = gameinfo.automap.DoorColor;
}

// Sutherland–Hodgman algorithm
void AutoMap::ClipTile(TArray<FVector2> &points) const
{
	for(int i = 0;i < 4;++i)
	{
		TArray<FVector2> input(points);
		points.Clear();
		FVector2 *s = &input[0];
		for(unsigned j = input.Size();j-- > 0;)
		{
			FVector2 &e = input[j];

			bool Ein, Sin;
			switch(i)
			{
				case 0: // Left
					Ein = e.X > amx;
					Sin = s->X > amx;
					break;
				case 1: // Top
					Ein = e.Y > amy;
					Sin = s->Y > amy;
					break;
				case 2: // Right
					Ein = e.X < amx+amsizex;
					Sin = s->X < amx+amsizex;
					break;
				case 3: // Bottom
					Ein = e.Y < amy+amsizey;
					Sin = s->Y < amy+amsizey;
					break;
			}
			if(Ein)
			{
				if(!Sin)
					points.Push(GetClipIntersection(e, *s, i));
				points.Push(e);
			}
			else if(Sin)
				points.Push(GetClipIntersection(e, *s, i));
			s = &e;
		}
	}
}

struct AMPWall
{
public:
	TArray<FVector2> points;
	FTextureID texid;
	float shiftx, shifty;
};
void AutoMap::Draw()
{
	TArray<AMPWall> pwalls;
	TArray<FVector2> points;
	const unsigned int mapwidth = map->GetHeader().width;
	const unsigned int mapheight = map->GetHeader().height;

	const fixed scale = GetScreenScale();

	if(!(amFlags & AMF_Overlay))
		screen->Clear(amx, amy+1, amx+amsizex, amy+amsizey+1, BackgroundColor.palcolor, BackgroundColor.color);

	const fixed playerx = players[0].mo->x;
	const fixed playery = players[0].mo->y;

	if(fullRefresh || amangle != ((amFlags & AMF_Rotate) ? players[0].mo->angle-ANGLE_90 : 0))
	{
		amangle = (amFlags & AMF_Rotate) ? players[0].mo->angle-ANGLE_90 : 0;
		minmaxSel = amangle/ANGLE_90;
		amsin = finesine[amangle>>ANGLETOFINESHIFT];
		amcos = finecosine[amangle>>ANGLETOFINESHIFT];

		// For rotating the tiles, this table includes the point offsets based on the current scale
		rottable[0][0] = FIXED2FLOAT(FixedMul(scale, amcos)); rottable[0][1] = FIXED2FLOAT(FixedMul(scale, amsin));
		rottable[1][0] = FIXED2FLOAT(FixedMul(scale, amcos) - FixedMul(scale, amsin)); rottable[1][1] = FIXED2FLOAT(FixedMul(scale, amsin) + FixedMul(scale, amcos));

		fullRefresh = false;
	}

	const fixed pany = FixedMul(ampany, amcos) - FixedMul(ampanx, amsin);
	const fixed panx = FixedMul(ampany, amsin) + FixedMul(ampanx, amcos);
	const fixed ofsx = playerx - panx;
	const fixed ofsy = playery - pany;

	const double originx = (amx+amsizex/2) - (FIXED2FLOAT(FixedMul(FixedMul(scale, ofsx&0xFFFF), amcos) - FixedMul(FixedMul(scale, ofsy&0xFFFF), amsin)));
	const double originy = (amy+amsizey/2) - (FIXED2FLOAT(FixedMul(FixedMul(scale, ofsx&0xFFFF), amsin) + FixedMul(FixedMul(scale, ofsy&0xFFFF), amcos)));
	const double origTexScale = FIXED2FLOAT(scale>>6);

	for(unsigned int my = 0;my < mapheight;++my)
	{
		MapSpot spot = map->GetSpot(0, my, 0);
		for(unsigned int mx = 0;mx < mapwidth;++mx, ++spot)
		{
			if(!((spot->amFlags & AM_Visible) || am_cheat || gamestate.fullmap) ||
				((amFlags & AMF_Overlay) && (spot->amFlags & AM_DontOverlay)))
				continue;

			if(TransformTile(spot, FixedMul((mx<<FRACBITS)-ofsx, scale), FixedMul((my<<FRACBITS)-ofsy, scale), points))
			{
				double texScale = origTexScale;

				FTexture *tex;
				Color *color = NULL;
				int brightness;
				if(spot->tile && !spot->pushAmount && !spot->pushReceptor)
				{
					brightness = 256;
					if((amFlags & AMF_DrawTexturedWalls))
					{
						if(spot->tile->overhead.isValid())
							tex = TexMan(spot->tile->overhead);
						else if(spot->tile->offsetHorizontal)
							tex = TexMan(spot->texture[MapTile::North]);
						else
							tex = TexMan(spot->texture[MapTile::East]);
					}
					else
					{
						tex = NULL;
						if(spot->tile->offsetHorizontal || spot->tile->offsetVertical)
							color = &DoorColor;
						else
							color = &WallColor;
					}
				}
				else if(spot->sector && !(amFlags & AMF_Overlay))
				{
					if(amFlags & AMF_DrawFloor)
					{
						brightness = 128;
						tex = TexMan(spot->sector->texture[MapSector::Floor]);
					}
					else
					{
						brightness = 256;
						tex = NULL;
						if(FloorColor.color != BackgroundColor.color)
							color = &FloorColor;
					}
				}
				else
				{
					tex = NULL;
				}

				if(tex)
				{
					// As a special case, since Noah's Ark stores the Automap
					// graphics in the TILE8, we need to override the scaling.
					if(tex->UseType == FTexture::TEX_FontChar)
						texScale *= 8;
					screen->FillSimplePoly(tex, &points[0], points.Size(), originx, originy, texScale, texScale, ~amangle, &NormalLight, brightness);
				}
				else if(color)
					screen->FillSimplePoly(NULL, &points[0], points.Size(), originx, originy, texScale, texScale, ~amangle, &NormalLight, brightness, color->palcolor, color->color);
			}

			// We need to check this even if the origin tile isn't visible since
			// the destination spot may be!
			if(spot->pushAmount)
			{
				fixed tx = (mx<<FRACBITS), ty = my<<FRACBITS;
				switch(spot->pushDirection)
				{
					default:
					case MapTile::East:
						tx += (spot->pushAmount<<10);
						break;
					case MapTile::West:
						tx -= (spot->pushAmount<<10);
						break;
					case MapTile::South:
						ty += (spot->pushAmount<<10);
						break;
					case MapTile::North:
						ty -= (spot->pushAmount<<10);
						break;
				}
				if(TransformTile(spot, FixedMul(tx-ofsx, scale), FixedMul(ty-ofsy, scale), points))
				{
					AMPWall pwall;
					pwall.points = points;
					pwall.texid = spot->tile->overhead.isValid() ? spot->tile->overhead : spot->texture[0];
					pwall.shiftx = (float)(FIXED2FLOAT(FixedMul(FixedMul(scale, tx&0xFFFF), amcos) - FixedMul(FixedMul(scale, ty&0xFFFF), amsin)));
					pwall.shifty = (float)(FIXED2FLOAT(FixedMul(FixedMul(scale, tx&0xFFFF), amsin) + FixedMul(FixedMul(scale, ty&0xFFFF), amcos)));
					pwalls.Push(pwall);
				}
			}
		}
	}

	for(unsigned int pw = pwalls.Size();pw-- > 0;)
	{
		AMPWall &pwall = pwalls[pw];
		if((amFlags & AMF_DrawTexturedWalls))
		{
			double texScale = origTexScale;
			FTexture *tex = TexMan(pwall.texid);
			if(tex)
			{
				// Noah's ark TILE8
				if(tex->UseType == FTexture::TEX_FontChar)
					texScale *= 8;
				screen->FillSimplePoly(tex, &pwall.points[0], pwall.points.Size(), originx + pwall.shiftx, originy + pwall.shifty, texScale, texScale, ~amangle, &NormalLight, 256);
			}
		}
		else
			screen->FillSimplePoly(NULL, &pwall.points[0], pwall.points.Size(), originx + pwall.shiftx, originy + pwall.shifty, origTexScale, origTexScale, ~amangle, &NormalLight, 256, WallColor.palcolor, WallColor.color);
	}

	DrawVector(AM_Arrow, 8, FixedMul(playerx - ofsx, scale), FixedMul(playery - ofsy, scale), scale, (amFlags & AMF_Rotate) ? 0 : ANGLE_90-players[0].mo->angle, ArrowColor);

	if((amFlags & AMF_ShowThings) && (am_cheat || gamestate.fullmap))
	{
		for(AActor::Iterator iter = AActor::GetIterator();iter.Next();)
		{
			if(am_cheat || (gamestate.fullmap && (iter->flags & FL_PLOTONAUTOMAP)))
				DrawActor(iter, FixedMul(iter->x - ofsx, scale), FixedMul(iter->y - ofsy, scale), scale);
		}
	}

	DrawStats();
}

void AutoMap::DrawActor(AActor *actor, fixed x, fixed y, fixed scale)
{
	{
		fixed tmp = ((FixedMul(x, amcos) - FixedMul(y, amsin) + (amsizex<<(FRACBITS-1)))>>FRACBITS) + amx;
		y = ((FixedMul(x, amsin) + FixedMul(y, amcos) + (amsizey<<(FRACBITS-1)))>>FRACBITS) + amy;
		x = tmp;

		int adiameter = FixedMul(actor->radius, scale)>>(FRACBITS-1);
		int aheight = scale>>FRACBITS;

		// Cull out actors that are obviously out of bounds
		if(x + adiameter < amx || x - adiameter >= amx+amsizex || y + aheight < amy || y - aheight >= amy+amsizey)
			return;
	}

	bool flip;
	FTexture *tex;
	if(actor->overheadIcon.isValid())
	{
		tex = actor->sprite != SPR_NONE ? TexMan(actor->overheadIcon) : NULL;
		flip = false;
	}
	else
		tex = R_GetAMSprite(actor, amangle, flip);

	if(!tex)
		return;

	if(tex->UseType != FTexture::TEX_FontChar)
	{
		double width = tex->GetScaledWidthDouble()*FIXED2FLOAT(scale>>6);
		double height = tex->GetScaledHeightDouble()*FIXED2FLOAT(scale>>6);
		screen->DrawTexture(tex, x, y,
			DTA_DestWidthF, width,
			DTA_DestHeightF, height,
			DTA_ClipLeft, amx,
			DTA_ClipRight, amx+amsizex,
			DTA_ClipTop, amy,
			DTA_ClipBottom, amy+amsizey,
			DTA_FlipX, flip,
			TAG_DONE);
	}
	else
	{
		screen->DrawTexture(tex, x - (scale>>(FRACBITS+1)), y - (scale>>(FRACBITS+1)),
			DTA_DestWidthF, FIXED2FLOAT(scale),
			DTA_DestHeightF, FIXED2FLOAT(scale),
			DTA_ClipLeft, amx,
			DTA_ClipRight, amx+amsizex,
			DTA_ClipTop, amy,
			DTA_ClipBottom, amy+amsizey,
			DTA_FlipX, flip,
			TAG_DONE);
	}
}

void AutoMap::DrawClippedLine(int x0, int y0, int x1, int y1, int palcolor, uint32 realcolor) const
{
	// Let us make an assumption that point 0 is always left of point 1
	if(x0 > x1)
	{
		swapvalues(x0, x1);
		swapvalues(y0, y1);
	}

	const int dx = x1 - x0, dy = y1 - y0;
	int ymin = MIN(y0, y1);
	int ymax = MAX(y0, y1);
	const bool inc = ymax == y0;
	bool clipped = false;

	// There will inevitably be precision issues so we may need to run the
	// clipper a few times.
	do
	{
		// Trivial culling
		if(x1 < amx || ymax < amy || x0 >= amx+amsizex || ymin >= amy+amsizey)
			return;

		if(x0 < amx) // Clip left
		{
			clipped = true;
			y0 += dy*(amx-x0)/dx;
			x0 = amx;
		}
		if(x1 >= amx+amsizex) // Clip right
		{
			clipped = true;
			y1 += dy*(amx+amsizex-1-x1)/dx;
			x1 = amx+amsizex-1;
		}
		if(ymin < amy) // Clip top
		{
			clipped = true;
			if(inc)
			{
				x1 += dx*(amy-y1)/dy;
				y1 = amy;
			}
			else
			{
				x0 += dx*(amy-y0)/dy;
				y0 = amy;
			}
		}
		if(ymax >= amy+amsizey) // Clip bottom
		{
			clipped = true;
			if(inc)
			{
				x0 += dx*(amy+amsizey-1-y0)/dy;
				y0 = amy+amsizey-1;
			}
			else
			{
				x1 += dx*(amy+amsizey-1-y1)/dy;
				y1 = amy+amsizey-1;
			}
		}

		if(!clipped)
			break;
		clipped = false;

		// Fix ymin/max for the next iteration
		if(inc)
		{
			ymin = y1;
			ymax = y0;
		}
		else
		{
			ymin = y0;
			ymax = y1;
		}
	}
	while(true);

	screen->DrawLine(x0, y0+1, x1, y1+1, palcolor, realcolor);
}

void AutoMap::DrawStats() const
{
	if(!(amFlags & (AMF_DispInfo|AMF_DispRatios)))
		return;

	FString statString;
	unsigned int infHeight = 0;

	if(amFlags & AMF_DispInfo)
	{
		infHeight = SmallFont->GetHeight()+2;
		screen->Dim(GPalette.BlackIndex, 0.5f, 0, 0, screenWidth, infHeight*CleanYfac);

		screen->DrawText(SmallFont, gameinfo.automap.FontColor, 2*CleanXfac, CleanYfac, levelInfo->GetName(map),
			DTA_CleanNoMove, true,
			TAG_DONE);

		unsigned int seconds = gamestate.TimeCount/70;
		statString.Format("%02d:%02d:%02d", seconds/3600, (seconds%3600)/60, seconds%60);
		screen->DrawText(SmallFont, gameinfo.automap.FontColor,
			screenWidth - (SmallFont->GetCharWidth('0')*6 + SmallFont->GetCharWidth(':')*2 + 2)*CleanXfac, CleanYfac,
			statString,
			DTA_CleanNoMove, true,
			TAG_DONE);
	}

	if(amFlags & AMF_DispRatios)
	{
		statString.Format("K: %d/%d\nS: %d/%d\nT: %d/%d",
			gamestate.killcount, gamestate.killtotal,
			gamestate.secretcount, gamestate.secrettotal,
			gamestate.treasurecount, gamestate.treasuretotal);

		word sw, sh;
		VW_MeasurePropString(SmallFont, statString, sw, sh);
		screen->Dim(GPalette.BlackIndex, 0.5f, 0, infHeight*CleanYfac, (sw+3)*CleanXfac, (sh+2)*CleanYfac);

		screen->DrawText(SmallFont, gameinfo.automap.FontColor, 2*CleanXfac, infHeight*CleanYfac, statString,
			DTA_CleanNoMove, true,
			TAG_DONE);
	}
}

void AutoMap::DrawVector(const AMVectorPoint *points, unsigned int numPoints, fixed x, fixed y, fixed scale, angle_t angle, const Color &c) const
{
	int x1, y1, x2, y2;

	fixed tmp = ((FixedMul(x, amcos) - FixedMul(y, amsin) + (amsizex<<(FRACBITS-1)))>>FRACBITS) + amx;
	y = ((FixedMul(x, amsin) + FixedMul(y, amcos) + (amsizey<<(FRACBITS-1)))>>FRACBITS) + amy;
	x = tmp;

	x1 = FixedMul(points[numPoints-1].X, scale)>>FRACBITS;
	y1 = FixedMul(points[numPoints-1].Y, scale)>>FRACBITS;
	if(angle)
	{
		const fixed rsin = finesine[angle>>ANGLETOFINESHIFT];
		const fixed rcos = finecosine[angle>>ANGLETOFINESHIFT];
		int tmp;
		tmp = FixedMul(x1, rcos) - FixedMul(y1, rsin);
		y1 = FixedMul(x1, rsin) + FixedMul(y1, rcos);
		x1 = tmp;

		for(unsigned int i = numPoints-1;i-- > 0;x1 = x2, y1 = y2)
		{
			x2 = FixedMul(points[i].X, scale)>>FRACBITS;
			y2 = FixedMul(points[i].Y, scale)>>FRACBITS;

			tmp = FixedMul(x2, rcos) - FixedMul(y2, rsin);
			y2 = FixedMul(x2, rsin) + FixedMul(y2, rcos);
			x2 = tmp;

			DrawClippedLine(x + x1, y + y1, x + x2, y + y2, c.palcolor, c.color);
		}
	}
	else
	{
		for(unsigned int i = numPoints-1;i-- > 0;x1 = x2, y1 = y2)
		{
			x2 = FixedMul(points[i].X, scale)>>FRACBITS;
			y2 = FixedMul(points[i].Y, scale)>>FRACBITS;

			DrawClippedLine(x + x1, y + y1, x + x2, y + y2, c.palcolor, c.color);
		}
	}
}

FVector2 AutoMap::GetClipIntersection(const FVector2 &p1, const FVector2 &p2, unsigned edge) const
{
	// If we are clipping a vertical line, it should be because our angle is
	// 90 degrees and it's clip against the top or bottom
	if((edge&1) == 1 && ((amangle>>ANGLETOFINESHIFT)&(ANG90-1)) == 0)
	{
		if(edge == 1)
			return FVector2(p1.X, amy);
		return FVector2(p1.X, amy+amsizey);
	}
	else
	{
		const float A = p2.Y - p1.Y;
		const float B = p1.X - p2.X;
		const float C = A*p1.X + B*p1.Y;
		switch(edge)
		{
			default:
			case 0: // Left
				return FVector2(amx, (C - A*amx)/B);
			case 1: // Top
				return FVector2((C - B*amy)/A, amy);
			case 2: // Right
				return FVector2(amx+amsizex, (C - A*(amx+amsizex))/B);
			case 3: // Bottom
				return FVector2((C - B*(amy+amsizey))/A, amy+amsizey);
		}
	}
}

// Returns size of a tile in pixels
fixed AutoMap::GetScreenScale() const
{
	// Some magic, min scale is approximately small enough so that a rotated automap will fit on screen (22/32 ~ 1/sqrt(2))
	const fixed minscale = ((screenHeight*22)<<(FRACBITS-5))/map->GetHeader().height;
	return minscale + FixedMul(absscale, (screenHeight<<(FRACBITS-3)) - minscale);
}

void AutoMap::SetFlags(unsigned int flags, bool set)
{
	if(set)
		amFlags |= flags;
	else
		amFlags &= ~flags;
}

void AutoMap::SetPanning(fixed x, fixed y, bool relative)
{
	if(relative)
	{
		// TODO: Make panning absolute instead of relative to the player so this isn't weird
		fixed posx, posy, maxx, maxy;
		if(amangle)
		{
			const int sizex = map->GetHeader().width;
			const int sizey = map->GetHeader().height;
			maxx = abs(sizex*amcos) + abs(sizey*amsin);
			maxy = abs(sizex*amsin) + abs(sizey*amcos);
			posx = players[0].mo->x - (sizex<<(FRACBITS-1));
			posy = players[0].mo->y - (sizey<<(FRACBITS-1));
			fixed tmp = (FixedMul(posx, amcos) - FixedMul(posy, amsin)) + (maxx>>1);
			posy = (FixedMul(posy, amcos) + FixedMul(posx, amsin)) + (maxy>>1);
			posx = tmp;
		}
		else
		{
			maxx = map->GetHeader().width<<FRACBITS;
			maxy = map->GetHeader().height<<FRACBITS;
			posx = players[0].mo->x;
			posy = players[0].mo->y;
		}
		ampanx = clamp<fixed>(ampanx+x, posx - maxx, posx);
		ampany = clamp<fixed>(ampany+y, posy - maxy, posy);
	}
	else
	{
		ampanx = x;
		ampany = y;
	}
}

void AutoMap::SetScale(fixed scale, bool relative)
{
	if(relative)
		absscale = FixedMul(absscale, scale);
	else
		absscale = scale;
	absscale = clamp<fixed>(absscale, FRACUNIT/50, FRACUNIT);

	fullRefresh = true;
}

bool AutoMap::TransformTile(MapSpot spot, fixed x, fixed y, TArray<FVector2> &points) const
{
	fixed rotx = FixedMul(x, amcos) - FixedMul(y, amsin) + (amsizex<<(FRACBITS-1));
	fixed roty = FixedMul(x, amsin) + FixedMul(y, amcos) + (amsizey<<(FRACBITS-1));
	double x1 = amx + FIXED2FLOAT(rotx);
	double y1 = amy + FIXED2FLOAT(roty);
	points.Resize(4);
	points[0] = FVector2(x1, y1);
	points[1] = FVector2(x1 + rottable[0][0], y1 + rottable[0][1]);
	points[2] = FVector2(x1 + rottable[1][0], y1 + rottable[1][1]);
	points[3] = FVector2(x1 - rottable[0][1], y1 + rottable[0][0]);


	const float xmin = points[AM_MinMax[minmaxSel].X.Min].X;
	const float xmax = points[AM_MinMax[minmaxSel].X.Max].X;
	const float ymin = points[AM_MinMax[minmaxSel].Y.Min].Y;
	const float ymax = points[AM_MinMax[minmaxSel].Y.Max].Y;

	// Cull out tiles which never touch the automap area
	if((xmax < amx || xmin > amx+amsizex) || (ymax < amy || ymin > amy+amsizey))
		return false;

	// If the tile is partially in the automap area, clip
	if((ymax > amy+amsizey || ymin < amy) || (xmax > amx+amsizex || xmin < amx))
		ClipTile(points);
	return true;
}

void BasicOverhead()
{
	if(am_needsrecalc)
		AM_ChangeResolution();

	switch(automap)
	{
		case AMA_Overlay:
			AM_Overlay.Draw();
			break;
		case AMA_Normal:
		{
			int oldviewsize = viewsize;
			viewsize = 20;
			DrawPlayScreen();
			viewsize = oldviewsize;
			AM_Main.Draw();
			break;
		}
		default: break;
	}
}
