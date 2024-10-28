package com.karin.idTech4Amm.ui;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.view.View;
import android.widget.Toast;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.n0n3m4.q3e.Q3EUtils;

import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

public class SharedPreferenceViewer implements Runnable
{
    private final Context context;

    public SharedPreferenceViewer(Context context)
    {
        this.context = context;
    }

    public void run()
    {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        Map<String, ?> map = new TreeMap<>(preferences.getAll());

        final int size = map.size();
        final CharSequence[] items = new CharSequence[size];
        final String[] values = new String[size];
        final boolean[] selected = new boolean[size];
        final Set<String> result = new HashSet<>();

        int i = 0;
        for (Map.Entry<String, ?> entry : map.entrySet())
        {
            String sb = (i + 1) + ". " + entry.getKey()
                    // + ": " + entry.getValue()
                    ;
            items[i] = sb;
            values[i] = entry.getKey();
            i++;
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle("Shared preferences (" + i + ")");
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
                alert.getButton(AlertDialog.BUTTON_NEUTRAL).setEnabled(result.size() > 0);
                alert.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(result.size() == 1);
            }
        });
        builder.setNegativeButton(R.string.cancel, null);
        builder.setNeutralButton(R.string.remove, null);
        builder.setPositiveButton(R.string.view, null);
        AlertDialog dialog = builder.create();
        dialog.setOnShowListener(new DialogInterface.OnShowListener() {
            @Override
            public void onShow(DialogInterface d)
            {
                dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view)
                    {
                        if(result.size() > 0)
                        {
                            dialog.dismiss();
                            SharedPreferences.Editor editor = preferences.edit();
                            for (String cmd : result)
                            {
                                editor.remove(cmd);
                            }
                            editor.commit();
                            run();
                        }
                        else
                        {
                            Toast.makeText(context, "Must choose a key", Toast.LENGTH_SHORT).show();
                        }
                    }
                });
                dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view)
                    {
                        dialog.dismiss();
                    }
                });
                dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view)
                    {
                        if(result.size() == 1)
                        {
                            dialog.dismiss();
                            String key = result.iterator().next();
                            String value = "" + map.get(key);
                            AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(context, key, value);
                            builder.setPositiveButton(R.string.copy, new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which)
                                {
                                    Q3EUtils.CopyToClipboard(context, value);
                                    Toast.makeText(context, R.string.success, Toast.LENGTH_SHORT).show();
                                    dialog.dismiss();
                                }
                            });
                            builder.setNegativeButton(R.string.close, new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which)
                                {
                                    dialog.dismiss();
                                }
                            });
                            builder.setNeutralButton(R.string.remove, new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which)
                                {
                                    preferences.edit().remove(key).commit();
                                    dialog.dismiss();
                                }
                            });
                            builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                                @Override
                                public void onDismiss(DialogInterface dialog)
                                {
                                    run();
                                }
                            });
                            AlertDialog dialog = builder.create();
                            dialog.show();
                        }
                        else
                        {
                            Toast.makeText(context, "Must choose a key", Toast.LENGTH_SHORT).show();
                        }
                    }
                });

                dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setEnabled(result.size() > 0);
                dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(result.size() == 1);
            }
        });
        dialog.show();
    }
}
