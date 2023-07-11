package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.ui.cvar.CVarSettingWidget;
import com.n0n3m4.DIII4A.GameLauncher;

public final class CVarEditorFunc extends GameLauncherFunc
{
    private String m_game;

    public CVarEditorFunc(GameLauncher gameLauncher)
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

        m_game = data.getString("game");

        run();
    }

    public void run()
    {
        CVarSettingWidget widget = new CVarSettingWidget(m_gameLauncher);
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.cvar_editor)
                .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        m_gameLauncher.SetCmdText(widget.DumpCommand(m_gameLauncher.GetCmdText()));
                    }
                })
                .setNegativeButton(R.string.remove, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        m_gameLauncher.SetCmdText(widget.RemoveCommand(m_gameLauncher.GetCmdText()));
                    }
                })
                .setNeutralButton(R.string.reset, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which)
                    {
                        m_gameLauncher.SetCmdText(widget.ResetCommand(m_gameLauncher.GetCmdText()));
                    }
                })
        ;
        widget.SetGame(m_game);
        widget.RestoreCommand(m_gameLauncher.GetCmdText());
        builder.setView(widget);
        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
