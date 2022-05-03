package com.harmattan.DIII4APlusPlus;
import android.preference.PreferenceFragment;
import android.os.Bundle;
import android.preference.PreferenceScreen;
import android.preference.Preference;
import android.content.SharedPreferences;
import com.n0n3m4.q3e.Q3EUtils;
import android.content.Context;
import android.preference.PreferenceManager;
import android.app.AlertDialog;
import android.content.DialogInterface;
import com.n0n3m4.DIII4A.R;

public class DebugPreference extends PreferenceFragment
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
        Context activity = getContext();
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
}
