package com.n0n3m4.q3e.event;

import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.karin.KOnceRunnable;

public class Q3EWheelEvent extends KOnceRunnable
{
    public final float x;
    public final float y;

    public Q3EWheelEvent(float x, float y)
    {
        this.x = x;
        this.y = y;
    }

    @Override
    protected void Run()
    {
        Q3EJNI.sendWheelEvent(x, y);
    }
}
