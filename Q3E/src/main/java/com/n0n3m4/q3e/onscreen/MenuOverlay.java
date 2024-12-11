package com.n0n3m4.q3e.onscreen;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.view.Gravity;
import android.view.View;
import android.widget.Toast;

import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUiView;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.R;
import com.n0n3m4.q3e.gl.Q3EGL;
import com.n0n3m4.q3e.karin.KResultRunnable;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

// MenuOverlay with OpenGL
public class MenuOverlay extends Paintable implements UiViewOverlay
{
    private static final int ON_SCREEN_BUTTON_MIN_SIZE = 0;
    private static final int ON_SCREEN_BUTTON_HOLD_INTERVAL = 500; // 100
    boolean mover_down = false;
    private Toast m_info;

    View view;
    int cx;
    int cy;
    int width;
    int height;
    int texw;
    int texh;
    float[] verts = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};
    FloatBuffer verts_p;
    float[] texcoords = {0, 0, 0, 1, 1, 1, 1, 0};
    FloatBuffer tex_p;
    byte[] indices = {0, 1, 2, 0, 2, 3};
    ByteBuffer inds_p;
    int tex_ind;
    Bitmap texbmp;

    public MenuOverlay(int center_x, int center_y, int w, int h, View view)
    {
        this.view = view;

        cx = center_x;
        cy = center_y;
        width = w;
        height = h;
        texw = Q3EUtils.nextpowerof2(w);
        texh = Q3EUtils.nextpowerof2(h);

        float[] myvrts = new float[verts.length];

        for (int i = 0; i < verts.length; i += 2)
        {
            myvrts[i] = verts[i] * width;
            myvrts[i + 1] = verts[i + 1] * height;
        }

        verts_p = ByteBuffer.allocateDirect(4 * myvrts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        verts_p.put(myvrts);
        verts_p.position(0);

        inds_p = ByteBuffer.allocateDirect(indices.length);
        inds_p.put(indices);
        inds_p.position(0);

        tex_p = ByteBuffer.allocateDirect(4 * texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
        tex_p.put(texcoords);
        tex_p.position(0);

        Bitmap bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(bmp);
        Paint p = new Paint();
        p.setTextSize(1);
        String alphastr = "- " + Q3ELang.tr(view.getContext(), R.string.opacity) + " +";
        while (p.measureText(alphastr) < width)
            p.setTextSize(p.getTextSize() + 1);
        p.setTextSize(p.getTextSize() - 1);
        p.setAntiAlias(true);
        c.drawARGB(0, 0, 0, 0);
        p.setStyle(Paint.Style.STROKE);
        p.setStrokeWidth(4);
        p.setARGB(120, 255, 255, 255);
        c.drawRect(new Rect(0, 0, width, height), p);
        p.setARGB(255, 255, 255, 255);
        String sizestr = "- " + Q3ELang.tr(view.getContext(), R.string.size) + " +";
        Rect bnd = new Rect();

        p.setStyle(Paint.Style.FILL);
        p.setStrokeWidth(1);

        p.getTextBounds(sizestr, 0, sizestr.length(), bnd);
        c.drawText(sizestr, (int) ((width - p.measureText(sizestr)) / 2), bnd.height() * 3 / 2, p);

        c.drawText(alphastr, (int) ((width - p.measureText(alphastr)) / 2), height - bnd.height() * 1 / 2, p);

        texbmp = Bitmap.createScaledBitmap(bmp, texw, texh, true);
    }

    @Override
    public void loadtex(GL10 gl)
    {
        tex_ind = Q3EGL.loadGLTexture(gl, texbmp);
    }

    boolean hidden = true;
    FingerUi fngr;

    public void hide()
    {
        hidden = true;
    }

    public void show(int x, int y, FingerUi fn)
    {
        x = Math.min(Math.max(width / 2, x), ((Q3EUiView) view).width - width / 2);
        y = Math.min(Math.max(height / 2 + ((Q3EUiView) view).yoffset, y), ((Q3EUiView) view).height + ((Q3EUiView) view).yoffset - height / 2);
        cx = x;
        cy = y;
        fngr = new FingerUi(fn.target, 9000);
        hidden = false;

        PrintInfo(fn);
    }

    @Override
    public void Paint(GL11 gl)
    {
        if (hidden)
            return;
        gl.glColor4f(1, 1, 1, 1);
        gl.glBindTexture(GL10.GL_TEXTURE_2D, tex_ind);
        gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, tex_p);
        gl.glVertexPointer(2, GL10.GL_FLOAT, 0, verts_p);
        gl.glPushMatrix();
        {
            gl.glTranslatef(cx, cy, 0.0f);
            gl.glDrawElements(GL10.GL_TRIANGLES, 6, GL10.GL_UNSIGNED_BYTE, inds_p);
        }
        gl.glPopMatrix();
    }

    public boolean tgtresize(boolean dir)
    {
        Q3EUiView uiView = (Q3EUiView) view;
        boolean res = UiViewOverlay.Resize(dir, uiView.step, fngr, uiView);
        if(res)
            PrintInfo(fngr);
        return res;
    }

    public boolean tgtalpha(boolean dir)
    {
        Paintable target = (Paintable) fngr.target;
        target.alpha += dir ? 0.1 : -0.1;
        ((Q3EUiView) view).SetModified();
        if (target.alpha < 0.01)
        {
            target.alpha = 0.1f;
            return false;
        }
        if (target.alpha > 1)
        {
            target.alpha = 1.0f;
            return false;
        }
        PrintInfo(fngr);
        return true;
    }

    @Override
    public boolean onTouchEvent(int x, int y, int act)
    {
        if (act == 1)
        {
            mover_down = true;
            final KResultRunnable rn;
            if (y < cy)
            {
                if (x > cx)
                    rn = new KResultRunnable()
                    {
                        @Override
                        public boolean Run()
                        {
                            return tgtresize(true);
                        }
                    };
                else
                    rn = new KResultRunnable()
                    {
                        @Override
                        public boolean Run()
                        {
                            return tgtresize(false);
                        }
                    };
            }
            else
            {
                if (x > cx) rn = new KResultRunnable()
                {
                    @Override
                    public boolean Run()
                    {
                        return tgtalpha(true);
                    }
                };
                else rn = new KResultRunnable()
                {
                    @Override
                    public boolean Run()
                    {
                        return tgtalpha(false);
                    }
                };
            }
            ((Q3EUiView) view).Post(new Runnable()
            {

                @Override
                public void run()
                {
                    if (!mover_down) return;
                    rn.run();
                    if (rn.GetResult())
                        ((Q3EUiView) view).Post(this, ON_SCREEN_BUTTON_HOLD_INTERVAL);
                }
            });

        }
        if (act == -1)
            mover_down = false;
        return true;
    }

    @Override
    public boolean isInside(int x, int y)
    {
        return ((!hidden) && (2 * Math.abs(cx - x) < width) && (2 * Math.abs(cy - y) < height));
    }

    public void PrintInfo(FingerUi fn)
    {
        if (null != m_info)
        {
            m_info.cancel();
            m_info = null;
        }

        Context context = view.getContext();
        StringBuilder sb = new StringBuilder();
        if (fn.target instanceof Slider)
        {
            Slider tmp = (Slider) fn.target;
            sb.append(Q3ELang.tr(context, R.string.position_))
                    .append(tmp.cx)
                    .append(", ")
                    .append(tmp.cy)
            ;
            sb.append("\n");
            sb.append(Q3ELang.tr(context, R.string.size_))
                    .append(tmp.width)
                    .append("x")
                    .append(tmp.height)
            ;
            sb.append("\n");
            sb.append(Q3ELang.tr(context, R.string.opacity_))
                    .append(String.format("%.1f", tmp.alpha))
            ;
        }
        else if (fn.target instanceof Button)
        {
            Button tmp = (Button) fn.target;
            sb.append(Q3ELang.tr(context, R.string.position_))
                    .append(tmp.cx)
                    .append(", ")
                    .append(tmp.cy)
            ;
            sb.append("\n");
            sb.append(Q3ELang.tr(context, R.string.size_))
                    .append(tmp.width)
                    .append("x")
                    .append(tmp.height)
            ;
            sb.append("\n");
            sb.append(Q3ELang.tr(context, R.string.opacity_))
                    .append(String.format("%.1f", tmp.alpha))
            ;
        }
        else if (fn.target instanceof Joystick)
        {
            Joystick tmp = (Joystick) fn.target;
            sb.append(Q3ELang.tr(context, R.string.position_))
                    .append(tmp.cx)
                    .append(", ")
                    .append(tmp.cy)
            ;
            sb.append("\n");
            sb.append(Q3ELang.tr(context, R.string.radius_))
                    .append(tmp.size)
            ;
            sb.append("\n");
            sb.append(Q3ELang.tr(context, R.string.opacity_))
                    .append(String.format("%.1f", tmp.alpha))
            ;
        }
        //k
        else if (fn.target instanceof Disc)
        {
            Disc tmp = (Disc) fn.target;
            sb.append(Q3ELang.tr(context, R.string.position_))
                    .append(tmp.cx)
                    .append(", ")
                    .append(tmp.cy)
            ;
            sb.append("\n");
            sb.append(Q3ELang.tr(context, R.string.center_radius_))
                    .append(tmp.size)
            ;
            sb.append("\n");
            sb.append(Q3ELang.tr(context, R.string.opacity_))
                    .append(String.format("%.1f", tmp.alpha))
            ;
        }
        if (sb.length() > 0)
        {
            m_info = Toast.makeText(view.getContext(), sb.toString(), Toast.LENGTH_SHORT);
            m_info.setGravity(Gravity.CENTER_HORIZONTAL | Gravity.TOP, 0, 0);
            m_info.show();
        }
    }
}
