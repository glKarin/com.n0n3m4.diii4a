package com.n0n3m4.DIII4A.launcher;

import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceManager;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.misc.PreferenceBackup;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.Game;
import com.karin.idTech4Amm.sys.GameManager;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EGameConstants;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KLog;
import com.n0n3m4.q3e.karin.KStr;

import java.io.File;
import java.io.OutputStream;

public final class CreateGameFolderFunc extends GameLauncherFunc
{
    private final int m_code;

    public CreateGameFolderFunc(GameLauncher gameLauncher, int code)
    {
        super(gameLauncher);
        m_code = code;
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();
        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Tr(R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Tr(R.string.create_game_folders)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    private boolean CreateFolder(String path)
    {
        File dir = new File(path);
        if(dir.exists())
            return false;
        KLog.I("Create game folder: %s", path);
        return Q3EUtils.mkdir(path, true);
    }

    private boolean CreateTipText(String path, String fileGameName, String defaultName)
    {
        File dir = new File(path);
        if(!dir.isDirectory())
            return false;

        String filePath = KStr.AppendPath(path, Tr(R.string.put_game_files_here, fileGameName) + ".txt");
        KLog.I("Create game folder tip file: %s", filePath);
        if(Q3EUtils.file_put_contents(filePath, Tr(R.string.please_put_game_data_files_into_this_folder, "'" + fileGameName + "'")))
            return true;
        filePath = KStr.AppendPath(path, Tr(R.string.put_game_files_here, defaultName) + ".txt");
        KLog.I("Create game folder tip file: %s", filePath);
        return Q3EUtils.file_put_contents(filePath, Tr(R.string.please_put_game_data_files_into_this_folder, defaultName));
    }

    public void run()
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        final String DataDir = preferences.getString(Q3EPreference.pref_datapath, "");
        final boolean Standalone = preferences.getBoolean(Q3EPreference.GAME_STANDALONE_DIRECTORY, true);
        Game[] values = Game.values();
        int i = 0;
        for (String game : GameManager.Games)
        {
            for (Game value : values)
            {
                if(!value.type.equals(game))
                    continue;

                String subdir = GetSubDir(value.type, Standalone);
                String name = value.GetName(m_gameLauncher);
                String fileGameName = name.replaceAll("[)^]", "").replaceAll("[:.\\\\(/]", " ");
                String defName = value.toString();
                String gameDataDir = value.file;
                String path = KStr.AppendPath(DataDir, subdir);

                if(Q3EGameConstants.GAME_GZDOOM.equals(game))
                {
                    if(CreateFolder(path))
                        i++;
                    CreateTipText(path, fileGameName, defName);

                    String appHome = Q3EUtils.GetAppInternalSearchPath(m_gameLauncher, null);
                    String path2 = KStr.AppendPath(appHome, subdir);

                    if(!path2.equals(path))
                    {
                        if(CreateFolder(path2))
                            i++;
                        CreateTipText(path2, fileGameName, defName);
                    }
                }
                else
                {
                    path = KStr.AppendPath(path, gameDataDir);
                    if(CreateFolder(path))
                        i++;
                    CreateTipText(path, fileGameName, defName);

                    if(!Q3EGameConstants.GAME_QUAKE1.equals(game) && !Q3EGameConstants.GAME_FTEQW.equals(game))
                    {
                        String appHome = Q3EUtils.GetAppInternalSearchPath(m_gameLauncher, null);
                        String path2 = KStr.AppendPath(appHome, subdir, gameDataDir);

                        if(!path2.equals(path))
                        {
                            if(CreateFolder(path2))
                                i++;
                            CreateTipText(path2, fileGameName, defName);
                        }
                    }
                }
            }
        }

        Toast_short(R.string.create_game_folders_successful);
    }

    private String GetSubDir(String game, boolean standalone)
    {
        if(standalone || Q3EInterface.IsStandaloneGame(game))
            return Q3EInterface.GetGameStandaloneDirectory(game);
        else
            return null;
    }
}
