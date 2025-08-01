package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.karin.KStr;

import java.util.Set;

/**
 * Controller preference fragment
 */
public class ControllerConfigPreference extends PreferenceFragment implements Preference.OnPreferenceChangeListener
{

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.controller_preference);

        findPreference(Q3EPreference.pref_harm_dpad_as_arrow_key).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.pref_harm_left_joystick_deadzone).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.pref_harm_right_joystick_deadzone).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.pref_harm_right_joystick_sensitivity).setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference)
    {
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    private boolean CheckFloat(Object newValue, Float min, Float max)
    {
        if(null == newValue)
            return false;
        String str = (String) newValue;
        if(KStr.IsBlank(str))
            return false;
        float f;
        try
        {
            f = Float.parseFloat(str);
            if(null != min && f < min)
                return false;
            if(null != max && f > max)
                return false;
            return true;
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        return false;
    }
    
    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue)
    {
        String key = preference.getKey();
        Context context = ContextUtility.GetContext(this);
        switch (key)
        {
            case Q3EPreference.pref_harm_left_joystick_deadzone:
            case Q3EPreference.pref_harm_right_joystick_deadzone:
            {
                return CheckFloat(newValue, 0.0f, 1.0f);
            }
            case Q3EPreference.pref_harm_right_joystick_sensitivity:
            {
                return CheckFloat(newValue, 0.1f, 10.0f);
            }
            default:
                return true;
        }
    }
}
