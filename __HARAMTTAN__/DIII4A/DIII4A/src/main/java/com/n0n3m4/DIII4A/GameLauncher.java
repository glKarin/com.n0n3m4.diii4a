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

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioGroup;
import android.widget.TabHost;
import android.widget.TextView;
import android.widget.Toast;
import com.harmattan.DIII4APlusPlus.ContextUtility;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EMain;
import com.n0n3m4.q3e.Q3EUiConfig;
import com.n0n3m4.q3e.Q3EUtils;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import android.widget.RadioButton;
import com.harmattan.DIII4APlusPlus.TextHelper;
import com.harmattan.DIII4APlusPlus.UncaughtExceptionHandler;
import com.harmattan.DIII4APlusPlus.FileUtility;
import com.harmattan.DIII4APlusPlus.FileBrowserDialog;
import com.harmattan.DIII4APlusPlus.Constants;
import com.harmattan.DIII4APlusPlus.DebugDialog;
import com.harmattan.DIII4APlusPlus.LauncherSettingsDialog;
import com.harmattan.DIII4APlusPlus.ConfigEditorActivity;

public class GameLauncher extends Activity{		
    private static final int CONST_REQUEST_EXTERNAL_STORAGE_FOR_START_RESULT_CODE = 1;
    private static final int CONST_REQUEST_EXTERNAL_STORAGE_FOR_EDIT_CONFIG_FILE_RESULT_CODE = 2;
    private static final int CONST_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_FOLDER_RESULT_CODE = 3;
     
