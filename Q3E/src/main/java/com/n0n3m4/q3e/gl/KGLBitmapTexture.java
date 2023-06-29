package com.n0n3m4.q3e.gl;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.graphics.RectF;

import javax.microedition.khronos.opengles.GL10;

public final class KGLBitmapTexture
{
    private KGLBitmapTexture() {}

    public static int GenCircleRingTexture(GL10 gl, int width, float ringWidth, int[] rgba)
    {
        if(width <= 0)
            return 0;

        final float radius = (float)width / 2.0f;
        final float internalsize = radius - ringWidth;

        Bitmap bmp = Bitmap.createBitmap(width, width, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(bmp);
        Paint p = new Paint();
        p.setAntiAlias(true);
        p.setARGB(rgba[0], rgba[1], rgba[2], rgba[3]);
        c.drawARGB(0, 0, 0, 0);
        c.drawCircle(radius, radius, radius, p);
        p.setXfermode(new PorterDuffXfermode(android.graphics.PorterDuff.Mode.CLEAR));
        c.drawCircle(radius, radius, internalsize, p);

        return Q3EGL.loadGLTexture(gl, bmp);
    }

    public static int GenCircleTexture(GL10 gl, int width, int[] rgba)
    {
        if(width <= 0)
            return 0;

        final float radius = (float)width / 2.0f;

        Bitmap bmp = Bitmap.createBitmap(width, width, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(bmp);
        Paint p = new Paint();
        p.setAntiAlias(true);
        p.setARGB(rgba[0], rgba[1], rgba[2], rgba[3]);
        c.drawARGB(0, 0, 0, 0);
        c.drawCircle(radius, radius, radius, p);

        return Q3EGL.loadGLTexture(gl, bmp);
    }

    public static int GenRectBorderTexture(GL10 gl, int width, int height, float borderWidth, int[] rgba)
    {
        if(width <= 0)
            return 0;

        if(height <= 0)
            height = width;
        Rect rect = new Rect(0, 0, width, height);
        Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(bmp);
        Paint p = new Paint();
        p.setAntiAlias(true);
        p.setARGB(rgba[0], rgba[1], rgba[2], rgba[3]);
        c.drawARGB(0, 0, 0, 0);
        c.drawRect(rect, p);
        p.setXfermode(new PorterDuffXfermode(android.graphics.PorterDuff.Mode.CLEAR));
        RectF rectf = new RectF(borderWidth, borderWidth, width - borderWidth, height - borderWidth);
        c.drawRect(rectf, p);

        return Q3EGL.loadGLTexture(gl, bmp);
    }
}
