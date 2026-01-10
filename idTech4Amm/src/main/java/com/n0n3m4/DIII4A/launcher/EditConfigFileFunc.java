package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;

import com.karin.idTech4Amm.ConfigEditorActivity;
import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public final class EditConfigFileFunc extends GameLauncherFunc
{
    private final int m_code;
    private String m_path;
    private String m_game;
    private String m_home;
    private String m_base;
    private String[] m_files;
    private String m_file;

    public EditConfigFileFunc(GameLauncher gameLauncher, int code)
    {
        super(gameLauncher);
        m_code = code;
    }

    public void Reset()
    {
        m_file = null;
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        m_path = data.getString("path");
        m_game = data.getString("game");
        m_home = data.getString("home");
        m_base = data.getString("base");
        m_files = data.getStringArray("files");

        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Tr(R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Tr(R.string.access_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;

        List<String> list = new ArrayList<>();
        for(String file : m_files)
        {
            String str = file.replaceAll("<mod>", m_game);
            str = str.replaceAll("<base>", m_base);
            if(!list.contains(str))
                list.add(str);
        }
        if(KStr.NotEmpty(m_home) && !list.isEmpty())
        {
            List<String> tmpList = new ArrayList<>(list);
            for(String s : tmpList)
            {
                list.add(KStr.AppendPath(m_home, s));
            }
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.edit_config_file);
        builder.setItems(list.toArray(new String[0]), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
                m_file = KStr.AppendPath(m_path, list.get(which));
                dialog.dismiss();
                StartEdit();
            }
        });
        builder.setNegativeButton(R.string.cancel, null);
        builder.create().show();
    }

    public void StartEdit()
    {
        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Tr(R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Tr(R.string.access_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    public void run()
    {
        if(KStr.IsEmpty(m_file))
            return;

        File f = new File(m_file);
        if(!f.isFile() || !f.canWrite() || !f.canRead())
        {
            Toast_long(Tr(R.string.file_can_not_access) + m_file);
            //return;
        }

        Intent intent = new Intent(m_gameLauncher, ConfigEditorActivity.class);
        intent.putExtra("CONST_FILE_PATH", m_file);
        intent.putExtra("CONST_CREATING", true);
        m_gameLauncher.startActivity(intent);
    }
}
