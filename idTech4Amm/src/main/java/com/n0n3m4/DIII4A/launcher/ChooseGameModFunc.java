package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.preference.PreferenceManager;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.karin.idTech4Amm.misc.Function;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EGame;
import com.n0n3m4.q3e.Q3EGameConstants;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.KidTech4Command;
import com.n0n3m4.q3e.karin.KidTechCommand;

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
    private String m_file;

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
        m_file = data.getString("file");

        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Tr(R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Tr(R.string.load_game_mod_list)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;

        run();
    }

    public void run()
    {
        FileBrowser fileBrowser = new FileBrowser();
        final boolean UsingFile = Q3E.q3ei.isDOOM;
        final boolean AllowExtraFiles = false; // Q3E.q3ei.isDOOM;
        fileBrowser.SetFilter(FileBrowser.ID_FILTER_DIRECTORY);
        fileBrowser.SetIgnoreDotDot(true);
        fileBrowser.SetShowHidden(false);
        fileBrowser.SetCurrentPath(m_path);

        final List<CharSequence> items = new ArrayList<>();
        Map<String, String> map = new HashMap<>();
        final List<FileBrowser.FileModel> values = new ArrayList<>();
        final List<String> TotalList = new ArrayList<>();
        for(Q3EGame q3eGame : Q3EGame.values())
        {
            if(KStr.IsEmpty(q3eGame.BASE))
                continue;
            String base;
            if(q3eGame.ID == Q3EGameConstants.GAME_ID_QUAKE1)
                base = Q3EGameConstants.GAME_BASE_QUAKE1_DIR;
            else
                base = q3eGame.BASE;
            TotalList.add(base);
        }
        List<String> blackList = new ArrayList<>();
        boolean standalone = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher).getBoolean(Q3EPreference.GAME_STANDALONE_DIRECTORY, true);
        if(!standalone)
        {
            blackList.addAll(TotalList);
            for(Q3EGame q3eGame : Q3EGame.values())
            {
                blackList.add(q3eGame.DIR);
            }
        }

        Q3EGame q3eGame = Q3E.q3ei.GameInfo();
        if(standalone)
            blackList.add(q3eGame.BASE);
        else
            blackList.remove(q3eGame.BASE);

        String gameHomePath = Q3E.q3ei.GetGameHomeDirectoryPath();
        if(null != gameHomePath)
        {
            int i = gameHomePath.indexOf("/");
            if(i > 0)
                blackList.add(gameHomePath.substring(0, i));
            else
                blackList.add(gameHomePath);
        }

        List<FileBrowser.FileModel> fileModels;
        if(UsingFile)
        {
            fileBrowser.SetDirNameWithSeparator(true);
            fileModels = new ArrayList<>(fileBrowser.FileList());

            if(Q3E.q3ei.isDOOM)
                fileBrowser.SetExtension(".wad", ".ipk3");
            fileBrowser.SetFilter(FileBrowser.ID_FILTER_FILE);
            List<FileBrowser.FileModel> allFiles = fileBrowser.ListAllFiles();
            fileModels.addAll(allFiles);

            fileModels.sort(new FileBrowser.NameComparator());
        }
        else
        {
            fileBrowser.SetDirNameWithSeparator(false);
            fileModels = fileBrowser.FileList();
        }

        for (FileBrowser.FileModel fileModel : fileModels)
        {
            String name = "";
            if(blackList.contains(fileModel.name))
                continue;
            if(q3eGame.BASE.equalsIgnoreCase(fileModel.name))
                name = q3eGame.NAME;

/*            String guessGame = m_gameLauncher.GetGameManager().GetGameOfMod(fileModel.name);
            if(null != guessGame)
            {
                q3eGame = Q3EGame.FindOrNull(guessGame);
                if(null != q3eGame)
                {
                    if(q3eGame.ID == Q3E.q3ei.game_id)
                        continue;
                }
            }*/

            if(!UsingFile)
            {
                String desc = GetDescription(fileModel.path);
                if(KStr.NotBlank(desc))
                    name = desc + " (" + fileModel.name + ")";
            }
            if(name.isEmpty())
                name = fileModel.name;

            // name += "\n " + FormatSize(fileModel.path);

            map.put(fileModel.name, name);
            values.add(fileModel);
        }

        Collections.sort(values, new Comparator<FileBrowser.FileModel>() {
            @Override
            public int compare(FileBrowser.FileModel a, FileBrowser.FileModel b)
            {
                if(TotalList.contains(a.name))
                    return -1;
                if(TotalList.contains(b.name))
                    return 1;
                return a.name.compareTo(b.name);
            }
        });

        for (FileBrowser.FileModel value : values)
        {
            items.add(map.get(value.name));
        }

        int selected = -1;
        String mod = null != m_mod ? KStr.UnEscapeQuotes(m_mod) : null;
        if(KStr.NotEmpty(mod))
        {
            for (int i = 0; i < values.size(); i++)
            {
                if(values.get(i).name.equals(mod))
                {
                    selected = i;
                    break;
                }
            }
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(Q3E.q3ei.game_name + " " + Tr(R.string.mod));
        builder.setSingleChoiceItems(items.toArray(new CharSequence[0]), selected, new DialogInterface.OnClickListener(){
            public void onClick(DialogInterface dialog, int p)
            {
                FileBrowser.FileModel file = values.get(p);
                List<String> efiles = new ArrayList<>();
                String lib = SelectMod(file, efiles);
                Callback(lib);
                dialog.dismiss();
                if(AllowExtraFiles)
                {
                    ChooseExtraFiles(efiles, lib);
                }
            }
        });
        if(AllowExtraFiles)
        {
            builder.setNeutralButton(R.string.files, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int p)
                {
                    ChooseExtraFiles(null, mod);
                }
            });
        }
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

    private String GetDescription(String path)
    {
        String desc = Q3EUtils.file_get_contents(KStr.AppendPath(path, "description.txt"));
        if(null != desc)
        {
            desc = desc.trim();
            return desc;
        }
        else
            return null;
    }

    private String FormatSize(String path)
    {
        File dir = new File(path);
        return FileUtility.FormatSize(FileUtility.du(path, new Function() {
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
    }

    private String SelectMod(FileBrowser.FileModel file, List<String> efiles)
    {
        String lib;
        if(Q3E.q3ei.isDOOM && file.type == FileBrowser.FileModel.ID_FILE_TYPE_DIRECTORY)
        {
            List<FileBrowser.FileModel> fileModels = ChooseExtrasFileFunc.ListZDOOMFiles(file.path);
            List<String> wads = new ArrayList<>();

            for(FileBrowser.FileModel fileModel : fileModels)
            {
                String relativePath = FileUtility.RelativePath(fileModel.path, m_path);
                relativePath = KStr.TrimLeft(relativePath, File.separatorChar);
                if(fileModel.name.toLowerCase().endsWith(".wad"))
                    wads.add(relativePath);
                else
                    efiles.add(relativePath);
            }

            lib = KidTech4Command.JoinValue(wads, true);
        }
        else
        {
            //String relativePath = FileUtility.RelativePath(file.path, m_path);
            lib = KStr.CmdStr(file.name);
        }

        return lib;
    }

    private void ChooseExtraFiles(List<String> selectFiles, String mod)
    {
        ChooseExtrasFileFunc m_chooseExtrasFileFunc = new ChooseExtrasFileFunc(m_gameLauncher, m_code);
        final List<String> files = new ArrayList<>();

        if(KStr.NotBlank(m_file))
        {
            List<String> strings = KidTechCommand.SplitValue(m_file, true);
            files.addAll(strings);
        }
        int m = 0;
        if(null != selectFiles)
        {
            for(String selectFile : selectFiles)
            {
                if(!files.contains(selectFile))
                    files.add(selectFile);
            }
        }

        m_chooseExtrasFileFunc.SetCallback(new Runnable() {
            @Override
            public void run()
            {
                ChooseGameModFunc.this.Callback(m_chooseExtrasFileFunc.<String>GetResult());
            }
        });

        Bundle bundle = new Bundle();
        bundle.putString("mod", mod);
        bundle.putString("path", m_path);
        if(Q3E.q3ei.isDOOM)
            bundle.putString("file", KidTechCommand.JoinValue(files, true));
        m_chooseExtrasFileFunc.Start(bundle);
    }
}
