package com.karin.idTech4Amm.ui;
import android.content.SharedPreferences;
import android.preference.EditTextPreference;
import android.preference.PreferenceFragment;
import android.os.Bundle;
import android.preference.PreferenceScreen;
import android.preference.Preference;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.sys.Constants;
import com.n0n3m4.q3e.Q3EUtils;
import java.util.Set;
import com.n0n3m4.q3e.Q3EJNI;

import android.util.Log;
import android.view.View;

/**
 * Launcher preference fragment
 */
public class LauncherSettingPreference extends PreferenceFragment implements Preference.OnPreferenceChangeListener
{

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.launcher_settings_preference);
        findPreference(Constants.PreferenceKey.LAUNCHER_ORIENTATION).setOnPreferenceChangeListener(this);
        findPreference(Constants.PreferenceKey.MAP_BACK).setOnPreferenceChangeListener(this);
        findPreference(Constants.PreferenceKey.REDIRECT_OUTPUT_TO_FILE).setOnPreferenceChangeListener(this);
        findPreference(Constants.PreferenceKey.HIDE_AD_BAR).setOnPreferenceChangeListener(this);
        findPreference(Constants.PreferenceKey.CONTROLS_CONFIG_POSITION_UNIT).setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference)
    {
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }
    
    @Override
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
        else if(Constants.PreferenceKey.HIDE_AD_BAR.equals(key))
        {
            boolean b = (boolean)newValue;
            View view = getActivity().findViewById(R.id.main_ad_layout);
            if(null != view)
                view.setVisibility(b ? View.GONE : View.VISIBLE);
            return true;
        }
        else if(Constants.PreferenceKey.REDIRECT_OUTPUT_TO_FILE.equals(key))
        {
			Q3EJNI.SetRedirectOutputToFile((boolean)newValue);
            return true;
        }
        else if(Constants.PreferenceKey.CONTROLS_CONFIG_POSITION_UNIT.equals(key))
        {
            int i;
            try
            {
                i = Integer.parseInt((String)newValue);
                if(i >= 0)
                    return true;
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
            return false;
        }
        else
            return false;
    }
}
