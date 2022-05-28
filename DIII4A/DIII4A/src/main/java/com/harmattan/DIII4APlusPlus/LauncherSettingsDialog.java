package com.harmattan.DIII4APlusPlus;
import android.app.DialogFragment;
import android.os.Bundle;
import android.app.FragmentManager;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.view.View;
import android.app.Dialog;
import com.n0n3m4.DIII4A.R;

public class LauncherSettingsDialog extends DialogFragment
{

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
    {
        View view = inflater.inflate(R.layout.launcher_settings_dialog, container);

        FragmentManager manager = getChildFragmentManager();
        manager.beginTransaction()
            .add(R.id.launcher_settings_dialog_main_layout, new LauncherSettingPreference(), "_Settings_preference")
            .commit()
            ;
        return view;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState)
    {
        Dialog dialog = super.onCreateDialog(savedInstanceState);
        dialog.setTitle("Settings");
        return dialog;
    }
    
    public static LauncherSettingsDialog newInstance()
    {
        LauncherSettingsDialog dialog = new LauncherSettingsDialog();
        return dialog;
    }
}
