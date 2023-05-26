package com.n0n3m4.q3e.onscreen;

public interface TouchListener
{
    public abstract boolean onTouchEvent(int x, int y, int act);

    public abstract boolean isInside(int x, int y);
}
