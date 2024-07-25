package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.View;

import com.karin.idTech4Amm.R;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.karin.KStr;

import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Set;

public final class ChooseCommandRecordFunc extends GameLauncherFunc
{
    private String m_cmd;
    private String m_key;

    public ChooseCommandRecordFunc(GameLauncher gameLauncher, Runnable runnable)
    {
        super(gameLauncher, runnable);
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();
        m_cmd = data.getString("command");
        m_key = data.getString("key");
        run();
    }

    public void run()
    {
        Set<String> records = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher).getStringSet(m_key, new HashSet<>());

        final int size = records.size();
        final CharSequence[] items = new CharSequence[size];
        final String[] values = new String[size];
        final boolean[] selected = new boolean[size];
        final Set<String> result = new HashSet<>();

        int i = 0;
        for (String f : records)
        {
            items[i] = f;
            values[i] = f;
            selected[i] = f.equals(m_cmd);
            if(selected[i])
                result.add(f);
            i++;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.choose_command_record);
        builder.setMultiChoiceItems(items, selected, new DialogInterface.OnMultiChoiceClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which, boolean isChecked)
            {
                String cmd = values[which];
                if(isChecked)
                    result.add(cmd);
                else
                    result.remove(cmd);
            }
        });
        builder.setNegativeButton(R.string.remove, null);
        builder.setNeutralButton(R.string.add, null);
        builder.setPositiveButton(R.string.choose, null);
        AlertDialog dialog = builder.create();
        dialog.setOnShowListener(new DialogInterface.OnShowListener()
        {
            @Override
            public void onShow(DialogInterface d)
            {
                dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view)
                    {
                        if(Add(m_cmd) > 0)
                        {
                            dialog.dismiss();
                            run();
                        }
                    }
                });
                dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view)
                    {
                        if(Remove(result) > 0)
                        {
                            dialog.dismiss();
                            run();
                        }
                    }
                });
                dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view)
                    {
                        String cmd = Choose(result);
                        if(null != cmd)
                        {
                            SetResult(cmd);
                            Callback();
                            dialog.dismiss();
                        }
                    }
                });
            }
        });
        dialog.show();
    }

    private int Add(String cmd)
    {
        if(KStr.IsBlank(cmd))
            return 0;
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        Set<String> records = new LinkedHashSet<>(preferences.getStringSet(m_key, new HashSet<>()));
        if(records.contains(cmd))
            return 0;
        records.add(cmd);
        preferences.edit().putStringSet(m_key, records).commit();
        return 1;
    }

    private int Remove(Set<String> cmds)
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        Set<String> records = new LinkedHashSet<>(preferences.getStringSet(m_key, new HashSet<>()));
        int res = 0;
        for (String cmd : cmds)
        {
            records.remove(cmd);
            res++;
        }
        preferences.edit().putStringSet(m_key, records).commit();
        return res;
    }

    private String Choose(Set<String> cmds)
    {
        if(cmds.size() != 1)
        {
            Toast_short(Q3ELang.tr(m_gameLauncher, R.string.must_choose_a_command));
            return null;
        }
        return cmds.iterator().next();
    }
}
