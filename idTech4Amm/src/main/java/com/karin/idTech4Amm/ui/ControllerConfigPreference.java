package com.karin.idTech4Amm.ui;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;

import com.karin.idTech4Amm.ControllerConfigActivity;
import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.widget.SelectPreference;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
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

        Setup();
    }

    private void Setup()
    {
        findPreference(Q3EPreference.pref_harm_dpad_as_arrow_key).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.pref_harm_left_joystick_deadzone).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.pref_harm_right_joystick_deadzone).setOnPreferenceChangeListener(this);
        findPreference(Q3EPreference.pref_harm_right_joystick_sensitivity).setOnPreferenceChangeListener(this);

        Preference preference;

        for (String button : Q3EKeyCodes.CONTROLLER_BUTTONS) {
            preference = findPreference(Q3EPreference.pref_harm_gamepad_keymap + "_" + button);
            preference.setOnPreferenceChangeListener(this);
        }

        SetupGamePadButtons();
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference)
    {
        String key = preference.getKey();
        switch (key)
        {
            case "harm_controller_reset":
                ((ControllerConfigActivity)getActivity()).RestoreConfig();
                return true;
        }
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
                if(key.startsWith(Q3EPreference.pref_harm_gamepad_keymap))
                {
                    SetupGamePadButton(key, newValue);
                }
                return true;
        }
    }

    private void SetupGamePadButton(String button, Object newValue)
    {
        if(null == newValue)
            return;
        button = button.substring(Q3EPreference.pref_harm_gamepad_keymap.length() + 1);
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(getActivity());
        Set<String> codeSet = new HashSet<>(preferences.getStringSet(Q3EPreference.pref_harm_gamepad_keymap, new HashSet<>()));
        Set<String> rmList = new HashSet<>();
        for (String s : codeSet) {
            String b = s.split(":")[0];
            if(button.equalsIgnoreCase(b))
            {
                rmList.add(s);
                //break;
            }
        }
        codeSet.removeAll(rmList);
        codeSet.add(button  + ":" + newValue);
        preferences.edit().putStringSet(Q3EPreference.pref_harm_gamepad_keymap, codeSet).commit();
    }

    private void SetupGamePadButtons()
    {
        SelectPreference preference;
        Integer code;
        Integer defCode;

        Map<String, Integer> codeMap = Q3EKeyCodes.LoadGamePadButtonCodeMap(getActivity());

        for (String button : Q3EKeyCodes.CONTROLLER_BUTTONS) {
            preference = (SelectPreference)findPreference(Q3EPreference.pref_harm_gamepad_keymap + "_" + button);
            defCode = Q3EKeyCodes.GetDefaultGamePadButtonCode(button);
            if(null != defCode)
            {
                preference.setDefaultValue(defCode);
            }
            code = codeMap.get(button);
            if(null != code)
            {
                preference.setValue(code);
            }
            else if(null != defCode)
            {
                preference.setValue(defCode);
            }
        }
    }

    public void Reset()
    {
        SharedPreferences.Editor edit = PreferenceManager.getDefaultSharedPreferences(getActivity()).edit();
        edit.putBoolean(Q3EPreference.pref_harm_dpad_as_arrow_key, false);
        edit.putString(Q3EPreference.pref_harm_left_joystick_deadzone, "0.01");
        edit.putString(Q3EPreference.pref_harm_right_joystick_deadzone, "0");
        edit.putString(Q3EPreference.pref_harm_right_joystick_sensitivity, "1");

        edit.remove(Q3EPreference.pref_harm_gamepad_keymap);

        SetupGamePadButtons();

        edit.commit();
    }
}
