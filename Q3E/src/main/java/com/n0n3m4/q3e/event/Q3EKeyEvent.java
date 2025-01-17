package com.n0n3m4.q3e.event;

import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.karin.KOnceRunnable;

public class Q3EKeyEvent extends KOnceRunnable
{
    public final boolean down;
    public final int     keycode;
    public final int     charcode;

    public Q3EKeyEvent(boolean down, int keycode, int charcode)
    {
        this.down = down;
        this.keycode = keycode;
        this.charcode = charcode;
    }

    @Override
    protected void Run()
    {
        Q3EJNI.sendKeyEvent(down ? 1 : 0, keycode, charcode);
    }
}
