package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.provider.DocumentFile;
import android.view.View;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.lib.FileUtility;
import com.karin.idTech4Amm.misc.FileBrowser;
import com.karin.idTech4Amm.ui.FileBrowserDialog;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.karin.KLog;

import java.io.File;

public final class ChooseGameFolderFunc extends GameLauncherFunc
{
    private final int m_code;
    private final int m_uriCode;
    private String m_path;

    public ChooseGameFolderFunc(GameLauncher gameLauncher, int code, int uriCode, Runnable runnable)
    {
        super(gameLauncher, runnable);
        m_code = code;
        m_uriCode = uriCode;
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        m_path = data.getString("path");

        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Tr(R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Tr(R.string.access_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    public void run()
    {
        FileBrowserDialog dialog = new FileBrowserDialog(m_gameLauncher);
        dialog.SetupUI(Tr(R.string.choose_data_folder));
        FileBrowserDialog.FileBrowserCallback callback = new FileBrowserDialog.FileBrowserCallback()
        {
            @Override
            public void Check(String path)
            {
                ContextUtility.GrantUriPermission(m_gameLauncher, path, m_uriCode);
            }
        };

        dialog.SetCallback(callback);

        String gamePath = m_path;

        String defaultPath = Environment.getExternalStorageDirectory().getAbsolutePath(); //System.getProperty("user.home");
        if(null == gamePath || gamePath.isEmpty())
            gamePath = defaultPath;

        FileBrowser fileBrowser = dialog.GetFileBrowser();
        int checked = fileBrowser.GetFileType(gamePath);
        switch (checked)
        {
            case FileBrowser.FileModel.ID_FILE_TYPE_FILE:
                gamePath = FileUtility.ParentPath(gamePath);
                break;
            case FileBrowser.FileModel.ID_FILE_TYPE_NOT_EXISTS:
                gamePath = defaultPath;
                break;
        }

        KLog.i("Launcher", "Default data directory: " + GameLauncher.default_gamedata);
        dialog.SetPath(gamePath);
        dialog.setButton(AlertDialog.BUTTON_NEGATIVE, Tr(R.string.cancel), new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        });
        dialog.setButton(AlertDialog.BUTTON_POSITIVE, Tr(R.string.choose_current_directory), new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                String result = ((FileBrowserDialog) dialog).Path();
                SetResult(result);
                dialog.dismiss();
                Callback();
            }
        });

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
        {
            dialog.setButton(AlertDialog.BUTTON_NEUTRAL, Tr(R.string._default), (AlertDialog.OnClickListener)null);
            dialog.create();
            dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    dialog.SetPath(GameLauncher.default_gamedata);
                }
            });
        }
        else
        {
            dialog.setButton(AlertDialog.BUTTON_NEUTRAL, Tr(R.string.reset), new AlertDialog.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    SetResult(GameLauncher.default_gamedata);
                    dialog.dismiss();
                    Callback();
                }
            });
        }

        dialog.show();
    }
}
