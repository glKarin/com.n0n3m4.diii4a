package com.n0n3m4.q3e.event;

import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.karin.KOnceRunnable;

public class Q3EMouseEvent extends KOnceRunnable
{
    public final float x;
    public final float y;
    public final boolean relativeMode;

    public Q3EMouseEvent(float x, float y, boolean relativeMode)
    {
        this.x = x;
        this.y = y;
        this.relativeMode = relativeMode;
    }

    @Override
    protected void Run()
    {
        Q3EJNI.sendMouseEvent(x, y, relativeMode ? 1 : 0);
    }
}
