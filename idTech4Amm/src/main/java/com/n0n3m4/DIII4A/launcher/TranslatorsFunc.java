package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.widget.ListView;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.ui.TranslatorAdapter;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;

public final class TranslatorsFunc extends GameLauncherFunc
{
    public TranslatorsFunc(GameLauncher gameLauncher)
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
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(Q3ELang.tr(m_gameLauncher, R.string.translators) + "(" + Q3ELang.tr(m_gameLauncher, R.string.sorting_by_add_time) + ")");
        ListView view = new ListView(m_gameLauncher);
        view.setAdapter(new TranslatorAdapter(m_gameLauncher));
        view.setDivider(null);

        builder.setView(view);
        builder.setPositiveButton(R.string.ok, null)
                .setNeutralButton(R.string.about, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                ContextUtility.OpenMessageDialog(m_gameLauncher, m_gameLauncher.getString(R.string.note), TextHelper.GetDialogMessage(
                        "Translations are supplied by volunteers." + TextHelper.GetDialogMessageEndl()
                        + "Thanks for all everyone working." + TextHelper.GetDialogMessageEndl()
                        + "If has some incorrect, ambiguous and sensitive words, you can contact the translator by clicking name."
                ));
            }
        });
        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
