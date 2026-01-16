package com.n0n3m4.q3e.onscreen;

import android.graphics.Point;

public class UiElement
{
    public int cx;
    public int cy;
    public int size;
    public int alpha;

    public UiElement(int incx, int incy, int insize, int inalpha)
    {
        cx = incx;
        cy = incy;
        size = insize;
        alpha = inalpha;
    }

    public UiElement(String str, int maxw, int maxh)
    {
        LoadFromString(str, maxw, maxh);
    }

    public void LoadFromString(String str, int maxw, int maxh)
    {
        try
        {
            String[] spl = str.split(" ");
            cx = Integer.parseInt(spl[0]);
            cy = Integer.parseInt(spl[1]);
            Point point = Q3EButtonLayoutManager.ToAbsPosition(cx, cy, maxw, maxh);
            cx = point.x;
            cy = point.y;
            size = Integer.parseInt(spl[2]);
            alpha = Integer.parseInt(spl[3]);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public String SaveToString(int maxw, int maxh)
    {
        Point point = Q3EButtonLayoutManager.ToRelPosition(cx, cy, maxw, maxh);
        return point.x + " " + point.y + " " + size + " " + alpha;
    }

    public UiElement(String str)
    {
        LoadFromString(str);
    }

    public void LoadFromString(String str)
    {
        String[] spl = str.split(" ");
        cx = Integer.parseInt(spl[0]);
        cy = Integer.parseInt(spl[1]);
        size = Integer.parseInt(spl[2]);
        alpha = Integer.parseInt(spl[3]);
    }

    public String SaveToString()
    {
        return cx + " " + cy + " " + size + " " + alpha;
    }
}
