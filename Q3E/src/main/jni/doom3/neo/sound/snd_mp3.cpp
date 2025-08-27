
#define MP3DEC_SHORT_BUFFER_SIZE MINIMP3_MAX_SAMPLES_PER_FRAME
#define MP3DEC_GET_SAMPLES_BY_MP3DEC_EX_READ 1

#ifdef MP3DEC_GET_SAMPLES_BY_MP3DEC_EX_READ
/*
====================
mp3dec_ex_read_s
 buf: allow NULL
====================
*/
static size_t mp3dec_ex_read_s(mp3dec_ex_t *dec, mp3d_sample_t *buf, size_t samples)
{
    if (!dec)
    {
        dec->last_error = MP3D_E_PARAM;
        return 0;
    }
    mp3dec_frame_info_t frame_info;
    memset(&frame_info, 0, sizeof(frame_info));
    size_t samples_requested = samples;
    while (samples)
    {
        mp3d_sample_t *buf_frame = NULL;
        size_t read_samples = mp3dec_ex_read_frame(dec, &buf_frame, &frame_info, samples);
        if (!read_samples)
        {
            break;
        }
        if(buf)
        {
            memcpy(buf, buf_frame, read_samples * sizeof(mp3d_sample_t));
            buf += read_samples;
        }
        samples -= read_samples;
    }
    return samples_requested - samples;
}
#endif

static const char* my_minimp3_strerror(int mp3decError)
{
    switch(mp3decError)
    {
#define ERRCASE(X) \
		case MP3D_E_ ## X : return #X;
        ERRCASE( PARAM )
        ERRCASE( MEMORY )
        ERRCASE( IOERROR )
        ERRCASE( USER )
        ERRCASE( DECODE )

        default: return "No Error";
#undef ERRCASE
    }
    assert(0 && "unknown minimp3 errorcode!");
    return "Unknown minimp3 Error!";
}

/*
====================
idWaveFile::OpenMP3
====================
*/
int idWaveFile::OpenMP3(const char *strFileName, waveformatex_t *pwfx)
{
    memset( pwfx, 0, sizeof( waveformatex_t ) );

    mhmmio = fileSystem->OpenFileRead( strFileName );
    if ( !mhmmio ) {
        return -1;
    }

    Sys_EnterCriticalSection( CRITICAL_SECTION_ONE );

    int fileSize = mhmmio->Length();
    byte* buf = (byte*)Mem_Alloc( fileSize );

    mhmmio->Read( buf, fileSize );

	mp3dec_ex_t dec;
	int res = mp3dec_ex_open_buf(&dec, buf, fileSize, MP3D_SEEK_TO_SAMPLE | MP3D_ALLOW_MONO_STEREO_TRANSITION);

    if( res < 0 ) {
        Mem_Free( buf );
        Sys_LeaveCriticalSection( CRITICAL_SECTION_ONE );
        common->Warning( "Opening MP3 file '%s' with minimp3 failed: %s\n", strFileName, my_minimp3_strerror(res) );
        fileSystem->CloseFile( mhmmio );
        mhmmio = NULL;
        return -1;
    }

    mfileTime = mhmmio->Timestamp();

    // num frames, not samples of all channels
    int numSamples = 0;

    mp3dec_frame_info_t info;
    memset(&info, 0, sizeof(info));

    int offset = 0;
    while (offset < fileSize) {
		info.frame_bytes = 0;
        // mp3dec_decode_frame return num frames, not samples of all channels
		int frames = mp3dec_decode_frame(&dec.mp3d, buf + offset, fileSize - offset, 0, &info);

        if (frames == 0)
        {
            if(info.frame_bytes == 0) // EOF
            {
                break;
            }
//            else if(info.frame_bytes > 0) // skip ID3 or other extras data, but no samples
//            {
//            }
        }
        else
            numSamples += frames;

        offset += info.frame_bytes;
    }

#ifdef MP3DEC_GET_SAMPLES_BY_MP3DEC_EX_READ //karin: try to get samples with mp3dec_ex_read
    mp3dec_ex_t dec_temp = dec;
    int numSamplesEx = 0;
    while( true ) {
        // mp3dec_ex_read return samples of all channels, not num frames
        int ret = mp3dec_ex_read_s(&dec, NULL, MP3DEC_SHORT_BUFFER_SIZE);
        if ( ret == 0 ) {
            if ( dec.last_error != 0) {
                common->Warning( "Opening MP3 file '%s' with minimp3 mp3dec_ex_read_s() failed: %s\n", strFileName, my_minimp3_strerror(dec.last_error) );
                numSamplesEx = -1;
            }
            break;
        }
        numSamplesEx += ret;
    }
    dec = dec_temp;

    if(numSamplesEx > 0)
    {
        // numSamplesEx is samples of all channels, but we need num frames
        numSamplesEx /= info.channels;
        if(numSamples != numSamplesEx)
        {
            common->Warning( "Opening MP3 file '%s' with minimp3: mp3dec_decode_frame() read %d samples, but mp3dec_ex_read() read %d samples. using mp3dec_ex_read() result\n", strFileName, numSamples, numSamplesEx );
            numSamples = numSamplesEx;
        }
    }
#endif

    if( numSamples == 0 ) {
        common->Warning( "Couldn't get sound length of '%s' with minimp3: %d\n", strFileName, numSamples );
        // TODO:  return -1 etc?
    }

	//printf("mp3 %s %d %d %d, %d %d\n", strFileName, info.hz, info.channels, numSamples, info.layer, info.bitrate_kbps);
    mpwfx.Format.nSamplesPerSec = info.hz;
    mpwfx.Format.nChannels = info.channels;
    mpwfx.Format.wBitsPerSample = sizeof(short) * 8;
    mdwSize = numSamples * info.channels;	// pcm samples * num channels
    mbIsReadingFromMemory = false;

    if ( idSoundSystemLocal::s_realTimeDecoding.GetBool() ) {

        fileSystem->CloseFile( mhmmio );
        mhmmio = NULL;
        Mem_Free( buf );

        mpwfx.Format.wFormatTag = WAVE_FORMAT_TAG_MP3;
        mhmmio = fileSystem->OpenFileRead( strFileName );
        mMemSize = mhmmio->Length();

    } else {

		mp3 = Mem_Alloc(sizeof(mp3dec_ex_t));
		*((mp3dec_ex_t *)mp3) = dec;
        mp3Data = buf;

        mpwfx.Format.wFormatTag = WAVE_FORMAT_TAG_PCM;
        mMemSize = mdwSize * sizeof( short );
    }

    memcpy( pwfx, &mpwfx, sizeof( waveformatex_t ) );

    Sys_LeaveCriticalSection( CRITICAL_SECTION_ONE );

    isMp3 = true;

    return 0;
}

