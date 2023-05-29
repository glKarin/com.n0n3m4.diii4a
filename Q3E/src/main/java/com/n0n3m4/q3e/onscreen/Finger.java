package com.n0n3m4.q3e.onscreen;

import android.view.MotionEvent;

public class Finger
{
    public TouchListener target;
    public int id;

    public Finger(TouchListener tgt, int myid)
    {
        id = myid;
        target = tgt;
    }

    public boolean onTouchEvent(MotionEvent event)
    {
        int act = 0;
        if (event.getPointerId(event.getActionIndex()) == id)
        {
            if ((event.getActionMasked() == MotionEvent.ACTION_DOWN) || (event.getActionMasked() == MotionEvent.ACTION_POINTER_DOWN))
                act = 1;
            if ((event.getActionMasked() == MotionEvent.ACTION_UP) || (event.getActionMasked() == MotionEvent.ACTION_POINTER_UP) || (event.getActionMasked() == MotionEvent.ACTION_CANCEL))
                act = -1;
        }
        return target.onTouchEvent((int) event.getX(event.findPointerIndex(id)), (int) event.getY(event.findPointerIndex(id)), act);
    }
}
