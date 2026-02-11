/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


//I had one at least, back in the day.
//should be fine for dosbox, if nothing else.

//warning: this sound code doesn't seem to cope well with low framerates. the dma buffer is too small.
//4096 bytes 16bit stereo means 1024 samples. so less than 10 fps and the mixer will miss buffer wraps.


#include <quakedef.h>

#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/nearptr.h>

#define SDRVNAME "SoundBlaster"

/*
===============================================================================

BLASTER SUPPORT

===============================================================================
*/

_go32_dpmi_seginfo dma_buffer_memory;
static short *dma_buffer=0;	//realigned pointer
quintptr_t dma_buffer_phys;	//realigned physical address - must be within the first 16mb
static int dma_size;
static	int dma;

static	int dsp_port;
static	int irq;
static	int low_dma;
static	int high_dma;
static	int mixer_port;
static	int mpu401_port;

static int dsp_version;
static int dsp_minor_version;

static int timeconstant=-1;
static int		oldmixervalue;

static int mode_reg;
static int flipflop_reg;
static int disable_reg;
static int clear_reg;

static soundcardinfo_t *becauseglobalssuck;	//just protects against multiple devices being spawned at once.


static void PrintBits (qbyte b)
{
	int	i;
	char	str[9];
	
	for (i=0 ; i<8 ; i++)
		str[i] = '0' + ((b & (1<<(7-i))) > 0);
		
	str[8] = 0;
	Con_Printf ("%s (%i)", str, b);
}

// =======================================================================
// Interprets BLASTER variable
// =======================================================================

static int GetBLASTER(void)
{
	char *BLASTER;
	char *param;

	BLASTER = getenv("BLASTER");
	if (!BLASTER)
		return 0;

	param = strchr(BLASTER, 'A');
	if (!param)
		param = strchr(BLASTER, 'a');
	if (!param)
		return 0;
	sscanf(param+1, "%x", &dsp_port);

	param = strchr(BLASTER, 'I');
	if (!param)
		param = strchr(BLASTER, 'i');
	if (!param)
		return 0;
	sscanf(param+1, "%d", &irq);

	param = strchr(BLASTER, 'D');
	if (!param)
		param = strchr(BLASTER, 'd');
	if (!param)
		return 0;
	sscanf(param+1, "%d", &low_dma);

	param = strchr(BLASTER, 'H');
	if (!param)
		param = strchr(BLASTER, 'h');
	if (param)
		sscanf(param+1, "%d", &high_dma);

	param = strchr(BLASTER, 'M');
	if (!param)
		param = strchr(BLASTER, 'm');
	if (param)
		sscanf(param+1, "%x", &mixer_port);
	else
		mixer_port = dsp_port;

	param = strchr(BLASTER, 'P');
	if (!param)
		param = strchr(BLASTER, 'p');
	if (param)
		sscanf(param+1, "%x", &mpu401_port);

	return 1;

}

// ==================================================================
// Resets DSP.  Returns 0 on success.
// ==================================================================

static int ResetDSP(void)
{
	volatile int i;

	outportb(dsp_port + 6, 1);
	for (i=65536 ; i ; i--) ;
	outportb(dsp_port + 6, 0);
	for (i=65536 ; i ; i--)
	{
		if (!(inportb(dsp_port + 0xe) & 0x80)) continue;
		if (inportb(dsp_port + 0xa) == 0xaa) break;
	}
	if (i) return 0;
	else return 1;

}

static int ReadDSP(void)
{
	while (!(inportb(dsp_port+0xe)&0x80)) ;
	return inportb(dsp_port+0xa);
}

static void WriteDSP(int val)
{
	while ((inportb(dsp_port+0xc)&0x80)) ;
	outportb(dsp_port+0xc, val);
}

static int ReadMixer(int addr)
{
	outportb(mixer_port+4, addr);
	return inportb(mixer_port+5);
}

static void WriteMixer(int addr, int val)
{
	outportb(mixer_port+4, addr);
	outportb(mixer_port+5, val);
}

