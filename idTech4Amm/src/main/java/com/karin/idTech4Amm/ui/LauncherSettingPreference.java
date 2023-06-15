package com.karin.idTech4Amm.ui;
import android.preference.PreferenceFragment;
import android.os.Bundle;
import android.preference.PreferenceScreen;
import android.preference.Preference;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.sys.Constants;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import java.util.Set;
import com.n0n3m4.q3e.Q3EJNI;

import android.view.View;
import android.widget.Toast;

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
        findPreference(Q3EPreference.REDIRECT_OUTPUT_TO_FILE).setOnPreferenceChangeListener(this);
        findPreference(Constants.PreferenceKey.HIDE_AD_BAR).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.CONTROLS_CONFIG_POSITION_UNIT).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.pref_harm_function_key_toolbar_y).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.LANG).setOnPreferenceChangeListener(this);
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
        switch (key)
        {
            case Constants.PreferenceKey.LAUNCHER_ORIENTATION:
                int o = (boolean) newValue ? 0 : 1;
                ContextUtility.SetScreenOrientation(getActivity(), o);
                return true;
            case Constants.PreferenceKey.MAP_BACK:
            {
                Set<String> values = (Set<String>) newValue;
                int r = 0;
                for (String str : values)
                {
                    r |= Integer.parseInt(str);
                }
                preference.getSharedPreferences().edit().putInt(Q3EPreference.pref_harm_mapBack, r).commit();
                return true;
            }
            case Constants.PreferenceKey.HIDE_AD_BAR:
                boolean b = (boolean) newValue;
                View view = getActivity().findViewById(R.id.main_ad_layout);
                if (null != view)
                    view.setVisibility(b ? View.GONE : View.VISIBLE);
                return true;
            case Q3EPreference.REDIRECT_OUTPUT_TO_FILE:
                Q3EJNI.SetRedirectOutputToFile((boolean) newValue);
                return true;
            case Q3EPreference.CONTROLS_CONFIG_POSITION_UNIT:
            {
                int i;
                try
                {
                    i = Integer.parseInt((String) newValue);
                    if (i >= 0 && i % 5 == 0)
                        return true;
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                }
                return false;
            }
            case Q3EPreference.pref_harm_function_key_toolbar_y:
            {
                int i;
                try
                {
                    i = Integer.parseInt((String) newValue);
                    if (i >= 0)
                        return true;
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                }
                return false;
            }
            case Q3EPreference.LANG:
            {
                Toast.makeText(ContextUtility.GetContext(this), R.string.be_available_on_reboot_the_next_time, Toast.LENGTH_SHORT).show();
                return true;
            }
            default:
                return false;
        }
    }
}
