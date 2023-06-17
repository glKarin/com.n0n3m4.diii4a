package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.network.CheckUpdate;
import com.karin.idTech4Amm.sys.Constants;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;

public final class CheckForUpdateFunc extends GameLauncherFunc
{
    private ProgressDialog m_progressDialog = null;
    private HandlerThread m_handlerThread = null;
    private Handler m_handler = null;
    private CheckUpdate m_checkUpdate;

    public CheckForUpdateFunc(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    public void Reset()
    {
        if(null != m_handler)
        {
            m_handler = null;
        }
        if(null != m_handlerThread)
        {
            m_handlerThread.quit();
            m_handlerThread = null;
        }
        if(null != m_progressDialog)
        {
            m_progressDialog.dismiss();
            m_progressDialog = null;
        }
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        if(null != m_handlerThread || null != m_handler || null != m_progressDialog)
            return;
        ProgressDialog dialog = new ProgressDialog(m_gameLauncher);
        dialog.setTitle(R.string.check_for_update);
        dialog.setMessage(Q3ELang.tr(m_gameLauncher, R.string.network_for_github));
        dialog.setCancelable(false);
        dialog.setButton(DialogInterface.BUTTON_NEGATIVE, Q3ELang.tr(m_gameLauncher, R.string.cancel), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.cancel();
            }
        });
        dialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                m_progressDialog = null;
                Reset();
            }
        });
        dialog.show();
        m_progressDialog = dialog;
        m_handlerThread = new HandlerThread("CHECK_FOR_UPDATE");
        m_handlerThread.start();
        m_handler = new Handler(m_handlerThread.getLooper());
        m_handler.post(new Runnable() {
            @Override
            public void run() {
                CheckForUpdate();
            }
        });
    }

    // non-main thread
    private void CheckForUpdate()
    {
        if(null == m_checkUpdate)
            m_checkUpdate = new CheckUpdate();

        m_checkUpdate.CheckForUpdate();
        m_gameLauncher.runOnUiThread(this);
    }

    public void run()
    {
        Reset();
        if(null == m_checkUpdate)
            return;

        int release = m_checkUpdate.release;
        String version = m_checkUpdate.version;
        String update = m_checkUpdate.update;
        String apk_url = m_checkUpdate.apk_url;
        String changes = m_checkUpdate.release < 0 ? m_checkUpdate.error : m_checkUpdate.changes;
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        if(release <= 0)
        {
            builder.setTitle(R.string.error)
                    .setMessage(changes)
                    .setNegativeButton(R.string.close, null)
            ;
        }
        else if(release > Constants.CONST_UPDATE_RELEASE)
        {
            StringBuilder sb = new StringBuilder();
            final String endl = TextHelper.GetDialogMessageEndl();
            sb.append(Q3ELang.tr(m_gameLauncher, R.string.version_)).append(version).append(endl);
            sb.append(Q3ELang.tr(m_gameLauncher, R.string.update_)).append(update).append(endl);
            sb.append(Q3ELang.tr(m_gameLauncher, R.string.changes_)).append(endl);
            if(null != changes && !changes.isEmpty())
            {
                String[] changesArr = changes.split("\n");
                for(String str : changesArr)
                {
                    if(null != str)
                        sb.append(str);
                    sb.append(endl);
                }
            }
            CharSequence msg = TextHelper.GetDialogMessage(sb.toString());
            builder.setTitle(Q3ELang.tr(m_gameLauncher, R.string.new_update_release) + "(" + release + ")")
                    .setMessage(msg)
                    .setPositiveButton(R.string.download, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            ContextUtility.OpenUrlExternally(m_gameLauncher, apk_url);
                        }
                    })
                    .setNeutralButton("Github", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            ContextUtility.OpenUrlExternally(m_gameLauncher, Constants.CONST_MAIN_PAGE);
                        }
                    })
                    .setNegativeButton("F-Droid", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            ContextUtility.OpenUrlExternally(m_gameLauncher, Constants.CONST_FDROID);
                        }
                    })
            ;
        }
        else
        {
            builder.setTitle(R.string.no_update_release)
                    .setMessage(R.string.current_version_is_newest)
                    .setPositiveButton(R.string.ok, null)
            ;
        }
        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