/*
================
StartSB

================
*/
static void StartSB(soundcardinfo_t *sc)
{
	int		i;

// version 4.xx startup code
	if (dsp_version >= 4)
	{
		Con_Printf("Version 4 SB startup\n");
		WriteDSP(0xd1); // turn on speaker

		WriteDSP(0x41);

		WriteDSP(sc->sn.speed>>8);
		WriteDSP(sc->sn.speed&0xff);

		WriteDSP(0xb6);	// 16-bit output
		WriteDSP(0x30);	// stereo
		WriteDSP((sc->sn.samples-1) & 0xff);	// # of samples - 1
		WriteDSP((sc->sn.samples-1) >> 8);
	}
// version 3.xx startup code
	else if (dsp_version == 3)
	{
		Con_Printf("Version 3 SB startup\n");
		WriteDSP(0xd1); // turn on speaker

		oldmixervalue = ReadMixer (0xe);
		WriteMixer (0xe, oldmixervalue | 0x2);// turn on stereo

		WriteDSP(0x14);			// send one byte
		WriteDSP(0x0);
		WriteDSP(0x0);

		for (i=0 ; i<0x10000 ; i++)
			inportb(dsp_port+0xe);		// ack the dsp
		
		timeconstant = 65536-(256000000/(sc->sn.numchannels*sc->sn.speed));
		WriteDSP(0x40);
		WriteDSP(timeconstant>>8);

		WriteMixer (0xe, ReadMixer(0xe) | 0x20);// turn off filter

		WriteDSP(0x48);
		WriteDSP((sc->sn.samples-1) & 0xff);	// # of samples - 1
		WriteDSP((sc->sn.samples-1) >> 8);

		WriteDSP(0x90); // high speed 8 bit stereo
	}
// normal speed mono
	else
	{
		Con_Printf("Version 2 SB startup\n");
		WriteDSP(0xd1); // turn on speaker

		timeconstant = 65536-(256000000/(sc->sn.numchannels*sc->sn.speed));
		WriteDSP(0x40);
		WriteDSP(timeconstant>>8);

		WriteDSP(0x48);
		WriteDSP((sc->sn.samples-1) & 0xff);	// # of samples - 1
		WriteDSP((sc->sn.samples-1) >> 8);

		WriteDSP(0x1c); // normal speed 8 bit mono
	}
}

static const int page_reg[] = { 0x87, 0x83, 0x81, 0x82, 0x8f, 0x8b, 0x89, 0x8a };
static const int addr_reg[] = { 0, 2, 4, 6, 0xc0, 0xc4, 0xc8, 0xcc };
static const int count_reg[] = { 1, 3, 5, 7, 0xc2, 0xc6, 0xca, 0xce };

/*
================
StartDMA

================
*/
static void StartDMA(void)
{
	int mode;

// use a high dma channel if specified
	if (high_dma && dsp_version >= 4)	// 8 bit snd can never use 16 bit dma
		dma = high_dma;
	else
		dma = low_dma;

	Con_Printf ("Using DMA channel %i\n", dma);

	if (dma > 3)
	{
		mode_reg = 0xd6;
		flipflop_reg = 0xd8;
		disable_reg = 0xd4;
		clear_reg = 0xdc;
	}
	else
	{
		mode_reg = 0xb;
		flipflop_reg = 0xc;
		disable_reg = 0xa;
		clear_reg = 0xe;
	}

	outportb(disable_reg, dma|4);	// disable channel
	// set mode- see "undocumented pc", p.876
	mode =	(1<<6)	// single-cycle
		+(0<<5)		// address increment
		+(1<<4)		// auto-init dma
		+(2<<2)		// read
		+(dma&3);	// channel #
	outportb(mode_reg, mode);
	
// set address
	// set page
	outportb(page_reg[dma], dma_buffer_phys >> 16);

	if (dma > 3)
	{	// address is in words
		outportb(flipflop_reg, 0);		// prepare to send 16-bit value
		outportb(addr_reg[dma], (dma_buffer_phys>>1) & 0xff);
		outportb(addr_reg[dma], (dma_buffer_phys>>9) & 0xff);

		outportb(flipflop_reg, 0);		// prepare to send 16-bit value
		outportb(count_reg[dma], ((dma_size>>1)-1) & 0xff);
		outportb(count_reg[dma], ((dma_size>>1)-1) >> 8);
	}
	else
	{	// address is in bytes
		outportb(flipflop_reg, 0);		// prepare to send 16-bit value
		outportb(addr_reg[dma], dma_buffer_phys & 0xff);
		outportb(addr_reg[dma], (dma_buffer_phys>>8) & 0xff);

		outportb(flipflop_reg, 0);		// prepare to send 16-bit value
		outportb(count_reg[dma], (dma_size-1) & 0xff);
		outportb(count_reg[dma], (dma_size-1) >> 8);
	}

	outportb(clear_reg, 0);		// clear write mask
	outportb(disable_reg, dma&~4);
}

