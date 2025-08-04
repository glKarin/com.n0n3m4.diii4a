package com.n0n3m4.q3e;

import com.n0n3m4.q3e.event.Q3EAnalogEvent;
import com.n0n3m4.q3e.event.Q3ECharEvent;
import com.n0n3m4.q3e.event.Q3EKeyEvent;
import com.n0n3m4.q3e.event.Q3EMotionEvent;
import com.n0n3m4.q3e.event.Q3EMouseEvent;
import com.n0n3m4.q3e.event.Q3ETextEvent;

public interface Q3EEventEngine
{
	public void SendKeyEvent(final boolean down, final int keycode, final int charcode);
	public void SendMotionEvent(final float deltax, final float deltay);
	public void SendAnalogEvent(final boolean down, final float x, final float y);
	public void SendMouseEvent(final float x, final float y);

	public void SendTextEvent(final String text);
	public void SendCharEvent(final int ch);
}

class Q3EEventEngineNative implements Q3EEventEngine
{
	@Override
	public void SendKeyEvent(boolean down, int keycode, int charcode)
	{
		Q3EJNI.PushKeyEvent(down ? 1 : 0, keycode, charcode);
	}

	@Override
	public void SendMotionEvent(float deltax, float deltay)
	{
		Q3EJNI.PushMotionEvent(deltax, deltay);
	}

	@Override
	public void SendAnalogEvent(boolean down, float x, float y)
	{
		Q3EJNI.PushAnalogEvent(down ? 1 : 0, x, y);
	}

	@Override
	public void SendMouseEvent(float x, float y)
	{
		Q3EJNI.PushMouseEvent(x, y);
	}

	@Override
	public void SendTextEvent(String text)
	{
		Q3EJNI.PushTextEvent(text);
	}

	@Override
	public void SendCharEvent(int ch)
	{
		Q3EJNI.PushCharEvent(ch);
	}
}

class Q3EEventEngineJava implements Q3EEventEngine
{
	@Override
	public void SendKeyEvent(boolean down, int keycode, int charcode)
	{
/*		Q3EUtils.q3ei.callbackObj.PushEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendKeyEvent(down ? 1 : 0, keycode, charcode);
            }
        });*/
		Q3EUtils.q3ei.callbackObj.PushEvent(new Q3EKeyEvent(down, keycode, charcode));
	}

	@Override
	public void SendMotionEvent(float deltax, float deltay)
	{
/*        Q3EUtils.q3ei.callbackObj.PushEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendMotionEvent(deltax, deltay);
            }
        });*/
		Q3EUtils.q3ei.callbackObj.PushEvent(new Q3EMotionEvent(deltax, deltay));
	}

	@Override
	public void SendAnalogEvent(boolean down, float x, float y)
	{
/*		Q3EUtils.q3ei.callbackObj.PushEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendAnalog(down ? 1 : 0, x, y);
            }
        });*/
		Q3EUtils.q3ei.callbackObj.PushEvent(new Q3EAnalogEvent(down, x, y));
	}

	@Override
	public void SendMouseEvent(float x, float y)
	{
/*        Q3EUtils.q3ei.callbackObj.PushEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendMouseEvent(x, y);
            }
        });*/
		Q3EUtils.q3ei.callbackObj.PushEvent(new Q3EMouseEvent(x, y));
	}

	@Override
	public void SendTextEvent(String text)
	{
/*        Q3EUtils.q3ei.callbackObj.PushEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.Q3ETextEvent(text);
            }
        });*/
		Q3EUtils.q3ei.callbackObj.PushEvent(new Q3ETextEvent(text));
	}

	@Override
	public void SendCharEvent(int ch)
	{
/*        Q3EUtils.q3ei.callbackObj.PushEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.Q3ECharEvent(ch);
            }
        });*/
		Q3EUtils.q3ei.callbackObj.PushEvent(new Q3ECharEvent(ch));
	}
}
