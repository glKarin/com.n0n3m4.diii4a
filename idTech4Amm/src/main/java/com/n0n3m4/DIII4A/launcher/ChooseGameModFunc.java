package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.karin.idTech4Amm.misc.Function;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EGlobals;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class ChooseGameModFunc extends GameLauncherFunc
{
    private final int m_code;
    private String m_path;
    private String m_mod;

    public ChooseGameModFunc(GameLauncher gameLauncher, int code)
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

        m_path = data.getString("path");
        m_mod = data.getString("mod");

        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.load_game_mod_list)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;

        run();
    }

    public void run()
    {
        FileBrowser fileBrowser = new FileBrowser();
        fileBrowser.SetFilter(FileBrowser.ID_FILTER_DIRECTORY);
        fileBrowser.SetIgnoreDotDot(true);
        fileBrowser.SetDirNameWithSeparator(false);
        fileBrowser.SetShowHidden(true);
        fileBrowser.SetCurrentPath(m_path);

        final List<CharSequence> items = new ArrayList<>();
        Map<String, String> map = new HashMap<>();
        final List<String> values = new ArrayList<>();
        for (FileBrowser.FileModel fileModel : fileBrowser.FileList())
        {
            String name = "";
            if (Q3EUtils.q3ei.isQ4)
            {
                if(Q3EGlobals.GAME_BASE_DOOM3.equals(fileModel.name) || Q3EGlobals.GAME_BASE_PREY.equals(fileModel.name))
                    continue;
                if(Q3EGlobals.GAME_BASE_QUAKE4.equals(fileModel.name))
                    name = Q3EGlobals.GAME_NAME_QUAKE4;
            }
            else if(Q3EUtils.q3ei.isPrey)
            {
                if(Q3EGlobals.GAME_BASE_DOOM3.equals(fileModel.name) || Q3EGlobals.GAME_BASE_QUAKE4.equals(fileModel.name))
                    continue;
                if(Q3EGlobals.GAME_BASE_PREY.equals(fileModel.name))
                    name = Q3EGlobals.GAME_NAME_PREY;
            }
            else
            {
                if(Q3EGlobals.GAME_BASE_QUAKE4.equals(fileModel.name) || Q3EGlobals.GAME_BASE_PREY.equals(fileModel.name))
                    continue;
                if(Q3EGlobals.GAME_BASE_DOOM3.equals(fileModel.name))
                    name = Q3EGlobals.GAME_NAME_DOOM3;
            }

            String guessGame = m_gameLauncher.GetGameManager().GetGameOfMod(fileModel.name);
            if(null != guessGame)
            {
                switch (guessGame)
                {
                    case Q3EGlobals.GAME_QUAKE4:
                        if(!Q3EUtils.q3ei.isQ4)
                            continue;
                        break;
                    case Q3EGlobals.GAME_PREY:
                        if(!Q3EUtils.q3ei.isPrey)
                            continue;
                        break;
                    case Q3EGlobals.GAME_DOOM3:
                        if(Q3EUtils.q3ei.isQ4 || Q3EUtils.q3ei.isPrey)
                            continue;
                        break;
                }
            }

            String desc = FileUtility.file_get_contents(fileModel.path + File.separator + "description.txt");
            if(null != desc)
            {
                desc = desc.trim();
                if(!desc.isEmpty())
                    name = desc + " (" + fileModel.name + ")";
            }
            if(name.isEmpty())
                name = fileModel.name;

            File dir = new File(fileModel.path);
            name += "\n " + FileUtility.FormatSize(FileUtility.du(fileModel.path, new Function() {
                @Override
                public Object Invoke(Object... args)
                {
                    File f = (File)args[0];
                    String relativePath = FileUtility.RelativePath(dir, f);
                    if(f.isDirectory())
                    {
                        return !"/savegames".equalsIgnoreCase(relativePath);
                    }
                    else
                    {
                        return !"/.console_history.dat".equalsIgnoreCase(relativePath);
                    }
                }
            }));

            map.put(fileModel.name, name);
            values.add(fileModel.name);
        }

        Collections.sort(values, new Comparator<String>() {
            @Override
            public int compare(String a, String b)
            {
                if(Q3EGlobals.GAME_BASE_DOOM3.equals(a) || Q3EGlobals.GAME_BASE_QUAKE4.equals(a) || Q3EGlobals.GAME_BASE_PREY.equals(a))
                    return -1;
                if(Q3EGlobals.GAME_BASE_DOOM3.equals(b) || Q3EGlobals.GAME_BASE_QUAKE4.equals(b) || Q3EGlobals.GAME_BASE_PREY.equals(b))
                    return 1;
                return a.compareTo(b);
            }
        });

        for (String value : values)
        {
            items.add(map.get(value));
        }

        int selected = -1;
        if(null != m_mod && !m_mod.isEmpty())
        {
            for (int i = 0; i < values.size(); i++)
            {
                if(values.get(i).equals(m_mod))
                {
                    selected = i;
                    break;
                }
            }
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(Q3EUtils.q3ei.game_name + " " + Q3ELang.tr(m_gameLauncher, R.string.mod));
        builder.setSingleChoiceItems(items.toArray(new CharSequence[0]), selected, new DialogInterface.OnClickListener(){
            public void onClick(DialogInterface dialog, int p)
            {
                String lib = values.get(p);
                Callback(lib);
                dialog.dismiss();
            }
        });
        builder.setNegativeButton(R.string.unset, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                Callback("");
                dialog.dismiss();
            }
        });
        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
