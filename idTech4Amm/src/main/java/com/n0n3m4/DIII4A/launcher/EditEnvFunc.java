package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;

import com.karin.idTech4Amm.R;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.karin.KStr;

import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Set;

public final class EditEnvFunc extends GameLauncherFunc
{
    private String m_key;

    public EditEnvFunc(GameLauncher gameLauncher)
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

    private String FormatCommand(String cmd)
    {
        if(KStr.IsEmpty(cmd))
            return "";
        return cmd;
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
            items[i] = FormatCommand(f);
            values[i] = f;
            i++;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.env);
        builder.setMultiChoiceItems(items, selected, new DialogInterface.OnMultiChoiceClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which, boolean isChecked)
            {
                String cmd = values[which];
                if (isChecked)
                    result.add(cmd);
                else
                    result.remove(cmd);

                AlertDialog alert = (AlertDialog) dialog;
                alert.getButton(AlertDialog.BUTTON_NEUTRAL).setEnabled(result.size() == 1);
                alert.getButton(AlertDialog.BUTTON_NEGATIVE).setEnabled(!result.isEmpty());
            }
        }).setNegativeButton(R.string.remove, null);
        builder.setNeutralButton(R.string.edit, null);
        builder.setPositiveButton(R.string.add, null);
        AlertDialog dialog = builder.create();

        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface d)
            {
                dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view)
                    {
                        int csize = result.size();
                        if(csize == 1)
                        {
                            dialog.dismiss();
                            Edit(result);
                        }
                        else
                        {
                            Toast_short(R.string.must_choose_a_env);
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
                        dialog.dismiss();
                        Add();
                    }
                });

                dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setEnabled(result.size() == 1);
                dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setEnabled(!result.isEmpty());
            }
        });
        dialog.show();
    }

    private void Edit(Set<String> envs)
    {
        if(envs.size() != 1)
        {
            Toast_short(R.string.must_choose_a_env);
            return;
        }
        final String env = envs.iterator().next();
        EditEnv(env);
    }

    private void Add()
    {
        EditEnv(null);
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

    private void EditEnv(String env)
    {
        String[] envs = KStr.ParseEnv(env);

        View view = LayoutInflater.from(m_gameLauncher).inflate(R.layout.env_editor, null, false);

        EditText nameEdit = view.findViewById(R.id.env_editor_name);
        EditText valueEdit = view.findViewById(R.id.env_editor_value);

        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.edit_env);
        builder.setView(view);
        builder.setPositiveButton(R.string.save, null);
        if(null != envs)
        {
            nameEdit.setText(envs[0]);
            valueEdit.setText(envs[1]);
            builder.setNeutralButton(R.string.reset, null);
            builder.setNegativeButton(R.string.remove, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which)
                {
                    Remove(new LinkedHashSet<>(Collections.singletonList(env)));
                    dialog.dismiss();
                    run();
                }
            });
        }
        else
            builder.setNegativeButton(R.string.cancel, null);
        AlertDialog dialog = builder.create();

        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface d)
            {
                dialog.getButton(DialogInterface.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view)
                    {
                        String name = nameEdit.getText().toString();
                        if(KStr.IsBlank(name))
                        {
                            Toast_short(R.string.name_must_not_empty);
                            return;
                        }
                        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
                        Set<String> envList = new LinkedHashSet<>(preferences.getStringSet(m_key, new HashSet<>()));
                        Set<String> editList = new LinkedHashSet<>();
                        for (String e : envList)
                        {
                            String[] es = KStr.ParseEnv(e);
                            if(null == es)
                                continue;
                            if(name.equals(es[0]))
                                continue;
                            editList.add(e);
                        }
                        String value = valueEdit.getText().toString();
                        String e = KStr.MakeEnv(name, value);
                        editList.add(e);
                        preferences.edit().putStringSet(m_key, editList).commit();
                        dialog.dismiss();
                    }
                });
                if(null != envs)
                {
                    dialog.getButton(DialogInterface.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view)
                        {
                            nameEdit.setText(envs[0]);
                            valueEdit.setText(envs[1]);
                        }
                    });
                }
            }
        });

        dialog.setOnDismissListener(new DialogInterface.OnDismissListener()
        {
            @Override
            public void onDismiss(DialogInterface dialog)
            {
                run();
            }
        });
        dialog.show();
    }
}
