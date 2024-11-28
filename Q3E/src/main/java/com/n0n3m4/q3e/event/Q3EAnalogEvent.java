package com.n0n3m4.q3e.event;

import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.karin.KOnceRunnable;

public class Q3EAnalogEvent extends KOnceRunnable
{
    public final boolean down;
    public final float   x;
    public final float   y;

    public Q3EAnalogEvent(boolean down, float x, float y)
    {
        this.down = down;
        this.x = x;
        this.y = y;
    }

    @Override
    protected void Run()
    {
        Q3EJNI.sendAnalog(down ? 1 : 0, x, y);
    }
}
