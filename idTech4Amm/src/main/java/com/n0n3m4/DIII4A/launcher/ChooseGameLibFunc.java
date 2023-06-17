package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EJNI;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;

public final class ChooseGameLibFunc extends GameLauncherFunc
{
    private String m_key;

    public ChooseGameLibFunc(GameLauncher gameLauncher)
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

        m_key = data.getString("key");

        run();
    }

    public void run()
    {
        final SharedPreferences preference = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        final String libPath = ContextUtility.NativeLibDir(m_gameLauncher) + "/";
        final String[] Libs = Q3EUtils.q3ei.libs;
        final String PreferenceKey = m_key;
        final String[] items = new String[Libs.length];
        String lib = preference.getString(PreferenceKey, "");
        int selected = -1;
        for(int i = 0; i < Libs.length; i++)
        {
            items[i] = "lib" + Libs[i] + ".so";
            if((libPath + items[i]).equals(lib))
            {
                selected = i;
            }
        }

        StringBuilder sb = new StringBuilder();
        if(Q3EJNI.IS_64)
            sb.append("armv8-a 64");
        else
            sb.append("armv7-a neon");
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(Q3EUtils.q3ei.game_name + " " + Q3ELang.tr(m_gameLauncher, R.string.game_library) + "(" + sb.toString() + ")");
        builder.setSingleChoiceItems(items, selected, new DialogInterface.OnClickListener(){
            public void onClick(DialogInterface dialog, int p)
            {
                String lib = libPath + items[p];
                SetResult(lib);
                Callback();
                dialog.dismiss();
            }
        });
        builder.setNeutralButton(R.string.unset, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int p)
            {
                SetResult("");
                Callback();
                dialog.dismiss();
            }
        });
        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
