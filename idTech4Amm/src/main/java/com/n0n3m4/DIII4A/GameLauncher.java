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

import android.annotation.SuppressLint;
import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TabHost;
import android.widget.TextView;
import android.widget.Toast;

import com.karin.idTech4Amm.OnScreenButtonConfigActivity;
import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.UIUtility;
import com.karin.idTech4Amm.lib.Utility;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.Constants;
import com.karin.idTech4Amm.sys.Game;
import com.karin.idTech4Amm.sys.GameManager;
import com.karin.idTech4Amm.sys.PreferenceKey;
import com.karin.idTech4Amm.ui.DebugDialog;
import com.karin.idTech4Amm.ui.LauncherSettingsDialog;
import com.n0n3m4.DIII4A.launcher.AddExternalLibraryFunc;
import com.n0n3m4.DIII4A.launcher.BackupPreferenceFunc;
import com.n0n3m4.DIII4A.launcher.CVarEditorFunc;
import com.n0n3m4.DIII4A.launcher.CheckForUpdateFunc;
import com.n0n3m4.DIII4A.launcher.ChooseGameFolderFunc;
import com.n0n3m4.DIII4A.launcher.ChooseGameLibFunc;
import com.n0n3m4.DIII4A.launcher.ChooseGameModFunc;
import com.n0n3m4.DIII4A.launcher.DebugPreferenceFunc;
import com.n0n3m4.DIII4A.launcher.DebugTextHistoryFunc;
import com.n0n3m4.DIII4A.launcher.EditConfigFileFunc;
import com.n0n3m4.DIII4A.launcher.EditExternalLibraryFunc;
import com.n0n3m4.DIII4A.launcher.ExtractPatchResourceFunc;
import com.n0n3m4.DIII4A.launcher.ExtractSourceFunc;
import com.n0n3m4.DIII4A.launcher.OpenSourceLicenseFunc;
import com.n0n3m4.DIII4A.launcher.RestorePreferenceFunc;
import com.n0n3m4.DIII4A.launcher.SetupControlsThemeFunc;
import com.n0n3m4.DIII4A.launcher.StartGameFunc;
import com.n0n3m4.DIII4A.launcher.SupportDeveloperFunc;
import com.n0n3m4.DIII4A.launcher.TranslatorsFunc;
import com.n0n3m4.q3e.Q3EAd;
import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EMain;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUiConfig;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.gl.Q3EGLConstants;
import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.KUncaughtExceptionHandler;
import com.n0n3m4.q3e.karin.KidTech4Command;
import com.n0n3m4.q3e.onscreen.Q3EControls;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

@SuppressLint({"ApplySharedPref", "CommitPrefEdits"})
public class GameLauncher extends Activity
{
    private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_START = 1;
    private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_EDIT_CONFIG_FILE = 2;
    private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_FOLDER = 3;
    private static final int CONST_RESULT_CODE_REQUEST_EXTRACT_PATCH_RESOURCE = 4;
    private static final int CONST_RESULT_CODE_REQUEST_BACKUP_PREFERENCES_CHOOSE_FILE = 5;
    private static final int CONST_RESULT_CODE_REQUEST_RESTORE_PREFERENCES_CHOOSE_FILE = 6;
    private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_LIBRARY = 7;
    private static final int CONST_RESULT_CODE_REQUEST_ADD_EXTERNAL_GAME_LIBRARY = 8;
    private static final int CONST_RESULT_CODE_REQUEST_EDIT_EXTERNAL_GAME_LIBRARY = 9;
	private static final int CONST_RESULT_CODE_REQUEST_EXTRACT_SOURCE = 10;
	private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_MOD = 11;
	private static final int CONST_RESULT_CODE_ACCESS_ANDROID_DATA = 12;

	private final GameManager m_gameManager = new GameManager();
    // GameLauncher function
    private ExtractPatchResourceFunc m_extractPatchResourceFunc;
    private CheckForUpdateFunc m_checkForUpdateFunc;
    private BackupPreferenceFunc m_backupPreferenceFunc;
    private RestorePreferenceFunc m_restorePreferenceFunc;
    private EditConfigFileFunc m_editConfigFileFunc;
    private ChooseGameFolderFunc m_chooseGameFolderFunc;
    private StartGameFunc m_startGameFunc;
    private AddExternalLibraryFunc m_addExternalLibraryFunc;
    private ChooseGameLibFunc m_chooseGameLibFunc;
    private EditExternalLibraryFunc m_editExternalLibraryFunc;
	private OpenSourceLicenseFunc m_openSourceLicenseFunc;
	private ExtractSourceFunc m_extractSourceFunc;
	private ChooseGameModFunc m_chooseGameModFunc;

