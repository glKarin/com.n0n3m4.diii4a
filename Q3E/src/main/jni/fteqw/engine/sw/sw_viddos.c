#include "quakedef.h"
#ifdef SWQUAKE
#include "sw.h"




/* from http://www.delorie.com/djgpp/doc/ug/graphics/vesa.html */
typedef struct VESA_INFO
{
	unsigned char  VESASignature[4];
	unsigned short VESAVersion          __attribute__ ((packed));
	unsigned long  OEMStringPtr         __attribute__ ((packed));
	unsigned char  Capabilities[4];
	unsigned long  VideoModePtr         __attribute__ ((packed));
	unsigned short TotalMemory          __attribute__ ((packed));
	unsigned short OemSoftwareRev       __attribute__ ((packed));
	unsigned long  OemVendorNamePtr     __attribute__ ((packed));
	unsigned long  OemProductNamePtr    __attribute__ ((packed));
	unsigned long  OemProductRevPtr     __attribute__ ((packed));
	unsigned char  Reserved[222];
	unsigned char  OemData[256];
} VESA_INFO;



#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>


static VESA_INFO vesa_info;


static int get_vesa_info()
{
	__dpmi_regs r;
	long dosbuf;
	int c;

	/* use the conventional memory transfer buffer */
	dosbuf = __tb & 0xFFFFF;

	/* initialize the buffer to zero */
	for (c=0; c<sizeof(VESA_INFO); c++)
		_farpokeb(_dos_ds, dosbuf+c, 0);

	dosmemput("VBE2", 4, dosbuf);

	/* call the VESA function */
	r.x.ax = 0x4F00;
	r.x.di = dosbuf & 0xF;
	r.x.es = (dosbuf>>4) & 0xFFFF;
	__dpmi_int(0x10, &r);

	/* quit if there was an error */
	if (r.h.ah)
		return -1;

	/* copy the resulting data into our structure */
	dosmemget(dosbuf, sizeof(VESA_INFO), &vesa_info);

	/* check that we got the right magic marker value */
	if (strncmp(vesa_info.VESASignature, "VESA", 4) != 0)
		return -1;

	/* it worked! */
	return 0;
}


typedef struct MODE_INFO
{
	unsigned short ModeAttributes       __attribute__ ((packed));
	unsigned char  WinAAttributes;
	unsigned char  WinBAttributes;
	unsigned short WinGranularity       __attribute__ ((packed));
	unsigned short WinSize              __attribute__ ((packed));
	unsigned short WinASegment          __attribute__ ((packed));
	unsigned short WinBSegment          __attribute__ ((packed));
	unsigned long  WinFuncPtr           __attribute__ ((packed));
	unsigned short BytesPerScanLine     __attribute__ ((packed));
	unsigned short XResolution          __attribute__ ((packed));
	unsigned short YResolution          __attribute__ ((packed));
	unsigned char  XCharSize;
	unsigned char  YCharSize;
	unsigned char  NumberOfPlanes;
	unsigned char  BitsPerPixel;
	unsigned char  NumberOfBanks;
	unsigned char  MemoryModel;
	unsigned char  BankSize;
	unsigned char  NumberOfImagePages;
	unsigned char  Reserved_page;
	unsigned char  RedMaskSize;
	unsigned char  RedMaskPos;
	unsigned char  GreenMaskSize;
	unsigned char  GreenMaskPos;
	unsigned char  BlueMaskSize;
	unsigned char  BlueMaskPos;
	unsigned char  ReservedMaskSize;
	unsigned char  ReservedMaskPos;
	unsigned char  DirectColorModeInfo;
	unsigned long  PhysBasePtr          __attribute__ ((packed));
	unsigned long  OffScreenMemOffset   __attribute__ ((packed));
	unsigned short OffScreenMemSize     __attribute__ ((packed));
	unsigned char  Reserved[206];
} MODE_INFO;


static MODE_INFO mode_info;


static int get_mode_info(int mode)
{
	__dpmi_regs r;
	long dosbuf;
	int c;

	/* use the conventional memory transfer buffer */
	dosbuf = __tb & 0xFFFFF;

	/* initialize the buffer to zero */
	for (c=0; c<sizeof(MODE_INFO); c++)
		_farpokeb(_dos_ds, dosbuf+c, 0);

	/* call the VESA function */
	r.x.ax = 0x4F01;
	r.x.di = dosbuf & 0xF;
	r.x.es = (dosbuf>>4) & 0xFFFF;
	r.x.cx = mode;
	__dpmi_int(0x10, &r);

	/* quit if there was an error */
	if (r.h.ah)
		return -1;

	/* copy the resulting data into our structure */
	dosmemget(dosbuf, sizeof(MODE_INFO), &mode_info);

	/* it worked! */
	return 0;
}


