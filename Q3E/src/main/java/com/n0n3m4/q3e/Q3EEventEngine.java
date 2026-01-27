package com.n0n3m4.q3e;

import android.view.MotionEvent;

import com.n0n3m4.q3e.event.Q3EAnalogEvent;
import com.n0n3m4.q3e.event.Q3EKeyEvent;
import com.n0n3m4.q3e.event.Q3EMotionEvent;
import com.n0n3m4.q3e.event.Q3EMouseEvent;
import com.n0n3m4.q3e.sdl.Q3ESDL;

public interface Q3EEventEngine
{
	public void SendKeyEvent(boolean down, int keycode, int charcode);
	public void SendMotionEvent(float deltax, float deltay);
	public void SendAnalogEvent(boolean down, float x, float y);
	public void SendMouseEvent(float x, float y, boolean relativeMode);
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
	public void SendMouseEvent(float x, float y, boolean relativeMode)
	{
		Q3EJNI.PushMouseEvent(x, y, relativeMode ? 1 : 0);
	}

	@Override
	public void SendTextEvent(String text)
	{
		//Q3EJNI.PushTextEvent(text);
	}

	@Override
	public void SendCharEvent(int ch)
	{
		//Q3EJNI.PushCharEvent(ch);
	}

	@Override
	public void SendWheelEvent(float xaxis, float yaxis)
	{
		//Q3EJNI.PushWheelEvent(xaxis, yaxis);
	}
}

class Q3EEventEngineJava implements Q3EEventEngine
{
	@Override
	public void SendKeyEvent(boolean down, int keycode, int charcode)
	{
		Q3E.callbackObj.PushEvent(new Q3EKeyEvent(down, keycode, charcode));
	}

	@Override
	public void SendMotionEvent(float deltax, float deltay)
	{
		Q3E.callbackObj.PushEvent(new Q3EMotionEvent(deltax, deltay));
	}

	@Override
	public void SendAnalogEvent(boolean down, float x, float y)
	{
		Q3E.callbackObj.PushEvent(new Q3EAnalogEvent(down, x, y));
	}

	@Override
	public void SendMouseEvent(float x, float y, boolean relativeMode)
	{
		Q3E.callbackObj.PushEvent(new Q3EMouseEvent(x, y, relativeMode));
	}

	@Override
	public void SendTextEvent(String text)
	{
		//Q3E.callbackObj.PushEvent(new Q3ETextEvent(text));
	}

	@Override
	public void SendCharEvent(int ch)
	{
		//Q3E.callbackObj.PushEvent(new Q3ECharEvent(ch));
	}

	@Override
	public void SendWheelEvent(float x, float y)
	{
		//Q3E.callbackObj.PushEvent(new Q3EWheelEvent(x, y));
	}
}

abstract class Q3EEventEngineVirtualMouse implements Q3EEventEngine
{
	@Override
	public void SendMotionEvent(float deltax, float deltay)
	{
		Q3E.virtualMouse.Motion(deltax, deltay);
	}

	@Override
	public void SendMouseEvent(float x, float y, boolean relativeMode)
	{
		Q3E.virtualMouse.SetAbsPosition(x, y);
	}
}

class Q3EEventEngineSDL extends Q3EEventEngineVirtualMouse
{
	private final Q3EEventEngine engine;

	public Q3EEventEngineSDL(Q3EEventEngine engine)
	{
		this.engine = engine;
	}

	@Override
	public void SendKeyEvent(boolean down, int keycode, int charcode)
	{
		if(keycode < 0)
		{
			Q3ESDL.onNativeMouse(-keycode, down ? MotionEvent.ACTION_DOWN : MotionEvent.ACTION_UP, Q3E.virtualMouse.GetX(), Q3E.virtualMouse.GetY(), Q3E.virtualMouse.IsRelativeMode());
		}
		else
		{
			if(down)
			{
				Q3ESDL.onNativeKeyDown(keycode);
				if(charcode > 0)
				{
					String text = "" + (char)charcode;
					Q3ESDL.nativeCommitText(text, 1);
				}
			}
			else
			{
				Q3ESDL.onNativeKeyUp(keycode);
			}
		}
	}

	@Override
	public void SendMotionEvent(float deltax, float deltay)
	{
		super.SendMotionEvent(deltax, deltay);
		Q3ESDL.onNativeMouse(0, MotionEvent.ACTION_MOVE, Q3E.virtualMouse.GetX(), Q3E.virtualMouse.GetY(), Q3E.virtualMouse.IsRelativeMode());
	}

	@Override
	public void SendAnalogEvent(boolean down, float x, float y)
	{
		engine.SendAnalogEvent(down, x, y);
	}

	@Override
	public void SendMouseEvent(float x, float y, boolean relativeMode)
	{
		super.SendMouseEvent(x, y, relativeMode);
		Q3ESDL.onNativeMouse(0, MotionEvent.ACTION_MOVE, Q3E.virtualMouse.GetX(), Q3E.virtualMouse.GetY(), Q3E.virtualMouse.IsRelativeMode());
	}

	@Override
	public void SendTextEvent(String text)
	{
		Q3ESDL.nativeCommitText(text, 0);
	}

	@Override
	public void SendCharEvent(int ch)
	{
		String text = "" + (char)ch;
		Q3ESDL.nativeCommitText(text, 1);
	}

	@Override
	public void SendWheelEvent(float xaxis, float yaxis)
	{
		Q3ESDL.onNativeMouse(0, MotionEvent.ACTION_SCROLL, xaxis, yaxis, false);
	}
}

class Q3EEventEngineSam extends Q3EEventEngineVirtualMouse
{
	private final Q3EEventEngine engine;

	public Q3EEventEngineSam(Q3EEventEngine engine)
	{
		this.engine = engine;
	}

	@Override
	public void SendKeyEvent(boolean down, int keycode, int charcode)
	{
		engine.SendKeyEvent(down, keycode, charcode);
	}

	@Override
	public void SendMotionEvent(float deltax, float deltay)
	{
		super.SendMotionEvent(deltax, deltay);
		engine.SendMotionEvent(deltax, deltay);
		engine.SendMouseEvent(Q3E.virtualMouse.GetX(), Q3E.virtualMouse.GetY(), Q3E.virtualMouse.IsRelativeMode());
	}

	@Override
	public void SendAnalogEvent(boolean down, float x, float y)
	{
		engine.SendAnalogEvent(down, x, y);
	}

	@Override
	public void SendMouseEvent(float x, float y, boolean relativeMode)
	{
		super.SendMouseEvent(x, y, relativeMode);
		engine.SendMouseEvent(Q3E.virtualMouse.GetX(), Q3E.virtualMouse.GetY(), Q3E.virtualMouse.IsRelativeMode());
	}

	@Override
	public void SendTextEvent(String text)
	{
	}

	@Override
	public void SendCharEvent(int ch)
	{
	}

	@Override
	public void SendWheelEvent(float xaxis, float yaxis)
	{
	}
}
