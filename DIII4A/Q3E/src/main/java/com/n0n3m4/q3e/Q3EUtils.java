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

import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.ArrayList;

import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

import com.n0n3m4.q3e.Q3EKeyCodes.KeyCodes;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuffXfermode;
import android.graphics.Bitmap.Config;
import android.net.Uri;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import android.preference.PreferenceManager;
import android.text.AlteredCharSequence;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.graphics.RectF;
import java.util.List;

public class Q3EUtils {
	public static final int ONSCREEN_BUTTON_STYPE_FULL = 0;
	public static final int ONSCREEN_BUTTON_STYPE_RIGHT_BOTTOM = 1;
	public static final int ONSCREEN_BUTTON_STYPE_CENTER = 2;
	public static final int ONSCREEN_BUTTON_STYPE_LEFT_TOP = 3;

	public static final int ONSCRREN_BUTTON_NOT_HOLD = 0;
	public static final int ONSCRREN_BUTTON_CAN_HOLD = 1;

	public static final int ONSCRREN_SLIDER_STYLE_LEFT_RIGHT = 0;
	public static final int ONSCRREN_SLIDER_STYLE_DOWN_RIGHT = 1;
	
	public static Q3EInterface q3ei;
	public static boolean isOuya=false;
	
	static
	{
		Q3EUtils.isOuya = "cardhu".equals(android.os.Build.DEVICE)||android.os.Build.DEVICE.contains("ouya");//Dunno wtf is cardhu
	}
	
	static final String adpkgsn[]=new String[]{"com.n0n3m4.Q4A","com.n0n3m4.QII4A","com.n0n3m4.QIII4A",
								"com.n0n3m4.rtcw4a","com.n0n3m4.droidc","com.n0n3m4.droidpascal"};
	
