/*
	Copyright (C) 2012 n0n3m4
	
    This file is part of Q3E.

    Q3E is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Q3E is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Q3E.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.n0n3m4.q3e;

import java.nio.ByteBuffer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.HandlerThread;
import java.util.LinkedList;

public class Q3ECallbackObj {
	public Q3EAudioTrack mAudioTrack;	
	byte[] mAudioData;
	public static boolean reqThreadrunning=true;	
	public Q3EControlView vw;
	public int state = Q3EGlobals.STATE_NONE;

    private final LinkedList<Runnable> m_eventQueue = new LinkedList<>();
    public boolean notinmenu = true;
    public boolean inLoading = true;
    public boolean inConsole = false;
	
	public void setState(int newstate)
	{
        state = newstate;
        inConsole = (newstate & Q3EGlobals.STATE_CONSOLE) == Q3EGlobals.STATE_CONSOLE;
        notinmenu = ((newstate & Q3EGlobals.STATE_GAME) == Q3EGlobals.STATE_GAME) && !inConsole;
        inLoading = (newstate & Q3EGlobals.STATE_LOADING) == Q3EGlobals.STATE_LOADING;
		vw.setState(newstate);
	}
	
	int sync=0;
	
	public void pause()
	{
		if(mAudioTrack == null)
			return;				
		mAudioTrack.pause();	
		reqThreadrunning=false;
	}
	
	public void resume()
	{
		if(mAudioTrack == null)
			return;				
		mAudioTrack.play();	
		reqThreadrunning=true;
	}

    private static final String TAG = "Q3EAudio";
    private HandlerThread m_thread;
    private Handler m_handler;
    public void init(int size)
    {
		/*                        _.---"'"""""'`--.._
                             _,.-'                   `-._
                         _,."                            -.
                     .-""   ___...---------.._             `.
                     `---'""                  `-.            `.
                                                 `.            \
                                                   `.           \
                                                     \           \
                                                      .           \
                                                      |            .
                                                      |            |
                                _________             |            |
                          _,.-'"         `"'-.._      :            |
                      _,-'                      `-._.'             |
                   _.'            OUYA               `.             '
        _.-.    _,+......__                           `.          .
      .'    `-"'           `"-.,-""--._                 \        /
     /    ,'                  |    __  \                 \      /
    `   ..                       +"  )  \                 \    /
     `.'  \          ,-"`-..    |       |                  \  /
      / " |        .'       \   '.    _.'                   .'
     |,.."--"""--..|    "    |    `""`.                     |
   ,"               `-._     |        |                     |
 .'                     `-._+         |                     |
/                           `.                        /     |
|    `     '                  |                      /      |
`-.....--.__                  |              |      /       |
   `./ "| / `-.........--.-   '              |    ,'        '
     /| ||        `.'  ,'   .'               |_,-+         /
    / ' '.`.        _,'   ,'     `.          |   '   _,.. /
   /   `.  `"'"'""'"   _,^--------"`.        |    `.'_  _/
  /... _.`:.________,.'              `._,.-..|        "'
 `.__.'                                 `._  /
                                           "' */

        if(mAudioTrack != null) return;     
        if ((Q3EUtils.q3ei.isQ3)||(Q3EUtils.q3ei.isRTCW)||(Q3EUtils.q3ei.isQ1)||(Q3EUtils.q3ei.isQ2)) size/=8;      

        mAudioData=new byte[size];      
        int sampleFreq = 44100;                     
        int minBufferSize = AudioTrack.getMinBufferSize(sampleFreq, AudioFormat.CHANNEL_CONFIGURATION_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        int bufferSize = Math.max((Q3EUtils.isOuya) ? 0 : 3 * size, minBufferSize);       
        //k bufferSize = size; //k set original buffer size
        bufferSize = Math.max(size, minBufferSize);
        //k Log.e(TAG, "" + size + " - " + minBufferSize);
        mAudioTrack = new Q3EAudioTrack(AudioManager.STREAM_MUSIC,sampleFreq,AudioFormat.CHANNEL_CONFIGURATION_STEREO,
                                        AudioFormat.ENCODING_PCM_16BIT,bufferSize,AudioTrack.MODE_STREAM);
        mAudioTrack.play();
        
        m_thread = new HandlerThread(TAG);
        m_thread.start();
        m_handler = new Handler(m_thread.getLooper());
	}
    
//k NEW: 
// Now call directly by native of libdante.so, and don't need call requestAudioData in Java.
// if offset >= 0 and length > 0, only write.
// if offset >= 0 and length < 0, length = -length, then write and flush.
// If length == 0 and offset < 0, only flush.
    public int writeAudio(ByteBuffer audioData, int offset, int len)
    {
        if(!IsInited())
            return 0;
        //Log.e(TAG, "write audio " + offset + "  " + len );         
        if (!reqThreadrunning)
            return 0;
		if(offset >= 0 && len == 0)
			return 0;  
		AudioOptRunnable runnable = new AudioOptRunnable(audioData, offset, len);
        m_handler.post(runnable);
        //Log.e(TAG, m_queueCursor + " " + runnable + "  " + runnable.m_flush);
        return runnable.Length();
	}
    
    private boolean IsInited()
    {
        return mAudioTrack != null && m_handler != null;
    }
    
    public void OnDestroy()
    {
        if(m_handler != null)
            m_handler = null;
        if(m_thread != null)
        {
            m_thread.quit();
            m_thread = null;
        }
        if(mAudioTrack != null)
        {
            mAudioTrack.release();
            mAudioTrack = null;
        }
    }

    // It is will run on GLThread. Call by DOOM from JNI, for some MessageBox in game.
    public void PullEvent(boolean execCmd)
    {
        synchronized(m_eventQueue) {
            if(execCmd)
            {
                while(!m_eventQueue.isEmpty())
                    m_eventQueue.removeFirst().run();
            }
            else
                m_eventQueue.clear();
        }
    }

    public void PushEvent(Runnable r)
    {
        synchronized(m_eventQueue) {
            m_eventQueue.add(r);
        }
    }
    
    private class AudioOptRunnable implements Runnable
    {
        private byte[] m_data = null;
		private int m_length = 0;
		private boolean m_flush = false;
        
        public AudioOptRunnable(ByteBuffer audioData, int offset, int len)
        {
            if(offset < 0)
                m_flush = true;
            else
            {
                if(len < 0)
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

		public int Length()
		{
			return m_length;
		}
        
        @Override
        public void run()
        {
            if(mAudioTrack != null)
			{
                if(m_length > 0)
                    mAudioTrack.write(m_data, 0, m_length);
                if(m_flush)
                    mAudioTrack.flush();
			}
            m_data = null;
        }
    }
}

