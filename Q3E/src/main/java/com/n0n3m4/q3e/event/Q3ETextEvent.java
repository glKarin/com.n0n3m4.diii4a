package com.n0n3m4.q3e.event;

import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.karin.KOnceRunnable;

public class Q3ETextEvent extends KOnceRunnable
{
    public final String text;

    public Q3ETextEvent(String text)
    {
        this.text = text;
    }

    @Override
    protected void Run()
    {
        Q3EJNI.sendTextEvent(text);
    }
}
