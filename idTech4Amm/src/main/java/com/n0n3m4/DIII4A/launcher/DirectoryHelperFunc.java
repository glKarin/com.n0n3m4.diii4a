package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.View;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.KCVarSystem;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.Game;
import com.karin.idTech4Amm.sys.GameManager;
import com.karin.idTech4Amm.ui.CVarListView;
import com.karin.idTech4Amm.ui.PathTipsListView;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EContextUtils;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.util.ArrayList;
import java.util.List;

public final class DirectoryHelperFunc extends GameLauncherFunc
{
    public DirectoryHelperFunc(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        run();
    }

    public void run()
    {
        PathTipsListView pathTipsListView = new PathTipsListView(m_gameLauncher, GetHelperText());
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.current_game_data_path);
        builder.setView(pathTipsListView);
        builder.setPositiveButton(R.string.create_folders, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Callback();
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
                        pathTipsListView.ExpandAll();
                    }
                });

                dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        pathTipsListView.CollapseAll();
                    }
                });
            }
        });

        dialog.show();
    }

    public List<PathTipsListView.PathTips> GetHelperText()
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        final String DataDir = preferences.getString(Q3EPreference.pref_datapath, "");
        final boolean Standalone = preferences.getBoolean(Q3EPreference.GAME_STANDALONE_DIRECTORY, true);

        List<PathTipsListView.PathTips> list = new ArrayList<>();
        Game[] values = Game.values();
        for (String game : GameManager.Games(false))
        {
            String gameName = Tr(GameManager.GetGameNameTs(game));
            List<PathTipsListView.ModPathTips> mods = new ArrayList<>();

            for (Game value : values)
            {
                if(!value.type.equals(game))
                    continue;

                String modName = value.GetName(m_gameLauncher);
                List<String> paths = new ArrayList<>();

                String subdir = GetSubDir(value.type, Standalone);
                String gameDataDir = value.file;
                String path = KStr.AppendPath(DataDir, subdir, gameDataDir);

                paths.add(path);

                if(Q3EInterface.IsSupportSecondaryDirGame(game))
                {
                    String appHome = Q3EContextUtils.GetAppInternalSearchPath(m_gameLauncher, null);
                    String path2 = KStr.AppendPath(appHome, subdir, gameDataDir);

                    if(!path2.equals(path))
                        paths.add(path2);
                }

                mods.add(new PathTipsListView.ModPathTips(modName, paths));
            }

            list.add(new PathTipsListView.PathTips(gameName, mods));
        }
        return list;
    }

    private String GetSubDir(String game, boolean standalone)
    {
        if(standalone || Q3EInterface.IsStandaloneGame(game))
            return Q3EInterface.GetGameStandaloneDirectory(game);
        else
            return null;
    }
}