/*
==============
BLASTER_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
static unsigned int SBLASTER_GetDMAPos(soundcardinfo_t *sc)
{
	int count;

// this function is called often.  acknowledge the transfer completions
// all the time so that it loops
	if (dsp_version >= 4)
		inportb(dsp_port+0xf);	// 16 bit audio
	else
		inportb(dsp_port+0xe);	// 8 bit audio

// clear 16-bit reg flip-flop
// load the current dma count register
	if (dma < 4)
	{
		outportb(0xc, 0);
		count = inportb(dma*2+1);
		count += inportb(dma*2+1) << 8;
		if (sc->sn.samplebytes == 2)
			count /= 2;
		count = sc->sn.samples - (count+1);
	}
	else
	{
		outportb(0xd8, 0);
		count = inportb(0xc0+(dma-4)*4+2);
		count += inportb(0xc0+(dma-4)*4+2) << 8;
		if (sc->sn.samplebytes == 1)
			count *= 2;
		count = sc->sn.samples - (count+1);
	}

//	Con_Printf("DMA pos = 0x%x\n", count);

//	sc->sn.samplepos = count & (sc->sn.samples-1);
	return count;

}

/*
==============
BLASTER_Shutdown

Reset the sound device for exiting
===============
*/
static void SBLASTER_Shutdown(soundcardinfo_t *sc)
{
	if (becauseglobalssuck == sc)
		becauseglobalssuck = NULL;

	if (dsp_version >= 4)
	{
	}
	else if (dsp_version == 3)
	{
		ResetDSP ();			// stop high speed mode
		WriteMixer (0xe, oldmixervalue); // turn stereo off and filter on
	}
	else
	{
	
	}
	
	WriteDSP(0xd3); // turn off speaker
	ResetDSP ();

	outportb(disable_reg, dma|4);	// disable dma channel

	_go32_dpmi_free_dos_memory(&dma_buffer_memory);
}

//simple ring buffer
static void *SBLASTER_LockBuffer(soundcardinfo_t *sc, unsigned int *sampidx)
{
	return sc->sn.buffer;
}

//that's permanently locked
static void SBLASTER_UnlockBuffer(soundcardinfo_t *sc, void *buffer)
{
}

//that the hardware has direct access to.
static void SBLASTER_Submit (soundcardinfo_t *sc, int start, int end)
{
}

//returns the address of some memory.
//ctx is required to free the memory afterwards
static qboolean dosmem_alloc(_go32_dpmi_seginfo *ctx, size_t size)
{
	ctx->size = (size+15)>>4;
	if (_go32_dpmi_allocate_dos_memory(ctx))
		return false;	//failed
	return true;
}

static quintptr_t dosmem_phys(_go32_dpmi_seginfo *ctx)
{
	return ctx->rm_segment<<4;
}
static void *dosmem_ptr(_go32_dpmi_seginfo *ctx)
{
	__djgpp_nearptr_enable();
	return (void*)(__djgpp_conventional_base+dosmem_phys(ctx));
}


