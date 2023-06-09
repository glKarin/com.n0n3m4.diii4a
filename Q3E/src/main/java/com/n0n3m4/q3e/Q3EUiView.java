/*
	Copyright (C) 2012 n0n3m4
	
    This file is part of Q3E.

    Q3E is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Q3E is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Q3E.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.n0n3m4.q3e;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

import com.n0n3m4.q3e.gl.GL;
import com.n0n3m4.q3e.onscreen.Button;
import com.n0n3m4.q3e.onscreen.Disc;
import com.n0n3m4.q3e.onscreen.FingerUi;
import com.n0n3m4.q3e.onscreen.Joystick;
import com.n0n3m4.q3e.onscreen.MenuOverlay;
import com.n0n3m4.q3e.onscreen.Paintable;
import com.n0n3m4.q3e.onscreen.Slider;
import com.n0n3m4.q3e.onscreen.TouchListener;
import com.n0n3m4.q3e.onscreen.UiElement;
import com.n0n3m4.q3e.onscreen.UiLoader;

import android.content.Context;
import android.content.SharedPreferences.Editor;
import android.opengl.GLSurfaceView;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.view.MotionEvent;

public class Q3EUiView extends GLSurfaceView implements GLSurfaceView.Renderer {

	private boolean m_usingUnit = false;
	private int m_unit = 0;
	public final int step = Q3EUtils.dip2px(getContext(), 5);
	private FloatBuffer m_gridBuffer = null;
	private int m_numGridLineVertex = 0;

	public Q3EUiView(Context context) {
		super(context);						
		
		GL.usegles20=false;
		
		setRenderer(this);						

		setFocusable(true);
		setFocusableInTouchMode(true);

		String unit = PreferenceManager.getDefaultSharedPreferences(context).getString(Q3EPreference.CONTROLS_CONFIG_POSITION_UNIT, "0");
		if(null != unit)
		{
			m_unit = Integer.parseInt(unit);
			m_usingUnit = m_unit > 0;
		}
	}

	@Override
	public void onDrawFrame(GL10 gl) {		
		gl.glClear(gl.GL_COLOR_BUFFER_BIT|gl.GL_DEPTH_BUFFER_BIT);
		
		gl.glMatrixMode(gl.GL_PROJECTION);
		gl.glLoadIdentity();
		gl.glOrthof(0, width, yoffset+height, yoffset, -1, 1);					
		
		((GL11)gl).glTexEnvi(gl.GL_TEXTURE_ENV, gl.GL_TEXTURE_ENV_MODE, gl.GL_MODULATE);			
						
		gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);

		if(null != m_gridBuffer && m_numGridLineVertex > 0)
		{
			gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
			gl.glLineWidth(1);
			gl.glBindTexture(gl.GL_TEXTURE_2D, 0);
			gl.glColor4f(1,1,1,0.382f);
			gl.glVertexPointer(2, gl.GL_FLOAT, 0, m_gridBuffer);
			gl.glDrawArrays(gl.GL_LINES, 0, m_numGridLineVertex);
		}
		gl.glLineWidth(4);
		
		gl.glBindTexture(gl.GL_TEXTURE_2D, 0);
		gl.glColor4f(1,0,0,0.7f);
		gl.glVertexPointer(2, gl.GL_FLOAT, 0, linebuffer);
		gl.glDrawArrays(gl.GL_LINES, 0, 2);
		
		gl.glColor4f(1,1,1,0.2f);
		gl.glVertexPointer(2, gl.GL_FLOAT, 0, notifybuffer);
		gl.glPushMatrix();
		{
			if (yoffset==0)
			{
				gl.glTranslatef(0, height-height/8, 0);
				gl.glDrawArrays(gl.GL_TRIANGLE_STRIP, 0, 4);
				//gl.glTranslatef(0, -(height-height/8), 0);
			}
			else
			{
				gl.glTranslatef(0, yoffset, 0);
				gl.glDrawArrays(gl.GL_TRIANGLE_STRIP, 0, 4);
				//gl.glTranslatef(0, -yoffset, 0);
			}
		}
		gl.glPopMatrix();
		
		gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);									
		
		for (Paintable p: paint_elements)
			p.Paint((GL11)gl);
		
		mover.Paint((GL11)gl);
		
		
	}		
	
	Handler mHandler=new Handler();
	
	public FingerUi[] fingers=new FingerUi[10];
	public ArrayList<TouchListener> touch_elements=new ArrayList<>(0);
	public ArrayList<Paintable> paint_elements=new ArrayList<>(0);
	
	boolean mInit=false;
	public int width;
	public int height;
	
	public int downtostep(int a,int la)
	{
		return a - la;
		/*int k=Math.round((float)(a-la)/(float)step);
		//int k=(a-la)/step;
		return k*step;*/
	}		
	
	public void RefreshTgt(FingerUi fn)
	{
		if (fn.target instanceof Button) {
			final Button tmp = (Button) fn.target;
			final Button newb = new Button(tmp.view, uildr.gl, tmp.cx, tmp.cy, tmp.width, tmp.height, tmp.tex_androidid, tmp.keycode, tmp.style, tmp.canbeheld, tmp.alpha);
			fn.target = newb;
			newb.tex_ind = tmp.tex_ind;
			touch_elements.set(touch_elements.indexOf(tmp), newb);
			paint_elements.set(paint_elements.indexOf(tmp), newb);
		}

		if (fn.target instanceof Joystick) {
			final Joystick tmp = (Joystick) fn.target;
			final Joystick newj = new Joystick(tmp.view, uildr.gl, tmp.size / 2, tmp.alpha, tmp.cx, tmp.cy, Q3EUtils.q3ei.joystick_release_range, Q3EUtils.q3ei.joystick_inner_dead_zone, Q3EUtils.q3ei.joystick_unfixed, true);
			fn.target = newj;
			Joystick.Move(newj, tmp);
			touch_elements.set(touch_elements.indexOf(tmp), newj);
			paint_elements.set(paint_elements.indexOf(tmp), newj);
		}

		if (fn.target instanceof Slider) {
			final Slider tmp = (Slider) fn.target;
			final Slider news = new Slider(tmp.view, uildr.gl, tmp.cx, tmp.cy, tmp.width, tmp.height, tmp.tex_androidid, tmp.lkey, tmp.ckey, tmp.rkey, tmp.style, tmp.alpha);
			fn.target = news;
			news.tex_ind = tmp.tex_ind;
			touch_elements.set(touch_elements.indexOf(tmp), news);
			paint_elements.set(paint_elements.indexOf(tmp), news);
		}    

        //k
        if (fn.target instanceof Disc)
        {
            final Disc tmp = (Disc)fn.target;
            final Disc newj = new Disc(tmp.view, uildr.gl, tmp.cx, tmp.cy, tmp.size/2,tmp.alpha, null);
            fn.target = newj;
            Disc.Move(newj, tmp);
            touch_elements.set(touch_elements.indexOf(tmp), newj);
            paint_elements.set(paint_elements.indexOf(tmp), newj);
		}
	}

	private boolean NormalizeTgtPosition(FingerUi fn)
	{
		if(!m_usingUnit)
			return false;

		if (fn.target instanceof Slider)
		{
			Slider tmp=(Slider)fn.target;
			float halfw = (float)tmp.width / 2.0f;
			float halfh = (float)tmp.height / 2.0f;
			tmp.cx = Math.round(((float)tmp.cx - halfw) / (float)m_unit) * m_unit + Math.round(halfw);
			tmp.cy = Math.round(((float)tmp.cy - halfh) / (float)m_unit) * m_unit + Math.round(halfh);
			return true;
		}
		else if (fn.target instanceof Button)
		{
			Button tmp=(Button)fn.target;
			float halfw = (float)tmp.width / 2.0f;
			float halfh = (float)tmp.height / 2.0f;
			tmp.cx = Math.round(((float)tmp.cx - halfw) / (float)m_unit) * m_unit + Math.round(halfw);
			tmp.cy = Math.round(((float)tmp.cy - halfh) / (float)m_unit) * m_unit + Math.round(halfh);
			return true;
		}
		else if (fn.target instanceof Joystick)
		{
			Joystick tmp=(Joystick)fn.target;
			float halfw = (float)tmp.size / 2.0f;
			int cx = Math.round(((float)tmp.cx - halfw) / (float)m_unit) * m_unit + Math.round(halfw);
			int cy = Math.round(((float)tmp.cy - halfw) / (float)m_unit) * m_unit + Math.round(halfw);
			tmp.SetPosition(cx, cy);
			return true;
		}
		//k
		else if (fn.target instanceof Disc)
		{
			Disc tmp=(Disc)fn.target;
			float halfw = (float)tmp.size / 2.0f;
			int cx = Math.round(((float)tmp.cx - halfw) / (float)m_unit) * m_unit + Math.round(halfw);
			int cy = Math.round(((float)tmp.cy - halfw) / (float)m_unit) * m_unit + Math.round(halfw);
			tmp.SetPosition(cx, cy);
			return true;
		}
		return false;
	}

	public void ModifyTgt(FingerUi fn,int dx,int dy)
	{
		if (fn.target instanceof Slider) {
			Slider tmp = (Slider) fn.target;
			tmp.cx += dx;
			tmp.cy += dy;
		}
		if (fn.target instanceof Button) {
			Button tmp = (Button) fn.target;
			tmp.cx += dx;
			tmp.cy += dy;
		}
		if (fn.target instanceof Joystick) {
			Joystick tmp = (Joystick) fn.target;
			tmp.Translate(dx, dy);
		}
        //k
        if (fn.target instanceof Disc)
        {
            Disc tmp=(Disc)fn.target;
            tmp.cx+=dx;
            tmp.cy+=dy;
		}
	}
	
	public void UiOnTouchEvent(FingerUi fn, MotionEvent event)
	{
		int act = 0;
		if (event.getPointerId(event.getActionIndex()) == fn.id) {
			if ((event.getActionMasked() == MotionEvent.ACTION_DOWN) || (event.getActionMasked() == MotionEvent.ACTION_POINTER_DOWN))
				act = 1;
			if ((event.getActionMasked() == MotionEvent.ACTION_UP) || (event.getActionMasked() == MotionEvent.ACTION_POINTER_UP) || (event.getActionMasked() == MotionEvent.ACTION_CANCEL))
				act = -1;
		}

		int x = (int) event.getX(event.findPointerIndex(fn.id));
		int y = (int) event.getY(event.findPointerIndex(fn.id));

		if (fn.target == mover) {
			fn.onTouchEvent(event);
			return;
		}

		if (act == 1) {
			fn.lastx = x;
			fn.lasty = y;
			fn.movd = false;
		}

		int dx = downtostep(x, fn.lastx);
		fn.lastx += dx;
		int dy = downtostep(y, fn.lasty);
		fn.lasty += dy;
		if ((dx != 0) || (dy != 0)) {
			fn.movd = true;
			ModifyTgt(fn, dx, dy);
			RefreshTgt(fn);
		}

		if ((act == -1) && (!fn.movd)) {
			mover.show(x, y, fn);
		}

		//k: renormalize position
		if (act == -1 && fn.movd)
		{
			if(NormalizeTgtPosition(fn))
				RefreshTgt(fn);
		}
	}
	
	public void SaveAll()
	{
		Editor mEdtr=PreferenceManager.getDefaultSharedPreferences(getContext()).edit();
		for (int i=0;i<touch_elements.size();i++)
		{
			if (touch_elements.get(i) instanceof Button) {
				Button tmp = (Button) touch_elements.get(i);
				mEdtr.putString(Q3EPreference.pref_controlprefix + i, new UiElement(tmp.cx, tmp.cy, tmp.width, (int) (tmp.alpha * 100)).SaveToString());
			}

			if (touch_elements.get(i) instanceof Slider) {
				Slider tmp = (Slider) touch_elements.get(i);
				mEdtr.putString(Q3EPreference.pref_controlprefix + i, new UiElement(tmp.cx, tmp.cy, tmp.width, (int) (tmp.alpha * 100)).SaveToString());
			}

			if (touch_elements.get(i) instanceof Joystick) {
				Joystick tmp = (Joystick) touch_elements.get(i);
				mEdtr.putString(Q3EPreference.pref_controlprefix + i, new UiElement(tmp.cx, tmp.cy, tmp.size / 2, (int) (tmp.alpha * 100)).SaveToString());
			}
            //k
            if (touch_elements.get(i) instanceof Disc)
            {
                Disc tmp = (Disc)touch_elements.get(i);
                mEdtr.putString(Q3EPreference.pref_controlprefix + i, new UiElement(tmp.cx, tmp.cy, tmp.size / 2, (int)(tmp.alpha * 100)).SaveToString());
			}
		}
		mEdtr.commit();
	}
	
	UiLoader uildr;
	MenuOverlay mover;
	
	public int yoffset=0;
	FloatBuffer linebuffer=ByteBuffer.allocateDirect(4*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
	FloatBuffer notifybuffer=ByteBuffer.allocateDirect(4*8).order(ByteOrder.nativeOrder()).asFloatBuffer();
		
	@Override
	public void onSurfaceChanged(GL10 gl, int w, int h) {
		if (!mInit) {
			width = w;
			height = h;

			gl.glViewport(0, 0, width, height);
			float ratio = (float) width / height;
			gl.glMatrixMode(GL10.GL_PROJECTION);
			gl.glLoadIdentity();
			gl.glFrustumf(-ratio, ratio, -1, 1, 1, 10);
			gl.glEnable(GL10.GL_TEXTURE_2D);
			gl.glEnable(GL10.GL_BLEND);
			gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);
			gl.glClearColor(0, 0, 0, 1);

			float[] line = {0, height, width, height};
			linebuffer.put(line);
			linebuffer.position(0);

			gl.glLineWidth(4);

			float[] notifyquad = {0, 0, width, 0, 0, height / 8, width, height / 8};
			notifybuffer.put(notifyquad);
			notifybuffer.position(0);

			uildr = new UiLoader(this, gl, width, height);

			mover = new MenuOverlay(0, 0, width / 4, width / 6, this);
			mover.loadtex(gl);

			for (int i = 0; i < Q3EUtils.q3ei.UI_SIZE; i++) {
				Object o = uildr.LoadElement(i, true);
				touch_elements.add((TouchListener) o);
				paint_elements.add((Paintable) o);
			}

			for (Paintable p : paint_elements) p.loadtex(gl);

			for (int i = 0; i < fingers.length; i++)
				fingers[i] = new FingerUi(null, i);

			MakeGrid();
			mInit = true;
		}
	}		
	
	@Override
	public boolean onTouchEvent(MotionEvent event) {		
		if (!mInit) return true;
		
		event.offsetLocation(0, yoffset);

		if ((event.getActionMasked() == MotionEvent.ACTION_DOWN) || (event.getActionMasked() == MotionEvent.ACTION_POINTER_DOWN)) {
			int pid = event.getPointerId(event.getActionIndex());
			int x = (int) event.getX(event.getActionIndex());
			int y = (int) event.getY(event.getActionIndex());

			if (mover.isInside(x, y)) {
				fingers[pid].target = mover;
			} else
				for (TouchListener tl : touch_elements)
					if (tl.isInside(x, y)) {
						fingers[pid].target = tl;
						break;
					}

			if (fingers[pid].target == null) {
				mover.hide();

				if ((yoffset == 0) && (y > height - height / 6)) {
					yoffset = height / 3;
				} else {
					if (y < yoffset + height / 6)
						yoffset = 0;
				}

			}
		}

		for (FingerUi f : fingers)
			if (f.target != null) {
				if ((f.target != mover) && (!touch_elements.contains(f.target)))
					f.target = null;
				else
					UiOnTouchEvent(f, event);
			}

		if ((event.getActionMasked() == MotionEvent.ACTION_UP) || (event.getActionMasked() == MotionEvent.ACTION_POINTER_UP) || (event.getActionMasked() == MotionEvent.ACTION_CANCEL)) {
			int pid = event.getPointerId(event.getActionIndex());
			fingers[pid].target = null;
		}		
		
		return true;
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		if (mInit) {
			mover.loadtex(gl);
			for (Paintable p : paint_elements) p.loadtex(gl);
		}		
	}

	private void MakeGrid()
	{
		if(!m_usingUnit)
			return;

		final int countX = width / m_unit + (width % m_unit != 0 ? 1 : 0);
		final int countY = height / m_unit + (height % m_unit != 0 ? 1 : 0);
		if(countX <= 0 || countY <= 0)
			return;

		final int w = countX * m_unit;
		final int h = countY * m_unit;
		m_numGridLineVertex = ((countX + 1) + (countY + 1)) * 2;
		final int numFloats = 2 * m_numGridLineVertex;
		final int sizeof_float = 4;
		m_gridBuffer = ByteBuffer.allocateDirect(numFloats * sizeof_float).order(ByteOrder.nativeOrder()).asFloatBuffer();
		float[] vertexBuf = new float[numFloats];
		int ptr = 0;
		for(int i = 0; i <= countX; i++)
		{
			vertexBuf[ptr * 2 * 2] = m_unit * i; // [(x,), ()]
			vertexBuf[ptr * 2 * 2 + 1] = 0; // [(, y), ()]
			vertexBuf[ptr * 2 * 2 + 2] = m_unit * i; // [(), (x, )]
			vertexBuf[ptr * 2 * 2 + 3] = h; // [(), (, y)]
			ptr++;
		}
		for(int i = 0; i <= countY; i++)
		{
			vertexBuf[ptr * 2 * 2] = 0; // [(x,), ()]
			vertexBuf[ptr * 2 * 2 + 1] = m_unit * i; // [(, y), ()]
			vertexBuf[ptr * 2 * 2 + 2] = w; // [(), (x, )]
			vertexBuf[ptr * 2 * 2 + 3] = m_unit * i; // [(), (, y)]
			ptr++;
		}
		m_gridBuffer.put(vertexBuf);
		m_gridBuffer.position(0);
	}

	@Override
	protected void onDetachedFromWindow() {
		super.onDetachedFromWindow();
		GL.usegles20 = true;
	}

	public void Post(Runnable runnable, int...delayed)
	{
		if(null != delayed && delayed.length > 0)
			mHandler.postDelayed(runnable, delayed[0]);
		else
			mHandler.post(runnable);
	}
}
