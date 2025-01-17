package com.n0n3m4.q3e.event;

import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.karin.KOnceRunnable;

public class Q3EExitEvent extends KOnceRunnable
{

    public Q3EExitEvent()
    {
    }

    @Override
    protected void Run()
    {
        Q3E.running = false;
        Q3EJNI.shutdown();
    }
}