/*
==================
BLASTER_Init

Returns false if nothing is found.
==================
*/
static qboolean SBLASTER_InitCard(soundcardinfo_t *sc, const char *pcmname)
{
	int 	size;
	int		p;

	if (becauseglobalssuck)
		return 0;

//
// must have a blaster variable set
//
	if (!GetBLASTER())
	{
		Con_NotifyBox (
		"The BLASTER environment variable\n"
		"is not set, sound effects are\n"
		"disabled.  See README.TXT for help.\n"
		);			
		return 0;
	}

	if (ResetDSP())
	{
		Con_Printf("Could not reset SB");
		return 0;
	}

//
// get dsp version
//
	WriteDSP(0xe1);
	dsp_version = ReadDSP();
	dsp_minor_version = ReadDSP();

// we need at least v2 for auto-init dma
	if (dsp_version < 2)
	{
		Con_Printf ("Sound blaster must be at least v2.0\n");
		return 0;
	}

// allow command line parm to set quality down
	p = COM_CheckParm ("-dsp");
	if (p && p < com_argc - 1)
	{
		p = Q_atoi (com_argv[p+1]);
		if (p < 2 || p > 4)
			Con_Printf ("-dsp parameter can only be 2, 3, or 4\n");
		else if (p > dsp_version)
			Con_Printf ("Can't -dsp %i on v%i hardware\n", p, dsp_version);
		else
			dsp_version = p;
	}	


// everyone does 11khz sampling rate unless told otherwise
//	sc->sn.speed = 11025;
//	rc = COM_CheckParm("-sspeed");
//	if (rc)
//		sc->sn.speed = Q_atoi(com_argv[rc+1]);

// version 4 cards (sb 16) do 16 bit stereo
	if (dsp_version >= 4)
	{
		if (sc->sn.numchannels != 1)
			sc->sn.numchannels = 2;
		if (sc->sn.samplebytes != 1)
			sc->sn.samplebytes = 2;
	}
// version 3 cards (sb pro) do 8 bit stereo
	else if (dsp_version == 3)
	{
		if (sc->sn.numchannels != 1)
			sc->sn.numchannels = 2;
		sc->sn.samplebytes = 1;	
	}
// v2 cards do 8 bit mono
	else
	{
		sc->sn.numchannels = 1;
		sc->sn.samplebytes = 1;
	}

	if (sc->sn.samplebytes == 2)
		sc->sn.sampleformat = QSF_S16;
	else
		sc->sn.sampleformat = QSF_U8;

	sc->Lock		= SBLASTER_LockBuffer;
	sc->Unlock		= SBLASTER_UnlockBuffer;
	sc->Shutdown	= SBLASTER_Shutdown;
	sc->GetDMAPos	= SBLASTER_GetDMAPos;
	sc->Submit		= SBLASTER_Submit;	

	size = 4096;

// allocate 8k and get a 4k-aligned buffer from it
	if (!dosmem_alloc(&dma_buffer_memory, size*2))
	{
		Con_Printf("Couldn't allocate sound dma buffer");
		return false;
	}
	dma_buffer_phys = ((dosmem_phys(&dma_buffer_memory) + size) & ~(size-1));
	dma_buffer = (short *)((qbyte*)dosmem_ptr(&dma_buffer_memory) + dma_buffer_phys-dosmem_phys(&dma_buffer_memory));

	dma_size = size;
	memset(dma_buffer, 0, dma_size);

	sc->sn.samples = size/sc->sn.samplebytes;
	sc->sn.samplepos = 0;
	sc->sn.buffer = (unsigned char *) dma_buffer;
	sc->sn.samples = size/sc->sn.samplebytes;

	StartDMA();
	StartSB(sc);

	becauseglobalssuck = sc;

	return true;
}




static qboolean QDECL SBLASTER_Enumerate(void (QDECL *cb) (const char *drivername, const char *devicecode, const char *readablename))
{
	return false;
}

sounddriver_t SBLASTER_Output =
{
	SDRVNAME,
	SBLASTER_InitCard,
	SBLASTER_Enumerate
};

