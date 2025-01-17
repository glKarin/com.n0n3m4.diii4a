package com.n0n3m4.q3e.event;

import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.karin.KOnceRunnable;

public class Q3EMotionEvent extends KOnceRunnable
{
    public final float deltax;
    public final float deltay;

    public Q3EMotionEvent(float deltax, float deltay)
    {
        this.deltax = deltax;
        this.deltay = deltay;
    }

    @Override
    protected void Run()
    {
        Q3EJNI.sendMotionEvent(deltax, deltay);
    }
}
