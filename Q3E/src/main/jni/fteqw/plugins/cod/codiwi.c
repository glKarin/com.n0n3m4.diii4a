#include "../plugin.h"

static plugimagefuncs_t *imagefuncs;
static struct pendingtextureinfo *Image_ReadIWIFile(unsigned int imgflags, const char *fname, qbyte *filedata, size_t filesize)
{
	if (filesize >= 12 && filedata[0]=='I'&&filedata[1]=='W'&&filedata[2]=='i' && (filedata[3]==5/*cod2*/||filedata[3]==6/*cod4*/))
	{
		static const enum uploadfmt fmts[] = {PTI_INVALID/*0*/, PTI_RGBA8/*1*/, PTI_RGB8/*2*/, PTI_L8A8/*3, VALIDATE!*/, PTI_INVALID/*4,PTI_A8*/, PTI_INVALID/*5*/,PTI_INVALID/*6*/,PTI_INVALID/*7*/,PTI_INVALID/*8*/,PTI_INVALID/*9*/,PTI_INVALID/*10*/,PTI_BC1_RGBA/*11*/, PTI_BC2_RGBA/*12*/, PTI_BC3_RGBA/*13*/};
		enum uploadfmt fmt = PTI_INVALID;
		unsigned int bb,bw,bh,bd;
		unsigned int iw,ih,id, l;
		struct pendingtextureinfo *mips;
		unsigned int offsets[4];
		qbyte *end = filedata+filesize;
		enum {
			IWI_STANDARD=0,
			IWI_MIPLESS=3,
			IWI_CUBEMAP=6,
//			IWI_NORMALMAP=0x20,
//			IWI_UNKNOWN=0x40,
//			IWI_UNKNOWN=0x80,
		} usage = filedata[5];
		if (filedata[4] < countof(fmts))
			fmt = fmts[filedata[4]];
		if (fmt == PTI_INVALID)
		{	//bail. with warning
			Con_Printf(CON_WARNING"Image_ReadIWIFile(%s): unsupported iwi pixelformat %x\n", fname, filedata[4]);
			return NULL;
		}
		iw = filedata[6] | (filedata[7]<<8);
		ih = filedata[8] | (filedata[9]<<8);
		id = filedata[10] | (filedata[11]<<8);

		imagefuncs->BlockSizeForEncoding(fmt, &bb, &bw, &bh, &bd);
		if (!bb)
		{
			Con_Printf(CON_WARNING"Image_ReadIWIFile(%s): unsupported fte pixelformat %x(%i)\n", fname, filedata[4], fmt);
			return NULL;
		}

		mips = plugfuncs->Malloc(sizeof(*mips));
		mips->encoding = fmt;
		if ((filedata[5]&0xf) == IWI_CUBEMAP && id==1)
		{
			mips->type = PTI_CUBE;
			id *= 6;
		}
		else
			mips->type = (id>1)?PTI_3D:PTI_2D;
		mips->extrafree = filedata;
		filedata += 12;
		for (l = 0; l < countof(offsets); l++, filedata+=4)
			offsets[l] = filedata[0] | (filedata[1]<<8) | (filedata[2]<<16) | (filedata[3]<<24);	//dunno what this 4 values are for. looks like descending ends?
		if (mips->type != PTI_2D || (usage&0xf)==IWI_MIPLESS)
			mips->mipcount = l = 1;
		else for (l = 0; l < countof(mips->mip); l++)
		{
			if ((iw >> l) || (ih >> l) || (id >> l))
				mips->mipcount++;
			else
				break;
		}
		//these are smallest to biggest.
		while (l --> 0)
		{
			unsigned int w = iw>>l;
			unsigned int h = ih>>l;
			unsigned int d = (mips->type==PTI_3D)?id>>l:id;
			size_t datasize;
			if (!w && !h && ((mips->type==PTI_3D)?!d:true))
				break;
			if (!w)
				w = 1;
			if (!h)
				h = 1;
			if (!d)
				d = 1;
			datasize = ((w+bw-1)/bw) * ((h+bh-1)/bh) * ((d+bd-1)/bd) * bb;

			if (filedata + datasize > end)
			{
				Con_Printf(CON_WARNING"%s: truncated\n", fname);
				Con_Printf("%s: %#x\n", fname, usage);
				plugfuncs->Free(mips);
				return NULL;	//doesn't fit...
			}
			mips->mip[l].width = w;
			mips->mip[l].height = h;
			mips->mip[l].depth = d;
			mips->mip[l].data = filedata;
			mips->mip[l].datasize = datasize;
			mips->mip[l].needfree = false;
			filedata += datasize;
		}
		if (filedata != end)
		{
			Con_Printf(CON_WARNING"%s: trailing data\n", fname);
			Con_Printf("%s: %#x\n", fname, usage);
			plugfuncs->Free(mips);
			return NULL;	//doesn't fit...
		}
		return mips;
	}
	return NULL;
}
static plugimageloaderfuncs_t iwifuncs =
{
	"InfinityWard Image",
	sizeof(struct pendingtextureinfo),
	false,
	Image_ReadIWIFile,
};
qboolean IWI_Init(void)
{
	imagefuncs = plugfuncs->GetEngineInterface(plugimagefuncs_name, sizeof(*imagefuncs));
	if (!imagefuncs)
		return false;
	return plugfuncs->ExportInterface(plugimageloaderfuncs_name, &iwifuncs, sizeof(iwifuncs));
}
