/*
** r_sprites.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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

#include "textures/textures.h"
#include "c_cvars.h"
#include "r_sprites.h"
#include "linkedlist.h"
#include "tarray.h"
#include "templates.h"
#include "actor.h"
#include "thingdef/thingdef.h"
#include "v_palette.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_main.h"
#include "wl_play.h"
#include "wl_shade.h"
#include "zstring.h"
#include "r_data/colormaps.h"
#include "a_inventory.h"
#include "id_us.h"
#include "id_vh.h"

#define TEX_DWNAME(tex) MAKE_ID(tex->Name[0], tex->Name[1], tex->Name[2], tex->Name[3])

struct SpriteInfo
{
	union
	{
		char 		name[5];
		uint32_t	iname;
	};
	unsigned int	frames;
	unsigned int	numFrames;
};

struct Sprite
{
	static const uint8_t NO_FRAMES = 255; // If rotations == NO_FRAMES

	FTextureID	texture[8];
	uint8_t		rotations;
	uint16_t	mirror; // Mirroring bitfield
};

static TArray<Sprite> spriteFrames;
static TArray<SpriteInfo> loadedSprites;

bool R_CheckSpriteValid(unsigned int spr)
{
	if(spr < NUM_SPECIAL_SPRITES)
		return true;

	SpriteInfo &sprite = loadedSprites[spr];
	if(sprite.numFrames == 0)
		return false;
	return true;
}

uint32_t R_GetNameForSprite(unsigned int index)
{
	return loadedSprites[index].iname;
}

// Cache sprite name lookups
unsigned int R_GetSprite(const char* spr)
{
	static unsigned int mid = 0;

	union
	{
		char 		name[4];
		uint32_t	iname;
	} tmp;
	memcpy(tmp.name, spr, 4);

	if(tmp.iname == loadedSprites[mid].iname)
		return mid;

	for(mid = 0;mid < NUM_SPECIAL_SPRITES;++mid)
	{
		if(tmp.iname == loadedSprites[mid].iname)
			return mid;
	}

	unsigned int max = loadedSprites.Size()-1;
	unsigned int min = NUM_SPECIAL_SPRITES;
	mid = (min+max)/2;
	do
	{
		if(tmp.iname == loadedSprites[mid].iname)
			return mid;

		if(tmp.iname < loadedSprites[mid].iname)
			max = mid-1;
		else if(tmp.iname > loadedSprites[mid].iname)
			min = mid+1;
		mid = (min+max)/2;
	}
	while(max >= min);

	// I don't think this should ever happen, but if it does return no sprite.
	return 0;
}

FTexture *R_GetAMSprite(AActor *actor, angle_t rotangle, bool &flip)
{
	if(actor->sprite == SPR_NONE || loadedSprites[actor->sprite].numFrames == 0)
		return NULL;

	const Sprite &spr = spriteFrames[loadedSprites[actor->sprite].frames+actor->state->frame];
	FTexture *tex;
	if(spr.rotations == 0)
	{
		tex = TexMan[spr.texture[0]];
		flip = false;
	}
	else
	{
		int rot = (rotangle-actor->angle-(ANGLE_90-ANGLE_45/2))/ANGLE_45;
		tex = TexMan[spr.texture[rot]];
		flip = (spr.mirror>>rot)&1;
	}
	return tex;
}

void R_InstallSprite(Sprite &frame, FTexture *tex, int dir, bool mirror)
{
	if(dir < -1 || dir >= 8)
	{
		printf("Invalid frame data for '%s'.\n", tex->Name.GetChars());
		return;
	}

	if(dir == -1)
	{
		frame.rotations = 0;
		dir = 0;
	}
	else
		frame.rotations = 8;

	frame.texture[dir] = tex->GetID();
	if(mirror)
		frame.mirror |= 1<<dir;
}

unsigned int R_GetNumLoadedSprites()
{
	return loadedSprites.Size();
}

void R_GetSpriteHitlist(BYTE* hitlist)
{
	// Start by getting a list of currently in use sprites and then tell the
	// precacher to load them.

	BYTE* sprites = new BYTE[loadedSprites.Size()];
	memset(sprites, 0, loadedSprites.Size());

	for(AActor::Iterator iter = AActor::GetIterator();iter.Next();)
	{
		sprites[iter->state->spriteInf] = 1;
	}

	for(unsigned int i = loadedSprites.Size();i-- > NUM_SPECIAL_SPRITES;)
	{
		if(!sprites[i])
			continue;

		SpriteInfo &sprInf = loadedSprites[i];
		Sprite *frame = &spriteFrames[sprInf.frames];
		for(unsigned int j = sprInf.numFrames;j-- > 0;++frame)
		{
			if(frame->rotations == Sprite::NO_FRAMES)
				continue;

			// 0 rotations means 1 rotation in this context
			for(unsigned int k = MAX<unsigned int>(frame->rotations, 1);k-- > 0;)
			{
				if(frame->texture[k].isValid())
					hitlist[frame->texture[k].GetIndex()] |= 1;
			}
		}
	}

	delete[] sprites;
}

int SpriteCompare(const void *s1, const void *s2)
{
	uint32_t n1 = static_cast<const SpriteInfo *>(s1)->iname;
	uint32_t n2 = static_cast<const SpriteInfo *>(s2)->iname;
	if(n1 < n2)
		return -1;
	else if(n1 > n2)
		return 1;
	return 0;
}

void R_InitSprites()
{
	static const uint8_t MAX_SPRITE_FRAMES = 29; // A-Z, [, \, ]

	// First sort the loaded sprites list
	qsort(&loadedSprites[NUM_SPECIAL_SPRITES], loadedSprites.Size()-NUM_SPECIAL_SPRITES, sizeof(loadedSprites[0]), SpriteCompare);

	typedef LinkedList<FTexture*> SpritesList;
	typedef TMap<uint32_t, SpritesList> SpritesMap;

	SpritesMap spritesMap;

	// Collect potential sprite list (linked list of sprites by name)
	for(unsigned int i = TexMan.NumTextures();i-- > 0;)
	{
		FTexture *tex = TexMan.ByIndex(i);
		if(tex->UseType == FTexture::TEX_Sprite && strlen(tex->Name) >= 6)
		{
			SpritesList &list = spritesMap[TEX_DWNAME(tex)];
			list.Push(tex);
		}
	}

	// Now process the sprites if we need to load them
	for(unsigned int i = NUM_SPECIAL_SPRITES;i < loadedSprites.Size();++i)
	{
		SpritesList &list = spritesMap[loadedSprites[i].iname];
		if(list.Size() == 0)
			continue;
		loadedSprites[i].frames = spriteFrames.Size();

		Sprite frames[MAX_SPRITE_FRAMES];
		uint8_t maxframes = 0;
		for(unsigned int j = 0;j < MAX_SPRITE_FRAMES;++j)
		{
			frames[j].rotations = Sprite::NO_FRAMES;
			frames[j].mirror = 0;
		}

		for(SpritesList::Iterator iter = list.Head();iter;++iter)
		{
			FTexture *tex = iter;
			unsigned char frame = tex->Name[4] - 'A';
			if(frame < MAX_SPRITE_FRAMES)
			{
				if(frame > maxframes)
					maxframes = frame;
				R_InstallSprite(frames[frame], tex, tex->Name[5] - '1', false);

				if(strlen(tex->Name) == 8)
				{
					frame = tex->Name[6] - 'A';
					if(frame < MAX_SPRITE_FRAMES)
					{
						if(frame > maxframes)
							maxframes = frame;
						R_InstallSprite(frames[frame], tex, tex->Name[7] - '1', true);
					}
				}
			}
		}

		++maxframes;
		for(unsigned int j = 0;j < maxframes;++j)
		{
			// Check rotations
			if(frames[j].rotations == 8)
			{
				for(unsigned int r = 0;r < 8;++r)
				{
					if(!frames[j].texture[r].isValid())
					{
						printf("Sprite %s is missing rotations for frame %c.\n", loadedSprites[i].name, j);
						break;
					}
				}
			}

			spriteFrames.Push(frames[j]);
		}

		loadedSprites[i].numFrames = maxframes;
	}

	// Special case for PLAY, if it doesn't exist then swap with UNKN.
	if(spritesMap[MAKE_ID('P','L','A','Y')].Size() == 0)
	{
		SpriteInfo &playsprite = loadedSprites[R_GetSprite("PLAY")];
		SpriteInfo &unknsprite = loadedSprites[R_GetSprite("UNKN")];
		playsprite.frames = spriteFrames.Size();
		playsprite.numFrames = MAX_SPRITE_FRAMES;
		for(char i = 0;i < MAX_SPRITE_FRAMES;++i)
			// Force early copy here since TArray does not copy before growing
			// the array (assumes that reference is not from the array)
			spriteFrames.Push(Sprite(spriteFrames[unknsprite.frames]));
	}
}

void R_LoadSprite(const FString &name)
{
	if(loadedSprites.Size() == 0)
	{
		// Make sure the special sprites are loaded
		SpriteInfo sprInf;
		sprInf.frames = 0;
		strcpy(sprInf.name, "TNT1");
		loadedSprites.Push(sprInf);
	}

	if(name.Len() != 4)
	{
		printf("Sprite name invalid.\n");
		return;
	}

	static uint32_t lastSprite = 0;
	SpriteInfo sprInf;
	sprInf.frames = 0;
	sprInf.numFrames = 0;

	strcpy(sprInf.name, name.GetChars());
	if(loadedSprites.Size() > 0)
	{
		if(sprInf.iname == lastSprite)
			return;

		for(unsigned int i = 0;i < loadedSprites.Size();++i)
		{
			if(loadedSprites[i].iname == sprInf.iname)
			{
				sprInf = loadedSprites[i];
				lastSprite = sprInf.iname;
				return;
			}
		}
	}
	lastSprite = sprInf.iname;

	loadedSprites.Push(sprInf);
}

////////////////////////////////////////////////////////////////////////////////

// From wl_draw.cpp
unsigned int CalcRotate(AActor *ob);
extern byte* vbuf;
extern unsigned vbufPitch;
extern int viewshift;
extern fixed viewz;

void ScaleSprite(AActor *actor, int xcenter, const Frame *frame, unsigned height)
{
	// height is a 13.3 fixed point number indicating the number of screen
	// pixels that the sprite should occupy.
	if(height < 8)
		return;

	// Check if we're rendering completely off screen.
	// Simpler form:
	// topoffset = ( viewheight/2 - viewshift - (signed(height>>3)*(viewz+(32<<FRACBITS))/(32<<FRACBITS)) )<<3;
	const int topoffset = (viewheight<<2) - (viewshift<<3) -
	                      FixedMul(height, (viewz+(actor->z<<6)+(32<<FRACBITS))>>5);
	if(-topoffset >= (signed)height)
		return;

	if(actor->sprite == SPR_NONE || loadedSprites[actor->sprite].numFrames == 0)
		return;

	bool flip = false;
	const Sprite &spr = spriteFrames[loadedSprites[actor->sprite].frames+frame->frame];
	FTexture *tex;
	if(spr.rotations == 0)
		tex = TexMan[spr.texture[0]];
	else
	{
		const unsigned int rot = CalcRotate(actor);
		tex = TexMan[spr.texture[rot]];
		flip = (spr.mirror>>rot)&1;
	}
	if(tex == NULL)
		return;

	const double dyScale = (height/256.0)*FIXED2FLOAT(actor->scaleY);
	const int upperedge = topoffset + height - static_cast<int>((tex->GetScaledTopOffsetDouble())*dyScale*8);

	const double dxScale = (height/256.0)*FIXED2FLOAT(FixedDiv(actor->scaleX, yaspect));
	const int actx = static_cast<int>(xcenter - tex->GetScaledLeftOffsetDouble()*dxScale);

	const unsigned int texWidth = tex->GetWidth();
	const unsigned int startX = -MIN(actx, 0);
	const unsigned int startY = -MIN(upperedge>>3, 0);
	const fixed xStep = static_cast<fixed>(tex->xScale/dxScale);
	const fixed yStep = static_cast<fixed>(tex->yScale/dyScale);
	const fixed xRun = MIN<fixed>(texWidth<<FRACBITS, xStep*(viewwidth-actx));
	const fixed yRun = MIN<fixed>(tex->GetHeight()<<FRACBITS, (yStep*((viewheight<<3)-upperedge))>>3);

	const BYTE *colormap;
	if((actor->flags & FL_BRIGHT) || frame->fullbright)
		colormap = NormalLight.Maps;
	else
	{
		const int shade = LIGHT2SHADE(gLevelLight + r_extralight);
		const int tz = FixedMul(r_depthvisibility<<8, height);
		colormap = &NormalLight.Maps[GETPALOOKUP(MAX(tz, MINZ), shade)<<8];
	}
	const BYTE *src;
	byte *destBase = vbuf + actx + startX + ((upperedge>>3) > 0 ? vbufPitch*(upperedge>>3) : 0);
	byte *dest = destBase;
	unsigned int i;
	fixed x, y;
	for(i = actx+startX, x = startX*xStep;x < xRun;x += xStep, ++i, dest = ++destBase)
	{
		if(wallheight[i] > (signed)height)
			continue;

		src = tex->GetColumn(flip ? texWidth - (x>>FRACBITS) - 1 : (x>>FRACBITS), NULL);

		for(y = startY*yStep;y < yRun;y += yStep)
		{
			if(src[y>>FRACBITS])
				*dest = colormap[src[y>>FRACBITS]];
			dest += vbufPitch;
		}
	}
}

void Scale3DSpriter(AActor *actor, int x1, int x2, FTexture *tex, bool flip, const Frame *frame, fixed ny1, fixed ny2, fixed nx1, fixed nx2)
{
	if(actor->sprite == SPR_NONE || loadedSprites[actor->sprite].numFrames == 0)
		return;

	const unsigned int texWidth = tex->GetWidth();
	unsigned height1 = (word)(heightnumerator/(nx1>>8));
	unsigned height2 = (word)(heightnumerator/(nx2>>8));
	
	unsigned height = height1;

	int scale = height>>3; // Integer part of the height
	int topoffset = (scale*(viewz+(actor->z<<6)+(32<<FRACBITS))/(32<<FRACBITS));

	if(scale == 0 || -(viewheight/2 - viewshift - topoffset) >= scale)
		return;

	double dyScale = (height/256.0)*(actor->scaleY/65536.);
	int upperedge = static_cast<int>((viewheight/2 - viewshift - topoffset)+scale - tex->GetScaledTopOffsetDouble()*dyScale);
	
	fixed yStep = static_cast<fixed>(tex->yScale/dyScale);
	unsigned int startY = -MIN(upperedge, 0);
	fixed endY = MIN<fixed>(tex->GetHeight()<<FRACBITS, yStep*(viewheight-upperedge));

	// [XA] TODO: shade the sprite per-column?
	const BYTE *colormap;
	if((actor->flags & FL_BRIGHT) || frame->fullbright)
		colormap = NormalLight.Maps;
	else
	{
		const int shade = LIGHT2SHADE(gLevelLight + r_extralight);
		const int tz = FixedMul(r_depthvisibility<<8, height);
		colormap = &NormalLight.Maps[GETPALOOKUP(MAX(tz, MINZ), shade)<<8];
	}
	const BYTE *src;

	byte *dest;
	int i;
	unsigned int x;

	//printf("%f, %f, %f, %f\n", FIXED2FLOAT(ny1), FIXED2FLOAT(ny2), FIXED2FLOAT(nx1), FIXED2FLOAT(nx1));
	fixed dxx=(ny2-ny1)<<8,dzz=(nx2-nx1)<<8;
	fixed dxa = 0, dza = 0;
	dxx/=(signed)texWidth,dzz/=(signed)texWidth;
	dxa+=dxx,dza+=dzz;
	int nexti = (int)((ny1+(dxa>>8))*::scale/(nx1+(dza>>8))+centerx);
	src = tex->GetColumn(flip ? texWidth - 1 : 0, NULL);

	for(i = x1, x = 0; i < x2; ++i)
	{
		while(i > nexti)
		{
			++x;
			assert(x < texWidth);
			src = tex->GetColumn(flip ? texWidth - x - 1 : x, NULL);

			dxa += dxx;
			dza += dzz;
			nexti = (int)((ny1+(dxa>>8))*::scale/(nx1+(dza>>8))+centerx);
		}

		// linear interpolation oh no
		height = (unsigned)(height1 + ((int)height2 - (int)height1) * ((double)(i+1 - x1) / (x2 - x1)));

		// recalculation double oh no
		scale = height>>3;
		topoffset = (scale*(viewz+(actor->z<<6)+(32<<FRACBITS))/(32<<FRACBITS));

		if(i < 0 || i >= viewwidth || wallheight[i] > (signed)height || scale == 0 || -(viewheight/2 - viewshift - topoffset) >= scale)
			continue;
		
		dest = vbuf + i + (upperedge > 0 ? vbufPitch*upperedge : 0);
		for(fixed y = startY*yStep;y < endY;y += yStep)
		{
			if(src[y>>FRACBITS])
				*dest = colormap[src[y>>FRACBITS]];
			dest += vbufPitch;
		}

		dyScale = (height/256.0)*(actor->scaleY/65536.);
		upperedge = static_cast<int>((viewheight/2 - viewshift - topoffset)+scale - tex->GetScaledTopOffsetDouble()*dyScale);
		
		yStep = static_cast<fixed>(tex->yScale/dyScale);
		startY = -MIN(upperedge, 0);
		endY = MIN<fixed>(tex->GetHeight()<<FRACBITS, yStep*(viewheight-upperedge));
	}
}

bool UseWolf4SDL3DSpriteScaler = false;
void Scale3DShaper(int, int, FTexture *, uint32_t, fixed, fixed, fixed, fixed, byte *, unsigned);

// This function from Wolf4SDL more or less verbatim at the moment.
void Scale3DSprite(AActor *actor, const Frame *frame, unsigned height)
{
	bool flip = false;
	const Sprite &spr = spriteFrames[loadedSprites[actor->sprite].frames+frame->frame];
	FTexture *tex;
	if(spr.rotations == 0)
		tex = TexMan[spr.texture[0]];
	else
	{
		const unsigned int rot = CalcRotate(actor);
		tex = TexMan[spr.texture[rot]];
		flip = (spr.mirror>>rot)&1;
	}
	if(tex == NULL)
		return;

	fixed nx1,nx2,ny1,ny2;
	int viewx1,viewx2;
	fixed diradd;
	fixed playx = viewx;
	fixed playy = viewy;

	fixed gy1,gy2,gx1,gx2,gyt1,gyt2,gxt1,gxt2;

	// translate point to view centered coordinates
	const fixed scaledOffset = FixedMul(FLOAT2FIXED(tex->GetScaledLeftOffsetDouble()), actor->scaleX);
	const fixed scaledWidth = FixedMul(FLOAT2FIXED(tex->GetScaledWidthDouble()), actor->scaleX);
	gy1 = actor->y-playy-(FixedMul(scaledOffset, finecosine[actor->angle>>ANGLETOFINESHIFT])>>6);
	gy2 = gy1+(FixedMul(scaledWidth, finecosine[actor->angle>>ANGLETOFINESHIFT])>>6);
	gx1 = actor->x-playx-(FixedMul(scaledOffset, finesine[actor->angle>>ANGLETOFINESHIFT])>>6);
	gx2 = gx1+(FixedMul(scaledWidth, finesine[actor->angle>>ANGLETOFINESHIFT])>>6);
	
	// calculate newx
	gxt1 = FixedMul(gx1,viewcos);
	gxt2 = FixedMul(gx2,viewcos);
	gyt1 = FixedMul(gy1,viewsin);
	gyt2 = FixedMul(gy2,viewsin);
	nx1 = gxt1-gyt1;
	nx2 = gxt2-gyt2;
	
	// calculate newy
	gxt1 = FixedMul(gx1,viewsin);
	gxt2 = FixedMul(gx2,viewsin);
	gyt1 = FixedMul(gy1,viewcos);
	gyt2 = FixedMul(gy2,viewcos);
	ny1 = gyt1+gxt1;
	ny2 = gyt2+gxt2;
	
	if(nx1 < 0 || nx2 < 0) return; // TODO: Clip on viewplane
	
	// calculate perspective ratio
	if(nx1>=0 && nx1<=1792) nx1=1792;
	if(nx1<0 && nx1>=-1792) nx1=-1792;
	if(nx2>=0 && nx2<=1792) nx2=1792;
	if(nx2<0 && nx2>=-1792) nx2=-1792;

	viewx1=(int)(centerx+ny1*scale/nx1);
	viewx2=(int)(centerx+ny2*scale/nx2);

	// Switch between original Wolf4SDL scaler and a new one.
	if(UseWolf4SDL3DSpriteScaler)
	{
		if(viewx2 < viewx1)
		{
			Scale3DShaper(viewx2,viewx1+1,tex,0,ny2,ny1,nx2,nx1,vbuf,vbufPitch);
		}
		else
		{
			Scale3DShaper(viewx1,viewx2+1,tex,0,ny1,ny2,nx1,nx2,vbuf,vbufPitch);
		}
	}
	else
	{
		if(viewx2 < viewx1)
		{
			Scale3DSpriter(actor, viewx2, viewx1+1, tex, flip, frame, ny2, ny1, nx2, nx1);
		}
		else
		{
			Scale3DSpriter(actor, viewx1, viewx2+1, tex, flip, frame, ny1, ny2, nx1, nx2);
		}
	}
}

void R_DrawPlayerSprite(AActor *actor, const Frame *frame, fixed offsetX, fixed offsetY)
{
	if(frame->spriteInf == SPR_NONE || loadedSprites[frame->spriteInf].numFrames == 0)
		return;

	const Sprite &spr = spriteFrames[loadedSprites[frame->spriteInf].frames+frame->frame];
	FTexture *tex;
	if(spr.rotations == 0)
		tex = TexMan[spr.texture[0]];
	else
		tex = TexMan[spr.texture[(CalcRotate(actor)+4)%8]];
	if(tex == NULL)
		return;

	const BYTE *colormap;
	if(frame->fullbright)
		colormap = NormalLight.Maps;
	else
	{
		const int shade = LIGHT2SHADE(gLevelLight) - (gLevelMaxLightVis/LIGHTVISIBILITY_FACTOR);
		colormap = &NormalLight.Maps[GETPALOOKUP(0, shade)<<8];
	}

	const fixed scale = viewheight<<(FRACBITS-1);

	const fixed centeringOffset = (centerx - 2*centerxwide)<<FRACBITS;
	const fixed leftedge = FixedMul((160<<FRACBITS) - fixed(tex->GetScaledLeftOffsetDouble()*FRACUNIT) + offsetX, pspritexscale) + centeringOffset;
	fixed upperedge = ((100-32)<<FRACBITS) + fixed(tex->GetScaledTopOffsetDouble()*FRACUNIT) - offsetY - AspectCorrection[r_ratio].tallscreen;
	if(viewsize == 21 && players[ConsolePlayer].ReadyWeapon)
	{
		upperedge -= players[ConsolePlayer].ReadyWeapon->yadjust;
	}
	upperedge = scale - FixedMul(upperedge, pspriteyscale);

	// startX and startY indicate where the sprite becomes visible, we only
	// need to calculate the start since the end will be determined when we hit
	// the view during drawing.
	const unsigned int startX = -MIN<unsigned int>(leftedge>>FRACBITS, 0);
	const unsigned int startY = -MIN<unsigned int>(upperedge>>FRACBITS, 0);
	const fixed xStep = FixedDiv(tex->xScale, pspritexscale);
	const fixed yStep = FixedDiv(tex->yScale, pspriteyscale);

	const int x1 = leftedge>>FRACBITS;
	const int y1 = upperedge>>FRACBITS;
	const fixed xRun = MIN<fixed>(tex->GetWidth()<<FRACBITS, xStep*(viewwidth-x1-startX));
	const fixed yRun = MIN<fixed>(tex->GetHeight()<<FRACBITS, yStep*(viewheight-y1));
	const BYTE *src;
	byte *destBase = vbuf+x1+startX + (y1 > 0 ? vbufPitch*y1 : 0);
	byte *dest = destBase;
	fixed x, y;
	for(x = startX*xStep;x < xRun;x += xStep)
	{
		src = tex->GetColumn(x>>FRACBITS, NULL);

		for(y = startY*yStep;y < yRun;y += yStep)
		{
			if(src[y>>FRACBITS] != 0)
				*dest = colormap[src[y>>FRACBITS]];
			dest += vbufPitch;
		}

		dest = ++destBase;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// S3DNA Zoomer
//

IMPLEMENT_INTERNAL_CLASS(SpriteZoomer)

SpriteZoomer::SpriteZoomer(FTextureID texID, unsigned short zoomtime) :
	Thinker(ThinkerList::VICTORY), frame(NULL), texID(texID), count(0), zoomtime(zoomtime)
{
}

SpriteZoomer::SpriteZoomer(const Frame *frame, unsigned short zoomtime) :
	Thinker(ThinkerList::VICTORY), frame(frame), count(0), zoomtime(zoomtime)
{
	frametics = frame->duration;
}

void SpriteZoomer::Draw()
{
	FTexture *gmoverTex;
	if(frame)
	{
		const Sprite &spr = spriteFrames[loadedSprites[frame->spriteInf].frames+frame->frame];
		gmoverTex = TexMan[spr.texture[0]];
	}
	else
		gmoverTex = TexMan(texID);

	// What we're trying to do is zoom in a 160x160 player sprite to
	// fill the viewheight.  S3DNA use the player sprite rendering
	// function and passed count as the height. We won't do it like that
	// since that method didn't account for the view window size
	// (vanilla could crash) and our player sprite renderer may take
	// into account things we would rather not have here.
	const double yscale = double(viewheight*count)/double(zoomtime*64);
	const double xscale = yscale/FIXED2FLOAT(yaspect);

	screen->DrawTexture(gmoverTex, viewscreenx + (viewwidth>>1), viewscreeny + (viewheight>>1) + yscale*32,
		DTA_DestWidthF, gmoverTex->GetScaledWidthDouble()*xscale,
		DTA_DestHeightF, gmoverTex->GetScaledHeightDouble()*yscale,
		TAG_DONE);
}

void SpriteZoomer::Tick()
{
	if(frame)
	{
		if(--frametics <= 0)
		{
			do
			{
				frame = frame->next;
				frametics = frame->duration;
			}
			while(frametics == 0);
		}
	}

	assert(count <= zoomtime);
	if(++count > zoomtime)
		Destroy();
}

void R_DrawZoomer(FTextureID texID)
{
	TObjPtr<SpriteZoomer> zoomer = new SpriteZoomer(texID, 192);
	do
	{
		for(unsigned int t = tics;zoomer && t-- > 0;)
			zoomer->Tick();
		if(!zoomer)
			break;

		ThreeDRefresh();
		zoomer->Draw();
		VH_UpdateScreen();
		CalcTics();
	}
	while(true);
}
