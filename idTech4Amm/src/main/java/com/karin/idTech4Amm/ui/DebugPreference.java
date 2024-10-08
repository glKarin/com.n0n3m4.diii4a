package com.karin.idTech4Amm.ui;
import android.content.Intent;
import android.os.Process;
import android.preference.PreferenceFragment;
import android.os.Bundle;
import android.preference.PreferenceScreen;
import android.preference.Preference;
import android.content.SharedPreferences;
import android.content.Context;
import android.preference.PreferenceManager;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.karin.idTech4Amm.LogcatActivity;
import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.misc.TextHelper;
import com.karin.idTech4Amm.sys.Constants;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KUncaughtExceptionHandler;

import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

/**
 * Debug preference fragment
 */
public class DebugPreference extends PreferenceFragment implements Preference.OnPreferenceChangeListener
{

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.debug_preference);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference)
    {
        String key = preference.getKey();
        if("last_dalvik_crash_info".equals(key))
        {
            OpenCrashInfo();
        }
        else if("get_pid".equals(key))
        {
            GetPID();
        }
        else if("open_documentsui".equals(key))
        {
            OpenDocumentsUI();
        }
        else if("open_logcat".equals(key))
        {
            OpenLogcat();
        }
        else if("show_preference".equals(key))
        {
            ShowPreference();
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    private void OpenCrashInfo()
    {
        Context activity = ContextUtility.GetContext(this);
        String text = KUncaughtExceptionHandler.GetDumpExceptionContent();
        AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(activity, Q3ELang.tr(activity, R.string.last_crash_info), text != null ? text : Q3ELang.tr(activity, R.string.none));
        if(text != null)
        {
            builder.setNeutralButton(R.string.clear, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int id)
                {
                    KUncaughtExceptionHandler.ClearDumpExceptionContent();
                    dialog.dismiss();
                }
            });
            if(Constants.IsDebug())
            {
                builder.setNegativeButton("Trigger", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int id)
                    {
                        throw new RuntimeException("Manuel trigger exception for testing");
                    }
                });
            }
        }
        builder.create().show();
    }

    private void GetPID()
    {
        Context activity = ContextUtility.GetContext(this);
        final String PID = "" + Process.myPid();
        Toast.makeText(activity, getString(R.string.application_pid) + PID, Toast.LENGTH_LONG).show();
        Q3EUtils.CopyToClipboard(activity, PID);
    }

    private void OpenDocumentsUI()
    {
        Context activity = ContextUtility.GetContext(this);
        try
        {
            ContextUtility.OpenDocumentsUI(activity);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            Toast.makeText(activity, e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }

    private void OpenLogcat()
    {
        Context activity = ContextUtility.GetContext(this);
        activity.startActivity(new Intent(activity, LogcatActivity.class));
    }

    private void ShowPreference()
    {
        Context context = getContext();
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
                            ShowPreference();
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
                                    ShowPreference();
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

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue)
    {
        return true;
    }
}
