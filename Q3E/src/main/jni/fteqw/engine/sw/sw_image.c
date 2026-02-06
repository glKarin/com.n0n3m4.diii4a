#include "quakedef.h"
#ifdef SWQUAKE
#include "sw.h"

void SW_NukeAlpha(swimage_t *img)
{
	int x, y;
	unsigned int *d = img->data;
	for (y = 0; y < img->pheight; y++)
	{
		for (x = 0; x < img->pwidth; x++)
		{
			d[x] |= 0xff000000;
		}
		d += img->pitch;
	}
}

void SW_DestroyTexture(texid_t tex)
{
	swimage_t *img = tex->ptr;
	tex->ptr = NULL;

	/*make sure its not in use by the renderer*/
	SWRast_Sync(&commandqueue);

	/*okay, it can be killed*/
	BZ_Free(img);
}



void		SW_UpdateFiltering		(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float lodbias, float anis)
{
	//always nearest...
}

qboolean	SW_LoadTextureMips		(texid_t tex, const struct pendingtextureinfo *mips)
{
	swimage_t *img;
	int i;
	int nw = mips->mip[0].width;
	int nh = mips->mip[0].height;
	qbyte *indata = mips->mip[0].data;
	qbyte *imgdata;


	if (mips->type != PTI_2D)
		return false;

	//only accept formats that actually make sense here.
	switch(mips->encoding)
	{
	case PTI_RGBA8:
	case PTI_RGBX8:
	case PTI_BGRA8:
	case PTI_BGRX8:
		break;
	default:
		return false;
	}

	img = BZ_Malloc(sizeof(*img) - sizeof(img->data) + (nw * nh * 4));
	imgdata = (qbyte*)(img+1) - sizeof(img->data);
	tex->ptr = img;

	img->pwidth = nw;
	img->pheight = nh;
	img->pitch = nw;

	//precalculated
	img->pwidthmask = nw-1;
	img->pheightmask = nh-1;

	if (mips->encoding == PTI_RGBA8 || mips->encoding == PTI_RGBX8)
	{	//assuming PC hardware is bgr
		if (mips->encoding == PTI_RGBX8)
		{
			for (i = 0; i < nw*nh*4; i+=4)
			{
				imgdata[i+0] = indata[i+2];
				imgdata[i+1] = indata[i+1];
				imgdata[i+2] = indata[i+0];
				imgdata[i+3] = 255;
			}
		}
		else
		{
			for (i = 0; i < nw*nh*4; i+=4)
			{
				imgdata[i+0] = indata[i+2];
				imgdata[i+1] = indata[i+1];
				imgdata[i+2] = indata[i+0];
				imgdata[i+3] = indata[i+3];
			}
		}
	}
	else
	{
		memcpy(imgdata, indata, (nw * nh * 4));
		if (mips->encoding == PTI_BGRX8)
			for (i = 0; i < nw*nh*4; i+=4)
				imgdata[i+3] = 255;
	}

//	for (i = 0; i < mips->mipcount; i++)
//		if (mips->mip[i].needfree)
//			Z_Free(mips->mip[i].data);
//	if (mips->extrafree)
//		Z_Free(mips->extrafree);

	return true;
}
#endif
