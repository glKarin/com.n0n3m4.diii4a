#include "../plugin.h"

static plugimagefuncs_t *imagefuncs;

//many of these look like dupes, not really sure how they're meant to work. probably legacy.
typedef enum {
	VMF_INVALID=-1,
	VMF_RGBA8=0,
//	VMF_ABGR8=1,
	VMF_RGB8=2,
	VMF_BGR8=3,
//	VMF_RGB565=4,
	VMF_I8=5,
	VMF_IA8=6,
//	VMF_P8=7,
//	VMF_A8=8,
//	VMF_RGB8_BS=9,
//	VMF_BGR8_BS=10,
//	VMF_ARGB_BS=11,
	VMF_BGRA8=12,
	VMF_BC1=13,
	VMF_BC2=14,
	VMF_BC3=15,
	VMF_BGRX8=16,
//	VMF_BGR565=17,
//	VMF_BGRX5551=18,
//	VMF_BGRA4444=19,
	VMF_BC1A=20,
//	VMF_BGRA5551=21,
	VMF_UV88=22,
//	VMF_UVWQ8=23,
	VMF_RGBA16F=24,
//	VMF_RGBA16N=25,
//	VMF_UVLX8=26,
	VMF_MAX
} fmtfmt_t;
static uploadfmt_t ImageVTF_VtfToFTE(fmtfmt_t f)
{
	switch(f)
	{
	case VMF_BC1:
		return PTI_BC1_RGB;
	case VMF_BC1A:
		return PTI_BC1_RGBA;
	case VMF_BC2:
		return PTI_BC2_RGBA;
	case VMF_BC3:
		return PTI_BC3_RGBA;
	case VMF_RGB8:
		return PTI_RGB8;
	case VMF_RGBA8:
		return PTI_RGBA8;
	case VMF_BGR8:
		return PTI_BGR8;
	case VMF_BGRA8:
		return PTI_BGRA8;
	case VMF_BGRX8:
		return PTI_BGRX8;
	case VMF_RGBA16F:
		return PTI_RGBA16F;
	case VMF_UV88:
		return PTI_RG8;
	case VMF_I8:
		return PTI_L8;
	case VMF_IA8:
		return PTI_L8A8;
	case VMF_INVALID:
		return PTI_INVALID;

	default:
		return PTI_INVALID;
	}
}
static struct pendingtextureinfo *Image_ReadVTFFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	//FIXME: cba with endian.
	struct vtf_s
	{
		char magic[4];
		unsigned int major,minor;
		unsigned int headersize;

		unsigned short width, height;
		unsigned int flags;
		unsigned short numframes, firstframe;
		unsigned int pad1;

		vec3_t reflectivity;
		float pad2;

		float bumpmapscale;
		unsigned int imgformat;
		unsigned char mipmapcount;
		unsigned char lowresfmt_misaligned[4];
		unsigned char lowreswidth;
		unsigned char lowresheight;

		//7.2
		unsigned char depth_misaligned[2];
		//7.3
		unsigned char pad3[3];
		unsigned int numresources;
	} *vtf;
	fmtfmt_t vmffmt, lrfmt;
	unsigned int bw, bh, bd, bb;
	qbyte *end = filedata + filesize;
	unsigned int faces, frame, frames, miplevel, miplevels, img;
	unsigned int w, h, d = 1;
	size_t	datasize;
	unsigned int version;

	struct pendingtextureinfo *mips;

	vtf = (void*)filedata;

	if (memcmp(vtf->magic, "VTF\0", 4))
		return NULL;

	version = (vtf->major<<16)|vtf->minor;
	if (version > 0x00070005)
	{
		Con_Printf("%s: VTF version %i.%i is not supported\n", fname, vtf->major, vtf->minor);
		return NULL;
	}

	lrfmt = (vtf->lowresfmt_misaligned[0]<<0)|(vtf->lowresfmt_misaligned[1]<<16)|(vtf->lowresfmt_misaligned[2]<<16)|(vtf->lowresfmt_misaligned[3]<<24);
	vmffmt = vtf->imgformat;

	mips = NULL;
	if (version >= 0x00070003)
	{
		int i;
		struct
		{
			unsigned int rtype;
			unsigned int rdata; //usually an offset.
		} *restable = (void*)(filedata+sizeof(*vtf));
		for (i = 0; i < vtf->numresources; i++, restable++)
		{
			if ((restable->rtype & 0x00ffffff) == 0x30)
			{
				mips = plugfuncs->Malloc(sizeof(*mips));
				mips->extrafree = filedata;
				filedata += restable->rdata;
				break;
			}
			//other unknown resource types.
		}
	}
	if (!mips)
	{
		mips = plugfuncs->Malloc(sizeof(*mips));
		mips->extrafree = filedata;

		//skip the header
		filedata += vtf->headersize;
		//and skip the low-res image too.
		if (vtf->lowreswidth && vtf->lowresheight)
			imagefuncs->BlockSizeForEncoding(ImageVTF_VtfToFTE(lrfmt), &bb, &bw, &bh, &bd);
		else
			bb=bw=bh=bd=1;
		datasize = ((vtf->lowreswidth+bw-1)/bw) * ((vtf->lowresheight+bh-1)/bh) * ((1/*vtf->lowresdepth*/+bd-1)/bd) * bb;
		filedata += datasize;
	}

	//now handle the high-res image
	if (mips)
	{
		mips->type = (vtf->flags & 0x4000)?PTI_CUBE:PTI_2D;

		mips->encoding = ImageVTF_VtfToFTE(vmffmt);
		imagefuncs->BlockSizeForEncoding(mips->encoding, &bb, &bw, &bh, &bd);

		miplevels = vtf->mipmapcount;
		frames = 1;//vtf->numframes;
		faces = ((mips->type==PTI_CUBE)?6:1);	//no cubemaps yet.

		mips->mipcount = miplevels * frames;
		while (mips->mipcount > countof(mips->mip))
		{
			if (miplevels > 1)
				miplevels--;
			else
				frames--;
			mips->mipcount = miplevels * frames;
		}
		if (!mips->mipcount)
		{
			plugfuncs->Free(mips);
			return NULL;
		}
		for (miplevel = vtf->mipmapcount; miplevel-- > 0;)
		{	//smallest to largest, which is awkward.
			w = vtf->width>>miplevel;
			h = vtf->height>>miplevel;
			if (!w)
				w = 1;
			if (!h)
				h = 1;
			datasize = ((w+bw-1)/bw) * ((h+bh-1)/bh) * ((d+bd-1)/bd) * bb;
			for (frame = 0; frame < vtf->numframes; frame++)
			{
				if (miplevel < miplevels)
				{
					img = miplevel + frame*miplevels;
					if (img >= countof(mips->mip))
						break;	//erk?
					if (filedata + datasize > end)
						break;	//no more data here...
					mips->mip[img].width = w;
					mips->mip[img].height = h;
					mips->mip[img].depth = faces;
					mips->mip[img].data = filedata;
					mips->mip[img].datasize = datasize*faces;
				}
				filedata += datasize*faces;
			}
		}
	}
	return mips;
}

static plugimageloaderfuncs_t vtffuncs =
{
	"Valve Texture File",
	sizeof(struct pendingtextureinfo),
	true,
	Image_ReadVTFFile,
};

qboolean VTF_Init(void)
{
	imagefuncs = plugfuncs->GetEngineInterface(plugimagefuncs_name, sizeof(*imagefuncs));
	if (!imagefuncs)
		return false;
	return plugfuncs->ExportInterface(plugimageloaderfuncs_name, &vtffuncs, sizeof(vtffuncs));
}

