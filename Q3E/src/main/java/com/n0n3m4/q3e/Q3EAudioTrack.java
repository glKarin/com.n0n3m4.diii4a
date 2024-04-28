package com.n0n3m4.q3e;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.HandlerThread;

import java.nio.ByteBuffer;
import java.util.Arrays;

public class Q3EAudioTrack extends AudioTrack
{
    private static final String TAG = "Q3EAudio";
    private HandlerThread m_thread;
    private Handler m_handler;
    private boolean m_inited = false;
    //k byte[] mAudioData;

    private Q3EAudioTrack(int streamType, int sampleRateInHz, int channelConfig, int audioFormat, int bufferSizeInBytes, int mode) throws IllegalStateException
    {
        super(streamType, sampleRateInHz, channelConfig, audioFormat,
                bufferSizeInBytes, mode);
    }

    private void Init(int size)
    {
        if (m_inited)
            return;

        m_thread = new HandlerThread(TAG);
        m_thread.start();
        m_handler = new Handler(m_thread.getLooper());
        //k mAudioData = new byte[size];

        m_inited = true;
    }

    @Override
    public void play() throws IllegalStateException
    {
        flush();
        super.play();
    }

    public void Pause()
    {
        if (!m_inited)
            return;
        pause();
    }

    public void Resume()
    {
        if (!m_inited)
            return;
        play();
    }

    public static Q3EAudioTrack Instance(int size)
    {
        if ((Q3EUtils.q3ei.isQ3) || (Q3EUtils.q3ei.isRTCW) || (Q3EUtils.q3ei.isQ1) || (Q3EUtils.q3ei.isQ2))
            size /= 8;

        int sampleFreq = 44100;
        int minBufferSize = AudioTrack.getMinBufferSize(sampleFreq, AudioFormat.CHANNEL_CONFIGURATION_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        int bufferSize = Math.max((Q3EUtils.isOuya) ? 0 : 3 * size, minBufferSize);
        //k bufferSize = size; //k set original buffer size
        bufferSize = Math.max(size, minBufferSize);
        //k Log.e(TAG, "" + size + " - " + minBufferSize);
        Q3EAudioTrack mAudioTrack = new Q3EAudioTrack(AudioManager.STREAM_MUSIC, sampleFreq, AudioFormat.CHANNEL_CONFIGURATION_STEREO,
                AudioFormat.ENCODING_PCM_16BIT, bufferSize, AudioTrack.MODE_STREAM);

        mAudioTrack.Init(size);
        mAudioTrack.play();

        return mAudioTrack;
    }

    private boolean IsInited()
    {
        return m_inited/* && m_handler != null*/;
    }

    public int writeAudio(ByteBuffer audioData, int offset, int len)
    {
        //Log.e(TAG, "write audio " + offset + "  " + len );
        if (!m_inited || (offset >= 0 && len == 0))
            return 0;
        AudioOptRunnable runnable = new AudioOptRunnable(audioData, offset, len);
        m_handler.post(runnable);
        return runnable.m_length;
    }

    public int writeAudio(byte[] audioData, int offset, int len)
    {
        //Log.e(TAG, "write audio " + offset + "  " + len );
        if (!m_inited || (offset >= 0 && len == 0))
            return 0;
        AudioOptRunnable runnable = new AudioOptRunnable(audioData, offset, len);
        m_handler.post(runnable);
        return runnable.m_length;
    }

    public synchronized void Shutdown()
    {
        if (!m_inited)
            return;
        m_inited = false;
        if (m_handler != null)
            m_handler = null;
        if (m_thread != null)
        {
            m_thread.quit();
            m_thread = null;
        }
        stop();
        release();
    }

    private class AudioOptRunnable implements Runnable
    {
        private byte[] m_data = null;
        public int m_length = 0;
        private boolean m_flush = false;

        public AudioOptRunnable(ByteBuffer audioData, int offset, int len)
        {
            if (offset < 0)
                m_flush = true;
            else
            {
                if (len < 0)
                {
                    m_flush = true;
                    len = -len;
                }
                audioData.position(offset);
                m_data = new byte[len];
                audioData.get(m_data, 0, len);
                m_length = len;
            }
        }

        public AudioOptRunnable(byte[] audioData, int offset, int len)
        {
            if (offset < 0)
                m_flush = true;
            else
            {
                if (len < 0)
                {
                    m_flush = true;
                    len = -len;
                }
                m_data = new byte[len];
                System.arraycopy(audioData, offset, m_data, 0, len);
                m_length = len;
            }
        }

        @Override
        public void run()
        {
            if (m_inited)
            {
                if (m_length > 0)
                    write(m_data, 0, m_length);
                if (m_flush)
                    flush();
            }
            m_data = null;
        }
    }
}
