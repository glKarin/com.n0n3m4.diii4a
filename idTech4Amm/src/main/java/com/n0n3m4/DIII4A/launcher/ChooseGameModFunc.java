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
import com.n0n3m4.q3e.Q3EGameConstants;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.KidTech4Command;
import com.n0n3m4.q3e.karin.KidTechCommand;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
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
        final boolean UsingFile = Q3EUtils.q3ei.isDOOM;
        final boolean AllowExtraFiles = false; // Q3EUtils.q3ei.isDOOM;
        fileBrowser.SetFilter(FileBrowser.ID_FILTER_DIRECTORY);
        fileBrowser.SetIgnoreDotDot(true);
        fileBrowser.SetShowHidden(false);
        fileBrowser.SetCurrentPath(m_path);

        final List<CharSequence> items = new ArrayList<>();
        Map<String, String> map = new HashMap<>();
        final List<FileBrowser.FileModel> values = new ArrayList<>();
        final List<String> TotalList = new ArrayList<>(Arrays.asList(
                Q3EGameConstants.GAME_BASE_DOOM3,
                Q3EGameConstants.GAME_BASE_QUAKE4,
                Q3EGameConstants.GAME_BASE_PREY,
                Q3EGameConstants.GAME_BASE_QUAKE1_DIR,
                Q3EGameConstants.GAME_BASE_QUAKE2,
                Q3EGameConstants.GAME_BASE_QUAKE3,
                Q3EGameConstants.GAME_BASE_RTCW,
                Q3EGameConstants.GAME_BASE_DOOM3BFG,
                Q3EGameConstants.GAME_BASE_TDM,
                Q3EGameConstants.GAME_BASE_GZDOOM,
                Q3EGameConstants.GAME_BASE_ETW,
                Q3EGameConstants.GAME_BASE_REALRTCW,
                Q3EGameConstants.GAME_BASE_JA,
                Q3EGameConstants.GAME_BASE_JO
        ));
        List<String> blackList = new ArrayList<>();
        boolean standalone = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher).getBoolean(Q3EPreference.GAME_STANDALONE_DIRECTORY, true);
        if(!standalone)
        {
            blackList.addAll(TotalList);
            blackList.addAll(Arrays.asList(
                    Q3EGameConstants.GAME_SUBDIR_DOOM3,
                    Q3EGameConstants.GAME_SUBDIR_QUAKE4,
                    Q3EGameConstants.GAME_SUBDIR_PREY,
                    Q3EGameConstants.GAME_SUBDIR_QUAKE1,
                    Q3EGameConstants.GAME_SUBDIR_QUAKE1,
                    Q3EGameConstants.GAME_SUBDIR_QUAKE2,
                    Q3EGameConstants.GAME_SUBDIR_QUAKE3,
                    Q3EGameConstants.GAME_SUBDIR_RTCW,
                    Q3EGameConstants.GAME_SUBDIR_TDM,
                    Q3EGameConstants.GAME_SUBDIR_GZDOOM,
                    Q3EGameConstants.GAME_SUBDIR_ETW,
                    Q3EGameConstants.GAME_SUBDIR_REALRTCW,
                    Q3EGameConstants.GAME_SUBDIR_FTEQW,
                    Q3EGameConstants.GAME_SUBDIR_JA,
                    Q3EGameConstants.GAME_SUBDIR_JO,
                    Q3EGameConstants.GAME_SUBDIR_SAMTFE,
                    Q3EGameConstants.GAME_SUBDIR_SAMTSE,
                    Q3EGameConstants.GAME_SUBDIR_XASH3D
            ));
        }

        if (Q3EUtils.q3ei.isQ4)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_QUAKE4);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_QUAKE4);
        }
        else if(Q3EUtils.q3ei.isPrey)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_PREY);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_PREY);
        }
        else if(Q3EUtils.q3ei.isQ2)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_QUAKE2);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_QUAKE2);
        }
        else if(Q3EUtils.q3ei.isQ3)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_QUAKE3);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_QUAKE3);
        }
        else if(Q3EUtils.q3ei.isRTCW)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_RTCW);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_RTCW);
        }
        else if(Q3EUtils.q3ei.isETW)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_ETW);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_ETW);
        }
        else if(Q3EUtils.q3ei.isRealRTCW)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_REALRTCW);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_REALRTCW);
        }
        else if(Q3EUtils.q3ei.isFTEQW)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_FTEQW);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_FTEQW);
        }
        else if(Q3EUtils.q3ei.isJA)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_JA);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_JA);
        }
        else if(Q3EUtils.q3ei.isJO)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_JO);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_JO);
        }
        else if(Q3EUtils.q3ei.isSamTFE)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_SAMTFE);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_SAMTFE);
        }
        else if(Q3EUtils.q3ei.isSamTSE)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_SAMTSE);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_SAMTSE);
        }
        else if(Q3EUtils.q3ei.isXash3D)
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_XASH3D);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_XASH3D);
        }
