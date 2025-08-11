package com.n0n3m4.q3e.event;

import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.karin.KOnceRunnable;

public class Q3ECharEvent extends KOnceRunnable
{
    public final int ch;

    public Q3ECharEvent(int ch)
    {
        this.ch = ch;
    }

    @Override
    protected void Run()
    {
        Q3EJNI.sendCharEvent(ch);
    }
}
