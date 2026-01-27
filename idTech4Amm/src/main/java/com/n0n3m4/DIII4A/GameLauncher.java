/*
	Copyright (C) 2012 n0n3m4
	
    This file is part of DIII4A.

    DIII4A is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    DIII4A is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DIII4A.  If not, see <http://www.gnu.org/licenses/>.
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
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.Switch;
import android.widget.TabHost;
import android.widget.TextView;
import android.widget.Toast;

import com.karin.idTech4Amm.ControllerConfigActivity;
import com.karin.idTech4Amm.OnScreenButtonConfigActivity;
import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.UIUtility;
import com.karin.idTech4Amm.lib.Utility;
import com.karin.idTech4Amm.misc.ChangeLog;
import com.karin.idTech4Amm.misc.Function;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.Constants;
import com.karin.idTech4Amm.sys.Game;
import com.karin.idTech4Amm.sys.GameManager;
import com.karin.idTech4Amm.sys.PreferenceKey;
import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.karin.Theme;
import com.karin.idTech4Amm.ui.ChangelogView;
import com.karin.idTech4Amm.ui.DebugDialog;
import com.karin.idTech4Amm.ui.AdvanceDialog;
import com.karin.idTech4Amm.ui.LauncherSettingsDialog;
import com.karin.idTech4Amm.widget.Divider;
import com.n0n3m4.DIII4A.launcher.AddExternalLibraryFunc;
import com.n0n3m4.DIII4A.launcher.BackupPreferenceFunc;
import com.n0n3m4.DIII4A.launcher.CVarEditorFunc;
import com.n0n3m4.DIII4A.launcher.CVarHelperFunc;
import com.n0n3m4.DIII4A.launcher.CheckForUpdateFunc;
import com.n0n3m4.DIII4A.launcher.ChooseCommandRecordFunc;
import com.n0n3m4.DIII4A.launcher.ChooseExtrasFileFunc;
import com.n0n3m4.DIII4A.launcher.ChooseGameFolderFunc;
import com.n0n3m4.DIII4A.launcher.ChooseGameLibFunc;
import com.n0n3m4.DIII4A.launcher.ChooseGameModFunc;
import com.n0n3m4.DIII4A.launcher.CreateCommandShortcutFunc;
import com.n0n3m4.DIII4A.launcher.CreateGameFolderFunc;
import com.n0n3m4.DIII4A.launcher.CreateShortcutFunc;
import com.n0n3m4.DIII4A.launcher.DebugPreferenceFunc;
import com.n0n3m4.DIII4A.launcher.DebugTextHistoryFunc;
import com.n0n3m4.DIII4A.launcher.DirectoryHelperFunc;
import com.n0n3m4.DIII4A.launcher.EditConfigFileFunc;
import com.n0n3m4.DIII4A.launcher.EditEnvFunc;
import com.n0n3m4.DIII4A.launcher.EditExternalLibraryFunc;
import com.n0n3m4.DIII4A.launcher.ExtractPatchResourceFunc;
import com.n0n3m4.DIII4A.launcher.ExtractSourceFunc;
import com.n0n3m4.DIII4A.launcher.GameAd;
import com.n0n3m4.DIII4A.launcher.GameChooserFunc;
import com.n0n3m4.DIII4A.launcher.OpenSourceLicenseFunc;
import com.n0n3m4.DIII4A.launcher.RestorePreferenceFunc;
import com.n0n3m4.DIII4A.launcher.SetupControlsThemeFunc;
import com.n0n3m4.DIII4A.launcher.StartGameFunc;
import com.n0n3m4.DIII4A.launcher.SupportDeveloperFunc;
import com.n0n3m4.DIII4A.launcher.TranslatorsFunc;
import com.n0n3m4.DIII4A.launcher.UpdateCompatFunc;
import com.n0n3m4.q3e.Q3EControlView;
import com.n0n3m4.q3e.Q3EGame;
import com.n0n3m4.q3e.Q3EGameConstants;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUiConfig;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.gl.Q3EGLConstants;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.KUncaughtExceptionHandler;
import com.n0n3m4.q3e.karin.KidTechCommand;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

@SuppressLint({"ApplySharedPref", "CommitPrefEdits"})
public class GameLauncher extends Activity
{
	private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_START               = 1;
	private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_EDIT_CONFIG_FILE    = 2;
	private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_FOLDER       = 3;
	private static final int CONST_RESULT_CODE_REQUEST_EXTRACT_PATCH_RESOURCE                   = 4;
	private static final int CONST_RESULT_CODE_REQUEST_BACKUP_PREFERENCES_CHOOSE_FILE           = 5;
	private static final int CONST_RESULT_CODE_REQUEST_RESTORE_PREFERENCES_CHOOSE_FILE          = 6;
	private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_LIBRARY = 7;
	private static final int CONST_RESULT_CODE_REQUEST_ADD_EXTERNAL_GAME_LIBRARY                = 8;
	private static final int CONST_RESULT_CODE_REQUEST_EDIT_EXTERNAL_GAME_LIBRARY               = 9;
	private static final int CONST_RESULT_CODE_REQUEST_EXTRACT_SOURCE                           = 10;
	private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_MOD     = 11;
	private static final int CONST_RESULT_CODE_ACCESS_ANDROID_DATA                              = 12;
	private static final int CONST_RESULT_CODE_REQUEST_CREATE_SHORTCUT                          = 13;
	private static final int CONST_RESULT_CODE_REQUEST_CREATE_SHORTCUT_WITH_COMMAND             = 14;
	private static final int CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_EXTRAS_FILE  = 15;
	private static final int CONST_RESULT_CODE_REQUEST_CREATE_GAME_FOLDER                       = 16;

	private final GameManager m_gameManager = new GameManager();
	private final Map<String, RadioGroup> groupRadios = new HashMap<>();
	private final Map<Integer, String> menuGames = new HashMap<>();
	private final Map<String, RadioGroup> versionGroupRadios = new HashMap<>();
    // GameLauncher function
	private ExtractPatchResourceFunc  m_extractPatchResourceFunc;
	private CheckForUpdateFunc        m_checkForUpdateFunc;
	private BackupPreferenceFunc      m_backupPreferenceFunc;
	private RestorePreferenceFunc     m_restorePreferenceFunc;
	private EditConfigFileFunc        m_editConfigFileFunc;
	private ChooseGameFolderFunc      m_chooseGameFolderFunc;
	private StartGameFunc             m_startGameFunc;
	private AddExternalLibraryFunc    m_addExternalLibraryFunc;
	private ChooseGameLibFunc         m_chooseGameLibFunc;
	private EditExternalLibraryFunc   m_editExternalLibraryFunc;
	private OpenSourceLicenseFunc     m_openSourceLicenseFunc;
	private ExtractSourceFunc         m_extractSourceFunc;
	private ChooseGameModFunc         m_chooseGameModFunc;
	private CreateShortcutFunc        m_createShortcutFunc;
	private CreateCommandShortcutFunc m_createCommandShortcutFunc;
	private ChooseExtrasFileFunc      m_chooseExtrasFileFunc;
	private CreateGameFolderFunc      m_createGameFolderFunc;
	private GameAd                    m_adFunc;

    public static String default_gamedata = Environment.getExternalStorageDirectory() + "/diii4a";
    private final ViewHolder V = new ViewHolder();
    private boolean m_cmdUpdateLock = false;
	private String m_edtPathFocused = "";
    private final CompoundButton.OnCheckedChangeListener m_checkboxChangeListener = new CompoundButton.OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
        {
			int id = buttonView.getId();
			if (id == R.id.useetc1cache)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("r_useETC1cache", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_useetc1cache, isChecked)
						.commit();
			}
			else if (id == R.id.nolight)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("r_noLight", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_nolight, isChecked)
						.commit();
			}
			else if (id == R.id.useetc1)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("r_useETC1", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_useetc1, isChecked)
						.commit();
			}
			else if (id == R.id.usedxt)
			{
				if(Q3E.q3ei.IsIdTech4())
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
						.putBoolean(Q3E.q3ei.GetEnableModPreferenceKey(), isChecked)
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
				if (isChecked && (Q3E.q3ei.IsIdTech4() || Q3E.q3ei.isRTCW || Q3E.q3ei.isRealRTCW))
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
			}
			else if (id == R.id.using_mouse)
			{
				UpdateMouseMenu(isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_using_mouse, isChecked)
						.commit();
			}
			else if (id == R.id.tab2_hide_joystick_center)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_hide_joystick_center, isChecked)
						.commit();
			}
			else if (id == R.id.tab2_disable_mouse_button_motion)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_disable_mouse_button_motion, isChecked)
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
			else if (id == R.id.cb_s_useOpenAL)
			{
				if(Q3E.q3ei.IsIdTech4())
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
				if(Q3E.q3ei.IsIdTech4())
					setProp("s_useEAXReverb", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_s_useEAXReverb, isChecked)
						.commit();
			}
			else if (id == R.id.readonly_command)
			{
				SetupCommandLine(!isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(PreferenceKey.READONLY_COMMAND, !isChecked)
						.commit();
			}
			else if (id == R.id.editable_temp_command)
			{
				SetupTempCommandLine(isChecked);
			}
			else if (id == R.id.cb_stencilShadowTranslucent)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("harm_r_stencilShadowTranslucent", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_stencilShadowTranslucent, isChecked)
						.commit();
			}
			else if (id == R.id.cb_stencilShadowSoft)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("harm_r_stencilShadowSoft", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_stencilShadowSoft, isChecked)
						.commit();
			}
			else if (id == R.id.cb_stencilShadowCombine)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("harm_r_stencilShadowCombine", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_stencilShadowCombine, isChecked)
						.commit();
			}
			else if (id == R.id.image_useetc2)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("r_useETC2", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_image_useetc2, isChecked)
						.commit();
			}
			else if (id == R.id.cb_perforatedShadow)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("r_forceShadowMapsOnAlphaTestedSurfaces", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_shadowMapPerforatedShadow, isChecked)
						.commit();
			}
			else if (id == R.id.collapse_mods)
			{
				CollapseMods(isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(PreferenceKey.COLLAPSE_MODS, isChecked)
						.commit();
			}
			else if (id == R.id.cb_useHighPrecision)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("harm_r_useHighPrecision", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_useHighPrecision, isChecked)
						.commit();
			}
			else if (id == R.id.cb_renderToolsMultithread)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("harm_r_renderToolsMultithread", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_renderToolsMultithread, isChecked)
						.commit();
			}
			else if (id == R.id.cb_r_autoAspectRatio)
			{
				int value = isChecked ? 1 : 0;
				if(Q3E.q3ei.IsIdTech4())
				{
					if(isChecked)
						SetProp_temp("harm_r_autoAspectRatio", value);
					else
						RemoveProp_temp("harm_r_autoAspectRatio");
				}
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_harm_r_autoAspectRatio, value)
						.commit();
			}
			else if (id == R.id.cb_r_occlusionCulling)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("harm_r_occlusionCulling", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_occlusionCulling, isChecked)
						.commit();
			}
			else if (id == R.id.cb_gui_useD3BFGFont)
			{
				if(Q3E.q3ei.IsIdTech4())
				{
					setProp("harm_gui_useD3BFGFont", isChecked);
					setProp("harm_gui_wideCharLang", isChecked);
				}
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_gui_useD3BFGFont, isChecked)
						.commit();
			}
			else if (id == R.id.cb_shadowMapCombine)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("harm_r_shadowMapCombine", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_shadowMapCombine, isChecked)
						.commit();
			}
			else if (id == R.id.cb_g_skipHitEffect)
			{
				if(Q3E.q3ei.IsIdTech4())
				{
					setProp("harm_g_skipHitEffect", isChecked);
				}
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_g_skipHitEffect, isChecked)
						.commit();
			}
			else if (id == R.id.cb_r_globalIllumination)
			{
				if(Q3E.q3ei.IsIdTech4())
					setProp("harm_r_globalIllumination", isChecked);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_globalIllumination, isChecked)
						.commit();
			}
			else if (id == R.id.cb_g_botEnableBuiltinAssets)
			{
				if(Q3E.q3ei.IsIdTech4())
				{
					setProp("harm_g_botEnableBuiltinAssets", isChecked);
				}
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_g_botEnableBuiltinAssets, isChecked)
						.commit();
			}
			else if (id == R.id.use_custom_resolution)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_ratio_use_custom_resolution, isChecked)
						.commit();

				if(V.rg_scrres.getCheckedRadioButtonId() == R.id.rg_scrres_scheme_ratio)
					UpdateCustomResolution(isChecked);
				UpdateResolutionText();
			}

			// Doom 3 BFG
			else if (id == R.id.doom3bfg_useCompressionCache)
			{
				if(Q3E.q3ei.isD3BFG)
					setProp("harm_image_useCompressionCache", isChecked);
			}
			else if (id == R.id.doom3bfg_useMediumPrecision)
			{
				if(Q3E.q3ei.isD3BFG)
					setProp("harm_r_useMediumPrecision", isChecked);
			}

			// RealRTCW
			else if (id == R.id.realrtcw_sv_cheats)
			{
				if(Q3E.q3ei.isRealRTCW)
					setProp("harm_sv_cheats", isChecked);
			}
			else if (id == R.id.realrtcw_stencilShadowPersonal)
			{
				if(Q3E.q3ei.isRealRTCW)
					setProp("harm_r_stencilShadowPersonal", isChecked);
			}

			// ETW
			else if (id == R.id.etw_omnibot_enable)
			{
				if(Q3E.q3ei.isETW)
					setProp("omnibot_enable", isChecked);
			}
			else if (id == R.id.etw_stencilShadowPersonal)
			{
				if(Q3E.q3ei.isETW)
					setProp("harm_r_stencilShadowPersonal", isChecked);
			}
			else if (id == R.id.etw_ui_disableAndroidMacro)
			{
				if(Q3E.q3ei.isETW)
					setProp("harm_ui_disableAndroidMacro", isChecked);
			}

			// ZDOOM
			else if (id == R.id.zdoom_load_lights_pk3)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_zdoom_load_lights_pk3, isChecked)
						.commit();
				SetupZDOOMFiles("file", "lights.pk3", isChecked);
			}
			else if (id == R.id.zdoom_load_brightmaps_pk3)
			{
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_zdoom_load_brightmaps_pk3, isChecked)
						.commit();
				SetupZDOOMFiles("file", "brightmaps.pk3", isChecked);
			}

			// The Dark Mod
			else if (id == R.id.tdm_useMediumPrecision)
			{
				if(Q3E.q3ei.isTDM)
					setProp("harm_r_useMediumPrecision", isChecked);
			}
        }
    };
    private final RadioGroup.OnCheckedChangeListener m_groupCheckChangeListener = new RadioGroup.OnCheckedChangeListener() {
        @Override
        public void onCheckedChanged(RadioGroup radioGroup, int id)
        {
            int index;
			int rgId = radioGroup.getId();
			if (rgId == R.id.r_harmclearvertexbuffer)
			{
				index = GetRadioGroupSelectIndex(radioGroup, id);
				if(Q3E.q3ei.IsIdTech4())
					SetProp("harm_r_clearVertexBuffer", index);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_harm_r_harmclearvertexbuffer, index)
						.commit();
			}
			else if (rgId == R.id.rg_scrres)
			{
				index = GetRadioGroupSelectIndex(radioGroup, id);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_scrres_scheme, index)
						.commit();
				UpdateResolution(id);
			}
			else if (rgId == R.id.rg_harm_r_lightingModel)
			{
				String value = "" + ((GetRadioGroupSelectIndex(radioGroup, id) + 1) % V.rg_harm_r_lightingModel.getChildCount());
				if(Q3E.q3ei.IsIdTech4())
					SetProp("harm_r_lightingModel", value);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putString(Q3EPreference.pref_harm_r_lightingModel, value)
						.commit();
			}
			else if (rgId == R.id.rg_msaa)
			{
				index = GetRadioGroupSelectIndex(radioGroup, id);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_msaa, index)
						.commit();
			}
			else if (rgId == R.id.rg_color_bits)
			{
				index = GetRadioGroupSelectIndex(radioGroup, id) - 1;
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_32bit, index == -1)
						.putInt(Q3EPreference.pref_harm_16bit, index)
						.commit();
			}
			else if (rgId == R.id.rg_curpos)
			{
				index = GetRadioGroupSelectIndex(radioGroup, id);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_mousepos, index)
						.commit();
			}
			else if (rgId == R.id.rg_s_driver)
			{
				String value2 = GetRadioGroupSelectIndex(radioGroup, id) == 1 ? "OpenSLES" : "AudioTrack";
				if(Q3E.q3ei.IsIdTech4())
					SetProp("s_driver", value2);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putString(Q3EPreference.pref_harm_s_driver, value2)
						.commit();
			}
			else if (rgId == R.id.rg_harm_r_shadow)
			{
				boolean useShadowMapping = GetRadioGroupSelectIndex(radioGroup, id) == 1;
				String value = useShadowMapping ? "1" : "0";
				if(Q3E.q3ei.IsIdTech4())
					SetProp("r_useShadowMapping", value);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putBoolean(Q3EPreference.pref_harm_r_useShadowMapping, useShadowMapping)
						.commit();
			}
			else if (rgId == R.id.rg_opengl)
			{
				int glVersion = GetRadioGroupSelectIndex(radioGroup, id) == 1 ? Q3EGLConstants.OPENGLES30 : Q3EGLConstants.OPENGLES20;
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_harm_opengl, glVersion)
						.commit();
			}
/*			else if (rgId == R.id.rg_r_autoAspectRatio)
			{
				index = GetRadioGroupSelectIndex(radioGroup, id);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_harm_r_autoAspectRatio, index)
						.commit();
				if (index > 0 && Q3E.q3ei.IsIdTech4())
					SetProp_temp("harm_r_autoAspectRatio", index);
				else
					RemoveProp_temp("harm_r_autoAspectRatio");
			}*/
			else if (rgId == R.id.rg_depth_bits)
			{
				index = GetRadioGroupSelectIndex(radioGroup, id);
				int bits = Q3EPreference.DepthBitsByIndex(index);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_harm_depth_bit, bits)
						.commit();
			}

			// change game mods
			else if (groupRadios.values().contains(radioGroup))
			{
				RadioButton checked = radioGroup.findViewById(id);
				SetGameInternalMod((String)checked.getTag());
			}

			// change game version
			else if (
					rgId == R.id.rg_version_realrtcw
							|| rgId == R.id.rg_version_d3bfg
							|| rgId == R.id.rg_version_tdm
			)
			{
				RadioButton checked = radioGroup.findViewById(id);
				SetGameVersion((String)checked.getTag());
			}

			// Quake 2
			else if (rgId == R.id.yquake2_vid_renderer)
			{
				if(Q3E.q3ei.isQ2)
				{
					index = GetRadioGroupSelectIndex(radioGroup, id);
					if(index < 0 || index >= Q3EGameConstants.QUAKE2_RENDERER_BACKENDS.length)
						index = 0;
					String value2 = Q3EGameConstants.QUAKE2_RENDERER_BACKENDS[index];
					SetProp("vid_renderer", value2);
				}
			}

			// Doom 3 BFG
			else if (rgId == R.id.doom3bfg_useCompression)
			{
				if(Q3E.q3ei.isD3BFG)
				{
					index = GetRadioGroupSelectIndex(radioGroup, id);
					SetProp("harm_image_useCompression", index);
				}
			}

			// RealRTCW
			else if (rgId == R.id.realrtcw_shadows)
			{
				if(Q3E.q3ei.isRealRTCW)
				{
					index = GetRadioGroupSelectIndex(radioGroup, id);
					SetProp("cg_shadows", index);
				}
			}

			// ETW
			else if (rgId == R.id.etw_shadows)
			{
				if(Q3E.q3ei.isETW)
				{
					index = GetRadioGroupSelectIndex(radioGroup, id);
					SetProp("cg_shadows", index);
				}
			}

			// ZDOOM
			else if (rgId == R.id.zdoom_vid_preferbackend)
			{
				if(Q3E.q3ei.isDOOM)
				{
					int value2 = GetRadioGroupSelectIndex(radioGroup, id);
					RemovePropPrefix(KidTechCommand.ARG_PREFIX_ALL, "vid_preferbackend");
					SetPropPrefix(KidTechCommand.ARG_PREFIX_IDTECH, "vid_preferbackend", value2);
				}
			}
			else if (rgId == R.id.zdoom_gl_es)
			{
				if(Q3E.q3ei.isDOOM)
				{
					int value2 = GetRadioGroupSelectIndex(radioGroup, id);
					if(value2 > 0)
						value2++;
					RemovePropPrefix(KidTechCommand.ARG_PREFIX_ALL, "harm_gl_es");
					SetPropPrefix(KidTechCommand.ARG_PREFIX_IDTECH, "harm_gl_es", value2);
				}
			}
			else if (rgId == R.id.zdoom_gl_version)
			{
				if(Q3E.q3ei.isDOOM)
				{
					int value2 = GetRadioGroupSelectIndex(radioGroup, id);
					String value = value2 >= 0 && value2 < Q3EGameConstants.ZDOOM_GL_VERSIONS.length ? Q3EGameConstants.ZDOOM_GL_VERSIONS[value2] : Q3EGameConstants.ZDOOM_GL_VERSIONS[0];
					RemovePropPrefix(KidTechCommand.ARG_PREFIX_ALL, "harm_gl_version");
					SetPropPrefix(KidTechCommand.ARG_PREFIX_IDTECH, "harm_gl_version", value);
				}
			}

			// FTEQW
			else if (rgId == R.id.fteqw_vid_renderer)
			{
				if(Q3E.q3ei.isFTEQW)
				{
					int value2 = GetRadioGroupSelectIndex(radioGroup, id);
					RemovePropPrefix(KidTechCommand.ARG_PREFIX_ALL, "vid_renderer");
					SetPropPrefix(KidTechCommand.ARG_PREFIX_IDTECH, "vid_renderer", value2 == 0 ? "vk" : "gl");
				}
			}

			// Xash3D
			else if (rgId == R.id.xash3d_ref)
			{
				if(Q3E.q3ei.isXash3D)
				{
					index = GetRadioGroupSelectIndex(radioGroup, id);
					if(index < 0 || index >= Q3EGameConstants.XASH3D_REFS.length)
						index = 0;
					String value2 = Q3EGameConstants.XASH3D_REFS[index];
					SetParam("ref", value2);
				}
			}
			else if (rgId == R.id.xash3d_sv_cl)
			{
				if(Q3E.q3ei.isXash3D)
				{
					index = GetRadioGroupSelectIndex(radioGroup, id);
					if(index < 0 || index >= Q3EGameConstants.XASH3D_SV_CLS.length)
						index = 0;
					String value2 = Q3EGameConstants.XASH3D_SV_CLS[index];
					if(KStr.NotEmpty(value2))
						SetParam("sv_cl", value2);
					else
						RemoveParam("sv_cl");
				}
			}

			// Source Engine
			else if (rgId == R.id.source_sv_cl)
			{
				if(Q3E.q3ei.isSource)
				{
					index = GetRadioGroupSelectIndex(radioGroup, id);
					if(index < 0 || index >= Q3EGameConstants.SOURCE_ENGINE_SV_CLS.length)
						index = 0;
					String value2 = Q3EGameConstants.SOURCE_ENGINE_SV_CLS[index];
					if(KStr.NotEmpty(value2))
						SetParam("sv_cl", value2);
					else
						RemoveParam("sv_cl");
				}
			}

			// SDL
			else if (rgId == R.id.sdl_audio_driver)
			{
				index = GetRadioGroupSelectIndex(radioGroup, id);
				if(index < 0 || index >= Q3EGameConstants.SDL_AUDIO_DRIVER.length)
					index = 0;
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putString(Q3EPreference.pref_harm_sdl_audio_driver, Q3EGameConstants.SDL_AUDIO_DRIVER[index])
						.commit();
			}

			// OpenAL
			else if (rgId == R.id.openal_driver)
			{
				index = GetRadioGroupSelectIndex(radioGroup, id);
				if(index < 0 || index >= Q3EGameConstants.OPENAL_DRIVER.length)
					index = 0;
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putString(Q3EPreference.pref_harm_openal_driver, Q3EGameConstants.OPENAL_DRIVER[index])
						.commit();
			}
        }
    };
    private final View.OnClickListener m_buttonClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View view)
        {
			int id = view.getId();
			if (id == R.id.launcher_tab1_edit_config)
			{
				EditFile();
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
			else if (id == R.id.launcher_tab1_command_record)
			{
				OpenCommandChooser();
			}
			else if (id == R.id.launcher_tab1_create_shortcut)
			{
				OpenShortcutWithCommandCreator();
			}
			else if (id == R.id.show_directory_helper)
			{
				OpenDirectoryHelper();
			}
			else if (id == R.id.launcher_tab1_patch_resource)
			{
				OpenResourceFileDialog(false);
			}
			else if (id == R.id.zdoom_choose_extras_file)
			{
				OpenExtrasFileChooser();
			}
			else if (id == R.id.launcher_tab1_change_game)
			{
				OpenGameList();
			}
			else if (id == R.id.launcher_tab1_open_menu)
			{
				OpenMenu();
			}
			else if (id == R.id.setup_controller)
			{
				OpenControllerSetting();
			}
			else if (id == R.id.launcher_tab1_edit_env)
			{
				OpenEnvEditor();
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

			else if (viewId == R.id.spinner_r_renderMode)
			{
				if(Q3E.q3ei.IsIdTech4())
					SetProp("r_renderMode", position);
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.pref_harm_r_renderMode, position)
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

	private final SeekBar.OnSeekBarChangeListener m_seekListener = new SeekBar.OnSeekBarChangeListener() {
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
		{
			if(seekBar.getId() == V.consoleHeightFracValue.getId())
			{
				if(fromUser)
				{
					PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
							.putInt(Q3EPreference.pref_harm_max_console_height_frac, progress)
							.commit();
					V.consoleHeightFracText.setText(GetConsoleHeightText(progress));
				}
			}
		}

		@Override
		public void onStartTrackingTouch(SeekBar seekBar)
		{

		}

		@Override
		public void onStopTrackingTouch(SeekBar seekBar)
		{

		}
	};
    private class SavePreferenceTextWatcher implements TextWatcher
    {
        private final String name;
        private final String defValue;
		private final Runnable runnable;

        public SavePreferenceTextWatcher(String name, String defValue)
        {
            this(name, defValue, null);
        }

		public SavePreferenceTextWatcher(String name, String defValue, Runnable runnable)
		{
			this.name = name;
			this.defValue = defValue;
			this.runnable = runnable;
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
			if(null != runnable)
				runnable.run();
        }
    }
	private class SaveFloatPreferenceTextWatcher implements TextWatcher
	{
		private final String name;
		private final String preference;
		private final float defValue;

        public SaveFloatPreferenceTextWatcher(String name, String preference, float defValue)
		{
			this.name = name;
			this.preference = preference;
			this.defValue = defValue;
		}

		public void onTextChanged(CharSequence s, int start, int before, int count)
		{
			if(Q3E.q3ei.IsIdTech4())
				SetProp(name, s);
		}

		public void beforeTextChanged(CharSequence s, int start, int count, int after)
		{
		}

		public void afterTextChanged(Editable s)
		{
			String value = s.length() == 0 ? "" + defValue : s.toString();
			Q3EPreference.SetFloatFromString(GameLauncher.this, preference, value, defValue);
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
			String value = s.length() == 0 ? Q3EGameConstants.GAME_EXECUABLE : s.toString();
			PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
					.putString(Q3E.q3ei.GetGameCommandPreferenceKey(), value)
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

    public void InitQ3E(String game)
    {
        // Q3EKeyCodes.InitD3Keycodes();
        Q3EInterface q3ei = new Q3EInterface();
		SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		q3ei.standalone = mPrefs.getBoolean(Q3EPreference.GAME_STANDALONE_DIRECTORY, true);

        q3ei.InitD3();

		q3ei.InitUIDefaultLayout(this);

        q3ei.default_path = default_gamedata;
		q3ei.datadir = mPrefs.getString(Q3EPreference.pref_datapath, default_gamedata); //k add 20241113

        q3ei.SetupDOOM3(); //k armv7-a only support neon now

		if(null != game && !game.isEmpty())
		{
			q3ei.SetupGame(game);
			q3ei.SetupEngineVersion(this);
		}

		//q3ei.LoadTypeAndArgTablePreference(this);

        Q3E.q3ei = q3ei;
    }

	@Override
	public void onWindowFocusChanged(boolean hasFocus)
	{
		super.onWindowFocusChanged(hasFocus);
		if(hasFocus)
		{
			CollapseMods(V.collapse_mods.isChecked());
			CollapseCmdline(Q3EPreference.GetIntFromString(this, PreferenceKey.COLLAPSE_CMDLINE, 0));
		}
	}

	@Override
    public void onAttachedToWindow()
    {
        super.onAttachedToWindow();
		Q3E.q3ei.InitUIDefaultLayout(this);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        LoadAds();
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

	private void EnableRadioGroup(RadioGroup rg, boolean enabled)
	{
		for (int i = 0; i < rg.getChildCount(); i++)
		{
			View item = rg.getChildAt(i);
			if (item instanceof RadioButton)
			{
				item.setEnabled(enabled);
				//item.setClickable(enabled);
			}
		}
		rg.setEnabled(enabled);
		rg.setClickable(enabled);
	}

	private void SelectRadioGroup(RadioGroup rg, int index)
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


	private int GetRadioGroupSelectIndex(RadioGroup rg)
    {
        return GetRadioGroupSelectIndex(rg, rg.getCheckedRadioButtonId());
    }

	private int GetRadioGroupSelectIndex(RadioGroup rg, int id)
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

	private RadioButton GetRadioGroupSelectItem(RadioGroup rg)
	{
		for (int i = 0; i < rg.getChildCount(); i++)
		{
			View item = rg.getChildAt(i);
			if (item instanceof RadioButton)
			{
				RadioButton gb = (RadioButton)item;
				if(gb.isChecked())
					return gb;
			}
		}
		return null;
	}

	private int GetRadioGroupNum(RadioGroup rg)
	{
		int j = 0;
		for (int i = 0; i < rg.getChildCount(); i++)
		{
			View item = rg.getChildAt(i);
			if (item instanceof RadioButton)
				j++;
		}
		return j;
	}

    public boolean getProp(String name)
    {
		Boolean b = Q3E.q3ei.GetGameCommandEngine(GetCmdText()).GetBoolProp(name, false);
        // Boolean b = KidTech4Command.GetBoolProp(GetCmdText(), name, false);
        return null != b ? b : false;
    }

    public void setProp(String name, boolean val)
    {
        SetProp(name, KidTechCommand.btostr(val));
    }

	// update widget from command line
    public void updatehacktings()
    {
    	LockCmdUpdate();
		String str;

		if(Q3E.q3ei.IsIdTech4()) // only for idTech4 games
		{
			SyncCmdCheckbox(V.usedxt, "r_useDXT", false);
			SyncCmdCheckbox(V.useetc1, "r_useETC1", false);
			SyncCmdCheckbox(V.useetc1cache, "r_useETC1cache", false);
			SyncCmdCheckbox(V.nolight, "r_noLight", false);
			SyncCmdCheckbox(V.image_useetc2, "r_useETC2", false);
			SyncCmdRadioGroup(V.r_harmclearvertexbuffer, "harm_r_clearVertexBuffer", 2, 3);
			SyncCmdRadioGroup(V.rg_harm_r_lightingModel, "harm_r_lightingModel", 1, 1, V.rg_harm_r_lightingModel.getChildCount());
			SyncCmdEditText(V.edt_harm_r_specularExponent, "harm_r_specularExponent", "3.0");
			SyncCmdEditText(V.edt_harm_r_specularExponentBlinnPhong, "harm_r_specularExponentBlinnPhong", "12.0");
			SyncCmdEditText(V.edt_harm_r_specularExponentPBR, "harm_r_specularExponentPBR", "5.0");
			SyncCmdEditText(V.edt_harm_r_PBRNormalCorrection, "harm_r_PBRNormalCorrection", "0.25");
			SyncCmdEditText(V.edt_harm_r_ambientLightingBrightness, "harm_r_ambientLightingBrightness", "1.0");
			SyncCmdRadioGroupV(V.rg_s_driver, "s_driver", Q3EGameConstants.DOOM3_SOUND_DRIVER);
			SyncCmdEditText(V.edt_harm_r_maxFps, "r_maxFps", "0");
			SyncCmdRadioGroup(V.rg_harm_r_shadow, "r_useShadowMapping", 0);
			SyncCmdEditText(V.edt_harm_r_shadowMapAlpha, "harm_r_shadowMapAlpha", "1.0");
			SyncCmdCheckbox(V.cb_stencilShadowTranslucent, "harm_r_stencilShadowTranslucent", false);
			SyncCmdEditText(V.edt_harm_r_stencilShadowAlpha, "harm_r_stencilShadowAlpha", "1.0");
			SyncCmdCheckbox(V.cb_stencilShadowSoft, "harm_r_stencilShadowSoft", false);
			SyncCmdCheckbox(V.cb_stencilShadowCombine, "harm_r_stencilShadowCombine", false);
			SyncCmdCheckbox(V.cb_perforatedShadow, "r_forceShadowMapsOnAlphaTestedSurfaces", false);
			SyncCmdCheckbox(V.cb_shadowMapCombine, "harm_r_shadowMapCombine", true);

			V.cb_s_useOpenAL.setChecked(getProp("s_useOpenAL", true));
			if (!IsProp("s_useOpenAL"))
			{
				setProp("s_useOpenAL", false);
				V.cb_s_useEAXReverb.setChecked(false);
				setProp("s_useEAXReverb", false);
			}
			else
			{
				SyncCmdCheckbox(V.cb_s_useEAXReverb, "s_useEAXReverb", true);
			}

			SyncCmdCheckbox(V.cb_useHighPrecision, "harm_r_useHighPrecision", false);
			SyncCmdCheckbox(V.cb_renderToolsMultithread, "harm_r_renderToolsMultithread", true);
			SyncCmdCheckbox(V.cb_r_occlusionCulling, "harm_r_occlusionCulling", false);

			V.cb_gui_useD3BFGFont.setChecked(getProp("harm_gui_useD3BFGFont", false));
			if (!IsProp("harm_gui_useD3BFGFont"))
			{
				setProp("harm_gui_useD3BFGFont", false);
				setProp("harm_gui_wideCharLang", false);
			}
			else
			{
				if (!IsProp("harm_gui_wideCharLang")) setProp("harm_gui_wideCharLang", true);
			}

			SyncCmdCheckbox(V.cb_r_globalIllumination, "harm_r_globalIllumination", false);
			SyncCmdEditText(V.edt_r_globalIlluminationBrightness, "harm_r_globalIlluminationBrightness", "0.3");

			str = GetProp("r_renderMode");
			if (null != str)
				V.spinner_r_renderMode.setSelection(Q3EUtils.parseInt_s(str, 0));
			if (!IsProp("r_renderMode")) SetProp("r_renderMode", "0");

			SyncCmdCheckbox(V.cb_g_skipHitEffect, "harm_g_skipHitEffect", false);
			SyncCmdCheckbox(V.cb_g_botEnableBuiltinAssets, "harm_g_botEnableBuiltinAssets", false);
		}
		else if(Q3E.q3ei.isQ2)
		{
			Updatehacktings_Quake2();
		}
		else if(Q3E.q3ei.isD3BFG)
		{
			Updatehacktings_Doom3BFG();
		}
		else if(Q3E.q3ei.isRealRTCW)
		{
			Updatehacktings_RealRTCW();
		}
		else if(Q3E.q3ei.isETW)
		{
			Updatehacktings_ETW();
		}
		else if(Q3E.q3ei.isDOOM)
		{
			Updatehacktings_ZDOOM();
		}
		else if(Q3E.q3ei.isTDM)
		{
			Updatehacktings_TDM();
		}
		else if(Q3E.q3ei.isFTEQW)
		{
			Updatehacktings_FTEQW();
		}
		else if(Q3E.q3ei.isXash3D)
		{
			Updatehacktings_Xash3D();
		}
		else if(Q3E.q3ei.isSource)
		{
			Updatehacktings_Source();
		}
		else if(Q3E.q3ei.isUrT)
		{
			Updatehacktings_UrT();
		}

		// game mods for every games
		str = GetGameModFromCommand();
		if (!V.fs_game_user.isChecked()) // internal game
		{
			if (str != null)
			{
				GameManager.GameProp prop = m_gameManager.ChangeGameMod(str, false);
				if(prop.IsValid())
				{
					SelectRadioGroup(GetGameModRadioGroup(), prop.index);
				}
/*				else
				{
					RemoveGameModFromCommand();
					RemoveProp("fs_game_base");
				}*/
			}
			else
			{
				SelectRadioGroup(GetGameModRadioGroup(), -1);
			}
		}
		else // user mod
		{
			if (str != null)
			{
				GameManager.GameProp prop = m_gameManager.ChangeGameMod(str, false);
				if(!prop.IsValid())
				{
					String cur = V.edt_fs_game.getText().toString();
					if (!str.equals(cur))
						V.edt_fs_game.setText(str);
				}
			}
			else
			{
				{
					String cur = V.edt_fs_game.getText().toString();
					if (!"".equals(cur))
						V.edt_fs_game.setText("");
				}
			}
		}

		// graphics
		int checkedRadioButtonId = V.rg_scrres.getCheckedRadioButtonId();
		UpdateResolution(checkedRadioButtonId);

		UnlockCmdUpdate();
    }

	private void Updatehacktings_Quake2()
	{
		SyncCmdRadioGroupV(V.yquake2_vid_renderer, "vid_renderer", Q3EGameConstants.QUAKE2_RENDERER_BACKENDS);
	}

	private void Updatehacktings_Doom3BFG()
	{
		SyncCmdRadioGroup(V.doom3bfg_useCompression, "harm_image_useCompression", 0);
		SyncCmdCheckbox(V.doom3bfg_useCompressionCache, "harm_image_useCompressionCache", false);
		SyncCmdCheckbox(V.doom3bfg_useMediumPrecision, "harm_r_useMediumPrecision", false);
	}

	private void Updatehacktings_TDM()
	{
		SyncCmdCheckbox(V.tdm_useMediumPrecision, "harm_r_useMediumPrecision", false);
	}

	private void Updatehacktings_RealRTCW()
	{
		SyncCmdRadioGroup(V.realrtcw_shadows, "cg_shadows", 1);
		SyncCmdCheckbox(V.realrtcw_sv_cheats, "harm_sv_cheats", false);
		SyncCmdCheckbox(V.realrtcw_stencilShadowPersonal, "harm_r_stencilShadowPersonal", true);
	}

	private void Updatehacktings_ETW()
	{
		SyncCmdCheckbox(V.etw_omnibot_enable, "omnibot_enable", false);
		SyncCmdRadioGroup(V.etw_shadows, "cg_shadows", 1);
		SyncCmdCheckbox(V.etw_stencilShadowPersonal, "harm_r_stencilShadowPersonal", true);
		SyncCmdCheckbox(V.etw_ui_disableAndroidMacro, "harm_ui_disableAndroidMacro", false);
	}

	private void Updatehacktings_ZDOOM()
	{
		String str;

		str = GetPropPrefix(KidTechCommand.ARG_PREFIX_ALL, "vid_preferbackend");
		SyncCmdRadioGroup2(V.zdoom_vid_preferbackend, "vid_preferbackend", str, 2);

		str = GetPropPrefix(KidTechCommand.ARG_PREFIX_ALL, "harm_gl_es");
		SyncCmdRadioGroup2(V.zdoom_gl_es, "harm_gl_es", str, 1, 1, GetRadioGroupNum(V.zdoom_gl_es));

		str = GetPropPrefix(KidTechCommand.ARG_PREFIX_ALL, "harm_gl_version");
		SyncCmdRadioGroup2V(V.zdoom_gl_version, "harm_gl_version", str, Q3EGameConstants.ZDOOM_GL_VERSIONS);
	}

	private void Updatehacktings_FTEQW()
	{
		String str;

		str = GetPropPrefix(KidTechCommand.ARG_PREFIX_ALL, "vid_renderer");
		SyncCmdRadioGroup2i(V.fteqw_vid_renderer, "vid_renderer", str, 1, Q3EGameConstants.FTEQW_VID_RENDERER);
	}

	private void Updatehacktings_Xash3D()
	{
		String str;

		str = GetParam("ref");
		SyncCmdRadioGroup2V(V.xash3d_ref, "ref", str, Q3EGameConstants.XASH3D_REFS);

		str = GetParam("sv_cl");
		SyncCmdRadioGroup2V(V.xash3d_sv_cl, "sv_cl", str, Q3EGameConstants.XASH3D_SV_CLS);
	}

	private void Updatehacktings_Source()
	{
		String str;

		str = GetParam("sv_cl");
		SyncCmdRadioGroup2V(V.source_sv_cl, "sv_cl", str, Q3EGameConstants.SOURCE_ENGINE_SV_CLS);
	}

	private void Updatehacktings_UrT()
	{
		SyncCmdEditText(V.urt_bot_autoAdd, "harm_bot_autoAdd", "0");
		SyncCmdEditText(V.urt_bot_level, "harm_bot_level", "0");
	}

    private void ThrowException()
    {
        ((String) null).toString();
    }

    private void ShowDebugTextHistoryDialog()
    {
        new DebugTextHistoryFunc(this).Start(new Bundle());
    }

	private void EnableGameChooser(boolean enabled)
	{
		for(RadioGroup rg : groupRadios.values())
		{
			EnableRadioGroup(rg, enabled);
		}
	}

    private void UpdateUserGame(boolean on)
    {
		EnableGameChooser(!on);
		RadioGroup radioGroup = GetGameModRadioGroup();

		SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(GameLauncher.this);
		String game = null;
		boolean isUser;
		if(on) // user mod enabled
		{
			game = preference.getString(Q3E.q3ei.GetGameUserModPreferenceKey(), "");
			if (null == game)
				game = "";
			isUser = true;
		}
		else
		{
			RadioButton rb = GetRadioGroupSelectItem(radioGroup);
			if(null == rb)
			{
				SelectRadioGroup(radioGroup, 0);
				rb = GetRadioGroupSelectItem(radioGroup);
			}
			if(null != rb)
			{
				game = rb.getTag().toString();
				GameManager.GameProp prop = m_gameManager.ChangeGameMod(game, true);
				game = prop.fs_game;
				isUser = prop.is_user;
			}
			else
			{
				game = "";
				isUser = false;
			}
		}

        preference.edit().putString(Q3EPreference.pref_harm_game_lib, "").commit();
		if (!isUser && game.isEmpty())
			RemoveGameModFromCommand();
		else
			SetGameModToCommand(game);
        V.fs_game_user.setText(on ? R.string.mod_ : R.string.user_mod);
        V.edt_fs_game.setEnabled(on);
        //V.launcher_tab1_user_game_layout.setVisibility(on ? View.VISIBLE : View.GONE);
    }

    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

		default_gamedata = Q3EUtils.GetDefaultGameDirectory(this);
		KUncaughtExceptionHandler.DumpPID(this);
        Q3ELang.Locale(this);

        //k
        KUncaughtExceptionHandler.HandleUnexpectedException(this);
        setTitle(R.string.app_title);
        final SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        ContextUtility.SetScreenOrientation(this, mPrefs.getBoolean(PreferenceKey.LAUNCHER_ORIENTATION, false) ? 0 : 1);

		Theme.SetTheme(this, false);
        setContentView(R.layout.main);

        ActionBar actionBar = getActionBar();
        if (null != actionBar)
            actionBar.setDisplayHomeAsUpEnabled(true);

        InitQ3E(null); // gameType
        Q3E.q3ei.SetAppStoragePath(this);

        TabHost th = (TabHost) findViewById(R.id.tabhost);
        th.setup();
        th.addTab(th.newTabSpec("tab1").setIndicator(Q3ELang.tr(this, R.string.general)).setContent(R.id.launcher_tab1));
        th.addTab(th.newTabSpec("tab2").setIndicator(Q3ELang.tr(this, R.string.controls)).setContent(R.id.launcher_tab2));
        th.addTab(th.newTabSpec("tab3").setIndicator(Q3ELang.tr(this, R.string.graphics)).setContent(R.id.launcher_tab3));

        V.Setup();

		InitGameList();

		InitGameVersionList();

		SetupUI();

        updatehacktings();

		AfterCreated();
    }

	private void SetupUI()
	{
		final SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this);

		String gameType = mPrefs.getString(Q3EPreference.pref_harm_game, Q3EGameConstants.GAME_DOOM3);

		V.main_ad_layout.setVisibility(mPrefs.getBoolean(PreferenceKey.HIDE_AD_BAR, true) ? View.GONE : View.VISIBLE);

		SetGame(gameType);

		V.edt_cmdline.setText(mPrefs.getString(Q3E.q3ei.GetGameCommandPreferenceKey(), Q3EGameConstants.GAME_EXECUABLE));
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
		V.tab2_hide_joystick_center.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_hide_joystick_center, false));
		V.tab2_disable_mouse_button_motion.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_disable_mouse_button_motion, false));
		V.detectmouse.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.detectmouse.setChecked(mPrefs.getBoolean(Q3EPreference.pref_detectmouse, true));

		UpdateMouseManualMenu(!V.detectmouse.isChecked());

		SelectRadioGroup(V.rg_curpos, mPrefs.getInt(Q3EPreference.pref_mousepos, 3));
		int scrres = mPrefs.getInt(Q3EPreference.pref_scrres_scheme, Q3EGlobals.SCREEN_FULL);
		SelectRadioGroup(V.rg_scrres, scrres);
		V.rg_scrres.setOnCheckedChangeListener(m_groupCheckChangeListener);
		int scrresScheme = Utility.Step(mPrefs.getInt(Q3EPreference.pref_scrres_scale, 100), 10);
		V.res_scale.setProgress(scrresScheme);
		V.tv_scale_current.setText(V.res_scale.getProgress() + "%");
		V.res_scale.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
			{
				int newProgress = Utility.Step(progress, 10);
				if(newProgress != progress)
				{
					seekBar.setProgress(newProgress);
					return;
				}
				V.tv_scale_current.setText(progress + "%");
			}

			@Override
			public void onStartTrackingTouch(SeekBar seekBar)
			{

			}

			@Override
			public void onStopTrackingTouch(SeekBar seekBar)
			{
				mPrefs.edit().putInt(Q3EPreference.pref_scrres_scale, seekBar.getProgress()).commit();
				UpdateResolutionText();
			}
		});
		SelectRadioGroup(V.rg_msaa, mPrefs.getInt(Q3EPreference.pref_msaa, 0));
		V.rg_msaa.setOnCheckedChangeListener(m_groupCheckChangeListener);
		//k
		V.usedxt.setChecked(mPrefs.getBoolean(Q3EPreference.pref_usedxt, false));
		V.useetc1.setChecked(mPrefs.getBoolean(Q3EPreference.pref_useetc1, false));
		V.useetc1cache.setChecked(mPrefs.getBoolean(Q3EPreference.pref_useetc1cache, false));
		V.nolight.setChecked(mPrefs.getBoolean(Q3EPreference.pref_nolight, false));
		V.image_useetc2.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_image_useetc2, false));
		SelectRadioGroup(V.rg_color_bits, mPrefs.getInt(Q3EPreference.pref_harm_16bit, -1) + 1);
		V.rg_color_bits.setOnCheckedChangeListener(m_groupCheckChangeListener);
		SelectRadioGroup(V.r_harmclearvertexbuffer, mPrefs.getInt(Q3EPreference.pref_harm_r_harmclearvertexbuffer, 2));
		V.r_harmclearvertexbuffer.setOnCheckedChangeListener(m_groupCheckChangeListener);
		int index = Q3EUtils.parseInt_s(mPrefs.getString(Q3EPreference.pref_harm_r_lightingModel, "1"), 1) - 1;
		if(index < 0)
			index = V.rg_harm_r_lightingModel.getChildCount() - 1;
		SelectRadioGroup(V.rg_harm_r_lightingModel, index);
		V.rg_harm_r_lightingModel.setOnCheckedChangeListener(m_groupCheckChangeListener);
		SelectRadioGroup(V.rg_s_driver, "OpenSLES".equalsIgnoreCase(mPrefs.getString(Q3EPreference.pref_harm_s_driver, "AudioTrack")) ? 1 : 0);
		V.rg_s_driver.setOnCheckedChangeListener(m_groupCheckChangeListener);
		SelectRadioGroup(V.rg_harm_r_shadow, mPrefs.getBoolean(Q3EPreference.pref_harm_r_useShadowMapping, false) ? 1 : 0);
		V.rg_harm_r_shadow.setOnCheckedChangeListener(m_groupCheckChangeListener);
		SelectRadioGroup(V.rg_opengl, mPrefs.getInt(Q3EPreference.pref_harm_opengl, Q3EGLConstants.GetPreferOpenGLESVersion()) == Q3EGLConstants.OPENGLES30 ? 1 : 0);
		V.rg_opengl.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.launcher_tab2_enable_gyro.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_view_motion_control_gyro, false));
		int autoAspectRatio = mPrefs.getInt(Q3EPreference.pref_harm_r_autoAspectRatio, 1);
		if(autoAspectRatio > 0 && Q3E.q3ei.IsIdTech4())
			SetProp_temp("harm_r_autoAspectRatio", autoAspectRatio);