	private ViewHolder V = new ViewHolder();
    private boolean m_cmdUpdateLock = false;
	private CompoundButton.OnCheckedChangeListener m_checkboxChangeListener = new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				switch(buttonView.getId())
				{
					case R.id.useetc1cache:
						setProp("r_useETC1cache", isChecked);
						break;
					case R.id.nolight:
						setProp("r_noLight", isChecked);
						break;
					case R.id.useetc1:
						setProp("r_useETC1", isChecked);
						break;
					case R.id.usedxt:
						setProp("r_useDXT", isChecked);
						break;
					case R.id.detectmouse:
						UpdateMouseManualMenu(!isChecked);
						break;
					case R.id.hideonscr:
						UpdateMouseMenu(isChecked);
						break;
					default:
						break;
				}
			}
		};
	private RadioGroup.OnCheckedChangeListener m_groupCheckChangeListener = new RadioGroup.OnCheckedChangeListener() {
		@Override
		public void onCheckedChanged(RadioGroup radioGroup, int id)
		{
			switch(radioGroup.getId())
			{
				case R.id.rg_scrres:
					GameLauncher.this.UpdateCustomerResulotion(id == R.id.res_custom);
					break;
				case R.id.r_harmclearvertexbuffer:
					SetProp("harm_r_clearVertexBuffer", GetCheckboxIndex(radioGroup, id));
					break;
                case R.id.rg_harm_r_lightModel:
                    SetProp("harm_r_lightModel", GetCheckboxIndex(radioGroup, id) == 1 ? "blinn_phong" : "phong");
					break;
				case R.id.rg_fs_game:
					SetGameDLL(id);
					break;
				default:
					break;
			}
		}
	};
	private View.OnClickListener m_buttonClickListener = new View.OnClickListener(){
        @Override
		public void onClick(View view)
		{
			switch(view.getId())
			{
				case R.id.launcher_tab1_edit_autoexec:
					EditFile("autoexec.cfg");
					break;
				case R.id.launcher_tab1_edit_doomconfig:
                    EditFile("DoomConfig.cfg");
					break;
				case R.id.launcher_tab1_game_lib_button:
                    OpenGameLibChooser();
					break;
				case R.id.launcher_tab1_game_data_chooser_button:
					OpenFolderChooser();
					break;
				default:
					break;
			}
		}
	};
	
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
    final int UI_CONSOLE = 12;
    final int UI_4 = 13;
    final int UI_5 = 14;
    final int UI_6 = 15;
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
        final int sr = r / 6 * 5;
        q3ei.defaults_table[UI_1] = String.format("%d %d %d %d", width - sr / 2 - sr * 2, (sliders_width * 5 / 8 + sr / 2), sr, 30);
        q3ei.defaults_table[UI_2] = String.format("%d %d %d %d", width - sr / 2 - sr, (sliders_width * 5 / 8 + sr / 2), sr, 30);
        q3ei.defaults_table[UI_3] = String.format("%d %d %d %d", width - sr / 2, (sliders_width * 5 / 8 + sr / 2), sr, 30);
        q3ei.defaults_table[UI_KBD] = String.format("%d %d %d %d", sliders_width + sr / 2, sr / 2, sr, 30);
        q3ei.defaults_table[UI_CONSOLE] = String.format("%d %d %d %d", sliders_width / 2 + sr / 2, sliders_width / 2 + sr / 2, sr, 30);
		
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

        q3ei.arg_table[UI_CONSOLE*4]=Q3EKeyCodes.KeyCodesD3.K_CONSOLE;
        q3ei.arg_table[UI_CONSOLE*4+1]=0;
        q3ei.arg_table[UI_CONSOLE*4+2]=0;
		q3ei.arg_table[UI_CONSOLE*4+3]=0;

        final int start = UI_4;
        for(int i = start; i <= UI_6; i++)
        {
            q3ei.arg_table[i*4]=Q3EKeyCodes.KeyCodesD3BFG.K_4 + (i - start);
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
		q3ei.texture_table[UI_CONSOLE]="btn_notepad.png";
		q3ei.texture_table[UI_4]="btn_activate.png";
		q3ei.texture_table[UI_5]="btn_binocular.png";
		q3ei.texture_table[UI_6]="btn_kick.png";
		
		Q3EUtils.q3ei=q3ei;
	}
	
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		Q3EUtils.LoadAds(this);
		super.onConfigurationChanged(newConfig);
	}
	
	public void support(View vw)
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
	
	public void UpdateMouseMenu(boolean show)
	{
		V.layout_mouseconfig.setVisibility(show?LinearLayout.VISIBLE:LinearLayout.GONE);
	}
	
	public void UpdateMouseManualMenu(boolean show)
	{
		V.layout_manualmouseconfig.setVisibility(show?LinearLayout.VISIBLE:LinearLayout.GONE);
	}
	
	public void SelectCheckbox(RadioGroup rg, int index)
	{
        for(int i = 0, j = 0; i < rg.getChildCount(); i++)
        {
            View item = rg.getChildAt(i);
            if(item instanceof RadioButton)
            {
                if(j == index)
                {
                    rg.check(item.getId());
                    return;
                }
                j++;
            }
        }
		//rg.check(rg.getChildAt(index).getId());
	}


    public int GetCheckboxIndex(RadioGroup rg)
    {
        return GetCheckboxIndex(rg, rg.getCheckedRadioButtonId());
    }
	
	public int GetCheckboxIndex(RadioGroup rg, int id)
	{
        for(int i = 0, j = 0; i < rg.getChildCount(); i++)
        {
            View item = rg.getChildAt(i);
            if(item instanceof RadioButton)
            {
                if(item.getId() == id)
                {
                    return j;
                }
                j++;
            }
        }
        return -1;
		//return rg.indexOfChild(findViewById(rg.getCheckedRadioButtonId()));
	}

    public int GetCheckboxId(RadioGroup rg, int index)
    {
        for(int i = 0, j = 0; i < rg.getChildCount(); i++)
        {
            View item = rg.getChildAt(i);
            if(item instanceof RadioButton)
            {
                if(j == index)
                {
                    return item.getId();
                }
                j++;
            }
        }
        return -1;
        //return rg.getChildAt(index).getId();
	}
	
	public boolean getProp(String name)
	{
        return "1".equals(GetProp(name));
	}
    
	public void setProp(String name,boolean val)
	{
        SetProp(name, val ? "1" : "0");
	}
	
	public void updatehacktings()
	{
        //k
		V.usedxt.setChecked(getProp("r_useDXT", false));
		V.useetc1.setChecked(getProp("r_useETC1", false));
		V.useetc1cache.setChecked(getProp("r_useETC1cache", false));
		V.nolight.setChecked(getProp("r_noLight", false));
        
        //k fill commandline
        if(!IsProp("r_useDXT")) setProp("r_useDXT", false);
        if(!IsProp("r_useETC1")) setProp("r_useETC1", false);
        if(!IsProp("r_useETC1cache")) setProp("r_useETC1cache", false);
        if(!IsProp("r_noLight")) setProp("r_noLight", false);
        
        String str = GetProp("harm_r_clearVertexBuffer");
        int index = 2;
        if(str != null)
        {
            try
            {
                index = Integer.parseInt(str);   
            }
            catch(Exception e)
            {
                e.printStackTrace();
            }
            if(index < 0 || index > 2)
                index = 2;
        }
        else
            SetProp("harm_r_clearVertexBuffer", 2);
        SelectCheckbox(V.r_harmclearvertexbuffer, index);
        if(!IsProp("harm_r_clearVertexBuffer")) SetProp("harm_r_clearVertexBuffer", 2);
        
        str = GetProp("harm_r_lightModel");
        index = 0;
        if(str != null)
        {
            if("blinn_phong".equalsIgnoreCase(str))
                index = 1;
        }
        SelectCheckbox(V.rg_harm_r_lightModel, index);
        if(!IsProp("harm_r_lightModel")) SetProp("harm_r_lightModel", "phong");
        
        index = 0;
        str = GetProp("fs_game");
        if(str != null)
        {
            if("".equals(str))
                index = 0;
            else if("d3xp".equals(str))
                index = 1;
            else if("cdoom".equals(str))
                index = 2;
            else if("d3le".equals(str))
                index = 3;
            else
                index = 4;
        }
        if(index == 4)
        {
            EditText edit = V.edt_fs_game;
            String cur = edit.getText().toString();
            if(!str.equals(cur))
                edit.setText(str);
        }
		SelectCheckbox(V.rg_fs_game, index);
	}		
	
	public void onCreate(Bundle savedInstanceState) 
	{		
		super.onCreate(savedInstanceState);				
        //k
        HandleUnexperted();
        setTitle(R.string.app_title);
		SharedPreferences mPrefs=PreferenceManager.getDefaultSharedPreferences(this);
        ContextUtility.SetScreenOrientation(this, mPrefs.getBoolean(Constants.PreferenceKey.LAUNCHER_ORIENTATION, false) ? 0 : 1);
        
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

		V.Setup();
		
		V.edt_cmdline.setText(mPrefs.getString(Q3EUtils.pref_params, "game.arm"));
		V.edt_mouse.setText(mPrefs.getString(Q3EUtils.pref_eventdev, "/dev/input/event???"));
		V.edt_path.setText(mPrefs.getString(Q3EUtils.pref_datapath, default_gamedata));
		V.hideonscr.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.hideonscr.setChecked(mPrefs.getBoolean(Q3EUtils.pref_hideonscr, false));
		
		UpdateMouseMenu(V.hideonscr.isChecked());
				
		V.mapvol.setChecked(mPrefs.getBoolean(Q3EUtils.pref_mapvol, false));
		V.secfinglmb.setChecked(mPrefs.getBoolean(Q3EUtils.pref_2fingerlmb, false));
		V.smoothjoy.setChecked(mPrefs.getBoolean(Q3EUtils.pref_analog, true));
		V.detectmouse.setOnCheckedChangeListener(m_checkboxChangeListener);						
		V.detectmouse.setChecked(mPrefs.getBoolean(Q3EUtils.pref_detectmouse, true));
		
		UpdateMouseManualMenu(!V.detectmouse.isChecked());
		
		SelectCheckbox(V.rg_curpos,mPrefs.getInt(Q3EUtils.pref_mousepos, 3));
        V.rg_scrres.setOnCheckedChangeListener(m_groupCheckChangeListener);
		SelectCheckbox(V.rg_scrres,mPrefs.getInt(Q3EUtils.pref_scrres, 0));
		SelectCheckbox(V.rg_msaa,mPrefs.getInt(Q3EUtils.pref_msaa, 0));
        //k
		SelectCheckbox(V.rg_color_bits, mPrefs.getInt(Q3EUtils.pref_harm_16bit, -1) + 1);
		SelectCheckbox(V.r_harmclearvertexbuffer, mPrefs.getInt(Q3EUtils.pref_harm_r_harmclearvertexbuffer, 2));
        V.r_harmclearvertexbuffer.setOnCheckedChangeListener(m_groupCheckChangeListener);
		SelectCheckbox(V.rg_harm_r_lightModel, "blinn_phong".equalsIgnoreCase(mPrefs.getString(Q3EUtils.pref_harm_r_lightModel, "phong")) ? 1 : 0);
        V.rg_harm_r_lightModel.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.edt_cmdline.setOnEditorActionListener(new TextView.OnEditorActionListener(){
           public boolean onEditorAction(TextView view, int id, KeyEvent ev)
           {
               if(ev.getKeyCode() == KeyEvent.KEYCODE_ENTER)
               {
                   if(ev.getAction() == KeyEvent.ACTION_UP)
                   {
                       V.edt_path.requestFocus();
                   }
                   return true;
               }
               return false;
           }
        });
        V.launcher_tab1_edit_autoexec.setOnClickListener(m_buttonClickListener);
        V.launcher_tab1_edit_doomconfig.setOnClickListener(m_buttonClickListener);
        String game = GetProp("fs_game");
        if(game != null)
        {
            int index = 0;
            if(game.isEmpty() || "base".equals(game))
                index = 0;
            else if("d3xp".equals(game))
                index = 1;
            else if("cdoom".equals(game))
                index = 2;
            else if("d3le".equals(game))
                index = 3;
            else
                index = 4;
            SelectCheckbox(V.rg_fs_game, index);
        }
        V.launcher_tab1_game_lib_button.setEnabled(V.rg_fs_game.getCheckedRadioButtonId() == R.id.fs_game_user);
        V.rg_fs_game.setOnCheckedChangeListener(m_groupCheckChangeListener);
        V.edt_fs_game.addTextChangedListener(new TextWatcher() {           
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    if(V.rg_fs_game.getCheckedRadioButtonId() == R.id.fs_game_user)
                        SetProp("fs_game", s);
                }           
                public void beforeTextChanged(CharSequence s, int start, int count,int after) {}            
                public void afterTextChanged(Editable s) {}
            });
        V.launcher_tab1_game_lib_button.setOnClickListener(m_buttonClickListener);
		V.edt_harm_r_specularExponent.setText(Float.toString(mPrefs.getFloat(Q3EUtils.pref_harm_r_specularExponent, 4.0f)));
        V.edt_harm_r_specularExponent.addTextChangedListener(new TextWatcher() {           
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    SetProp("harm_r_specularExponent", s);
                }           
                public void beforeTextChanged(CharSequence s, int start, int count,int after) {}            
                public void afterTextChanged(Editable s) {}
            });
		
		V.res_x.setText(mPrefs.getString(Q3EUtils.pref_resx, "640"));
		V.res_y.setText(mPrefs.getString(Q3EUtils.pref_resy, "480"));
        V.launcher_tab1_game_data_chooser_button.setOnClickListener(m_buttonClickListener);
		
		//DIII4A-specific					
		V.edt_cmdline.addTextChangedListener(new TextWatcher() {			
			public void onTextChanged(CharSequence s, int start, int before, int count) {
                boolean cond = V.edt_cmdline.isInputMethodTarget() && !IsCmdUpdateLocked();
                if(cond)
				    updatehacktings();
			}			
			public void beforeTextChanged(CharSequence s, int start, int count,int after) {}			
			public void afterTextChanged(Editable s) {}
		});
		V.usedxt.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.useetc1.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.useetc1cache.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.nolight.setOnCheckedChangeListener(m_checkboxChangeListener);
		updatehacktings();
		
		Q3EUtils.LoadAds(this);
        
        OpenUpdate();
	}
	
	public void start(View vw)
	{
		//k
        WritePreferences();
        /*
		String dir=V.edt_path.getText().toString();
		if ((new File(dir+"/base").exists())&&(!new File(dir+"/base/gl2progs").exists()))
		getgl2progs(dir+"/base/");
        */
        
        //k check external storage permission
        int res = ContextUtility.CheckPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE, CONST_REQUEST_EXTERNAL_STORAGE_FOR_START_RESULT_CODE);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast.makeText(this, "Can't start game!\nRead/Write external storage permission is not granted!", Toast.LENGTH_LONG).show();
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
		
		finish();
		startActivity(new Intent(this,Q3EMain.class));
	}
	
	public void resetcontrols(View vw)
	{
        ContextUtility.Confirm(this, "Warning", "Reset on-screen controls?", new Runnable() {
            public void run()
            {
                SharedPreferences.Editor mEdtr=PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit();
                for (int i=0;i<UI_SIZE;i++)
                    mEdtr.putString(Q3EUtils.pref_controlprefix+i, null);
                mEdtr.commit();
                Toast.makeText(GameLauncher.this, "On-screen controls has reset.", Toast.LENGTH_SHORT).show();
            }
        }, null);
	}
	
	public void controls(View vw)
	{		
		startActivity(new Intent(this,Q3EUiConfig.class));
	}

    //k
    public boolean getProp(String name, boolean defaultValueIfNotExists)
    {
        String val = GetProp(name);
        if(val != null && !val.trim().isEmpty())
            return "1".equals(val);
        return defaultValueIfNotExists;
	}
    
    private void SetProp(String name, Object val)
    {
        boolean lock = LockCmdUpdate();
        name = " +set " + name + " ";
        String str = GetCmdText();
		String insertCmd = val.toString().trim();
        if (str.contains(name))
        {
            int start = str.indexOf(name) + name.length();
            int end = str.indexOf(' ', start);
            if(end != -1)
            {
                String next = str.substring(start, end).trim();
                if("+set".equals(next))
                    str = str.substring(0, start) + insertCmd + str.substring(start);
                else
                    str = str.substring(0, start) + insertCmd + str.substring(end);
            }
            else
                str = str.substring(0, start) + insertCmd;
        }
        else
            str += name + insertCmd;
        SetCmdText(str);
        if(lock) UnlockCmdUpdate();
	}

    private String GetProp(String name)
    {
        name = " +set " + name + " ";
        String str = GetCmdText();
        if (str.contains(name))
        {
            int start = str.indexOf(name) + name.length();
            int end = str.indexOf(' ', start);
            if(end != -1)
            {
                String val = str.substring(start, end).trim();
                if("+set".equals(val))
                    return null;
                else
                    str = val;
            }
            else
                str = str.substring(start).trim();
            return str;
        }
        return null;
	}

    private boolean RemoveProp(String name)
    {
        boolean lock = LockCmdUpdate();
        name = " +set " + name + " ";
        EditText edit = V.edt_cmdline;
        String str = edit.getText().toString();
        boolean res = false;
        if (str.contains(name))
        {
            int start = str.indexOf(name);
            int len = start + name.length();
            int end = str.indexOf(' ', len);
            if(end != -1)
            {
                String next = str.substring(len, end).trim();
                if("+set".equals(next))
                    str = str.substring(0, start) + str.substring(len);
                else
                    str = str.substring(0, start) + str.substring(end);
            }
            else
                str = str.substring(0, start);
            edit.setText(str);
            res = true;
        }
        if(lock) UnlockCmdUpdate();
        return res;
	}

    private boolean IsProp(String name)
    {
        name=" +set "+name+" ";
        String str=GetCmdText();
        return(str.contains(name));
	}
    
    private void EditFile(String file)
    {
        int res = ContextUtility.CheckPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE, CONST_REQUEST_EXTERNAL_STORAGE_FOR_EDIT_CONFIG_FILE_RESULT_CODE);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast.makeText(this, "Can't access file!\nRead/Write external storage permission is not granted!", Toast.LENGTH_LONG).show();
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        
        String gamePath = V.edt_path.getText().toString();
        String game = GetProp("fs_game");
        if(game == null || game.isEmpty())
            game = "base";
        String basePath = gamePath + File.separator + game + File.separator + file;
        File f = new File(basePath);
        if(!f.isFile() || !f.canWrite() || !f.canRead())
        {
            Toast.makeText(this, "File can not access(" + basePath + ")!", Toast.LENGTH_LONG).show();
            return;
        }
        
        Intent intent = new Intent(this, ConfigEditorActivity.class);
        intent.putExtra("CONST_FILE_PATH", basePath);
        startActivity(intent);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.activity_main, menu);

        if(ContextUtility.BuildIsDebug(this))
        {
            menu.findItem(R.id.main_menu_dev_menu).setVisible(true);
        }
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        switch(item.getItemId())
        {
            case R.id.main_menu_support_developer:
                support(null);
                return true;
        case R.id.main_menu_save_settings:
            WritePreferences();
            Toast.makeText(this, "Preferences settings saved!", Toast.LENGTH_LONG).show();
            return true;
        case R.id.main_menu_changes:
            OpenChanges();
            return true;
        case R.id.main_menu_source:
            OpenAbout();
            return true;
            case R.id.main_menu_help:
            OpenHelp();
                return true;
            case R.id.main_menu_settings:
                OpenSettings();
                return true;
        case R.id.main_menu_runtime_log:
            OpenRuntimeLog();
            return true;
            
        case R.id.main_menu_debug:
            OpenDebugDialog();
            return true;
        case R.id.main_menu_test:
            Test();
            return true;
        default:
        return super.onOptionsItemSelected(item);
        }
    }
    
    private void OpenChanges()
    {
        ContextUtility.OpenMessageDialog(this, "Changes", TextHelper.GetChangesText());
    }

    private void OpenAbout()
    {
        ContextUtility.OpenMessageDialog(this, "About " + Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")", TextHelper.GetAboutText(this));
    }
  
    private void HandleUnexperted()
    {
        Thread.setDefaultUncaughtExceptionHandler(new UncaughtExceptionHandler(this));
    }
    
    private void OpenRuntimeLog()
    {
        String path = V.edt_path.getText().toString() + File.separatorChar + "stdout.txt";
        String text = FileUtility.file_get_contents(path);
        if (text != null)
        {
            ContextUtility.OpenMessageDialog(this, "Last runtime log", text);
        }
        else
        {
            Toast.makeText(this, "Log file can not access(" + path + ")", Toast.LENGTH_LONG).show();
        }
    }
    
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        List<String> list = null;
        for(int i = 0; i < permissions.length; i++)
        {
            if(grantResults[i] != PackageManager.PERMISSION_GRANTED)
            {
                if(list == null)
                    list = new ArrayList<String>();
                list.add(permissions[i]);
            }
        }
        if(list == null || list.isEmpty())
            return;
            
        String opt = null;
        switch(requestCode)
        {
            case CONST_REQUEST_EXTERNAL_STORAGE_FOR_START_RESULT_CODE:
                opt = "Start game";
                break;
            case CONST_REQUEST_EXTERNAL_STORAGE_FOR_EDIT_CONFIG_FILE_RESULT_CODE:
                opt = "Edit config file";
                break;
            case CONST_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_FOLDER_RESULT_CODE:
                opt = "Choose game folder";
                break;
            default:
                opt = "Operation";
                break;
        }
        StringBuilder sb = new StringBuilder();
        String endl = TextHelper.GetDialogMessageEndl();
        for(String str : list)
        {
            if(str != null)
                sb.append("  * " + str);
            sb.append(endl);
        }
        AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(this, opt + " request necessary permissions", TextHelper.GetDialogMessage(sb.toString()));
        builder.setNeutralButton("Grant", new AlertDialog.OnClickListener() {          
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    ContextUtility.OpenAppSetting(GameLauncher.this);
                    dialog.dismiss();
                }
            });
        builder.create().show();
	}
    
    private void OpenFolderChooser()
    {
        int res = ContextUtility.CheckPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE, CONST_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_FOLDER_RESULT_CODE);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast.makeText(this, "Can't choose folder!\nRead/Write external storage permission is not granted!", Toast.LENGTH_LONG).show();
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;

        String defaultPath = Environment.getExternalStorageDirectory().getAbsolutePath(); //System.getProperty("user.home");
        String gamePath = V.edt_path.getText().toString();
        if(gamePath == null || gamePath.isEmpty())
            gamePath = defaultPath;
        File f = new File(gamePath);
        if(!f.exists())
        {
            gamePath = defaultPath;
            f = new File(gamePath);
        }
        if(!f.isDirectory())
        {
            gamePath = f.getParent();
            f = f.getParentFile();
        }
        if(!f.canRead())
        {
            gamePath = defaultPath;
            f = new File(gamePath);
        }
        
        FileBrowserDialog dialog = new FileBrowserDialog(this);
        dialog.SetupUI("Chooser data folder", gamePath);
        dialog.setButton(AlertDialog.BUTTON_NEGATIVE, "Cancel", new AlertDialog.OnClickListener() {          
                @Override
                public void onClick(DialogInterface dialog, int which) { 
                    dialog.dismiss();
                }
            });
        dialog.setButton(AlertDialog.BUTTON_POSITIVE, "Choose current directory", new AlertDialog.OnClickListener() {          
                @Override
                public void onClick(DialogInterface dialog, int which) { 
					V.edt_path.setText(((FileBrowserDialog)dialog).Path());
                    dialog.dismiss();
                }
            });
            
        dialog.show();
    }
    
    private void WritePreferences()
    {
        SharedPreferences.Editor mEdtr=PreferenceManager.getDefaultSharedPreferences(this).edit();
        mEdtr.putString(Q3EUtils.pref_params, GetCmdText());
        mEdtr.putString(Q3EUtils.pref_eventdev, V.edt_mouse.getText().toString());
        mEdtr.putString(Q3EUtils.pref_datapath, V.edt_path.getText().toString());
        mEdtr.putBoolean(Q3EUtils.pref_hideonscr, V.hideonscr.isChecked());
        //k mEdtr.putBoolean(Q3EUtils.pref_32bit, true);
        int index = GetCheckboxIndex(V.rg_color_bits) - 1;
        mEdtr.putBoolean(Q3EUtils.pref_32bit, index == -1);
        mEdtr.putInt(Q3EUtils.pref_harm_16bit, index);
        mEdtr.putInt(Q3EUtils.pref_harm_r_harmclearvertexbuffer, GetCheckboxIndex(V.r_harmclearvertexbuffer));
        mEdtr.putString(Q3EUtils.pref_harm_r_lightModel, GetCheckboxIndex(V.rg_harm_r_lightModel) == 1 ? "blinn_phong" : "phong");
		mEdtr.putFloat(Q3EUtils.pref_harm_r_specularExponent, Float.parseFloat(V.edt_harm_r_specularExponent.getText().toString()));

        mEdtr.putBoolean(Q3EUtils.pref_analog, true);
        mEdtr.putBoolean(Q3EUtils.pref_mapvol, V.mapvol.isChecked());
        mEdtr.putBoolean(Q3EUtils.pref_analog, V.smoothjoy.isChecked());
        mEdtr.putBoolean(Q3EUtils.pref_2fingerlmb, V.secfinglmb.isChecked());
        mEdtr.putBoolean(Q3EUtils.pref_detectmouse, V.detectmouse.isChecked());
        mEdtr.putInt(Q3EUtils.pref_mousepos, GetCheckboxIndex(V.rg_curpos));
        mEdtr.putInt(Q3EUtils.pref_scrres, GetCheckboxIndex(V.rg_scrres));
        mEdtr.putInt(Q3EUtils.pref_msaa, GetCheckboxIndex(V.rg_msaa));
        mEdtr.putString(Q3EUtils.pref_resx, V.res_x.getText().toString());
        mEdtr.putString(Q3EUtils.pref_resy, V.res_y.getText().toString());
		mEdtr.commit();
    }

    private void OpenHelp()
    {
        ContextUtility.OpenMessageDialog(this, "Help", TextHelper.GetHelpText());
    }
    
    private void OpenUpdate()
    {       
        if(IsUpdateRelease())
            ContextUtility.OpenMessageDialog(this, "Update: " + Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")", TextHelper.GetUpdateText(this));
    }

    private boolean IsUpdateRelease()
    {
        final String UPDATE_RELEASE = "UPDATE_RELEASE";
        SharedPreferences pref =  PreferenceManager.getDefaultSharedPreferences(this);
        int r = pref.getInt(UPDATE_RELEASE, 0);
        if(r == Constants.CONST_UPDATE_RELEASE)
            return false;
        pref.edit().putInt(UPDATE_RELEASE, Constants.CONST_UPDATE_RELEASE).commit();
        return true;
    }
    
    private void SetCmdText(String text)
    {
        EditText edit = V.edt_cmdline;
        int pos = edit.getSelectionStart();
        edit.setText(text);
        if(text != null && !text.isEmpty())
        {
            pos = Math.max(0, Math.min(pos, text.length()));
            try
            {
                edit.setSelection(pos);      
            }
            catch(Exception e)
            {
                e.printStackTrace();
            }
        }
    }

    private String GetCmdText()
    {
        return V.edt_cmdline.getText().toString();
    }
    
    private void OpenGameLibChooser()
    {
        boolean is_neon = Q3EJNI.detectNeon();
        final String libPath = "/data/data/" + getPackageName() + "/lib/";
        final String items[] = new String[Constants.LIBS.length];
        String lib = PreferenceManager.getDefaultSharedPreferences(this).getString(Q3EUtils.pref_harm_game_lib, "");
        int selected = 0;
        for(int i = 0; i < Constants.LIBS.length; i++)
        {
            items[i] = "lib" + Constants.LIBS[i] + (is_neon ? "_neon" : "") + ".so";
            if((libPath + items[i]).equals(lib))
            {
                selected = i;
            }
        }
        
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Choose game library(" + (is_neon ? "neon" : "vfp") + ")");
        builder.setSingleChoiceItems(items, selected, new DialogInterface.OnClickListener(){
            public void onClick(DialogInterface dialog, int p)
            {
                String lib = libPath + items[p];
                SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit();
                editor.putString(Q3EUtils.pref_harm_game_lib, lib).commit();
                SetProp("harm_fs_gameLibPath", lib);
                dialog.dismiss();
            }
        });
        AlertDialog dialog = builder.create();
        dialog.show();
    }
    
    private void Test()
    {
        ((Object)null).toString();
    }

    private void OpenSettings()
    {
        LauncherSettingsDialog dialog = LauncherSettingsDialog.newInstance();
        dialog.show(getFragmentManager(), "LauncherSettingsDialog");
    }
    
    private void OpenDebugDialog()
    {
        DebugDialog dialog = DebugDialog.newInstance();
        dialog.show(getFragmentManager(), "DebugDialog");
    }
    
    private void UpdateCustomerResulotion(boolean enabled)
    {
        LinearLayout layout = V.res_customlayout;
        final int count = layout.getChildCount();
        for(int i = 0; i < count; i++)
        {
            layout.getChildAt(i).setEnabled(enabled);
        }
        
        layout.setEnabled(enabled);
    }

	private void SetGameDLL(int val)
	{
		switch(val)
		{
			case R.id.fs_game_base:
				RemoveProp("fs_game");
				RemoveProp("fs_game_base");
				V.launcher_tab1_game_lib_button.setEnabled(false);
				RemoveProp("harm_fs_gameLibPath");
				break;
			case R.id.fs_game_d3xp:
				SetProp("fs_game", "d3xp");
				RemoveProp("fs_game_base");
				V.launcher_tab1_game_lib_button.setEnabled(false);
				RemoveProp("harm_fs_gameLibPath");
				break;
			case R.id.fs_game_cdoom:
				SetProp("fs_game", "cdoom");
				RemoveProp("fs_game_base");
				V.launcher_tab1_game_lib_button.setEnabled(false);
				RemoveProp("harm_fs_gameLibPath");
				break;
			case R.id.fs_game_lost_mission:
				SetProp("fs_game", "d3le");
				SetProp("fs_game_base", "d3xp"); // must load d3xp pak
				V.launcher_tab1_game_lib_button.setEnabled(false);
				RemoveProp("harm_fs_gameLibPath");
				break;
			case R.id.fs_game_user:
				SetProp("fs_game", V.edt_fs_game.getText().toString());
				//RemoveProp("fs_game_base");
				V.launcher_tab1_game_lib_button.setEnabled(true);
				String lib = PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).getString(Q3EUtils.pref_harm_game_lib, "");
				if(lib != null && !lib.isEmpty())
					SetProp("harm_fs_gameLibPath", lib);
				break;
			default:
				break;
		}
	}

    private boolean LockCmdUpdate()
    {
        boolean res = m_cmdUpdateLock;
        m_cmdUpdateLock = true;
        return !res;
    }

    private void UnlockCmdUpdate()
    {
        m_cmdUpdateLock = false;
    }

    private boolean IsCmdUpdateLocked()
    {
        return m_cmdUpdateLock;
    }

	private class ViewHolder
	{
		public EditText edt_cmdline;
		public LinearLayout res_customlayout;
		public CheckBox nolight;
		public CheckBox useetc1cache;
		public CheckBox useetc1;
        public CheckBox usedxt;
		public RadioGroup r_harmclearvertexbuffer;
		public RadioGroup rg_scrres;
		public RadioGroup rg_msaa;
		public RadioGroup rg_color_bits;
		public RadioGroup rg_fs_game;
		public EditText edt_fs_game;
		public EditText edt_mouse;
		public EditText edt_path;
		public CheckBox hideonscr;
		public CheckBox mapvol;
		public CheckBox secfinglmb;
		public CheckBox smoothjoy;
		public CheckBox detectmouse;
		public LinearLayout layout_mouseconfig;
		public LinearLayout layout_manualmouseconfig;
		public Button launcher_tab1_game_lib_button;
		public EditText res_x;
		public EditText res_y;
		public Button launcher_tab1_edit_autoexec;
		public Button launcher_tab1_edit_doomconfig;
		public Button launcher_tab1_game_data_chooser_button;
		public RadioGroup rg_curpos;
		public EditText edt_harm_r_specularExponent;
		public RadioGroup rg_harm_r_lightModel;

		public void Setup()
		{
			edt_cmdline = (EditText)findViewById(R.id.edt_cmdline);
			res_customlayout = (LinearLayout)findViewById(R.id.res_customlayout);
			nolight = (CheckBox)findViewById(R.id.nolight);
			useetc1cache = (CheckBox)findViewById(R.id.useetc1cache);
			useetc1 = (CheckBox)findViewById(R.id.useetc1);
            usedxt = (CheckBox)findViewById(R.id.usedxt);
			r_harmclearvertexbuffer = (RadioGroup)findViewById(R.id.r_harmclearvertexbuffer);
			rg_scrres = (RadioGroup)findViewById(R.id.rg_scrres);
			rg_msaa = (RadioGroup)findViewById(R.id.rg_msaa);
			rg_color_bits = (RadioGroup)findViewById(R.id.rg_color_bits);
			rg_fs_game = (RadioGroup)findViewById(R.id.rg_fs_game);
			edt_fs_game = (EditText)findViewById(R.id.edt_fs_game);
			edt_mouse = (EditText)findViewById(R.id.edt_mouse);
			edt_path = (EditText)findViewById(R.id.edt_path);
			hideonscr = (CheckBox)findViewById(R.id.hideonscr);
			mapvol = (CheckBox)findViewById(R.id.mapvol);
			secfinglmb = (CheckBox)findViewById(R.id.secfinglmb);
			smoothjoy = (CheckBox)findViewById(R.id.smoothjoy);
			detectmouse = (CheckBox)findViewById(R.id.detectmouse);
			layout_mouseconfig = (LinearLayout)findViewById(R.id.layout_mouseconfig);
			layout_manualmouseconfig = (LinearLayout)findViewById(R.id.layout_manualmouseconfig);
			launcher_tab1_game_lib_button = (Button)findViewById(R.id.launcher_tab1_game_lib_button);
			res_x = (EditText)findViewById(R.id.res_x);
			res_y = (EditText)findViewById(R.id.res_y);
			launcher_tab1_edit_autoexec = (Button)findViewById(R.id.launcher_tab1_edit_autoexec);
			launcher_tab1_edit_doomconfig = (Button)findViewById(R.id.launcher_tab1_edit_doomconfig);
			launcher_tab1_game_data_chooser_button = (Button)findViewById(R.id.launcher_tab1_game_data_chooser_button);
			rg_curpos = (RadioGroup)findViewById(R.id.rg_curpos);
			edt_harm_r_specularExponent = (EditText)findViewById(R.id.edt_harm_r_specularExponent);
		    rg_harm_r_lightModel = (RadioGroup)findViewById(R.id.rg_harm_r_lightModel);
		}
	}
}
