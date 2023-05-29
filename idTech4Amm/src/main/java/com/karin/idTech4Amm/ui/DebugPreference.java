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
import com.n0n3m4.q3e.Q3EJNI;

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
        findPreference(Constants.PreferenceKey.NO_HANDLE_SIGNALS).setOnPreferenceChangeListener(this);
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
        Context activity;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M)
            activity = getContext();
        else
            activity = getActivity();
        final SharedPreferences mPrefs = PreferenceManager.getDefaultSharedPreferences(activity);
        String text = mPrefs.getString(Constants.CONST_PREFERENCE_APP_CRASH_INFO, null);
        AlertDialog.Builder builder = ContextUtility.CreateMessageDialogBuilder(activity, "Last crash info", text != null ? text : "None");
        if(text != null)
        {
            builder.setNeutralButton("Clear", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int id)
                    {
                        mPrefs.edit().remove(Constants.CONST_PREFERENCE_APP_CRASH_INFO).commit();
                        dialog.dismiss();
                    }
                });
        }
        builder.create().show();
    }
    
    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue)
    {
        String key = preference.getKey();
        if(Constants.PreferenceKey.NO_HANDLE_SIGNALS.equals(key))
        {
            Q3EJNI.SetNoHandleSignals((boolean)newValue);
            return true;
        }
        else
            return false;
    }
}
