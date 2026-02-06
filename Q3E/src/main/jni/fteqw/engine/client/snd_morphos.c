/*
Copyright (C) 2006-2007 Mark Olsen

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

#include <exec/exec.h>
#include <devices/ahi.h>
#include <proto/exec.h>
#define USE_INLINE_STDARG
#include <proto/ahi.h>

#include "quakedef.h"

#warning Remove this once Spike fixes the sound input code.
void S_UpdateCapture(void)
{
}

struct AHIChannelInfo
{
	struct AHIEffChannelInfo aeci;
	ULONG offset;
};

struct AHIdata
{
	struct MsgPort *msgport;
	struct AHIRequest *ahireq;
	int ahiopen;
	struct AHIAudioCtrl *audioctrl;
	void *samplebuffer;
	struct Hook EffectHook;
	struct AHIChannelInfo aci;
	unsigned int readpos;
};

ULONG EffectFunc()
{
	struct Hook *hook = (struct Hook *)REG_A0;
	struct AHIEffChannelInfo *aeci = (struct AHIEffChannelInfo *)REG_A1;

	struct AHIdata *ad;

	ad = hook->h_Data;

	ad->readpos = aeci->ahieci_Offset[0];

	return 0;
}

static struct EmulLibEntry EffectFunc_Gate =
{
	TRAP_LIB, 0, (void (*)(void))EffectFunc
};

static void AHI_Shutdown(soundcardinfo_t *sc)
{
	struct AHIdata *ad;

	struct Library *AHIBase;

	ad = sc->handle;

	AHIBase = (struct Library *)ad->ahireq->ahir_Std.io_Device;

	ad->aci.aeci.ahie_Effect = AHIET_CHANNELINFO|AHIET_CANCEL;
	AHI_SetEffect(&ad->aci.aeci, ad->audioctrl);
	AHI_ControlAudio(ad->audioctrl,
	                 AHIC_Play, FALSE,
	                 TAG_END);

	AHI_FreeAudio(ad->audioctrl);
	FreeVec(ad->samplebuffer);
	CloseDevice((struct IORequest *)ad->ahireq);
	DeleteIORequest((struct IORequest *)ad->ahireq);
	DeleteMsgPort(ad->msgport);
	FreeVec(ad);
}

static unsigned int AHI_GetDMAPos(soundcardinfo_t *sc)
{
	struct AHIdata *ad;

	ad = sc->handle;

	sc->sn.samplepos = ad->readpos*sc->sn.numchannels;

	return sc->sn.samplepos;
}

static void AHI_UnlockBuffer(soundcardinfo_t *sc, void *buffer)
{
}

static void *AHI_LockBuffer(soundcardinfo_t *sc, unsigned int *sampidx)
{
	return sc->sn.buffer;
}

static void AHI_Submit(soundcardinfo_t *sc)
{
}

static qboolean AHI_InitCard(soundcardinfo_t *sc, const char *cardname)
{
	struct AHIdata *ad;

	ULONG channels;
	ULONG speed;
	ULONG bits;

	ULONG r;

	struct Library *AHIBase;

	struct AHISampleInfo sample;

	if (cardname && *cardname)
		return false; /* only allow the default audio device */

	ad = AllocVec(sizeof(*ad), MEMF_ANY);
	if (ad)
	{
		ad->msgport = CreateMsgPort();
		if (ad->msgport)
		{
			ad->ahireq = (struct AHIRequest *)CreateIORequest(ad->msgport, sizeof(struct AHIRequest));
			if (ad->ahireq)
			{
				ad->ahiopen = !OpenDevice("ahi.device", AHI_NO_UNIT, (struct IORequest *)ad->ahireq, 0);
				if (ad->ahiopen)
				{
					AHIBase = (struct Library *)ad->ahireq->ahir_Std.io_Device;

					ad->audioctrl = AHI_AllocAudio(AHIA_AudioID, AHI_DEFAULT_ID,
					                               AHIA_MixFreq, sc->sn.speed,
					                               AHIA_Channels, 1,
					                               AHIA_Sounds, 1,
					                               TAG_END);

					if (ad->audioctrl)
					{
						AHI_GetAudioAttrs(AHI_INVALID_ID, ad->audioctrl,
						                  AHIDB_BufferLen, sizeof(sc->name),
						                  AHIDB_Name, (ULONG)sc->name,
						                  AHIDB_MaxChannels, (ULONG)&channels,
						                  AHIDB_Bits, (ULONG)&bits,
						                  TAG_END);

						AHI_ControlAudio(ad->audioctrl,
						                 AHIC_MixFreq_Query, (ULONG)&speed,
						                 TAG_END);

						if (bits == 8 || bits == 16)
						{
							if (channels > 2)
								channels = 2;

							sc->sn.speed = speed;
							sc->sn.samplebytes = bits/8;
							sc->sn.numchannels = channels;
							sc->sn.samples = speed*channels;

							ad->samplebuffer = AllocVec(speed*(bits/8)*channels, MEMF_ANY);
							if (ad->samplebuffer)
							{
								sc->sn.buffer = ad->samplebuffer;

								if (channels == 1)
								{
									if (bits == 8)
										sample.ahisi_Type = AHIST_M8S;
									else
										sample.ahisi_Type = AHIST_M16S;
								}
								else
								{
									if (bits == 8)
										sample.ahisi_Type = AHIST_S8S;
									else
										sample.ahisi_Type = AHIST_S16S;
								}
								sc->sn.sampleformat = (bits==8)?QSF_S8:QSF_S16;

								sample.ahisi_Address = ad->samplebuffer;
								sample.ahisi_Length = (speed*(bits/8)*channels)/AHI_SampleFrameSize(sample.ahisi_Type);

								r = AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, &sample, ad->audioctrl);
								if (r == 0)
								{
									r = AHI_ControlAudio(ad->audioctrl,
									                     AHIC_Play, TRUE,
									                     TAG_END);

									if (r == 0)
									{
										AHI_Play(ad->audioctrl,
										         AHIP_BeginChannel, 0,
										         AHIP_Freq, speed,
										         AHIP_Vol, 0x10000,
										         AHIP_Pan, 0x8000,
										         AHIP_Sound, 0,
										         AHIP_EndChannel, NULL,
										         TAG_END);

										ad->aci.aeci.ahie_Effect = AHIET_CHANNELINFO;
										ad->aci.aeci.ahieci_Func = &ad->EffectHook;
										ad->aci.aeci.ahieci_Channels = 1;

										ad->EffectHook.h_Entry = (void *)&EffectFunc_Gate;
										ad->EffectHook.h_Data = ad;

										AHI_SetEffect(&ad->aci, ad->audioctrl);

										sc->handle = ad;

										sc->Lock = AHI_LockBuffer;
										sc->Unlock = AHI_UnlockBuffer;
										sc->Submit = AHI_Submit;
										sc->Shutdown = AHI_Shutdown;
										sc->GetDMAPos = AHI_GetDMAPos;

										Con_Printf("Using AHI mode \"%s\" for audio output\n", sc->name);
										Con_Printf("Channels: %d bits: %d frequency: %d\n", channels, bits, speed);

										return true;
									}
								}
							}
							FreeVec(ad->samplebuffer);
						}
						AHI_FreeAudio(ad->audioctrl);
					}
					else
						Con_Printf("Failed to allocate AHI audio\n");

					CloseDevice((struct IORequest *)ad->ahireq);
				}
				DeleteIORequest((struct IORequest *)ad->ahireq);
			}
			DeleteMsgPort(ad->msgport);
		}
		FreeVec(ad);
	}

	return false;
}

sounddriver_t AHI_AudioOutput =
{
	"AHI",
	AHI_InitCard,
	NULL
};
