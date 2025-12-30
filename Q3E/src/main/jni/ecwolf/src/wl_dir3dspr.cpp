#include "version.h"

#include "actor.h"
#include "wl_def.h"
#include "wl_draw.h"
#include "wl_shade.h"
#include "wl_main.h"
#include "c_cvars.h"

void Scale3DShaper(int x1, int x2, FTexture *shape, uint32_t flags, fixed ny1, fixed ny2,
				fixed nx1, fixed nx2, byte *vbuf, unsigned vbufPitch)
{
	//printf("%s(%d, %d, %p, %d, %f, %f, %f, %f, %p, %d)\n", __FUNCTION__, x1, x2, shape, flags, FIXED2FLOAT(ny1), FIXED2FLOAT(ny2), FIXED2FLOAT(nx1), FIXED2FLOAT(nx2), vbuf, vbufPitch);
	fixed dxx=(ny2-ny1)<<8,dzz=(nx2-nx1)<<8;
	fixed dxa=0,dza=0;

	int len=shape->GetWidth();

	ny1+=dxx>>9;
	nx1+=dzz>>9;

	dxa=-(dxx>>1),dza=-(dzz>>1);

	fixed height1 = heightnumerator/((nx1+(dza>>8))>>8);
	fixed height2 = heightnumerator/((nx1+((dza+dzz)>>8))>>8);
	fixed height=(height1<<12)+2048;

	int slinex = (int)((ny1+(dxa>>8))*scale/(nx1+(dza>>8))+centerx);
	int elinex = (int)((ny1+((dxa+dxx)>>8))*scale/(nx1+((dza+dzz)>>8))+centerx);
	int dx = elinex - slinex;
	if(!dx) return;

	fixed dheight=((height2-height1)<<12)/(fixed)dx;

	if(x2>viewwidth) x2=viewwidth;

	// Clip left edge
	if(slinex < 0)
	{
		height -= dheight*slinex;
		slinex = 0;
	}

	dxx/=len,dzz/=len;
	for(int i=0;i<len && slinex < viewwidth;i++)
	{
		if(i == len-1) // Absorb any round off error at the end of the sprite.
			elinex = x2;
		else
		{
			dxa+=dxx,dza+=dzz;
			elinex=(int)((ny1+(dxa>>8))*scale/(nx1+(dza>>8))+centerx);
			if(elinex < 0)
				continue;
		}

		const FTexture::Span *spans;
		const BYTE *line=shape->GetColumn(i, &spans);

		for(;slinex<elinex && slinex<x2;slinex++, height += dheight)
		{
			unsigned scale1=(unsigned)(height>>14);

			if(wallheight[slinex]<(height>>12) && scale1)
			{
				int pixheight=scale1;
				int upperedge=(viewheight-pixheight)/2;

				const FTexture::Span *span = spans;
				while(unsigned endy = span->TopOffset+span->Length)
				{
					unsigned j=span->TopOffset;
					++span;

					int ycnt=j*pixheight;
					int screndy=(ycnt>>6)+upperedge;
					byte *vmem;
					if(screndy<0) vmem=vbuf+slinex;
					else vmem=vbuf+screndy*vbufPitch+slinex;
					for(;j<endy;j++)
					{
						int scrstarty=screndy;
						ycnt+=pixheight;
						screndy=(ycnt>>6)+upperedge;
						if(scrstarty!=screndy && screndy>0)
						{
							BYTE col=line[j];
							if(scrstarty<0) scrstarty=0;
							if(screndy>viewheight) screndy=viewheight,j=endy;

							while(scrstarty<screndy)
							{
								*vmem=col;
								vmem+=vbufPitch;
								scrstarty++;
							}
						}
					}
				}
			}
		}

		slinex = elinex;
	}
}
