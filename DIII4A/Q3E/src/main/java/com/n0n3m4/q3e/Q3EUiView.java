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

import com.n0n3m4.q3e.Q3EUtils.Button;
import com.n0n3m4.q3e.Q3EUtils.Finger;
import com.n0n3m4.q3e.Q3EUtils.Joystick;
import com.n0n3m4.q3e.Q3EUtils.Disc;
import com.n0n3m4.q3e.Q3EUtils.Paintable;
import com.n0n3m4.q3e.Q3EUtils.Slider;
import com.n0n3m4.q3e.Q3EUtils.TouchListener;
import com.n0n3m4.q3e.Q3EUtils.UiElement;
import com.n0n3m4.q3e.Q3EUtils.UiLoader;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;

public class Q3EUiView extends GLSurfaceView implements GLSurfaceView.Renderer {

	public Q3EUiView(Context context) {
		super(context);						
		
		Q3EUtils.usegles20=false;
		
		setRenderer(this);						

		setFocusable(true);
		setFocusableInTouchMode(true);
	}

	@Override
	public void onDrawFrame(GL10 gl) {		
		gl.glClear(gl.GL_COLOR_BUFFER_BIT|gl.GL_DEPTH_BUFFER_BIT);
		
		gl.glMatrixMode(gl.GL_PROJECTION);
		gl.glLoadIdentity();
		gl.glOrthof(0, width, yoffset+height, yoffset, -1, 1);					
		
		((GL11)gl).glTexEnvi(gl.GL_TEXTURE_ENV, gl.GL_TEXTURE_ENV_MODE, gl.GL_MODULATE);			
						
		gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
		
		gl.glBindTexture(gl.GL_TEXTURE_2D, 0);
		gl.glColor4f(1,0,0,0.7f);
		gl.glVertexPointer(2, gl.GL_FLOAT, 0, linebuffer);
		gl.glDrawArrays(gl.GL_LINES, 0, 2);
		
		gl.glColor4f(1,1,1,0.2f);
		gl.glVertexPointer(2, gl.GL_FLOAT, 0, notifybuffer);
		if (yoffset==0)
		{
			gl.glTranslatef(0, height-height/8, 0);
			gl.glDrawArrays(gl.GL_TRIANGLE_STRIP, 0, 4);
			gl.glTranslatef(0, -(height-height/8), 0);
		}
		else
		{
			gl.glTranslatef(0, yoffset, 0);
			gl.glDrawArrays(gl.GL_TRIANGLE_STRIP, 0, 4);
			gl.glTranslatef(0, -yoffset, 0);
		}
		
		gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);									
		
		for (Paintable p: paint_elements)
			p.Paint((GL11)gl);
		