/*
====================
idWaveFile::ReadMP3
 total: total shorts, byte size = total * sizeof(short)
 ret: read shorts/samples(= frames * channel), byte size = ret * sizeof(short)
====================
*/
int idWaveFile::ReadMP3(byte *pBuffer, int dwSizeToRead, int *pdwSizeRead)
{
    int total = dwSizeToRead/sizeof(short);
    short *bufferPtr = (short *)pBuffer;
    mp3dec_ex_t *dec = (mp3dec_ex_t *) mp3;

    do {
#ifdef MINIMP3_FLOAT_OUTPUT
		float fbuffer[2 * MP3DEC_SHORT_BUFFER_SIZE];
        int numShorts = MINIMP3_MIN(MP3DEC_SHORT_BUFFER_SIZE, total);
        int ret = mp3dec_ex_read(dec, fbuffer, numShorts);
#else
        int numShorts = MINIMP3_MIN(MP3DEC_SHORT_BUFFER_SIZE/*/2*/, total);
        // mp3dec_ex_read return samples of all channels, not num frames
        int ret = mp3dec_ex_read(dec, bufferPtr, numShorts);
#endif
        if ( ret == 0 ) {
            if ( dec->last_error != 0) {
				common->Warning( "idWaveFile::ReadMP3() mp3dec_ex_read() %d shorts failed: %s\n", numShorts, my_minimp3_strerror(dec->last_error) );
				return -1;
            }
            break;
        }
#ifdef MINIMP3_FLOAT_OUTPUT
		mp3dec_f32_to_s16(fbuffer, bufferPtr, ret);
#endif
		// ret is samples of all channels
        bufferPtr += ret;
        total -= ret;// / mpwfx.Format.nChannels;
    } while( total > 0 );

    dwSizeToRead = (byte *)bufferPtr - pBuffer;

    if ( pdwSizeRead != NULL ) {
        *pdwSizeRead = dwSizeToRead;
    }

    return dwSizeToRead;
}

/*
====================
idWaveFile::CloseMP3
====================
*/
int idWaveFile::CloseMP3(void)
{
    if ( mp3 != NULL ) {
		mp3dec_ex_t *dec = (mp3dec_ex_t *)mp3;
        Sys_EnterCriticalSection( CRITICAL_SECTION_ONE );
		mp3dec_ex_close(dec);
		Mem_Free(mp3);
        Sys_LeaveCriticalSection( CRITICAL_SECTION_ONE );
        fileSystem->CloseFile( mhmmio );
        mhmmio = NULL;
        mp3 = NULL;
        Mem_Free( mp3Data );
        mp3Data = NULL;
        return 0;
    }
    return -1;
}