//		SelectRadioGroup(V.rg_r_autoAspectRatio, mPrefs.getInt(Q3EPreference.pref_harm_r_autoAspectRatio, 1));
		V.cb_r_autoAspectRatio.setChecked(autoAspectRatio > 0);
		V.cb_r_autoAspectRatio.setOnCheckedChangeListener(m_checkboxChangeListener);
		boolean skipIntro = mPrefs.getBoolean(Q3EPreference.pref_harm_skip_intro, false);
		V.skip_intro.setChecked(skipIntro);
		if (skipIntro && (Q3E.q3ei.IsIdTech4() || Q3E.q3ei.IsIdTech3()))
			SetCommand_temp("disconnect", false);
		boolean autoQuickLoad = mPrefs.getBoolean(Q3EPreference.pref_harm_auto_quick_load, false);
		V.auto_quick_load.setChecked(autoQuickLoad);
		if (autoQuickLoad && (Q3E.q3ei.IsIdTech4() || Q3E.q3ei.isRTCW || Q3E.q3ei.isRealRTCW))
			SetParam_temp("loadGame", "QuickSave");
		boolean multithreading = mPrefs.getBoolean(Q3EPreference.pref_harm_multithreading, true);
		V.multithreading.setChecked(multithreading);
//		V.rg_r_autoAspectRatio.setOnCheckedChangeListener(m_groupCheckChangeListener);
		int consoleHeightFrac = mPrefs.getInt(Q3EPreference.pref_harm_max_console_height_frac, 0);
		V.consoleHeightFracValue.setProgress(consoleHeightFrac);
		V.consoleHeightFracText.setText(GetConsoleHeightText(consoleHeightFrac));
		V.consoleHeightFracValue.setOnSeekBarChangeListener(m_seekListener);
		SelectRadioGroup(V.rg_depth_bits, Q3EPreference.DepthIndexByBits(mPrefs.getInt(Q3EPreference.pref_harm_depth_bit, Q3EGlobals.DEFAULT_DEPTH_BITS)));
		V.rg_depth_bits.setOnCheckedChangeListener(m_groupCheckChangeListener);
		boolean skipHitEffect = mPrefs.getBoolean(Q3EPreference.pref_harm_g_skipHitEffect, false);
