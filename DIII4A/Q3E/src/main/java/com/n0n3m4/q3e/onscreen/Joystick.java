package com.n0n3m4.q3e.onscreen;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuffXfermode;
import android.view.View;

import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.gl.GL;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

public class Joystick extends Paintable implements TouchListener
{
    float[] verts_back = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};
    FloatBuffer verts_p;
    float[] verts_dot = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};
    FloatBuffer vertsd_p;
    float[] texcoords = {0, 0, 0, 1, 1, 1, 1, 0};
    FloatBuffer tex_p;
    byte[] indices = {0, 1, 2, 0, 2, 3};
    ByteBuffer inds_p;
    float[] posx = new float[8];
    float[] posy = new float[8];
    public int size;
    int dot_size;
    float internalsize;
    public int cx;
    public int cy;
    public int tex_ind;
    int texd_ind;

    int dot_pos = -1;
    int dotx, doty;
    boolean dotjoyenabled = false;
    public View view;
    private int m_joystickReleaseRange_2 = 0;
    private int m_size_2 = 0;

    public void Paint(GL11 gl)
    {
        //main paint
        super.Paint(gl);
        GL.DrawVerts(gl, tex_ind, 6, tex_p, verts_p, inds_p, 0, 0, red, green, blue, alpha);

        int dp = dot_pos;//Multithreading.
        if (dotjoyenabled)
            GL.DrawVerts(gl, texd_ind, 6, tex_p, vertsd_p, inds_p, dotx, doty, red, green, blue, alpha);
        else if (dp != -1)
            GL.DrawVerts(gl, texd_ind, 6, tex_p, vertsd_p, inds_p, posx[dp], posy[dp], red, green, blue, alpha);
    }

    public Joystick(View vw, GL10 gl, int center_x, int center_y, int r, float a)
    {
        view = vw;
        cx = center_x;
        cy = center_y;
        size = r * 2;
        dot_size = size / 3;
        alpha = a;

        if (Q3EUtils.q3ei.joystick_release_range >= 1.0f)
        {
            int jrr = Math.round(size * Q3EUtils.q3ei.joystick_release_range);
            if (jrr >= size)
                m_joystickReleaseRange_2 = jrr * jrr;
        }
        m_size_2 = size * size;

        for (int i = 0; i < verts_back.length; i += 2)
        {
            verts_back[i] = verts_back[i] * size + cx;
            verts_back[i + 1] = verts_back[i + 1] * size + cy;
        }

        for (int i = 0; i < verts_dot.length; i += 2)
        {
            verts_dot[i] = verts_dot[i] * dot_size + cx;
            verts_dot[i + 1] = verts_dot[i + 1] * dot_size + cy;
        }
        verts_p = ByteBuffer.allocateDirect(4 * verts_back.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        verts_p.put(verts_back);
        verts_p.position(0);

        vertsd_p = ByteBuffer.allocateDirect(4 * verts_dot.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        vertsd_p.put(verts_dot);
        vertsd_p.position(0);

        inds_p = ByteBuffer.allocateDirect(indices.length);
        inds_p.put(indices);
        inds_p.position(0);

        tex_p = ByteBuffer.allocateDirect(4 * texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        tex_p.put(texcoords);
        tex_p.position(0);

        internalsize = (size * 11 / 24) - ((float) dot_size) / 2;
        for (int i = 0; i < 8; i++)
        {
            posx[i] = (float) (internalsize * Math.sin(i * Math.PI / 4));
            posy[i] = -(float) (internalsize * Math.cos(i * Math.PI / 4));
        }

    }

    @Override
    public void loadtex(GL10 gl)
    {
        Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(bmp);
        Paint p = new Paint();
        p.setAntiAlias(true);
        p.setARGB(255, 255, 255, 255);
        c.drawARGB(0, 0, 0, 0);
        c.drawCircle(size / 2, size / 2, size / 2, p);
        p.setXfermode(new PorterDuffXfermode(android.graphics.PorterDuff.Mode.CLEAR));
        int internalsize = (size * 11 / 24);
        c.drawCircle(size / 2, size / 2, internalsize, p);

        tex_ind = GL.loadGLTexture(gl, bmp);

        bmp = Bitmap.createBitmap(dot_size, dot_size, Bitmap.Config.ARGB_8888);
        c = new Canvas(bmp);
        p = new Paint();
        p.setAntiAlias(true);
        p.setARGB(255, 255, 255, 255);
        c.drawARGB(0, 0, 0, 0);
        c.drawCircle(dot_size / 2, dot_size / 2, dot_size / 2, p);
        texd_ind = GL.loadGLTexture(gl, bmp);
    }

    int[] codes = {Q3EKeyCodes.KeyCodes.J_UP, Q3EKeyCodes.KeyCodes.J_RIGHT, Q3EKeyCodes.KeyCodes.J_DOWN, Q3EKeyCodes.KeyCodes.J_LEFT};
    private final static int[] Menu_Codes = {Q3EKeyCodes.KeyCodes.K_UPARROW, Q3EKeyCodes.KeyCodes.K_RIGHTARROW, Q3EKeyCodes.KeyCodes.K_DOWNARROW, Q3EKeyCodes.KeyCodes.K_LEFTARROW};
    boolean[] keys = {false, false, false, false};

    public void setenabled(int ind, boolean b)
    {
        if ((keys[ind] != b))
        {
            keys[ind] = b;
            ((Q3EControlView) (view)).sendKeyEvent(b, (Q3EUtils.q3ei.callbackObj.notinmenu ? codes : Menu_Codes)[ind], 0);
        }
    }

    public void setenabledarr(boolean[] arr)
    {
        for (int i = 0; i <= 3; i++)
            setenabled(i, arr[i]);
    }

    boolean[] enarr = new boolean[4];

    @Override
    public boolean onTouchEvent(int x, int y, int act)
    {
        boolean res = true;
        if (m_joystickReleaseRange_2 > 0)
        {
            if (4 * ((cx - x) * (cx - x) + (cy - y) * (cy - y)) > m_joystickReleaseRange_2)
                res = false;
        }
        if (res && act != -1)
        {
            if (Q3EUtils.q3ei.callbackObj.notinmenu)
            {
                if (((Q3EControlView) (view)).analog)
                {
                    dotjoyenabled = true;
                    dotx = x - cx;
                    doty = y - cy;
                    float analogx = (dotx) / internalsize;
                    float analogy = (-doty) / internalsize;
                    if (Math.abs(analogx) > 1.0f) analogx = analogx / Math.abs(analogx);
                    if (Math.abs(analogy) > 1.0f) analogy = analogy / Math.abs(analogy);

                    double dist = Math.sqrt(dotx * dotx + doty * doty);
                    if (dist > internalsize)
                    {
                        dotx = (int) (dotx * internalsize / dist);
                        doty = (int) (doty * internalsize / dist);
                    }
                    ((Q3EControlView) (view)).sendAnalog(true, analogx, analogy);
                } else
                {
                    int angle = (int) (112.5 - 180 * (Math.atan2(cy - y, x - cx) / Math.PI));
                    if (angle < 0) angle += 360;
                    if (angle >= 360) angle -= 360;
                    dot_pos = (int) (angle / 45);
                    enarr[0] = (dot_pos % 7 < 2);
                    enarr[1] = (dot_pos > 0) && (dot_pos < 4);
                    enarr[2] = (dot_pos > 2) && (dot_pos < 6);
                    enarr[3] = (dot_pos > 4);
                }

            } else
            {
                //IN MENU
                int angle = (int) (135 - 180 * (Math.atan2(cy - y, x - cx) / Math.PI));
                if (angle < 0) angle += 360;
                if (angle >= 360) angle -= 360;
                dot_pos = (int) (angle / 90);
                enarr[0] = false;
                enarr[1] = false;
                enarr[2] = false;
                enarr[3] = false;
                enarr[dot_pos] = true;
                dot_pos *= 2;
            }
        } else
        {
            if (((Q3EControlView) (view)).analog)
            {
                dotjoyenabled = false;
                ((Q3EControlView) (view)).sendAnalog(false, 0, 0);
            }
            dot_pos = -1;
            enarr[0] = false;
            enarr[1] = false;
            enarr[2] = false;
            enarr[3] = false;
        }
        setenabledarr(enarr);
        return res;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        return 4 * ((cx - x) * (cx - x) + (cy - y) * (cy - y)) <= m_size_2/*size * size*/;
    }
}
