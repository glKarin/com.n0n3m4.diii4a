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

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.InputDevice;
import android.view.Surface;
import android.widget.Toast;

import com.n0n3m4.q3e.gl.Q3EGLConstants;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.KidTechCommand;
import com.n0n3m4.q3e.keycode.KeyCodesGeneric;
import com.n0n3m4.q3e.sdl.Q3ESDL;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class Q3EGameHelper
{
    private Activity m_context;

    public Q3EGameHelper()
    {
    }

    public Q3EGameHelper(Activity context)
    {
        this.m_context = context;
    }

    public void SetContext(Activity context)
    {
        this.m_context = context;
    }

    public void ShowMessage(String s)
    {
        Toast.makeText(m_context, s, Toast.LENGTH_LONG).show();
    }

    public void ShowMessage(int resId)
    {
        Toast.makeText(m_context, resId, Toast.LENGTH_LONG).show();
    }

    public int CheckDevices()
    {
        int[] deviceIds = InputDevice.getDeviceIds();
        KLog.I("Support devices: " + deviceIds.length);
        int mask = 0;
        for(int i = 0; i < deviceIds.length; i++)
        {
            int deviceId = deviceIds[i];
            InputDevice device = InputDevice.getDevice(deviceId);

            if(null == device)
                continue;

            int sources = device.getSources();
            if ((sources & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE)
                mask |= Q3EGlobals.DEVICE_MOUSE;

            if ((sources & InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN)
                mask |= Q3EGlobals.DEVICE_TOUCHSCREEN;

            if ((sources & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD)
                mask |= Q3EGlobals.DEVICE_KEYBOARD;

            if ((sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK)
                mask |= Q3EGlobals.DEVICE_JOYSTICK;

            if ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD)
                mask |= Q3EGlobals.DEVICE_GAMEPAD;

            if ((sources & InputDevice.SOURCE_TRACKBALL) == InputDevice.SOURCE_TRACKBALL)
                mask |= Q3EGlobals.DEVICE_TRACKBALL;

            if ((sources & InputDevice.SOURCE_TOUCHPAD) == InputDevice.SOURCE_TOUCHPAD)
                mask |= Q3EGlobals.DEVICE_TOUCHPAD;

            final Object[] Sources = {
                    InputDevice.SOURCE_KEYBOARD, "keyboard",
                    InputDevice.SOURCE_DPAD, "DPad",
                    InputDevice.SOURCE_GAMEPAD, "game pad",
                    InputDevice.SOURCE_TOUCHSCREEN, "touch screen",
                    InputDevice.SOURCE_MOUSE, "mouse",
                    InputDevice.SOURCE_STYLUS, "stylus",
                    InputDevice.SOURCE_BLUETOOTH_STYLUS, "Bluetooth stylus",
                    InputDevice.SOURCE_TRACKBALL, "trackball",
                    InputDevice.SOURCE_MOUSE_RELATIVE, "relative mouse",
                    InputDevice.SOURCE_TOUCHPAD, "touch pad",
                    InputDevice.SOURCE_TOUCH_NAVIGATION, "touch navigation",
                    InputDevice.SOURCE_ROTARY_ENCODER, "rotary encoder",
                    InputDevice.SOURCE_JOYSTICK, "joystick",
                    InputDevice.SOURCE_HDMI, "HDMI",
                    InputDevice.SOURCE_ANY, "any",
            };
            List<String> sourceList = new ArrayList<>();
            for(int m = 0; m < Sources.length; m+=2)
            {
                if(Q3EUtils.IncludeBit(sources, (Integer) Sources[m])) sourceList.add((String)Sources[m+1]);
            }

            String source = String.join(", ", sourceList);

            KLog.I("%d: ID=%d, name=%s, type=%s", i, deviceId, device.getName(), source);
        }

        return mask;
    }

    private void LoadControllerKeymap()
    {
        Integer code;
        String fieldName;

        Map<String, Integer> codeMap = Q3EKeyCodes.LoadGamePadButtonCodeMap(m_context);
        for (String button : Q3EKeyCodes.CONTROLLER_BUTTONS) {
            code = codeMap.get(button); // code = generic
            fieldName = Q3EKeyCodes.GetDefaultGamePadButtonFieldName(button);
            if(null == code)
            {
                code = Q3EKeyCodes.GetDefaultGamePadButtonCode(button); // code = generic
            }
            code = Q3EKeyCodes.GetRealKeyCode(code); // code = game
            Q3EKeyCodes.SetKeycodeByName(fieldName, code);
        }
    }

    public boolean checkGameFiles()
    {
        String dataDir = Q3E.q3ei.GetGameDataDirectoryPath(null);
        if (!new File(dataDir).exists())
        {
            ShowMessage(Q3ELang.tr(m_context, R.string.game_files_weren_t_found_put_game_files_to) + dataDir);
            return false;
        }

        return true;
    }

    private int GetSDLAudioDriverID(String sdlAudioDriverName)
    {
        if(KStr.IsEmpty(sdlAudioDriverName))
            return 0;
        int res = Q3EUtils.ArrayIndexOf(Q3EGameConstants.SDL_AUDIO_DRIVER, sdlAudioDriverName, false);
        if(res < 0)
            return 0;
        boolean supportAAudio = Build.VERSION.SDK_INT >= Build.VERSION_CODES.O;
        switch (res)
        {
            case 1:
                return 1;
            case 2:
                return supportAAudio ? 2 : 0;
            default:
                return supportAAudio ? 2 : 1;
        }
    }

    private String GetOpenALDriverNames(String openalAudioDriverName)
    {
        if(KStr.IsEmpty(openalAudioDriverName))
            return null;
        int res = Q3EUtils.ArrayIndexOf(Q3EGameConstants.OPENAL_DRIVER, openalAudioDriverName, false);
        if(res <= 0)
            return null;
        String[] drivers = new String[2];
        if(res == 2)
        {
            drivers[0] = Q3EGameConstants.OPENAL_DRIVER[2];
            drivers[1] = Q3EGameConstants.OPENAL_DRIVER[1];
        }
        else
        {
            drivers[0] = Q3EGameConstants.OPENAL_DRIVER[1];
            drivers[1] = Q3EGameConstants.OPENAL_DRIVER[2];
        }
        return String.join(",", drivers);
    }

    public void InitGlobalEnv(String gameTypeName, String gameCommand)
    {
        KLog.I("Game initial: " + gameTypeName + " -> " + gameCommand);

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);
        if(KStr.IsEmpty(gameTypeName))
            gameTypeName = preferences.getString(Q3EPreference.pref_harm_game, Q3EGameConstants.GAME_DOOM3);
        else
            Q3E.q3ei.ResetGameState();

        if (!Q3E.q3ei.IsInitGame()) // not from GameLauncher::startActivity
        {
            Q3E.q3ei.standalone = preferences.getBoolean(Q3EPreference.GAME_STANDALONE_DIRECTORY, true);

            Q3EKeyCodes.InitD3Keycodes();

            Q3E.q3ei.InitD3();

            Q3E.q3ei.InitDefaultsTable();

            Q3E.q3ei.default_path = Environment.getExternalStorageDirectory() + "/diii4a";

            Q3E.q3ei.SetupGame(gameTypeName);

            //Q3E.q3ei.LoadTypeAndArgTablePreference(m_context);

            Q3E.q3ei.start_temporary_extra_command = Q3E.q3ei.MakeTempBaseCommand(m_context);
        }

        Q3E.q3ei.LoadTypeAndArgTablePreference(m_context);
        Q3E.q3ei.LoadLayoutTablePreference(m_context, ((Q3EMain)m_context).IsPortrait());

        Q3E.q3ei.SetupEngineVersion(m_context);

        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Run " + Q3E.q3ei.game_name);

        Q3E.q3ei.SetupEngineLib(); //k setup engine library here again
        Q3E.function_key_toolbar = preferences.getBoolean(Q3EPreference.pref_harm_function_key_toolbar, true);
        Q3E.builtin_virtual_keyboard = preferences.getBoolean(Q3EPreference.BUILTIN_VIRTUAL_KEYBOARD, false);
        Q3E.joystick_smooth = preferences.getBoolean(Q3EPreference.pref_analog, true);
        Q3EKeyCodes.VOLUME_UP_KEY_CODE = Q3EKeyCodes.GetRealKeyCode(preferences.getInt(Q3EPreference.VOLUME_UP_KEY, KeyCodesGeneric.K_F3));
        Q3EKeyCodes.VOLUME_DOWN_KEY_CODE = Q3EKeyCodes.GetRealKeyCode(preferences.getInt(Q3EPreference.VOLUME_DOWN_KEY, KeyCodesGeneric.K_F2));

        // DOOM 3: Hardscorps and Quake4: hardqore mod template disable smooth joystick
/*        if(Q3E.q3ei.joystick_smooth)
        {
            String game = preferences.getString(Q3E.q3ei.GetGameModPreferenceKey(), "");
            if(Q3E.q3ei.isQ4)
            {
                if("hardqore".equals(game))
                    Q3E.q3ei.joystick_smooth = false;
            }
            else if(Q3E.q3ei.isD3)
            {
                if("hardscorps".equals(game))
                    Q3E.q3ei.joystick_smooth = false;
            }
        }*/

        Q3E.q3ei.SetAppStoragePath(m_context);

        Q3E.q3ei.datadir = preferences.getString(Q3EPreference.pref_datapath, Q3E.q3ei.default_path);
        if (null == Q3E.q3ei.datadir)
            Q3E.q3ei.datadir = Q3E.q3ei.default_path;
        if ((Q3E.q3ei.datadir.length() > 0) && (Q3E.q3ei.datadir.charAt(0) != '/'))//lolwtfisuserdoing?
        {
            Q3E.q3ei.datadir = "/" + Q3E.q3ei.datadir;
            preferences.edit().putString(Q3EPreference.pref_datapath, Q3E.q3ei.datadir).commit();
        }

        final boolean useUserCommand = null != gameCommand;
        String cmd;
        if(useUserCommand)
            cmd = gameCommand;
        else
            cmd = preferences.getString(Q3E.q3ei.GetGameCommandPreferenceKey(), Q3EGameConstants.GAME_EXECUABLE);
        if(null == cmd)
            cmd = Q3EGameConstants.GAME_EXECUABLE;

        if(Q3E.q3ei.IsIdTech4())
        {
            boolean multithread = preferences.getBoolean(Q3EPreference.pref_harm_multithreading, true);
            if(multithread)
            {
                KidTechCommand command = Q3E.q3ei.GetGameCommandEngine(cmd);
                command.SetProp("r_multithread", "1");
                cmd = command.toString();
            }
            int glVersion = preferences.getInt(Q3EPreference.pref_harm_opengl, Q3EGLConstants.GetPreferOpenGLESVersion());
            if(glVersion != 0)
            {
                KidTechCommand command = Q3E.q3ei.GetGameCommandEngine(cmd);
                command.SetProp("harm_r_openglVersion", glVersion == Q3EGLConstants.OPENGLES20 ? "GLES2" : "GLES3.0");
                cmd = command.toString();
            }
        }

        if(!useUserCommand)
        {
            if(preferences.getBoolean(Q3EPreference.pref_harm_find_dll, false))
            {
                if(Q3E.q3ei.IsIdTech4())
                {
                    KidTechCommand command = Q3E.q3ei.GetGameCommandEngine(cmd);
                    String fs_game = command.Prop(Q3E.q3ei.GetGameCommandParm());
                    if(null == fs_game || fs_game.isEmpty())
                    {
                        switch (Q3E.q3ei.game)
                        {
                            case Q3EGameConstants.GAME_PREY:
                                fs_game = Q3EGameConstants.GAME_BASE_PREY;
                                break;
                            case Q3EGameConstants.GAME_QUAKE4:
                                fs_game = Q3EGameConstants.GAME_BASE_QUAKE4;
                                break;
                            case Q3EGameConstants.GAME_DOOM3:
                            default:
                                fs_game = Q3EGameConstants.GAME_BASE_DOOM3;
                                break;
                        }
                    }
                    CleanDLLCachePath(Q3E.q3ei.game);
                    String dll = FindDLL_idTech4(fs_game);
                    if(null != dll)
                        command.SetProp("harm_fs_gameLibPath", dll);
                    cmd = command.toString();
                }
                else if(Q3E.q3ei.isXash3D)
                {
                    KidTechCommand command = Q3E.q3ei.GetGameCommandEngine(cmd);
                    String fs_game = command.Param(Q3E.q3ei.GetGameCommandParm());
                    if(null == fs_game || fs_game.isEmpty())
                    {
                        fs_game = Q3EGameConstants.GAME_BASE_XASH3D;
                    }
                    String dll;
                    Set<String> dlls = new HashSet<>();
                    boolean loaded = false;

                    CleanDLLCachePath(Q3E.q3ei.game);

                    dll = FindDLL_Xash3D(fs_game, "client", command.Param("clientlib"), dlls);
                    if(null != dll)
                    {
                        loaded = true;
                        command.SetParam("clientlib", dll);
                    }

                    dll = FindDLL_Xash3D(fs_game, "menu", command.Param("menulib"), dlls);
                    if(null != dll)
                    {
                        loaded = true;
                        command.SetParam("menulib", dll);
                    }

                    dll = FindDLL_Xash3D(fs_game, "server", command.Param("dll"), dlls);
                    if(null != dll)
                    {
                        loaded = true;
                        command.SetParam("dll", dll);
                    }

                    dll = FindDLL_Xash3D(fs_game, "OTHER", null, dlls);

                    if(loaded)
                        command.SetParam("gamelibdir", dll);

                    cmd = command.toString();
                }
            }

            cmd += " " + Q3E.q3ei.start_temporary_extra_command/* + " +set harm_fs_gameLibDir " + lib_dir*/;
        }

        String binDir = Q3E.q3ei.GetGameDataDirectoryPath(null);
        cmd = binDir + "/" + cmd;
        Q3E.q3ei.cmd = cmd;

        // load controller button keymap
        LoadControllerKeymap();
    }

    private String FindDLL_idTech4(String fs_game)
    {
        String DLLPath = Q3E.q3ei.GetGameDataDirectoryPath(fs_game); // /sdcard/diii4a/<fs_game>
        KLog.I("Find idTech4 dll in " + DLLPath + "......");
        String Suffix = "game" + Q3EGlobals.ARCH + ".so"; // gameaarch64.so(64) / gamearm.so(32)
        String p = KStr.AppendPath(DLLPath, Suffix);
        return Q3E.CopyDLLToCache(p, Q3E.q3ei.game, Suffix);
    }

    public void CleanDLLCachePath(String dir)
    {
        String path = Q3E.GetDLLCachePath(dir);
        KLog.I("Clean external library cache directory: " + path);
        Q3EUtils.rmdir_r(path);
    }

    private String FindDLL_Xash3D(String fs_game, String type, String name, Collection<String> dlls)
    {
        String DLLPath = Q3E.q3ei.GetGameDataDirectoryPath(fs_game);
        DLLPath = KStr.AppendPath(DLLPath, "lib", Q3EGlobals.ARCH_DIR); // /sdcard/diii4a/<fs_game>/lib/<arm64/arm>/
        KLog.I("Find Xash3D dll " + type + " in " + DLLPath + "......");
        String libname;
        String res;
        switch(type)
        {
            case "client":
                libname = KStr.NotEmpty(name) ? name : "client.so";
                res = Q3E.CopyDLLToCache(DLLPath, libname, Q3E.q3ei.game, null);
                if(null != res)
                    dlls.add(KStr.Filename(res));
                break;
            case "server":
                libname = KStr.NotEmpty(name) ? name : "server.so";
                res = Q3E.CopyDLLToCache(DLLPath, libname, Q3E.q3ei.game, null);
                if(null != res)
                    dlls.add(KStr.Filename(res));
                break;
            case "menu":
                libname = KStr.NotEmpty(name) ? name : "menu.so";
                res = Q3E.CopyDLLToCache(DLLPath, libname, Q3E.q3ei.game, null);
                if(null != res)
                    dlls.add(KStr.Filename(res));
                break;
            default:
                res = Q3E.CopyDLLsToCache(DLLPath, Q3E.q3ei.game, dlls.toArray(new String[0]));
                break;
        }
        return res;
    }

    private boolean ExtractCopy(String systemFolderPath, boolean overwrite, String...assetPaths)
    {
        InputStream bis = null;
        FileOutputStream fileoutputstream = null;

        try
        {
            Q3EUtils.mkdir(systemFolderPath, true);

            for (String assetPath : assetPaths)
            {
                bis = m_context.getAssets().open(assetPath);

                String tmpname;
                int index = assetPath.lastIndexOf('/');
                if(index >= 0)
                    tmpname = assetPath.substring(index + 1);
                else
                    tmpname = assetPath;

                String entryName = systemFolderPath + "/" + tmpname;
                File file = new File(entryName);

                if(!overwrite && file.exists())
                    continue;

                fileoutputstream = new FileOutputStream(entryName);
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying " + assetPath + " to " + systemFolderPath);
                Q3EUtils.Copy(fileoutputstream, bis, 4096);
                fileoutputstream.close();
                fileoutputstream = null;
                bis.close();
                bis = null;
            }
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(fileoutputstream);
            Q3EUtils.Close(bis);
        }
    }

    private boolean ExtractCopy(String assetFolderPath, String systemFolderPath, boolean overwrite, String...assetPaths)
    {
        InputStream bis = null;
        FileOutputStream fileoutputstream = null;

        try
        {
            Q3EUtils.mkdir(systemFolderPath, true);

            for (String assetPath : assetPaths)
            {
                String sourcePath = assetFolderPath + "/" + assetPath;
                bis = m_context.getAssets().open(sourcePath);

                String entryName = systemFolderPath + "/" + assetPath;
                File file = new File(entryName);

                if(!overwrite && file.exists())
                    continue;

                Q3EUtils.mkdir(file.getParent(), true);

                fileoutputstream = new FileOutputStream(entryName);
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copying " + sourcePath + " to " + entryName);
                Q3EUtils.Copy(fileoutputstream, bis, 4096);
                fileoutputstream.close();
                fileoutputstream = null;
                bis.close();
                bis = null;
            }
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(fileoutputstream);
            Q3EUtils.Close(bis);
        }
    }

    private boolean ExtractZip(String assetPath, String systemFolderPath, boolean overwrite)
    {
        InputStream bis = null;
        ZipInputStream zipinputstream = null;
        FileOutputStream fileoutputstream = null;

        try
        {
            bis = m_context.getAssets().open(assetPath);
            zipinputstream = new ZipInputStream(bis);

            ZipEntry zipentry;
            Q3EUtils.mkdir(systemFolderPath, true);
            while ((zipentry = zipinputstream.getNextEntry()) != null)
            {
                String tmpname = zipentry.getName();

                String entryName = systemFolderPath + "/" + tmpname;
                entryName = entryName.replace('/', File.separatorChar);
                entryName = entryName.replace('\\', File.separatorChar);
                File file = new File(entryName);

                if (zipentry.isDirectory())
                {
                    if(!file.exists())
                        Q3EUtils.mkdir(entryName, true);
                    continue;
                }
                if(!overwrite && file.exists())
                    continue;

                fileoutputstream = new FileOutputStream(entryName);
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Extracting " + tmpname + " to " + systemFolderPath);
                Q3EUtils.Copy(fileoutputstream, zipinputstream, 4096);
                fileoutputstream.close();
                fileoutputstream = null;
                zipinputstream.closeEntry();
            }
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return false;
        }
        finally
        {
            Q3EUtils.Close(fileoutputstream);
            Q3EUtils.Close(zipinputstream);
            Q3EUtils.Close(bis);
        }
    }

    private boolean CheckExtractResourceOverwrite(String systemVersionPath, String apkVersion, String name)
    {
        boolean overwrite = false;
        KLog.I("Check " + name + " resource file version: " + systemVersionPath + " with version " + apkVersion);

        try
        {
            File versionFile = new File(systemVersionPath);
            if(!versionFile.isFile() || !versionFile.canRead())
            {
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, name + " file version not exists.");
                overwrite = true;
            }
            else
            {
                String version = Q3EUtils.file_get_contents(versionFile);
                if(null != version)
                    version = version.trim();
                if(!apkVersion.equalsIgnoreCase(version))
                {
                    Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, String.format(name + " file version is mismatch: %s != %s.", version, apkVersion));
                    overwrite = true;
                }
            }
            if(overwrite)
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, name + " file will be overwrite.");
            else
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, name + " file will keep exists version.");
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Check " + name + "(" + systemVersionPath + ") version file error.");
        }
        return overwrite;
    }

    private boolean CheckExtractResourceVersion(String systemVersionPath, String apkVersion, String name)
    {
        boolean change = false;

        try
        {
            File versionFile = new File(systemVersionPath);
            if(!versionFile.isFile() || !versionFile.canRead())
            {
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, name + " file engine version not exists.");
                change = true;
            }
            else
            {
                String version = Q3EUtils.file_get_contents(versionFile);
                if(null != version)
                    version = version.trim();
                if(!apkVersion.equalsIgnoreCase(version))
                {
                    Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, String.format(name + " file engine version is mismatch: %s != %s.", version, apkVersion));
                    change = true;
                }
            }
            if(change)
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, name + " engine file will be change.");
            else
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, name + " engine file will keep exists version.");
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Check " + name + "(" + systemVersionPath + ") engine version file error.");
        }
        return change;
    }

    private void DumpExtractResourceVersion(String systemVersionPath, String apkVersion, String name)
    {
        Q3EUtils.mkdir(Q3EUtils.FileDir(systemVersionPath), true);
        Q3EUtils.file_put_contents(systemVersionPath, apkVersion);
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Write " + name + " file version is " + apkVersion);
    }

    public void ExtractTDMGLSLShaderSource()
    {
        Q3EPatchResourceManager manager = new Q3EPatchResourceManager(m_context);
        final String versionFile = KStr.AppendPath(Q3E.q3ei.datadir, Q3EGameConstants.GAME_SUBDIR_TDM, "glslprogs/idtech4amm.version");
        String version = Q3EGameConstants.TDM_GLSL_SHADER_VERSION;
        String name = Q3ELang.tr(m_context, R.string.the_dark_mod_glsl_shader);
        Q3EGameConstants.PatchResource patchResource = Q3EGameConstants.PatchResource.TDM_GLSL_SHADER;

        boolean overwrite = CheckExtractResourceOverwrite(versionFile, version, name);

        if(manager.Fetch(patchResource, overwrite) != null)
        {
            if (overwrite)
            {
                DumpExtractResourceVersion(versionFile, version, name);
            }
        }
        else
            ShowMessage(Q3ELang.tr(m_context, R.string.extract_files_fail, name));
    }

    public void ExtractDOOM3BFGHLSLShaderSource()
    {
        Q3EPatchResourceManager manager = new Q3EPatchResourceManager(m_context);
        final String versionFile = KStr.AppendPath(Q3E.q3ei.datadir, Q3E.q3ei.subdatadir, "base", "renderprogs/idtech4amm.version");
        final String version = Q3EGameConstants.RBDOOM3BFG_HLSL_SHADER_VERSION;
        final String name = Q3ELang.tr(m_context, R.string.rbdoom3_bfg_hlsl_shader);

        boolean overwrite = CheckExtractResourceOverwrite(versionFile, version, name);

        if(manager.Fetch(Q3EGameConstants.PatchResource.DOOM3BFG_HLSL_SHADER, overwrite) != null)
        {
            if (overwrite)
            {
                DumpExtractResourceVersion(versionFile, version, name);
            }
        }
        else
            ShowMessage(Q3ELang.tr(m_context, R.string.extract_files_fail, name));
    }

    public void ExtractZDOOMResource()
    {
        Q3EGameConstants.PatchResource zdoomResource;
        String versionCheckFile;
        //String versionName = "4.14.0";

        versionCheckFile = "idtech4amm.version";
        zdoomResource = Q3EGameConstants.PatchResource.ZDOOM_RESOURCE;

        Q3EPatchResourceManager manager = new Q3EPatchResourceManager(m_context);
        final String versionFile = KStr.AppendPath(Q3E.q3ei.datadir, Q3EGameConstants.GAME_SUBDIR_ZDOOM, versionCheckFile);
        final String version = Q3EGameConstants.ZDOOM_VERSION;
        String name = Q3ELang.tr(m_context, R.string.zdoom_builtin_resource);

        //boolean change = CheckExtractResourceVersion(engineVersionFile, versionName, name);
        boolean overwrite = CheckExtractResourceOverwrite(versionFile, version, name);
        if(manager.Fetch(zdoomResource, overwrite) != null)
        {
            if (overwrite)
            {
                DumpExtractResourceVersion(versionFile, version, name);
            }
        }
        else
            ShowMessage(Q3ELang.tr(m_context, R.string.extract_files_fail, name));
    }

    public void ExtractXash3DResource()
    {
        Q3EGameConstants.PatchResource resource;
        String versionCheckFile;

        versionCheckFile = "idtech4amm.version";
        resource = Q3EGameConstants.PatchResource.XASH3D_EXTRAS;

        Q3EPatchResourceManager manager = new Q3EPatchResourceManager(m_context);
        final String versionFile = KStr.AppendPath(Q3E.q3ei.datadir, Q3EGameConstants.GAME_SUBDIR_XASH3D, versionCheckFile);
        final String version = Q3EGameConstants.XASH3D_VERSION;
        String name = Q3ELang.tr(m_context, R.string.xash3d_extras);

        boolean overwrite = CheckExtractResourceOverwrite(versionFile, version, name);
        if(manager.Fetch(resource, overwrite) != null)
        {
            if (overwrite)
            {
                DumpExtractResourceVersion(versionFile, version, name);
            }
            // CS1.6
            resource = Q3EGameConstants.PatchResource.XASH3D_CS16_EXTRAS;
            name = Q3ELang.tr(m_context, R.string.cs16_xash3d_extras);
            if(manager.Fetch(resource, overwrite) == null)
                ShowMessage(Q3ELang.tr(m_context, R.string.extract_files_fail, name));
        }
        else
            ShowMessage(Q3ELang.tr(m_context, R.string.extract_files_fail, name));
    }

    public void ExtractETWResource()
    {
        Q3EGameConstants.PatchResource resource;
        String versionCheckFile;

        versionCheckFile = "idtech4amm.version";
        resource = Q3EGameConstants.PatchResource.ET_LEGACY_EXTRAS;

        Q3EPatchResourceManager manager = new Q3EPatchResourceManager(m_context);
        final String versionFile = KStr.AppendPath(Q3E.q3ei.datadir, Q3E.q3ei.subdatadir, "legacy", versionCheckFile);
        final String version = Q3EGameConstants.ETW_VERSION;
        String name = Q3ELang.tr(m_context, R.string.etlegacy_extras);

        boolean overwrite = CheckExtractResourceOverwrite(versionFile, version, name);
        if(manager.Fetch(resource, overwrite) != null)
        {
            if (overwrite)
            {
                DumpExtractResourceVersion(versionFile, version, name);
            }
        }
        else
            ShowMessage(Q3ELang.tr(m_context, R.string.extract_files_fail, name));
    }

    public void ExtractECWolfResource()
    {
        Q3EGameConstants.PatchResource resource;
        String versionCheckFile;

        versionCheckFile = "idtech4amm.version";
        resource = Q3EGameConstants.PatchResource.ECWOLF_RESOURCE;

        Q3EPatchResourceManager manager = new Q3EPatchResourceManager(m_context);
        final String versionFile = KStr.AppendPath(Q3E.q3ei.datadir, Q3EGameConstants.GAME_SUBDIR_WOLF3D, versionCheckFile);
        final String version = Q3EGameConstants.WOLF3D_VERSION;
        String name = Q3ELang.tr(m_context, R.string.ecwolf_builtin_resource);

        boolean overwrite = CheckExtractResourceOverwrite(versionFile, version, name);
        if(manager.Fetch(resource, overwrite) != null)
        {
            if (overwrite)
            {
                DumpExtractResourceVersion(versionFile, version, name);
            }
        }
        else
            ShowMessage(Q3ELang.tr(m_context, R.string.extract_files_fail, name));
    }

    public void ExtractSourceEngineResource()
    {
        Q3EGameConstants.PatchResource resource;
        String versionCheckFile;

        versionCheckFile = "idtech4amm.version";
        resource = Q3EGameConstants.PatchResource.SOURCE_ENGINE_EXTRAS;

        Q3EPatchResourceManager manager = new Q3EPatchResourceManager(m_context);
        final String versionFile = KStr.AppendPath(Q3E.q3ei.datadir, Q3EGameConstants.GAME_SUBDIR_SOURCE, versionCheckFile);
        final String version = Q3EGameConstants.SOURCE_ENGINE_VERSION;
        String name = Q3ELang.tr(m_context, R.string.sourceengine_extras);

        boolean overwrite = CheckExtractResourceOverwrite(versionFile, version, name);
        if(manager.Fetch(resource, overwrite) != null)
        {
            if (overwrite)
            {
                DumpExtractResourceVersion(versionFile, version, name);
            }
        }
        else
            ShowMessage(Q3ELang.tr(m_context, R.string.extract_files_fail, name));
    }

    // KARIN_NEW_GAME_BOOKMARK: add patch resource extract
    public void ExtractGameResource()
    {
        if(Q3E.q3ei.IsTDMTech()) // if game is TDM, extract glsl shader
            ExtractTDMGLSLShaderSource();
        else if(Q3E.q3ei.IsIdTech4BFG()) // if game is D3BFG, extract hlsl shader
            ExtractDOOM3BFGHLSLShaderSource();
        else if(Q3E.q3ei.isDOOM) // pk3
            ExtractZDOOMResource();
        else if(Q3E.q3ei.isXash3D) // extras.pk3
            ExtractXash3DResource();
        else if(Q3E.q3ei.isSource) // fonts
            ExtractSourceEngineResource();
        else if(Q3E.q3ei.isETW)
            ExtractETWResource();
        else if(Q3E.q3ei.isWolf3D)
            ExtractECWolfResource();
    }

    private int GetMSAA()
    {
        int msaa = PreferenceManager.getDefaultSharedPreferences(m_context).getInt(Q3EPreference.pref_msaa, 0);
        switch (msaa)
        {
            case 0: msaa = 0;break;
            case 1: msaa = 4;break;
            case 2: msaa = 16;break;
            case 3: msaa = 2; break;
            case 4: msaa = 8; break;
            case 5: msaa = -1; break;
        }
        return msaa;
    }

    public int GetPixelFormat()
    {
        if (PreferenceManager.getDefaultSharedPreferences(m_context).getBoolean(Q3EPreference.pref_32bit, false))
        {
            return PixelFormat.RGBA_8888;
        }
        else
        {
            //if (Q3E.q3ei.isD3)
            //k setEGLConfigChooser(new Q3EConfigChooser(5, 6, 5, 0, msaa, Q3EUtils.usegles20));
            //k getHolder().setFormat(PixelFormat.RGB_565);

            int harm16Bit = PreferenceManager.getDefaultSharedPreferences(m_context).getInt(Q3EPreference.pref_harm_16bit, 0);
            switch (harm16Bit)
            {
                case 1: // RGBA4444
                    return PixelFormat.RGBA_4444;
                case 2: // RGBA5551
                    return PixelFormat.RGBA_5551;
                case 3: // RGBA10101002
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
                        return PixelFormat.RGBA_1010102;
                    else
                        return PixelFormat.RGBA_8888;
                case 0: // RGB565
                default:
                    return PixelFormat.RGB_565;
            }
        }
    }

    public int GetGLFormat()
    {
        int pixelFormat = GetPixelFormat();
        int glFormat = 0x8888;
        switch (pixelFormat) {
            case PixelFormat.RGBA_4444:
                glFormat = Q3EGlobals.GLFORMAT_RGBA4444;
                break;
            case PixelFormat.RGBA_5551:
                glFormat = Q3EGlobals.GLFORMAT_RGBA5551;
                break;
            case PixelFormat.RGB_565:
                glFormat = Q3EGlobals.GLFORMAT_RGB565;
                break;
            case PixelFormat.RGBA_1010102:
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
                    glFormat = Q3EGlobals.GLFORMAT_RGBA1010102;
                else
                    glFormat = Q3EGlobals.GLFORMAT_RGBA8888;
                break;
            case PixelFormat.RGBA_8888:
                glFormat = Q3EGlobals.GLFORMAT_RGBA8888;
                break;
        }
        return glFormat;
    }

    private int[] GetFrameSize(int w, int h)
    {
        int[] size = Q3EContextUtils.GetSurfaceViewSize(m_context, w, h);
        KLog.i("Q3EView", "Game surface view size: %d x %d", size[0], size[1]);
        return size;
    }