//		SelectRadioGroup(V.rg_r_autoAspectRatio, mPrefs.getInt(Q3EPreference.pref_harm_r_autoAspectRatio, 1));
		V.cb_g_skipHitEffect.setChecked(skipHitEffect);
		V.cb_g_skipHitEffect.setOnCheckedChangeListener(m_checkboxChangeListener);
		boolean botEnableBuiltinAssets = mPrefs.getBoolean(Q3EPreference.pref_harm_g_botEnableBuiltinAssets, false);
		V.cb_g_botEnableBuiltinAssets.setChecked(botEnableBuiltinAssets);
		V.cb_g_botEnableBuiltinAssets.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.edt_cmdline.setOnEditorActionListener(new TextView.OnEditorActionListener() {
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
		V.launcher_tab1_edit_config.setOnClickListener(m_buttonClickListener);
		V.launcher_tab1_edit_cvar.setOnClickListener(m_buttonClickListener);
		V.launcher_tab1_command_record.setOnClickListener(m_buttonClickListener);
		V.launcher_tab1_create_shortcut.setOnClickListener(m_buttonClickListener);
		V.show_directory_helper.setOnClickListener(m_buttonClickListener);
		V.launcher_tab1_patch_resource.setOnClickListener(m_buttonClickListener);
		V.launcher_tab1_change_game.setOnClickListener(m_buttonClickListener);
		registerForContextMenu(V.launcher_tab1_open_menu);
		V.launcher_tab1_open_menu.setOnClickListener(m_buttonClickListener);
		V.launcher_tab1_edit_env.setOnClickListener(m_buttonClickListener);

		boolean userMod = mPrefs.getBoolean(Q3E.q3ei.GetEnableModPreferenceKey(), false);
		V.fs_game_user.setChecked(userMod);
		String game = mPrefs.getString(Q3E.q3ei.GetGameModPreferenceKey(), "");
		if (null == game)
			game = "";
		GameManager.GameProp prop = m_gameManager.ChangeGameMod(game, userMod);
		SelectRadioGroup(GetGameModRadioGroup(), prop.index);
		UpdateUserGame(userMod);
		V.edt_fs_game.setText(mPrefs.getString(Q3E.q3ei.GetGameUserModPreferenceKey(), ""));
		V.fs_game_user.setOnCheckedChangeListener(m_checkboxChangeListener);
		for(RadioGroup rg : groupRadios.values())
		{
			rg.setOnCheckedChangeListener(m_groupCheckChangeListener);
		}

		SelectGameVersion();
		for(RadioGroup rg : versionGroupRadios.values())
		{
			rg.setOnCheckedChangeListener(m_groupCheckChangeListener);
		}
		V.edt_fs_game.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(CharSequence s, int start, int before, int count)
			{
				if (V.fs_game_user.isChecked())
					SetGameModToCommand(s.toString());
			}

			public void beforeTextChanged(CharSequence s, int start, int count, int after)
			{
			}

			public void afterTextChanged(Editable s)
			{
				SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit();
				editor.putString(Q3E.q3ei.GetGameUserModPreferenceKey(), s.toString());
				if (V.fs_game_user.isChecked())
					editor.putString(Q3E.q3ei.GetGameModPreferenceKey(), s.toString());
				editor.commit();
			}
		});
		V.launcher_tab1_game_lib_button.setOnClickListener(m_buttonClickListener);
		V.launcher_tab1_game_mod_button.setOnClickListener(m_buttonClickListener);
		V.edt_harm_r_specularExponent.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_r_specularExponent, 3.0f));
		V.edt_harm_r_specularExponentBlinnPhong.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_r_specularExponentBlinnPhong, 12.0f));
		V.edt_harm_r_specularExponentPBR.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_r_specularExponentPBR, 5.0f));
		V.edt_harm_r_PBRNormalCorrection.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_r_PBRNormalCorrection, 0.25f));
		V.edt_harm_r_ambientLightingBrightness.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_r_ambientLightingBrightness, 1.0f));
		V.edt_harm_r_maxFps.setText(Q3EPreference.GetStringFromInt(mPrefs, Q3EPreference.pref_harm_r_maxFps, 0));

		V.use_custom_resolution.setChecked(mPrefs.getBoolean(Q3EPreference.pref_ratio_use_custom_resolution, false));
		V.use_custom_resolution.setOnCheckedChangeListener(m_checkboxChangeListener);
		Runnable customResChanged = new Runnable() {
			@Override
			public void run() {
				UpdateResolutionText();
			}
		};
		V.res_x.setText(mPrefs.getString(Q3EPreference.pref_resx, "" + Q3EGlobals.SCREEN_WIDTH));
		V.res_y.setText(mPrefs.getString(Q3EPreference.pref_resy, "" + Q3EGlobals.SCREEN_HEIGHT));
		V.res_x.addTextChangedListener(new SavePreferenceTextWatcher(Q3EPreference.pref_resx, "" + Q3EGlobals.SCREEN_WIDTH, customResChanged));
		V.res_y.addTextChangedListener(new SavePreferenceTextWatcher(Q3EPreference.pref_resy, "" + Q3EGlobals.SCREEN_HEIGHT, customResChanged));

		V.ratio_x.setText(mPrefs.getString(Q3EPreference.pref_ratiox, "" + Q3EGlobals.RATIO_WIDTH));
		V.ratio_y.setText(mPrefs.getString(Q3EPreference.pref_ratioy, "" + Q3EGlobals.RATIO_HEIGHT));
		V.ratio_x.addTextChangedListener(new SavePreferenceTextWatcher(Q3EPreference.pref_ratiox, "" + Q3EGlobals.RATIO_WIDTH, customResChanged));
		V.ratio_y.addTextChangedListener(new SavePreferenceTextWatcher(Q3EPreference.pref_ratioy, "" + Q3EGlobals.RATIO_HEIGHT, customResChanged));
		V.launcher_tab1_game_data_chooser_button.setOnClickListener(m_buttonClickListener);
		V.onscreen_button_setting.setOnClickListener(m_buttonClickListener);
		V.setup_onscreen_button_theme.setOnClickListener(m_buttonClickListener);
		V.setup_controller.setOnClickListener(m_buttonClickListener);
		UpdateResolutionByType(scrres);

		// SDL
		SetupUI_SDL();

		// OpenAL
		SetupUI_OpenAL();

		// Quake2
		SetupUI_Quake2();

		// DOOM 3 BFG
		SetupUI_Doom3BFG();

		// RealRTCW
		SetupUI_RealRTCW();

		// ETW
		SetupUI_ETW();

		// ZDOOM
		SetupUI_ZDOOM();

		// The Dark Mod
		SetupUI_TDM();

		// FTEQW
		SetupUI_FTEQW();

		// Xash3D
		SetupUI_Xash3D();

		// Source Engine
		SetupUI_Source();

		// UrT
		SetupUI_UrT();

		//DIII4A-specific
		SetupCommandTextWatcher(true);
		V.edt_harm_r_specularExponent.addTextChangedListener(new SaveFloatPreferenceTextWatcher("harm_r_specularExponent", Q3EPreference.pref_harm_r_specularExponent, 3.0f));
		V.edt_harm_r_specularExponentBlinnPhong.addTextChangedListener(new SaveFloatPreferenceTextWatcher("harm_r_specularExponentBlinnPhong", Q3EPreference.pref_harm_r_specularExponentBlinnPhong, 12.0f));
		V.edt_harm_r_specularExponentPBR.addTextChangedListener(new SaveFloatPreferenceTextWatcher("harm_r_specularExponentPBR", Q3EPreference.pref_harm_r_specularExponentPBR, 5.0f));
		V.edt_harm_r_PBRNormalCorrection.addTextChangedListener(new SaveFloatPreferenceTextWatcher("harm_r_PBRNormalCorrection", Q3EPreference.pref_harm_r_PBRNormalCorrection, 0.25f));
		V.edt_harm_r_ambientLightingBrightness.addTextChangedListener(new SaveFloatPreferenceTextWatcher("harm_r_ambientLightingBrightness", Q3EPreference.pref_harm_r_ambientLightingBrightness, 1.0f));
		V.edt_harm_r_maxFps.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(CharSequence s, int start, int before, int count)
			{
				if(Q3E.q3ei.IsIdTech4())
					SetProp("r_maxFps", s);
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
		V.image_useetc2.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.smoothjoy.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.launcher_tab2_joystick_unfixed.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.tab2_hide_joystick_center.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.tab2_disable_mouse_button_motion.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.launcher_tab2_joystick_visible.setOnItemSelectedListener(m_itemSelectedListener);
		V.edt_path.addTextChangedListener(new SavePreferenceTextWatcher(Q3EPreference.pref_datapath, default_gamedata, new Runnable() {
			@Override
			public void run() {
				Q3E.q3ei.datadir = V.edt_path.getText().toString();
			}
		}));
		V.edt_mouse.addTextChangedListener(new SavePreferenceTextWatcher(Q3EPreference.pref_eventdev, "/dev/input/event???"));
		V.rg_curpos.setOnCheckedChangeListener(m_groupCheckChangeListener);
		V.secfinglmb.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.mapvol.setOnCheckedChangeListener(m_checkboxChangeListener);
		UpdateMapVol(V.mapvol.isChecked());
		V.launcher_tab2_volume_up_map_config_keys.setOnItemSelectedListener(m_spinnerItemSelectedListener);
		V.launcher_tab2_volume_down_map_config_keys.setOnItemSelectedListener(m_spinnerItemSelectedListener);
		V.launcher_tab2_enable_gyro.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.launcher_tab2_gyro_x_axis_sens.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_view_motion_gyro_x_axis_sens, Q3EGlobals.GYROSCOPE_X_AXIS_SENS));
		V.launcher_tab2_gyro_y_axis_sens.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_view_motion_gyro_y_axis_sens, Q3EGlobals.GYROSCOPE_Y_AXIS_SENS));
		UpdateEnableGyro(V.launcher_tab2_enable_gyro.isChecked());
		V.launcher_tab2_gyro_x_axis_sens.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(CharSequence s, int start, int before, int count) {}
			public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
			public void afterTextChanged(Editable s) {
				String value = s.length() == 0 ? "" + Q3EGlobals.GYROSCOPE_X_AXIS_SENS : s.toString();
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putFloat(Q3EPreference.pref_harm_view_motion_gyro_x_axis_sens, Q3EUtils.parseFloat_s(value, Q3EGlobals.GYROSCOPE_Y_AXIS_SENS))
						.commit();
			}
		});
		V.launcher_tab2_gyro_y_axis_sens.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(CharSequence s, int start, int before, int count) {}
			public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
			public void afterTextChanged(Editable s) {
				String value = s.length() == 0 ? "" + Q3EGlobals.GYROSCOPE_Y_AXIS_SENS : s.toString();
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putFloat(Q3EPreference.pref_harm_view_motion_gyro_y_axis_sens, Q3EUtils.parseFloat_s(value, Q3EGlobals.GYROSCOPE_Y_AXIS_SENS))
						.commit();
			}
		});
		V.button_swipe_release_delay.setText(Q3EPreference.GetStringFromInt(mPrefs, Q3EPreference.BUTTON_SWIPE_RELEASE_DELAY, Q3EGlobals.BUTTON_SWIPE_RELEASE_DELAY_AUTO));
		V.button_swipe_release_delay.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(CharSequence s, int start, int before, int count) {}
			public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
			public void afterTextChanged(Editable s) {
				String value = s.length() == 0 ? "" + Q3EGlobals.BUTTON_SWIPE_RELEASE_DELAY_AUTO : s.toString();
				PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
						.putInt(Q3EPreference.BUTTON_SWIPE_RELEASE_DELAY, Q3EUtils.parseInt_s(value, Q3EGlobals.BUTTON_SWIPE_RELEASE_DELAY_AUTO))
						.commit();
			}
		});
		V.auto_quick_load.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.skip_intro.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.multithreading.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.find_dll.setOnCheckedChangeListener(m_checkboxChangeListener);
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
		V.readonly_command.setChecked(!readonlyCommand);
		SetupCommandLine(readonlyCommand);
		V.readonly_command.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.cb_stencilShadowTranslucent.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_r_stencilShadowTranslucent, false));
		V.cb_stencilShadowTranslucent.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.edt_harm_r_stencilShadowAlpha.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_r_stencilShadowAlpha, 1.0f));
		V.edt_harm_r_stencilShadowAlpha.addTextChangedListener(new SaveFloatPreferenceTextWatcher("harm_r_stencilShadowAlpha", Q3EPreference.pref_harm_r_stencilShadowAlpha, 1.0f));
		V.edt_harm_r_shadowMapAlpha.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_r_shadowMapAlpha, 1.0f));
		V.edt_harm_r_shadowMapAlpha.addTextChangedListener(new SaveFloatPreferenceTextWatcher("harm_r_shadowMapAlpha", Q3EPreference.pref_harm_r_shadowMapAlpha, 1.0f));

		V.cb_stencilShadowSoft.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_r_stencilShadowSoft, false));
		V.cb_stencilShadowSoft.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.cb_stencilShadowCombine.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_r_stencilShadowCombine, false));
		V.cb_stencilShadowCombine.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.cb_perforatedShadow.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_r_shadowMapPerforatedShadow, false));
		V.cb_perforatedShadow.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.cb_useHighPrecision.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_r_useHighPrecision, false));
		V.cb_useHighPrecision.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.cb_renderToolsMultithread.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_r_renderToolsMultithread, true));
		V.cb_renderToolsMultithread.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.cb_r_occlusionCulling.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_r_occlusionCulling, false));
		V.cb_r_occlusionCulling.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.cb_shadowMapCombine.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_r_shadowMapCombine, true));
		V.cb_shadowMapCombine.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.cb_r_globalIllumination.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_r_globalIllumination, false));
		V.cb_r_globalIllumination.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.edt_r_globalIlluminationBrightness.setText(Q3EPreference.GetStringFromFloat(mPrefs, Q3EPreference.pref_harm_r_globalIlluminationBrightness, 0.5f));
		V.edt_r_globalIlluminationBrightness.addTextChangedListener(new SaveFloatPreferenceTextWatcher("harm_r_globalIlluminationBrightness", Q3EPreference.pref_harm_r_globalIlluminationBrightness, 0.3f));
		V.spinner_r_renderMode.setSelection(mPrefs.getInt(Q3EPreference.pref_harm_r_renderMode, 0));
		V.spinner_r_renderMode.setOnItemSelectedListener(m_itemSelectedListener);

		V.cb_gui_useD3BFGFont.setChecked(mPrefs.getBoolean(Q3EPreference.pref_harm_gui_useD3BFGFont, false));
		V.cb_gui_useD3BFGFont.setOnCheckedChangeListener(m_checkboxChangeListener);

		boolean collapseMods = mPrefs.getBoolean(PreferenceKey.COLLAPSE_MODS, false);
		V.collapse_mods.setChecked(collapseMods);
		CollapseMods(collapseMods);
		V.collapse_mods.setOnCheckedChangeListener(m_checkboxChangeListener);

		SetupTempCommandLine(false);
		V.editable_temp_command.setOnCheckedChangeListener(m_checkboxChangeListener);
		V.edt_cmdline_temp.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(CharSequence s, int start, int before, int count)
			{
				if (V.edt_cmdline_temp.isInputMethodTarget())
				{
					Q3E.q3ei.start_temporary_extra_command = GetTempCmdText();
					UpdateTempCommand();
				}
			}

			public void beforeTextChanged(CharSequence s, int start, int count, int after) { }

			public void afterTextChanged(Editable s) { }
		});
	}

	private void SetupUI_SDL()
	{
		String str = PreferenceManager.getDefaultSharedPreferences(this).getString(Q3EPreference.pref_harm_sdl_audio_driver, Q3EGameConstants.SDL_AUDIO_DRIVER[0]);
		int index = 0;
		if(null != str)
		{
			index = Utility.ArrayIndexOf(Q3EGameConstants.SDL_AUDIO_DRIVER, str);
			if(index < 0)
				index = 0;
		}
		SelectRadioGroup(V.sdl_audio_driver, index);
		V.sdl_audio_driver.setOnCheckedChangeListener(m_groupCheckChangeListener);
	}

	private void SetupUI_OpenAL()
	{
		String str = PreferenceManager.getDefaultSharedPreferences(this).getString(Q3EPreference.pref_harm_openal_driver, Q3EGameConstants.OPENAL_DRIVER[0]);
		int index = 0;
		if(null != str)
		{
			index = Utility.ArrayIndexOf(Q3EGameConstants.OPENAL_DRIVER, str);
			if(index < 0)
				index = 0;
		}
		SelectRadioGroup(V.openal_driver, index);
		V.openal_driver.setOnCheckedChangeListener(m_groupCheckChangeListener);
	}

	private void SetupUI_Quake2()
	{
		String str = GetProp("vid_renderer");
		int index = 0;
		if(null != str)
		{
			index = Utility.ArrayIndexOf(Q3EGameConstants.QUAKE2_RENDERER_BACKENDS, str);
			if(index < 0)
				index = 0;
		}
		SelectRadioGroup(V.yquake2_vid_renderer, index);
		V.yquake2_vid_renderer.setOnCheckedChangeListener(m_groupCheckChangeListener);
	}

	private void SetupUI_Doom3BFG()
	{
		int index = 0;
		String str = GetProp("harm_image_useCompression");
		if (str != null)
		{
			index = Q3EUtils.parseInt_s(str, 0);
		}
		SelectRadioGroup(V.doom3bfg_useCompression, index);
		V.doom3bfg_useCompression.setOnCheckedChangeListener(m_groupCheckChangeListener);

		V.doom3bfg_useCompressionCache.setChecked(getProp("harm_image_useCompressionCache", false));
		V.doom3bfg_useCompressionCache.setOnCheckedChangeListener(m_checkboxChangeListener);

		V.doom3bfg_useMediumPrecision.setChecked(getProp("harm_r_useMediumPrecision", false));
		V.doom3bfg_useMediumPrecision.setOnCheckedChangeListener(m_checkboxChangeListener);
	}

	private void SetupUI_TDM()
	{
		V.tdm_useMediumPrecision.setChecked(getProp("harm_r_useMediumPrecision", false));
		V.tdm_useMediumPrecision.setOnCheckedChangeListener(m_checkboxChangeListener);
	}

	private void SetupUI_RealRTCW()
	{
		int index = 0;
		String str = GetProp("cg_shadows");
		if (str != null)
		{
			index = Q3EUtils.parseInt_s(str, 1);
		}
		SelectRadioGroup(V.realrtcw_shadows, index);
		V.realrtcw_shadows.setOnCheckedChangeListener(m_groupCheckChangeListener);

		V.realrtcw_sv_cheats.setChecked(getProp("harm_sv_cheats", false));
		V.realrtcw_sv_cheats.setOnCheckedChangeListener(m_checkboxChangeListener);

		V.realrtcw_stencilShadowPersonal.setChecked(getProp("harm_r_stencilShadowPersonal", true));
		V.realrtcw_stencilShadowPersonal.setOnCheckedChangeListener(m_checkboxChangeListener);
	}

	private void SetupUI_ETW()
	{
		int index;
		String str;

		V.etw_omnibot_enable.setChecked(getProp("omnibot_enable", false));
		V.etw_omnibot_enable.setOnCheckedChangeListener(m_checkboxChangeListener);

		index = 0;
		str = GetProp("cg_shadows");
		if (str != null)
		{
			index = Q3EUtils.parseInt_s(str, 1);
		}
		SelectRadioGroup(V.etw_shadows, index);
		V.etw_shadows.setOnCheckedChangeListener(m_groupCheckChangeListener);

		V.etw_stencilShadowPersonal.setChecked(getProp("harm_r_stencilShadowPersonal", true));
		V.etw_stencilShadowPersonal.setOnCheckedChangeListener(m_checkboxChangeListener);

		V.etw_ui_disableAndroidMacro.setChecked(getProp("harm_ui_disableAndroidMacro", false));
		V.etw_ui_disableAndroidMacro.setOnCheckedChangeListener(m_checkboxChangeListener);
	}

	private void SetupUI_ZDOOM()
	{
		int index;
		String str;

		SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this);

		boolean load = mPrefs.getBoolean(Q3EPreference.pref_harm_zdoom_load_lights_pk3, true);
		V.zdoom_load_lights_pk3.setChecked(load);
		if (load && (Q3E.q3ei.isDOOM))
			AddParam_temp("file", "lights.pk3");
		V.zdoom_load_lights_pk3.setOnCheckedChangeListener(m_checkboxChangeListener);

		load = mPrefs.getBoolean(Q3EPreference.pref_harm_zdoom_load_brightmaps_pk3, true);
		V.zdoom_load_brightmaps_pk3.setChecked(load);
		if (load && (Q3E.q3ei.isDOOM))
			AddParam_temp("file", "brightmaps.pk3");
		V.zdoom_load_brightmaps_pk3.setOnCheckedChangeListener(m_checkboxChangeListener);

		/*List<String> file = GetParamList("file");
		V.zdoom_load_lights_pk3.setChecked(null != file && file.contains("lights.pk3"));*/

		V.zdoom_choose_extras_file.setOnClickListener(m_buttonClickListener);

		index = 2;
		str = GetPropPrefix(KidTechCommand.ARG_PREFIX_ALL, "vid_preferbackend");
		if (str != null)
		{
			index = Q3EUtils.parseInt_s(str, 2);
		}
		SelectRadioGroup(V.zdoom_vid_preferbackend, index);
		V.zdoom_vid_preferbackend.setOnCheckedChangeListener(m_groupCheckChangeListener);

		index = 0;
		str = GetPropPrefix(KidTechCommand.ARG_PREFIX_ALL, "harm_gl_es");
		if (str != null)
		{
			index = Q3EUtils.parseInt_s(str, 0);
			if(index > 0)
				index--;
		}
		SelectRadioGroup(V.zdoom_gl_es, index);
		V.zdoom_gl_es.setOnCheckedChangeListener(m_groupCheckChangeListener);

		index = 0;
		str = GetPropPrefix(KidTechCommand.ARG_PREFIX_ALL, "harm_gl_version");
		if (str != null)
		{
			index = Utility.ArrayIndexOf(Q3EGameConstants.ZDOOM_GL_VERSIONS, str);
			if(index < 0)
				index = 0;
		}
		SelectRadioGroup(V.zdoom_gl_version, index);
		V.zdoom_gl_version.setOnCheckedChangeListener(m_groupCheckChangeListener);
	}

	private void SetupUI_FTEQW()
	{
		int index;
		String str;

		SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(this);

		index = 1;
		str = GetPropPrefix(KidTechCommand.ARG_PREFIX_ALL, "vid_renderer");
		if (str != null)
		{
			index = "vk".equalsIgnoreCase(str) ? 0 : 1;
		}
		SelectRadioGroup(V.fteqw_vid_renderer, index);
		V.fteqw_vid_renderer.setOnCheckedChangeListener(m_groupCheckChangeListener);
	}

	private void SetupUI_Xash3D()
	{
		String str = GetParam("ref");
		int index = 0;
		if(null != str)
		{
			index = Utility.ArrayIndexOf(Q3EGameConstants.XASH3D_REFS, str);
			if(index < 0)
				index = 0;
		}
		SelectRadioGroup(V.xash3d_ref, index);
		V.xash3d_ref.setOnCheckedChangeListener(m_groupCheckChangeListener);

		str = GetParam("sv_cl");
		index = 0;
		if(null != str)
		{
			index = Utility.ArrayIndexOf(Q3EGameConstants.XASH3D_SV_CLS, str);
			if(index < 0)
				index = 0;
		}
		SelectRadioGroup(V.xash3d_sv_cl, index);
		V.xash3d_sv_cl.setOnCheckedChangeListener(m_groupCheckChangeListener);
	}

	private void SetupUI_Source()
	{
		String str;
		int index;

		str = GetParam("sv_cl");
		index = 0;
		if(null != str)
		{
			index = Utility.ArrayIndexOf(Q3EGameConstants.SOURCE_ENGINE_SV_CLS, str);
			if(index < 0)
				index = 0;
		}
		SelectRadioGroup(V.source_sv_cl, index);
		V.source_sv_cl.setOnCheckedChangeListener(m_groupCheckChangeListener);
	}

	private void SetupUI_UrT()
	{
		String str = GetProp("harm_bot_autoAdd");
		if(null != str)
		{
			V.urt_bot_autoAdd.setText(str);
		}
		V.urt_bot_autoAdd.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(CharSequence s, int start, int before, int count)
			{
				if(Q3E.q3ei.isUrT)
					SetProp("harm_bot_autoAdd", s);
			}

			public void beforeTextChanged(CharSequence s, int start, int count, int after) { }

			public void afterTextChanged(Editable s)
			{
				if(Q3E.q3ei.isUrT)
				{
					String value = s.length() == 0 ? "0" : s.toString();
					SetProp("harm_bot_autoAdd", value);
				}
			}
		});

		str = GetProp("harm_bot_level");
		if(null != str)
		{
			V.urt_bot_level.setText(str);
		}
		V.urt_bot_level.addTextChangedListener(new TextWatcher() {
			public void onTextChanged(CharSequence s, int start, int before, int count)
			{
				if(Q3E.q3ei.isUrT)
					SetProp("harm_bot_level", s);
			}

			public void beforeTextChanged(CharSequence s, int start, int count, int after) { }

			public void afterTextChanged(Editable s)
			{
				if(Q3E.q3ei.isUrT)
				{
					String value = s.length() == 0 ? "0" : s.toString();
					SetProp("harm_bot_level", value);
				}
			}
		});
	}

	private void LoadAds()
	{
		//Q3EAd.LoadAds(this);
		if(null != m_adFunc)
			return;
		m_adFunc = new GameAd(this);
		m_adFunc.SetCallback(new Runnable() {
			@Override
			public void run()
			{
				ChangeGame((String)m_adFunc.GetResult());
			}
		});
		Bundle data = new Bundle();
		data.putString("game", Q3E.q3ei.game);
		m_adFunc.Start(data);
	}

	private void AfterCreated()
	{
		try
		{
			LoadAds();

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
						KLog.I("Launcher initial game: " + intentGame);
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
		ChooseEditOnscreenType(new Function() {
			@Override
			public Object Invoke(Object...args)
			{
				int which = (Integer)args[0];
				Intent intent = new Intent(GameLauncher.this, Q3EUiConfig.class);
				if(which > 0)
					intent.putExtra("game", Q3E.q3ei.game);
				startActivity(intent);
				return null;
			}
		});
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
		SetCmdText(Q3E.q3ei.GetGameCommandEngine(GetCmdText()).SetProp(name, val).toString());
        //SetCmdText(KidTech4Command.SetProp(GetCmdText(), name, val));
        UnlockCmdUpdate();
    }

    private String GetProp(String name)
    {
		return Q3E.q3ei.GetGameCommandEngine(GetCmdText()).Prop(name);
		// return KidTech4Command.GetProp(GetCmdText(), name);
    }

    private void RemoveProp(String name)
    {
		if(!LockCmdUpdate())
			return;
		String orig = GetCmdText();
		String str = Q3E.q3ei.GetGameCommandEngine(orig).RemoveProp(name).toString();
        if (!orig.equals(str))
            SetCmdText(str);
        UnlockCmdUpdate();
    }

    private boolean IsProp(String name)
    {
		return Q3E.q3ei.GetGameCommandEngine(GetCmdText()).IsProp(name);
        // return KidTech4Command.IsProp(GetCmdText(), name);
    }

	private void SetPropPrefix(String prefix, String name, Object val)
	{
		if(!LockCmdUpdate())
			return;
		SetCmdText(new KidTechCommand(prefix, GetCmdText()).SetProp(name, val).toString());
		UnlockCmdUpdate();
	}

	private String GetPropPrefix(String prefix, String name)
	{
		return new KidTechCommand(prefix, GetCmdText()).Prop(name);
		// return KidTech4Command.GetProp(GetCmdText(), name);
	}

	private void RemovePropPrefix(String prefix, String name)
	{
		if(!LockCmdUpdate())
			return;
		String orig = GetCmdText();
		String str = new KidTechCommand(prefix, orig).RemoveProp(name).toString();
		if (!orig.equals(str))
			SetCmdText(str);
		UnlockCmdUpdate();
	}

    private void EditFile()
    {
        if (null == m_editConfigFileFunc)
            m_editConfigFileFunc = new EditConfigFileFunc(this, CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_EDIT_CONFIG_FILE);

        Bundle bundle = new Bundle();
        String game = GetGameModFromCommand();
        if (game == null || game.isEmpty())
            game = Q3E.q3ei.game_base;
		else
		{
			String str = m_gameManager.GetGameFileOfMod(game);
			if(null != str)
				game = str;
		}
		String path = KStr.AppendPath(V.edt_path.getText().toString(), Q3E.q3ei.subdatadir);
        bundle.putString("game", game);
		bundle.putString("path", path);
		bundle.putString("base", Q3E.q3ei.game_base);
		bundle.putString("home", Q3E.q3ei.GetGameHomeDirectoryPath());
		bundle.putStringArray("files", Q3E.q3ei.GetGameConfigFiles());
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
        V.main_menu_game.setTitle(Q3E.q3ei.game_name);
		SubMenu subMenu = V.main_menu_game.getSubMenu();
		for(int i = 0; i < subMenu.size(); i++)
		{
			MenuItem item = subMenu.getItem(i);
			Drawable icon = item.getIcon();
			if(icon instanceof BitmapDrawable)
			{
				Bitmap bitmap = ((BitmapDrawable) icon).getBitmap();
				Bitmap scaledBitmap = Bitmap.createScaledBitmap(bitmap, 32, 32, true);
				item.setIcon(new BitmapDrawable(getResources(), scaledBitmap));
			}
		}

		boolean res = super.onCreateOptionsMenu(menu);

		// KARIN_NEW_GAME_BOOKMARK: add menu id to game mapping
		menuGames.clear();
		menuGames.put(R.id.main_menu_game_doom3, Q3EGameConstants.GAME_DOOM3);
		menuGames.put(R.id.main_menu_game_quake4, Q3EGameConstants.GAME_QUAKE4);
		menuGames.put(R.id.main_menu_game_prey, Q3EGameConstants.GAME_PREY);
		menuGames.put(R.id.main_menu_game_quake1, Q3EGameConstants.GAME_QUAKE1);
		menuGames.put(R.id.main_menu_game_quake2, Q3EGameConstants.GAME_QUAKE2);
		menuGames.put(R.id.main_menu_game_quake3, Q3EGameConstants.GAME_QUAKE3);
		menuGames.put(R.id.main_menu_game_rtcw, Q3EGameConstants.GAME_RTCW);
		menuGames.put(R.id.main_menu_game_tdm, Q3EGameConstants.GAME_TDM);
		menuGames.put(R.id.main_menu_game_doom3bfg, Q3EGameConstants.GAME_DOOM3BFG);
		menuGames.put(R.id.main_menu_game_doom, Q3EGameConstants.GAME_ZDOOM);
		menuGames.put(R.id.main_menu_game_etw, Q3EGameConstants.GAME_ETW);
		menuGames.put(R.id.main_menu_game_realrtcw, Q3EGameConstants.GAME_REALRTCW);
		menuGames.put(R.id.main_menu_game_fteqw, Q3EGameConstants.GAME_FTEQW);
		menuGames.put(R.id.main_menu_game_ja, Q3EGameConstants.GAME_JA);
		menuGames.put(R.id.main_menu_game_jo, Q3EGameConstants.GAME_JO);
		menuGames.put(R.id.main_menu_game_samtfe, Q3EGameConstants.GAME_SAMTFE);
		menuGames.put(R.id.main_menu_game_samtse, Q3EGameConstants.GAME_SAMTSE);
		menuGames.put(R.id.main_menu_game_xash3d, Q3EGameConstants.GAME_XASH3D);
		menuGames.put(R.id.main_menu_game_source, Q3EGameConstants.GAME_SOURCE);
		menuGames.put(R.id.main_menu_game_urt, Q3EGameConstants.GAME_URT);
		menuGames.put(R.id.main_menu_game_mohaa, Q3EGameConstants.GAME_MOHAA);
		menuGames.put(R.id.main_menu_game_wolf3d, Q3EGameConstants.GAME_WOLF3D);

		return res;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
		boolean res = SelectMenuItem(item);
		if(res)
			return true;
		return super.onOptionsItemSelected(item);
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo)
	{
		MenuInflater inflater = new MenuInflater(this);
		inflater.inflate(R.menu.activity_main, menu);
		super.onCreateContextMenu(menu, v, menuInfo);
	}

	@Override
	public boolean onContextItemSelected(MenuItem item) {
		boolean res = SelectMenuItem(item);
		if(res)
			return true;
		return super.onContextItemSelected(item);
	}

	public boolean SelectMenuItem(MenuItem item)
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
			OpenResourceFileDialog(true);
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
		else if (itemId == R.id.main_menu_advance)
		{
			OpenAdvanceDialog();
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
		else if (itemId == R.id.main_menu_shortcut)
		{
			OpenShortcutCreator();
			return true;
		}
		else if (itemId == R.id.main_menu_change_game)
		{
			OpenGameList();
			return true;
		}
		else if (itemId == R.id.main_menu_download_testing)
		{
			OpenDownloadTestingDialog();
			return true;
		}
		else if (itemId == android.R.id.home)
		{
			ChangeGame(null);
			return true;
		}
		// Change game
		else if(menuGames.containsKey(itemId))
		{
			ChangeGame(menuGames.get(itemId));
			return true;
		}
		return false;
	}

    private void OpenChanges()
    {
		ChangelogView changelogView = new ChangelogView(this, ChangeLog.GetChangeLogs());
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle(R.string.changes);
		builder.setView(changelogView);
		builder.setPositiveButton(R.string.ok, new AlertDialog.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which) {
				dialog.dismiss();
			}
		});
		builder.setNeutralButton(R.string.expand_all, null);
		builder.setNegativeButton(R.string.collapse_all, null);

		AlertDialog dialog = builder.create();
		dialog.create();
		dialog.setOnShowListener(new DialogInterface.OnShowListener() {
			@Override
			public void onShow(DialogInterface d)
			{
				dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View view) {
						changelogView.ExpandAll();
					}
				});

				dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View view) {
						changelogView.CollapseAll();
					}
				});
			}
		});

		dialog.show();
    }

    private void OpenAbout()
    {
    	Object[] args = { null };
		AlertDialog dialog = ContextUtility.OpenMessageDialog(this, Q3ELang.tr(this, R.string.about) + " " + Constants.CONST_APP_NAME + "(" + Constants.CONST_CODE + ")", TextHelper.GetAboutText(this), new Runnable() {
			@Override
			public void run()
			{
				AlertDialog.Builder builder = (AlertDialog.Builder)args[0];
				builder.setNeutralButton(R.string.license, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which)
					{
						if (null == m_openSourceLicenseFunc)
							m_openSourceLicenseFunc = new OpenSourceLicenseFunc(GameLauncher.this);
						m_openSourceLicenseFunc.Start(new Bundle());
					}
				});
				builder.setNegativeButton(R.string.extract_source, new DialogInterface.OnClickListener() {
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

	private void OpenRuntimeErrorLog()
	{
		String path = Q3E.q3ei.GetGameDataDirectoryPath("stderr.txt");
		String text = Q3EUtils.file_get_contents(path);

		AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(this, Q3ELang.tr(this, R.string.last_runtime_log) + ": stderr.txt", text);
		builder.setNeutralButton("stdout.txt", new AlertDialog.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which)
			{
				OpenRuntimeLog();
				dialog.dismiss();
			}
		});
		builder.create().show();
	}

    private void OpenRuntimeLog()
    {
		String path = Q3E.q3ei.GetGameDataDirectoryPath("stdout.txt");
		String text = Q3EUtils.file_get_contents(path);

		AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(this, Q3ELang.tr(this, R.string.last_runtime_log), text);
		builder.setNeutralButton("stderr.txt", new AlertDialog.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which)
			{
				OpenRuntimeErrorLog();
				dialog.dismiss();
			}
		});
		builder.create().show();

		//Toast.makeText(this, Q3ELang.tr(this, R.string.file_can_not_access) + path, Toast.LENGTH_LONG).show();
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
                    list = new ArrayList<>();
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

	private void SetupExtrasFiles(String extrasFiles)
	{
		if(":".equals(extrasFiles))
		{
			RemoveParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "file");
			RemoveParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "deh");
			RemoveParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "bex");
		}
		else
		{
			String files = extrasFiles.substring(1);
			String[] split = files.split(ChooseExtrasFileFunc.FILE_SEP);
			List<String> file = new ArrayList<>();
			List<String> deh = new ArrayList<>();
			List<String> bex = new ArrayList<>();

			for (String s : split)
			{
				if(s.toLowerCase().endsWith(".deh"))
					deh.add(s);
				else if(s.toLowerCase().endsWith(".bex"))
					bex.add(s);
				else
					file.add(s);
			}

			// if(V.zdoom_load_lights_pk3.isChecked() && !file.contains("lights.pk3")) file.add("light3.pk3");

			RemoveParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "file");
			RemoveParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "deh");
			RemoveParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "bex");

			if(!file.isEmpty())
				SetParamList("file", file);
			if(!deh.isEmpty())
				SetParamList("deh", deh);
			if(!bex.isEmpty())
				SetParamList("bex", bex);
		}
	}

	private void OpenExtrasFileChooser()
	{
		SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
		String preferenceKey = Q3E.q3ei.GetGameModPreferenceKey();

		if (null == m_chooseExtrasFileFunc)
			m_chooseExtrasFileFunc = new ChooseExtrasFileFunc(this, CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_EXTRAS_FILE);

		m_chooseExtrasFileFunc.SetCallback(new Runnable() {
			@Override
			public void run()
			{
				String extrasFiles = m_chooseExtrasFileFunc.GetResult();
				SetupExtrasFiles(extrasFiles);
			}
		});

		Bundle bundle = new Bundle();
		String path = KStr.AppendPath(preference.getString(Q3EPreference.pref_datapath, default_gamedata), Q3E.q3ei.subdatadir, Q3E.q3ei.GetGameModSubDirectory());
		bundle.putString("mod", preference.getString(preferenceKey, ""));
		bundle.putString("path", path);
		if(Q3E.q3ei.isDOOM)
			bundle.putString("file", GetParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "file") + " " + GetParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "deh") + " " + GetParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "bex"));
		m_chooseExtrasFileFunc.Start(bundle);
	}

    private void WritePreferences()
    {
        SharedPreferences.Editor mEdtr = PreferenceManager.getDefaultSharedPreferences(this).edit();
        mEdtr.putString(Q3E.q3ei.GetGameCommandPreferenceKey(), GetCmdText());
        mEdtr.putString(Q3EPreference.pref_eventdev, V.edt_mouse.getText().toString());
        mEdtr.putString(Q3EPreference.pref_datapath, V.edt_path.getText().toString());
        mEdtr.putBoolean(Q3EPreference.pref_hideonscr, V.hideonscr.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_harm_using_mouse, V.using_mouse.isChecked());
        //k mEdtr.putBoolean(Q3EUtils.pref_32bit, true);
        int index = GetRadioGroupSelectIndex(V.rg_color_bits) - 1;
        mEdtr.putBoolean(Q3EPreference.pref_32bit, index == -1);
        mEdtr.putInt(Q3EPreference.pref_harm_16bit, index);
        mEdtr.putInt(Q3EPreference.pref_harm_r_harmclearvertexbuffer, GetRadioGroupSelectIndex(V.r_harmclearvertexbuffer));
        mEdtr.putString(Q3EPreference.pref_harm_r_lightingModel, "" + ((GetRadioGroupSelectIndex(V.rg_harm_r_lightingModel) + 1) % V.rg_harm_r_lightingModel.getChildCount()));
        mEdtr.putFloat(Q3EPreference.pref_harm_r_specularExponent, Q3EUtils.parseFloat_s(V.edt_harm_r_specularExponent.getText().toString(), 3.0f));
		mEdtr.putFloat(Q3EPreference.pref_harm_r_specularExponentBlinnPhong, Q3EUtils.parseFloat_s(V.edt_harm_r_specularExponentBlinnPhong.getText().toString(), 12.0f));
		mEdtr.putFloat(Q3EPreference.pref_harm_r_specularExponentPBR, Q3EUtils.parseFloat_s(V.edt_harm_r_specularExponentPBR.getText().toString(), 5.0f));
		mEdtr.putFloat(Q3EPreference.pref_harm_r_PBRNormalCorrection, Q3EUtils.parseFloat_s(V.edt_harm_r_PBRNormalCorrection.getText().toString(), 0.25f));
		mEdtr.putFloat(Q3EPreference.pref_harm_r_ambientLightingBrightness, Q3EUtils.parseFloat_s(V.edt_harm_r_ambientLightingBrightness.getText().toString(), 1.0f));
        mEdtr.putString(Q3EPreference.pref_harm_s_driver, GetRadioGroupSelectIndex(V.rg_s_driver) == 1 ? "OpenSLES" : "AudioTrack");
		mEdtr.putInt(Q3EPreference.pref_harm_r_maxFps, Q3EUtils.parseInt_s(V.edt_harm_r_maxFps.getText().toString(), 0));
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_useShadowMapping, GetRadioGroupSelectIndex(V.rg_harm_r_shadow) == 1);
		mEdtr.putInt(Q3EPreference.pref_harm_opengl, GetRadioGroupSelectIndex(V.rg_opengl) == 1 ? Q3EGLConstants.OPENGLES30 : Q3EGLConstants.OPENGLES20);
		mEdtr.putInt(Q3EPreference.pref_harm_depth_bit, Q3EPreference.DepthBitsByIndex(GetRadioGroupSelectIndex(V.rg_depth_bits)));

        mEdtr.putBoolean(Q3EPreference.pref_mapvol, V.mapvol.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_analog, V.smoothjoy.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_2fingerlmb, V.secfinglmb.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_detectmouse, V.detectmouse.isChecked());
        mEdtr.putInt(Q3EPreference.pref_mousepos, GetRadioGroupSelectIndex(V.rg_curpos));
		mEdtr.putInt(Q3EPreference.pref_scrres_scheme, GetRadioGroupSelectIndex(V.rg_scrres));
        mEdtr.putInt(Q3EPreference.pref_msaa, GetRadioGroupSelectIndex(V.rg_msaa));
        mEdtr.putString(Q3EPreference.pref_resx, V.res_x.getText().toString());
        mEdtr.putString(Q3EPreference.pref_resy, V.res_y.getText().toString());
		mEdtr.putString(Q3EPreference.pref_ratiox, V.ratio_x.getText().toString());
		mEdtr.putString(Q3EPreference.pref_ratioy, V.ratio_y.getText().toString());
		mEdtr.putBoolean(Q3EPreference.pref_ratio_use_custom_resolution, V.use_custom_resolution.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_useetc1cache, V.useetc1cache.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_useetc1, V.useetc1.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_usedxt, V.usedxt.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_nolight, V.nolight.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_image_useetc2, V.image_useetc2.isChecked());
        mEdtr.putBoolean(Q3E.q3ei.GetEnableModPreferenceKey(), V.fs_game_user.isChecked());
        mEdtr.putString(Q3EPreference.pref_harm_game, Q3E.q3ei.game);
        mEdtr.putBoolean(Q3EPreference.pref_harm_view_motion_control_gyro, V.launcher_tab2_enable_gyro.isChecked());
        mEdtr.putFloat(Q3EPreference.pref_harm_view_motion_gyro_x_axis_sens, Q3EUtils.parseFloat_s(V.launcher_tab2_gyro_x_axis_sens.getText().toString(), Q3EGlobals.GYROSCOPE_X_AXIS_SENS));
        mEdtr.putFloat(Q3EPreference.pref_harm_view_motion_gyro_y_axis_sens, Q3EUtils.parseFloat_s(V.launcher_tab2_gyro_y_axis_sens.getText().toString(), Q3EGlobals.GYROSCOPE_Y_AXIS_SENS));
        mEdtr.putBoolean(Q3EPreference.pref_harm_auto_quick_load, V.auto_quick_load.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_skip_intro, V.skip_intro.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_harm_multithreading, V.multithreading.isChecked());
        mEdtr.putBoolean(Q3EPreference.pref_harm_joystick_unfixed, V.launcher_tab2_joystick_unfixed.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_hide_joystick_center, V.tab2_hide_joystick_center.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_disable_mouse_button_motion, V.tab2_disable_mouse_button_motion.isChecked());
		mEdtr.putInt(Q3EPreference.pref_harm_joystick_visible, getResources().getIntArray(R.array.joystick_visible_mode_values)[V.launcher_tab2_joystick_visible.getSelectedItemPosition()]);
        mEdtr.putBoolean(Q3EPreference.pref_harm_find_dll, V.find_dll.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_s_useOpenAL, V.cb_s_useOpenAL.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_s_useEAXReverb, V.cb_s_useEAXReverb.isChecked());
		mEdtr.putBoolean(PreferenceKey.READONLY_COMMAND, !V.readonly_command.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_stencilShadowTranslucent, V.cb_stencilShadowTranslucent.isChecked());
		mEdtr.putFloat(Q3EPreference.pref_harm_r_stencilShadowAlpha, Q3EUtils.parseFloat_s(V.edt_harm_r_stencilShadowAlpha.getText().toString(), 1.0f));
		mEdtr.putFloat(Q3EPreference.pref_harm_r_shadowMapAlpha, Q3EUtils.parseFloat_s(V.edt_harm_r_shadowMapAlpha.getText().toString(), 1.0f));
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_stencilShadowSoft, V.cb_stencilShadowSoft.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_stencilShadowCombine, V.cb_stencilShadowCombine.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_shadowMapPerforatedShadow, V.cb_perforatedShadow.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_useHighPrecision, V.cb_useHighPrecision.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_renderToolsMultithread, V.cb_renderToolsMultithread.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_occlusionCulling, V.cb_r_occlusionCulling.isChecked());
//		mEdtr.putInt(Q3EPreference.pref_harm_r_autoAspectRatio, GetRadioGroupSelectIndex(V.rg_r_autoAspectRatio));
		mEdtr.putInt(Q3EPreference.pref_harm_r_autoAspectRatio, V.cb_r_autoAspectRatio.isChecked() ? 1 : 0);
		mEdtr.putBoolean(PreferenceKey.COLLAPSE_MODS, V.collapse_mods.isChecked());

		mEdtr.putBoolean(Q3EPreference.pref_harm_zdoom_load_lights_pk3, V.zdoom_load_lights_pk3.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_zdoom_load_brightmaps_pk3, V.zdoom_load_brightmaps_pk3.isChecked());
		mEdtr.putInt(Q3EPreference.pref_harm_max_console_height_frac, V.consoleHeightFracValue.getProgress());
		mEdtr.putString(Q3E.q3ei.GetGameUserModPreferenceKey(), V.edt_fs_game.getText().toString());
		mEdtr.putBoolean(Q3EPreference.pref_harm_gui_useD3BFGFont, V.cb_gui_useD3BFGFont.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_shadowMapCombine, V.cb_shadowMapCombine.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_g_skipHitEffect, V.cb_g_skipHitEffect.isChecked());
		mEdtr.putBoolean(Q3EPreference.pref_harm_r_globalIllumination, V.cb_r_globalIllumination.isChecked());
		mEdtr.putFloat(Q3EPreference.pref_harm_r_globalIlluminationBrightness, Q3EUtils.parseFloat_s(V.edt_r_globalIlluminationBrightness.getText().toString(), 0.5f));
		mEdtr.putInt(Q3EPreference.pref_harm_r_renderMode, V.spinner_r_renderMode.getSelectedItemPosition());
		mEdtr.putBoolean(Q3EPreference.pref_harm_g_botEnableBuiltinAssets, V.cb_g_botEnableBuiltinAssets.isChecked());
		mEdtr.putInt(Q3EPreference.BUTTON_SWIPE_RELEASE_DELAY, Q3EUtils.parseInt_s(V.button_swipe_release_delay.getText().toString(), Q3EGlobals.BUTTON_SWIPE_RELEASE_DELAY_AUTO));

        mEdtr.commit();
    }

    private void OpenHelp()
    {
        ContextUtility.OpenMessageDialog(this, Q3ELang.tr(this, R.string.help), TextHelper.GetHelpText());
    }

    private void OpenUpdate()
    {
		UpdateCompatFunc m_updateCompatFunc = new UpdateCompatFunc(this);
		m_updateCompatFunc.Start(new Bundle());
    }

	private void SetCmdText(String text)
    {
		if(null == text || text.isEmpty())
			text = Q3EGameConstants.GAME_EXECUABLE;
        EditText edit = V.edt_cmdline;
        if (edit.getText().toString().equals(text))
            return;
		// disable focus command edittext when toggle other checkbox/radiogroup/edittext
		boolean editable = edit.isFocusable() || edit.isFocusableInTouchMode();
		if(editable)
		{
			edit.setEnabled(false);
		}
        int pos = edit.getSelectionStart();
        edit.setText(text);
		pos = Math.max(0, Math.min(pos, text.length()));
		try
		{
			edit.setSelection(pos);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
		if(editable)
		{
			edit.setEnabled(true);
		}
    }

	private String GetCmdText()
    {
		String s = V.edt_cmdline.getText().toString();
		if(s.isEmpty())
			s = Q3EGameConstants.GAME_EXECUABLE;
		return s;
    }

    private void OpenGameLibChooser()
    {
        final String PreferenceKey = Q3E.q3ei.GetGameModLibPreferenceKey();
        if (null == m_chooseGameLibFunc)
        {
            m_chooseGameLibFunc = new ChooseGameLibFunc(this, CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_LIBRARY);
            m_chooseGameLibFunc.SetAddCallback(new Runnable() {
                @Override
                public void run()
                {
                    if (null == m_addExternalLibraryFunc)
                        m_addExternalLibraryFunc = new AddExternalLibraryFunc(GameLauncher.this, new Runnable() {
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
            m_chooseGameLibFunc.SetEditCallback(new Runnable() {
                @Override
                public void run()
                {
                    if (null == m_editExternalLibraryFunc)
                        m_editExternalLibraryFunc = new EditExternalLibraryFunc(GameLauncher.this, new Runnable() {
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

        m_chooseGameLibFunc.SetCallback(new Runnable() {
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

	private void OpenCommandChooser()
	{
		final String PreferenceKey = Q3E.q3ei.GetGameCommandRecordPreferenceKey();
		String cmd = GetCmdText();
		ChooseCommandRecordFunc m_chooseCommandRecordFunc = new ChooseCommandRecordFunc(this);
		m_chooseCommandRecordFunc.SetCallback(new Runnable() {
			@Override
			public void run()
			{
				String cmdResult = m_chooseCommandRecordFunc.GetResult();
				if(null != cmdResult && !cmd.equals(cmdResult))
				{
					// lock -> unregister listener -> set cmd text -> register listener -> unlock -> update GUI widgets
					LockCmdUpdate();
					SetupCommandTextWatcher(false);
					V.edt_cmdline.setText(cmdResult); // SetCmdText(cmdResult);
					SetupCommandTextWatcher(true);
					UnlockCmdUpdate();
					updatehacktings();
				}
			}
		});
		Bundle bundle = new Bundle();
		bundle.putString("command", cmd);
		bundle.putString("key", PreferenceKey);
		m_chooseCommandRecordFunc.Start(bundle);
	}

	private void OpenEnvEditor()
	{
		final String PreferenceKey = Q3E.q3ei.GetGameEnvPreferenceKey();
		EditEnvFunc m_editEnvFunc = new EditEnvFunc(this);
		Bundle bundle = new Bundle();
		bundle.putString("key", PreferenceKey);
		m_editEnvFunc.Start(bundle);
	}

	private void OpenDirectoryHelper()
	{
		DirectoryHelperFunc m_directoryHelperFunc = new DirectoryHelperFunc(this);
		m_directoryHelperFunc.SetCallback(new Runnable() {
			@Override
			public void run() {
				CreateGameFolders();
			}
		});
		Bundle bundle = new Bundle();
		m_directoryHelperFunc.Start(bundle);
	}

	private void OpenShortcutCreator()
	{
/*		if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.N_MR1)
		{
			Toast.makeText(this, R.string.only_support_on_android_version_7_1, Toast.LENGTH_LONG).show();
			return;
		}*/
		if (null == m_createShortcutFunc)
		{
			m_createShortcutFunc = new CreateShortcutFunc(this, CONST_RESULT_CODE_REQUEST_CREATE_SHORTCUT);
		}
		Bundle bundle = new Bundle();
		m_createShortcutFunc.Start(bundle);
	}

	private void OpenShortcutWithCommandCreator()
	{
/*		if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.N_MR1)
		{
			Toast.makeText(this, R.string.only_support_on_android_version_7_1, Toast.LENGTH_LONG).show();
			return;
		}*/
		if (null == m_createCommandShortcutFunc)
		{
			m_createCommandShortcutFunc = new CreateCommandShortcutFunc(this, CONST_RESULT_CODE_REQUEST_CREATE_SHORTCUT_WITH_COMMAND);
		}
		Bundle bundle = new Bundle();
		bundle.putString("game", Q3E.q3ei.game);
		String cmd = GetCmdText();
		String tmpCmd = GetTempCmdText();
		if(KStr.NotBlank(tmpCmd))
			cmd += " " + tmpCmd;
		bundle.putString("command", cmd);
		m_createCommandShortcutFunc.Start(bundle);
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

	private void OpenAdvanceDialog()
	{
		AdvanceDialog dialog = AdvanceDialog.newInstance();
		dialog.show(getFragmentManager(), "AdvanceDialog");
	}

	private void UpdateResolution(int rgid)
	{
		int type;

		if(rgid == R.id.rg_scrres_scheme_custom)
			type = Q3EGlobals.SCREEN_CUSTOM;
		else if(rgid == R.id.rg_scrres_scheme_scale_area)
			type = Q3EGlobals.SCREEN_SCALE_BY_AREA;
		else if(rgid == R.id.rg_scrres_scheme_scale_length)
			type = Q3EGlobals.SCREEN_SCALE_BY_LENGTH;
		else if(rgid == R.id.rg_scrres_scheme_ratio)
			type = Q3EGlobals.SCREEN_FIXED_RATIO;
		else
			type = Q3EGlobals.SCREEN_FULL;

		UpdateResolutionByType(type);
	}

	private void UpdateResolutionByType(int type)
	{
		UpdateCustomResolution(type == Q3EGlobals.SCREEN_CUSTOM || (type == Q3EGlobals.SCREEN_FIXED_RATIO && V.use_custom_resolution.isChecked()));
		UpdateResolutionScaleSchemeBar(type == Q3EGlobals.SCREEN_SCALE_BY_LENGTH || type == Q3EGlobals.SCREEN_SCALE_BY_AREA);
		UpdateCustomRatio(type == Q3EGlobals.SCREEN_FIXED_RATIO);
		UpdateResolutionText();
	}

	private void UpdateResolutionText()
	{
		int[] size = Q3EUtils.GetFullScreenSize(this);
		int width, height;
		if(Q3EUtils.ActiveIsLandscape(this))
		{
			width = size[0];
			height = size[1];
		}
		else
		{
			width = size[1];
			height = size[0];
		}
		size = Q3EUtils.GetSurfaceViewSize(this, width, height);
		V.tv_scrres_size.setText(size[0] + " x " + size[1]);
	}

    private void UpdateCustomResolution(boolean on)
    {
		V.res_x.setEnabled(on);
		V.res_y.setEnabled(on);
		V.res_customlayout.setEnabled(on);
		V.res_customlayout.setVisibility(on ? View.VISIBLE : View.GONE);
    }

	private void UpdateCustomRatio(boolean on)
	{
		V.ratio_x.setEnabled(on);
		V.ratio_y.setEnabled(on);
		V.res_ratiolayout.setEnabled(on);
		V.res_ratiolayout.setVisibility(on ? View.VISIBLE : View.GONE);
	}

    private void SetGameInternalMod(String val)
    {
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
        boolean userMod = V.fs_game_user.isChecked(); //preference.getBoolean(Q3E.q3ei.GetEnableModPreferenceKey(), false);
        String game = null != val ? val : "";
		GameManager.GameProp prop = m_gameManager.ChangeGameMod(game, userMod);
		HandleGameProp(prop);

        preference.edit().putString(Q3E.q3ei.GetGameModPreferenceKey(), game).commit();
    }

	private void SetGameVersion(String val)
	{
		String key = Q3E.q3ei.GetGameVersionPreferenceKey();
		if(null != key && Q3E.q3ei.HasVersions())
		{
			SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
			preference.edit().putString(key, val).commit();
			Q3E.q3ei.SetupEngineVersion(val);
		}
		else
			Q3E.q3ei.SetupEngineVersion((String)null);
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
				case CONST_RESULT_CODE_REQUEST_CREATE_SHORTCUT:
					if (null != m_createShortcutFunc)
						m_createShortcutFunc.run();
					break;
				case CONST_RESULT_CODE_REQUEST_CREATE_SHORTCUT_WITH_COMMAND:
					if (null != m_createCommandShortcutFunc)
						m_createCommandShortcutFunc.run();
					break;
				case CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_EXTRAS_FILE:
					if (null != m_chooseExtrasFileFunc)
						m_chooseExtrasFileFunc.run();
					break;
				case CONST_RESULT_CODE_REQUEST_CREATE_GAME_FOLDER:
					if (null != m_createGameFolderFunc)
						m_createGameFolderFunc.run();
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
			case CONST_RESULT_CODE_REQUEST_CREATE_SHORTCUT:
				opt = Q3ELang.tr(this, R.string.create_desktop_shortcut);
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
        builder.setNeutralButton(R.string.grant, new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
                ContextUtility.OpenAppSetting(GameLauncher.this);
                dialog.dismiss();
            }
        });
        builder.create().show();
    }

	private void ChooseEditOnscreenType(Function runnable)
	{
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle(R.string.edit_type);
		builder.setItems(new String[] {
				Q3ELang.tr(this, R.string.common), Q3ELang.tr(this, R.string.current_game)
		}, new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which)
			{
				dialog.dismiss();
				runnable.Invoke(which);
			}
		});
		builder.setNegativeButton(R.string.cancel, null);
		builder.create().show();
	}

    private void OpenOnScreenButtonSetting()
    {
		ChooseEditOnscreenType(new Function() {
			@Override
			public Object Invoke(Object...args)
			{
				int which = (Integer)args[0];
				Intent intent = new Intent(GameLauncher.this, OnScreenButtonConfigActivity.class);
				if(which > 0)
					intent.putExtra("game", Q3E.q3ei.game);
				startActivity(intent);
				return null;
			}
		});
    }

    private void OpenCvarListDetail()
    {
		CVarHelperFunc m_cVarHelperFunc = new CVarHelperFunc(this);
		Bundle bundle = new Bundle();
		m_cVarHelperFunc.Start(bundle);
    }

	private void SetGame(String game)
    {
        Q3E.q3ei.SetupGame(game);
		Q3E.q3ei.SetupEngineVersion(this);

        if (null != V.main_menu_game)
            V.main_menu_game.setTitle(Q3E.q3ei.game_name);
        ActionBar actionBar = getActionBar();
        Resources res = getResources();
        int colorId = GameManager.GetGameThemeColor();
        int iconId = GameManager.GetGameIcon();

		// KARIN_NEW_GAME_BOOKMARK
		boolean openglVisible = Q3E.q3ei.HasOpenGLSetting();
		boolean quickloadVisible = Q3E.q3ei.IsSupportQuickload();
		boolean skipintroVisible = Q3E.q3ei.IsSupportSkipIntro();
		boolean versionVisible = Q3E.q3ei.HasVersions();
		boolean modVisible = Q3E.q3ei.IsSupportMod();

        if (null != actionBar)
        {
            actionBar.setBackgroundDrawable(new ColorDrawable(res.getColor(colorId)));
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
            {
                actionBar.setHomeAsUpIndicator(iconId);
            }
        }
		V.gamemod_section.setVisibility(modVisible ? View.VISIBLE : View.GONE);

		for(String g : groupRadios.keySet())
		{
			groupRadios.get(g).setVisibility(g.equals(Q3E.q3ei.game) ? View.VISIBLE : View.GONE);
		}

		V.rg_version_d3bfg.setVisibility(versionVisible && Q3E.q3ei.isD3BFG ? View.VISIBLE : View.GONE);
		V.rg_version_realrtcw.setVisibility(versionVisible && Q3E.q3ei.isRealRTCW ? View.VISIBLE : View.GONE);
		V.rg_version_tdm.setVisibility(versionVisible && Q3E.q3ei.isTDM ? View.VISIBLE : View.GONE);

		V.versions_container.setVisibility(versionVisible ? View.VISIBLE : View.GONE);
		String gameVersion = Q3E.q3ei.game_version;
		if(KStr.NotEmpty(gameVersion))
		{
			V.gameversion_section.setVisibility(View.VISIBLE);
			V.gameversion_label.SetText(Q3ELang.tr(this, R.string.version) + " " + gameVersion);
		}
		else
		{
			V.gameversion_section.setVisibility(View.GONE);
			V.gameversion_label.SetText(Q3ELang.tr(this, R.string.version));
		}

		V.idtech4_section.setVisibility(Q3E.q3ei.IsIdTech4() ? View.VISIBLE : View.GONE);
		V.yquake2_section.setVisibility(Q3E.q3ei.isQ2 ? View.VISIBLE : View.GONE);
		V.doom3bfg_section.setVisibility(Q3E.q3ei.isD3BFG ? View.VISIBLE : View.GONE);
		V.realrtcw_section.setVisibility(Q3E.q3ei.isRealRTCW ? View.VISIBLE : View.GONE);
		V.etw_section.setVisibility(Q3E.q3ei.isETW ? View.VISIBLE : View.GONE);
		V.zdoom_section.setVisibility(Q3E.q3ei.isDOOM ? View.VISIBLE : View.GONE);
		V.tdm_section.setVisibility(Q3E.q3ei.isTDM ? View.VISIBLE : View.GONE);
		V.fteqw_section.setVisibility(Q3E.q3ei.isFTEQW ? View.VISIBLE : View.GONE);
		V.xash3d_section.setVisibility(Q3E.q3ei.isXash3D ? View.VISIBLE : View.GONE);
		V.source_section.setVisibility(Q3E.q3ei.isSource ? View.VISIBLE : View.GONE);
		V.urt_section.setVisibility(Q3E.q3ei.isUrT ? View.VISIBLE : View.GONE);
		V.sdl_section.setVisibility(Q3E.q3ei.IsUsingSDL() ? View.VISIBLE : View.GONE);
		V.openal_section.setVisibility(Q3E.q3ei.IsUsingOpenAL() ? View.VISIBLE : View.GONE);

		V.opengl_section.setVisibility(openglVisible ? View.VISIBLE : View.GONE);
		V.auto_quick_load.setVisibility(quickloadVisible ? View.VISIBLE : View.GONE);
		V.skip_intro.setVisibility(skipintroVisible ? View.VISIBLE : View.GONE);
		V.dll_section.setVisibility(Q3E.q3ei.IsSupportExternalDLL() ? View.VISIBLE : View.GONE);
		if(Q3E.q3ei.IsIdTech4())
			V.find_dll_desc.setText(R.string.using_external_game_library);
		else if(Q3E.q3ei.isXash3D)
			V.find_dll_desc.setText(R.string.using_external_game_library_xash3d);
		else
			V.find_dll_desc.setText("");

		String subdir = Q3E.q3ei.subdatadir;
		if(null == subdir)
			subdir = "";
		else
			subdir += "/";
		String gameCommandParm = Q3E.q3ei.GetGameCommandParm();
		V.launcher_fs_game_cvar.setText(KStr.NotEmpty(gameCommandParm) ? "(" + gameCommandParm + ")" : "");
		V.launcher_fs_game_subdir.setText(Q3ELang.tr(this, R.string.sub_directory) + subdir);
		V.launcher_fs_game_subdir.setVisibility(subdir.isEmpty() ? View.GONE : View.VISIBLE);

		CollapseMods(V.collapse_mods.isChecked());

		SelectGameVersion();
    }

	private void SelectGameVersion()
	{
		String[] versions = Q3EInterface.GetGameVersions(Q3E.q3ei.game);
		if(null == versions)
			return;
		String key = Q3E.q3ei.GetGameVersionPreferenceKey();
		if(null == key)
			return;
		RadioGroup radioGroup = GetGameVersionRadioGroup();
		if(null == radioGroup)
			return;

		String cur = PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).getString(key, Q3EGameConstants.GAME_VERSION_CURRENT);
		int index = 0;
		for(int i = 0; i < versions.length; i++)
		{
			String version = versions[i];
			if(null == cur && null == version)
			{
				index = i;
				break;
			}
			else if(null != cur && cur.equalsIgnoreCase(version))
			{
				index = i;
				break;
			}
			else if(null != version && version.equalsIgnoreCase(cur))
			{
				index = i;
				break;
			}
		}
		SelectRadioGroup(radioGroup, index);
	}

    private void ChangeGame(String newGame)
    {
		LockCmdUpdate();
		SetupCommandTextWatcher(false);
        if (null == newGame || newGame.isEmpty())
        {
            int i;
			String[] Games = GameManager.Games(false);
			for (i = 0; i < Games.length; i++)
            {
                if (Games[i].equalsIgnoreCase(Q3E.q3ei.game))
                    break;
            }
            newGame = Games[(i + 1) % Games.length];
        }
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
        preference.edit().putString(Q3EPreference.pref_harm_game, newGame).commit();
        SetGame(newGame);

		//Q3E.q3ei.LoadTypeAndArgTablePreference(this);

        preference.edit().putString(Q3EPreference.pref_harm_game_lib, "").commit();

		String cmd = preference.getString(Q3E.q3ei.GetGameCommandPreferenceKey(), Q3EGameConstants.GAME_EXECUABLE);
		V.edt_cmdline.setText(cmd);
		SetupCommandTextWatcher(true);
		UnlockCmdUpdate();

		// if is DOOM3/Quake4/Prey, update launcher
		updatehacktings();

		// put last
		String game = preference.getString(Q3E.q3ei.GetGameModPreferenceKey(), "");
		if (null == game)
			game = "";
		boolean userMod = preference.getBoolean(Q3E.q3ei.GetEnableModPreferenceKey(), false);
		V.fs_game_user.setChecked(userMod);
		String userModName = preference.getString(Q3E.q3ei.GetGameUserModPreferenceKey(), "");
		String cur = V.edt_fs_game.getText().toString();
		if (!userModName.equals(cur))
		{
			V.edt_fs_game.setText(userModName);
		}
		if(userMod)
			game = userModName;

		GameManager.GameProp prop = m_gameManager.ChangeGameMod(game, userMod);
		HandleGameProp(prop);
		SelectRadioGroup(GetGameModRadioGroup(), prop.index);

		Q3E.q3ei.start_temporary_extra_command = Q3E.q3ei.MakeTempBaseCommand(this);
		UpdateTempCommand();
    }

	private void SetupCommandTextWatcher(boolean b)
	{
		if(b)
		{
			V.edt_cmdline.addTextChangedListener(m_commandTextWatcher);
			m_commandTextWatcher.Install(true);
		}
		else
		{
			m_commandTextWatcher.Uninstall();
			V.edt_cmdline.removeTextChangedListener(m_commandTextWatcher);
		}
	}

    private void OpenResourceFileDialog(boolean all)
    {
        if (null == m_extractPatchResourceFunc)
            m_extractPatchResourceFunc = new ExtractPatchResourceFunc(this, CONST_RESULT_CODE_REQUEST_EXTRACT_PATCH_RESOURCE);
        Bundle bundle = new Bundle();
        bundle.putString("path", V.edt_path.getText().toString());
		bundle.putString("game", Q3E.q3ei.game);
		bundle.putBoolean("all", all);
		SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
		boolean userMod = preferences.getBoolean(Q3EInterface.GetEnableModPreferenceKey(Q3E.q3ei.game), false);
		String gamebase = Q3EInterface.GetGameBaseDirectory(Q3E.q3ei.game);
		String base;
		if(userMod)
		{
			base = preferences.getString(Q3EInterface.GetGameModPreferenceKey(Q3E.q3ei.game), gamebase);
		}
		else
			base = GetGameModFromCommand();
		if(KStr.IsEmpty(base))
			base = gamebase;
		bundle.putString("mod", base);
        m_extractPatchResourceFunc.Start(bundle);
    }

    private void RemoveParam(String name)
    {
        if(!LockCmdUpdate())
        	return;
        String orig = GetCmdText();
		String str = Q3E.q3ei.GetGameCommandEngine(orig).RemoveParam(name).toString();
        if (!orig.equals(str))
            SetCmdText(str);
        UnlockCmdUpdate();
    }

    private void SetParam(String name, Object val)
    {
		if(!LockCmdUpdate())
			return;
        SetCmdText(Q3E.q3ei.GetGameCommandEngine(GetCmdText()).SetParam(name, val).toString());
		// SetCmdText(KidTech4Command.SetParam(GetCmdText(), name, val));
        UnlockCmdUpdate();
    }

    private String GetParam(String name)
    {
		return Q3E.q3ei.GetGameCommandEngine(GetCmdText()).Param(name);
        // return KidTech4Command.GetParam(GetCmdText(), name);
    }

	private List<String> GetParamList(String name)
	{
		String parm = GetParam(name);
		if(null == parm)
			return null;
		else if(parm.trim().isEmpty())
			return new ArrayList<>();
		else
			return KidTechCommand.SplitValue(parm, true);
	}

	private void SetParamList(String name, List<String> parms)
	{
		SetParam(name, KidTechCommand.JoinValue(parms, true));
	}

    private void SetParamPrefix(String prefix, String name, Object val)
    {
		if(!LockCmdUpdate())
			return;
		SetCmdText(new KidTechCommand(prefix, GetCmdText()).SetParam(name, val).toString());
        UnlockCmdUpdate();
    }

	private void RemoveAllParamsPrefix(String prefix)
	{
		if(!LockCmdUpdate())
			return;
		SetCmdText(new KidTechCommand(prefix, GetCmdText()).RemoveAllParams().toString());
		UnlockCmdUpdate();
	}

	private String GetParamPrefix(String prefix, String name)
	{
		return new KidTechCommand(prefix, GetCmdText()).Param(name);
		// return KidTech4Command.GetParam(GetCmdText(), name);
	}

	private void RemoveParamPrefix(String prefix, String name)
	{
		if(!LockCmdUpdate())
			return;
		String orig = GetCmdText();
		String str = new KidTechCommand(prefix, orig).RemoveParam(name).toString();
		if (!orig.equals(str))
			SetCmdText(str);
		UnlockCmdUpdate();
	}

	private String GetTempCmdText()
	{
		return V.edt_cmdline_temp.getText().toString();
	}

    private void RemoveParam_temp(String name, String...val)
    {
		String orig = GetTempCmdText();
        String str = Q3E.q3ei.GetGameCommandEngine(orig).RemoveParam(name, val).toString();
        if (!orig.equals(str))
            Q3E.q3ei.start_temporary_extra_command = str;
		UpdateTempCommand();
    }

	private void AddParam_temp(String name, Object val)
	{
		Q3E.q3ei.start_temporary_extra_command = Q3E.q3ei.GetGameCommandEngine(GetTempCmdText()).AddParam(name, val).toString();
		UpdateTempCommand();
	}

    private void SetParam_temp(String name, Object val)
    {
		Q3E.q3ei.start_temporary_extra_command = Q3E.q3ei.GetGameCommandEngine(GetTempCmdText()).SetParam(name, val).toString();
		UpdateTempCommand();
    }

	private void SetProp_temp(String name, Object val)
	{
		Q3E.q3ei.start_temporary_extra_command = Q3E.q3ei.GetGameCommandEngine(GetTempCmdText()).SetProp(name, val).toString();
		UpdateTempCommand();
	}

	private void RemoveProp_temp(String name)
	{
		String orig = GetTempCmdText();
		String str = Q3E.q3ei.GetGameCommandEngine(orig).RemoveProp(name).toString();
		if (!orig.equals(str))
			Q3E.q3ei.start_temporary_extra_command = str;
		UpdateTempCommand();
	}

	private void SetCommand_temp(String name, boolean prepend)
	{
		Q3E.q3ei.start_temporary_extra_command = Q3E.q3ei.GetGameCommandEngine(GetTempCmdText()).SetCommand(name, prepend).toString();
		UpdateTempCommand();
	}

	private void RemoveCommand_temp(String name)
	{
		String orig = GetTempCmdText();
		String str = Q3E.q3ei.GetGameCommandEngine(orig).RemoveCommand(name).toString();
		if (!orig.equals(str))
			Q3E.q3ei.start_temporary_extra_command = str;
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
        // V.launcher_tab2_volume_map_config_layout.setVisibility(on ? View.VISIBLE : View.GONE); // always visible
        int key = preference.getInt(Q3EPreference.VOLUME_UP_KEY, Q3EKeyCodes.KeyCodes.K_F3);
        V.launcher_tab2_volume_up_map_config_keys.setSelection(Utility.ArrayIndexOf(keyCodes, key));
        key = preference.getInt(Q3EPreference.VOLUME_DOWN_KEY, Q3EKeyCodes.KeyCodes.K_F2);
        V.launcher_tab2_volume_down_map_config_keys.setSelection(Utility.ArrayIndexOf(keyCodes, key));
		// only disable
		V.launcher_tab2_volume_up_map_config_keys.setEnabled(on);
		V.launcher_tab2_volume_down_map_config_keys.setEnabled(on);
    }

    private void UpdateEnableGyro(boolean on)
    {
        // V.launcher_tab2_enable_gyro_layout.setVisibility(on ? View.VISIBLE : View.GONE); // always visible
        SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
        V.launcher_tab2_gyro_x_axis_sens.setText(Q3EPreference.GetStringFromFloat(preference, Q3EPreference.pref_harm_view_motion_gyro_x_axis_sens, Q3EGlobals.GYROSCOPE_X_AXIS_SENS));
        V.launcher_tab2_gyro_y_axis_sens.setText(Q3EPreference.GetStringFromFloat(preference, Q3EPreference.pref_harm_view_motion_gyro_y_axis_sens, Q3EGlobals.GYROSCOPE_Y_AXIS_SENS));
		// only disable
		V.launcher_tab2_gyro_x_axis_sens.setEnabled(on);
		V.launcher_tab2_gyro_y_axis_sens.setEnabled(on);
    }

    private void OpenCheckForUpdateDialog()
    {
        if (null == m_checkForUpdateFunc)
            m_checkForUpdateFunc = new CheckForUpdateFunc(this);
        m_checkForUpdateFunc.Start(new Bundle());
    }

	private void OpenDownloadTestingDialog()
	{
		String url = Constants.CONST_TESTING_URL;
		if(!Q3EJNI.Is64())
			url += "_armv7";
		ContextUtility.OpenUrlExternally(this, url);
	}

    private RadioGroup GetGameModRadioGroup()
    {
		return V.GameGroups[Q3E.q3ei.game_id];
    }

	private RadioGroup GetGameVersionRadioGroup()
	{
		return new RadioGroup[]{
				null,
				null,
				null,
				null,
				null,
				null,
				null,
				V.rg_version_d3bfg,
				V.rg_version_tdm,
				null,
				null,
				V.rg_version_realrtcw,
				null,
				null,
				null,
		}[Q3E.q3ei.game_id];
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

	private void OpenControllerSetting()
	{
		Intent intent = new Intent(this, ControllerConfigActivity.class);
		startActivity(intent);
	}

    private String GetExternalGameLibraryPath()
    {
        return getFilesDir() + File.separator + Q3E.q3ei.game + File.separator + Q3EGlobals.ARCH;
    }

    private void OpenTranslatorsDialog()
	{
		new TranslatorsFunc(this).Start(new Bundle());
	}

	private void EditCVar()
	{
		Bundle bundle = new Bundle();
		bundle.putString("game", GetGameModFromCommand());
		bundle.putString("command", Q3E.q3ei.start_temporary_extra_command);
		bundle.putString("baseCommand", Q3E.q3ei.MakeTempBaseCommand(this));
		// bundle.putString("persistentCommand", GetCmdText());
		CVarEditorFunc cVarEditorFunc = new CVarEditorFunc(this, new Runnable() {
			@Override
			public void run()
			{
				Q3E.q3ei.start_temporary_extra_command = CVarEditorFunc.GetResultFromBundle(bundle);
				UpdateTempCommand();
			}
		});
		cVarEditorFunc.Start(bundle);
	}

	private void UpdateTempCommand()
	{
		Q3E.q3ei.start_temporary_extra_command = Q3E.q3ei.start_temporary_extra_command.trim(); //k: trim it 20241114

		if(Q3E.q3ei.start_temporary_extra_command.equals(GetTempCmdText()))
			return;

		V.edt_cmdline_temp.setText(Q3E.q3ei.start_temporary_extra_command);
		V.temp_cmdline.setVisibility(Q3E.q3ei.start_temporary_extra_command.length() > 0 ? View.VISIBLE : View.GONE);
	}

	private void OpenGameModChooser()
	{
		SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(this);
		String preferenceKey = Q3E.q3ei.GetGameModPreferenceKey();
		if (null == m_chooseGameModFunc)
		{
			m_chooseGameModFunc = new ChooseGameModFunc(this, CONST_RESULT_CODE_REQUEST_EXTERNAL_STORAGE_FOR_CHOOSE_GAME_MOD);
		}

		m_chooseGameModFunc.SetCallback(new Runnable() {
			@Override
			public void run()
			{
				String mod = m_chooseGameModFunc.GetResult();
				if(mod.startsWith(":"))
					SetupExtrasFiles(mod);
				else
				{
					String cur = V.edt_fs_game.getText().toString();
					if (!mod.equals(cur))
						V.edt_fs_game.setText(mod);
				}
			}
		});
		Bundle bundle = new Bundle();
		String path = KStr.AppendPath(preference.getString(Q3EPreference.pref_datapath, default_gamedata), Q3E.q3ei.subdatadir, Q3E.q3ei.GetGameModSubDirectory());
		bundle.putString("mod", preference.getString(preferenceKey, ""));
		bundle.putString("path", path);
		if(Q3E.q3ei.isDOOM)
			bundle.putString("file", GetParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "file") + " " + GetParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "deh") + " " + GetParamPrefix(KidTechCommand.ARG_PREFIX_ALL, "bex"));
		m_chooseGameModFunc.Start(bundle);
	}

	private void UpdateResolutionScaleSchemeBar(boolean on)
	{
		V.res_scale.setEnabled(on);
		V.res_scale_layout.setEnabled(on);
		V.res_scale_layout.setVisibility(on ? View.VISIBLE : View.GONE);
	}

	private void HandleGameProp(GameManager.GameProp prop)
	{
		boolean b = m_commandTextWatcher.IsEnabled();
		m_commandTextWatcher.Uninstall();
		if(prop.is_user)
		{
			if (!prop.fs_game.isEmpty())
				SetGameModToCommand(prop.fs_game);
			else
				RemoveGameModFromCommand();
			RemoveSecondaryGameModFromCommand();
			RemoveProp("harm_fs_gameLibPath");
		}
		else
		{
			if(null == prop.fs_game || prop.fs_game.isEmpty() || !prop.IsValid())
				RemoveGameModFromCommand();
			else
				SetGameModToCommand(prop.fs_game);
			if(null == prop.fs_game_base || prop.fs_game_base.isEmpty() || !prop.IsValid())
				RemoveSecondaryGameModFromCommand();
			else if(Q3E.q3ei.IsIdTech4() || Q3E.q3ei.isD3BFG || Q3E.q3ei.isTDM || Q3E.q3ei.isFTEQW)
				SetSecondaryGameModToCommand(prop.fs_game_base);
			RemoveProp("harm_fs_gameLibPath");
		}
		m_commandTextWatcher.Install(b);
	}

	public GameManager GetGameManager()
	{
		return m_gameManager;
	}

	private void InitGameVersionList()
	{
		versionGroupRadios.clear();
		RadioButton radio;
		RadioGroup group;
		RadioGroup.LayoutParams layoutParams;

		SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);

		versionGroupRadios.put(Q3EGameConstants.GAME_DOOM3BFG, V.rg_version_d3bfg);
		versionGroupRadios.put(Q3EGameConstants.GAME_REALRTCW, V.rg_version_realrtcw);
		versionGroupRadios.put(Q3EGameConstants.GAME_TDM, V.rg_version_tdm);

		for (String type : GameManager.Games(false))
		{
			group = versionGroupRadios.get(type);
			if(null == group)
				continue;
			String[] versions = Q3EInterface.GetGameVersions(type);
			if(null == versions)
				continue;
			String key = Q3EInterface.GetGameVersionPreferenceKey(type);
			if(null == key)
				continue;
			layoutParams = new RadioGroup.LayoutParams(RadioGroup.LayoutParams.WRAP_CONTENT, RadioGroup.LayoutParams.WRAP_CONTENT);
			String cur = preferences.getString(key, Q3EGameConstants.GAME_VERSION_CURRENT);

			for(String version : versions)
			{
				radio = new RadioButton(group.getContext());
				String name;
				if(KStr.IsEmpty(version))
					name = Q3ELang.tr(this, R.string._default);
				else
					name = version;
				radio.setText(name);
				radio.setTag(version);
				group.addView(radio, layoutParams);
				if(null == cur && null == version)
					radio.setChecked(true);
				else if(null != cur)
					radio.setChecked(cur.equalsIgnoreCase(version));
				else if(null != version)
					radio.setChecked(version.equalsIgnoreCase(cur));
			}
		}
	}

	private void InitGameList()
	{
		RadioButton radio;
		RadioGroup group;
		RadioGroup.LayoutParams layoutParams;

		Game[] values = Game.values();

		for (Game value : values)
		{
/*			String subdir = "";

			if(Q3E.q3ei.standalone)
			{
				subdir = Q3EInterface.GetGameStandaloneDirectory(value.type);
				if(!subdir.isEmpty())
					subdir += "/";
			}*/

			group = groupRadios.get(value.type);
			layoutParams = new RadioGroup.LayoutParams(RadioGroup.LayoutParams.WRAP_CONTENT, RadioGroup.LayoutParams.WRAP_CONTENT);
			radio = new RadioButton(group.getContext());
			String name;
			if(value.name instanceof Integer)
				name = Q3ELang.tr(this, (Integer)value.name);
			else if(value.name instanceof String)
				name = (String)value.name;
			else
				name = "";
			if(Q3EGameConstants.GAME_ZDOOM.equalsIgnoreCase(value.type))
			{
				name += " (" + /*subdir +*/ value.file + ")";
			}
			else
			{
				if(KStr.NotEmpty(value.fs_game))
					name += " [" + value.fs_game + "]";
				if(KStr.NotEmpty(value.file))
					name += "(" + /*subdir +*/ value.file + "/)";
			}
			radio.setText(name);
			radio.setTag(value.game);
			group.addView(radio, layoutParams);
			radio.setChecked(!value.is_mod);
		}
	}

	private void SetupCommandLine(boolean readonly)
	{
		UIUtility.EditText__SetReadOnly(V.edt_cmdline, readonly, InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
		V.edt_cmdline.post(new Runnable() {
			@Override
			public void run() {
				CollapseCmdline(Q3EPreference.GetIntFromString(GameLauncher.this, PreferenceKey.COLLAPSE_CMDLINE, 0));
			}
		});
	}

	private void SetupTempCommandLine(boolean editable)
	{
		UIUtility.EditText__SetReadOnly(V.edt_cmdline_temp, !editable, InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
	}

	public void OpenSuggestGameWorkingDirectory(String curPath)
	{
/*		if(ContextUtility.InScopedStorage() && !ContextUtility.IsInAppPrivateDirectory(GameLauncher.this, curPath))
		{
			String path = Q3EUtils.GetAppStoragePath(GameLauncher.this);
			Toast.makeText(GameLauncher.this, Q3ELang.tr(this, R.string.suggest_game_woring_directory_tips, path), Toast.LENGTH_LONG).show();
		}*/
		m_edtPathFocused = curPath;
	}

	private void SetGameModToCommand(String mod)
	{
		String arg = Q3E.q3ei.GetGameCommandParm();
		if(Q3E.q3ei.isQ1 || Q3E.q3ei.isXash3D || Q3E.q3ei.isSource || Q3E.q3ei.isWolf3D)
			SetParam(arg, mod);
		else if(Q3E.q3ei.isDOOM)
		{
			RemoveParamPrefix(KidTechCommand.ARG_PREFIX_ALL, arg);
			SetParam(arg, mod);
		}
		else if(Q3E.q3ei.isFTEQW)
		{
			RemoveAllParamsPrefix(KidTechCommand.ARG_PREFIX_QUAKETECH);
			SetParamPrefix(KidTechCommand.ARG_PREFIX_QUAKETECH, mod, "");
		}
		else if(Q3E.q3ei.isSamTFE) {}
		else if(Q3E.q3ei.isSamTSE) {}
		else
			SetProp(arg, mod);
	}

	private String GetGameModFromCommand()
	{
		String arg = Q3E.q3ei.GetGameCommandParm();
		if(Q3E.q3ei.isQ1 || Q3E.q3ei.isXash3D || Q3E.q3ei.isSource || Q3E.q3ei.isWolf3D)
			return GetParam(arg);
		else if(Q3E.q3ei.isDOOM)
			return GetParamPrefix(KidTechCommand.ARG_PREFIX_ALL, arg);
		else if(Q3E.q3ei.isFTEQW)
		{
			List<String> list = new KidTechCommand(KidTechCommand.ARG_PREFIX_ALL, GetCmdText()).GetAllParamNames();
			if(!list.isEmpty())
			{
				String parm = list.get(0);
				return KStr.TrimLeft(parm, '-');
			}
			else
				return "";
		}
		else if(Q3E.q3ei.isSamTFE)
			return "";
		else if(Q3E.q3ei.isSamTSE)
			return "";
		else
			return GetProp(arg);
	}

	private void RemoveGameModFromCommand()
	{
		String arg = Q3E.q3ei.GetGameCommandParm();
		if(Q3E.q3ei.isQ1 || Q3E.q3ei.isXash3D || Q3E.q3ei.isSource || Q3E.q3ei.isWolf3D)
			RemoveParam(arg);
		else if(Q3E.q3ei.isDOOM)
			RemoveParamPrefix(KidTechCommand.ARG_PREFIX_ALL, arg);
		else if(Q3E.q3ei.isFTEQW)
			RemoveAllParamsPrefix(KidTechCommand.ARG_PREFIX_QUAKETECH);
		else if(Q3E.q3ei.isSamTFE);
		else if(Q3E.q3ei.isSamTSE);
		else
			RemoveProp(arg);
	}

	private void SetSecondaryGameModToCommand(String mod)
	{
		String arg = Q3E.q3ei.GetSecondaryGameCommandParm();
		if(KStr.IsEmpty(arg))
			return;
		if(Q3E.q3ei.isFTEQW)
		{
			SetParamPrefix(KidTechCommand.ARG_PREFIX_QUAKETECH, arg, mod);
		}
		else
			SetProp(arg, mod);
	}

	private void RemoveSecondaryGameModFromCommand()
	{
		String arg = Q3E.q3ei.GetSecondaryGameCommandParm();
		if(KStr.IsEmpty(arg))
			return;
		if(Q3E.q3ei.isFTEQW)
			RemoveParamPrefix(KidTechCommand.ARG_PREFIX_QUAKETECH, arg);
		else
			RemoveProp(arg);
	}

	private void SetupZDOOMFiles(String name, String file, boolean checked)
	{
		PreferenceManager.getDefaultSharedPreferences(GameLauncher.this).edit()
				.putBoolean(Q3EPreference.pref_harm_zdoom_load_lights_pk3, checked)
				.commit();
		if (checked)
			AddParam_temp(name, file);
		else
			RemoveParam_temp(name, file);
/*
// from parsing command
		boolean changed = false;
		List<String> files = GetParamList(name);
		if(checked)
		{
			if(null == files)
			{
				files = new ArrayList<>();
				files.add(file);
				changed = true;
			}
			else if(!files.contains(file))
			{
				files.add(0, file);
				changed = true;
			}
		}
		else
		{
			if(null != files && files.contains(file))
			{
				files.remove(file);
				changed = true;
			}
		}
		if(changed)
			SetParamList(name, files);*/
	}

	private void CollapseMods(boolean on)
	{
		final int VisibleRadioCount = 4;
		RadioGroup radioGroup = null;
		int childCount = V.mods_container_layout.getChildCount();
		for(int i = 0; i < childCount; i++)
		{
			View view = V.mods_container_layout.getChildAt(i);
			if(view.getVisibility() == View.VISIBLE)
			{
				int radioCount = ((RadioGroup)view).getChildCount();
				if(radioCount > VisibleRadioCount)
				{
					radioGroup = (RadioGroup)view;
				}
				break;
			}
		}

		ViewGroup.LayoutParams layoutParams = V.mods_container.getLayoutParams();
		if(null == radioGroup)
		{
			layoutParams.height = ViewGroup.LayoutParams.WRAP_CONTENT;
			V.mods_container.setLayoutParams(layoutParams);
			V.mods_container.setNestedScrollingEnabled(false);
			return;
		}

		if(on)
		{
			int height = 0;
			for (int m = 0; m < VisibleRadioCount; m++)
			{
				View radio = radioGroup.getChildAt(m);
				height += Math.max(radio.getHeight(), radio.getMeasuredHeight());
			}
			layoutParams.height = height;
		}
		else
		{
			layoutParams.height = ViewGroup.LayoutParams.WRAP_CONTENT;
		}
		V.mods_container.setLayoutParams(layoutParams);
		V.mods_container.setNestedScrollingEnabled(on);
	}

	private String GetConsoleHeightText(int progress)
	{
		if(progress <= 0 || progress >= 100)
			return Q3ELang.tr(this, R.string.no_limit);
		else
			return progress + "%";
	}

	private void OpenGameList()
	{
		GameChooserFunc gameChooserFunc = new GameChooserFunc(this);
		Bundle bundle = new Bundle();
		bundle.putString("game", Q3E.q3ei.game);
		gameChooserFunc.SetCallback(new Runnable() {
			@Override
			public void run()
			{
				ChangeGame((String)gameChooserFunc.GetResult());
			}
		});
		gameChooserFunc.Start(bundle);
	}

	private void OpenMenu()
	{
		openContextMenu(V.launcher_tab1_open_menu);
	}

	private void CreateGameFolders()
	{
		if (null == m_createGameFolderFunc)
			m_createGameFolderFunc = new CreateGameFolderFunc(this, CONST_RESULT_CODE_REQUEST_CREATE_GAME_FOLDER);
		m_createGameFolderFunc.Start(new Bundle());
	}

	private void CollapseCmdline(int maxHeight)
	{
		ViewGroup.LayoutParams layoutParams = V.cmdline_container.getLayoutParams();
		if(maxHeight > 0 && V.edt_cmdline.getHeight() > maxHeight)
		{
			layoutParams.height = maxHeight;
			V.cmdline_container.setLayoutParams(layoutParams);
			V.cmdline_container.setNestedScrollingEnabled(true);
		}
		else
		{
			layoutParams.height = ViewGroup.LayoutParams.WRAP_CONTENT;
			V.cmdline_container.setLayoutParams(layoutParams);
			V.cmdline_container.setNestedScrollingEnabled(false);
		}
	}

	private void SyncCmdCheckbox(CheckBox cb, String prop, boolean def)
	{
		cb.setChecked(getProp(prop, def));
		//k fill commandline
		if (!IsProp(prop)) setProp(prop, def);
	}

	private void SyncCmdEditText(EditText ed, String prop, Object def)
	{
		String str = GetProp(prop);
		if (null != str)
			ed.setText(str);
		if (!IsProp(prop)) SetProp(prop, Objects.toString(def, ""));
	}

	private void SyncCmdRadioGroup(RadioGroup rg, String prop, Object def, Function func)
	{
		String str = GetProp(prop);
		String defStr = Objects.toString(def, "");
		Integer index = null;
		if(null != str)
			index = (Integer)func.Invoke(str);
		if(null == index)
			index = (Integer)func.Invoke(defStr);
		if(null == index)
			index = 0;
		int max = GetRadioGroupNum(rg);
		if(index < 0)
			index = 0;
		else if(index >= max)
			index = max - 1;
		SelectRadioGroup(rg, index);
		if (!IsProp(prop)) SetProp(prop, defStr);
	}

	private void SyncCmdRadioGroupV(RadioGroup rg, String prop, String...options)
	{
		SyncCmdRadioGroup(rg, prop, options[0], new Function() {
			@Override
			public Object Invoke(Object... args) {
				Object str = args[0];
				int index = Utility.ArrayIndexOf(options, str);
				return index < 0 ? null : index;
			}
		});
	}

	private void SyncCmdRadioGroup(RadioGroup rg, String prop, Object def, Object[] options)
	{
		SyncCmdRadioGroup(rg, prop, def, new Function() {
			@Override
			public Object Invoke(Object... args) {
				Object str = args[0];
				int index = Utility.ArrayIndexOf(options, str);
				return index < 0 ? null : index;
			}
		});
	}

	private void SyncCmdRadioGroup(RadioGroup rg, String prop, int def, int count)
	{
		SyncCmdRadioGroup(rg, prop, "" + def, new Function() {
			@Override
			public Object Invoke(Object... args) {
				int str = Q3EUtils.parseInt_s((String)args[0], def);
				if(str >= 0 && str < count)
					return str;
				else
					return null;
			}
		});
	}

	private void SyncCmdRadioGroup(RadioGroup rg, String prop, int def)
	{
		SyncCmdRadioGroup(rg, prop, def, GetRadioGroupNum(rg));
	}

	private void SyncCmdRadioGroup(RadioGroup rg, String prop, int def, int min, int max)
	{
		SyncCmdRadioGroup(rg, prop, "" + def, new Function() {
			@Override
			public Object Invoke(Object... args) {
				int str = Q3EUtils.parseInt_s((String)args[0], def);
				if(str >= min && str <= max)
					return str - min;
				else
					return null;
			}
		});
	}

	private void SyncCmdRadioGroup2(RadioGroup rg, String prop, String str, Object def, Function func)
	{
		String defStr = Objects.toString(def, "");
		Integer index = null;
		if(null != str)
			index = (Integer)func.Invoke(str);
		if(null == index)
			index = (Integer)func.Invoke(defStr);
		if(null == index)
			index = 0;
		int max = GetRadioGroupNum(rg);
		if(index < 0)
			index = 0;
		else if(index >= max)
			index = max - 1;
		SelectRadioGroup(rg, index);
	}

	private void SyncCmdRadioGroup2V(RadioGroup rg, String prop, String val, String...options)
	{
		SyncCmdRadioGroup2(rg, prop, val, options[0], new Function() {
			@Override
			public Object Invoke(Object... args) {
				Object str = args[0];
				int index = Utility.ArrayIndexOf(options, str);
				return index < 0 ? null : index;
			}
		});
	}

	private void SyncCmdRadioGroup2(RadioGroup rg, String prop, String val, Object def, Object[] options)
	{
		SyncCmdRadioGroup2(rg, prop, val, def, new Function() {
			@Override
			public Object Invoke(Object... args) {
				Object str = args[0];
				int index = Utility.ArrayIndexOf(options, str);
				return index < 0 ? null : index;
			}
		});
	}

	private void SyncCmdRadioGroup2i(RadioGroup rg, String prop, String val, int defIndex, Object[] options)
	{
		SyncCmdRadioGroup2(rg, prop, val, options[defIndex], options);
	}

	private void SyncCmdRadioGroup2(RadioGroup rg, String prop, String val, int def, int count)
	{
		SyncCmdRadioGroup2(rg, prop, val, "" + def, new Function() {
			@Override
			public Object Invoke(Object... args) {
				int str = Q3EUtils.parseInt_s((String)args[0], def);
				if(str >= 0 && str < count)
					return str;
				else
					return null;
			}
		});
	}

	private void SyncCmdRadioGroup2(RadioGroup rg, String prop, String val, int def)
	{
		SyncCmdRadioGroup2(rg, prop, val, def, GetRadioGroupNum(rg));
	}

	private void SyncCmdRadioGroup2(RadioGroup rg, String prop, String val, int def, int min, int max)
	{
		SyncCmdRadioGroup2(rg, prop, val, "" + def, new Function() {
			@Override
			public Object Invoke(Object... args) {
				int str = Q3EUtils.parseInt_s((String)args[0], def);
				if(str >= min && str <= max)
					return str - min;
				else
					return null;
			}
		});
	}



    private class ViewHolder
    {
		public RadioGroup[] GameGroups;

        public MenuItem main_menu_game;
        public EditText edt_cmdline;
        public LinearLayout res_customlayout;
        public CheckBox nolight;
        public CheckBox useetc1cache;
        public CheckBox useetc1;
        public CheckBox usedxt;
        public RadioGroup r_harmclearvertexbuffer;
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
        public Button launcher_tab1_edit_config;
        public Button launcher_tab1_game_data_chooser_button;
        public RadioGroup rg_curpos;
        public EditText edt_harm_r_specularExponent;
        public RadioGroup rg_harm_r_lightingModel;
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
		public CheckBox cb_r_autoAspectRatio;
        public RadioGroup rg_fs_preygame;
        public CheckBox multithreading;
        public RadioGroup rg_s_driver;
        public CheckBox launcher_tab2_joystick_unfixed;
        public Button setup_onscreen_button_theme;
		public Button setup_controller;
        public CheckBox using_mouse;
        public TextView tv_mprefs;
        public LinearLayout layout_mouse_device;
        public CheckBox find_dll;
		public TextView find_dll_desc;
		public EditText edt_harm_r_maxFps;
		public Button launcher_tab1_edit_cvar;
		public Button launcher_tab1_patch_resource;
		public EditText edt_cmdline_temp;
		public CheckBox skip_intro;
		public Button launcher_tab1_game_mod_button;
		public RadioGroup rg_harm_r_shadow;
		public RadioGroup rg_opengl;
		public CheckBox cb_s_useOpenAL;
		public CheckBox cb_s_useEAXReverb;
		public Switch readonly_command;
		public CheckBox cb_stencilShadowTranslucent;
		public CheckBox cb_useHighPrecision;
		public CheckBox cb_renderToolsMultithread;
		public Switch editable_temp_command;
		public LinearLayout temp_cmdline;
		public LinearLayout idtech4_section;
		public LinearLayout opengl_section;
		public LinearLayout mod_section;
		public LinearLayout gamemod_section;
		public LinearLayout dll_section;
		// KARIN_NEW_GAME_BOOKMARK: add game RadioGroup view holder
		public RadioGroup rg_fs_q1game;
		public RadioGroup rg_fs_q2game;
		public RadioGroup rg_fs_q3game;
		public RadioGroup rg_fs_rtcwgame;
		public RadioGroup rg_fs_tdmgame;
		public RadioGroup rg_fs_d3bfggame;
		public RadioGroup rg_fs_doomgame;
		public RadioGroup rg_fs_etwgame;
		public RadioGroup rg_fs_realrtcwgame;
		public RadioGroup rg_fs_fteqwgame;
		public RadioGroup rg_fs_jagame;
		public RadioGroup rg_fs_jogame;
		public RadioGroup rg_fs_samtfegame;
		public RadioGroup rg_fs_samtsegame;
		public RadioGroup rg_fs_xash3dgame;
		public RadioGroup rg_fs_sourcegame;
		public RadioGroup rg_fs_urtgame;
		public RadioGroup rg_fs_mohaagame;
		public RadioGroup rg_fs_wolf3dgame;
		public Spinner launcher_tab2_joystick_visible;
		public TextView launcher_fs_game_subdir;
		public CheckBox cb_stencilShadowSoft;
		public EditText edt_harm_r_stencilShadowAlpha;
		//public RadioGroup rg_r_autoAspectRatio;
		public CheckBox cb_stencilShadowCombine;
		public EditText edt_harm_r_shadowMapAlpha;
		public TextView launcher_fs_game_cvar;
		public SeekBar res_scale;
		public RadioGroup rg_scrres;
		public TextView tv_scrres_size;
		public TextView tv_scale_current;
		public LinearLayout res_scale_layout;
		public Button launcher_tab1_command_record;
		public Button launcher_tab1_create_shortcut;
		public CheckBox image_useetc2;
		public EditText edt_harm_r_specularExponentBlinnPhong;
		public EditText edt_harm_r_specularExponentPBR;
		public View show_directory_helper;
		public CheckBox cb_perforatedShadow;
		public Switch collapse_mods;
		public android.support.v4.widget.NestedScrollView mods_container;
		public LinearLayout mods_container_layout;
		public EditText edt_harm_r_ambientLightingBrightness;
		public LinearLayout yquake2_section;
		public RadioGroup yquake2_vid_renderer;
		public LinearLayout doom3bfg_section;
		public RadioGroup doom3bfg_useCompression;
		public CheckBox doom3bfg_useCompressionCache;
		public CheckBox doom3bfg_useMediumPrecision;
		public LinearLayout realrtcw_section;
		public CheckBox realrtcw_sv_cheats;
		public RadioGroup realrtcw_shadows;
		public CheckBox realrtcw_stencilShadowPersonal;
		public LinearLayout etw_section;
		public CheckBox etw_omnibot_enable;
		public RadioGroup etw_shadows;
		public CheckBox etw_stencilShadowPersonal;
		public CheckBox     etw_ui_disableAndroidMacro;
		public LinearLayout zdoom_section;
		public CheckBox zdoom_load_lights_pk3;
		public CheckBox   zdoom_load_brightmaps_pk3;
		public Button     zdoom_choose_extras_file;
		public RadioGroup zdoom_vid_preferbackend;
		public RadioGroup   zdoom_gl_es;
		public RadioGroup   zdoom_gl_version;
		public LinearLayout tdm_section;
		public CheckBox tdm_useMediumPrecision;
		public LinearLayout fteqw_section;
		public RadioGroup fteqw_vid_renderer;
		public SeekBar consoleHeightFracValue;
		public TextView consoleHeightFracText;
		public RadioGroup rg_depth_bits;
		public RadioGroup rg_version_realrtcw;
		public RadioGroup rg_version_d3bfg;
		public LinearLayout gameversion_section;
		public Divider      gameversion_label;
		public LinearLayout versions_container;
		public Button launcher_tab1_change_game;
		public Button launcher_tab1_open_menu;
		public CheckBox cb_r_occlusionCulling;
		public RadioGroup rg_version_tdm;
		public CheckBox cb_gui_useD3BFGFont;
		public CheckBox cb_shadowMapCombine;
		public android.support.v4.widget.NestedScrollView cmdline_container;
		public CheckBox cb_g_skipHitEffect;
		public CheckBox cb_r_globalIllumination;
		public EditText edt_r_globalIlluminationBrightness;
		public Spinner spinner_r_renderMode;
		public CheckBox cb_g_botEnableBuiltinAssets;
		public EditText button_swipe_release_delay;
		public LinearLayout xash3d_section;
		public RadioGroup xash3d_ref;
		public RadioGroup xash3d_sv_cl;
		public LinearLayout source_section;
		public RadioGroup source_sv_cl;
		public LinearLayout sdl_section;
		public RadioGroup sdl_audio_driver;
		public LinearLayout openal_section;
		public RadioGroup openal_driver;
		public Button launcher_tab1_edit_env;
		public LinearLayout urt_section;
		public EditText urt_bot_autoAdd;
		public EditText urt_bot_level;
		public EditText edt_harm_r_PBRNormalCorrection;
		public CheckBox tab2_hide_joystick_center;
		public CheckBox tab2_disable_mouse_button_motion;
		public EditText ratio_x;
		public EditText ratio_y;
		public LinearLayout res_ratiolayout;
		public CheckBox use_custom_resolution;

		private RadioGroup CreateGameRadioGroup(int[] id)
		{
			LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
			RadioGroup radioGroup = new RadioGroup(mods_container_layout.getContext());
			mods_container_layout.addView(radioGroup, params);
			String game = Q3EGame.Find(id[0]).TYPE;
			radioGroup.setTag(game);
			GameGroups[id[0]] = radioGroup;
			groupRadios.put(game, radioGroup);
			id[0]++;
			return radioGroup;
		}

        public void Setup()
        {
			// KARIN_NEW_GAME_BOOKMARK
			GameGroups = new RadioGroup[Q3EGameConstants.NUM_SUPPORT_GAME];
			int[] gameId = { Q3EGameConstants.GAME_ID_DOOM3 };

			// KARIN_NEW_GAME_BOOKMARK: create game RadioGroup
			mods_container_layout = findViewById(R.id.mods_container_layout);
			rg_fs_game = CreateGameRadioGroup(gameId);
			rg_fs_q4game = CreateGameRadioGroup(gameId);
			rg_fs_preygame = CreateGameRadioGroup(gameId);
			rg_fs_rtcwgame = CreateGameRadioGroup(gameId);
			rg_fs_q3game = CreateGameRadioGroup(gameId);
			rg_fs_q2game = CreateGameRadioGroup(gameId);
			rg_fs_q1game = CreateGameRadioGroup(gameId);
			rg_fs_d3bfggame = CreateGameRadioGroup(gameId);
			rg_fs_tdmgame = CreateGameRadioGroup(gameId);
			rg_fs_doomgame = CreateGameRadioGroup(gameId);
			rg_fs_etwgame = CreateGameRadioGroup(gameId);
			rg_fs_realrtcwgame = CreateGameRadioGroup(gameId);
			rg_fs_fteqwgame = CreateGameRadioGroup(gameId);
			rg_fs_jagame = CreateGameRadioGroup(gameId);
			rg_fs_jogame = CreateGameRadioGroup(gameId);
			rg_fs_samtfegame = CreateGameRadioGroup(gameId);
			rg_fs_samtsegame = CreateGameRadioGroup(gameId);
			rg_fs_xash3dgame = CreateGameRadioGroup(gameId);
			rg_fs_sourcegame = CreateGameRadioGroup(gameId);
			rg_fs_urtgame = CreateGameRadioGroup(gameId);
			rg_fs_mohaagame = CreateGameRadioGroup(gameId);
			rg_fs_wolf3dgame = CreateGameRadioGroup(gameId);

            edt_cmdline = findViewById(R.id.edt_cmdline);
            res_customlayout = findViewById(R.id.res_customlayout);
            nolight = findViewById(R.id.nolight);
            useetc1cache = findViewById(R.id.useetc1cache);
            useetc1 = findViewById(R.id.useetc1);
            usedxt = findViewById(R.id.usedxt);
            r_harmclearvertexbuffer = findViewById(R.id.r_harmclearvertexbuffer);
            rg_msaa = findViewById(R.id.rg_msaa);
            rg_color_bits = findViewById(R.id.rg_color_bits);
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
            launcher_tab1_edit_config = findViewById(R.id.launcher_tab1_edit_config);
            launcher_tab1_game_data_chooser_button = findViewById(R.id.launcher_tab1_game_data_chooser_button);
            rg_curpos = findViewById(R.id.rg_curpos);
            edt_harm_r_specularExponent = findViewById(R.id.edt_harm_r_specularExponent);
            rg_harm_r_lightingModel = findViewById(R.id.rg_harm_r_lightingModel);
            onscreen_button_setting = findViewById(R.id.onscreen_button_setting);
            launcher_tab1_user_game_layout = findViewById(R.id.launcher_tab1_user_game_layout);
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
            multithreading = findViewById(R.id.multithreading);
            rg_s_driver = findViewById(R.id.rg_s_driver);
            launcher_tab2_joystick_unfixed = findViewById(R.id.launcher_tab2_joystick_unfixed);
            setup_onscreen_button_theme = findViewById(R.id.setup_onscreen_button_theme);
			setup_controller = findViewById(R.id.setup_controller);
            using_mouse = findViewById(R.id.using_mouse);
            tv_mprefs = findViewById(R.id.tv_mprefs);
            layout_mouse_device = findViewById(R.id.layout_mouse_device);
            find_dll = findViewById(R.id.find_dll);
			find_dll_desc = findViewById(R.id.find_dll_desc);
			edt_harm_r_maxFps = findViewById(R.id.edt_harm_r_maxFps);
			launcher_tab1_edit_cvar = findViewById(R.id.launcher_tab1_edit_cvar);
			launcher_tab1_patch_resource = findViewById(R.id.launcher_tab1_patch_resource);
			edt_cmdline_temp = findViewById(R.id.edt_cmdline_temp);
			skip_intro = findViewById(R.id.skip_intro);
			launcher_tab1_game_mod_button = findViewById(R.id.launcher_tab1_game_mod_button);
			rg_harm_r_shadow = findViewById(R.id.rg_harm_r_shadow);
			rg_opengl = findViewById(R.id.rg_opengl);
			cb_s_useOpenAL = findViewById(R.id.cb_s_useOpenAL);
			cb_s_useEAXReverb = findViewById(R.id.cb_s_useEAXReverb);
			readonly_command = findViewById(R.id.readonly_command);
			cb_stencilShadowTranslucent = findViewById(R.id.cb_stencilShadowTranslucent);
			cb_useHighPrecision = findViewById(R.id.cb_useHighPrecision);
			cb_renderToolsMultithread = findViewById(R.id.cb_renderToolsMultithread);
			editable_temp_command = findViewById(R.id.editable_temp_command);
			temp_cmdline = findViewById(R.id.temp_cmdline);
			idtech4_section = findViewById(R.id.idtech4_section);
			opengl_section = findViewById(R.id.opengl_section);
			dll_section = findViewById(R.id.dll_section);
			mod_section = findViewById(R.id.mod_section);
			gamemod_section = findViewById(R.id.gamemod_section);
			launcher_tab2_joystick_visible = findViewById(R.id.launcher_tab2_joystick_visible);
			launcher_fs_game_subdir = findViewById(R.id.launcher_fs_game_subdir);
			cb_stencilShadowSoft = findViewById(R.id.cb_stencilShadowSoft);
			edt_harm_r_stencilShadowAlpha = findViewById(R.id.edt_harm_r_stencilShadowAlpha);
			//rg_r_autoAspectRatio = findViewById(R.id.rg_r_autoAspectRatio);
			cb_r_autoAspectRatio = findViewById(R.id.cb_r_autoAspectRatio);
			cb_stencilShadowCombine = findViewById(R.id.cb_stencilShadowCombine);
			edt_harm_r_shadowMapAlpha = findViewById(R.id.edt_harm_r_shadowMapAlpha);
			launcher_fs_game_cvar = findViewById(R.id.launcher_fs_game_cvar);
			res_scale = findViewById(R.id.res_scale);
			rg_scrres = findViewById(R.id.rg_scrres);
			tv_scrres_size = findViewById(R.id.tv_scrres_size);
			tv_scale_current = findViewById(R.id.tv_scale_current);
			res_scale_layout = findViewById(R.id.res_scale_layout);
			launcher_tab1_command_record = findViewById(R.id.launcher_tab1_command_record);
			launcher_tab1_create_shortcut = findViewById(R.id.launcher_tab1_create_shortcut);
			image_useetc2 = findViewById(R.id.image_useetc2);
			edt_harm_r_specularExponentBlinnPhong = findViewById(R.id.edt_harm_r_specularExponentBlinnPhong);
			edt_harm_r_specularExponentPBR = findViewById(R.id.edt_harm_r_specularExponentPBR);
			show_directory_helper = findViewById(R.id.show_directory_helper);
			cb_perforatedShadow = findViewById(R.id.cb_perforatedShadow);
			collapse_mods = findViewById(R.id.collapse_mods);
			mods_container = findViewById(R.id.mods_container);
			edt_harm_r_ambientLightingBrightness = findViewById(R.id.edt_harm_r_ambientLightingBrightness);
			yquake2_section = findViewById(R.id.yquake2_section);
			yquake2_vid_renderer = findViewById(R.id.yquake2_vid_renderer);
			doom3bfg_section = findViewById(R.id.doom3bfg_section);
			doom3bfg_useCompression = findViewById(R.id.doom3bfg_useCompression);
			doom3bfg_useCompressionCache = findViewById(R.id.doom3bfg_useCompressionCache);
			doom3bfg_useMediumPrecision = findViewById(R.id.doom3bfg_useMediumPrecision);
			realrtcw_section = findViewById(R.id.realrtcw_section);
			realrtcw_sv_cheats = findViewById(R.id.realrtcw_sv_cheats);
			realrtcw_shadows = findViewById(R.id.realrtcw_shadows);
			realrtcw_stencilShadowPersonal = findViewById(R.id.realrtcw_stencilShadowPersonal);
			etw_section = findViewById(R.id.etw_section);
			etw_omnibot_enable = findViewById(R.id.etw_omnibot_enable);
			etw_shadows = findViewById(R.id.etw_shadows);
			etw_stencilShadowPersonal = findViewById(R.id.etw_stencilShadowPersonal);
			etw_ui_disableAndroidMacro = findViewById(R.id.etw_ui_disableAndroidMacro);
			zdoom_section = findViewById(R.id.zdoom_section);
			zdoom_load_lights_pk3 = findViewById(R.id.zdoom_load_lights_pk3);
			zdoom_load_brightmaps_pk3 = findViewById(R.id.zdoom_load_brightmaps_pk3);
			zdoom_choose_extras_file = findViewById(R.id.zdoom_choose_extras_file);
			zdoom_vid_preferbackend = findViewById(R.id.zdoom_vid_preferbackend);
			zdoom_gl_es = findViewById(R.id.zdoom_gl_es);
			zdoom_gl_version = findViewById(R.id.zdoom_gl_version);
			tdm_section = findViewById(R.id.tdm_section);
			tdm_useMediumPrecision = findViewById(R.id.tdm_useMediumPrecision);
			fteqw_section = findViewById(R.id.fteqw_section);
			fteqw_vid_renderer = findViewById(R.id.fteqw_vid_renderer);
			consoleHeightFracValue = findViewById(R.id.consoleHeightFracValue);
			consoleHeightFracText = findViewById(R.id.consoleHeightFracText);
			rg_depth_bits = findViewById(R.id.rg_depth_bits);
			rg_version_realrtcw = findViewById(R.id.rg_version_realrtcw);
			rg_version_d3bfg = findViewById(R.id.rg_version_d3bfg);
			gameversion_section = findViewById(R.id.gameversion_section);
			gameversion_label = findViewById(R.id.gameversion_label);
			versions_container = findViewById(R.id.versions_container);
			launcher_tab1_change_game = findViewById(R.id.launcher_tab1_change_game);
			launcher_tab1_open_menu = findViewById(R.id.launcher_tab1_open_menu);
			cb_r_occlusionCulling = findViewById(R.id.cb_r_occlusionCulling);
			rg_version_tdm = findViewById(R.id.rg_version_tdm);
			cb_gui_useD3BFGFont = findViewById(R.id.cb_gui_useD3BFGFont);
			cb_shadowMapCombine = findViewById(R.id.cb_shadowMapCombine);
			cmdline_container = findViewById(R.id.cmdline_container);
			cb_g_skipHitEffect = findViewById(R.id.cb_g_skipHitEffect);
			cb_r_globalIllumination = findViewById(R.id.cb_r_globalIllumination);
			edt_r_globalIlluminationBrightness = findViewById(R.id.edt_r_globalIlluminationBrightness);
			spinner_r_renderMode = findViewById(R.id.spinner_r_renderMode);
			cb_g_botEnableBuiltinAssets = findViewById(R.id.cb_g_botEnableBuiltinAssets);
			button_swipe_release_delay = findViewById(R.id.button_swipe_release_delay);
			xash3d_section = findViewById(R.id.xash3d_section);
			xash3d_ref = findViewById(R.id.xash3d_ref);
			xash3d_sv_cl = findViewById(R.id.xash3d_sv_cl);
			source_section = findViewById(R.id.source_section);
			source_sv_cl = findViewById(R.id.source_sv_cl);
			sdl_section = findViewById(R.id.sdl_section);
			sdl_audio_driver = findViewById(R.id.sdl_audio_driver);
			openal_section = findViewById(R.id.openal_section);
			openal_driver = findViewById(R.id.openal_driver);
			launcher_tab1_edit_env = findViewById(R.id.launcher_tab1_edit_env);
			urt_section = findViewById(R.id.urt_section);
			urt_bot_autoAdd = findViewById(R.id.urt_bot_autoAdd);
			urt_bot_level = findViewById(R.id.urt_bot_level);
			edt_harm_r_PBRNormalCorrection = findViewById(R.id.edt_harm_r_PBRNormalCorrection);
			tab2_hide_joystick_center = findViewById(R.id.tab2_hide_joystick_center);
			tab2_disable_mouse_button_motion = findViewById(R.id.tab2_disable_mouse_button_motion);
			ratio_x = findViewById(R.id.ratio_x);
			ratio_y = findViewById(R.id.ratio_y);
			res_ratiolayout = findViewById(R.id.res_ratiolayout);
			use_custom_resolution = findViewById(R.id.use_custom_resolution);
        }
    }
}