		mover.Paint((GL11)gl);
		
		
	}		
	
	Handler mHandler=new Handler();
	boolean mover_down=false;
	
	class MenuOverlay extends Paintable implements TouchListener
	{		
		View view;
		int cx;int cy;int width;int height;
		int texw;int texh;
		float[] verts={-0.5f,-0.5f,-0.5f,0.5f,0.5f,0.5f,0.5f,-0.5f};FloatBuffer verts_p;
		float[] texcoords={0,0,0,1,1,1,1,0};FloatBuffer tex_p;
		byte indices[] = {0,1,2,0,2,3};ByteBuffer inds_p;
		int tex_ind;
		Bitmap texbmp;
		
		public MenuOverlay(int center_x,int center_y,int w, int h) {
			
			cx=center_x;
			cy=center_y;
			width=w;
			height=h;
			texw=Q3EUtils.nextpowerof2(w);
			texh=Q3EUtils.nextpowerof2(h);
			
			float[] myvrts=new float[verts.length];
			
			for (int i=0;i<verts.length;i+=2)
			{
				myvrts[i]=verts[i]*width+cx;
				myvrts[i+1]=verts[i+1]*height+cy;
			}
			
			verts_p=ByteBuffer.allocateDirect(4*myvrts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();			
			verts_p.put(myvrts);
			verts_p.position(0);
			
			inds_p=ByteBuffer.allocateDirect(indices.length);			
			inds_p.put(indices);
			inds_p.position(0);
			
			tex_p=ByteBuffer.allocateDirect(4*texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
			tex_p.put(texcoords);
			tex_p.position(0);						
			
			Bitmap bmp=Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
			Canvas c=new Canvas(bmp);
			Paint p=new Paint();			
			p.setTextSize(1);
			String alphastr="- opacity +";
			while (p.measureText(alphastr)<width)
				p.setTextSize(p.getTextSize()+1);
			p.setTextSize(p.getTextSize()-1);
			p.setAntiAlias(true);			
			c.drawARGB(0, 0, 0, 0);
			p.setStyle(Style.STROKE);
			p.setStrokeWidth(4);
			p.setARGB(120, 255, 255, 255);
			c.drawRect(new Rect(0,0,width,height), p);
			p.setARGB(255, 255, 255, 255);
			String sizestr="- size +";			
			Rect bnd=new Rect();
			
			p.setStyle(Style.FILL);
			p.setStrokeWidth(1);
			
			p.getTextBounds(sizestr, 0, sizestr.length(), bnd);
			c.drawText(sizestr,(int)((width-p.measureText(sizestr))/2),bnd.height()*3/2,p);
						
			c.drawText(alphastr,(int)((width-p.measureText(alphastr))/2),height-bnd.height()*1/2,p);
			
			texbmp=Bitmap.createScaledBitmap(bmp, texw, texh, true);
		}
		
		@Override
		public void loadtex(GL10 gl)
		{
			tex_ind=Q3EUtils.loadGLTexture(gl, texbmp);
		}
		
		boolean hidden=true;
		FingerUi fngr;		
		
		public void hide()
		{
			hidden=true;			
		}
		
		public void show(int x,int y,FingerUi fn)
		{						
			x=Math.min(Math.max(width/2,x),Q3EUiView.this.width-width/2);
			y=Math.min(Math.max(height/2+yoffset,y),Q3EUiView.this.height+yoffset-height/2);
			cx=x;
			cy=y;						
			fngr=new FingerUi(fn.target,9000);
			float[] myvrts=new float[verts.length];
			for (int i=0;i<verts.length;i+=2)
			{
				myvrts[i]=verts[i]*width+cx;
				myvrts[i+1]=verts[i+1]*height+cy;
			}
			verts_p.put(myvrts);
			verts_p.position(0);
			hidden=false;
		}
		
		@Override
		public void Paint(GL11 gl) {		
			if (hidden)
				return;
			gl.glColor4f(1, 1, 1, 1);
			gl.glBindTexture(GL10.GL_TEXTURE_2D, tex_ind);						
			gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, tex_p);
			gl.glVertexPointer(2, GL10.GL_FLOAT, 0, verts_p);
			gl.glDrawElements(GL10.GL_TRIANGLES, 6, GL10.GL_UNSIGNED_BYTE, inds_p);
		}
		
		public void tgtresize(boolean dir)
		{	
			Object o=fngr.target;
			int st=dir?step:-step;
			if (o instanceof Button)
			{
			Button tmp=(Button)o;
			float aspect=(float)tmp.height/tmp.width;
			if (tmp.width+st>step)
			{
			tmp.width+=st;
			tmp.height=(int)(aspect*tmp.width+0.5f);
			}
			}
			
			if (o instanceof Slider)
			{
			Slider tmp=(Slider)o;
			float aspect=(float)tmp.height/tmp.width;
			if (tmp.width+st>step)
			{
			tmp.width+=st;
			tmp.height=(int)(aspect*tmp.width+0.5f);
			}
			}
			
			if (o instanceof Joystick)
			{
			Joystick tmp=(Joystick)o;
			if (tmp.size+st>step)
			{
			tmp.size+=st;
			}
			}
            //k
            if (o instanceof Disc)
            {
                Disc tmp=(Disc)o;
                if (tmp.size+st>step)
                {
                    tmp.size+=st;
                }
			}							
			RefreshTgt(fngr);			
		}
		
		public void tgtalpha(boolean dir)
		{
			((Paintable)fngr.target).alpha+=dir?0.1:-0.1;
			if (((Paintable)fngr.target).alpha<0.01) ((Paintable)fngr.target).alpha+=0.1;
			if (((Paintable)fngr.target).alpha>1) ((Paintable)fngr.target).alpha-=0.1;
		}

		@Override
		public void onTouchEvent(int x, int y, int act) {
			if (act==1)
			{
			mover_down=true;
			final Runnable rn;
			if (y<cy)
			{			
			if (x>cx) 
				rn=new Runnable() {@Override public void run() {tgtresize(true);}};
				else rn=new Runnable() {@Override public void run() {tgtresize(false);}};
			}
			else
			{
			if (x>cx) rn=new Runnable() {@Override public void run() {tgtalpha(true);}};
			else rn=new Runnable() {@Override public void run() {tgtalpha(false);}};	
			}
			mHandler.post(new Runnable() {
				
				@Override
				public void run() {
					if (!mover_down) return;
					rn.run();
					mHandler.postDelayed(this, 100);
				}
			});
			
			}
			if (act==-1)
			mover_down=false;
			
		}

		@Override
		public boolean isInside(int x, int y) {			
			return ((!hidden)&&(2*Math.abs(cx-x)<width)&&(2*Math.abs(cy-y)<height));
		}
		
	}
	
	class FingerUi extends Finger
	{
		int lastx;
		int lasty;
		boolean movd;
		FingerUi(TouchListener tgt, int myid) {
			super(tgt, myid);
		}		
	}
	
	public FingerUi[] fingers=new FingerUi[10];
	public ArrayList<TouchListener> touch_elements=new ArrayList<Q3EUtils.TouchListener>(0);
	public ArrayList<Paintable> paint_elements=new ArrayList<Q3EUtils.Paintable>(0);
	
	public final int step=Q3EUtils.dip2px(getContext(), 5);
	
	boolean mInit=false;
	int width;int height;
	
	public int downtostep(int a,int la)
	{
		int k=(a-la)/step;		
		return k*step;
	}		
	
	public void RefreshTgt(FingerUi fn)
	{
		if (fn.target instanceof Button)
		{
			final Button tmp=(Button)fn.target;			
			final Button newb=new Button(tmp.view, uildr.gl, tmp.cx, tmp.cy, tmp.width, tmp.height, tmp.tex_androidid, tmp.keycode, tmp.style, tmp.canbeheld,tmp.alpha);
			fn.target=newb;								
			newb.tex_ind=tmp.tex_ind;
			touch_elements.set(touch_elements.indexOf(tmp),newb);
			paint_elements.set(paint_elements.indexOf(tmp),newb);
		}	
		
		if (fn.target instanceof Joystick)
		{
			final Joystick tmp=(Joystick)fn.target;			
			final Joystick newj=new Joystick(tmp.view, uildr.gl, tmp.cx, tmp.cy, tmp.size/2,tmp.alpha);
			fn.target=newj;
			newj.tex_ind=tmp.tex_ind;
			touch_elements.set(touch_elements.indexOf(tmp),newj);
			paint_elements.set(paint_elements.indexOf(tmp),newj);			
		}
		
		if (fn.target instanceof Slider)
		{
			final Slider tmp=(Slider)fn.target;			
			final Slider news=new Slider(tmp.view, uildr.gl, tmp.cx, tmp.cy, tmp.width, tmp.height, tmp.tex_androidid,tmp.lkey,tmp.ckey,tmp.rkey,tmp.style,tmp.alpha);
			fn.target=news;
			news.tex_ind=tmp.tex_ind;
			touch_elements.set(touch_elements.indexOf(tmp),news);
			paint_elements.set(paint_elements.indexOf(tmp),news);			
		}    

        //k
        if (fn.target instanceof Disc)
        {
            final Disc tmp=(Disc)fn.target;         
            final Disc newj=new Disc(tmp.view, uildr.gl, tmp.cx, tmp.cy, tmp.size/2,tmp.alpha);
            fn.target=newj;
            Disc.Move(newj, tmp);
            touch_elements.set(touch_elements.indexOf(tmp),newj);
            paint_elements.set(paint_elements.indexOf(tmp),newj);           
		}
	}
	
	public void ModifyTgt(FingerUi fn,int dx,int dy)
	{
		if (fn.target instanceof Slider)
		{
		Slider tmp=(Slider)fn.target;
		tmp.cx+=dx;
		tmp.cy+=dy;		
		}
		if (fn.target instanceof Button)
		{
		Button tmp=(Button)fn.target;
		tmp.cx+=dx;
		tmp.cy+=dy;		
		}
		if (fn.target instanceof Joystick)
		{
		Joystick tmp=(Joystick)fn.target;
		tmp.cx+=dx;
		tmp.cy+=dy;		
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
		int act=0;
		if (event.getPointerId(event.getActionIndex())==fn.id)
		{
			if ((event.getActionMasked()==MotionEvent.ACTION_DOWN)||(event.getActionMasked()==MotionEvent.ACTION_POINTER_DOWN))
				act=1;
			if ((event.getActionMasked()==MotionEvent.ACTION_UP)||(event.getActionMasked()==MotionEvent.ACTION_POINTER_UP)||(event.getActionMasked()==MotionEvent.ACTION_CANCEL))
				act=-1;
		}								
		
		int x=(int)event.getX(event.findPointerIndex(fn.id));
		int y=(int)event.getY(event.findPointerIndex(fn.id));
		
		if (fn.target==mover)
		{
			fn.onTouchEvent(event);
			return;
		}
		
		if (act==1)
		{
			fn.lastx=x;
			fn.lasty=y;
			fn.movd=false;			
		}
		
		int dx=downtostep(x, fn.lastx);
		fn.lastx+=dx;
		int dy=downtostep(y, fn.lasty);
		fn.lasty+=dy;
		if ((dx!=0)||(dy!=0))
		{					
		fn.movd=true;					
		ModifyTgt(fn, dx, dy);
		RefreshTgt(fn);
		}
		
		if ((act==-1)&&(!fn.movd))
		{
			mover.show(x, y, fn);
		}
	}
	
	public void SaveAll()
	{
		Editor mEdtr=PreferenceManager.getDefaultSharedPreferences(getContext()).edit();
		for (int i=0;i<touch_elements.size();i++)
		{
			if (touch_elements.get(i) instanceof Button)
			{
			Button tmp=(Button)touch_elements.get(i);
			mEdtr.putString(Q3EUtils.pref_controlprefix+i, new UiElement(tmp.cx, tmp.cy, tmp.width, (int)(tmp.alpha*100)).SaveToString());
			}
			
			if (touch_elements.get(i) instanceof Slider)
			{
			Slider tmp=(Slider)touch_elements.get(i);
			mEdtr.putString(Q3EUtils.pref_controlprefix+i, new UiElement(tmp.cx, tmp.cy, tmp.width, (int)(tmp.alpha*100)).SaveToString());
			}
			
			if (touch_elements.get(i) instanceof Joystick)
			{
			Joystick tmp=(Joystick)touch_elements.get(i);
			mEdtr.putString(Q3EUtils.pref_controlprefix+i, new UiElement(tmp.cx, tmp.cy, tmp.size/2, (int)(tmp.alpha*100)).SaveToString());
			}
            //k
            if (touch_elements.get(i) instanceof Disc)
            {
                Disc tmp=(Disc)touch_elements.get(i);
                mEdtr.putString(Q3EUtils.pref_controlprefix+i, new UiElement(tmp.cx, tmp.cy, tmp.size/2, (int)(tmp.alpha*100)).SaveToString());
			}
		}
		mEdtr.commit();
	}
	
	UiLoader uildr;
	MenuOverlay mover;
	
	int yoffset=0;
	FloatBuffer linebuffer=ByteBuffer.allocateDirect(4*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
	FloatBuffer notifybuffer=ByteBuffer.allocateDirect(4*8).order(ByteOrder.nativeOrder()).asFloatBuffer();
		
	@Override
	public void onSurfaceChanged(GL10 gl, int w, int h) {
		if(mInit == false)
		{				
			width=w;
			height=h;
			
			gl.glViewport(0, 0, width, height);			
		    float ratio = (float) width / height;
		    gl.glMatrixMode(GL10.GL_PROJECTION);
		    gl.glLoadIdentity();
		    gl.glFrustumf(-ratio, ratio, -1, 1, 1, 10);
		    gl.glEnable(GL10.GL_TEXTURE_2D);
		    gl.glEnable(GL10.GL_BLEND);				    
		    gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);
		    gl.glClearColor(0, 0, 0, 1);
		    
		    float[] line={0,height,width,height};
		    linebuffer.put(line);
		    linebuffer.position(0);
		    
		    gl.glLineWidth(4);
		    		    		    
		    float[] notifyquad={0,0,width,0,0,height/8,width,height/8};
		    notifybuffer.put(notifyquad);
		    notifybuffer.position(0);		    		    
			
			uildr=new UiLoader(this, gl, width, height);		
			
			mover=new MenuOverlay(0, 0, width/4, width/6);
			mover.loadtex(gl);			
			
			for (int i=0;i<Q3EUtils.q3ei.UI_SIZE;i++)
			{
				Object o=uildr.LoadElement(i);
				touch_elements.add((TouchListener)o);
				paint_elements.add((Paintable)o);				
			}													
			
			for (Paintable p:paint_elements) p.loadtex(gl);			
			
			for (int i=0;i<fingers.length;i++)
				fingers[i]=new FingerUi(null, i);								
			
			mInit = true;
		}
	}		
	
	@Override
	public boolean onTouchEvent(MotionEvent event) {		
		if (!mInit) return true;
		
		event.offsetLocation(0, yoffset);
		
		if ((event.getActionMasked()==MotionEvent.ACTION_DOWN)||(event.getActionMasked()==MotionEvent.ACTION_POINTER_DOWN))
		{
			int pid=event.getPointerId(event.getActionIndex());
			int x=(int)event.getX(event.getActionIndex());
			int y=(int)event.getY(event.getActionIndex());
			
			if (mover.isInside(x, y))
			{
				fingers[pid].target=mover;				
			}
			else
			for (TouchListener tl:touch_elements)
			if (tl.isInside(x, y))
			{
				fingers[pid].target=tl;
				break;
			}
			
			if (fingers[pid].target==null)
			{				
				mover.hide();
								
				if ((yoffset==0)&&(y>height-height/6))
				{
					yoffset=height/3;					
				}
				else
				{
					if (y<yoffset+height/6)
					yoffset=0;
				}
				
			}
		}
		
		for (FingerUi f:fingers)
			if (f.target!=null)
			{
				if ((f.target instanceof TouchListener)&&(f.target!=mover)&&(touch_elements.indexOf((TouchListener)f.target)==-1))
					f.target=null;
				else
					UiOnTouchEvent(f, event);
			}
		
		if ((event.getActionMasked()==MotionEvent.ACTION_UP)||(event.getActionMasked()==MotionEvent.ACTION_POINTER_UP)||(event.getActionMasked()==MotionEvent.ACTION_CANCEL))
		{
			int pid=event.getPointerId(event.getActionIndex());
			fingers[pid].target=null;
		}		
		
		return true;
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		if (mInit)
		{					
		mover.loadtex(gl);
		for (Paintable p:paint_elements) p.loadtex(gl);
		}		
	}

}
