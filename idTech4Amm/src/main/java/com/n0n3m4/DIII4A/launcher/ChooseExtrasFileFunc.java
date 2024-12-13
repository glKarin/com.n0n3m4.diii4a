package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.Utility;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;
import com.n0n3m4.q3e.karin.KidTechCommand;

import java.util.ArrayList;
import java.util.List;

public final class ChooseExtrasFileFunc extends GameLauncherFunc
{
    public static final String FILE_SEP = " /// ";

    private final int m_code;
    private String m_path;
    private String m_mod;
    private String m_file;

    public ChooseExtrasFileFunc(GameLauncher gameLauncher, int code)
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
            Toast_long(Tr(R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Tr(R.string.access_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    public void run()
    {
        List<FileBrowser.FileModel> fileModels = ListGZDOOMFiles(m_path);

        final List<CharSequence> items = new ArrayList<>();
        final List<String> files = new ArrayList<>();
        int m = 0;

        // 1. remove -iwad file
        if(KStr.NotEmpty(m_mod))
        {
            List<String> iwads = KidTechCommand.SplitValue(m_mod, true);
            while (m < fileModels.size())
            {
                if(Utility.Contains(iwads, fileModels.get(m).name, false))
                    fileModels.remove(m);
                else
                    m++;
            }
        }

        // 2. setup multi choice items
        for (FileBrowser.FileModel fileModel : fileModels)
        {
            items.add(fileModel.name);
        }

        // 3. load selected from command line
        if(KStr.NotBlank(m_file))
        {
            List<String> strings = KidTechCommand.SplitValue(m_file, true);
            files.addAll(strings);
        }

        // 4. remove not exists files from command line
        m = 0;
        while (m < files.size())
        {
            if(!items.contains(files.get(m)))
                files.remove(m);
            else
                m++;
        }

        // 5. setup selected items
        final boolean[] selected = new boolean[fileModels.size()];
        for (int i = 0; i < fileModels.size(); i++)
        {
            FileBrowser.FileModel fileModel = fileModels.get(i);
            selected[i] = files.contains(fileModel.name);
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(Q3EUtils.q3ei.game_name + " " + Tr(R.string.mod) + ": " + Tr(R.string._files));
        builder.setMultiChoiceItems(items.toArray(new CharSequence[0]), selected, new DialogInterface.OnMultiChoiceClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which, boolean isChecked)
            {
                String lib = items.get(which).toString();
                if(isChecked)
                {
                    if(!files.contains(lib))
                        files.add(lib);
                }
                else
                    files.remove(lib);
            }
        });
        builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                String join = KStr.Join(files, FILE_SEP);
                Callback(":" + join);
                dialog.dismiss();
            }
        });
        builder.setNeutralButton(R.string.unset, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                Callback(":");
                dialog.dismiss();
            }
        });
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                dialog.dismiss();
            }
        });
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    public static List<FileBrowser.FileModel> ListGZDOOMFiles(String path)
    {
        FileBrowser fileBrowser = new FileBrowser();
        fileBrowser.SetExtension(".wad", ".pk3", ".ipk3", ".deh", ".bex");
        fileBrowser.SetFilter(FileBrowser.ID_FILTER_FILE);

        fileBrowser.SetIgnoreDotDot(true);
        fileBrowser.SetDirNameWithSeparator(false);
        fileBrowser.SetShowHidden(true);
        fileBrowser.SetCurrentPath(path);
        return fileBrowser.ListAllFiles();
    }
}