    public static final String default_gamedata = Environment.getExternalStorageDirectory() + "/diii4a";
    private final ViewHolder V = new ViewHolder();
    private boolean m_cmdUpdateLock = false;
	private String m_edtPathFocused = "";
    private final CompoundButton.OnCheckedChangeListener m_checkboxChangeListener = new CompoundButton.OnCheckedChangeListener()
    {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
        {
			int id = buttonView.getId();
			if (id == R.id.useetc1cache)
			{
				setProp("r_useETC1cache", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_useetc1cache, isChecked)
						.commit();
			}
			else if (id == R.id.nolight)
			{
				setProp("r_noLight", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_nolight, isChecked)
						.commit();
			}
			else if (id == R.id.useetc1)
			{
				setProp("r_useETC1", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_useetc1, isChecked)
						.commit();
			}
			else if (id == R.id.usedxt)
			{
				setProp("r_useDXT", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_usedxt, isChecked)
						.commit();
			}
			else if (id == R.id.detectmouse)
			{
				UpdateMouseManualMenu(!isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_detectmouse, isChecked)
						.commit();
			}
			else if (id == R.id.hideonscr)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_hideonscr, isChecked)
						.commit();
			}
			else if (id == R.id.smoothjoy)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_analog, isChecked)
						.commit();
			}
			else if (id == R.id.mapvol)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_mapvol, isChecked)
						.commit();
				UpdateMapVol(isChecked);
			}
			else if (id == R.id.secfinglmb)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_2fingerlmb, isChecked)
						.commit();
			}
			else if (id == R.id.fs_game_user)
			{
				UpdateUserGame(isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EUtils.q3ei.GetEnableModPreferenceKey(), isChecked)
						.commit();
			}
			else if (id == R.id.launcher_tab2_enable_gyro)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_view_motion_control_gyro, isChecked)
						.commit();
				UpdateEnableGyro(isChecked);
			}
			else if (id == R.id.auto_quick_load)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_auto_quick_load, isChecked)
						.commit();
				if (isChecked && (Q3EUtils.q3ei.IsIdTech4() || Q3EUtils.q3ei.IsIdTech3() || Q3EUtils.q3ei.IsTDMTech()))
					SetParam_temp("loadGame", "QuickSave");
				else
					RemoveParam_temp("loadGame");
			}
			else if (id == R.id.multithreading)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_multithreading, isChecked)
						.commit();
			}
			else if (id == R.id.launcher_tab2_joystick_unfixed)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_joystick_unfixed, isChecked)
						.commit();
				Q3EUtils.q3ei.joystick_unfixed = isChecked;
			}
			else if (id == R.id.using_mouse)
			{
				UpdateMouseMenu(isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_using_mouse, isChecked)
						.commit();
			}
			else if (id == R.id.find_dll)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_find_dll, isChecked)
						.commit();
			}
			else if (id == R.id.skip_intro)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_skip_intro, isChecked)
						.commit();
				if (isChecked)
					SetCommand_temp("disconnect", true);
				else
					RemoveCommand_temp("disconnect");
			}
			else if (id == R.id.scale_by_screen_area)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_scale_by_screen_area, isChecked)
						.commit();
			}
			else if (id == R.id.cb_s_useOpenAL)
			{
				setProp("s_useOpenAL", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_s_useOpenAL, isChecked)
						.commit();
				if(!isChecked)
				{
					V.cb_s_useEAXReverb.setChecked(false);
				}
				V.cb_s_useEAXReverb.setEnabled(isChecked);
			}
			else if (id == R.id.cb_s_useEAXReverb)
			{
				if(isChecked)
				{
					V.cb_s_useOpenAL.setChecked(true);
				}
				setProp("s_useEAXReverb", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_s_useEAXReverb, isChecked)
						.commit();
			}
			else if (id == R.id.readonly_command)
			{
				SetupCommandLine(isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(PreferenceKey.READONLY_COMMAND, isChecked)
						.commit();
			}
			else if (id == R.id.editable_temp_command)
			{
				SetupTempCommandLine(isChecked);
			}
			else if (id == R.id.cb_translucentStencilShadow)
			{
				setProp("harm_r_stencilShadowTranslucent", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_s_useOpenAL, isChecked)
						.commit();
			}
        }
    };
    private final RadioGroup.OnCheckedChangeListener m_groupCheckChangeListener = new RadioGroup.OnCheckedChangeListener()
    {
        @Override
        public void onCheckedChanged(RadioGroup radioGroup, int id)
        {
            int index;
			int rgId = radioGroup.getId();
			if (rgId == R.id.rg_scrres)
			{
				GameLauncher.this.UpdateCustomerResulotion(id == R.id.res_custom);
				GameLauncher.this.UpdateResolutionScaleScheme(id);
				index = GetCheckboxIndex(radioGroup, id);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_scrres, index)
						.commit();
			}
			else if (rgId == R.id.r_harmclearvertexbuffer)
			{
				index = GetCheckboxIndex(radioGroup, id);
				SetProp("harm_r_clearVertexBuffer", index);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_harm_r_harmclearvertexbuffer, index)
						.commit();
			}
			else if (rgId == R.id.rg_harm_r_lightModel)
			{
				String value = GetCheckboxIndex(radioGroup, id) == 1 ? "blinn_phong" : "phong";
				SetProp("harm_r_lightModel", value);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putString(Q3EPreference.pref_harm_r_lightModel, value)
						.commit();
			}
			else if (rgId == R.id.rg_fs_game || rgId == R.id.rg_fs_q4game || rgId == R.id.rg_fs_preygame || rgId == R.id.rg_fs_q2game || rgId == R.id.rg_fs_q3game || rgId == R.id.rg_fs_rtcwgame || rgId == R.id.rg_fs_tdmgame || rgId == R.id.rg_fs_q1game || rgId == R.id.rg_fs_d3bfggame)
			{
				RadioButton checked = radioGroup.findViewById(id);
				SetGameDLL((String)checked.getTag());
			}
			else if (rgId == R.id.rg_msaa)
			{
				index = GetCheckboxIndex(radioGroup, id);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_msaa, index)
						.commit();
			}
			else if (rgId == R.id.rg_color_bits)
			{
				index = GetCheckboxIndex(radioGroup, id) - 1;
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_32bit, index == -1)
						.putInt(Q3EPreference.pref_harm_16bit, index)
						.commit();
			}
			else if (rgId == R.id.rg_curpos)
			{
				index = GetCheckboxIndex(radioGroup, id);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_mousepos, index)
						.commit();
			}
			else if (rgId == R.id.rg_s_driver)
			{
				String value2 = GetCheckboxIndex(radioGroup, id) == 1 ? "OpenSLES" : "AudioTrack";
				SetProp("s_driver", value2);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putString(Q3EPreference.pref_harm_s_driver, value2)
						.commit();
			}
			else if (rgId == R.id.rg_harm_r_shadow)
			{
				boolean useShadowMapping = GetCheckboxIndex(radioGroup, id) == 1;
				String value = useShadowMapping ? "1" : "0";
				SetProp("r_useShadowMapping", value);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_useShadowMapping, useShadowMapping)
						.commit();
			}
			else if (rgId == R.id.rg_opengl)
			{
				int glVersion = GetCheckboxIndex(radioGroup, id) == 1 ? Q3EGLConstants.OPENGLES30 : Q3EGLConstants.OPENGLES20;
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_harm_opengl, glVersion)
						.commit();
			}
        }
    };
    private final View.OnClickListener m_buttonClickListener = new View.OnClickListener()
    {
        @Override
        public void onClick(View view)
        {
			int id = view.getId();
			if (id == R.id.launcher_tab1_edit_autoexec)
			{
				EditFile("autoexec.cfg");
			}
			else if (id == R.id.launcher_tab1_edit_doomconfig)
			{
				EditFile(Q3EUtils.q3ei.config_name);
			}
			else if (id == R.id.launcher_tab1_game_lib_button)
			{
				OpenGameLibChooser();
			}
			else if (id == R.id.launcher_tab1_game_data_chooser_button)
			{
				OpenFolderChooser();
			}
			else if (id == R.id.onscreen_button_setting)
			{
				OpenOnScreenButtonSetting();
			}
			else if (id == R.id.setup_onscreen_button_theme)
			{
				OpenOnScreenButtonThemeSetting();
			}
			else if (id == R.id.launcher_tab1_edit_cvar)
			{
				EditCVar();
			}
			else if (id == R.id.launcher_tab1_game_mod_button)
			{
				OpenGameModChooser();
			}
        }
    };

	private final AdapterView.OnItemSelectedListener m_itemSelectedListener = new AdapterView.OnItemSelectedListener()
	{
		@Override
		public void onItemSelected(AdapterView<?> parent, View view, int position, long id)
		{
			int viewId = parent.getId();
			if(viewId == R.id.launcher_tab2_joystick_visible)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_harm_joystick_visible, getResources().getIntArray(R.array.joystick_visible_mode_values)[position])
						.commit();
			}
		}

		@Override
		public void onNothingSelected(AdapterView<?> parent)
		{
			int viewId = parent.getId();
			if(viewId == R.id.launcher_tab2_joystick_visible)
			{
				int position = Utility.ArrayIndexOf(getResources().getIntArray(R.array.joystick_visible_mode_values), Q3EGlobals.ONSCRREN_JOYSTICK_VISIBLE_ALWAYS);
				parent.setSelection(position);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_harm_joystick_visible, Q3EGlobals.ONSCRREN_JOYSTICK_VISIBLE_ALWAYS)
						.commit();
			}
		}
	};
    private class SavePreferenceTextWatcher implements TextWatcher
    {
        private final String name;
        private final String defValue;

        public SavePreferenceTextWatcher(String name, String defValue)
        {
            this.name = name;
            this.defValue = defValue;
        }

        public SavePreferenceTextWatcher(String name)
        {
            this(name, null);
        }

        public void onTextChanged(CharSequence s, int start, int before, int count)
        {
        }

        public void beforeTextChanged(CharSequence s, int start, int count, int after)
        {
        }

        public void afterTextChanged(Editable s)
        {
            String value = s.length() == 0 && null != defValue ? defValue : s.toString();
            PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
                    .putString(name, value)
                    .commit();
        }
    }

	private class CommandTextWatcher implements TextWatcher
	{
		private boolean enabled;

		public boolean IsEnabled()
		{
			return enabled;
		}

		public void Install(boolean e)
		{
			enabled = e;
		}

		public void Uninstall()
		{
			enabled = false;
		}

		public void onTextChanged(CharSequence s, int start, int before, int count)
		{
			boolean cond = enabled && V.edt_cmdline.isInputMethodTarget() && !IsCmdUpdateLocked();
			if (cond)
				updatehacktings();
		}

		public void afterTextChanged(Editable s)
		{
			String value = s.length() == 0 ? Q3EGlobals.GAME_EXECUABLE : s.toString();
			PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
					.putString(Q3EUtils.q3ei.GetGameCommandPreferenceKey(), value)
					.commit();
		}

		public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
	}
	private final CommandTextWatcher m_commandTextWatcher = new CommandTextWatcher();

    private final AdapterView.OnItemSelectedListener m_spinnerItemSelectedListener = new AdapterView.OnItemSelectedListener()
    {
        public void onItemSelected(AdapterView adapter, View view, int position, long id)
        {
            int[] keyCodes;
			int vid = adapter.getId();
			if (vid == R.id.launcher_tab2_volume_up_map_config_keys)
			{
				keyCodes = getResources().getIntArray(R.array.key_map_codes);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.VOLUME_UP_KEY, keyCodes[position])
						.commit();
			}
			else if (vid == R.id.launcher_tab2_volume_down_map_config_keys)
			{
				keyCodes = getResources().getIntArray(R.array.key_map_codes);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.VOLUME_DOWN_KEY, keyCodes[position])
						.commit();
			}
        }

        public void onNothingSelected(AdapterView adapter)
        {
        }
    };

    private void InitUIDefaultLayout(Q3EInterface q3ei)
    {
		q3ei.defaults_table = Q3EControls.GetDefaultLayout(this, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_FRIENDLY_EDGE, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_SIZE_SCALE, Q3EControls.CONST_DEFAULT_ON_SCREEN_BUTTON_OPACITY, false);
    }

    public void InitQ3E(String game)
    {
        // Q3EKeyCodes.InitD3Keycodes();
        Q3EInterface q3ei = new Q3EInterface();
		q3ei.standalone = PreferenceManager.getDefaultSharedPreferences(this).getBoolean(Q3EPreference.GAME_STANDALONE_DIRECTORY, false);

        q3ei.InitD3();

        InitUIDefaultLayout(q3ei);

        q3ei.default_path = default_gamedata;

        q3ei.SetupDOOM3(); //k armv7-a only support neon now

        // Q3EInterface.DumpDefaultOnScreenConfig(q3ei.arg_table, q3ei.type_table);

		q3ei.LoadTypeAndArgTablePreference(this);

		if(null != game && !game.isEmpty())
			q3ei.SetupGame(game);

        Q3EUtils.q3ei = q3ei;
    }

    @Override
    public void onAttachedToWindow()
    {
        super.onAttachedToWindow();
        InitUIDefaultLayout(Q3EUtils.q3ei);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        Q3EAd.LoadAds(this);
        super.onConfigurationChanged(newConfig);
    }

    public void support(View vw)
    {
        new SupportDeveloperFunc(this).Start(new Bundle());
    }

    public void UpdateMouseMenu(boolean show)
    {
        final boolean supportGrabMouse = Q3EUtils.SupportMouse() == Q3EGlobals.MOUSE_EVENT;
        V.tv_mprefs.setVisibility(supportGrabMouse ? LinearLayout.GONE : LinearLayout.VISIBLE);
        V.layout_mouse_device.setVisibility(supportGrabMouse ? LinearLayout.GONE : LinearLayout.VISIBLE);
        V.layout_mouseconfig.setVisibility(show ? LinearLayout.VISIBLE : LinearLayout.GONE);
    }

    public void UpdateMouseManualMenu(boolean show)
    {
        final boolean supportGrabMouse = Q3EUtils.SupportMouse() == Q3EGlobals.MOUSE_EVENT;
        V.layout_manualmouseconfig.setVisibility(!supportGrabMouse && show ? LinearLayout.VISIBLE : LinearLayout.GONE);
    }

    public void SelectCheckbox(RadioGroup rg, int index)
    {
        for (int i = 0, j = 0; i < rg.getChildCount(); i++)
        {
            View item = rg.getChildAt(i);
            if (item instanceof RadioButton)
            {
                if (j == index)
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
        for (int i = 0, j = 0; i < rg.getChildCount(); i++)
        {
            View item = rg.getChildAt(i);
            if (item instanceof RadioButton)
            {
                if (item.getId() == id)
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
        for (int i = 0, j = 0; i < rg.getChildCount(); i++)
        {
            View item = rg.getChildAt(i);
            if (item instanceof RadioButton)
            {
                if (j == index)
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
        Boolean b = KidTech4Command.GetBoolProp(GetCmdText(), name, false);
        return null != b ? b : false;
    }

    public void setProp(String name, boolean val)
    {
        SetProp(name, KidTech4Command.btostr(val));
    }

    public void updatehacktings()
    {
    	LockCmdUpdate();
        //k
        V.usedxt.setChecked(getProp("r_useDXT", false));
        V.useetc1.setChecked(getProp("r_useETC1", false));
        V.useetc1cache.setChecked(getProp("r_useETC1cache", false));
        V.nolight.setChecked(getProp("r_noLight", false));

        //k fill commandline
        if (!IsProp("r_useDXT")) setProp("r_useDXT", false);
        if (!IsProp("r_useETC1")) setProp("r_useETC1", false);
        if (!IsProp("r_useETC1cache")) setProp("r_useETC1cache", false);
        if (!IsProp("r_noLight")) setProp("r_noLight", false);

        String str = GetProp("harm_r_clearVertexBuffer");
        int index = Q3EUtils.parseInt_s(str, 2);
        SelectCheckbox(V.r_harmclearvertexbuffer, index);
        if (!IsProp("harm_r_clearVertexBuffer")) SetProp("harm_r_clearVertexBuffer", 2);

        str = GetProp("harm_r_lightModel");
        index = 0;
        if (str != null)
        {
            if ("blinn_phong".equalsIgnoreCase(str))
                index = 1;
        }
        SelectCheckbox(V.rg_harm_r_lightModel, index);
        if (!IsProp("harm_r_lightModel")) SetProp("harm_r_lightModel", "phong");
        str = GetProp("harm_r_specularExponent");
        if (null != str)
            V.edt_harm_r_specularExponent.setText(str);
        if (!IsProp("harm_r_specularExponent")) SetProp("harm_r_specularExponent", "4.0");

        str = GetProp("s_driver");
        index = 0;
        if (str != null)
        {
            if ("OpenSLES".equalsIgnoreCase(str))
                index = 1;
        }
        SelectCheckbox(V.rg_s_driver, index);

		str = GetProp("harm_r_maxFps");
		if (null != str)
			V.edt_harm_r_maxFps.setText(str);
		if (!IsProp("harm_r_maxFps")) SetProp("harm_r_maxFps", "0");

		str = GetProp("r_useShadowMapping");
		index = 0;
		if (str != null)
		{
			if ("1".equalsIgnoreCase(str))
				index = 1;
		}
		SelectCheckbox(V.rg_harm_r_shadow, index);
		if (!IsProp("r_useShadowMapping")) SetProp("r_useShadowMapping", "0");

		V.cb_translucentStencilShadow.setChecked(getProp("harm_r_stencilShadowTranslucent", false));
		if (!IsProp("harm_r_stencilShadowTranslucent")) setProp("r_useDXT", false);

		V.cb_s_useOpenAL.setChecked(getProp("s_useOpenAL", false));
		if (!IsProp("s_useOpenAL"))
		{
			setProp("s_useOpenAL", false);
			V.cb_s_useEAXReverb.setChecked(false);
			setProp("s_useEAXReverb", false);
		}
		else
		{
			V.cb_s_useEAXReverb.setChecked(getProp("s_useEAXReverb", false));
			if (!IsProp("s_useEAXReverb")) setProp("s_useEAXReverb", false);
		}

        str = GetProp(Q3EUtils.q3ei.GetGameCommandParm());
        if (str != null)
        {
            if (!V.fs_game_user.isChecked())
            {
				GameManager.GameProp prop = m_gameManager.ChangeGameMod(str, false);
				if(prop.IsValid())
				{
					SelectCheckbox(GetGameModRadioGroup(), prop.index);
				}
/*				else
				{
					RemoveProp(Q3EUtils.q3ei.GetGameCommandParm());
					RemoveProp("fs_game_base");
				}*/
            }
            else
            {
                String cur = V.edt_fs_game.getText().toString();
                if (!str.equals(cur))
                    V.edt_fs_game.setText(str);
            }
        }
        else
        {
            SelectCheckbox(GetGameModRadioGroup(), -1);
        }
		int checkedRadioButtonId = V.rg_scrres.getCheckedRadioButtonId();
		GameLauncher.this.UpdateCustomerResulotion(checkedRadioButtonId == R.id.res_custom);
		GameLauncher.this.UpdateResolutionScaleScheme(checkedRadioButtonId);

		UnlockCmdUpdate();
    }

    private void ThrowException()
    {
        ((String) null).toString();
    }

    private void ShowDebugTextHistoryDialog()
    {
        new DebugTextHistoryFunc(this).Start(new Bundle());
    }

    private void UpdateUserGame(boolean on)
    {
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(GameLauncher.this);
        String game = preference.getString(Q3EUtils.q3ei.GetGameModPreferenceKey(), "");
        if (null == game)
            game = "";

		GameManager.GameProp prop = m_gameManager.ChangeGameMod(game, true);
		if(prop.IsValid())
			SelectCheckbox(GetGameModRadioGroup(), prop.index);
		game = prop.fs_game;

        preference.edit().putString(Q3EPreference.pref_harm_game_lib, "").commit();
        if (on)
        {
            SetProp(Q3EUtils.q3ei.GetGameCommandParm(), game);
            //RemoveProp("fs_game_base");
        }
        else
        {
			if (!prop.is_user && !game.isEmpty())
				SetProp(Q3EUtils.q3ei.GetGameCommandParm(), game);
			else
				RemoveProp(Q3EUtils.q3ei.GetGameCommandParm());
            //RemoveProp("fs_game_base");
        }
        V.edt_fs_game.setText(game);
        V.rg_fs_game.setEnabled(!on);
        V.rg_fs_q4game.setEnabled(!on);
        V.rg_fs_preygame.setEnabled(!on);
		V.rg_fs_q1game.setEnabled(!on);
		V.rg_fs_q2game.setEnabled(!on);
		V.rg_fs_q3game.setEnabled(!on);
		V.rg_fs_rtcwgame.setEnabled(!on);
		V.rg_fs_tdmgame.setEnabled(!on);
		V.rg_fs_d3bfggame.setEnabled(!on);
        V.fs_game_user.setText(on ? R.string.mod_ : R.string.user_mod);
        //V.launcher_tab1_game_lib_button.setEnabled(on);
        V.edt_fs_game.setEnabled(on);
        //V.launcher_tab1_user_game_layout.setVisibility(on ? View.VISIBLE : View.GONE);
    }

    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
		Q3EUtils.DumpPID(this);
        Q3ELang.Locale(this);

        //k
        KUncaughtExceptionHandler.HandleUnexpectedException(this);
        setTitle(R.string.app_title);
        final SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        ContextUtility.SetScreenOrientation(this, mPrefs.getBoolean(PreferenceKey.LAUNCHER_ORIENTATION, false) ? 0 : 1);

        setContentView(R.layout.main);

        ActionBar actionBar = getActionBar();
        if (null != actionBar)
            actionBar.setDisplayHomeAsUpEnabled(true);

		String gameType = mPrefs.getString(Q3EPreference.pref_harm_game, Q3EGlobals.GAME_DOOM3);
        InitQ3E(null); // gameType
        Q3EUtils.q3ei.joystick_release_range = mPrefs.getFloat(Q3EPreference.pref_harm_joystick_release_range, 0.0f);
        Q3EUtils.q3ei.joystick_unfixed = mPrefs.getBoolean(Q3EPreference.pref_harm_joystick_unfixed, false);
        Q3EUtils.q3ei.joystick_inner_dead_zone = mPrefs.getFloat(Q3EPreference.pref_harm_joystick_inner_dead_zone, 0.0f);
        Q3EUtils.q3ei.SetAppStoragePath(this);

        TabHost th = (TabHost) findViewById(R.id.tabhost);
        th.setup();
        th.addTab(th.newTabSpec("tab1").setIndicator(Q3ELang.tr(this, R.string.general)).setContent(R.id.launcher_tab1));
        th.addTab(th.newTabSpec("tab2").setIndicator(Q3ELang.tr(this, R.string.controls)).setContent(R.id.launcher_tab2));
        th.addTab(th.newTabSpec("tab3").setIndicator(Q3ELang.tr(this, R.string.graphics)).setContent(R.id.launcher_tab3));

        V.Setup();
		InitGameList();

		V.main_ad_layout.setVisibility(mPrefs.getBoolean(PreferenceKey.HIDE_AD_BAR, true) ? View.GONE : View.VISIBLE);

        SetGame(gameType);

        V.edt_cmdline.setText(mPrefs.getString(Q3EUtils.q3ei.GetGameCommandPreferenceKey(), Q3EGlobals.GAME_EXECUABLE));
        V.edt_mouse.setText(mPrefs.getString(Q3EPreference.pref_eventdev, "/dev/input/event???"));
		V.edt_path.setText(mPrefs.getString(Q3EPreference.pref_datapath, default_gamedata));
		m_edtPathFocused = V.edt_path.getText().toString();
		if(ContextUtility.InScopedStorage())
		{
			V.edt_path.setOnFocusChangeListener(new View.OnFocusChangeListener() {
				@Override
				public void onFocusChange(View v, boolean hasFocus) {
					String curPath = V.edt_path.getText().toString();
					if(curPath.equals(m_edtPathFocused))
						return;
					if(!hasFocus)
					{
						OpenSuggestGameWorkingDirectory(curPath);
					}
				}
			});
		}
        V.hideonscr.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.hideonscr.setChecked(mPrefs.getBoolean(Q3EPreference.pref_hideonscr, false));
        V.using_mouse.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_using_mouse, false));
        V.using_mouse.setOnCheckedChangeListener(m_checkboxChangeListener);

        UpdateMouseMenu(V.using_mouse.isChecked());

        V.mapvol.setChecked(mPrefs.getBoolean(Q3EPreference.pref_mapvol, false));
        V.secfinglmb.setChecked(mPrefs.getBoolean(Q3EPreference.pref_2fingerlmb, false));
        V.smoothjoy.setChecked(mPrefs.getBoolean(Q3EPreference.pref_analog, true));
        V.launcher_tab2_joystick_unfixed.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_joystick_unfixed, false));
		V.launcher_tab2_joystick_visible.setSelection(Utility.ArrayIndexOf(getResources().getIntArray(R.array.joystick_visible_mode_values), mPrefs.getInt(Q3EPreference.pref_harm_joystick_visible, Q3EGlobals.ONSCRREN_JOYSTICK_VISIBLE_ALWAYS)));
        V.detectmouse.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.detectmouse.setChecked(mPrefs.getBoolean(Q3EPreference.pref_detectmouse, true));

        UpdateMouseManualMenu(!V.detectmouse.isChecked());

        SelectCheckbox(V.rg_curpos, mPrefs.getInt(Q3EPreference.pref_mousepos, 3));
        V.rg_scrres.setOnCheckedChangeListener(m_groupCheckChangeListener);
        SelectCheckbox(V.rg_scrres, mPrefs.getInt(Q3EPreference.pref_scrres, 0));
        SelectCheckbox(V.rg_msaa, mPrefs.getInt(Q3EPreference.pref_msaa, 0));
        V.rg_msaa.setOnCheckedChangeListener(m_groupCheckChangeListener);
        //k
        V.usedxt.setChecked(mPrefs.getBoolean(Q3EPreference.pref_usedxt, false));
        V.useetc1.setChecked(mPrefs.getBoolean(Q3EPreference.pref_useetc1, false));
        V.useetc1cache.setChecked(mPrefs.getBoolean(Q3EPreference.pref_useetc1cache, false));
        V.nolight.setChecked(mPrefs.getBoolean(Q3EPreference.pref_nolight, false));
        SelectCheckbox(V.rg_color_bits, mPrefs.getInt(Q3EPreference.pref_harm_16bit, -1) + 1);
        V.rg_color_bits.setOnCheckedChangeListener(m_groupCheckChangeListener);
        SelectCheckbox(V.r_harmclearvertexbuffer, mPrefs.getInt(Q3EPreference.pref_harm_r_harmclearvertexbuffer, 2));
        V.r_harmclearvertexbuffer.setOnCheckedChangeListener(m_groupCheckChangeListener);
        SelectCheckbox(V.rg_harm_r_lightModel, "blinn_phong".equalsIgnoreCase(mPrefs.getString(Q3EPreference.pref_harm_r_lightModel, "phong")) ? 1 : 0);
        V.rg_harm_r_lightModel.setOnCheckedChangeListener(m_groupCheckChangeListener);
        SelectCheckbox(V.rg_s_driver, "OpenSLES".equalsIgnoreCase(mPrefs.getString(Q3EPreference.pref_harm_s_driver, "AudioTrack")) ? 1 : 0);
        V.rg_s_driver.setOnCheckedChangeListener(m_groupCheckChangeListener);
		SelectCheckbox(V.rg_harm_r_shadow, mPrefs.getBoolean(Q3EPreference.pref_harm_r_useShadowMapping, false) ? 1 : 0);
		V.rg_harm_r_shadow.setOnCheckedChangeListener(m_groupCheckChangeListener);
		SelectCheckbox(V.rg_opengl, mPrefs.getInt(Q3EPreference.pref_harm_opengl, Q3EGLConstants.GetPreferOpenGLESVersion()) == Q3EGLConstants.OPENGLES30 ? 1 : 0);
		V.rg_opengl.setOnCheckedChangeListener(m_groupCheckChangeListener);
        V.launcher_tab2_enable_gyro.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_view_motion_control_gyro, false));
		boolean skipIntro = mPrefs.getBoolean(Q3EPreference.pref_harm_skip_intro, false);
		V.skip_intro.setChecked(skipIntro);
		if (skipIntro)
			SetCommand_temp("disconnect", true);
        boolean autoQuickLoad = mPrefs.getBoolean(Q3EPreference.pref_harm_auto_quick_load, false);
        V.auto_quick_load.setChecked(autoQuickLoad);
        if (autoQuickLoad && (Q3EUtils.q3ei.IsIdTech4() || Q3EUtils.q3ei.IsIdTech3() || Q3EUtils.q3ei.IsTDMTech()))
            SetParam_temp("loadGame", "QuickSave");
        boolean multithreading = mPrefs.getBoolean(Q3EPreference.pref_harm_multithreading, false);
        V.multithreading.setChecked(multithreading);
        V.edt_cmdline.setOnEditorActionListener(new TextView.OnEditorActionListener()
        {
            public boolean onEditorAction(TextView view, int id, KeyEvent ev)
            {
                if (ev.getKeyCode() == KeyEvent.KEYCODE_ENTER)
                {
                    if (ev.getAction() == KeyEvent.ACTION_UP)
                    {
                        V.edt_path.requestFocus();
                    }
                    return true;
                }
                return false;
            }
        });
        boolean findDll = mPrefs.getBoolean(Q3EPreference.pref_harm_find_dll, false);
        V.find_dll.setChecked(findDll);
        V.launcher_tab1_edit_autoexec.setOnClickListener(m_buttonClickListener);
        V.launcher_tab1_edit_doomconfig.setOnClickListener(m_buttonClickListener);
		V.launcher_tab1_edit_cvar.setOnClickListener(m_buttonClickListener);

        boolean userMod = mPrefs.getBoolean(Q3EUtils.q3ei.GetEnableModPreferenceKey(), false);
        V.fs_game_user.setChecked(userMod);
		String game = mPrefs.getString(Q3EUtils.q3ei.GetGameModPreferenceKey(), "");
		if (null == game)
			game = "";
		GameManager.GameProp prop = m_gameManager.ChangeGameMod(game, userMod);
		SelectCheckbox(GetGameModRadioGroup(), prop.index);
        UpdateUserGame(userMod);
        V.fs_game_user.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.rg_fs_game.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.rg_fs_q4game.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.rg_fs_preygame.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.rg_fs_q1game.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.rg_fs_q2game.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.rg_fs_q3game.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.rg_fs_rtcwgame.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.rg_fs_tdmgame.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.rg_fs_d3bfggame.setOnCheckedChangeListener(m_groupCheckChangeListener);
        V.edt_fs_game.addTextChangedListener(new TextWatcher()
        {
            public void onTextChanged(CharSequence s, int start, int before, int count)
            {
                if (V.fs_game_user.isChecked())
                    SetProp(Q3EUtils.q3ei.GetGameCommandParm(), s);
            }

            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
            }

            public void afterTextChanged(Editable s)
            {
                PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
                        .putString(Q3EUtils.q3ei.GetGameModPreferenceKey(), s.toString())
                        .commit();
            }
        });
        V.launcher_tab1_game_lib_button.setOnClickListener(m_buttonClickListener);
		V.launcher_tab1_game_mod_button.setOnClickListener(m_buttonClickListener);
        V.edt_harm_r_specularExponent.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_r_specularExponent, 4.0f));
		V.edt_harm_r_maxFps.setText(Q3EPreference.GetStringFromInt(mPrefs, Q3EPreference.pref_harm_r_maxFps, 0));

        V.res_x.setText(mPrefs.getString(Q3EPreference.pref_resx, "" + Q3EGlobals.SCREEN_WIDTH));
        V.res_y.setText(mPrefs.getString(Q3EPreference.pref_resy, "" + Q3EGlobals.SCREEN_HEIGHT));
        V.res_x.addTextChangedListener(new SavePreferenceTextWatcher(Q3EPreference.pref_resx, "" + Q3EGlobals.SCREEN_WIDTH));
        V.res_y.addTextChangedListener(new SavePreferenceTextWatcher(Q3EPreference.pref_resy, "" + Q3EGlobals.SCREEN_HEIGHT));
        V.launcher_tab1_game_data_chooser_button.setOnClickListener(m_buttonClickListener);
        V.onscreen_button_setting.setOnClickListener(m_buttonClickListener);
        V.setup_onscreen_button_theme.setOnClickListener(m_buttonClickListener);

        //DIII4A-specific
		SetupCommandTextWatcher(true);
        V.edt_harm_r_specularExponent.addTextChangedListener(new TextWatcher()
        {
            public void onTextChanged(CharSequence s, int start, int before, int count)
            {
                SetProp("harm_r_specularExponent", s);
            }

            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
            }

            public void afterTextChanged(Editable s)
            {
                String value = s.length() == 0 ? "4.0" : s.toString();
				Q3EPreference.SetFloatFromString(GameLauncher.this, Q3EPreference.pref_harm_r_specularExponent, value, 4.0f);
            }
        });
		V.edt_harm_r_maxFps.addTextChangedListener(new TextWatcher()
		{
			public void onTextChanged(CharSequence s, int start, int before, int count)
			{
				SetProp("harm_r_maxFps", s);
			}

			public void beforeTextChanged(CharSequence s, int start, int count, int after) { }

			public void afterTextChanged(Editable s)
			{
				String value = s.length() == 0 ? "0" : s.toString();
				Q3EPreference.SetIntFromString(GameLauncher.this, Q3EPreference.pref_harm_r_maxFps, value, 0);
			}
		});
        V.usedxt.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.useetc1.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.useetc1cache.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.nolight.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.smoothjoy.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.launcher_tab2_joystick_unfixed.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.launcher_tab2_joystick_visible.setOnItemSelectedListener(m_itemSelectedListener);
        V.edt_path.addTextChangedListener(new SavePreferenceTextWatcher(Q3EPreference.pref_datapath, default_gamedata));
        V.edt_mouse.addTextChangedListener(new SavePreferenceTextWatcher(Q3EPreference.pref_eventdev, "/dev/input/event???"));
        V.rg_curpos.setOnCheckedChangeListener(m_groupCheckChangeListener);
        V.secfinglmb.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.mapvol.setOnCheckedChangeListener(m_checkboxChangeListener);
        UpdateMapVol(V.mapvol.isChecked());
        V.launcher_tab2_volume_up_map_config_keys.setOnItemSelectedListener(m_spinnerItemSelectedListener);
        V.launcher_tab2_volume_down_map_config_keys.setOnItemSelectedListener(m_spinnerItemSelectedListener);
        V.launcher_tab2_enable_gyro.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.launcher_tab2_gyro_x_axis_sens.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_view_motion_gyro_x_axis_sens, Q3EControlView.GYROSCOPE_X_AXIS_SENS));
        V.launcher_tab2_gyro_y_axis_sens.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_view_motion_gyro_y_axis_sens, Q3EControlView.GYROSCOPE_Y_AXIS_SENS));
        UpdateEnableGyro(V.launcher_tab2_enable_gyro.isChecked());
        V.launcher_tab2_gyro_x_axis_sens.addTextChangedListener(new TextWatcher()
        {
            public void onTextChanged(CharSequence s, int start, int before, int count)
            {
            }

            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
            }

            public void afterTextChanged(Editable s)
            {
                String value = s.length() == 0 ? "" + Q3EControlView.GYROSCOPE_X_AXIS_SENS : s.toString();
                PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
                        .putFloat(Q3EPreference.pref_harm_view_motion_gyro_x_axis_sens, Q3EUtils.parseFloat_s(value, Q3EControlView.GYROSCOPE_Y_AXIS_SENS))
                        .commit();
            }
        });
        V.launcher_tab2_gyro_y_axis_sens.addTextChangedListener(new TextWatcher()
        {
            public void onTextChanged(CharSequence s, int start, int before, int count)
            {
            }

            public void beforeTextChanged(CharSequence s, int start, int count, int after)
            {
            }

            public void afterTextChanged(Editable s)
            {
                String value = s.length() == 0 ? "" + Q3EControlView.GYROSCOPE_Y_AXIS_SENS : s.toString();
                PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
                        .putFloat(Q3EPreference.pref_harm_view_motion_gyro_y_axis_sens, Q3EUtils.parseFloat_s(value, Q3EControlView.GYROSCOPE_Y_AXIS_SENS))
                        .commit();
            }
        });
        V.auto_quick_load.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.skip_intro.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.multithreading.setOnCheckedChangeListener(m_checkboxChangeListener);
        V.find_dll.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.scale_by_screen_area.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.scale_by_screen_area.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_scale_by_screen_area, false));
		boolean useOpenAL = mPrefs.getBoolean(Q3EPreference.pref_harm_s_useOpenAL, true);
		V.cb_s_useOpenAL.setChecked(useOpenAL);
		V.cb_s_useEAXReverb.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_s_useEAXReverb, true));
		V.cb_s_useOpenAL.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.cb_s_useEAXReverb.setOnCheckedChangeListener(m_checkboxChangeListener);
		if(!useOpenAL)
		{
			V.cb_s_useEAXReverb.setChecked(false);
			V.cb_s_useEAXReverb.setEnabled(false);
		}
		boolean readonlyCommand = mPrefs.getBoolean(PreferenceKey.READONLY_COMMAND, false);
		V.readonly_command.setChecked(readonlyCommand);
		SetupCommandLine(readonlyCommand);
		V.readonly_command.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.cb_translucentStencilShadow.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_r_translucentStencilShadow, false));
		V.cb_translucentStencilShadow.setOnCheckedChangeListener(m_checkboxChangeListener);
		SetupTempCommandLine(false);
		V.editable_temp_command.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.edt_cmdline_temp.addTextChangedListener(new TextWatcher()
		{
			public void onTextChanged(CharSequence s, int start, int before, int count)
			{
				if (V.edt_cmdline_temp.isInputMethodTarget())
				{
					Q3EUtils.q3ei.start_temporary_extra_command = GetTempCmdText();
					UpdateTempCommand();
				}
			}

			public void beforeTextChanged(CharSequence s, int start, int count, int after) { }

			public void afterTextChanged(Editable s) { }
		});

        updatehacktings();

		AfterCreated();
    }

	private void AfterCreated()
	{
		try
		{
			Q3EAd.LoadAds(this);

			OpenUpdate();

			Intent intent = getIntent();
			if(null != intent)
			{
				Bundle extras = intent.getExtras();
				if(null != extras)
				{
					String intentGame = extras.getString("game");
					if(null != intentGame && !intentGame.isEmpty())
					{
						ChangeGame(intentGame);
					}
				}
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        if (null != m_checkForUpdateFunc)
            m_checkForUpdateFunc.Reset();
		if (null != m_openSourceLicenseFunc)
			m_openSourceLicenseFunc.Reset();
    }

    public void start(View vw)
    {
        //k
        WritePreferences();

        if (null == m_startGameFunc)
            m_startGameFunc = new StartGameFunc(this, CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_START);
        Bundle bundle = new Bundle();
        bundle.putString("path", V.edt_path.getText().toString());
        m_startGameFunc.Start(bundle);
    }

    public void controls(View vw)
    {
        startActivity(new Intent(this, Q3EUiConfig.class));
    }

    //k
    public boolean getProp(String name, boolean defaultValueIfNotExists)
    {
        String val = GetProp(name);
        if (val != null && !val.trim().isEmpty())
            return "1".equals(val);
        return defaultValueIfNotExists;
    }

    private void SetProp(String name, Object val)
    {
		if(!LockCmdUpdate())
			return;
        SetCmdText(KidTech4Command.SetProp(GetCmdText(), name, val));
        UnlockCmdUpdate();
    }

    private String GetProp(String name)
    {
        return KidTech4Command.GetProp(GetCmdText(), name);
    }

    private void RemoveProp(String name)
    {
		if(!LockCmdUpdate())
			return;
        boolean[] res = {false};
        String str = KidTech4Command.RemoveProp(GetCmdText(), name, res);
        if (res[0])
            SetCmdText(str);
        UnlockCmdUpdate();
    }

    private boolean IsProp(String name)
    {
        return KidTech4Command.IsProp(GetCmdText(), name);
    }

    private void EditFile(String file)
    {
        if (null == m_editConfigFileFunc)
            m_editConfigFileFunc = new EditConfigFileFunc(this, CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_EDIT_CONFIG_FILE);

        Bundle bundle = new Bundle();
        String game = GetProp(Q3EUtils.q3ei.GetGameCommandParm());
        if (game == null || game.isEmpty())
            game = Q3EUtils.q3ei.game_base;
		String path = KStr.AppendPath(V.edt_path.getText().toString(), Q3EUtils.q3ei.subdatadir, Q3EUtils.q3ei.GetGameHomeDirectoryPath());
        bundle.putString("game", game);
		bundle.putString("path", path);
        bundle.putString("file", file);
        m_editConfigFileFunc.Start(bundle);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.activity_main, menu);

        if (ContextUtility.BuildIsDebug(this))
        {
            menu.findItem(R.id.main_menu_dev_menu).setVisible(true);
        }
        V.main_menu_game = menu.findItem(R.id.main_menu_game);
        V.main_menu_game.setTitle(Q3EUtils.q3ei.game_name);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
		int itemId = item.getItemId();
		if (itemId == R.id.main_menu_support_developer)
		{
			support(null);
			return true;
		}
		else if (itemId == R.id.main_menu_changes)
		{
			OpenChanges();
			return true;
		}
		else if (itemId == R.id.main_menu_source)
		{
			OpenAbout();
			return true;
		}
		else if (itemId == R.id.main_menu_help)
		{
			OpenHelp();
			return true;
		}
		else if (itemId == R.id.main_menu_settings)
		{
			OpenSettings();
			return true;
		}
		else if (itemId == R.id.main_menu_runtime_log)
		{
			OpenRuntimeLog();
			return true;
		}
		else if (itemId == R.id.main_menu_cvar_list)
		{
			OpenCvarListDetail();
			return true;
		}
		else if (itemId == R.id.main_menu_extract_resource)
		{
			OpenResourceFileDialog();
			return true;
		}
		else if (itemId == R.id.main_menu_save_settings)
		{
			WritePreferences();
			Toast.makeText(this, R.string.preferences_settings_saved, Toast.LENGTH_LONG).show();
			return true;
		}
		else if (itemId == R.id.main_menu_backup_settings)
		{
			RequestBackupPreferences();
			return true;
		}
		else if (itemId == R.id.main_menu_restore_settings)
		{
			RequestRestorePreferences();
			return true;
		}
		else if (itemId == R.id.main_menu_debug)
		{
			OpenDebugDialog();
			return true;
		}
		else if (itemId == R.id.main_menu_test)
		{
			Test();
			return true;
		}
		else if (itemId == R.id.main_menu_show_preference)
		{
			ShowPreferenceDialog();
			return true;
		}
		else if (itemId == R.id.main_menu_debug_text_history)
		{
			ShowDebugTextHistoryDialog();
			return true;
		}
		else if (itemId == R.id.main_menu_gen_exception)
		{
			ThrowException();
			return true;
		}
		else if (itemId == R.id.main_menu_check_for_update)
		{
			OpenCheckForUpdateDialog();
			return true;
		}
		else if (itemId == R.id.main_menu_translators)
		{
			OpenTranslatorsDialog();
			return true;
		}

		else if (itemId == R.id.main_menu_game_doom3)
		{
			ChangeGame(Q3EGlobals.GAME_DOOM3);
			return true;
		}
		else if (itemId == R.id.main_menu_game_quake4)
		{
			ChangeGame(Q3EGlobals.GAME_QUAKE4);
			return true;
		}
		else if (itemId == R.id.main_menu_game_prey)
		{
			ChangeGame(Q3EGlobals.GAME_PREY);
			return true;
		}
		else if (itemId == R.id.main_menu_game_quake1)
		{
			ChangeGame(Q3EGlobals.GAME_QUAKE1);
			return true;
		}
		else if (itemId == R.id.main_menu_game_quake2)
		{
			ChangeGame(Q3EGlobals.GAME_QUAKE2);
			return true;
		}
		else if (itemId == R.id.main_menu_game_quake3)
		{
			ChangeGame(Q3EGlobals.GAME_QUAKE3);
			return true;
		}
		else if (itemId == R.id.main_menu_game_rtcw)
		{
			ChangeGame(Q3EGlobals.GAME_RTCW);
			return true;
		}
		else if (itemId == R.id.main_menu_game_tdm)
		{
			ChangeGame(Q3EGlobals.GAME_TDM);
			return true;
		}
		else if (itemId == R.id.main_menu_game_doom3bfg)
		{
			ChangeGame(Q3EGlobals.GAME_DOOM3BFG);
			return true;
		}
		else if (itemId == android.R.id.home)
		{
			ChangeGame();
			return true;
		}
		/*else if (itemId == R.id.main_menu_move_game_data)
		{
			MoveGameDataToAppPrivateDirectory();
			return true;
		}*/
		return super.onOptionsItemSelected(item);
	}

    private void OpenChanges()
    {
        ContextUtility.OpenMessageDialog(this, Q3ELang.tr(this, R.string.changes), TextHelper.GetChangesText());
    }

    private void OpenAbout()
    {
    	Object[] args = { null };
		AlertDialog dialog = ContextUtility.OpenMessageDialog(this, Q3ELang.tr(this, R.string.about) + " " + Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")", TextHelper.GetAboutText(this), new Runnable()
		{
			@Override
			public void run()
			{
				AlertDialog.Builder builder = (AlertDialog.Builder)args[0];
				builder.setNeutralButton(R.string.license, new DialogInterface.OnClickListener()
				{
					@Override
					public void onClick(DialogInterface dialog, int which)
					{
						if (null == m_openSourceLicenseFunc)
							m_openSourceLicenseFunc = new OpenSourceLicenseFunc(GameLauncher.this);
						m_openSourceLicenseFunc.Start(new Bundle());
					}
				});
				builder.setNegativeButton(R.string.extract_source, new DialogInterface.OnClickListener()
				{
					@Override
					public void onClick(DialogInterface dialog, int which)
					{
						if (null == m_extractSourceFunc)
							m_extractSourceFunc = new ExtractSourceFunc(GameLauncher.this, CONST_RESULT_CODE_REQUEST_EXTRACT_SOURCE);
						m_extractSourceFunc.Start(new Bundle());
					}
				});
			}
		}, args);
		dialog.show();
	}

    private void OpenRuntimeLog()
    {
        String path = V.edt_path.getText().toString() + File.separatorChar + "stdout.txt";
        String text = Q3EUtils.file_get_contents(path);
        if (text != null)
        {
            ContextUtility.OpenMessageDialog(this, Q3ELang.tr(this, R.string.last_runtime_log), text);
        }
        else
        {
            Toast.makeText(this, Q3ELang.tr(this, R.string.file_can_not_access) + path, Toast.LENGTH_LONG).show();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        List<String> list = null;
        for (int i = 0; i < permissions.length; i++)
        {
            if (grantResults[i] != PackageManager.PERMISSION_GRANTED)
            {
                if (list == null)
                    list = new ArrayList<String>();
                list.add(permissions[i]);
            }
        }

        HandleGrantPermissionResult(requestCode, list);
    }

    private void OpenFolderChooser()
    {
        if (null == m_chooseGameFolderFunc)
            m_chooseGameFolderFunc = new ChooseGameFolderFunc(this, CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_FOLDER, CONST_RESULT_CODE_ACCESS_ANDROID_DATA, new Runnable()
            {
                @Override
                public void run()
                {
                    V.edt_path.setText(m_chooseGameFolderFunc.GetResult());
					OpenSuggestGameWorkingDirectory(V.edt_path.getText().toString());
                }
            });
        Bundle bundle = new Bundle();
        bundle.putString("path", V.edt_path.getText().toString());
        m_chooseGameFolderFunc.Start(bundle);
    }

    private void WritePreferences()
    {
        SharedPreferences.Editor mEdtr = PreferenceManager.getDefaultSharedPreferences(this).edit();
        mEdtr.putString(Q3EUtils.q3ei.GetGameCommandPreferenceKey(), GetCmdText());
        mEdtr.putString(Q3EPreference.pref_eventdev, V.edt_mouse.getText().toString());
        mEdtr.putString(Q3EPreference.pref_datapath, V.edt_path.getText().toString());
        mEdtr.putBoolean(Q3EPreference.pref_hideonscr, V.hideonscr.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_harm_using_mouse, V.using_mouse.isChecked());
        //k mEdtr.putBoolean(Q3EUtils.pref_32bit, true);
        int index = GetCheckboxIndex(V.rg_color_bits) - 1;
        mEdtr.putBoolean(Q3EPreference.pref_32bit, index == -1);
        mEdtr.putInt(Q3EPreference.pref_harm_16bit, index);
        mEdtr.putInt(Q3EPreference.pref_harm_r_harmclearvertexbuffer, GetCheckboxIndex(V.r_harmclearvertexbuffer));
        mEdtr.putString(Q3EPreference.pref_harm_r_lightModel, GetCheckboxIndex(V.rg_harm_r_lightModel) == 1 ? "blinn_phong" : "phong");
        mEdtr.putFloat(Q3EPreference.pref_harm_r_specularExponent, Q3EUtils.parseFloat_s(V.edt_harm_r_specularExponent.getText().toString(), 4.0f));
        mEdtr.putString(Q3EPreference.pref_harm_s_driver, GetCheckboxIndex(V.rg_s_driver) == 1 ? "OpenSLES" : "AudioTrack");
		mEdtr.putInt(Q3EPreference.pref_harm_r_maxFps, Q3EUtils.parseInt_s(V.edt_harm_r_maxFps.getText().toString(), 0));
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_useShadowMapping, GetCheckboxIndex(V.rg_harm_r_shadow) == 1);
		mEdtr.putInt(Q3EPreference.pref_harm_opengl, GetCheckboxIndex(V.rg_opengl) == 1 ? Q3EGLConstants.OPENGLES30 : Q3EGLConstants.OPENGLES20);

        mEdtr.putBoolean(Q3EPreference.pref_mapvol, V.mapvol.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_analog, V.smoothjoy.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_2fingerlmb, V.secfinglmb.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_detectmouse, V.detectmouse.isChecked());
        mEdtr.putInt(Q3EPreference.pref_mousepos, GetCheckboxIndex(V.rg_curpos));
        mEdtr.putInt(Q3EPreference.pref_scrres, GetCheckboxIndex(V.rg_scrres));
        mEdtr.putInt(Q3EPreference.pref_msaa, GetCheckboxIndex(V.rg_msaa));
        mEdtr.putString(Q3EPreference.pref_resx, V.res_x.getText().toString());
        mEdtr.putString(Q3EPreference.pref_resy, V.res_y.getText().toString());
        mEdtr.putBoolean(Q3EPreference.pref_useetc1cache, V.useetc1cache.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_useetc1, V.useetc1.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_usedxt, V.usedxt.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_nolight, V.nolight.isChecked());
        mEdtr.putBoolean(Q3EUtils.q3ei.GetEnableModPreferenceKey(), V.fs_game_user.isChecked());
        mEdtr.putString(Q3EPreference.pref_harm_game, Q3EUtils.q3ei.game);
        mEdtr.putBoolean(Q3EPreference.pref_harm_view_motion_control_gyro, V.launcher_tab2_enable_gyro.isChecked());
        mEdtr.putFloat(Q3EPreference.pref_harm_view_motion_gyro_x_axis_sens, Q3EUtils.parseFloat_s(V.launcher_tab2_gyro_x_axis_sens.getText().toString(), Q3EControlView.GYROSCOPE_X_AXIS_SENS));
        mEdtr.putFloat(Q3EPreference.pref_harm_view_motion_gyro_y_axis_sens, Q3EUtils.parseFloat_s(V.launcher_tab2_gyro_y_axis_sens.getText().toString(), Q3EControlView.GYROSCOPE_Y_AXIS_SENS));
        mEdtr.putBoolean(Q3EPreference.pref_harm_auto_quick_load, V.auto_quick_load.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_skip_intro, V.skip_intro.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_harm_multithreading, V.multithreading.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_harm_joystick_unfixed, V.launcher_tab2_joystick_unfixed.isChecked());
		mEdtr.putInt(Q3EPreference.pref_harm_joystick_visible, getResources().getIntArray(R.array.joystick_visible_mode_values)[V.launcher_tab2_joystick_visible.getSelectedItemPosition()]);
        mEdtr.putBoolean(Q3EPreference.pref_harm_find_dll, V.find_dll.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_scale_by_screen_area, V.scale_by_screen_area.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_s_useOpenAL, V.cb_s_useOpenAL.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_s_useEAXReverb, V.cb_s_useEAXReverb.isChecked());
		mEdtr.putBoolean(PreferenceKey.READONLY_COMMAND, V.readonly_command.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_translucentStencilShadow, V.cb_translucentStencilShadow.isChecked());

		// mEdtr.putString(Q3EUtils.q3ei.GetGameModPreferenceKey(), V.edt_fs_game.getText().toString());
        mEdtr.commit();
    }

    private void OpenHelp()
    {
        ContextUtility.OpenMessageDialog(this, Q3ELang.tr(this, R.string.help), TextHelper.GetHelpText());
    }

    private void OpenUpdate()
    {
        if (IsUpdateRelease())
            ContextUtility.OpenMessageDialog(this, Q3ELang.tr(this, R.string.update_) + Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")", TextHelper.GetUpdateText(this));
    }

    private boolean IsUpdateRelease()
    {
        final String UPDATE_RELEASE = "UPDATE_RELEASE";
        SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
        int r = pref.getInt(UPDATE_RELEASE, 0);
        if (r == Constants.CONST_UPDATE_RELEASE)
            return false;
        pref.edit().putInt(UPDATE_RELEASE, Constants.CONST_UPDATE_RELEASE).commit();
        return true;
    }

	private void SetCmdText(String text)
    {
		if(null == text || text.isEmpty())
			text = Q3EGlobals.GAME_EXECUABLE;
        EditText edit = V.edt_cmdline;
        if (edit.getText().toString().equals(text))
            return;
        int pos = edit.getSelectionStart();
        edit.setText(text);
        if (text != null && !text.isEmpty())
        {
            pos = Math.max(0, Math.min(pos, text.length()));
            try
            {
                edit.setSelection(pos);
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }
    }

	private String GetCmdText()
    {
		String s = V.edt_cmdline.getText().toString();
		if(s.isEmpty())
			s = Q3EGlobals.GAME_EXECUABLE;
		return s;
    }

    private void OpenGameLibChooser()
    {
        final String PreferenceKey = Q3EUtils.q3ei.GetGameModLibPreferenceKey();
        if (null == m_chooseGameLibFunc)
        {
            m_chooseGameLibFunc = new ChooseGameLibFunc(this, CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_LIBRARY);
            m_chooseGameLibFunc.SetAddCallback(new Runnable()
            {
                @Override
                public void run()
                {
                    if (null == m_addExternalLibraryFunc)
                        m_addExternalLibraryFunc = new AddExternalLibraryFunc(GameLauncher.this, new Runnable()
                        {
                            @Override
                            public void run()
                            {
                                OpenGameLibChooser();
                            }
                        }, CONST_RESULT_CODE_REQUEST_ADD_EXTERNAL_GAME_LIBRARY);
                    Bundle bundle = new Bundle();
                    bundle.putString("path", GetExternalGameLibraryPath());
                    m_addExternalLibraryFunc.Start(bundle);
                }
            });
            m_chooseGameLibFunc.SetEditCallback(new Runnable()
            {
                @Override
                public void run()
                {
                    if (null == m_editExternalLibraryFunc)
                        m_editExternalLibraryFunc = new EditExternalLibraryFunc(GameLauncher.this, new Runnable()
                        {
                            @Override
                            public void run()
                            {
                                OpenGameLibChooser();
                            }
                        }, CONST_RESULT_CODE_REQUEST_EDIT_EXTERNAL_GAME_LIBRARY);
                    Bundle bundle = new Bundle();
                    bundle.putString("path", GetExternalGameLibraryPath());
                    m_editExternalLibraryFunc.Start(bundle);
                }
            });
        }

        m_chooseGameLibFunc.SetCallback(new Runnable()
        {
            @Override
            public void run()
            {
                final SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(GameLauncher.this);
                String lib = m_chooseGameLibFunc.GetResult();
                if (null != lib && !lib.isEmpty())
                {
                    preference.edit().putString(PreferenceKey, lib).commit();
                    SetProp("harm_fs_gameLibPath", lib);
                }
                else
                {
                    preference.edit().putString(PreferenceKey, "").commit();
                    RemoveProp("harm_fs_gameLibPath");
                }
            }
        });
        Bundle bundle = new Bundle();
        bundle.putString("key", PreferenceKey);
        bundle.putString("path", GetExternalGameLibraryPath());
        m_chooseGameLibFunc.Start(bundle);
    }

    private void Test()
    {
		// test function
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
        for (int i = 0; i < count; i++)
        {
            layout.getChildAt(i).setEnabled(enabled);
        }

        layout.setEnabled(enabled);
    }

    private void SetGameDLL(String val)
    {
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
        boolean userMod = V.fs_game_user.isChecked(); //preference.getBoolean(Q3EUtils.q3ei.GetEnableModPreferenceKey(), false);
        String game = null != val ? val : "";
		GameManager.GameProp prop = m_gameManager.ChangeGameMod(game, userMod);
		HandleGameProp(prop);

        if (userMod)
        {
            V.edt_fs_game.setText(prop.fs_game);
        }
        preference.edit().putString(Q3EUtils.q3ei.GetGameModPreferenceKey(), game).commit();
    }

    private boolean LockCmdUpdate()
    {
    	if(m_cmdUpdateLock)
    		return false;
        m_cmdUpdateLock = true;
        return true;
    }

    private void UnlockCmdUpdate()
    {
        m_cmdUpdateLock = false;
    }

    private boolean IsCmdUpdateLocked()
    {
        return m_cmdUpdateLock;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == RESULT_OK)
        {
            switch (requestCode)
            {
                case CONST_RESULT_CODE_REQUEST_BACKUP_PREFERENCES_CHOOSE_FILE:
                    if (null != m_backupPreferenceFunc)
                    {
                        Bundle bundle = new Bundle();
                        bundle.putParcelable("uri", data.getData());
                        m_backupPreferenceFunc.Start(bundle);
                    }
                    break;
                case CONST_RESULT_CODE_REQUEST_RESTORE_PREFERENCES_CHOOSE_FILE:
                    if (null != m_restorePreferenceFunc)
                    {
                        Bundle bundle = new Bundle();
                        bundle.putParcelable("uri", data.getData());
                        m_restorePreferenceFunc.Start(bundle);
                    }
                    break;
                case CONST_RESULT_CODE_REQUEST_ADD_EXTERNAL_GAME_LIBRARY:
                    if (null != m_addExternalLibraryFunc)
                    {
                        Bundle bundle = new Bundle();
                        bundle.putParcelable("uri", data.getData());
                        m_addExternalLibraryFunc.Start(bundle);
                    }
					break;
				case CONST_RESULT_CODE_REQUEST_EXTRACT_SOURCE:
					if (null != m_extractSourceFunc)
					{
						Bundle bundle = new Bundle();
						bundle.putParcelable("uri", data.getData());
						m_extractSourceFunc.Start(bundle);
					}
					break;
				case CONST_RESULT_CODE_ACCESS_ANDROID_DATA:
					ContextUtility.PersistableUriPermission(this, data.getData());
					break;
            }
        }
		else
		{
			// Android SDK > 28: resultCode is RESULT_CANCELED
			if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) // Android 11 FS permission
			{
				if(requestCode == CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_START
						|| requestCode == CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_EDIT_CONFIG_FILE
						|| requestCode == CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_FOLDER
						|| requestCode == CONST_RESULT_CODE_REQUEST_EXTRACT_PATCH_RESOURCE
						|| requestCode == CONST_RESULT_CODE_REQUEST_BACKUP_PREFERENCES_CHOOSE_FILE
						|| requestCode == CONST_RESULT_CODE_REQUEST_RESTORE_PREFERENCES_CHOOSE_FILE
						|| requestCode == CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_LIBRARY
						|| requestCode == CONST_RESULT_CODE_REQUEST_ADD_EXTERNAL_GAME_LIBRARY
						|| requestCode == CONST_RESULT_CODE_REQUEST_EDIT_EXTERNAL_GAME_LIBRARY
						|| requestCode == CONST_RESULT_CODE_REQUEST_EXTRACT_SOURCE
						|| requestCode == CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_MOD
				)
				{
					List<String> list = null;
					if (!Environment.isExternalStorageManager()) // reject
					{
						list = Collections.singletonList(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
					}
					HandleGrantPermissionResult(requestCode, list);
				}
			}
		}
    }

    private void HandleGrantPermissionResult(int requestCode, List<String> list)
    {
        if (null == list || list.isEmpty())
        {
            switch (requestCode)
            {
                case CONST_RESULT_CODE_REQUEST_EXTRACT_PATCH_RESOURCE:
                    if (null != m_extractPatchResourceFunc)
                        m_extractPatchResourceFunc.run();
                    break;
                case CONST_RESULT_CODE_REQUEST_BACKUP_PREFERENCES_CHOOSE_FILE:
                    if (null != m_backupPreferenceFunc)
                        m_backupPreferenceFunc.run();
                    break;
                case CONST_RESULT_CODE_REQUEST_RESTORE_PREFERENCES_CHOOSE_FILE:
                    if (null != m_restorePreferenceFunc)
                        m_restorePreferenceFunc.run();
                    break;
                case CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_EDIT_CONFIG_FILE:
                    if (null != m_editConfigFileFunc)
                        m_editConfigFileFunc.run();
                    break;
                case CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_FOLDER:
                    if (null != m_chooseGameFolderFunc)
                        m_chooseGameFolderFunc.run();
                    break;
                case CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_START:
                    if (null != m_startGameFunc)
                        m_startGameFunc.run();
                    break;
                case CONST_RESULT_CODE_REQUEST_ADD_EXTERNAL_GAME_LIBRARY:
                    if (null != m_addExternalLibraryFunc)
                        m_addExternalLibraryFunc.run();
                    break;
                case CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_LIBRARY:
                    if (null != m_chooseGameLibFunc)
                        m_chooseGameLibFunc.run();
                    break;
                case CONST_RESULT_CODE_REQUEST_EDIT_EXTERNAL_GAME_LIBRARY:
                    if (null != m_editExternalLibraryFunc)
                        m_editExternalLibraryFunc.run();
                    break;
				case CONST_RESULT_CODE_REQUEST_EXTRACT_SOURCE:
					if (null != m_extractSourceFunc)
						m_extractSourceFunc.run();
					break;
				case CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_MOD:
					if (null != m_chooseGameModFunc)
						m_chooseGameModFunc.run();
					break;
                default:
                    break;
            }
            return;
        }
        // not grant
        String opt;
        switch (requestCode)
        {
            case CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_START:
                opt = Q3ELang.tr(this, R.string.start_game);
                break;
            case CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_EDIT_CONFIG_FILE:
                opt = Q3ELang.tr(this, R.string.edit_config_file);
                break;
            case CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_FOLDER:
                opt = Q3ELang.tr(this, R.string.choose_data_folder);
                break;
            case CONST_RESULT_CODE_REQUEST_EXTRACT_PATCH_RESOURCE:
                opt = Q3ELang.tr(this, R.string.extract_resource);
                break;
            case CONST_RESULT_CODE_REQUEST_BACKUP_PREFERENCES_CHOOSE_FILE:
                opt = Q3ELang.tr(this, R.string.backup_settings);
                break;
            case CONST_RESULT_CODE_REQUEST_RESTORE_PREFERENCES_CHOOSE_FILE:
                opt = Q3ELang.tr(this, R.string.restore_settings);
                break;
            case CONST_RESULT_CODE_REQUEST_ADD_EXTERNAL_GAME_LIBRARY:
                opt = Q3ELang.tr(this, R.string.add_external_game_library);
                break;
            case CONST_RESULT_CODE_REQUEST_EDIT_EXTERNAL_GAME_LIBRARY:
                opt = Q3ELang.tr(this, R.string.edit_external_game_library);
                break;
            case CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_LIBRARY:
                opt = Q3ELang.tr(this, R.string.access_external_game_library);
                break;
            default:
                opt = Q3ELang.tr(this, R.string.operation);
                break;
        }
        StringBuilder sb = new StringBuilder();
        String endl = TextHelper.GetDialogMessageEndl();
        for (String str : list)
        {
            if (str != null)
                sb.append("  * ").append(str);
            sb.append(endl);
        }
        AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(this, opt + " " + Q3ELang.tr(this, R.string.request_necessary_permissions), TextHelper.GetDialogMessage(sb.toString()));
        builder.setNeutralButton(R.string.grant, new AlertDialog.OnClickListener()
        {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
                ContextUtility.OpenAppSetting(GameLauncher.this);
                dialog.dismiss();
            }
        });
        builder.create().show();
    }

    private void OpenOnScreenButtonSetting()
    {
        Intent intent = new Intent(this, OnScreenButtonConfigActivity.class);
        startActivity(intent);
    }

    private void OpenCvarListDetail()
    {
        ContextUtility.OpenMessageDialog(this, Q3ELang.tr(this, R.string.special_cvar_list), TextHelper.GetCvarText());
    }

    private void SetGame(String game)
    {
        Q3EUtils.q3ei.SetupGame(game);
        V.launcher_tab1_edit_doomconfig.setText(getString(R.string.edit_) + Q3EUtils.q3ei.config_name);
        if (null != V.main_menu_game)
            V.main_menu_game.setTitle(Q3EUtils.q3ei.game_name);
        ActionBar actionBar = getActionBar();
        Resources res = getResources();
        int colorId = GameManager.GetGameThemeColor();
        int iconId = GameManager.GetGameIcon();

        boolean d3Visible = false;
        boolean q4Visible = false;
        boolean preyVisible = false;
		boolean q1Visible = false;
		boolean q2Visible = false;
		boolean q3Visible = false;
		boolean rtcwVisible = false;
		boolean tdmVisible = false;
		boolean d3bfgVisible = false;

		boolean rendererVisible = true;
		boolean soundVisible = true;
		boolean otherVisible = true;
		boolean openglVisible = true;
		boolean modVisible = true;
		boolean dllVisible = true;
		boolean quickloadVisible = true;
		boolean skipintroVisible = true;

        if (Q3EUtils.q3ei.isPrey)
        {
            preyVisible = true;
        }
        else if (Q3EUtils.q3ei.isQ4)
        {
            q4Visible = true;
        }
		else if (Q3EUtils.q3ei.isQ1)
		{
			q1Visible = true;
			rendererVisible = false;
			soundVisible = false;
			otherVisible = false;
			openglVisible = false;
			modVisible = false;
			dllVisible = false;
			quickloadVisible = false;
			skipintroVisible = false;
		}
		else if (Q3EUtils.q3ei.isQ2)
		{
			q2Visible = true;
			rendererVisible = false;
			soundVisible = false;
			otherVisible = false;
			openglVisible = false;
			dllVisible = false;
			quickloadVisible = false;
			skipintroVisible = false;
		}
		else if (Q3EUtils.q3ei.isQ3)
		{
			q3Visible = true;
			rendererVisible = false;
			soundVisible = false;
			otherVisible = false;
			openglVisible = false;
			dllVisible = false;
			quickloadVisible = false;
		}
		else if (Q3EUtils.q3ei.isRTCW)
		{
			rtcwVisible = true;
			rendererVisible = false;
			soundVisible = false;
			otherVisible = false;
			openglVisible = false;
			dllVisible = false;
		}
		else if (Q3EUtils.q3ei.isTDM)
		{
			tdmVisible = true;
			rendererVisible = false;
			soundVisible = false;
			otherVisible = false;
			openglVisible = false;
			modVisible = false;
			dllVisible = false;
			quickloadVisible = false;
			skipintroVisible = false;
		}
		else if (Q3EUtils.q3ei.isD3BFG)
		{
			d3bfgVisible = true;
			rendererVisible = false;
			soundVisible = false;
			otherVisible = false;
			openglVisible = false;
			dllVisible = false;
			quickloadVisible = false;
			skipintroVisible = false;
		}
        else
        {
            d3Visible = true;
        }
        if (null != actionBar)
        {
            actionBar.setBackgroundDrawable(new ColorDrawable(res.getColor(colorId)));
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
            {
                actionBar.setHomeAsUpIndicator(iconId);
            }
        }
        V.rg_fs_game.setVisibility(d3Visible ? View.VISIBLE : View.GONE);
        V.rg_fs_q4game.setVisibility(q4Visible ? View.VISIBLE : View.GONE);
        V.rg_fs_preygame.setVisibility(preyVisible ? View.VISIBLE : View.GONE);
		V.rg_fs_q1game.setVisibility(q1Visible ? View.VISIBLE : View.GONE);
		V.rg_fs_q2game.setVisibility(q2Visible ? View.VISIBLE : View.GONE);
		V.rg_fs_q3game.setVisibility(q3Visible ? View.VISIBLE : View.GONE);
		V.rg_fs_rtcwgame.setVisibility(rtcwVisible ? View.VISIBLE : View.GONE);
		V.rg_fs_tdmgame.setVisibility(tdmVisible ? View.VISIBLE : View.GONE);
		V.rg_fs_d3bfggame.setVisibility(d3bfgVisible ? View.VISIBLE : View.GONE);

		V.renderer_section.setVisibility(rendererVisible ? View.VISIBLE : View.GONE);
		V.sound_section.setVisibility(soundVisible ? View.VISIBLE : View.GONE);
		V.other_section.setVisibility(otherVisible ? View.VISIBLE : View.GONE);
		V.opengl_section.setVisibility(openglVisible ? View.VISIBLE : View.GONE);
		V.mod_section.setVisibility(modVisible ? View.VISIBLE : View.GONE);
		V.dll_section.setVisibility(dllVisible ? View.VISIBLE : View.GONE);
		V.auto_quick_load.setVisibility(quickloadVisible ? View.VISIBLE : View.GONE);
		V.skip_intro.setVisibility(skipintroVisible ? View.VISIBLE : View.GONE);

		String subdir = Q3EUtils.q3ei.subdatadir;
		if(null == subdir)
			subdir = "";
		else
			subdir += "/";
		V.launcher_fs_game_subdir.setText(Q3ELang.tr(this, R.string.sub_directory) + subdir);
		V.launcher_fs_game_subdir.setVisibility(subdir.isEmpty() ? View.GONE : View.VISIBLE);
    }

    private void ChangeGame(String... games)
    {
		LockCmdUpdate();
		SetupCommandTextWatcher(false);
        String newGame = games.length > 0 ? games[0] : null;
        if (null == newGame || newGame.isEmpty())
        {
            int i;
            for (i = 0; i < GameManager.Games.length; i++)
            {
                if (GameManager.Games[i].equalsIgnoreCase(Q3EUtils.q3ei.game))
                    break;
            }
            if (i >= GameManager.Games.length)
                i = GameManager.Games.length - 1;
            newGame = GameManager.Games[(i + 1) % GameManager.Games.length];
        }
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
        preference.edit().putString(Q3EPreference.pref_harm_game, newGame).commit();
        SetGame(newGame);
        preference.edit().putString(Q3EPreference.pref_harm_game_lib, "").commit();

		String cmd = preference.getString(Q3EUtils.q3ei.GetGameCommandPreferenceKey(), Q3EGlobals.GAME_EXECUABLE);
		V.edt_cmdline.setText(cmd);
		SetupCommandTextWatcher(true);
		UnlockCmdUpdate();

		// put last
		String game = preference.getString(Q3EUtils.q3ei.GetGameModPreferenceKey(), "");
		if (null == game)
			game = "";
		V.edt_fs_game.setText(game);
		boolean userMod = preference.getBoolean(Q3EUtils.q3ei.GetEnableModPreferenceKey(), false);
		V.fs_game_user.setChecked(userMod);

		GameManager.GameProp prop = m_gameManager.ChangeGameMod(game, userMod);
		HandleGameProp(prop);
		SelectCheckbox(GetGameModRadioGroup(), prop.index);
    }

	private void SetupCommandTextWatcher(boolean b)
	{
		if(b)
		{
			V.edt_cmdline.addTextChangedListener(m_commandTextWatcher);
			m_commandTextWatcher.Install(Q3EUtils.q3ei.IsIdTech4() || Q3EUtils.q3ei.IsIdTech2() || Q3EUtils.q3ei.IsIdTech3() || Q3EUtils.q3ei.IsTDMTech());
		}
		else
		{
			m_commandTextWatcher.Uninstall();
			V.edt_cmdline.removeTextChangedListener(m_commandTextWatcher);
		}
	}

    private void OpenResourceFileDialog()
    {
        if (null == m_extractPatchResourceFunc)
            m_extractPatchResourceFunc = new ExtractPatchResourceFunc(this, CONST_RESULT_CODE_REQUEST_EXTRACT_PATCH_RESOURCE);
        Bundle bundle = new Bundle();
        bundle.putString("path", V.edt_path.getText().toString());
        m_extractPatchResourceFunc.Start(bundle);
    }

    private boolean RemoveParam(String name)
    {
        if(!LockCmdUpdate())
        	return false;
        boolean[] res = {false};
        String str = KidTech4Command.RemoveParam(GetCmdText(), name, res);
        if (res[0])
            SetCmdText(str);
        UnlockCmdUpdate();
        return res[0];
    }

    private void SetParam(String name, Object val)
    {
		if(!LockCmdUpdate())
			return;
        SetCmdText(KidTech4Command.SetParam(GetCmdText(), name, val));
        UnlockCmdUpdate();
    }

    private String GetParam(String name)
    {
        return KidTech4Command.GetParam(GetCmdText(), name);
    }

	private String GetTempCmdText()
	{
		return V.edt_cmdline_temp.getText().toString();
	}

    private void RemoveParam_temp(String name)
    {
        boolean[] res = {false};
        String str = KidTech4Command.RemoveParam(GetTempCmdText(), name, res);
        if (res[0])
            Q3EUtils.q3ei.start_temporary_extra_command = str;
		UpdateTempCommand();
    }

    private void SetParam_temp(String name, Object val)
    {
        Q3EUtils.q3ei.start_temporary_extra_command = (KidTech4Command.SetParam(GetTempCmdText(), name, val));
		UpdateTempCommand();
    }

	private void SetCommand_temp(String name, boolean prepend)
	{
		Q3EUtils.q3ei.start_temporary_extra_command = (KidTech4Command.SetCommand(GetTempCmdText(), name, prepend));
		UpdateTempCommand();
	}

	private void RemoveCommand_temp(String name)
	{
		boolean[] res = {false};
		String str = KidTech4Command.RemoveCommand(GetTempCmdText(), name, res);
		if (res[0])
			Q3EUtils.q3ei.start_temporary_extra_command = str;
		UpdateTempCommand();
	}

    private void ShowPreferenceDialog()
    {
        new DebugPreferenceFunc(this).Start(new Bundle());
    }

    private void UpdateMapVol(boolean on)
    {
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
        int[] keyCodes = getResources().getIntArray(R.array.key_map_codes);
        V.launcher_tab2_volume_map_config_layout.setVisibility(on ? View.VISIBLE : View.GONE);
        int key = preference.getInt(Q3EPreference.VOLUME_UP_KEY, Q3EKeyCodes.KeyCodes.K_F3);
        V.launcher_tab2_volume_up_map_config_keys.setSelection(Utility.ArrayIndexOf(keyCodes, key));
        key = preference.getInt(Q3EPreference.VOLUME_DOWN_KEY, Q3EKeyCodes.KeyCodes.K_F2);
        V.launcher_tab2_volume_down_map_config_keys.setSelection(Utility.ArrayIndexOf(keyCodes, key));
    }

    private void UpdateEnableGyro(boolean on)
    {
        V.launcher_tab2_enable_gyro_layout.setVisibility(on ? View.VISIBLE : View.GONE);
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
        V.launcher_tab2_gyro_x_axis_sens.setText(Q3EPreference.GetStringFromFloat(preference, Q3EPreference.pref_harm_view_motion_gyro_x_axis_sens, Q3EControlView.GYROSCOPE_X_AXIS_SENS));
        V.launcher_tab2_gyro_y_axis_sens.setText(Q3EPreference.GetStringFromFloat(preference, Q3EPreference.pref_harm_view_motion_gyro_y_axis_sens, Q3EControlView.GYROSCOPE_Y_AXIS_SENS));
    }

    private void OpenCheckForUpdateDialog()
    {
        if (null == m_checkForUpdateFunc)
            m_checkForUpdateFunc = new CheckForUpdateFunc(this);
        m_checkForUpdateFunc.Start(new Bundle());
    }

    private RadioGroup GetGameModRadioGroup()
    {
		if(Q3EUtils.q3ei.isQ4)
			return V.rg_fs_q4game;
		else if(Q3EUtils.q3ei.isPrey)
			return V.rg_fs_preygame;
		else if(Q3EUtils.q3ei.isQ1)
			return V.rg_fs_q1game;
		else if(Q3EUtils.q3ei.isQ2)
			return V.rg_fs_q2game;
		else if(Q3EUtils.q3ei.isQ3)
			return V.rg_fs_q3game;
		else if(Q3EUtils.q3ei.isRTCW)
			return V.rg_fs_rtcwgame;
		else if(Q3EUtils.q3ei.isTDM)
			return V.rg_fs_tdmgame;
		else if(Q3EUtils.q3ei.isD3BFG)
			return V.rg_fs_d3bfggame;
		else
        	return V.rg_fs_game;
    }

    private void RequestBackupPreferences()
    {
        if (null == m_backupPreferenceFunc)
            m_backupPreferenceFunc = new BackupPreferenceFunc(this, CONST_RESULT_CODE_REQUEST_BACKUP_PREFERENCES_CHOOSE_FILE);
        m_backupPreferenceFunc.Start(new Bundle());
    }

    private void RequestRestorePreferences()
    {
        if (null == m_restorePreferenceFunc)
            m_restorePreferenceFunc = new RestorePreferenceFunc(this, CONST_RESULT_CODE_REQUEST_RESTORE_PREFERENCES_CHOOSE_FILE);
        m_restorePreferenceFunc.Start(new Bundle());
    }

    private void OpenOnScreenButtonThemeSetting()
    {
        new SetupControlsThemeFunc(this).Start(new Bundle());
    }

    private String GetExternalGameLibraryPath()
    {
        return getFilesDir() + File.separator + Q3EUtils.q3ei.game + File.separator + Q3EGlobals.ARCH;
    }

    private void OpenTranslatorsDialog()
	{
		new TranslatorsFunc(this).Start(new Bundle());
	}

	private void EditCVar()
	{
		Bundle bundle = new Bundle();
		bundle.putString("game", GetProp(Q3EUtils.q3ei.GetGameCommandParm()));
		bundle.putString("command", Q3EUtils.q3ei.start_temporary_extra_command);
		bundle.putString("baseCommand", GetTempBaseCommand());
		CVarEditorFunc cVarEditorFunc = new CVarEditorFunc(this, new Runnable()
		{
			@Override
			public void run()
			{
				Q3EUtils.q3ei.start_temporary_extra_command = CVarEditorFunc.GetResultFromBundle(bundle);
				UpdateTempCommand();
			}
		});
		cVarEditorFunc.Start(bundle);
	}

	private String GetTempBaseCommand()
	{
		SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(GameLauncher.this);
		String tempCmd = "";
		boolean skipIntro = preferences.getBoolean(Q3EPreference.pref_harm_skip_intro, false);
		if (skipIntro)
			tempCmd += " +disconnect";
		boolean quickSave = preferences.getBoolean(Q3EPreference.pref_harm_auto_quick_load, false);
		if (quickSave && (Q3EUtils.q3ei.IsIdTech4() || Q3EUtils.q3ei.IsIdTech3() || Q3EUtils.q3ei.IsTDMTech()))
			tempCmd += " +loadGame QuickSave";
		return tempCmd.trim();
	}

	private void UpdateTempCommand()
	{
		if(Q3EUtils.q3ei.start_temporary_extra_command.equals(GetTempCmdText()))
			return;

		V.edt_cmdline_temp.setText(Q3EUtils.q3ei.start_temporary_extra_command);
		V.temp_cmdline.setVisibility(Q3EUtils.q3ei.start_temporary_extra_command.length() > 0 ? View.VISIBLE : View.GONE);
	}

	private void OpenGameModChooser()
	{
		SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
		String preferenceKey = Q3EUtils.q3ei.GetGameModPreferenceKey();
		if (null == m_chooseGameModFunc)
		{
			m_chooseGameModFunc = new ChooseGameModFunc(this, CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_MOD);
		}

		m_chooseGameModFunc.SetCallback(new Runnable()
		{
			@Override
			public void run()
			{
				String mod = m_chooseGameModFunc.GetResult();
				V.edt_fs_game.setText(mod);
			}
		});
		Bundle bundle = new Bundle();
		bundle.putString("mod", preference.getString(preferenceKey, ""));
		bundle.putString("path", KStr.AppendPath(preference.getString(Q3EPreference.pref_datapath, default_gamedata), Q3EUtils.q3ei.subdatadir));
		m_chooseGameModFunc.Start(bundle);
	}

	private void UpdateResolutionScaleScheme(int checkedId)
	{
		boolean usingPercent = checkedId == R.id.res_05x
				|| checkedId == R.id.res_2x
				|| checkedId == R.id.res_1p3x
				|| checkedId == R.id.res_1p4x
				;
		V.scale_by_screen_area.setEnabled(usingPercent);
	}

	public String GetDefaultGameDirectory()
	{
		if(Build.VERSION.SDK_INT > Build.VERSION_CODES.P)
			return Environment.getExternalStorageDirectory() + "/Android/data/" + getApplicationContext().getPackageName();
		else
			return default_gamedata;
	}

	private void HandleGameProp(GameManager.GameProp prop)
	{
		if(prop.is_user)
		{
			if (!prop.fs_game.isEmpty())
				SetProp(Q3EUtils.q3ei.GetGameCommandParm(), prop.fs_game);
			else
				RemoveProp(Q3EUtils.q3ei.GetGameCommandParm());
			RemoveProp("fs_game_base");
			RemoveProp("harm_fs_gameLibPath");
		}
		else
		{
			if(null == prop.fs_game || prop.fs_game.isEmpty() || !prop.IsValid())
				RemoveProp(Q3EUtils.q3ei.GetGameCommandParm());
			else
				SetProp(Q3EUtils.q3ei.GetGameCommandParm(), prop.fs_game);
			if(null == prop.fs_game_base || prop.fs_game_base.isEmpty() || !prop.IsValid())
				RemoveProp("fs_game_base");
			else
				SetProp("fs_game_base", prop.fs_game_base);
			RemoveProp("harm_fs_gameLibPath");
		}
	}

	public GameManager GetGameManager()
	{
		return m_gameManager;
	}

	private void InitGameList()
	{
		Map<String, RadioGroup> groups = new HashMap<>();
		RadioButton radio;
		RadioGroup group;
		RadioGroup.LayoutParams layoutParams;

		groups.put(Q3EGlobals.GAME_DOOM3, V.rg_fs_game);
		groups.put(Q3EGlobals.GAME_QUAKE4, V.rg_fs_q4game);
		groups.put(Q3EGlobals.GAME_PREY, V.rg_fs_preygame);
		groups.put(Q3EGlobals.GAME_QUAKE1, V.rg_fs_q1game);
		groups.put(Q3EGlobals.GAME_QUAKE2, V.rg_fs_q2game);
		groups.put(Q3EGlobals.GAME_QUAKE3, V.rg_fs_q3game);
		groups.put(Q3EGlobals.GAME_RTCW, V.rg_fs_rtcwgame);
		groups.put(Q3EGlobals.GAME_TDM, V.rg_fs_tdmgame);
		groups.put(Q3EGlobals.GAME_DOOM3BFG, V.rg_fs_d3bfggame);
		Game[] values = Game.values();

		for (Game value : values)
		{
			String subdir = "";

			if(Q3EUtils.q3ei.standalone)
			{
				subdir = Q3EInterface.GetGameStandaloneDirectory(value.type);
				if(!subdir.isEmpty())
					subdir += "/";
			}

			group = groups.get(value.type);
			layoutParams = new RadioGroup.LayoutParams(RadioGroup.LayoutParams.WRAP_CONTENT, RadioGroup.LayoutParams.WRAP_CONTENT);
			radio = new RadioButton(group.getContext());
			String name;
			if(value.name instanceof Integer)
				name = Q3ELang.tr(this, (Integer)value.name);
			else if(value.name instanceof String)
				name = (String)value.name;
			else
				name = "";
			name += "(" + subdir + value.game + ")";
			radio.setText(name);
			radio.setTag(value.game);
			group.addView(radio, layoutParams);
			radio.setChecked(!value.is_mod);
		}
	}
	private void SetupCommandLine(boolean readonly)
	{
		UIUtility.EditText__SetReadOnly(V.edt_cmdline, readonly, InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
	}

	private void SetupTempCommandLine(boolean editable)
	{
		UIUtility.EditText__SetReadOnly(V.edt_cmdline_temp, !editable, InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
	}

	public void OpenSuggestGameWorkingDirectory(String curPath)
	{
		if(ContextUtility.InScopedStorage() && !ContextUtility.IsInAppPrivateDirectory(GameLauncher.this, curPath))
		{
			String path = Q3EUtils.GetAppStoragePath(GameLauncher.this);
			Toast.makeText(GameLauncher.this, Q3ELang.tr(this, R.string.suggest_game_woring_directory_tips, path), Toast.LENGTH_LONG).show();
		}
		m_edtPathFocused = curPath;
	}



    private class ViewHolder
    {
        public MenuItem main_menu_game;
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
        public Button onscreen_button_setting;
        public LinearLayout launcher_tab1_user_game_layout;
        public RadioGroup rg_fs_q4game;
        public CheckBox fs_game_user;
        public LinearLayout launcher_tab2_volume_map_config_layout;
        public Spinner launcher_tab2_volume_up_map_config_keys;
        public Spinner launcher_tab2_volume_down_map_config_keys;
        public View main_ad_layout;
        public CheckBox launcher_tab2_enable_gyro;
        public LinearLayout launcher_tab2_enable_gyro_layout;
        public EditText launcher_tab2_gyro_x_axis_sens;
        public EditText launcher_tab2_gyro_y_axis_sens;
        public CheckBox auto_quick_load;
        public RadioGroup rg_fs_preygame;
        public CheckBox multithreading;
        public RadioGroup rg_s_driver;
        public CheckBox launcher_tab2_joystick_unfixed;
        public Button setup_onscreen_button_theme;
        public CheckBox using_mouse;
        public TextView tv_mprefs;
        public LinearLayout layout_mouse_device;
        public CheckBox find_dll;
		public EditText edt_harm_r_maxFps;
		public Button launcher_tab1_edit_cvar;
		public EditText edt_cmdline_temp;
		public CheckBox skip_intro;
		public Button launcher_tab1_game_mod_button;
		public CheckBox scale_by_screen_area;
		public RadioGroup rg_harm_r_shadow;
		public RadioGroup rg_opengl;
		public CheckBox cb_s_useOpenAL;
		public CheckBox cb_s_useEAXReverb;
		public Switch readonly_command;
		public CheckBox cb_translucentStencilShadow;
		public Switch editable_temp_command;
		public LinearLayout temp_cmdline;
		public LinearLayout renderer_section;
		public LinearLayout sound_section;
		public LinearLayout other_section;
		public LinearLayout opengl_section;
		public LinearLayout mod_section;
		public LinearLayout dll_section;
		public RadioGroup rg_fs_q1game;
		public RadioGroup rg_fs_q2game;
		public RadioGroup rg_fs_q3game;
		public RadioGroup rg_fs_rtcwgame;
		public RadioGroup rg_fs_tdmgame;
		public RadioGroup rg_fs_d3bfggame;
		public Spinner launcher_tab2_joystick_visible;
		public TextView launcher_fs_game_subdir;

        public void Setup()
        {
            edt_cmdline = findViewById(R.id.edt_cmdline);
            res_customlayout = findViewById(R.id.res_customlayout);
            nolight = findViewById(R.id.nolight);
            useetc1cache = findViewById(R.id.useetc1cache);
            useetc1 = findViewById(R.id.useetc1);
            usedxt = findViewById(R.id.usedxt);
            r_harmclearvertexbuffer = findViewById(R.id.r_harmclearvertexbuffer);
            rg_scrres = findViewById(R.id.rg_scrres);
            rg_msaa = findViewById(R.id.rg_msaa);
            rg_color_bits = findViewById(R.id.rg_color_bits);
            rg_fs_game = findViewById(R.id.rg_fs_game);
            edt_fs_game = findViewById(R.id.edt_fs_game);
            edt_mouse = findViewById(R.id.edt_mouse);
            edt_path = findViewById(R.id.edt_path);
            hideonscr = findViewById(R.id.hideonscr);
            mapvol = findViewById(R.id.mapvol);
            secfinglmb = findViewById(R.id.secfinglmb);
            smoothjoy = findViewById(R.id.smoothjoy);
            detectmouse = findViewById(R.id.detectmouse);
            layout_mouseconfig = findViewById(R.id.layout_mouseconfig);
            layout_manualmouseconfig = findViewById(R.id.layout_manualmouseconfig);
            launcher_tab1_game_lib_button = findViewById(R.id.launcher_tab1_game_lib_button);
            res_x = findViewById(R.id.res_x);
            res_y = findViewById(R.id.res_y);
            launcher_tab1_edit_autoexec = findViewById(R.id.launcher_tab1_edit_autoexec);
            launcher_tab1_edit_doomconfig = findViewById(R.id.launcher_tab1_edit_doomconfig);
            launcher_tab1_game_data_chooser_button = findViewById(R.id.launcher_tab1_game_data_chooser_button);
            rg_curpos = findViewById(R.id.rg_curpos);
            edt_harm_r_specularExponent = findViewById(R.id.edt_harm_r_specularExponent);
            rg_harm_r_lightModel = findViewById(R.id.rg_harm_r_lightModel);
            onscreen_button_setting = findViewById(R.id.onscreen_button_setting);
            launcher_tab1_user_game_layout = findViewById(R.id.launcher_tab1_user_game_layout);
            rg_fs_q4game = findViewById(R.id.rg_fs_q4game);
            fs_game_user = findViewById(R.id.fs_game_user);
            launcher_tab2_volume_map_config_layout = findViewById(R.id.launcher_tab2_volume_map_config_layout);
            launcher_tab2_volume_up_map_config_keys = findViewById(R.id.launcher_tab2_volume_up_map_config_keys);
            launcher_tab2_volume_down_map_config_keys = findViewById(R.id.launcher_tab2_volume_down_map_config_keys);
            main_ad_layout = findViewById(R.id.main_ad_layout);
            launcher_tab2_enable_gyro = findViewById(R.id.launcher_tab2_enable_gyro);
            launcher_tab2_enable_gyro_layout = findViewById(R.id.launcher_tab2_enable_gyro_layout);
            launcher_tab2_gyro_x_axis_sens = findViewById(R.id.launcher_tab2_gyro_x_axis_sens);
            launcher_tab2_gyro_y_axis_sens = findViewById(R.id.launcher_tab2_gyro_y_axis_sens);
            auto_quick_load = findViewById(R.id.auto_quick_load);
            rg_fs_preygame = findViewById(R.id.rg_fs_preygame);
            multithreading = findViewById(R.id.multithreading);
            rg_s_driver = findViewById(R.id.rg_s_driver);
            launcher_tab2_joystick_unfixed = findViewById(R.id.launcher_tab2_joystick_unfixed);
            setup_onscreen_button_theme = findViewById(R.id.setup_onscreen_button_theme);
            using_mouse = findViewById(R.id.using_mouse);
            tv_mprefs = findViewById(R.id.tv_mprefs);
            layout_mouse_device = findViewById(R.id.layout_mouse_device);
            find_dll = findViewById(R.id.find_dll);
			edt_harm_r_maxFps = findViewById(R.id.edt_harm_r_maxFps);
			launcher_tab1_edit_cvar = findViewById(R.id.launcher_tab1_edit_cvar);
			edt_cmdline_temp = findViewById(R.id.edt_cmdline_temp);
			skip_intro = findViewById(R.id.skip_intro);
			launcher_tab1_game_mod_button = findViewById(R.id.launcher_tab1_game_mod_button);
			scale_by_screen_area = findViewById(R.id.scale_by_screen_area);
			rg_harm_r_shadow = findViewById(R.id.rg_harm_r_shadow);
			rg_opengl = findViewById(R.id.rg_opengl);
			cb_s_useOpenAL = findViewById(R.id.cb_s_useOpenAL);
			cb_s_useEAXReverb = findViewById(R.id.cb_s_useEAXReverb);
			readonly_command = findViewById(R.id.readonly_command);
			cb_translucentStencilShadow = findViewById(R.id.cb_translucentStencilShadow);
			editable_temp_command = findViewById(R.id.editable_temp_command);
			temp_cmdline = findViewById(R.id.temp_cmdline);
			renderer_section = findViewById(R.id.renderer_section);
			sound_section = findViewById(R.id.sound_section);
			other_section = findViewById(R.id.other_section);
			opengl_section = findViewById(R.id.opengl_section);
			dll_section = findViewById(R.id.dll_section);
			mod_section = findViewById(R.id.mod_section);
			rg_fs_q1game = findViewById(R.id.rg_fs_q1game);
			rg_fs_q2game = findViewById(R.id.rg_fs_q2game);
			rg_fs_q3game = findViewById(R.id.rg_fs_q3game);
			rg_fs_rtcwgame = findViewById(R.id.rg_fs_rtcwgame);
			rg_fs_tdmgame = findViewById(R.id.rg_fs_tdmgame);
			rg_fs_d3bfggame = findViewById(R.id.rg_fs_d3bfggame);
			launcher_tab2_joystick_visible = findViewById(R.id.launcher_tab2_joystick_visible);
			launcher_fs_game_subdir = findViewById(R.id.launcher_fs_game_subdir);
        }
    }
}