static int find_vesa_mode(int w, int h, int bpp)
{
	int mode_list[256];
	int number_of_modes;
	long mode_ptr;
	int c;

	/* check that the VESA driver exists, and get information about it */
	if (get_vesa_info() != 0)
		return 0;

	/* convert the mode list pointer from seg:offset to a linear address */
	mode_ptr = ((vesa_info.VideoModePtr & 0xFFFF0000) >> 12) + (vesa_info.VideoModePtr & 0xFFFF);

	number_of_modes = 0;

	/* read the list of available modes */
	while (_farpeekw(_dos_ds, mode_ptr) != 0xFFFF)
	{
		mode_list[number_of_modes] = _farpeekw(_dos_ds, mode_ptr);
		number_of_modes++;
		mode_ptr += 2;
	}

	/* scan through the list of modes looking for the one that we want */
	for (c=0; c<number_of_modes; c++)
	{
		/* get information about this mode */
		if (get_mode_info(mode_list[c]) != 0)
			continue;

		/* check the flags field to make sure this is a color graphics mode,
		* and that it is supported by the current hardware */
		if ((mode_info.ModeAttributes & 0x19) != 0x19)
			continue;

		/* check that this mode is the right size */
		if ((mode_info.XResolution != w) || (mode_info.YResolution != h))
			continue;

		/* check that there is only one color plane */
		if (mode_info.NumberOfPlanes != 1)
			continue;

		/* check that it is a packed-pixel mode (other values are used for
		* different memory layouts, eg. 6 for a truecolor resolution) */
		if (mode_info.MemoryModel != ((bpp==8)?4:6))
			continue;

		/* check that this is an 8-bit (256 color) mode */
		if (mode_info.BitsPerPixel != bpp)
			continue;

		/* if it passed all those checks, this must be the mode we want! */
		return mode_list[c];
	}

	/* oh dear, there was no mode matching the one we wanted! */
	return 0; 
}


static int set_vesa_mode(int w, int h, int bpp)
{
	__dpmi_regs r;
	int mode_number;

	/* find the number for this mode */
	mode_number = find_vesa_mode(w, h, bpp);
	if (!mode_number)
		return -1;

	/* call the VESA mode set function */
	r.x.ax = 0x4F02;
	r.x.bx = mode_number;
	__dpmi_int(0x10, &r);
	if (r.h.ah)
		return -1;

	/* it worked! */
	return 0;
}

void set_vesa_bank(int bank_number)
{
	__dpmi_regs r;

	r.x.ax = 0x4F05;
	r.x.bx = 0;
	r.x.dx = bank_number;
	__dpmi_int(0x10, &r);
}

static void copy_to_vesa_screen(char *memory_buffer, int screen_size)
{
	//FIXME: use OffScreenMemOffset if possible.

	int bank_size = mode_info.WinSize*1024;
	int bank_granularity = mode_info.WinGranularity*1024;
	int bank_number = 0;
	int todo = screen_size;
	int copy_size;

	while (todo > 0)
	{
		/* select the appropriate bank */
		set_vesa_bank(bank_number);

		/* how much can we copy in one go? */
		if (todo > bank_size)
			copy_size = bank_size;
		else
			copy_size = todo;

		/* copy a bank of data to the screen */
		dosmemput(memory_buffer, copy_size, 0xA0000);

		/* move on to the next bank of data */
		todo -= copy_size;
		memory_buffer += copy_size;
		bank_number += bank_size/bank_granularity;
	}
}


extern int nostdout;	//we flag with 0x800 to disable printfs while displaying stuff.
static qboolean videoatexitregistered;
static void videoatexit(void)
{
	if (nostdout & 0x800)
	{
		nostdout &= ~0x800;

		__dpmi_regs r;
		r.x.ax = 0x0000 | 3;
		__dpmi_int(0x10, &r);
	}
}


static unsigned int *backbuffer;
static unsigned int *depthbuffer;
static unsigned int framenumber;

//#define NORENDER

qboolean SW_VID_Init(rendererstate_t *info, unsigned char *palette)
{
	int bpp = info->bpp;
	vid.pixelwidth = info->width;
	vid.pixelheight = info->height;

	if (bpp != 32)
		bpp = 32; //sw renderer supports only this

#ifndef NORENDER
	nostdout |= 0x800;
	if (set_vesa_mode(vid.pixelwidth, vid.pixelheight, bpp) < 0)
		return false;
#endif

	if (!videoatexitregistered)
	{
		videoatexitregistered = true;
		atexit(videoatexit);
	}

	backbuffer = BZ_Malloc(vid.pixelwidth * vid.pixelheight * sizeof(*backbuffer));
	if (!backbuffer)
		return false;
	depthbuffer = BZ_Malloc(vid.pixelwidth * vid.pixelheight * sizeof(*depthbuffer));
	if (!depthbuffer)
		return false;

	return true;
}
void SW_VID_DeInit(void)
{
	BZ_Free(backbuffer);
	backbuffer = NULL;
	BZ_Free(depthbuffer);
	depthbuffer = NULL;
}
qboolean SW_VID_ApplyGammaRamps		(unsigned int rampcount, unsigned short *ramps)
{	//no gamma ramps with VESA.
	return false;
}
char *SW_VID_GetRGBInfo(int *bytestride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt)
{
	void *ret = BZ_Malloc(vid.pixelwidth*vid.pixelheight*4);
	if (!ret)
		return NULL;

	memcpy(ret, backbuffer, vid.pixelwidth*vid.pixelheight*4);
	*bytestride = vid.pixelwidth*4;
	*truevidwidth = vid.pixelwidth;
	*truevidheight = vid.pixelheight;
	*fmt = TF_BGRX32;
	return ret;
}
void SW_VID_SetWindowCaption(const char *msg)
{
}
void SW_VID_SwapBuffers(void)
{
#ifndef NORENDER
	copy_to_vesa_screen((char*)backbuffer, vid.pixelwidth*vid.pixelheight*4);
#endif
	framenumber++;
}
void SW_VID_UpdateViewport(wqcom_t *com)
{
	com->viewport.cbuf = backbuffer + vid.pixelwidth*(vid.pixelheight-1);
	com->viewport.dbuf = depthbuffer;
	com->viewport.width = vid.pixelwidth;
	com->viewport.height = vid.pixelheight;
	com->viewport.stride = -vid.pixelwidth;	//this is in pixels. which is stupid.
	com->viewport.framenum = framenumber;
}

#endif
