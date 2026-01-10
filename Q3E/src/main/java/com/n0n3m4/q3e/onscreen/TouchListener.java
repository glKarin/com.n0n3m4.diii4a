package com.n0n3m4.q3e.onscreen;

import com.n0n3m4.q3e.Q3EGlobals;

import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

public interface TouchListener
{
    public static final int ACT_PRESS = 1;
    public static final int ACT_MOTION = 0;
    public static final int ACT_RELEASE = -1;

    public abstract boolean onTouchEvent(int x, int y, int act/*, int id*/);

    public abstract boolean isInside(int x, int y);

    public int Type();

    //public abstract boolean SupportMultiTouch();

    public static class TouchListenerCmp implements Comparator<TouchListener>
    {
        private final List<Integer> Type_Priority = Arrays.asList(
                Q3EGlobals.TYPE_BUTTON,
                Q3EGlobals.TYPE_SLIDER,
                Q3EGlobals.TYPE_DISC,
                Q3EGlobals.TYPE_JOYSTICK,
                Q3EGlobals.TYPE_MOUSE,
                Q3EGlobals.TYPE_MOUSE_BUTTON
        );

        @Override
        public int compare(TouchListener a, TouchListener b)
        {
            int ai = Type_Priority.indexOf(a.Type());
            int bi = Type_Priority.indexOf(b.Type());
            return ai - bi;
        }
    }
}
