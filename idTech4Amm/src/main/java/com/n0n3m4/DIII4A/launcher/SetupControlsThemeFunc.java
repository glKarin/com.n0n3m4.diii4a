package com.n0n3m4.DIII4A.launcher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Spinner;

import com.karin.idTech4Amm.R;
import com.karin.idTech4Amm.ui.ControlsThemeAdapter;
import com.n0n3m4.DIII4A.GameLauncher;
import com.n0n3m4.q3e.Q3EPreference;
import com.n0n3m4.q3e.Q3EUtils;

import java.util.ArrayList;
import java.util.LinkedHashMap;

public final class SetupControlsThemeFunc extends GameLauncherFunc
{
    public SetupControlsThemeFunc(GameLauncher gameLauncher)
    {
        super(gameLauncher);
    }

    public void Reset()
    {
    }

    public void Start(Bundle data)
    {
        super.Start(data);
        Reset();

        run();
    }

    public void run()
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(m_gameLauncher);
        builder.setTitle(R.string.controls_theme);
        View widget = m_gameLauncher.getLayoutInflater().inflate(R.layout.controls_theme_dialog, null, false);
        LinkedHashMap<String, String> schemes = Q3EUtils.GetControlsThemes(m_gameLauncher);

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(m_gameLauncher);
        String type = preferences.getString(Q3EPreference.CONTROLS_THEME, "");
        if(null == type)
            type = "";
        String[] theme = { type };
        ArrayList<String> types = new ArrayList<>(schemes.keySet());

        Spinner spinner = widget.findViewById(R.id.controls_theme_spinner);
        final ArrayAdapter<String> typeAdapter = new ArrayAdapter<>(m_gameLauncher, android.R.layout.simple_spinner_dropdown_item, new ArrayList<>(schemes.values()));
        typeAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spinner.setAdapter(typeAdapter);
        spinner.setSelection(types.indexOf(theme[0]));
        ListView list = widget.findViewById(R.id.controls_theme_list);
        ControlsThemeAdapter adapter = new ControlsThemeAdapter(widget.getContext());
        list.setAdapter(adapter);
        adapter.Update(theme[0]);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id)
            {
                theme[0] = types.get(position);
                adapter.Update(theme[0]);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent)
            {
            }
        });

        builder.setView(widget);
        builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                preferences.edit().putString(Q3EPreference.CONTROLS_THEME, theme[0]).commit();
            }
        })
                .setNegativeButton(R.string.cancel, null);
        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
