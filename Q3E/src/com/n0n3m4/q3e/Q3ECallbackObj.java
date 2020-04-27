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
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.support.v4.util.LongSparseArray;
import android.util.Log;

public class Q3ECallbackObj {
	public Q3EAudioTrack mAudioTrack;	
	byte[] mAudioData;
	public static boolean reqThreadrunning=true;	
	public Q3EView vw;
	
	public void setState(int newstate)
	{
		vw.setState(newstate);
	}
	
	public void init(int size)
	{
		if(mAudioTrack != null) return;		
		if ((Q3EUtils.q3ei.isQ3)||(Q3EUtils.q3ei.isRTCW)||(Q3EUtils.q3ei.isQ1)||(Q3EUtils.q3ei.isQ2)) size/=8;		
		
		mAudioData=new byte[size];		
		int sampleFreq = 44100;							
		
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
		
		int bufferSize = Math.max((Q3EUtils.isOuya)?0:3*size,AudioTrack.getMinBufferSize(sampleFreq, AudioFormat.CHANNEL_CONFIGURATION_STEREO, AudioFormat.ENCODING_PCM_16BIT));		
		mAudioTrack = new Q3EAudioTrack(AudioManager.STREAM_MUSIC,sampleFreq,AudioFormat.CHANNEL_CONFIGURATION_STEREO,
		AudioFormat.ENCODING_PCM_16BIT,bufferSize,AudioTrack.MODE_STREAM);
		mAudioTrack.play();
		long sleeptime=(size*1000000000l)/(2*2*sampleFreq);
		ScheduledThreadPoolExecutor stpe=new ScheduledThreadPoolExecutor(5);
		stpe.scheduleAtFixedRate(new Runnable() {			
			@Override
			public void run() {
				if (reqThreadrunning)
				{						
				Q3EJNI.requestAudioData();
				}							
			}
		}, 0, sleeptime, TimeUnit.NANOSECONDS);		
	}
	
	int sync=0;

	public void writeAudio(ByteBuffer audioData, int offset, int len)
	{
		if(mAudioTrack == null)
			return;			
		audioData.position(offset);
		audioData.get(mAudioData, 0, len);
		if (sync++%128==0)
		mAudioTrack.flush();
		mAudioTrack.write(mAudioData, 0, len);
	}
	
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
}

class Q3EAudioTrack extends AudioTrack
{	
	public Q3EAudioTrack(int streamType, int sampleRateInHz,
			int channelConfig, int audioFormat, int bufferSizeInBytes, int mode)
			throws IllegalStateException {
		super(streamType, sampleRateInHz, channelConfig, audioFormat,
				bufferSizeInBytes, mode);
	}
	
	@Override
	public void play() throws IllegalStateException {				
		flush();		
		super.play();		
	}
}