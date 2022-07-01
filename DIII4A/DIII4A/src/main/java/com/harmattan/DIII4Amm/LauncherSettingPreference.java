package com.harmattan.DIII4Amm;
import android.preference.PreferenceFragment;
import android.os.Bundle;
import android.preference.PreferenceScreen;
import android.preference.Preference;

import com.n0n3m4.q3e.Q3EUtils;
import java.util.Set;

import com.n0n3m4.DIII4A.R;

public class LauncherSettingPreference extends PreferenceFragment implements Preference.OnPreferenceChangeListener
{

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.launcher_settings_preference);
        findPreference(Constants.PreferenceKey.LAUNCHER_ORIENTATION).setOnPreferenceChangeListener(this);
        findPreference(Constants.PreferenceKey.MAP_BACK).setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference)
    {
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }
    
    public boolean onPreferenceChange(Preference preference, Object newValue)
    {
        String key = preference.getKey();
        if(Constants.PreferenceKey.LAUNCHER_ORIENTATION.equals(key))
        {
            int o = (boolean)newValue ? 0 : 1;
            ContextUtility.SetScreenOrientation(getActivity(), o);
            return true;
        }
        else if(Constants.PreferenceKey.MAP_BACK.equals(key))
        {
            Set<String> values = (Set<String>)newValue;
            int r = 0;
            for(String str : values)
            {
                r |= Integer.parseInt(str);
            }
            preference.getSharedPreferences().edit().putInt(Q3EUtils.pref_harm_mapBack, r).commit();
            return true;
        }
        else
            return false;
    }
}
