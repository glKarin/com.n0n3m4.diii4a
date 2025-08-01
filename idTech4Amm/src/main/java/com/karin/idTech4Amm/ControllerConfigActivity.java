package com.karin.idTech4Amm;

import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;

import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.sys.PreferenceKey;
import com.karin.idTech4Amm.sys.Theme;
import com.karin.idTech4Amm.ui.ControllerConfigPreference;
import com.n0n3m4.q3e.Q3ELang;

/**
 * controller config page
 */
public class ControllerConfigActivity extends PreferenceActivity
{
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Q3ELang.Locale(this);

        boolean o = PreferenceManager.getDefaultSharedPreferences(this).getBoolean(PreferenceKey.LAUNCHER_ORIENTATION, false);
        ContextUtility.SetScreenOrientation(this, o ? 0 : 1);

        Theme.SetTheme(this, false);

        getFragmentManager().beginTransaction().replace(android.R.id.content, new ControllerConfigPreference()).commit();
    }
}
