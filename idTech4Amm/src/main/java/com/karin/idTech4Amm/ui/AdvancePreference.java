package com.karin.idTech4Amm.ui;
import android.preference.PreferenceFragment;
import android.os.Bundle;

import com.karin.idTech4Amm.R;

/**
 * Advance preference fragment
 */
public class AdvancePreference extends PreferenceFragment
{

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.advance_preference);
    }
}
