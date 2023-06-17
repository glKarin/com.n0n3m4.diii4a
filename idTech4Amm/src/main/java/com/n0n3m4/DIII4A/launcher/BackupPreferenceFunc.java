package com.n0n3m4.DIII4A.launcher;

import android.annotation.TargetApi;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.widget.Toast;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.misc.PreferenceBackup;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;

import java.io.OutputStream;

public final class BackupPreferenceFunc extends GameLauncherFunc
{
    private final int m_code;

    public BackupPreferenceFunc(GameLauncher gameLauncher, int code)
    {
        super(gameLauncher);
        m_code = code;
    }

    public void Reset()
    {
    }

    private String GenDefaultBackupFileName()
    {
        return m_gameLauncher.getPackageName() + "_preferences_backup.xml";
    }

    public void Start(Bundle data)
    {
        Uri uri = data.getParcelable("uri");
        if(null != uri)
        {
            BackupPreferences(uri);
            return;
        }

        super.Start(data);
        Reset();
        int res = ContextUtility.CheckFilePermission(m_gameLauncher, m_code);
        if(res == ContextUtility.CHECK_PERMISSION_RESULT_REJECT)
            Toast_long(Q3ELang.tr(m_gameLauncher, R.string.can_t_s_read_write_external_storage_permission_is_not_granted, Q3ELang.tr(m_gameLauncher, R.string.save_preferences_file)));
        if(res != ContextUtility.CHECK_PERMISSION_RESULT_GRANTED)
            return;
        run();
    }

    @TargetApi(Build.VERSION_CODES.KITKAT)
    public void run()
    {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        intent.setType("*/*"); // application/xml
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.putExtra(Intent.EXTRA_TITLE, GenDefaultBackupFileName());

        m_gameLauncher.startActivityForResult(intent, m_code);
    }

    private void BackupPreferences(Uri uri)
    {
        try
        {
            OutputStream os = m_gameLauncher.getContentResolver().openOutputStream(uri);
            PreferenceBackup backup = new PreferenceBackup(m_gameLauncher);
            if(backup.Dump(os))
                Toast.makeText(m_gameLauncher, R.string.backup_preferences_file_success, Toast.LENGTH_LONG).show();
            else
            {
                String[] args = {""};
                backup.GetError(args);
                Toast.makeText(m_gameLauncher, Q3ELang.tr(m_gameLauncher, R.string.backup_preferences_file_fail) + args[0], Toast.LENGTH_LONG).show();
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Toast.makeText(m_gameLauncher, R.string.backup_preferences_file_error, Toast.LENGTH_LONG).show();
        }
    }
}