	public static ImageView createiwbyid(final Activity ctx,Bitmap orig,int width,int id)
	{
		ImageView iw=new ImageView(ctx);
		Bitmap bmtmp=Bitmap.createBitmap(orig,0,id*(orig.getHeight()/adpkgsn.length),orig.getWidth(), orig.getHeight()/adpkgsn.length);			
		Bitmap bm=Bitmap.createScaledBitmap(bmtmp, width, width*bmtmp.getHeight()/bmtmp.getWidth(), true);
		iw.setImageBitmap(bm);
		iw.setTag(adpkgsn[id]);			
		iw.setOnClickListener(new OnClickListener() {				
			@Override
			public void onClick(View v) {
				Intent marketIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("market://search?q=details?id="+v.getTag()));
				marketIntent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
				ctx.startActivity(marketIntent);	
			}
		});
		return iw;
	}
	
	public static boolean isAppInstalled(Activity ctx,String nm)
    {               
        try
        {
        	ctx.getPackageManager().getPackageInfo(nm,PackageManager.GET_ACTIVITIES);
        	return true;
        }
        catch (Exception e){return false;}        
}
	
	public static Runnable adrunnable=null;
	
	public static void LoadAds(final Activity ctx)
	{
		final LinearLayout ll=(LinearLayout)ctx.findViewById(R.id.adlayout_id);
		ll.removeAllViews();
		if (isOuya) return;//No Google Play on ouya
        
		Bitmap imagorig=BitmapFactory.decodeResource(ctx.getResources(), R.drawable.adlist);
		//carousel
		ArrayList<Integer> ids=new ArrayList<Integer>(0);		
		ArrayList<View> vws=new ArrayList<View>(0);
		Display display = ctx.getWindowManager().getDefaultDisplay(); 
		final int width = dip2px(ctx,320);//Magic number
		final int dspwidth = display.getWidth();
		for (int i=0;i<adpkgsn.length;i++)
		{				
			if (!isAppInstalled(ctx,adpkgsn[i]))
			ids.add(i);			
		}
		for (int i:ids)
		{												
			vws.add(createiwbyid(ctx,imagorig,width,i));			
		}
		int carouselcount=dspwidth/width+((dspwidth%width==0)?0:1);
		
		if (carouselcount<=ids.size())
		{		
		for (int i=0;i<carouselcount;i++)		
			vws.add(createiwbyid(ctx,imagorig,width,ids.get(i)));
		for (int i=0;i<carouselcount;i++)		
			vws.add(0,createiwbyid(ctx,imagorig,width,ids.get(ids.size()-1-i)));		
		MyHorizontalScrollView.maxscrollx=width*(ids.size())+carouselcount*width/2;			
		MyHorizontalScrollView.minscrollx=width*carouselcount/2;
		MyHorizontalScrollView.deltascrollx=width*ids.size();
		}
		else
		{
		MyHorizontalScrollView.maxscrollx=0;			
		MyHorizontalScrollView.minscrollx=0;
		MyHorizontalScrollView.deltascrollx=0;	
		}
		for (View v:vws)
		{
			ll.addView(v);
		}
		final MyHorizontalScrollView mhsw=(MyHorizontalScrollView)ctx.findViewById(R.id.adlayout_hsw);
		
		if (adrunnable!=null)
		{
		ll.removeCallbacks(adrunnable);
		}
		adrunnable=new Runnable() {			
			@Override
			public void run() {
				if ((!ctx.isFinishing())&&(mhsw.getScrollX()%width==0))
				{				
				mhsw.scrollBy(width, 0);
				ll.postDelayed(this, 10000);				
				}
			}
		};		
		
		ll.postDelayed(adrunnable, 10000);		
		
	}
	
	
	//GL20 Utils	
	private static final String vertexShaderCode =
		    "attribute vec4 vPosition;" +
		    "attribute vec4 vTexCoord;" +
		    "uniform vec4 uScale;" +
		    "uniform vec4 uTranslate;" +		    
		    "varying vec2 varTexCoord;" +
		    "void main() {" +		    
		    "  gl_Position = uScale*(vPosition+uTranslate);"+//vec4(uScale.x*(vPosition.x+uTranslate.x),uScale.y*(vPosition.y+uTranslate.y),vPosition.z,vPosition.w);" +
		    "  varTexCoord = vec2(vTexCoord.x,vTexCoord.y);" +
		    "}";

	private static final String fragmentShaderCode =
		    "precision mediump float;" +
		    "uniform sampler2D uTexture;" +
		    "uniform vec4 uColor;" +
		    "varying vec2 varTexCoord;" +
		    "void main() {" +
		    "  gl_FragColor = uColor*texture2D(uTexture, varTexCoord);" +
		    "}";
	
	public static int loadShader(int type, String shaderCode){
	    int shader = GLES20.glCreateShader(type);
	    GLES20.glShaderSource(shader, shaderCode);
	    GLES20.glCompileShader(shader);
	    return shader;
	}
	
	public static int vertexShader;
	public static int fragmentShader;
	public static int gl20program;
	
	public static int gl20tx;
	public static int gl20sc;
	public static int gl20tr;
	public static int gl20cl;
	
	public static boolean usegles20=true;	
	
	public static void initGL20()
	{
		vertexShader=loadShader(GLES20.GL_VERTEX_SHADER, vertexShaderCode);
		fragmentShader=loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentShaderCode);
		gl20program = GLES20.glCreateProgram();
	    GLES20.glAttachShader(gl20program, vertexShader);	    
	    GLES20.glAttachShader(gl20program, fragmentShader);
	    GLES20.glLinkProgram(gl20program); 	  	  
	    gl20tx=GLES20.glGetUniformLocation(gl20program, "uTexture");
	    gl20sc=GLES20.glGetUniformLocation(gl20program, "uScale");
	    gl20tr=GLES20.glGetUniformLocation(gl20program, "uTranslate");
	    gl20cl=GLES20.glGetUniformLocation(gl20program, "uColor");
	}
	
	//End GL20			
	
	public static void DrawVerts(GL11 gl,int texid, int cnt, FloatBuffer texcoord, FloatBuffer vertcoord, ByteBuffer inds, float trax, float tray
			,float r, float g, float b, float a)
	{
		if (usegles20)
		{ 															
			GLES20.glUseProgram(gl20program);
		    int mPositionHandle = GLES20.glGetAttribLocation(gl20program, "vPosition");
		    int mTexHandle = GLES20.glGetAttribLocation(gl20program, "vTexCoord");		       		    		    
		    
		    GLES20.glEnableVertexAttribArray(mPositionHandle);
		    GLES20.glVertexAttribPointer(mPositionHandle,2,GLES20.GL_FLOAT, false,0, vertcoord);		    
		    GLES20.glEnableVertexAttribArray(mTexHandle);
		    GLES20.glVertexAttribPointer(mTexHandle,2,GLES20.GL_FLOAT, false,0,texcoord);		    		   
		    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
		    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texid);		    
		    GLES20.glUniform1i(gl20tx, 0);
		    GLES20.glUniform4f(gl20sc, 2.0f/Q3EView.orig_width, -2.0f/Q3EView.orig_height,0.0f,1.0f);
		    GLES20.glUniform4f(gl20tr, -Q3EView.orig_width/2+trax, -Q3EView.orig_height/2+tray,0.0f,0.0f);
		    GLES20.glUniform4f(gl20cl,r,g,b,a);
		    GLES20.glDrawElements(GLES20.GL_TRIANGLES, cnt, GLES20.GL_UNSIGNED_BYTE, inds);
		    if ((!Q3EUtils.q3ei.isQ1)&&(!Q3EUtils.q3ei.isD3BFG))
		    {
	        GLES20.glDisableVertexAttribArray(mPositionHandle);
	        GLES20.glDisableVertexAttribArray(mTexHandle);
		    }
		}
		else
		{			
			gl.glColor4f(r,g,b,a);		
			gl.glBindTexture(GL10.GL_TEXTURE_2D, texid);						
			gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, texcoord);
			gl.glVertexPointer(2, GL10.GL_FLOAT, 0, vertcoord);
			gl.glTranslatef(trax, tray, 0);
			gl.glDrawElements(GL10.GL_TRIANGLES, cnt, GL10.GL_UNSIGNED_BYTE, inds);
			gl.glTranslatef(-trax, -tray, 0);
		}
	}
		
	public static Bitmap ResourceToBitmap(Context cnt,String assetname)
	{
		try
		{
		InputStream is = cnt.getAssets().open(assetname);
		Bitmap b=BitmapFactory.decodeStream(is);		
		is.close();
		return b;
		}
		catch (Exception e){};
		return null;
	}				
	
	public static final int TYPE_BUTTON=0;
	public static final int TYPE_SLIDER=1;
	public static final int TYPE_JOYSTICK=2;	
	public static final int TYPE_DISC=3;	
	
	
	public static final String pref_datapath="q3e_datapath";
	public static final String pref_params="q3e_params";
	public static final String pref_hideonscr="q3e_hideonscr";
	public static final String pref_mapvol="q3e_mapvol";
	public static final String pref_analog="q3e_analog";
	public static final String pref_detectmouse="q3e_detectmouse";
	public static final String pref_eventdev="q3e_eventdev";
	public static final String pref_mousepos="q3e_mousepos";
	public static final String pref_scrres="q3e_scrres";
	public static final String pref_resx="q3e_resx";
	public static final String pref_resy="q3e_resy";
	public static final String pref_32bit="q3e_32bit";
	public static final String pref_msaa="q3e_msaa";
	public static final String pref_2fingerlmb="q3e_2fingerlmb";
	public static final String pref_nolight="q3e_nolight";
	public static final String pref_useetc1="q3e_useetc1";
	public static final String pref_usedxt="q3e_usedxt";
	public static final String pref_useetc1cache="q3e_useetc1cache";
	public static final String pref_controlprefix="q3e_controls_";
	public static final String pref_harm_16bit="q3e_harm_16bit"; //k
	public static final String pref_harm_r_harmclearvertexbuffer="q3e_r_harmclearvertexbuffer"; //k
	public static final String pref_harm_fs_game="q3e_harm_fs_game"; //k
	public static final String pref_harm_game_lib="q3e_harm_game_lib"; //k
	public static final String pref_harm_r_specularExponent="q3e_harm_r_specularExponent"; //k
	public static final String pref_harm_r_lightModel="q3e_harm_r_lightModel"; //k
	public static final String pref_harm_mapBack="q3e_harm_map_back"; //k
	public static final String pref_harm_game="q3e_harm_game"; //k
    public static final String pref_harm_q4_fs_game="q3e_harm_q4_fs_game"; //k
	public static final String pref_harm_q4_game_lib="q3e_harm_q4_game_lib"; //k
	public static final String pref_harm_user_mod="q3e_harm_user_mod"; //k
	
	public static class UiElement
	{
		int cx;int cy;int size;int alpha;
		
		public UiElement(int incx, int incy,int insize,int inalpha)
		{
			cx=incx;
			cy=incy;
			size=insize;
			alpha=inalpha;
		}
		
		public UiElement(String str)
		{
			LoadFromString(str);
		}
		
		public void LoadFromString(String str)
		{
			String[] spl=str.split(" ");
			cx=Integer.parseInt(spl[0]);
			cy=Integer.parseInt(spl[1]);
			size=Integer.parseInt(spl[2]);
			alpha=Integer.parseInt(spl[3]);
		}
		
		public String SaveToString()
		{
			return cx+" "+cy+" "+size+" "+alpha;
		}
	}
	
	public static int dip2px(Context ctx,int dip)
	{
		return (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dip, ctx.getResources().getDisplayMetrics());
	}
	
	public static class UiLoader
	{		
	View ctx;
	GL10 gl;
	int height;
	int width;
	//String filename;		
	
	public static String[] defaults_table;
	
	UiLoader(View cnt, GL10 gl10, int w, int h)//, String fname)
	{
		ctx=cnt;
		gl=gl10;		
		width=w;
		height=h;
		//filename=fname;
		defaults_table=q3ei.defaults_table;
		
		//Set defaults table					
	}
	
	public Object LoadElement(int id)
	{
		SharedPreferences shp=PreferenceManager.getDefaultSharedPreferences(ctx.getContext());
		String tmp=shp.getString(pref_controlprefix+id, null);
		if (tmp==null) tmp=defaults_table[id];
		UiElement el=new UiElement(tmp);				
		return LoadUiElement(id, el.cx, el.cy, el.size,el.alpha);
	}				
			
	private Object LoadUiElement(int id,int cx,int cy,int size,int alpha)
	{								
		int bh=size;
		if (q3ei.arg_table[id*4+2]==0)
			bh=size;
		if (q3ei.arg_table[id*4+2]==1)
			bh=size;
		if (q3ei.arg_table[id*4+2]==2)
			bh=size/2;
		
		int sh=size;
		if (q3ei.arg_table[id*4+3]==0)
			sh=size/2;
		if (q3ei.arg_table[id*4+3]==1)
			sh=size;				
		
		
		switch (q3ei.type_table[id])
		{
		case TYPE_BUTTON:
			return new Button(ctx, gl, cx, cy, size, bh, q3ei.texture_table[id], q3ei.arg_table[id*4], q3ei.arg_table[id*4+2], q3ei.arg_table[id*4+1]==1, (float)alpha/100);
		case TYPE_JOYSTICK:
			return new Joystick(ctx,gl,cx,cy,size,(float)alpha/100);
		case TYPE_SLIDER:
                return new Slider(ctx, gl, cx, cy, size, sh, q3ei.texture_table[id], q3ei.arg_table[id*4], q3ei.arg_table[id*4+1], q3ei.arg_table[id*4+2], q3ei.arg_table[id*4+3], (float)alpha/100);	
            case TYPE_DISC:
            {
                String keysStr = PreferenceManager.getDefaultSharedPreferences(ctx.getContext()).getString("harm_weapon_panel_keys", "1,2,3,4,5,6,7,8,9,q,0");
                char[] keys = null;
                if(null != keysStr && !keysStr.isEmpty())
                {
                    String[] arr = keysStr.split(",");
                    keys = new char[arr.length];
                    for(int i = 0; i < arr.length; i++)
                    {
                        keys[i] = arr[i].charAt(0);
                    }
                }
                return new Disc(ctx,gl,cx,cy,size,(float)alpha/100, keys);
            }
		}						
		return null;
	}
	
	}		
	
	public static int nextpowerof2(int x)
	{
		int candidate=1;
		while (candidate<x)
			candidate*=2;
		return candidate;
	}
	
	public static int loadGLTexture(GL10 gl, Bitmap bmp) {
		int t[]=new int[1];			
		
		if (usegles20)
		{
		GLES20.glEnable(GLES20.GL_TEXTURE_2D);
		GLES20.glGenTextures(1, t, 0);
		GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, t[0]);		
		GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
		GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR);
		GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
		GLES20.glTexParameteri(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
		}
		else
		{
		gl.glEnable(GL10.GL_TEXTURE_2D);
		gl.glGenTextures(1, t, 0);
		gl.glBindTexture(GL10.GL_TEXTURE_2D, t[0]);		
		gl.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        gl.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        gl.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        gl.glTexParameterx(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);
		}
        
        Bitmap powerof2bmp=Bitmap.createScaledBitmap(bmp, nextpowerof2(bmp.getWidth()), nextpowerof2(bmp.getHeight()), true);
        
		GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, powerof2bmp, 0);
		bmp.recycle();
		return t[0];
	}
	
	public static final int K_VKBD=9000; 
	
	public static void togglevkbd(View vw)
	{
		InputMethodManager imm = (InputMethodManager) vw.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);		
		imm.toggleSoftInput(InputMethodManager.SHOW_FORCED,0);
	}
	
	public static interface TouchListener
	{
		public abstract void onTouchEvent(int x,int y,int act);
		public abstract boolean isInside(int x,int y);
	}
	
	public static class Paintable
	{
		float red=1;
		float green=1;
		float blue=1;
		float alpha;		
		
		String tex_androidid;
		
		public void Paint(GL11 gl)
		{
			//Empty
		}
		
		public void loadtex(GL10 gl)
		{
		//Empty by default
		}
	}
	
	public static class Finger
	{
		public TouchListener target;
		int id;
		Finger(TouchListener tgt, int myid)
		{
			id=myid;
			target=tgt;
		}
		public void onTouchEvent(MotionEvent event)
		{
			int act=0;
			if (event.getPointerId(event.getActionIndex())==id)
			{
				if ((event.getActionMasked()==MotionEvent.ACTION_DOWN)||(event.getActionMasked()==MotionEvent.ACTION_POINTER_DOWN))
					act=1;
				if ((event.getActionMasked()==MotionEvent.ACTION_UP)||(event.getActionMasked()==MotionEvent.ACTION_POINTER_UP)||(event.getActionMasked()==MotionEvent.ACTION_CANCEL))
					act=-1;
			}
			target.onTouchEvent((int)event.getX(event.findPointerIndex(id)),(int)event.getY(event.findPointerIndex(id)),act);
		}
	}
	
		
	public static class Slider extends Paintable implements TouchListener
	{

		View view;
		int cx;int cy;int width;int height;
		float[] verts={-0.5f,-0.5f,-0.5f,0.5f,0.5f,0.5f,0.5f,-0.5f};FloatBuffer verts_p;
		float[] texcoords={0,0,0,1,1,1,1,0};FloatBuffer tex_p;
		byte indices[] = {0,1,2,0,2,3};ByteBuffer inds_p;
		int tex_ind;		
		int lkey,ckey,rkey;
		int startx,starty;
		int SLIDE_DIST;
		int style;
		
		public Slider(View vw,GL10 gl,int center_x,int center_y,int w, int h,String texid,
				int leftkey,int centerkey,int rightkey, int stl,float a)
		{
			view=vw;
			cx=center_x;
			cy=center_y;
			style=stl;
			alpha=a;
			width=w;
			height=h;
			SLIDE_DIST=w/3;
			lkey=leftkey;
			ckey=centerkey;
			rkey=rightkey;
			for (int i=0;i<verts.length;i+=2)
			{
				verts[i]=verts[i]*width+cx;
				verts[i+1]=verts[i+1]*height+cy;
			}
			
			verts_p=ByteBuffer.allocateDirect(4*verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();			
			verts_p.put(verts);
			verts_p.position(0);
			
			inds_p=ByteBuffer.allocateDirect(indices.length);			
			inds_p.put(indices);
			inds_p.position(0);
			
			tex_p=ByteBuffer.allocateDirect(4*texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
			tex_p.put(texcoords);
			tex_p.position(0);
							
			tex_androidid=texid;
		}
		
		@Override
		public void loadtex(GL10 gl)
		{
			tex_ind=loadGLTexture(gl, ResourceToBitmap(view.getContext(), tex_androidid));
		}

		@Override
		public void Paint(GL11 gl) {			
			super.Paint(gl);			
			DrawVerts(gl,tex_ind,6,tex_p,verts_p,inds_p,0,0,red,green,blue,alpha);
		}

		@Override
		public void onTouchEvent(int x, int y, int act) {
			if (act==1) 
				{
				startx=x;
				starty=y;
				}
			if (act==-1)
			{
				if (style==0)
				{
				if (x-startx<-SLIDE_DIST)
				{
				((Q3EControlView)(view)).sendKeyEvent(true,lkey,0);
				((Q3EControlView)(view)).sendKeyEvent(false,lkey,0);
				}
				else
				if (x-startx>SLIDE_DIST)
				{
					((Q3EControlView)(view)).sendKeyEvent(true,rkey,0);
					((Q3EControlView)(view)).sendKeyEvent(false,rkey,0);
				}
				else
				{					
					((Q3EControlView)(view)).sendKeyEvent(true,ckey,0);
					((Q3EControlView)(view)).sendKeyEvent(false,ckey,0);
				}
				}
				
                //k
				else if (style==1)
				{
				if ((y-starty>SLIDE_DIST) || (x-startx>SLIDE_DIST))
				{
					double ang=Math.abs(Math.atan2(y-starty, x-startx));
					if (ang>Math.PI/4 && ang<Math.PI*3/4)
					{					
					((Q3EControlView)(view)).sendKeyEvent(true,lkey,0);
					((Q3EControlView)(view)).sendKeyEvent(false,lkey,0);
					}
					else					
                    { //k
					((Q3EControlView)(view)).sendKeyEvent(true,rkey,0);
                        ((Q3EControlView)(view)).sendKeyEvent(false,rkey,0);
                    } //k
				}
				else
				{					
					((Q3EControlView)(view)).sendKeyEvent(true,ckey,0);
					((Q3EControlView)(view)).sendKeyEvent(false,ckey,0);
				}
				}
			}			
		}

		@Override
		public boolean isInside(int x, int y) {
			if (style==0)
			return ((2*Math.abs(cx-x)<width)&&(2*Math.abs(cy-y)<height));
			else
			return ((2*Math.abs(cx-x)<width)&&(2*Math.abs(cy-y)<height))&&(!((y>cy)&&(x>cx)));	
		}	
	}
	
	static ArrayList<Integer> heldarr=new ArrayList<Integer>(0);	
	
	public static class Button extends Paintable implements TouchListener
	{		
		View view;
		int cx;int cy;int width;int height; 
		float[] verts={-0.5f,-0.5f,-0.5f,0.5f,0.5f,0.5f,0.5f,-0.5f};FloatBuffer verts_p;
		float[] texcoords={0,0,0,1,1,1,1,0};FloatBuffer tex_p;
		byte indices[] = {0,1,2,0,2,3};ByteBuffer inds_p;		
		int tex_ind;		
		int keycode;
		int style;
		float initalpha;
		boolean canbeheld=false;		
		
		public Button(View vw,GL10 gl,int center_x,int center_y,int w,int h,String texid,int keyc,int stl,boolean canbheld,float a)
		{
			view=vw;
			cx=center_x;
			cy=center_y;
			alpha=a;
			initalpha=alpha;
			width=w;
			height=h;
			keycode=keyc;
			style=stl;			
			canbeheld=canbheld;
			for (int i=0;i<verts.length;i+=2)
			{
				verts[i]=verts[i]*w+cx;
				verts[i+1]=verts[i+1]*h+cy;
			}
			
			verts_p=ByteBuffer.allocateDirect(4*verts.length).order(ByteOrder.nativeOrder()).asFloatBuffer();			
			verts_p.put(verts);
			verts_p.position(0);
			
			inds_p=ByteBuffer.allocateDirect(indices.length);			
			inds_p.put(indices);
			inds_p.position(0);
			
			tex_p=ByteBuffer.allocateDirect(4*texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
			tex_p.put(texcoords);
			tex_p.position(0);										
			
			tex_androidid=texid;
		}
		
		@Override
		public void loadtex(GL10 gl)
		{
			tex_ind=loadGLTexture(gl, ResourceToBitmap(view.getContext(), tex_androidid));
		}

		@Override
		public void Paint(GL11 gl) {			
			super.Paint(gl);
			DrawVerts(gl,tex_ind,6,tex_p,verts_p,inds_p,0,0,red,green,blue,alpha);
		}
		
		private int lx;private int ly;
		
		@Override
		public void onTouchEvent(int x, int y, int act) {
			if (canbeheld)
			{
			if (act==1)
			{
			if (!heldarr.contains(keycode))
			{
				((Q3EControlView)(view)).sendKeyEvent(true,keycode,0);
				heldarr.add(keycode);
				alpha=Math.min(initalpha*2, 1f);
			}
			else
			{
				((Q3EControlView)(view)).sendKeyEvent(false,keycode,0);
				heldarr.remove(Integer.valueOf(keycode));
				alpha=initalpha;
			}
			}			
			return;
			}
			
			if (keycode==K_VKBD)
			{
				if (act==1)
					togglevkbd(view);
				return;
			}
			
			if (act==1) 
			{				
				lx=x;ly=y;
				((Q3EControlView)(view)).sendKeyEvent(true,keycode,0);
			}
			if (act==-1) 
				((Q3EControlView)(view)).sendKeyEvent(false,keycode,0);
			if (keycode==KeyCodes.K_MOUSE1)
			{
				if (((Q3EControlView)(view)).notinmenu)
				{
				((Q3EControlView)(view)).sendMotionEvent(x-lx,y-ly);
				lx=x;ly=y;
				}
				else
				{
				((Q3EControlView)(view)).sendMotionEvent(0,0);//???
				}
			}
		}

		@Override
		public boolean isInside(int x, int y) {
			if (style==0)
			return 4*((cx-x)*(cx-x)+(cy-y)*(cy-y))<=width*width;
			if (style==1)
			{
				int dx=x-cx;
				int dy=cy-y;
			return (((dy)<=(dx))&&(2*Math.abs(dx)<width)&&(2*Math.abs(dy)<height));
			}
			if (style==2)
			{
				int dx=x-cx;
				int dy=cy-y;
			return ((2*Math.abs(dx)<width)&&(2*Math.abs(dy)<height));
			}
			if (style==3)
			{
				int dx=cx-x;
				int dy=y-cy;
			return (((dy)<=(dx))&&(2*Math.abs(dx)<width)&&(2*Math.abs(dy)<height));
			}
			return false;
		}
		
	}
	
	public static class Joystick extends Paintable implements TouchListener
	{
		float[] verts_back={-0.5f,-0.5f,-0.5f,0.5f,0.5f,0.5f,0.5f,-0.5f};FloatBuffer verts_p;
		float[] verts_dot={-0.5f,-0.5f,-0.5f,0.5f,0.5f,0.5f,0.5f,-0.5f};FloatBuffer vertsd_p;
		float[] texcoords={0,0,0,1,1,1,1,0};FloatBuffer tex_p;
		byte indices[] = {0,1,2,0,2,3};ByteBuffer inds_p;
		float[] posx=new float[8];
		float[] posy=new float[8];								
		int size;int dot_size;
		float internalsize;
		int cx;int cy;
		int tex_ind;int texd_ind;
		
		int dot_pos=-1;	
		int dotx,doty;
		boolean dotjoyenabled=false;
		View view;
		
		public void Paint(GL11 gl)
		{																				
			//main paint		
			super.Paint(gl);
			DrawVerts(gl,tex_ind,6,tex_p,verts_p,inds_p,0,0,red,green,blue,alpha);
			
			int dp=dot_pos;//Multithreading.						
			if (dotjoyenabled)
			DrawVerts(gl,texd_ind,6,tex_p,vertsd_p,inds_p,dotx,doty,red,green,blue,alpha);	
			else				
			if (dp!=-1)
			DrawVerts(gl,texd_ind,6,tex_p,vertsd_p,inds_p,posx[dp],posy[dp],red,green,blue,alpha);			
		}
		
		public Joystick(View vw,GL10 gl,int center_x,int center_y,int r,float a)
		{			
			view=vw;
			cx=center_x;
			cy=center_y;
			size=r*2;
			dot_size=size/3;
			alpha=a;
			
			for (int i=0;i<verts_back.length;i+=2)
			{
				verts_back[i]=verts_back[i]*size+cx;
				verts_back[i+1]=verts_back[i+1]*size+cy;
			}
			
			for (int i=0;i<verts_dot.length;i+=2)
			{
				verts_dot[i]=verts_dot[i]*dot_size+cx;
				verts_dot[i+1]=verts_dot[i+1]*dot_size+cy;
			}
			verts_p=ByteBuffer.allocateDirect(4*verts_back.length).order(ByteOrder.nativeOrder()).asFloatBuffer();			
			verts_p.put(verts_back);
			verts_p.position(0);
			
			vertsd_p=ByteBuffer.allocateDirect(4*verts_dot.length).order(ByteOrder.nativeOrder()).asFloatBuffer();			
			vertsd_p.put(verts_dot);
			vertsd_p.position(0);
			
			inds_p=ByteBuffer.allocateDirect(indices.length);			
			inds_p.put(indices);
			inds_p.position(0);
			
			tex_p=ByteBuffer.allocateDirect(4*texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
			tex_p.put(texcoords);
			tex_p.position(0);						
			
			internalsize=(size*11/24)-((float)dot_size)/2;
			for (int i=0;i<8;i++)
			{
				posx[i]=(float)(internalsize*Math.sin(i*Math.PI/4));
				posy[i]=-(float)(internalsize*Math.cos(i*Math.PI/4));
			}
									
		}
		
		@Override
		public void loadtex(GL10 gl)
		{
			Bitmap bmp=Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);			
			Canvas c=new Canvas(bmp);
			Paint p=new Paint();
            p.setAntiAlias(true);
			p.setARGB(255, 255, 255, 255);
			c.drawARGB(0, 0, 0, 0);			
			c.drawCircle(size/2, size/2, size/2, p);			
			p.setXfermode(new PorterDuffXfermode(android.graphics.PorterDuff.Mode.CLEAR));
			int internalsize=(size*11/24);
			c.drawCircle(size/2, size/2, internalsize, p);
			
			tex_ind=loadGLTexture(gl, bmp);
			
			bmp=Bitmap.createBitmap(dot_size, dot_size, Bitmap.Config.ARGB_8888);
			c=new Canvas(bmp);
			p=new Paint();
            p.setAntiAlias(true);
			p.setARGB(255, 255, 255, 255);
			c.drawARGB(0, 0, 0, 0);
			c.drawCircle(dot_size/2, dot_size/2, dot_size/2, p);
			texd_ind=loadGLTexture(gl, bmp);
		}
		
		int[] codes={Q3EKeyCodes.KeyCodes.J_UP,Q3EKeyCodes.KeyCodes.J_RIGHT,Q3EKeyCodes.KeyCodes.J_DOWN,Q3EKeyCodes.KeyCodes.J_LEFT};
		boolean[] keys={false,false,false,false};				
		
		public void setenabled(int ind,boolean b)
		{
			if ((keys[ind]!=b))
			{
				keys[ind]=b;
				((Q3EControlView)(view)).sendKeyEvent(b,codes[ind],0);
			}		
		}
		
		public void setenabledarr(boolean[] arr)
		{
			for (int i=0;i<=3;i++)
				setenabled(i, arr[i]);
		}
		
		boolean[] enarr=new boolean[4];

		@Override
		public void onTouchEvent(int x, int y, int act) {
			if (act!=-1)
			{				
			if (((Q3EControlView)(view)).notinmenu)
			{														
			if (((Q3EControlView)(view)).analog)
			{
			dotjoyenabled=true;
			dotx=x-cx;
			doty=y-cy;			
			float analogx=(dotx)/internalsize;
			float analogy=(-doty)/internalsize;
			if (Math.abs(analogx)>1.0f) analogx=analogx/Math.abs(analogx);
			if (Math.abs(analogy)>1.0f) analogy=analogy/Math.abs(analogy);
			
			double dist=Math.sqrt(dotx*dotx+doty*doty);			
			if (dist>internalsize)
			{
				dotx=(int)(dotx*internalsize/dist);
				doty=(int)(doty*internalsize/dist);
			}			
			((Q3EControlView)(view)).sendAnalog(true,analogx,analogy);
			}
			else
			{
				int angle=(int)(112.5-180*(Math.atan2(cy-y, x-cx)/Math.PI));
				if (angle<0) angle+=360;	
				if (angle>=360) angle-=360;
				dot_pos=(int)(angle/45);
				enarr[0]=(dot_pos%7<2);
				enarr[1]=(dot_pos>0)&&(dot_pos<4);
				enarr[2]=(dot_pos>2)&&(dot_pos<6);
				enarr[3]=(dot_pos>4);
			}
			
			}
			else
			{
				//IN MENU
				int angle=(int)(135-180*(Math.atan2(cy-y, x-cx)/Math.PI));				
				if (angle<0) angle+=360;
				if (angle>=360) angle-=360;
				dot_pos=(int)(angle/90);
				enarr[0]=false;
				enarr[1]=false;
				enarr[2]=false;
				enarr[3]=false;
				enarr[dot_pos]=true;
				dot_pos*=2;						
			}			
			}
			else
			{
				if (((Q3EControlView)(view)).analog)
				{
				dotjoyenabled=false;	
				((Q3EControlView)(view)).sendAnalog(false,0,0);
				}
				dot_pos=-1;
				enarr[0]=false;
				enarr[1]=false;
				enarr[2]=false;
				enarr[3]=false;
			}			
			setenabledarr(enarr);
		}

		@Override
		public boolean isInside(int x, int y) {
			return 4*((cx-x)*(cx-x)+(cy-y)*(cy-y))<=size*size;
		}
	}		
	
	public static class MouseControl implements TouchListener
	{
		private Q3EControlView view;
		private boolean alreadydown;
		private int lx;private int ly;
		private boolean isleftbutton;
		public MouseControl(Q3EControlView vw, boolean islmb) {
			view=vw;
			alreadydown=false;
			isleftbutton=islmb;
		}		
		
		@Override
		public void onTouchEvent(int x, int y, int act) {
			if (act==1)		
			{
			if (isleftbutton)
			((Q3EControlView)(view)).sendKeyEvent(true,KeyCodes.K_MOUSE1,0);//Can be sent twice, unsafe.
			alreadydown=true;			
			}
			else
			{
			view.sendMotionEvent(x-lx,y-ly);
			}
			lx=x;ly=y;			
			
			if (act==-1)
			{
			if (isleftbutton)
				((Q3EControlView)(view)).sendKeyEvent(false,KeyCodes.K_MOUSE1,0);//Can be sent twice, unsafe.
			alreadydown=false;
			}
		}

		@Override
		public boolean isInside(int x, int y) {
			return !alreadydown;
		}
		
	}

    public static class Disc extends Paintable implements TouchListener
    {
        public static class Part
        {
            public float start;
            public float end;
            public char key;
            public int textureId;
            public int borderTextureId;
            public boolean pressed;
            public boolean disabled;
        }
        private FloatBuffer m_fanVertexArray;
        float[] verts_back={-0.5f,-0.5f,-0.5f,0.5f,0.5f,0.5f,0.5f,-0.5f};FloatBuffer verts_p;
        float[] texcoords={0,0,0,1,1,1,1,0};FloatBuffer tex_p;
        byte indices[] = {0,1,2,0,2,3};ByteBuffer inds_p;
        private Part[] m_parts = null;
        int size, dot_size;
        int cx, cy;
        int tex_ind;
        private int m_circleWidth;
        private char[] m_keys;

        int dotx,doty;
        boolean dotjoyenabled=false;
        View view;
        
        private Part GenPart(int index, char key, int total, GL10 gl)
        {
            Part res = new Part();
            res.key = key;
            double P = Math.PI * 2 / total;
            int centerR = size / 2;
            double start = P * index;
            double end = start + P;
            final double offset = -Math.PI / 2;
            final double mid = (end + start) / 2 + offset;
            int r = dot_size / 2;
            int sw = m_circleWidth / 2;
            final int fontSize = 10;

            Bitmap bmp=Bitmap.createBitmap(dot_size, dot_size, Bitmap.Config.ARGB_8888);            
            Canvas c=new Canvas(bmp);
            Paint p=new Paint();
            p.setAntiAlias(true);
            p.setARGB(255, 255, 255, 255);
            c.drawARGB(0, 0, 0, 0);  
            p.setStrokeWidth(sw);       
            p.setTextSize(sw * fontSize * 3 / 2);
            p.setTextAlign(Paint.Align.CENTER);
            Paint.FontMetrics fontMetrics = p.getFontMetrics();
            float fontHeight = (fontMetrics.descent - fontMetrics.ascent) / 2 - fontMetrics.descent;
            
            double rad = start + offset;
            double x = Math.cos(rad);
            double y = Math.sin(rad);
            c.drawLine(r + (int)(x * centerR), r + (int)(y * centerR), r + (int)(x * r), r + (int)(y * r), p);
            rad = end + offset;
            x = Math.cos(rad);
            y = Math.sin(rad);
            c.drawLine(r + (int)(x * centerR), r + (int)(y * centerR), r + (int)(x * r), r + (int)(y * r), p);

            x = Math.cos(mid);
            y = Math.sin(mid);
            int mr = (r + centerR) / 2;
            c.drawText("" + Character.toUpperCase(key), r + (int)(x * mr), r + (int)(y * mr + (fontHeight)), p);

            res.textureId = loadGLTexture(gl, bmp);
            bmp.recycle();

            res.start = rad2deg(start);
            res.end = rad2deg(end);
            
            sw = m_circleWidth;
            bmp = Bitmap.createBitmap(dot_size, dot_size, Bitmap.Config.ARGB_8888);            
            c = new Canvas(bmp);
            p = new Paint();
            p.setAntiAlias(true);
            p.setARGB(255, 255, 255, 255);
            c.drawARGB(0, 0, 0, 0);  
            p.setStrokeWidth(sw);       
            p.setTextSize(sw * fontSize);
            p.setTextAlign(Paint.Align.CENTER);
            fontMetrics = p.getFontMetrics();
            fontHeight = (fontMetrics.descent - fontMetrics.ascent) / 2 - fontMetrics.descent;
            rad = start + offset;
            x = Math.cos(rad);
            y = Math.sin(rad);
            centerR -= m_circleWidth;
            c.drawLine(r + (int)(x * centerR), r + (int)(y * centerR), r + (int)(x * r), r + (int)(y * r), p);
            rad = end + offset;
            x = Math.cos(rad);
            y = Math.sin(rad);
            c.drawLine(r + (int)(x * centerR), r + (int)(y * centerR), r + (int)(x * r), r + (int)(y * r), p);

            x = Math.cos(mid);
            y = Math.sin(mid);
            mr = (r + centerR) / 2;
            c.drawText("" + Character.toUpperCase(key), r + (int)(x * mr), r + (int)(y * mr + (fontHeight)), p);

            p.setStyle(Paint.Style.STROKE);
            start = start / Math.PI * 180;
            end = end / Math.PI * 180;
            RectF rect = new RectF(dot_size / 2 - size / 2 + sw / 2, dot_size / 2 - size / 2 + sw / 2, dot_size / 2 + size / 2  - sw / 2, dot_size / 2 + size / 2 - sw / 2);
            c.drawArc(rect, (float)(start - 90), (float)(end - start), false, p);
            
            rect = new RectF(0 + sw / 2, 0 + sw / 2, dot_size - sw / 2, dot_size - sw / 2);
            c.drawArc(rect, (float)(start - 90), (float)(end - start), false, p);
            
            res.borderTextureId = loadGLTexture(gl, bmp);
            bmp.recycle();
            
            return res;
        }

        public void Paint(GL11 gl)
        {                                                                               
            //main paint        
            super.Paint(gl);
            DrawVerts(gl,tex_ind,6,tex_p,verts_p,inds_p,0,0,red,green,blue,alpha);
            if(null == m_parts || m_parts.length == 0)
                return;
        
            if(dotjoyenabled)
            {
                for(Part p : m_parts)
                {
                    //DrawVerts(gl, p.textureId, 6, tex_p, m_fanVertexArray, inds_p, 0, 0, red,green,blue,p.pressed ? (float)Math.max(alpha, 0.9) : (float)(Math.min(alpha, 0.1)));
                    if(p.pressed)
                        DrawVerts(gl, p.borderTextureId, 6, tex_p, m_fanVertexArray, inds_p, 0, 0, red,green,blue, alpha + (1.0f - alpha) * 0.5f);
                    else
                        DrawVerts(gl, p.textureId, 6, tex_p, m_fanVertexArray, inds_p, 0, 0, red,green,blue, alpha - (alpha * 0.5f));
                }   
            }
        }

        public Disc(View vw,GL10 gl,int center_x,int center_y,int r,float a, char[] keys)
        {           
            view=vw;
            cx=center_x;
            cy=center_y;
            size=r*2;
            dot_size=size*3;
            alpha=a;

            float[] tmp = new float[verts_back.length];
            for (int i=0;i<verts_back.length;i+=2)
            {
                tmp[i]=verts_back[i]*size+cx;
                tmp[i+1]=verts_back[i+1]*size+cy;
            }

            verts_p=ByteBuffer.allocateDirect(4*verts_back.length).order(ByteOrder.nativeOrder()).asFloatBuffer();          
            verts_p.put(tmp);
            verts_p.position(0);

            inds_p=ByteBuffer.allocateDirect(indices.length);           
            inds_p.put(indices);
            inds_p.position(0);

            tex_p=ByteBuffer.allocateDirect(4*texcoords.length).order(ByteOrder.nativeOrder()).asFloatBuffer();
            tex_p.put(texcoords);
            tex_p.position(0);                   

            float[] tmp2 = new float[verts_back.length];
            for (int i=0;i<verts_back.length;i+=2)
            {
                tmp2[i]=verts_back[i]*dot_size+cx;
                tmp2[i+1]=verts_back[i+1]*dot_size+cy;
            }  

            m_fanVertexArray=ByteBuffer.allocateDirect(4*verts_back.length).order(ByteOrder.nativeOrder()).asFloatBuffer();          
            m_fanVertexArray.put(tmp2);
            m_fanVertexArray.position(0);
            
            m_keys = keys;
        }

        @Override
        public void loadtex(GL10 gl)
        {
            Bitmap bmp=Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);            
            Canvas c=new Canvas(bmp);
            Paint p=new Paint();
            p.setARGB(255, 255, 255, 255);
            c.drawARGB(0, 0, 0, 0);         
            c.drawCircle(size/2, size/2, size/2, p);            
            p.setXfermode(new PorterDuffXfermode(android.graphics.PorterDuff.Mode.CLEAR));
            int internalsize=(size/2*7/8);
            c.drawCircle(size/2, size/2, internalsize, p);

            m_circleWidth = size / 2 - internalsize;
            tex_ind=loadGLTexture(gl, bmp);
            bmp.recycle();

            if(null != m_keys && m_keys.length > 0)
            {
                m_parts = new Part[m_keys.length];
                for(int i = 0; i < m_keys.length; i++)
                {
                    m_parts[i] = GenPart(i, m_keys[i], m_keys.length, gl);
                }
            }
        } 

        @Override
        public void onTouchEvent(int x, int y, int act/* 1: Down, -1: Up */) { 
            if(null == m_parts || m_parts.length == 0)
                return;
            dotjoyenabled=true;
            dotx=x-cx;
            doty=y-cy;
            boolean inside = 4 * (dotx * dotx + doty * doty) <= size * size;
            
            switch(act)
            {
                case 1:
                    if(inside)
                        dotjoyenabled = true;
                    break;
                case -1:
                    if(dotjoyenabled)
                    {
                        if(!inside)
                        {
                            boolean has = false;
                            for(Part p : m_parts)
                            {
                                if(!has)
                                {
                                    if(p.pressed)
                                    {
                                        has = true;
                                        ((Q3EControlView)(view)).sendKeyEvent(true, p.key, 0);
                                        ((Q3EControlView)(view)).sendKeyEvent(false, p.key, 0);
                                    }
                                }
                                p.pressed = false;
                            }
                        }
                        else
                        {
                            for(Part p : m_parts)
                                p.pressed = false;
                        }
                    }
                    dotjoyenabled=false;    
                    break;
                case 0:
                default:
                    if(dotjoyenabled)
                    {
                        if(!inside)
                        {
                            float t = rad2deg(Math.atan2(doty, dotx) + Math.PI / 2);
                            boolean has = false;
                            for(Part p : m_parts)
                            {
                                boolean b = false;
                                if(!has)
                                {
                                    if(t >= p.start && t < p.end)
                                    {
                                        has = true;
                                        b = true;
                                    }
                                }
                                p.pressed = b;    
                            }
                        }
                        else
                        {
                            for(Part p : m_parts)
                                p.pressed = false;
                        }
                    }
                break;
            }
        }

        @Override
        public boolean isInside(int x, int y) {
            return 4*((cx-x)*(cx-x)+(cy-y)*(cy-y))<=size*size;
        }
        
        public static void Move(Disc target, Disc src)
        {
            target.tex_ind = src.tex_ind;
            target.m_parts = src.m_parts;
        }
        
        private static float rad2deg(double rad)
        {
            double deg = rad / Math.PI * 180.0;
            return FormatAngle((float)deg);
        }
        
        private static float FormatAngle(float deg)
        {
            while(deg > 360)
                deg -= 360;
            while(deg < 0)
                deg += 360.0;
            return deg;
        }
	}		
}