/*        else if(Q3EUtils.q3ei.isTDM)
        {
            blackList.remove(Q3EGameConstants.GAME_BASE_TDM);
        }*/
        else
        {
            if(standalone)
                blackList.add(Q3EGameConstants.GAME_BASE_DOOM3);
            else
                blackList.remove(Q3EGameConstants.GAME_BASE_DOOM3);
        }

        String gameHomePath = Q3EUtils.q3ei.GetGameHomeDirectoryPath();
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

            if(Q3EUtils.q3ei.isDOOM)
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
            if (Q3EUtils.q3ei.isQ4)
            {
                if(Q3EGameConstants.GAME_BASE_QUAKE4.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_QUAKE4;
            }
            else if(Q3EUtils.q3ei.isPrey)
            {
                if(Q3EGameConstants.GAME_BASE_PREY.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_PREY;
            }
            else if(Q3EUtils.q3ei.isQ1)
            {
                if(Q3EGameConstants.GAME_BASE_QUAKE1_DIR.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_QUAKE1;
            }
            else if(Q3EUtils.q3ei.isQ2)
            {
                if(Q3EGameConstants.GAME_BASE_QUAKE2.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_QUAKE2;
            }
            else if(Q3EUtils.q3ei.isQ3)
            {
                if(Q3EGameConstants.GAME_BASE_QUAKE3.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_QUAKE3;
            }
            else if(Q3EUtils.q3ei.isRTCW)
            {
                if(Q3EGameConstants.GAME_BASE_RTCW.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_RTCW;
            }
            else if(Q3EUtils.q3ei.isETW)
            {
                if(Q3EGameConstants.GAME_BASE_ETW.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_ETW;
            }
            else if(Q3EUtils.q3ei.isRealRTCW)
            {
                if(Q3EGameConstants.GAME_BASE_REALRTCW.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_REALRTCW;
            }
            else if(Q3EUtils.q3ei.isFTEQW)
            {
                if(Q3EGameConstants.GAME_BASE_FTEQW.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_FTEQW;
            }
            else if(Q3EUtils.q3ei.isJA)
            {
                if(Q3EGameConstants.GAME_BASE_JA.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_JA;
            }
            else if(Q3EUtils.q3ei.isJO)
            {
                if(Q3EGameConstants.GAME_BASE_JO.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_JO;
            }
            else if(Q3EUtils.q3ei.isSamTFE)
            {
                if(Q3EGameConstants.GAME_BASE_SAMTFE.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_SAMTFE;
            }
            else if(Q3EUtils.q3ei.isSamTSE)
            {
                if(Q3EGameConstants.GAME_BASE_SAMTSE.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_SAMTSE;
            }
            else if(Q3EUtils.q3ei.isXash3D)
            {
                if(Q3EGameConstants.GAME_BASE_XASH3D.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_XASH3D;
            }
/*            else if(Q3EUtils.q3ei.isTDM)
            {
                if(Q3EGameConstants.GAME_BASE_TDM.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_TDM;
            }*/
            else
            {
                if(Q3EGameConstants.GAME_BASE_DOOM3.equals(fileModel.name))
                    name = Q3EGameConstants.GAME_NAME_DOOM3;
            }

            String guessGame = m_gameLauncher.GetGameManager().GetGameOfMod(fileModel.name);
            if(null != guessGame)
            {
                switch (guessGame)
                {
                    case Q3EGameConstants.GAME_QUAKE4:
                        if(!Q3EUtils.q3ei.isQ4)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_PREY:
                        if(!Q3EUtils.q3ei.isPrey)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_QUAKE1:
                        if(!Q3EUtils.q3ei.isQ1)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_QUAKE2:
                        if(!Q3EUtils.q3ei.isQ2)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_QUAKE3:
                        if(!Q3EUtils.q3ei.isQ3)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_RTCW:
                        if(!Q3EUtils.q3ei.isRTCW)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_TDM:
                        if(!Q3EUtils.q3ei.isTDM)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_DOOM3BFG:
                        if(!Q3EUtils.q3ei.isD3BFG)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_ETW:
                        if(!Q3EUtils.q3ei.isETW)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_REALRTCW:
                        if(!Q3EUtils.q3ei.isRealRTCW)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_FTEQW:
                        if(!Q3EUtils.q3ei.isFTEQW)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_JA:
                        if(!Q3EUtils.q3ei.isJA)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_JO:
                        if(!Q3EUtils.q3ei.isJO)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_SAMTFE:
                        if(!Q3EUtils.q3ei.isSamTFE)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_SAMTSE:
                        if(!Q3EUtils.q3ei.isSamTSE)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_XASH3D:
                        if(!Q3EUtils.q3ei.isXash3D)
                            continue;
                        break;
                    case Q3EGameConstants.GAME_DOOM3:
                        if((Q3EUtils.q3ei.isQ4 || Q3EUtils.q3ei.isPrey) && !Q3EUtils.q3ei.isD3)
                            continue;
                        break;
                }
            }

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
        builder.setTitle(Q3EUtils.q3ei.game_name + " " + Tr(R.string.mod));
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
        if(Q3EUtils.q3ei.isDOOM && file.type == FileBrowser.FileModel.ID_FILE_TYPE_DIRECTORY)
        {
            List<FileBrowser.FileModel> fileModels = ChooseExtrasFileFunc.ListGZDOOMFiles(file.path);
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
        if(Q3EUtils.q3ei.isDOOM)
            bundle.putString("file", KidTechCommand.JoinValue(files, true));
        m_chooseExtrasFileFunc.Start(bundle);
    }
}
