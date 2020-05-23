/*
	Copyright (C) 2012 n0n3m4
	
    This file is part of RTCW4A.

    RTCW4A is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    RTCW4A is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with RTCW4A.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.n0n3m4.DIII4A;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.RandomAccessFile;
import java.io.UTFDataFormatException;
import java.nio.ByteBuffer;
import java.util.RandomAccess;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EMain;
import com.n0n3m4.q3e.Q3EUiConfig;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.DIII4A.*;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.TabHost;
import android.widget.TabHost.TabContentFactory;
import android.widget.Toast;
import android.app.ActivityManager;
import android.app.LauncherActivity;
import android.widget.RadioButton;

public class GameLauncher extends Activity{		
	
	final String default_gamedata="/sdcard/diii4a";
	
	final int UI_JOYSTICK=0;
	final int UI_SHOOT=1;
	final int UI_JUMP=2;
	final int UI_CROUCH=3;
	final int UI_RELOADBAR=4;	
	final int UI_PDA=5;
	final int UI_FLASHLIGHT=6;
	final int UI_SAVE=7;
	final int UI_1=8;
	final int UI_2=9;
	final int UI_3=10;	
	final int UI_KBD=11;
    final int UI_4 = 12;
    final int UI_5 = 13;
    final int UI_6 = 14;
	final int UI_SIZE=UI_6+1;
	
	public void getgl2progs(String destname) {
        try {            
            byte[] buf = new byte[4096];
            ZipInputStream zipinputstream = null;
            InputStream bis;
            ZipEntry zipentry;
                        
            bis=getAssets().open("gl2progs.zip");            
            zipinputstream=new ZipInputStream(bis);
            
            (new File(destname)).mkdirs();                               
            while ((zipentry= zipinputstream.getNextEntry())!=null) 
            {                	            	                
            	String tmpname=zipentry.getName();                	                	
            	
            	String entryName = destname + tmpname;
                entryName = entryName.replace('/', File.separatorChar);
                entryName = entryName.replace('\\', File.separatorChar);
            	                                    
                int n;
                FileOutputStream fileoutputstream;
                File newFile = new File(entryName);
                if (zipentry.isDirectory()) {
                    if (!newFile.mkdirs()) {
                    }                        
                    continue;
                }
                fileoutputstream = new FileOutputStream(entryName);               
                while ((n = zipinputstream.read(buf, 0, 4096)) > -1) {
                    fileoutputstream.write(buf, 0, n);                    
                }                                             
                fileoutputstream.close();                                                     
                zipinputstream.closeEntry();
            }

            zipinputstream.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
	
	public void InitQ3E(Context cnt, int width, int height)
	{			
		Q3EKeyCodes.InitD3Keycodes();
		Q3EInterface q3ei=new Q3EInterface();
		q3ei.isD3=true;
		q3ei.isD3BFG=true;
		q3ei.UI_SIZE=UI_SIZE;
		
		int r=Q3EUtils.dip2px(cnt,75);
		int rightoffset=r*3/4;
		int sliders_width=Q3EUtils.dip2px(cnt,125);
				
		q3ei.defaults_table=new String[UI_SIZE];
		q3ei.defaults_table[UI_JOYSTICK] =(r*4/3)+" "+(height-r*4/3)+" "+r+" "+30;
		q3ei.defaults_table[UI_SHOOT]    =(width-r/2-rightoffset)+" "+(height-r/2-rightoffset)+" "+r*3/2+" "+30;
		q3ei.defaults_table[UI_JUMP]     =(width-r/2)+" "+(height-r-2*rightoffset)+" "+r+" "+30;
		q3ei.defaults_table[UI_CROUCH]   =(width-r/2)+" "+(height-r/2)+" "+r+" "+30;
		q3ei.defaults_table[UI_RELOADBAR]=(width-sliders_width/2-rightoffset/3)+" "+(sliders_width*3/8)+" "+sliders_width+" "+30;		
		q3ei.defaults_table[UI_PDA]   =(width-r-2*rightoffset)+" "+(height-r/2)+" "+r+" "+30;
		q3ei.defaults_table[UI_FLASHLIGHT]     =(width-r/2-4*rightoffset)+" "+(height-r/2)+" "+r+" "+30;
		q3ei.defaults_table[UI_SAVE]     =sliders_width/2+" "+sliders_width/2+" "+sliders_width+" "+30;
        
		for (int i=UI_SAVE+1;i<UI_SIZE;i++)
			q3ei.defaults_table[i]=(r/2+r*(i-UI_SAVE-1))+" "+(height+r/2)+" "+r+" "+30;

        //k
        q3ei.defaults_table[UI_1] = String.format("%d %d %d %d", width - r / 2 - r * 2, (sliders_width * 5 / 8 + r / 2), r, 30);
        q3ei.defaults_table[UI_2] = String.format("%d %d %d %d", width - r / 2 - r, (sliders_width * 5 / 8 + r / 2), r, 30);
        q3ei.defaults_table[UI_3] = String.format("%d %d %d %d", width - r / 2, (sliders_width * 5 / 8 + r / 2), r, 30);
        q3ei.defaults_table[UI_KBD] = String.format("%d %d %d %d", sliders_width + r / 2, r / 2, r, 30);
		
		q3ei.arg_table=new int[UI_SIZE*4];
		q3ei.type_table=new int[UI_SIZE];		
		
		q3ei.type_table[UI_JOYSTICK]=Q3EUtils.TYPE_JOYSTICK;
		q3ei.type_table[UI_SHOOT]=Q3EUtils.TYPE_BUTTON;
		q3ei.type_table[UI_JUMP]=Q3EUtils.TYPE_BUTTON;
		q3ei.type_table[UI_CROUCH]=Q3EUtils.TYPE_BUTTON;
		q3ei.type_table[UI_RELOADBAR]=Q3EUtils.TYPE_SLIDER;		
		q3ei.type_table[UI_PDA]=Q3EUtils.TYPE_BUTTON;
		q3ei.type_table[UI_FLASHLIGHT]=Q3EUtils.TYPE_BUTTON;
		q3ei.type_table[UI_SAVE]=Q3EUtils.TYPE_SLIDER;
		for (int i=UI_SAVE+1;i<UI_SIZE;i++)
		q3ei.type_table[i]=Q3EUtils.TYPE_BUTTON;		
		
		q3ei.arg_table[UI_SHOOT*4]=Q3EKeyCodes.KeyCodes.K_MOUSE1;
		q3ei.arg_table[UI_SHOOT*4+1]=0;
		q3ei.arg_table[UI_SHOOT*4+2]=0;
		q3ei.arg_table[UI_SHOOT*4+3]=0;
		
		
		q3ei.arg_table[UI_JUMP*4]=Q3EKeyCodes.KeyCodes.K_SPACE;
		q3ei.arg_table[UI_JUMP*4+1]=0;
		q3ei.arg_table[UI_JUMP*4+2]=0;
		q3ei.arg_table[UI_JUMP*4+3]=0;
		
		q3ei.arg_table[UI_CROUCH*4]=Q3EKeyCodes.KeyCodesD3BFG.K_C;
		q3ei.arg_table[UI_CROUCH*4+1]=1;
		q3ei.arg_table[UI_CROUCH*4+2]=1;
		q3ei.arg_table[UI_CROUCH*4+3]=0;
		
		q3ei.arg_table[UI_RELOADBAR*4]=93;
		q3ei.arg_table[UI_RELOADBAR*4+1]=114;
		q3ei.arg_table[UI_RELOADBAR*4+2]=91;
		q3ei.arg_table[UI_RELOADBAR*4+3]=0;
		
		q3ei.arg_table[UI_PDA*4]=Q3EKeyCodes.KeyCodes.K_TAB;
		q3ei.arg_table[UI_PDA*4+1]=0;
		q3ei.arg_table[UI_PDA*4+2]=0;
		q3ei.arg_table[UI_PDA*4+3]=0;
		
		q3ei.arg_table[UI_FLASHLIGHT*4]=Q3EKeyCodes.KeyCodesD3BFG.K_F;
		q3ei.arg_table[UI_FLASHLIGHT*4+1]=0;
		q3ei.arg_table[UI_FLASHLIGHT*4+2]=0;
		q3ei.arg_table[UI_FLASHLIGHT*4+3]=0;
		
		q3ei.arg_table[UI_SAVE*4]=Q3EKeyCodes.KeyCodes.K_F5;
		q3ei.arg_table[UI_SAVE*4+1]=Q3EKeyCodes.KeyCodes.K_ESCAPE;
		q3ei.arg_table[UI_SAVE*4+2]=Q3EKeyCodes.KeyCodes.K_F9;
		q3ei.arg_table[UI_SAVE*4+3]=1;
		
		q3ei.arg_table[UI_1*4]=Q3EKeyCodes.KeyCodesD3BFG.K_1;
		q3ei.arg_table[UI_1*4+1]=0;
		q3ei.arg_table[UI_1*4+2]=0;
		q3ei.arg_table[UI_1*4+3]=0;
		
		q3ei.arg_table[UI_2*4]=Q3EKeyCodes.KeyCodesD3BFG.K_2;
		q3ei.arg_table[UI_2*4+1]=0;
		q3ei.arg_table[UI_2*4+2]=0;
		q3ei.arg_table[UI_2*4+3]=0;
		
		q3ei.arg_table[UI_3*4]=Q3EKeyCodes.KeyCodesD3BFG.K_3;
		q3ei.arg_table[UI_3*4+1]=0;
		q3ei.arg_table[UI_3*4+2]=0;
		q3ei.arg_table[UI_3*4+3]=0;
		
		q3ei.arg_table[UI_KBD*4]=Q3EUtils.K_VKBD;
		q3ei.arg_table[UI_KBD*4+1]=0;
		q3ei.arg_table[UI_KBD*4+2]=0;
		q3ei.arg_table[UI_KBD*4+3]=0;

        for(int i = UI_4; i <= UI_6; i++)
        {
            q3ei.arg_table[i*4]=Q3EKeyCodes.KeyCodesD3BFG.K_4 + (i - UI_4);
            q3ei.arg_table[i*4+1]=0;
            q3ei.arg_table[i*4+2]=0;
            q3ei.arg_table[i*4+3]=0;   
        }
		
		q3ei.default_path=default_gamedata;		
		if (Q3EJNI.detectNeon())
		q3ei.libname="libdante_neon.so";
		else
		q3ei.libname="libdante.so";
		q3ei.texture_table=new String[UI_SIZE];
		q3ei.texture_table[UI_JOYSTICK]="";
		q3ei.texture_table[UI_SHOOT]="btn_sht.png";
		q3ei.texture_table[UI_JUMP]="btn_jump.png";
		q3ei.texture_table[UI_CROUCH]="btn_crouch.png";
		q3ei.texture_table[UI_RELOADBAR]="btn_reload.png";
		q3ei.texture_table[UI_PDA]="btn_pda.png";
		q3ei.texture_table[UI_FLASHLIGHT]="btn_flashlight.png";
		q3ei.texture_table[UI_SAVE]="btn_pause.png";		
		q3ei.texture_table[UI_1]="btn_1.png";
		q3ei.texture_table[UI_2]="btn_2.png";
		q3ei.texture_table[UI_3]="btn_3.png";
		q3ei.texture_table[UI_KBD]="btn_keyboard.png";
		q3ei.texture_table[UI_4]="btn_foot.png";
		q3ei.texture_table[UI_5]="btn_light.png";
		q3ei.texture_table[UI_6]="btn_telescope.png";
		
		Q3EUtils.q3ei=q3ei;
	}
	
	public void DonateDialog()
	{
		AlertDialog.Builder bldr=new AlertDialog.Builder(this);
		bldr.setTitle("Do you want to support the developer?");
		bldr.setPositiveButton("Donate by PayPal", new AlertDialog.OnClickListener() {			
			@Override
			public void onClick(DialogInterface dialog, int which) {
				Intent ppIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=kosleb1169%40gmail%2ecom&lc=US&item_name=n0n3m4&no_note=0&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHostedGuest"));
				ppIntent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
				startActivity(ppIntent);				
				dialog.dismiss();
			}
		});
		bldr.setNegativeButton("Don't ask", new AlertDialog.OnClickListener() {			
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
			}
		});
		bldr.setNeutralButton("More apps by n0n3m4", new AlertDialog.OnClickListener() {			
			@Override
			public void onClick(DialogInterface dialog, int which) {
				Intent marketIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("market://search?q=pub:n0n3m4"));
				marketIntent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
				startActivity(marketIntent);				
				dialog.dismiss();
			}
		});
		AlertDialog dl=bldr.create();
		dl.setCancelable(false);
		dl.show();		
	}
	
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		Q3EUtils.LoadAds(this);
		super.onConfigurationChanged(newConfig);
	}
	
	public void support(View vw)
	{
		DonateDialog();
	}
	
	public void UpdateMouseMenu(boolean show)
	{
		((LinearLayout)findViewById(R.id.layout_mouseconfig)).setVisibility(show?LinearLayout.VISIBLE:LinearLayout.GONE);
	}
	
	public void UpdateMouseManualMenu(boolean show)
	{
		((LinearLayout)findViewById(R.id.layout_manualmouseconfig)).setVisibility(show?LinearLayout.VISIBLE:LinearLayout.GONE);
	}
	
	public void SelectCheckbox(int cbid,int id)
	{
		RadioGroup rg=(RadioGroup)findViewById(cbid);
		rg.check(rg.getChildAt(id).getId());
	}
	
	public int GetIdCheckbox(int cbid)
	{
		RadioGroup rg=(RadioGroup)findViewById(cbid);
		return rg.indexOfChild(findViewById(rg.getCheckedRadioButtonId()));
	}
	
	public boolean getProp(String name)
	{
		name=" +set "+name+" ";
		String str=((EditText)findViewById(R.id.edt_cmdline)).getText().toString();
		if (str.contains(name))
		{
			str=str.substring(str.indexOf(name)+name.length());
			if (str.startsWith("0"))
				return false;			
		}
		return true;
	}
	
	public void setProp(String name,boolean val)
	{
		name=" +set "+name+" ";
		String str=((EditText)findViewById(R.id.edt_cmdline)).getText().toString();
		if (str.contains(name))
		{
			String bef=str.substring(0,str.indexOf(name));
			String aft=str.substring(str.indexOf(name)+name.length());
				if (aft.length()>0) aft=aft.substring(1);
			str=bef+aft;		
		}		
		str+=name+((val)?"1":"0");	
		((EditText)findViewById(R.id.edt_cmdline)).setText(str);
	}
	
	public void updatehacktings()
	{
		((CheckBox)findViewById(R.id.usedxt)).setChecked(getProp("r_useDXT"));
		((CheckBox)findViewById(R.id.useetc1)).setChecked(getProp("r_useETC1"));
		((CheckBox)findViewById(R.id.useetc1cache)).setChecked(getProp("r_useETC1cache"));
		((CheckBox)findViewById(R.id.nolight)).setChecked(getProp("r_noLight"));
        
        String str = GetProp("r_harmClearVertexBuffer");
        if(str != null)
        {
            int index = Integer.valueOf(str);
            if(index < 0 || index > 2)
                index = 2;
            RadioGroup group = (RadioGroup)findViewById(R.id.r_harmclearvertexbuffer);
            group.check(group.getChildAt(index).getId());
        }
	}		
	
	public void onCreate(Bundle savedInstanceState) 
	{		
		super.onCreate(savedInstanceState);				
		setContentView(R.layout.main);
		
		Display display = getWindowManager().getDefaultDisplay(); 
		int width = Math.max(display.getWidth(),display.getHeight());
		int height = Math.min(display.getWidth(),display.getHeight());						
		
		InitQ3E(this, width, height);
		
		TabHost th=(TabHost)findViewById(R.id.tabhost);
		th.setup();					
	    th.addTab(th.newTabSpec("tab1").setIndicator("General").setContent(R.id.launcher_tab1));	    
	    th.addTab(th.newTabSpec("tab2").setIndicator("Controls").setContent(R.id.launcher_tab2));	    
	    th.addTab(th.newTabSpec("tab3").setIndicator("Graphics").setContent(R.id.launcher_tab3));		
		SharedPreferences mPrefs=PreferenceManager.getDefaultSharedPreferences(this);					
		
		((EditText)findViewById(R.id.edt_cmdline)).setText(mPrefs.getString(Q3EUtils.pref_params, "game.arm"));
		((EditText)findViewById(R.id.edt_mouse)).setText(mPrefs.getString(Q3EUtils.pref_eventdev, "/dev/input/event???"));
		((EditText)findViewById(R.id.edt_path)).setText(mPrefs.getString(Q3EUtils.pref_datapath, default_gamedata));
		((CheckBox)findViewById(R.id.hideonscr)).setOnCheckedChangeListener(new OnCheckedChangeListener() {			
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				UpdateMouseMenu(isChecked);
			}
		});
		((CheckBox)findViewById(R.id.hideonscr)).setChecked(mPrefs.getBoolean(Q3EUtils.pref_hideonscr, false));
		
		UpdateMouseMenu(((CheckBox)findViewById(R.id.hideonscr)).isChecked());
				
		((CheckBox)findViewById(R.id.mapvol)).setChecked(mPrefs.getBoolean(Q3EUtils.pref_mapvol, false));
		((CheckBox)findViewById(R.id.secfinglmb)).setChecked(mPrefs.getBoolean(Q3EUtils.pref_2fingerlmb, false));
		((CheckBox)findViewById(R.id.smoothjoy)).setChecked(mPrefs.getBoolean(Q3EUtils.pref_analog, true));
		((CheckBox)findViewById(R.id.detectmouse)).setOnCheckedChangeListener(new OnCheckedChangeListener() {			
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				UpdateMouseManualMenu(!isChecked);
			}
		});						
		((CheckBox)findViewById(R.id.detectmouse)).setChecked(mPrefs.getBoolean(Q3EUtils.pref_detectmouse, true));
		
		UpdateMouseManualMenu(!((CheckBox)findViewById(R.id.detectmouse)).isChecked());
		
		SelectCheckbox(R.id.rg_curpos,mPrefs.getInt(Q3EUtils.pref_mousepos, 3));
		SelectCheckbox(R.id.rg_scrres,mPrefs.getInt(Q3EUtils.pref_scrres, 0));
		SelectCheckbox(R.id.rg_msaa,mPrefs.getInt(Q3EUtils.pref_msaa, 0));
        //k
		SelectCheckbox(R.id.rg_color_bits, mPrefs.getInt(Q3EUtils.pref_harm_16bit, -1) + 1);
		SelectCheckbox(R.id.rg_run_background, mPrefs.getInt(Q3EUtils.pref_harm_run_background, 0));
        ((CheckBox)findViewById(R.id.setting_render_mem_status)).setChecked(mPrefs.getBoolean(Q3EUtils.pref_harm_render_mem_status, false));
		SelectCheckbox(R.id.r_harmclearvertexbuffer, mPrefs.getInt(Q3EUtils.pref_harm_r_harmclearvertexbuffer, 2));
        ((RadioGroup)findViewById(R.id.r_harmclearvertexbuffer)).setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener(){
           public void onCheckedChanged(RadioGroup group, int val)
           {
               if(val != -1)
               SetProp("r_harmClearVertexBuffer", group.indexOfChild(findViewById(val)));
           }
        });
		
		((EditText)findViewById(R.id.res_x)).setText(mPrefs.getString(Q3EUtils.pref_resx, "640"));
		((EditText)findViewById(R.id.res_y)).setText(mPrefs.getString(Q3EUtils.pref_resy, "480"));
		
		//DIII4A-specific					
		((EditText)findViewById(R.id.edt_cmdline)).addTextChangedListener(new TextWatcher() {			
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				updatehacktings();
			}			
			public void beforeTextChanged(CharSequence s, int start, int count,int after) {}			
			public void afterTextChanged(Editable s) {}
		});
		((CheckBox)findViewById(R.id.usedxt)).setOnCheckedChangeListener(new OnCheckedChangeListener() {			
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {				
				setProp("r_useDXT", isChecked);
			}
		});
		((CheckBox)findViewById(R.id.useetc1)).setOnCheckedChangeListener(new OnCheckedChangeListener() {			
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {				
				setProp("r_useETC1", isChecked);
			}
		});
		((CheckBox)findViewById(R.id.useetc1cache)).setOnCheckedChangeListener(new OnCheckedChangeListener() {			
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {				
				setProp("r_useETC1cache", isChecked);
			}
		});
		((CheckBox)findViewById(R.id.nolight)).setOnCheckedChangeListener(new OnCheckedChangeListener() {			
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {				
				setProp("r_noLight", isChecked);
			}
		});
		updatehacktings();
		
		Q3EUtils.LoadAds(this);
        
       // Log.e("maxxxxx", "" +Runtime.getRuntime().maxMemory()/1024/1024);
       // ActivityManager activityManager = (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
       //Toast.makeText(this, activityManager.getMemoryClass() + " " + activityManager.getLargeMemoryClass(), Toast.LENGTH_SHORT).show();
	}
	
	public void start(View vw)
	{
		Editor mEdtr=PreferenceManager.getDefaultSharedPreferences(this).edit();
		mEdtr.putString(Q3EUtils.pref_params, ((EditText)findViewById(R.id.edt_cmdline)).getText().toString());
		mEdtr.putString(Q3EUtils.pref_eventdev, ((EditText)findViewById(R.id.edt_mouse)).getText().toString());
		mEdtr.putString(Q3EUtils.pref_datapath, ((EditText)findViewById(R.id.edt_path)).getText().toString());
		mEdtr.putBoolean(Q3EUtils.pref_hideonscr, ((CheckBox)findViewById(R.id.hideonscr)).isChecked());
		//k mEdtr.putBoolean(Q3EUtils.pref_32bit, true);
        int index = GetIdCheckbox(R.id.rg_color_bits) - 1;
		mEdtr.putBoolean(Q3EUtils.pref_32bit, index == -1);
		mEdtr.putInt(Q3EUtils.pref_harm_16bit, index);
		mEdtr.putInt(Q3EUtils.pref_harm_run_background, GetIdCheckbox(R.id.rg_run_background));
		mEdtr.putBoolean(Q3EUtils.pref_harm_render_mem_status, ((CheckBox)findViewById(R.id.setting_render_mem_status)).isChecked());
		mEdtr.putInt(Q3EUtils.pref_harm_r_harmclearvertexbuffer, GetIdCheckbox(R.id.r_harmclearvertexbuffer));
        
		mEdtr.putBoolean(Q3EUtils.pref_analog, true);
		mEdtr.putBoolean(Q3EUtils.pref_mapvol, ((CheckBox)findViewById(R.id.mapvol)).isChecked());
		mEdtr.putBoolean(Q3EUtils.pref_analog, ((CheckBox)findViewById(R.id.smoothjoy)).isChecked());
		mEdtr.putBoolean(Q3EUtils.pref_2fingerlmb, ((CheckBox)findViewById(R.id.secfinglmb)).isChecked());
		mEdtr.putBoolean(Q3EUtils.pref_detectmouse, ((CheckBox)findViewById(R.id.detectmouse)).isChecked());
		mEdtr.putInt(Q3EUtils.pref_mousepos, GetIdCheckbox(R.id.rg_curpos));
		mEdtr.putInt(Q3EUtils.pref_scrres, GetIdCheckbox(R.id.rg_scrres));
		mEdtr.putInt(Q3EUtils.pref_msaa, GetIdCheckbox(R.id.rg_msaa));
		mEdtr.putString(Q3EUtils.pref_resx, ((EditText)findViewById(R.id.res_x)).getText().toString());
		mEdtr.putString(Q3EUtils.pref_resy, ((EditText)findViewById(R.id.res_y)).getText().toString());
		mEdtr.commit();
		
		String dir=((EditText)findViewById(R.id.edt_path)).getText().toString();
		if ((new File(dir+"/base").exists())&&(!new File(dir+"/base/gl2progs").exists()))
		getgl2progs(dir+"/base/");
		
		finish();
		startActivity(new Intent(this,Q3EMain.class));
	}
	
	public void resetcontrols(View vw)
	{
		Editor mEdtr=PreferenceManager.getDefaultSharedPreferences(this).edit();
		for (int i=0;i<UI_SIZE;i++)
			mEdtr.putString(Q3EUtils.pref_controlprefix+i, null);
		mEdtr.commit();
	}
	
	public void controls(View vw)
	{		
		startActivity(new Intent(this,Q3EUiConfig.class));
	}

    //k
    public void SetProp(String name, int val)
    {
        name=" +set "+name+" ";
        String str=((EditText)findViewById(R.id.edt_cmdline)).getText().toString();
        if (str.contains(name))
        {
            String bef=str.substring(0,str.indexOf(name));
            String aft=str.substring(str.indexOf(name)+name.length());
            if (aft.length()>0) aft=aft.substring(1);
            str=bef+aft;        
        }       
        str+=name+ val;  
        ((EditText)findViewById(R.id.edt_cmdline)).setText(str);
	}

    public String GetProp(String name)
    {
        name=" +set "+name+" ";
        String str=((EditText)findViewById(R.id.edt_cmdline)).getText().toString();
        if (str.contains(name))
        {
            int start = str.indexOf(name) + name.length();
            int end = str.indexOf(' ', start);
            if(end != -1)
                str=str.substring(start, end);
            else
                str = str.substring(start);
            return str;
        }
        return null;
	}
}
