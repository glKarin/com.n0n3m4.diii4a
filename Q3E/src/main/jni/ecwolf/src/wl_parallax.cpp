#include "c_cvars.h"
#include "g_mapinfo.h"
#include "id_ca.h"
#include "wl_def.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_play.h"
#include "wl_main.h"
#include "wl_draw.h"

extern fixed viewz;
extern int viewshift;

// Fill in a column of pixels. Could be potentially have a POT version but not
// sure if it's worthwhile.
static void DrawParallaxPlaneLoop(byte *vbuf, unsigned vbufPitch,
	const byte* src, int yshift, fixed h, fixed yStep, int y, int yend)
{
	vbuf += y*vbufPitch;

	for(fixed ty = (((y + yshift)) * yStep) % h; y < yend; vbuf += vbufPitch, ++y)
	{
		*vbuf = src[ty>>FRACBITS];

		if((ty += yStep) > h)
			ty -= h;
	}
}

// Draws one of the two sky planes: above or below wallheight
template<bool ceiling>
static void DrawParallaxPlane(byte *vbuf, unsigned vbufPitch,
	FTexture *skysource, int yshift,
	int midangle, fixed planeheight, int horizonheight, int skyscaledheight)
{
	const fixed heightFactor = abs(planeheight)>>8;

	const int w = skysource->GetWidth();
	const fixed h = (skysource->GetHeight())<<FRACBITS;
	const fixed yStep = h/skyscaledheight;

	const int cycle = skysource->GetScaledHeight() < 100 ? FINEANGLES : MIN(FINEANGLES, FINEANGLES*skysource->GetScaledWidth()/1024);

	int curtex = -1;
	const byte *skytex = NULL;
	for(int x = 0; x < viewwidth; x++)
	{
		int curang = pixelangle[x] + midangle;
		if(curang < 0) curang += FINEANGLES;
		else if(curang >= FINEANGLES) curang -= FINEANGLES;
		const int xtex = (FINEANGLES - curang - 1) * w / cycle;
		if(xtex != curtex)
		{
			curtex = xtex;

			skytex = skysource->GetColumn(xtex, NULL);
		}

		if(ceiling)
		{
			int yend = horizonheight - ((wallheight[x]*heightFactor)>>FRACBITS);
			if(yend <= 0)
				continue;
			if(yend >= viewheight)
				yend = viewheight;

			DrawParallaxPlaneLoop(vbuf+x, vbufPitch, skytex, yshift, h, yStep, 0, yend);
		}
		else
		{
			int ystart = horizonheight + ((wallheight[x]*heightFactor)>>FRACBITS);
			if(ystart < 0)
				ystart = 0;

			DrawParallaxPlaneLoop(vbuf+x, vbufPitch, skytex, yshift, h, yStep, ystart, viewheight);
		}
	}
}

void DrawParallax(byte *vbuf, unsigned vbufPitch)
{
	FTextureID skyid = levelInfo->Sky;
	double scrollSpeed = levelInfo->SkyScrollSpeed;
	int horizonOffset = levelInfo->SkyHorizonOffset;

	// Check if ROTT sky feature flags are set and use them if so
	if(map->GetHeader().sky.isValid())
	{
		skyid = map->GetHeader().sky;
		horizonOffset = map->GetHeader().skyHorizonOffset;
		scrollSpeed = 0.0;
	}

	if(!skyid.isValid())
		return;

	FTexture * const skysource = TexMan(skyid);
	const int skyheight = skysource->GetScaledHeight();

	// Texel of horizon line
	int skyhorizon = skyheight; // Wolf4SDL backwards compatibility
	if(skyheight >= 100 && skyheight <= 128)
	{
		// Transition to Doom - extend downwards 28 texels
		skyhorizon = 100;
	}
	else if(skyheight > 128 && skyheight <= 200)
	{
		// Doom/Hexen - extend upwards
		skyhorizon = skyheight-28;
	}
	else if(skyheight > 200)
	{
		// ROTT
		// Vanilla formula is 148 - horizonOffset - (top of player Z height/8)
		// We measure from the center horizon so add 100, and we take Z heights
		// from the floor instead of the ceiling so subtract 4.
		// Since most maps use a horizonOffset of 48 (8 in binary map terms),
		// we'll set that as the origin so MAPINFO defaults are sane.
		// TODO: Implement horizon shift from Z height which should be 1 texel
		//       per 8 units from the ground.  Viewbob and class view height do
		//       not apply.  Shift is limited to texture dimensions.
		skyhorizon = 196;
	}

	skyhorizon -= horizonOffset;

	// For a speed of 1 cycle roughly every 30 seconds (roughly in line with ZDoom)
	const angle_t scroll = xs_ToInt(scrollSpeed*gamestate.TimeCount*(1<<27)/TICRATE);
	const int midangle = (players[ConsolePlayer].camera->angle + scroll)>>ANGLETOFINESHIFT;
	// Position of world horizon line
	const int horizonheight = (viewheight >> 1) - viewshift;
	// We want to map the sky onto the upper and lower 100 pixels of the 320x200
	// canvas.  So we can use the psprite scale variables to determine the size.
	// Note: Round these up since the 1.2 scaling factor has round off
	const int skyscaledheight = ((MAX(100, skyheight)*pspriteyscale)+(FRACUNIT-1))>>FRACBITS;
	const int skyscaledhorizon = (skyhorizon*skyscaledheight+skyheight-1)/skyheight;

	// Determines the offset to y when determining texel
	int yshift = (skyscaledhorizon) - (viewheight>>1) + viewshift;
	if(yshift < 0)
		yshift = (skyscaledheight)-((-yshift)%(skyscaledheight));

	DrawParallaxPlane<true>(vbuf, vbufPitch, skysource, yshift, midangle, viewz+(map->GetPlane(0).depth<<FRACBITS), horizonheight, skyscaledheight);
	DrawParallaxPlane<false>(vbuf, vbufPitch, skysource, yshift, midangle, viewz, horizonheight, skyscaledheight);
}
