package com.karin.idTech4Amm.ui;
import android.preference.PreferenceFragment;
import android.os.Bundle;
import android.preference.PreferenceScreen;
import android.preference.Preference;
import android.content.SharedPreferences;
import android.content.Context;
import android.preference.PreferenceManager;
import android.app.AlertDialog;
import android.content.DialogInterface;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.sys.Constants;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.karin.KUncaughtExceptionHandler;

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

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue)
    {
        return true;
    }
}