/*
====================
idSampleDecoderLocal::DecodeMP3
====================
*/
int idSampleDecoderLocal::DecodeMP3(idSoundSample *sample, int sampleOffset44k, int sampleCount44k, float *dest)
{
	int readSamples, totalSamples;

	int shift = 22050 / sample->objectInfo.nSamplesPerSec;
	int sampleOffset = sampleOffset44k >> shift;
	int sampleCount = sampleCount44k >> shift;

	// open MP3 file if not yet opened
	if (lastSample == NULL) {
		// make sure there is enough space for another decoder
		if (decoderMemoryAllocator.GetFreeBlockMemory() < MIN_OGGVORBIS_MEMORY) {
			return 0;
		}

		if (sample->nonCacheData == NULL) {
            common->Warning( "Called idSampleDecoderLocal::DecodeMP3() on idSoundSample '%s' without nonCacheData\n", sample->name.c_str() );
			failed = true;
			return 0;
		}

        assert(mp3 == NULL);
		mp3dec_ex_t dec;
		int res = mp3dec_ex_open_buf(&dec, sample->nonCacheData, sample->objectMemSize, MP3D_SEEK_TO_SAMPLE | MP3D_ALLOW_MONO_STEREO_TRANSITION);
        if ( res < 0 ) {
            common->Warning( "idSampleDecoderLocal::DecodeMP3() mp3dec_ex_open_buf() for %s failed: %s\n",
                             sample->name.c_str(), my_minimp3_strerror(res) );
            failed = true;
            return 0;
        }

		lastFormat = WAVE_FORMAT_TAG_MP3;
		lastSample = sample;
		mp3 = (mp3dec_ex_t *)Mem_Alloc(sizeof(*mp3));
		*mp3 = dec;
	}

	// seek to the right offset if necessary
	if (sampleOffset != lastSampleOffset) {
        if ( mp3dec_ex_seek( mp3, sampleOffset / sample->objectInfo.nChannels ) < 0 ) {
            int offset = sampleOffset / sample->objectInfo.nChannels;
            common->Warning( "idSampleDecoderLocal::DecodeMP3() mp3dec_ex_seek(%d) for %s failed: %s\n",
                             offset, sample->name.c_str(), my_minimp3_strerror( mp3->last_error ) );
            failed = true;
            return 0;
        }
	}

	lastSampleOffset = sampleOffset;

	// decode MP3 samples
	totalSamples = sampleCount;
	readSamples = 0;

	do {
#ifdef MINIMP3_FLOAT_OUTPUT
        float samplesBuf[2 * MIXBUFFER_SAMPLES];
        int reqSamples = MINIMP3_MIN( MIXBUFFER_SAMPLES, totalSamples);
        int ret = mp3dec_ex_read( mp3, samplesBuf, reqSamples );
#else
        short samplesBuf[2 * MIXBUFFER_SAMPLES];
        int reqSamples = MINIMP3_MIN( MIXBUFFER_SAMPLES, totalSamples);
        // mp3dec_ex_read return samples of all channels, not num frames
        int ret = mp3dec_ex_read( mp3, samplesBuf, reqSamples );
#endif
        if ( reqSamples == 0 ) {
            common->DPrintf( "idSampleDecoderLocal::DecodeMP3() reqSamples == 0\n  for %s ?!\n", sample->name.c_str() );
            readSamples += totalSamples;
            totalSamples = 0;
            break;
        }
        if ( ret == 0 ) {
            if ( mp3->last_error == 0/* && reqSamples < 5 */) {
				readSamples += totalSamples;
				totalSamples = 0;
				break;
            } else {
                common->Warning( "idSampleDecoderLocal::DecodeMP3() mp3dec_ex_read() %d (%d) samples\n  for %s failed: %s\n",
                                 reqSamples, totalSamples, sample->name.c_str(), my_minimp3_strerror( mp3->last_error ) );
                failed = true;
				return 0;
            }
        }

#ifdef MINIMP3_FLOAT_OUTPUT
#if 0
		float b1[MIXBUFFER_SAMPLES] = { 0 };
		float b2[MIXBUFFER_SAMPLES] = { 0 };
		float* samples[2] = { b1, b2 };
		int numFrames = ret;
		for(int i = 0; i < ret; i++)
		{
			b1[i] = samplesBuf[2*i];
			b2[i] = samplesBuf[2*i+1];
		}

		SIMDProcessor->UpSampleOGGTo44kHz(dest + (readSamples << shift), samples, ret, sample->objectInfo.nSamplesPerSec, sample->objectInfo.nChannels);
#else
		short bufferPtr[MIXBUFFER_SAMPLES * 2];
		mp3dec_f32_to_s16(samplesBuf, bufferPtr, ret);

		SIMDProcessor->UpSamplePCMTo44kHz(dest + (readSamples << shift), bufferPtr, ret, sample->objectInfo.nSamplesPerSec, sample->objectInfo.nChannels);
#endif
#else
		SIMDProcessor->UpSamplePCMTo44kHz(dest + (readSamples << shift), samplesBuf, ret, sample->objectInfo.nSamplesPerSec, sample->objectInfo.nChannels);
#endif

		// ret is samples of all channels
		readSamples += ret;
		totalSamples -= ret;
	} while (totalSamples > 0);

	lastSampleOffset += readSamples;

	return (readSamples << shift);
}

