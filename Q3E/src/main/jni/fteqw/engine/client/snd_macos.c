/*
 
 Copyright (C) 2001-2002       A Nourai
 Copyright (C) 2006            Jacek Piszczek (Mac OSX port)
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 
 See the included (GNU.txt) GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "quakedef.h"
#include "sound.h"
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>

// Jacek:
// coreaudio is poorly documented so I'm not 100% sure the code below
// is correct :(

struct MacOSSound_Private
{
	AudioUnit	gOutputUnit;
	unsigned int readpos;
};

static OSStatus AudioRender(void	*inRefCon, 
	AudioUnitRenderActionFlags	*ioActionFlags, 
	const AudioTimeStamp		*inTimeStamp, 
	UInt32				inBusNumber, 
	UInt32				inNumberFrames, 
	AudioBufferList			*ioData)
{
	soundcardinfo_t *sc = inRefCon;
	struct MacOSSound_Private *pdata = sc->handle;

	int start = pdata->readpos;
	int buffersize = sc->sn.samples * sc->sn.samplebytes;
	int bytes = ioData->mBuffers[0].mDataByteSize;
	int remaining;

	start %= buffersize;
	if (start + bytes > buffersize)
	{
		remaining = bytes;
		bytes = buffersize - start;
		remaining -= bytes;
	}
	else
	{
		remaining = 0;
	}

	memcpy(ioData->mBuffers[0].mData, sc->sn.buffer + start, bytes);
	memcpy((char*)ioData->mBuffers[0].mData+bytes, sc->sn.buffer, remaining);
	
	pdata->readpos += inNumberFrames*sc->sn.numchannels * sc->sn.samplebytes;

	return noErr;
}

static void MacOS_Shutdown(soundcardinfo_t *sc)
{
	struct MacOSSound_Private *pdata = sc->handle;
	sc->handle = NULL;
	if (!pdata)
		return;

	// stop playback
	AudioOutputUnitStop (pdata->gOutputUnit);

	// release the unit
	AudioUnitUninitialize (pdata->gOutputUnit);

	// free the unit
	CloseComponent (pdata->gOutputUnit);

	// free the buffer memory
	Z_Free(sc->sn.buffer);
	Z_Free(pdata);
}

static unsigned int MacOS_GetDMAPos(soundcardinfo_t *sc)
{
	struct MacOSSound_Private *pdata = sc->handle;
	sc->sn.samplepos = pdata->readpos/sc->sn.samplebytes;
	return sc->sn.samplepos;
}

static void MacOS_Submit(soundcardinfo_t *sc)
{
}

static void *MacOS_Lock(soundcardinfo_t *sc, unsigned int *sampidx)
{
	return sc->sn.buffer;
}

static void MacOS_Unlock(soundcardinfo_t *sc, void *buffer)
{
}

static qboolean MacOS_InitCard(soundcardinfo_t *sc, const char *cardname)
{
	ComponentResult err = noErr;

	if (cardname && *cardname)
		return false;	//only the default device will be used for now.

	struct MacOSSound_Private *pdata = Z_Malloc(sizeof(*pdata));
	if (!pdata)
		return FALSE;

	// Open the default output unit
	ComponentDescription desc;
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	Component comp = FindNextComponent(NULL, &desc);
	if (comp == NULL)
	{
		Con_Printf("FindNextComponent failed\n");
		Z_Free(pdata);
		return FALSE;
	}

	err = OpenAComponent(comp, &pdata->gOutputUnit);
	if (comp == NULL)
	{
		Con_Printf("OpenAComponent failed\n");
		Z_Free(pdata);
		return FALSE;
	}

	// Set up a callback function to generate output to the output unit
	AURenderCallbackStruct input;
	input.inputProc = AudioRender;
	input.inputProcRefCon = sc;

	err = AudioUnitSetProperty (	pdata->gOutputUnit, 
					kAudioUnitProperty_SetRenderCallback, 
					kAudioUnitScope_Input,
					0, 
					&input, 
					sizeof(input));
	if (err) 
	{
		Con_Printf("AudioUnitSetProperty failed\n");
		CloseComponent(pdata->gOutputUnit);
		Z_Free(pdata);
		return FALSE;
	}

	// describe our audio data
	AudioStreamBasicDescription streamFormat;
	streamFormat.mSampleRate = sc->sn.speed;
	streamFormat.mFormatID = kAudioFormatLinearPCM;
	streamFormat.mFormatFlags = kAudioFormatFlagsNativeEndian
					| kLinearPCMFormatFlagIsPacked;
					//| kAudioFormatFlagIsNonInterleaved;
	streamFormat.mFramesPerPacket = 1;
	streamFormat.mChannelsPerFrame = 2;
	streamFormat.mBitsPerChannel = 16;
	if (streamFormat.mBitsPerChannel >= 16)
		streamFormat.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
	else
		streamFormat.mFormatFlags |= 0;
		
	streamFormat.mBytesPerFrame = streamFormat.mChannelsPerFrame * (streamFormat.mBitsPerChannel/8);
	streamFormat.mBytesPerPacket = streamFormat.mBytesPerFrame * streamFormat.mFramesPerPacket;

	err = AudioUnitSetProperty (pdata->gOutputUnit,
				kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Input,
				0,
				&streamFormat,
				sizeof(AudioStreamBasicDescription));
	if (err) 
	{
		Con_Printf("AudioUnitSetProperty failed\n");
		CloseComponent(pdata->gOutputUnit);
		Z_Free(pdata);
		return FALSE;
	}

	// set the shm structure
	sc->sn.speed = streamFormat.mSampleRate;
	sc->sn.samplebytes = streamFormat.mBitsPerChannel/8;
	sc->sn.sampleformat = QCF_S16;
	sc->sn.numchannels = streamFormat.mChannelsPerFrame;
	sc->sn.samples = 256 * 1024;
	sc->sn.buffer = Z_Malloc(sc->sn.samples*sc->sn.samplebytes);

	int i;
	for (i = 0; i < sc->sn.samples*sc->sn.samplebytes; i++)
		sc->sn.buffer[i] = rand();

	if (sc->sn.buffer == 0)
	{
		Con_Printf("Malloc failed - cannot allocate sound buffer\n");
		CloseComponent(pdata->gOutputUnit);
		Z_Free(pdata);
		return FALSE;
	}

	// Initialize unit
	err = AudioUnitInitialize(pdata->gOutputUnit);
	if (err) 
	{
		Con_Printf("AudioOutputInitialize failed\n");
		CloseComponent(pdata->gOutputUnit);
		Z_Free(sc->sn.buffer);
		Z_Free(pdata);
		return FALSE;
	}

	// start playing :)
	err = AudioOutputUnitStart (pdata->gOutputUnit);
	if (err) 
	{
		Con_Printf("AudioOutputUnitStart failed\n");
		AudioUnitUninitialize (pdata->gOutputUnit);
		CloseComponent(pdata->gOutputUnit);
		Z_Free(sc->sn.buffer);
		Z_Free(pdata);
		return FALSE;
	}

	sc->handle = pdata;
	sc->Lock = MacOS_Lock;
	sc->Unlock = MacOS_Unlock;
	sc->Submit = MacOS_Submit;
	sc->GetDMAPos = MacOS_GetDMAPos;
	sc->Shutdown = MacOS_Shutdown;

	Con_Printf("Sound initialised\n");
	return TRUE;
}

sounddriver_t MacOS_AudioOutput =
{
	"CoreAudio",
	MacOS_InitCard,
	NULL
};
