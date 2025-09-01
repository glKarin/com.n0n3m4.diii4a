package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.View;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.KCVarSystem;
import com.karin.idTech4Amm.sys.Game;
import com.karin.idTech4Amm.sys.GameManager;
import com.karin.idTech4Amm.ui.CVarListView;
import com.karin.idTech4Amm.ui.PathTipsListView;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.util.ArrayList;
import java.util.List;

public final class CVarHelperFunc extends GameLauncherFunc
{
    public CVarHelperFunc(GameLauncher gameLauncher)
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
        CVarListView cvarListView = new CVarListView(m_gameLauncher, new ArrayList<>(KCVarSystem.CVars().values()));
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.special_cvar_list);
        builder.setView(cvarListView);
        builder.setPositiveButton(R.string.ok, new AlertDialog.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
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
                        cvarListView.ExpandAll();
                    }
                });

                dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        cvarListView.CollapseAll();
                    }
                });
            }
        });

        dialog.show();
    }
}
