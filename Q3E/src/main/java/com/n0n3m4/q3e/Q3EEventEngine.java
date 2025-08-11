package com.n0n3m4.q3e;

import com.n0n3m4.q3e.event.Q3EAnalogEvent;
import com.n0n3m4.q3e.event.Q3ECharEvent;
import com.n0n3m4.q3e.event.Q3EKeyEvent;
import com.n0n3m4.q3e.event.Q3EMotionEvent;
import com.n0n3m4.q3e.event.Q3EMouseEvent;
import com.n0n3m4.q3e.event.Q3ETextEvent;
import com.n0n3m4.q3e.event.Q3EWheelEvent;

public interface Q3EEventEngine
{
	public void SendKeyEvent(boolean down, int keycode, int charcode);
	public void SendMotionEvent(float deltax, float deltay);
	public void SendAnalogEvent(boolean down, float x, float y);
	public void SendMouseEvent(float x, float y);

	public void SendTextEvent(String text);
	public void SendCharEvent(int ch);
	public void SendWheelEvent(float xaxis, float yaxis);
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

	@Override
	public void SendWheelEvent(float xaxis, float yaxis)
	{
		Q3EJNI.PushWheelEvent(xaxis, yaxis);
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

	@Override
	public void SendWheelEvent(float x, float y)
	{
/*        Q3EUtils.q3ei.callbackObj.PushEvent(new KOnceRunnable()
        {
            @Override
            public void Run()
            {
                Q3EJNI.sendWheelEvent(x, y);
            }
        });*/
		Q3EUtils.q3ei.callbackObj.PushEvent(new Q3EWheelEvent(x, y));
	}
}
