package com.n0n3m4.q3e.onscreen;

import android.graphics.Point;

public class Q3EButtonGeometry
{
    public int x, y, width_or_radius, alpha;

    Q3EButtonGeometry()
    {
    }

    public Q3EButtonGeometry(Number x, Number y, Number r_or_w, Number a)
    {
        this.Set(x, y, r_or_w, a);
    }

    public void Set(Number x, Number y, Number r_or_w, Number a)
    {
        this.Set(x, y, r_or_w);
        this.alpha = a.intValue();
    }

    public void Set(Number x, Number y, Number r_or_w)
    {
        this.x = x.intValue();
        this.y = y.intValue();
        this.width_or_radius = r_or_w.intValue();
    }

    @Override
    public String toString()
    {
        return x + " " + y + " " + width_or_radius + " " + alpha;
    }

    public String toString(int maxw, int maxh)
    {
        Point point = Q3EButtonLayoutManager.ToRelPosition(x, y, maxw, maxh);
        return point.x + " " + point.y + " " + width_or_radius + " " + alpha;
    }


    public static String[] ToStrings(Q3EButtonGeometry[] layouts)
    {
        String[] layoutTable = new String[layouts.length];
        for (int i = 0; i < layouts.length; i++)
        {
            layoutTable[i] = layouts[i].toString();
        }
        return layoutTable;
    }


    public static String[] ToStrings(Q3EButtonGeometry[] layouts, int maxw, int maxh)
    {
        String[] layoutTable = new String[layouts.length];
        for (int i = 0; i < layouts.length; i++)
        {
            layoutTable[i] = layouts[i].toString(maxw, maxh);
        }
        return layoutTable;
    }

    public static Q3EButtonGeometry[] Alloc(int size)
    {
        Q3EButtonGeometry[] layouts = new Q3EButtonGeometry[size];
        for (int i = 0; i < layouts.length; i++)
        {
            layouts[i] = new Q3EButtonGeometry();
        }
        return layouts;
    }
}
