package com.n0n3m4.q3e.onscreen;

import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EUtils;

import java.util.Arrays;

public class MouseControl implements TouchListener
{
    private final Q3EControlView view;
    private int lx;
    private int ly;
    private boolean clicked = false;

    public MouseControl(Q3EControlView vw)
    {
        view = vw;
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act)
    {
        if (act == 1)
        {
            clicked = true;
            lx = x;
            ly = y;
        }
        else if (act == -1)
        {
            clicked = false;
            lx = 0;
            ly = 0;
        }
        else
        {
            if(clicked)
            {
                Q3E.sendMotionEvent(x - lx, y - ly);
            }
            lx = x;
            ly = y;
        }
        return true;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        return !clicked;
    }

    public int Type()
    {
        return Q3EGlobals.TYPE_MOUSE;
    }
}
