package com.n0n3m4.q3e.karin;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.InsetDrawable;
import android.util.AttributeSet;
import android.view.View;

import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.R;

public class KMouseCursor extends View
{
    private int    m_x     = -1;
    private int    m_y     = -1;

    public static final int WIDTH = 64;
    public static final int HEIGHT = 64;

    public KMouseCursor(Context context)
    {
        super(context);
        Setup();
    }

    public KMouseCursor(Context context, AttributeSet attrs)
    {
        super(context, attrs);
        Setup();
    }

    public KMouseCursor(Context context, AttributeSet attrs, int defStyle)
    {
        super(context, attrs, defStyle);
        Setup();
    }

    public void Setup()
    {
        //m_texture = ((BitmapDrawable)getResources().getDrawable(R.drawable.pointer_arrow_large)).getBitmap();
        //m_texture = BitmapFactory.decodeResource(getResources(), R.drawable.pointer_arrow_large);
        Drawable cursor = getResources().getDrawable(R.drawable.pointer_arrow_large_white);
        cursor = new InsetDrawable(cursor, -8, -5, -8, -5);
        setBackground(cursor);
        setFocusable(false);
        setFocusableInTouchMode(false);
    }

    public void SetVisible(boolean visible)
    {
        if(visible)
            setVisibility(View.VISIBLE);
        else
            setVisibility(View.GONE);
    }

    public void Show()
    {
        setVisibility(View.VISIBLE);
    }

    public void Hide()
    {
        setVisibility(View.GONE);
    }

    public void SetPosition(int x, int y)
    {
        if(m_x == x && m_y == y)
            return;
        m_x = x;
        m_y = y;
        setX(m_x);
        setY(m_y);
        getParent().requestLayout();
    }
}