/*
    private int[] GetFrameSize_old(int w, int h)
    {
        int width;
        int height;
        SharedPreferences mPrefs=PreferenceManager.getDefaultSharedPreferences(m_context);
        boolean scaleByScreenArea = mPrefs.getBoolean(Q3EPreference.pref_harm_scale_by_screen_area, false);

        switch (mPrefs.getInt(Q3EPreference.pref_scrres, 0))
        {
            case 0:
                width = w;
                height = h;
                break;
            case 1:
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, new BigDecimal("0.5"));
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w / 2;
                    height = h / 2;
                }
                break;
            case 2:
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, new BigDecimal("2"));
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w * 2;
                    height = h * 2;
                }
                break;
            case 3:
                width = 1920;
                height = 1080;
                break;
            case 4:
                //k width=Integer.parseInt(mPrefs.getString(Q3EUtils.pref_resx, "640"));
                //k height=Integer.parseInt(mPrefs.getString(Q3EUtils.pref_resy, "480"));
                try
                {
                    String str = mPrefs.getString(Q3EPreference.pref_resx, "0");
                    if(null == str)
                        str = "0";
                    width = Integer.parseInt(str);
                }
                catch (Exception e)
                {
                    width = 0;
                }
                try
                {
                    String str = mPrefs.getString(Q3EPreference.pref_resy, "0");
                    if(null == str)
                        str = "0";
                    height = Integer.parseInt(str);
                }
                catch (Exception e)
                {
                    height = 0;
                }
                if (width <= 0 && height <= 0)
                {
                    width = w;
                    height = h;
                }
                if (width <= 0)
                {
                    width = (int)((float)height * (float)w / (float)h);
                }
                else if (height <= 0)
                {
                    height = (int)((float)width * (float)h / (float)w);
                }
                break;

            //k
            case 5: // 720p
                width = 1280;
                height = 720;
                break;
            case 6: // 480p
                width = 720;
                height = 480;
                break;
            case 7: // 360p
                width = 640;
                height = 360;
                break;
            case 8: // 1/3
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, BigDecimal.ONE.divide(new BigDecimal("3"), 2, RoundingMode.HALF_UP));
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w / 3;
                    height = h / 3;
                }
                break;
            case 9: // 1/4
                if(scaleByScreenArea)
                {
                    int[] size = Q3EUtils.CalcSizeByScaleScreenArea(w, h, new BigDecimal("0.25"));
                    width = size[0];
                    height = size[1];
                }
                else
                {
                    width = w / 4;
                    height = h / 4;
                }
                break;
            //k
            default:
                width = w;
                height = h;
                break;
        }
        Log.i("Q3EView", "FrameSize: (" + width + ", " + height + ")");
        return new int[] { width, height };
    }
    */

    private String GetArchName()
    {
        return Q3EJNI.Is64() ? "arm64" : "arm";
    }

    private String GetExternalLibPath()
    {
        String arch = GetArchName();
        return m_context.getFilesDir() + File.separator + "lib" + File.separator + arch;
    }

    private String GetDefaultLibrariesPath()
    {
        return Q3EContextUtils.GetGameLibDir(m_context);
    }

    private String GetExternalLocalLibPath()
    {
        String arch = GetArchName();
        return m_context.getCacheDir() + File.separator + "lib" + File.separator + arch;
    }

    private String CopyLocalLibraries(String def)
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);
        if(!preferences.getBoolean(Q3EPreference.USE_EXTERNAL_LIB_PATH, false))
            return def;

        String targetPath = GetExternalLocalLibPath();

        // clean old libraries
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Clean external libraries: " + targetPath);
        File dir = new File(targetPath);
        Q3EUtils.rmdir_r(dir);

        String arch = GetArchName();
        String localPath = Q3E.GetDataPath(File.separator + "lib" + File.separator + arch);
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Find local external libraries: " + localPath);
        File file = new File(localPath);
        if(!file.isDirectory())
            return targetPath;

        File[] files = file.listFiles();
        if(null == files || files.length == 0)
            return targetPath;

        List<File> list = new ArrayList<>();
        for(File f : files)
        {
            if(!f.isFile() || !f.canRead())
                continue;
            String name = f.getName();
            if(!name.startsWith("lib") || !name.endsWith(".so"))
                continue;
            Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Find local external library: " + (list.size() + 1) + " -> " + f.getAbsolutePath());
            list.add(f);
        }

        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Get local libraries: " + list.size());

        if(list.isEmpty())
            return targetPath;

        if(!dir.isDirectory())
        {
            Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Create dev external library directory: " + targetPath);
            if(!Q3EUtils.mkdir(targetPath, true))
            {
                Log.e(Q3EGlobals.CONST_Q3E_LOG_TAG, "Create dev external library directory fail: " + targetPath);
                return targetPath;
            }
        }

        for(File f : list)
        {
            String localFilePath = f.getAbsolutePath();
            String targetFilePath = targetPath + File.separator + f.getName();
            if(Q3EUtils.cp(f.getAbsolutePath(), targetFilePath) > 0)
            {
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copy local library to external: " + localFilePath + " -> " + targetFilePath);
                //new File(targetFilePath).deleteOnExit();
            }
            else
            {
                Log.e(Q3EGlobals.CONST_Q3E_LOG_TAG, "Copy local library to external fail: " + localFilePath + " -> " + targetFilePath);
            }
        }
        return targetPath;
    }

    public String GetDefaultEngineLib()
    {
        String libname = Q3E.q3ei.GetEngineLibName();
        return /*Q3EUtils.GetGameLibDir(m_context) + "/" +*/ libname; // Q3E.q3ei.GetEngineLibName();
    }

    public String GetEngineLib(String def)
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);
        String libname = Q3E.q3ei.GetEngineLibName();
        String libPath = def;
        if(preferences.getBoolean(Q3EPreference.LOAD_LOCAL_ENGINE_LIB, false))
        {
            String localLibPath = Q3E.q3ei.GetGameDataDirectoryPath(libname);
            KLog.I("Find local engine library at " + localLibPath + "......");
            String cacheFile = Q3E.CopyDLLToCache(localLibPath, "lib", null);
            if(null != cacheFile)
            {
                libPath = cacheFile;
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Load local engine library: " + cacheFile);
            }
            else
            {
                Log.e(Q3EGlobals.CONST_Q3E_LOG_TAG, "Upload local engine library fail: " + localLibPath);
            }
        }
        else if(preferences.getBoolean(Q3EPreference.USE_EXTERNAL_LIB_PATH, false))
        {
            String cacheFile = GetExternalLocalLibPath() + File.separator + /*Q3E.q3ei.*/libname;
            File file = new File(cacheFile);
            if(file.isFile() && file.canRead())
            {
                libPath = cacheFile;
                Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Load external engine library: " + cacheFile);
            }
        }
        return libPath;
    }

    public boolean Start(Surface surface, int surfaceWidth, int surfaceHeight)
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_context);

        // GL
        int msaa = GetMSAA();
        int glFormat = GetGLFormat();
        int[] size = GetFrameSize(surfaceWidth, surfaceHeight);
        int width = size[0];
        int height = size[1];
        int depthBits = preferences.getInt(Q3EPreference.pref_harm_depth_bit, Q3EGlobals.DEFAULT_DEPTH_BITS);

        // Args
        String cmd = Q3E.q3ei.cmd;
        boolean redirectOutputToFile = preferences.getBoolean(Q3EPreference.REDIRECT_OUTPUT_TO_FILE, true);
        //boolean noHandleSignals = preferences.getBoolean(Q3EPreference.NO_HANDLE_SIGNALS, false);
        int signalsHandler = Q3EPreference.GetIntFromString(m_context, Q3EPreference.SIGNALS_HANDLER, Q3EGlobals.SIGNALS_HANDLER_GAME);
        // final boolean noHandleSignals = signalsHandler != Q3EGlobals.SIGNALS_HANDLER_GAME;
        int runBackground = Q3EUtils.parseInt_s(preferences.getString(Q3EPreference.RUN_BACKGROUND, "0"), 0);
        int glVersion = preferences.getInt(Q3EPreference.pref_harm_opengl, Q3EGLConstants.GetPreferOpenGLESVersion());
        boolean usingMouse = preferences.getBoolean(Q3EPreference.pref_harm_using_mouse, false) && Q3EUtils.SupportMouse() == Q3EGlobals.MOUSE_EVENT;
        boolean useExternalLibPath = preferences.getBoolean(Q3EPreference.USE_EXTERNAL_LIB_PATH, false);
        int consoleMaxHeightFrac = preferences.getInt(Q3EPreference.pref_harm_max_console_height_frac, 0);
        int sdlAudioDriver = GetSDLAudioDriverID(preferences.getString(Q3EPreference.pref_harm_sdl_audio_driver, Q3EGameConstants.SDL_AUDIO_DRIVER[0]));
        String openalDriver = GetOpenALDriverNames(preferences.getString(Q3EPreference.pref_harm_openal_driver, Q3EGameConstants.OPENAL_DRIVER[0]));

        String subdatadir = Q3E.q3ei.subdatadir;

        int refreshRate = (int)Q3EContextUtils.GetRefreshRate(m_context);
        String appHome = Q3EContextUtils.GetAppInternalSearchPath(m_context, null);
		appHome = KStr.AppendPath(appHome, subdatadir);

        int eventQueue = Q3EPreference.GetIntFromString(preferences, Q3EPreference.EVENT_QUEUE, Q3EGlobals.EVENT_QUEUE_TYPE_NATIVE);
        int gameThread = Q3EPreference.GetIntFromString(preferences, Q3EPreference.GAME_THREAD, Q3EGlobals.GAME_THREAD_TYPE_NATIVE);
        int threadStackSize = Q3EPreference.GetIntFromString(preferences, Q3EPreference.GAME_THREAD_STACK_SIZE, 0);
        if(Q3E.q3ei.IsUsingSDL())
            eventQueue = Q3EGlobals.EVENT_QUEUE_TYPE_NATIVE;
        Q3EJNI.PreInit(eventQueue, gameThread, threadStackSize);

        String libpath;
        String engineLib;

        // first: get default engine lib path and libraries path
        libpath = GetDefaultLibrariesPath();
        engineLib = GetDefaultEngineLib();

        // second: find from dev env
        libpath = CopyLocalLibraries(libpath);
        engineLib = GetEngineLib(engineLib);

        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Engine library: " + engineLib);
        Log.i(Q3EGlobals.CONST_Q3E_LOG_TAG, "Game libraries directory: " + libpath);

        Q3E.surfaceWidth = width;
        Q3E.surfaceHeight = height;
        //Q3E.CalcRatio();

        String envPreferenceKey = Q3E.q3ei.GetGameEnvPreferenceKey();
        Set<String> envs = preferences.getStringSet(envPreferenceKey, null);
        if(null != envs)
        {
            for(String env : envs)
            {
                String[] e = KStr.ParseEnv(env);
                if(null != e)
                    Q3EJNI.Setenv(e[0], e[1], 1);
            }
        }

        //Q3EJNI.Setenv("ALSOFT_LOGFILE", "/sdcard/diii4a/openal.log");
        //Q3EJNI.Setenv("ALSOFT_LOGLEVEL", "3");
        if(null != openalDriver)
            Q3EJNI.Setenv("ALSOFT_DRIVERS", openalDriver, 1);

        boolean res = Q3EJNI.init(
                engineLib,
                libpath,
                width,
                height,
                Q3E.q3ei.datadir,
                subdatadir,
                cmd,
                surface,
                glFormat,
                depthBits,
                msaa, glVersion,
                redirectOutputToFile,
                signalsHandler, // noHandleSignals,
                usingMouse,
                refreshRate,
                appHome,
                Q3E.joystick_smooth,
                consoleMaxHeightFrac,
                useExternalLibPath,
                sdlAudioDriver,
                runBackground > 0
        );
        if(res)
        {
            Q3ESDL.usingSDL = Q3ESDL.UsingSDL();
            if(Q3E.q3ei.IsUsingSDL())
                Q3E.controlView.EnableSDL();

            m_context.runOnUiThread(new Runnable() {
                @Override
                public void run()
                {
                    Q3E.activity.MakeMouseCursor(surfaceWidth, surfaceHeight);
                    // Q3EUtils.RunLauncher(activity);
                }
            });

            Q3E.Start();
        }
        else
        {
            m_context.runOnUiThread(new Runnable() {
                @Override
                public void run()
                {
                    m_context.finish();
                    // Q3EUtils.RunLauncher(activity);
                }
            });
        }

        return res;
    }

    public Context GetContext()
    {
        return m_context;
    }
}
