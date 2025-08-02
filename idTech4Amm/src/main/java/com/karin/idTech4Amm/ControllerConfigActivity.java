package com.karin.idTech4Amm;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.Toast;

import com.karin.idTech4Amm.lib.ContextUtility;
import com.karin.idTech4Amm.sys.PreferenceKey;
import com.karin.idTech4Amm.sys.Theme;
import com.karin.idTech4Amm.ui.ControllerConfigPreference;
import com.n0n3m4.q3e.Q3EInterface;
import com.n0n3m4.q3e.Q3EKeyCodes;
import com.n0n3m4.q3e.Q3ELang;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;

/**
 * controller config page
 */
public class ControllerConfigActivity extends PreferenceActivity
{
    private ControllerConfigPreference m_preference;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        Q3ELang.Locale(this);

        boolean o = PreferenceManager.getDefaultSharedPreferences(this).getBoolean(PreferenceKey.LAUNCHER_ORIENTATION, false);
        ContextUtility.SetScreenOrientation(this, o ? 0 : 1);

        Theme.SetTheme(this, false);

        m_preference = new ControllerConfigPreference();
        getFragmentManager().beginTransaction().replace(android.R.id.content, m_preference).commit();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.controller_config_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        int itemId = item.getItemId();

        if (itemId == R.id.controller_config_reset)
        {
            RestoreConfig();
        }
        else
        {
            return super.onOptionsItemSelected(item);
        }
        return true;
    }

    public void RestoreConfig()
    {
        ContextUtility.Confirm(this, Q3ELang.tr(this, R.string.warning), Q3ELang.tr(this, R.string.reset_controller_config), new Runnable() {
            public void run()
            {
                m_preference.Reset();
                m_preference = new ControllerConfigPreference();
                getFragmentManager().beginTransaction().replace(android.R.id.content, m_preference).commit();
                Toast.makeText(ControllerConfigActivity.this, R.string.controller_configure_has_reset, Toast.LENGTH_SHORT).show();
            }
        }, null, null, null);
    }
}
