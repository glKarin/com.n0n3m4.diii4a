package com.n0n3m4.q3e;

import android.media.AudioTrack;

public class Q3EAudioTrack extends AudioTrack
{
    public Q3EAudioTrack(int streamType, int sampleRateInHz,
                         int channelConfig, int audioFormat, int bufferSizeInBytes, int mode)
            throws IllegalStateException
    {
        super(streamType, sampleRateInHz, channelConfig, audioFormat,
                bufferSizeInBytes, mode);
    }

    @Override
    public void play() throws IllegalStateException
    {
        flush();
        super.play();
    }
}
